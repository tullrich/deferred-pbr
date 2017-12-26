#pragma once
#include "common.h"

#define GBUFFER_ATTACHMENTS_COUNT 5

typedef struct
{
	// frame buffer
	GLuint fbo;

	// attachments
	union
	{
		struct
		{
			GLuint normal_render_buffer;
			GLuint albedo_render_buffer;
			GLuint roughness_render_buffer;
			GLuint metalness_render_buffer;
			GLuint depth_render_buffer;
		};
		GLint attachments[GBUFFER_ATTACHMENTS_COUNT];
	};
} GBuffer;


int gbuffer_initialize(GBuffer *g_buffer);
void gbuffer_bind(GBuffer *g_buffer);
