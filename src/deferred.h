#pragma once

#include "gbuffer.h"
#include "common.h"

typedef struct
{
	vec3 rot;
	mat4x4 camera;
} Scene;

typedef struct
{
	GLuint program;

	// shader vars
	GLint pos_loc;
	GLint texcoord_loc;
	GLint tex_loc;
	GLint view_loc;

	GBuffer g_buffer;

	// texture handles
	GLuint default_texture;

} Deferred;

int deferred_initialize(Deferred* d);
void deferred_render(Deferred* d, Scene *s);