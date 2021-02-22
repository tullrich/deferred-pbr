#pragma once
#include "common.h"

const vec3 Gravity = { 0.0f, -15.0f, 0.0f };
#define MAX_CONTACTS 512
#define MAX_RESOLVE_ITERATIONS (1024 * 10)
#define COLLISION_FUDGE 0.01f

#define PHYSICS_PRINT_ENABLE 0
#if PHYSICS_PRINT_ENABLE
# define VA_ARGS(...) , ##__VA_ARGS__
# define PHYSICS_PRINT(format, ...) printf(format VA_ARGS(__VA_ARGS__));
#else
# define PHYSICS_PRINT(format, ...)
#endif
