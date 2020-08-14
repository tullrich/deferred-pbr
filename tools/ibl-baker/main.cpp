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

static void dirToCubeFaces(const char* input_dir, char ** out_paths) {
		const char* faces[6] = { "posz.jpg", "negz.jpg", "posy.jpg", "negy.jpg", "posx.jpg", "negx.jpg" };
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
			out_paths[i] = face_path;
		}
}

static void print_usage() {
  printf("Usage: IBLBaker --mode=<irradiance|prefiltered|cubemap> <input_file> <output_file>\n");
}

static int bake_irradiance_map(const char* input_dir, const char* output_file) {
	printf("Baking irradiance map '%s' from '%s'\n", output_file, input_dir);
	char *face_paths[6];
	dirToCubeFaces(input_dir, face_paths);
	int ret = ibl_compute_irradiance_map((const char**)face_paths);

	for (int i = 0; i < 6; i++) {
		free(face_paths[i]);
	}
	return ret;
}

static int bake_prefiltered_env_map(const char* input_dir, const char* output_file) {
	printf("Baking prefiltered env map '%s' from '%s'\n", output_file, input_dir);
	return 0;
}

static int merge_cubemap(const char* input_dir, const char* output_file) {
	printf("Merging files in directory '%s' into cubemap '%s'\n", input_dir, output_file);
	char *face_paths[6];
	dirToCubeFaces(input_dir, face_paths);
	int ret = files_to_cubemap((const char**)face_paths, output_file);
	for (int i = 0; i < 6; i++) {
		free(face_paths[i]);
	}
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
