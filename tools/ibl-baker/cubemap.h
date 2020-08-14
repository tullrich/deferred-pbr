#pragma once

typedef enum
{
	CUBEMAP_FRONT,
	CUBEMAP_BACK,
	CUBEMAP_TOP,
	CUBEMAP_BOTTOM,
	CUBEMAP_LEFT,
	CUBEMAP_RIGHT
} CubeMapFaces;

int files_to_cubemap(const char** filepaths, const char* out_path);
