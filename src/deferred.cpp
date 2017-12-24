#include "deferred.h"

static int load_surface_shader(SurfaceShader* shader, const char* vert, const char* frag, const char** defines, int defines_count) {
	if(!(shader->program = utility_create_program_defines("shaders/mesh.vert", "shaders/mesh.frag",
																 defines, defines_count))) {
		return 1;
	}
	glBindFragDataLocation(shader->program, 0, "PositionOut");
	glBindFragDataLocation(shader->program, 1, "AlbedoOut");
	glBindFragDataLocation(shader->program, 2, "NormalOut");
	glBindFragDataLocation(shader->program, 3, "SpecularOut");
	glBindAttribLocation(shader->program, 0, "position");
	glBindAttribLocation(shader->program, 1, "normal");
	glBindAttribLocation(shader->program, 2, "tangent");
	glBindAttribLocation(shader->program, 3, "texcoord");
	if (utility_link_program(shader->program)) {
		return 1;
	}

	shader->pos_loc = glGetAttribLocation(shader->program, "position");
	shader->normal_loc = glGetAttribLocation(shader->program, "normal");
	shader->tangent_loc = glGetAttribLocation(shader->program, "tangent");
	shader->texcoord_loc = glGetAttribLocation(shader->program, "texcoord");
	shader->modelview_loc = glGetUniformLocation(shader->program, "ModelView");
	shader->invTModelview_loc = glGetUniformLocation(shader->program, "invTModelView");
	shader->view_loc = glGetUniformLocation(shader->program, "ModelViewProj");
	shader->albedo_base_loc = glGetUniformLocation(shader->program, "AlbedoBase");
	shader->specular_base_loc = glGetUniformLocation(shader->program, "SpecularBase");
	shader->ao_map_loc = glGetUniformLocation(shader->program, "AOMap");
	shader->albedo_map_loc = glGetUniformLocation(shader->program, "AlbedoOut");
	shader->normal_map_loc = glGetUniformLocation(shader->program, "NormalMap");
	shader->specular_map_loc = glGetUniformLocation(shader->program, "SpecularMap");
	shader->ao_map_loc = glGetUniformLocation(shader->program, "AOMap");
	return 0;
}

static int load_debug_shader(DebugShader* shader, const char* vert, const char* frag, const char** defines, int defines_count) {
	if (!(shader->program = utility_create_program_defines("shaders/passthrough.vert", "shaders/passthrough.frag", defines, defines_count))) {
		return 1;
	}
	shader->pos_loc = glGetAttribLocation(shader->program, "position");
	shader->texcoord_loc = glGetAttribLocation(shader->program, "texcoord");
	shader->gbuffer_render_loc = glGetUniformLocation(shader->program, "GBuffer_Render");
	shader->gbuffer_depth_loc = glGetUniformLocation(shader->program, "GBuffer_Depth");
	return 0;
}

