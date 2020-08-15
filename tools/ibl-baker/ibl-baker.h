#pragma once

#include <stdio.h>
#include <cstring>

#include "linmath.h"
#include "FreeImage.h"
#include "gli/gli.hpp"
#include "stb_image.h"

static inline void vec4_zero(vec4 r) {
	r[0] = r[1] = r[2] = r[3] = 0.0f;
}

static inline void mat4x4_zero(mat4x4 m) {
	for (int i=0; i<4; i++)
		vec4_zero(m[i]);
}

FREE_IMAGE_FORMAT getFreeimageFormat(const char *filepath);
