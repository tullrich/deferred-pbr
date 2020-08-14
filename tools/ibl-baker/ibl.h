#pragma once
#include "ibl-baker.h"
#include "cubemap.h"

// Image-based lighting functions
typedef struct
{
	mat4x4 inv_proj;
	mat4x4 inv_viewproj[6];
	FIBITMAP **cubemap;
	unsigned int height, width;
} IrradianceCompute;

int ibl_compute_irradiance_map(const char** filepaths);