int deferred_initialize(Deferred* d)
{
	memset(d, 0, sizeof(Deferred));
	d->render_mode = RENDER_MODE_SHADED;

	// Initialzie the skybox shader
	if(!(d->skybox_shader.program = utility_create_program("shaders/skybox.vert", "shaders/skybox.frag"))) {
		printf("Unable to load shader\n");
		return 1;
	}
	glBindAttribLocation( d->skybox_shader.program, 0, "position" );
	glBindAttribLocation( d->skybox_shader.program, 0, "texcoord" );
	if ( utility_link_program( d->skybox_shader.program ) ) {
		printf( "Unable to load shader\n" );
		return 1;
	}
	d->skybox_shader.pos_loc = glGetAttribLocation(d->skybox_shader.program, "position");
	d->skybox_shader.texcoord_loc = glGetAttribLocation(d->skybox_shader.program, "texcoord");
	d->skybox_shader.env_map_loc = glGetUniformLocation(d->skybox_shader.program, "SkyboxCube");
	d->skybox_shader.inv_vp_loc = glGetUniformLocation(d->skybox_shader.program, "InvViewProj");

	// Initialize box shader
	const char* uv_surf_shader_defines[] ={
		""
		//"#define MESH_VERTEX_UV1\n",
		//"#define USE_NORMAL_MAP\n",
		//"#define USE_ALBEDO_MAP\n",
		//"#define USE_SPECULAR_MAP\n",
		//"#define USE_AO_MAP\n",
	};
	if(load_surface_shader(&d->surf_shader[0], "shaders/mesh.vert", "shaders/mesh.frag",
							uv_surf_shader_defines, STATIC_ELEMENT_COUNT(uv_surf_shader_defines))) {
		printf("Unable to load shader\n");
		return 1;
	}
	if(load_surface_shader(&d->surf_shader[1], "shaders/mesh.vert", "shaders/mesh.frag",
							NULL, 0)) {
		printf("Unable to load shader\n");
		return 1;
	}

	// Initialize fullscreen quad shader
	if(!(d->lighting_shader.program = utility_create_program("shaders/passthrough.vert", "shaders/lighting.frag"))) {
		printf("Unable to load shader\n");
		return 1;
	}

	d->lighting_shader.pos_loc = glGetAttribLocation(d->lighting_shader.program, "position");
	d->lighting_shader.texcoord_loc = glGetAttribLocation(d->lighting_shader.program, "texcoord");
	d->lighting_shader.ambient_term_loc = glGetUniformLocation(d->lighting_shader.program, "AmbientTerm");
	d->lighting_shader.light_pos_loc = glGetUniformLocation(d->lighting_shader.program, "MainLightPosition");
	d->lighting_shader.light_color_loc = glGetUniformLocation(d->lighting_shader.program, "MainLightColor");
	d->lighting_shader.light_intensity_loc = glGetUniformLocation(d->lighting_shader.program, "MainLightIntensity");
	d->lighting_shader.eye_pos_loc = glGetUniformLocation(d->lighting_shader.program, "EyePosition");

	d->lighting_shader.gbuffer_position_loc = glGetUniformLocation(d->lighting_shader.program, "GBuffer_Position");
	d->lighting_shader.gbuffer_normal_loc = glGetUniformLocation(d->lighting_shader.program, "GBuffer_Normal");
	d->lighting_shader.gbuffer_albedo_loc = glGetUniformLocation(d->lighting_shader.program, "GBuffer_Albedo");
	d->lighting_shader.gbuffer_specular_loc = glGetUniformLocation(d->lighting_shader.program, "GBuffer_Specular");
	d->lighting_shader.gbuffer_depth_loc = glGetUniformLocation(d->lighting_shader.program, "GBuffer_Depth");
	d->lighting_shader.env_map_loc = glGetUniformLocation(d->lighting_shader.program, "EnvCubemap");
	d->lighting_shader.inv_view_loc = glGetUniformLocation(d->lighting_shader.program, "InvView");

	if(gbuffer_initialize(&d->g_buffer)) {
		printf("Unable to create g-buffer.\n");
		return 1;
	}

	// Initialize passthrough
	const char* debug_ndc_defines[] ={
		"#define DEBUG_RENDER_NORMALIZE\n"
	};
	if(load_debug_shader(&d->debug_shader[0], "shaders/passthrough.vert", "shaders/passthrough.frag", NULL, 0) ||
	   load_debug_shader(&d->debug_shader[1], "shaders/passthrough.vert", "shaders/passthrough.frag",
						  debug_ndc_defines, STATIC_ELEMENT_COUNT(debug_ndc_defines))) {
		printf("Unable to load debug shader\n");
		return 1;
	}

	return 0;
}

static void render_geometry(Deferred* d, Scene *s)
{
	gbuffer_bind(&d->g_buffer);

	utility_set_clear_color(0, 0, 0);
	glClearDepth(1.0f);
	glDisable(GL_BLEND);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!s->mesh.vertices)
		return;

	int shader_idx = (s->mesh.texcoords) ? 0:1;
	const SurfaceShader* shader = &d->surf_shader[shader_idx];
	glUseProgram(shader->program);

	// bind albedo base color
	glUniform3fv(shader->albedo_base_loc, 1, s->material.albedo_base);

	// bind specular base color
	glUniform3fv(shader->specular_base_loc, 1, s->material.specular_base);

	// bind albedo map
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, s->material.albedo_map);
	glUniform1i(shader->albedo_map_loc, 0);

	// bind normal map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, s->material.normal_map);
	glUniform1i(shader->normal_map_loc, 1);

	// bind specular map
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, s->material.specular_map);
	glUniform1i(shader->specular_map_loc, 2);

	// bind normal map
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, s->material.ao_map);
	glUniform1i(shader->ao_map_loc, 3);

	// calc model matrix
	mat4x4 model;
	mat4x4_identity(model);
	mat4x4_rotate_X(model, model, DEG_TO_RAD(s->model_rot[0]));
	mat4x4_rotate_Y(model, model, DEG_TO_RAD(s->model_rot[1]));
	mat4x4_rotate_Z(model, model, DEG_TO_RAD(s->model_rot[2]));
	mat4x4_scale_aniso(model, model, s->model_scale, s->model_scale, s->model_scale);
	vec3_sub(model[3], model[3], s->mesh.bounds.center);
	vec3_add(model[3], model[3], s->model_translation);

	// bind model-view matrix
	mat4x4 mv;
	mat4x4_mul(mv, s->camera.view, model);
	glUniformMatrix4fv(shader->modelview_loc, 1, GL_FALSE, (const GLfloat*)mv);

	// bind inverse transpose model-view matrix
	mat4x4 inv_mv, invT_mv;
	mat4x4_invert(inv_mv, mv);
	mat4x4_transpose(invT_mv, inv_mv);
	glUniformMatrix4fv(shader->invTModelview_loc, 1, GL_FALSE, (const GLfloat*)invT_mv);

	// bind model-view-projection matrix
	mat4x4 mvp;
	mat4x4_mul(mvp, s->camera.viewProj, model);
	glUniformMatrix4fv(shader->view_loc, 1, GL_FALSE, (const GLfloat*)mvp);

	mesh_draw(&s->mesh,
			  shader->texcoord_loc,
			  shader->normal_loc,
			  shader->tangent_loc,
			  shader->pos_loc);
}

