#pragma once

#include "common.h"
#include "scene.h"

typedef struct
{
	GLuint program;

	// attributes
	GLint vert_loc;
	GLint trans_loc;
	GLint rot_loc;
	GLint scale_loc;
	GLint uv_loc;
	GLint color_loc;

	// uniforms
	GLint modelviewproj_loc;
	GLint texture_loc;
} ParticleShader;

typedef struct
{
	ParticleShader particle_shader_flat;
	ParticleShader particle_shader_textured;
} Forward;

int forward_initialize(Forward* f);
void forward_render(Forward* f, Scene *s);
