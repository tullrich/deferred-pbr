#include "ibl.h"

static FREE_IMAGE_FORMAT getFreeimageFormat(const char *filepath) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	fif = FreeImage_GetFileType(filepath, 0);
	if (fif == FIF_UNKNOWN)
	{
		fif = FreeImage_GetFIFFromFilename(filepath);
	}
	if (fif != FIF_UNKNOWN && !FreeImage_FIFSupportsReading(fif))
	{
		return FIF_UNKNOWN;
	}

	return fif;
}


static void ir_face_view_mat4x4(mat4x4 out, CubeMapFaces face) {
	mat4x4_zero(out);
	out[3][3] = 1.0f;
	switch(face) {
	case CUBEMAP_FRONT: out[0][0] = out[1][1] = 1.0f; out[2][2] = -1.0f;  return;
	case CUBEMAP_BACK: out[0][0] = -1.0f; out[1][1] = out[2][2] = 1.0f;  return;

	case CUBEMAP_TOP: out[0][0] = 1.0f; out[2][1] = out[1][2] = -1.0f;  return;
	case CUBEMAP_BOTTOM: out[0][0] = out[1][2] = out[2][1] = 1.0f; return; //w

	case CUBEMAP_LEFT: out[1][1] = 1.0f; out[2][0] = out[0][2] = -1.0f; return; // w
	case CUBEMAP_RIGHT: out[1][1] = out[0][2] = out[2][0] = 1.0f; return; // w
	}
}

static void dir_for_fragment(vec3 out, int face, float u, float v, IrradianceCompute *ir) {
	vec4 view = {(float)u, (float)v, .0f, 1.0f};
	vec4 result;
	mat4x4_mul_vec4(result, ir->inv_viewproj[face], view);
	vec3_scale(out, result, 1.0f/result[3]); // divide by w
	vec3_norm(out, out);
}

static void sample_cubemap(vec3 out, const vec3 dir, FIBITMAP **cubemap, unsigned int width, unsigned int height ) {
	const int axes[6][3] = {
		{ 2, 0, 1 }, // CUBEMAP_FRONT +z
		{ 2, 0, 1 }, // CUBEMAP_BACK -z
		{ 1, 0, 2 }, // CUBEMAP_TOP +y
		{ 1, 0, 2 }, // CUBEMAP_BOTTOM -y
		{ 0, 2, 1 }, // CUBEMAP_LEFT +x
		{ 0, 2, 1 }  // CUBEMAP_RIGHT -x
	};
	const float signs[6][2] = {
		{ -1.0f, 1.0f },
		{ 1.0f, 1.0f },
		{ 1.0f, 1.0f },
		{ 1.0f, -1.0f },
		{ 1.0f, 1.0f },
		{ -1.0f, 1.0f }
	};
	int face;
	if (fabsf(dir[0]) > fabsf(dir[1])) {
		if (fabsf(dir[0]) > fabsf(dir[2])) {
			face = (dir[0] > 0.0f) ? CUBEMAP_LEFT : CUBEMAP_RIGHT;
		} else {
			face = (dir[2] > 0.0f) ? CUBEMAP_FRONT : CUBEMAP_BACK;
		}
	} else if (fabsf(dir[1]) > fabsf(dir[2])) {
		face = (dir[1] > 0.0f) ? CUBEMAP_TOP : CUBEMAP_BOTTOM;
	} else {
		face = (dir[2] > 0.0f) ? CUBEMAP_FRONT : CUBEMAP_BACK;
	}
	const int *axis = axes[face];
	const float *sign = signs[face];
	float u = (1.0f + sign[0]*dir[axis[1]]/fabsf(dir[axis[0]]))/2.0f;
	float v = (1.0f + sign[1]*dir[axis[2]]/fabsf(dir[axis[0]]))/2.0f;

	RGBQUAD value;
	FreeImage_GetPixelColor(cubemap[face], (unsigned int)(u*width), (unsigned int)(v*height), &value);
	out[0] = value.rgbRed / 255.0f;
	out[1] = value.rgbGreen / 255.0f;
	out[2] = value.rgbBlue / 255.0f;
}

static float area_element(float x, float y) {
	//http://www.rorydriscoll.com/2012/01/15/cubemap-texel-solid-angle/
	return atan2f(x * y, sqrtf(x * x + y * y + 1));
}

static float texcoord_solid_angle(float u, float v, float size) {
	float InvResolution = 1.0f / size;

	// U and V are the -1..1 texture coordinate on the current face.
	// Get projected area for this texel
	float x0 = u - InvResolution;
	float y0 = v - InvResolution;
	float x1 = u + InvResolution;
	float y1 = v + InvResolution;
	float SolidAngle = area_element(x0, y0) - area_element(x0, y1) - area_element(x1, y0) + area_element(x1, y1);

	return SolidAngle;
}

static void radiance_sample(vec3 result, vec3 eye_dir, int face, float su, float sv, float solid_angle, IrradianceCompute *ir) {
	vec3 ray;
	dir_for_fragment(ray, face, su, sv, ir);

	float lambert = fmaxf(vec3_mul_inner(ray, eye_dir), 0.0f);
	float term = lambert*solid_angle;

	vec3 sample;
	sample_cubemap(sample, ray, ir->cubemap, ir->width, ir->height);
	vec3_scale(sample, sample, term);
	vec3_add(result, result, sample);
}

