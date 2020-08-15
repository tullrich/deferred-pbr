#include "cubemap.h"
#include "ibl-baker.h"

#include "gli/gli.hpp"
#include "gli/convert.hpp"

static void dirToCubeFaces(const char* input_dir, const char* ext, char ** out_paths) {
		const char* faces[6] = { "posx.", "negx.", "posy.", "negy.", "posz.", "negz." };
		int input_len = strlen(input_dir);
		bool append_slash = input_dir[input_len-1] != '/';
		int face_len = input_len + 8 + ((append_slash) ? 1 : 0);
		for (int i = 0; i < 6; i++) {
			char* face_path = (char*)malloc(face_len);
			strcpy(face_path, input_dir);
			if (append_slash) {
				strcat(face_path, "/");
			}
			strcat(face_path, faces[i]);
			strcat(face_path, ext);
			out_paths[i] = face_path;
		}
}

void free_dir_faces(FIBITMAP *faces[6]) {
	for (int i = 0; i < 6; i++) {
		FreeImage_Unload(faces[i]);
	}
}

int load_dir_as_faces(const char* input_dir, const char* ext, FIBITMAP *out[6]) {
	int width, height;

	char *filepaths[6];
	dirToCubeFaces(input_dir, ext, filepaths);

	// Load cubemap
	for (int i = 0; i < 6; i++) {
		int img_width, img_height, components;

		FREE_IMAGE_FORMAT format = getFreeimageFormat(filepaths[i]);
		if (format == FIF_UNKNOWN || !(out[i] = FreeImage_Load(format, filepaths[i], 0))) {
			printf("Error loading stb image '%s' with error: %s\n", filepaths[i], stbi_failure_reason());
			free_dir_faces(out);
			return 1;
		}

		img_width = FreeImage_GetWidth(out[i]);
		img_height = FreeImage_GetHeight(out[i]);
		components = FreeImage_GetBPP(out[i])/8;

		if (i == 0) {
			width = img_width;
			height = img_height;
			if (!width || !height) {
				printf("Bad cubemap face size/components'%s': size <%i,%i> components %i\n", filepaths[i], width, height, components);
				free_dir_faces(out);
				return 1;
			}
		} else {
			if (img_width != width && img_height != height) {
				printf("Cubemap face size/components mismatch '%s'\n", filepaths[i]);
				free_dir_faces(out);
				return 1;
			}
		}

    if (img_width > 512) {
      FIBITMAP *tmp = FreeImage_Rescale(out[i], 512, 512, FILTER_BILINEAR);
      FreeImage_Unload(out[i]);
      out[i] = tmp;
    }

		printf("Read face '%s'\n", filepaths[i]);
	}

	for (int i = 0; i < 6; i++) {
		free(filepaths[i]);
	}

	return 0;
}

gli::texture_cube convert_faces_to_cubemap(FIBITMAP **faces) {
	int width = FreeImage_GetWidth(faces[0]);
	int height = FreeImage_GetHeight(faces[0]);
  int components = FreeImage_GetBPP(faces[0])/8;
	gli::texture_cube cube(gli::FORMAT_RGB8_UNORM_PACK8, gli::extent2d(width, height), 1);
  cube.clear(glm::u8vec3(0, 255, 0));
  printf("texture_cube: lods=%i layers=%i faces=%i extents=(%i, %i)\n", cube.levels(), cube.layers(), cube.faces(), cube.extent().x, cube.extent().y);
  for (int i = 0; i < 6; i++) {
    BYTE *bits = (BYTE*)FreeImage_GetBits(faces[i]);
    unsigned pitch = FreeImage_GetPitch(faces[i]);
    printf("FreeImage %i: %i x %i components %i pitch %i\n", i, width, height, components, pitch);
    for (int y = height-1; y >= 0; y--) {
      BYTE *pixel = (BYTE*)bits;
      for (int x = 0; x < width; x++) {
        cube.store(gli::extent2d(x, y), i, cube.base_level(), glm::u8vec3(pixel[FI_RGBA_RED], pixel[FI_RGBA_BLUE], pixel[FI_RGBA_GREEN]));
        // cube.store(gli::extent2d(x, y), i, cube.base_level(), glm::u8vec3(pixel[FI_RGBA_RED], pixel[FI_RGBA_GREEN], pixel[FI_RGBA_BLUE]));
        pixel += components;
      }
      bits += pitch;
    }
  }
  // gli::texture compressed = gli::convert(cube, gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16);
	return cube;
}

int save_cubemap(gli::texture_cube& cube, const char* out_path) {
	if (!gli::save_dds(cube, out_path)) {
		printf("Error saving dds '%s'\n", out_path);
		return 1;
	}
	printf("Wrote cubemap '%s'\n", out_path);
	return 0;
}


gli::texture_cube load_cubemap(const char* path) {
  gli::texture tex = gli::load_dds(path);
  return gli::texture_cube(tex);
}
