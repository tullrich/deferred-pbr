#pragma once
#include "common.h"

#include "particles.h"

#define SCENE_EMITTERS_MAX 256

typedef struct
{
	vec3 position;
	vec3 color;
	float intensity;
} PointLight;

typedef struct
{
	// position
	vec3 pos;

	// xyz-euler orientation
	vec3 rot;

	// camera boom length
	float boomLen;

	// view matrix
	mat4x4 view;

	// projection matrix
	mat4x4 proj;

	// view-projection matrix
	mat4x4 viewProj;
} Camera;

typedef struct
{
	// skybox cubemap
	GLuint env_cubemap;

	// irradiance map (pre-convolved env_cubemap)
	GLuint irr_cubemap;
} Skybox;

typedef enum
{
	BOX,
	SPHERE,
	NONE
} GeometryMode;

typedef struct
{
	// main camera values
	Camera camera;

	// Scene skybox
	Skybox skybox;

	// global lighting environment
	vec3 ambient_color;
	float ambient_intensity;

	// The selected geometry to render
	GeometryMode geo_mode;

	// main light
	PointLight main_light;

	// active particle emitters
	ParticleEmitter* emitters[SCENE_EMITTERS_MAX];
} Scene;

void scene_camera_forward(const Scene* s, vec3 out);
void scene_camera_up(const Scene* s, vec3 out);
