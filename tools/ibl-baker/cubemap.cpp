#include "cubemap.h"
#include "ibl-baker.h"

#include "gli/gli.hpp"

static void free_faces(FIBITMAP *faces[6]) {
	for (int i = 0; i < 6; i++) {
		FreeImage_Unload(faces[i]);
	}
}

static int load_faces(const char** filepaths, FIBITMAP *out[6]) {
	int width, height;

	// Load cubemap
	for (int i = 0; i < 6; i++) {
		int img_width, img_height, components;

		FREE_IMAGE_FORMAT format = getFreeimageFormat(filepaths[i]);
		if (format == FIF_UNKNOWN || !(out[i] = FreeImage_Load(format, filepaths[i], 0))) {
			printf("Error loading stb image '%s' with error: %s\n", filepaths[i], stbi_failure_reason());
			free_faces(out);
			return 1;
		}

		img_width = FreeImage_GetWidth(out[i]);
		img_height = FreeImage_GetHeight(out[i]);
		components = FreeImage_GetBPP(out[i])/8;

		if (i == 0) {
			width = img_width;
			height = img_height;
			if (!width || !height || components != 3) {
				printf("Bad cubemap face size/components'%s': size <%i,%i> components %i\n", filepaths[i], width, height, components);
				free_faces(out);
				return 1;
			}
		} else {
			if (img_width != width && img_height != height) {
				printf("Cubemap face size/components mismatch '%s'\n", filepaths[i]);
				free_faces(out);
				return 1;
			}
		}

		printf("Read face '%s'\n", filepaths[i]);
	}

	return 0;
}

int files_to_cubemap(const char** filepaths, const char* out_path) {
	FIBITMAP *faces[6] = { NULL };
	if (load_faces(filepaths, faces)) {
		printf("Error saving dds '%s'\n", out_path);
		return 1;
	}

	printf("All faces read successful\n");

	int width = FreeImage_GetWidth(faces[0]);
	int height = FreeImage_GetHeight(faces[0]);

	gli::texture_cube texture(gli::FORMAT_RGB_DXT1_UNORM_BLOCK8, gli::extent2d(width, height));

	if (!gli::save_dds(texture, out_path)) {
		printf("Error saving dds '%s'\n", out_path);
		free_faces(faces);
		return 1;
	}

	printf("Wrote cubemap '%s'\n", out_path);

	free_faces(faces);
	return 0;
}
