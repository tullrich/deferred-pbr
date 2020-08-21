#pragma once
#include "common.h"

#define ENUM_LightType(D)											\
	D(LIGHT_TYPE_POINT, 			"Point")					\
	D(LIGHT_TYPE_DIRECTIONAL, "Directional")

DECLARE_ENUM(LightType, light_type_strings, ENUM_LightType);

typedef struct
{
	LightType type;
	vec4 position;
	vec3 rot;
	vec3 color;
	float intensity;
} Light;

void light_initialize_point(Light *out, const vec3 pos, const vec3 color, float intensity);
void light_initialize_directional(Light *out, const vec3 pos, const vec3 color, float intensity);
void light_gui(Light *light);
