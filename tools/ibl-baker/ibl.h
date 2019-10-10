#pragma once
#include "FreeImage.h"
#include "linmath.h"

typedef enum
{
	CUBEMAP_FRONT,
	CUBEMAP_BACK,
	CUBEMAP_TOP,
	CUBEMAP_BOTTOM,
	CUBEMAP_LEFT,
	CUBEMAP_RIGHT
} CubeMapFaces;

// Image-based lighting functions
typedef struct
{
	mat4x4 inv_proj;
	mat4x4 inv_viewproj[6];
	FIBITMAP **cubemap;
	unsigned int height, width;
} IrradianceCompute;

int ibl_compute_irradiance_map(const char** filepaths);
