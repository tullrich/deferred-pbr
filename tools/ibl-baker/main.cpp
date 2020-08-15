#include "ibl.h"
#include "cubemap.h"

#include <stdio.h>
#include <cstdlib>
#include <cstring>

typedef enum
{
	BAKE_IRRADIANCE_MAP,
  BAKE_PREFILTERED_ENV_MAP,
	MERGE_CUBEMAP
} IBLBakerMode;

static bool prefix(const char *pre, const char *str) {
  return strncmp(pre, str, strlen(pre)) == 0;
}

static void print_usage() {
  printf("Usage: IBLBaker --mode=<irradiance|prefiltered|cubemap> <input_file> <output_file>\n");
}

static int bake_irradiance_map(const char* input_file, const char* output_file) {
	printf("Baking irradiance map '%s' from '%s'\n", output_file, input_file);
  gli::texture_cube env_map = load_cubemap(input_file);
  printf("loaded env cubemap successfully\n");
	return ibl_compute_irradiance_map(env_map, output_file);
}

static int bake_prefiltered_env_map(const char* input_file, const char* output_file) {
	printf("Baking prefiltered env map '%s' from '%s'\n", output_file, input_file);
	return 0;
}

static int merge_cubemap(const char* input_dir, const char* output_file) {
	printf("Merging files in directory '%s' into cubemap '%s'\n", input_dir, output_file);
	FIBITMAP *faces[6] = { NULL };
	if (load_dir_as_faces(input_dir, "jpg", faces)) {
    if (load_dir_as_faces(input_dir, "png", faces)) {
  		printf("Error reading input files '%s'\n", input_dir);
  		return 1;
  	}
	}
  gli::texture_cube cubemap = convert_faces_to_cubemap(faces);
	int ret = save_cubemap(cubemap, output_file);
  free_dir_faces(faces);
	return ret;
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
		print_usage();
    return 0;
  }

  IBLBakerMode mode = BAKE_IRRADIANCE_MAP;
  for (int i = 1; i < (argc-2); i++) {
    char* cur = argv[i];
    const char* mode_prefix = "--mode=";
    if (prefix(mode_prefix, cur)) {
      const char* mode_str = cur + strlen(mode_prefix);
      if (!strcmp(mode_str, "irradiance")) {
        mode = BAKE_IRRADIANCE_MAP;
      } else if (!strcmp(mode_str, "prefiltered")) {
        mode = BAKE_PREFILTERED_ENV_MAP;
			} else if (!strcmp(mode_str, "cubemap")) {
        mode = MERGE_CUBEMAP;
      } else {
				printf("Unknown mode: %s\n", mode_str);
				print_usage();
				return -1;
      }
    }
  }

  const char* input_dir = argv[argc-2];
  const char* output_file = argv[argc-1];
	switch (mode) {
		case BAKE_IRRADIANCE_MAP: return bake_irradiance_map(input_dir, output_file);
		case BAKE_PREFILTERED_ENV_MAP: return bake_prefiltered_env_map(input_dir, output_file);
		case MERGE_CUBEMAP: return merge_cubemap(input_dir, output_file);
	}

	return 0;
}
