#pragma once
#include "common.h"

const vec3 Gravity = { 0.0f, -50.0f, 0.0f };

#define PHYSICS_PRINT_ENABLE 0
#if PHYSICS_PRINT_ENABLE
# define PHYSICS_PRINT(format, ...) printf(format, __VA_ARGS__);
#else
# define PHYSICS_PRINT(format, ...)
#endif
