#pragma once
#include "common.h"

#include "particles.h"
#include "mesh.h"
#include "material.h"
#include "skybox.h"

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

	// camera field of view
	float fovy;

	// view matrix
	mat4x4 view;

	// projection matrix
	mat4x4 proj;

	// view-projection matrix
	mat4x4 viewProj;
} Camera;

typedef struct
{
	// main camera values
	Camera camera;

	// Scene skybox
	Skybox skybox;

	// global lighting environment
	vec3 ambient_color;
	float ambient_intensity;

	// The selected mesh to render
	Mesh mesh;

	// The material to render
	Material material;

	// Model translation
	vec3 model_translation;

	// Model euler angle rotation
	vec3 model_rot;

	// Model uniform scale
	float model_scale;

	// main light
	PointLight main_light;

	// active particle emitters
	ParticleEmitter* emitters[SCENE_EMITTERS_MAX];
} Scene;

void scene_camera_forward(const Scene* s, vec3 out);
void scene_camera_up(const Scene* s, vec3 out);
