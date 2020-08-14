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
			if (!width || !height) {
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
        cube.store(gli::extent2d(x, y), i, cube.base_level(), glm::u8vec3(pixel[FI_RGBA_RED], pixel[FI_RGBA_GREEN], pixel[FI_RGBA_BLUE]));
        pixel += components;
      }
      bits += pitch;
    }
  }

	if (!gli::save_dds(cube, out_path)) {
		printf("Error saving dds '%s'\n", out_path);
		free_faces(faces);
		return 1;
	}

	printf("Wrote cubemap '%s'\n", out_path);

	free_faces(faces);
	return 0;
}
