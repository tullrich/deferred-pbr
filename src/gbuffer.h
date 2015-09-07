#pragma once
#include "common.h"

typedef struct
{
	// frame buffer
	GLuint fbo;

	// depth buffer
	GLuint depth_render_buffer;

	GLuint rendered_texture;
	GLuint rendered_texture2;
} GBuffer;


int gbuffer_initialize(GBuffer *g_buffer);
void gbuffer_bind(GBuffer *g_buffer);