static void compute_irradiance_fragment(vec3 out, int face, float u, float v, IrradianceCompute *ir) {
	vec3 eye_dir;
	dir_for_fragment(eye_dir, face, u , v, ir);

#if 1
	vec3 result = { 0.0f };
	for (unsigned int sv = 0; sv < ir->height; sv++) {
		for (unsigned int su = 0; su < ir->width; su++) {
			float ndc_su = 2.0f*(su+0.5f)/(float)ir->width - 1.0f;
			float ndc_sv = 2.0f*(sv+0.5f)/(float)ir->height - 1.0f;
			float solid_angle = texcoord_solid_angle(ndc_su, ndc_sv, (float)ir->height);
			radiance_sample(result, eye_dir, 0, ndc_su, ndc_sv, solid_angle, ir);
			radiance_sample(result, eye_dir, 1, ndc_su, ndc_sv, solid_angle, ir);
			radiance_sample(result, eye_dir, 2, ndc_su, ndc_sv, solid_angle, ir);
			radiance_sample(result, eye_dir, 3, ndc_su, ndc_sv, solid_angle, ir);
			radiance_sample(result, eye_dir, 4, ndc_su, ndc_sv, solid_angle, ir);
			radiance_sample(result, eye_dir, 5, ndc_su, ndc_sv, solid_angle, ir);
		}
	}
	vec3_scale(out, result, 1.0f/(float)M_PI);
	//vec3_scale(out, result, 1.0f/result[3]);
#else
	vec4 result ={0.0f};
#define RANGE_0_1(val) (.5f*val+0.5f)
	out[0] = RANGE_0_1(eye_dir[0]);
	out[1] = RANGE_0_1(eye_dir[1]);
	out[2] = RANGE_0_1(eye_dir[2]);
#endif
}

static int build_irradiance_out_filepath(char* out_filepath, size_t out_len, const char* filepath) {
	strncpy(out_filepath, filepath, out_len);
	if (out_filepath[out_len-1]) {
		return 1;  // no enough space
	}

	char* ext = strchr(out_filepath, '.');
	if (!ext) {
		return 1;  // no extension
	}
	strncpy(ext, "_irradiance.png", out_len - (size_t)(ext-out_filepath));
	return (out_filepath[out_len-1]) ? 1:0;
}

// convolve radiance map to an irradiance map and write out
int ibl_compute_irradiance_map(const char** filepaths) {
	int width, height;
	FIBITMAP *data[6] = { NULL };
	FIBITMAP *out_data[6] = { NULL };
	char out_filepath[512];

	// Load cubemap
	int ret = 0;
	for (int i = 0; i < 6; i++) {
		int img_width, img_height, components;

		FREE_IMAGE_FORMAT format = getFreeimageFormat(filepaths[i]);
		if (format == FIF_UNKNOWN || !(data[i] = FreeImage_Load(format, filepaths[i], 0))) {
			printf("Error loading stb image '%s' with error: %s\n", filepaths[i], stbi_failure_reason());
			ret = 1;
			goto free_and_exit;
		}

		img_width = FreeImage_GetWidth(data[i]);
		img_height = FreeImage_GetHeight(data[i]);
		components = FreeImage_GetBPP(data[i])/8;

		if (i == 0) {
			width = img_width;
			height = img_height;
			if (!width || !height || components != 3) {
				printf("Bad cubemap face size/components'%s': size <%i,%i> components %i\n", filepaths[i], width, height, components);
				ret = 1;
				goto free_and_exit;
			}
		} else {
			if (img_width != width && img_height != height) {
				printf("Cubemap face size/components mismatch '%s'\n", filepaths[i]);
				ret = 1;
				goto free_and_exit;
			}
		}

		// Allocate out image
		out_data[i] =  FreeImage_Allocate(width, height, 24);
	}

	// Prepare input
	IrradianceCompute ir;
	ir.cubemap = data;
	ir.height = height;
	ir.width = width;
	mat4x4 proj;
	mat4x4_perspective(proj, (float)M_PI/2.0f, width/(float)height, 1.0f, 2.0f);
	mat4x4_invert(ir.inv_proj, proj);
	for (int i = 0; i < 6; i++) {
		mat4x4 view, inv_view, viewproj;
		ir_face_view_mat4x4(view, (CubeMapFaces)i);
		mat4x4_invert(inv_view, view);
		mat4x4_mul(viewproj, proj, inv_view);
		mat4x4_invert(ir.inv_viewproj[i], viewproj);
	}

	// Convolve step
	for (int i = 0; i < 6; i++) {
		FIBITMAP* out_img = out_data[i];
		for (int v = 0; v < height; v++) {
			for (int u = 0; u < width; u++) {
				vec3 irr;
				compute_irradiance_fragment(irr, i, 2.0f*(u+0.5f)/(float)width - 1.0f, 2.0f*(v+0.5f)/(float)height - 1.0f, &ir);
				RGBQUAD value;
				value.rgbRed = (char)(255 * irr[0]);
				value.rgbGreen = (char)(255 * irr[1]);
				value.rgbBlue = (char)(255 * irr[2]);
				FreeImage_SetPixelColor(out_img, u, v, &value);
			}
			printf("Convolved pixel: %f\n", 100.0f * (v+1)/(float)height);
		}

		// Write output
		if (build_irradiance_out_filepath(out_filepath, 512, filepaths[i]) ||
			!FreeImage_Save(FIF_PNG, out_img, out_filepath, 0)) {
			printf("Error writing irradiance map for file '%s'\n", filepaths[i]);
			ret = 1;
			goto free_and_exit;
		}
		printf("Wrote irradiance map '%s'\n", out_filepath);
	}

free_and_exit:
	for (int i = 0; i < 6; i++) {
		FreeImage_Unload(data[i]);
		FreeImage_Unload(out_data[i]);
	}
	return ret;
}