static void render_shading(Deferred* d, Scene *s)
{
	glUseProgram(d->lighting_shader.program);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);

	// bind gbuffer
	int i;
	for(i = 0; i < GBUFFER_ATTACHMENTS_COUNT; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, d->g_buffer.attachments[i]);
		glUniform1i(d->lighting_shader.gbuffer_locs[i], i);
	}

	// bind env map
	glActiveTexture(GL_TEXTURE0+i);
	glBindTexture(GL_TEXTURE_CUBE_MAP, s->skybox.irr_cubemap);
	glUniform1i(d->lighting_shader.env_map_loc, i);

	// light setup
	vec4 view_light_pos_in;
	view_light_pos_in[0] = s->main_light.position[0];
	view_light_pos_in[1] = s->main_light.position[1];
	view_light_pos_in[2] = s->main_light.position[2];
	view_light_pos_in[3] = 1.0f;

	vec4 view_light_pos;
	mat4x4_mul_vec4(view_light_pos, s->camera.view, view_light_pos_in);

	vec3 ambient_term;
	vec3_scale(ambient_term, s->ambient_color, s->ambient_intensity);
	glUniform3fv(d->lighting_shader.ambient_term_loc, 1, (const GLfloat*)ambient_term );
	glUniform3fv(d->lighting_shader.light_pos_loc, 1, (const GLfloat*)view_light_pos);
	glUniform3fv(d->lighting_shader.light_color_loc, 1, (const GLfloat*)s->main_light.color);
	glUniform1f(d->lighting_shader.light_intensity_loc, s->main_light.intensity);

	// View rotation only
	mat4x4 view_rot, inv_view_rot;
	mat4x4_dup(view_rot, s->camera.view);
	vec3_zero(view_rot[3]);
	mat4x4_invert(inv_view_rot, view_rot);
	glUniformMatrix4fv(d->lighting_shader.inv_view_loc, 1, GL_FALSE, (const GLfloat*)inv_view_rot);

	utility_draw_fullscreen_quad(d->lighting_shader.texcoord_loc, d->lighting_shader.pos_loc);
}

static void render_skybox(Deferred *d, Scene *s)
{
	glUseProgram(d->skybox_shader.program);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);

	glDisable(GL_BLEND);

	// Bind environment map
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, s->skybox.env_cubemap);
	glUniform1i(d->skybox_shader.env_map_loc, 0);

	mat4x4 view_rot, vp, inv_vp;
	mat4x4_dup(view_rot, s->camera.view);
	vec3_zero(view_rot[3]);
	mat4x4_mul(vp, s->camera.proj, view_rot);
	mat4x4_invert(inv_vp, vp);

	glUniformMatrix4fv(d->skybox_shader.inv_vp_loc, 1, GL_FALSE, (const GLfloat*)inv_vp);

	utility_draw_fullscreen_quad2(d->skybox_shader.texcoord_loc, d->skybox_shader.pos_loc);
}

static void render_debug(Deferred *d, Scene *s)
{
	int program_idx = 0;;
	GLuint render_buffer = 0;
	switch(d->render_mode) {
		case RENDER_MODE_POSITION: 	render_buffer = d->g_buffer.position_render_buffer; break;
		case RENDER_MODE_ALBEDO: 	render_buffer = d->g_buffer.albedo_render_buffer; break;
		case RENDER_MODE_NORMAL: 	render_buffer = d->g_buffer.normal_render_buffer; program_idx = 1; break;
		case RENDER_MODE_SPECULAR: 	render_buffer = d->g_buffer.specular_render_buffer; break;
		case RENDER_MODE_DEPTH: 	render_buffer = d->g_buffer.depth_render_buffer; break;
	}

	glUseProgram(d->debug_shader[program_idx].program);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, render_buffer);
	glUniform1i(d->debug_shader[program_idx].gbuffer_render_loc, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, d->g_buffer.depth_render_buffer);
	glUniform1i(d->debug_shader[program_idx].gbuffer_depth_loc, 1);

	utility_draw_fullscreen_quad(d->debug_shader[program_idx].texcoord_loc, d->debug_shader[program_idx].pos_loc);
}

void deferred_render(Deferred *d, Scene *s)
{
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	render_geometry(d, s);

	if (d->render_mode == RENDER_MODE_SHADED) {
		render_shading(d, s);
		render_skybox(d, s);
	} else {
		render_debug(d, s);
	}

	GL_CHECK_ERROR();
}
