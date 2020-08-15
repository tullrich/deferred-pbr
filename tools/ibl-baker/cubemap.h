#pragma once
#include "ibl-baker.h"

typedef enum
{
	CUBEMAP_FRONT,
	CUBEMAP_BACK,
	CUBEMAP_TOP,
	CUBEMAP_BOTTOM,
	CUBEMAP_LEFT,
	CUBEMAP_RIGHT
} CubeMapFaces;

int load_dir_as_faces(const char* input_dir, const char* ext, FIBITMAP *out[6]);
gli::texture_cube convert_faces_to_cubemap(FIBITMAP **faces);
void free_dir_faces(FIBITMAP *faces[6]);

int save_cubemap(gli::texture_cube& cube, const char* out_path);
gli::texture_cube load_cubemap(const char* path);
