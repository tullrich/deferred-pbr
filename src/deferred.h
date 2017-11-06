#pragma once

#include "common.h"
#include "gbuffer.h"
#include "scene.h"

typedef struct
{
	GLuint program;

	// shader vars
	GLint pos_loc;
	GLint texcoord_loc;

	GLint env_map_loc;

	GLint inv_vp_loc;
	GLint camera_pos_loc;
} SkyboxShader;

typedef struct
{
	GLuint program;

	// shader vars
	GLint pos_loc;
	GLint normal_loc;
	GLint tangent_loc;
	GLint texcoord_loc;

	GLint diffuse_map_loc;
	GLint normal_map_loc;
	GLint specular_map_loc;
	GLint ao_map_loc;

	GLint modelview_loc;
	GLint invTModelview_loc;
	GLint view_loc;
} CubeShader;

typedef struct
{
	GLuint program;

	// shader vars
	GLint pos_loc;
	GLint texcoord_loc;

	union
	{
		struct
		{
			GLint gbuffer_position_loc;
			GLint gbuffer_normal_loc;
			GLint gbuffer_diffuse_loc;
			GLint gbuffer_specular_loc;
			GLint gbuffer_depth_loc;
		};
		GLint gbuffer_locs[GBUFFER_ATTACHMENTS_COUNT];
	};

	// shader vars
	GLint ambient_term_loc;
	GLint light_pos_loc;
	GLint light_color_loc;
	GLint light_intensity_loc;
	GLint eye_pos_loc;

} LightingShader;

typedef struct
{
	GLuint program;

	// shader vars
	GLint pos_loc;
	GLint texcoord_loc;
	GLint gbuffer_render_loc;
	GLint gbuffer_depth_loc;

} DebugShader;

typedef struct
{
	RenderMode render_mode;
	SkyboxShader skybox_shader;
	CubeShader cube_shader;
	LightingShader lighting_shader;
	DebugShader debug_shader;
	GBuffer g_buffer;

	// texture handles
	GLuint cube_diffuse_map;
	GLuint cube_normal_map;
	GLuint cube_specular_map;
	GLuint cube_ao_map;
} Deferred;

int deferred_initialize(Deferred* d);
void deferred_render(Deferred* d, Scene *s);
