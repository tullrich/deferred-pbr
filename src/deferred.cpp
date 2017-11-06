#include "deferred.h"

int deferred_initialize(Deferred* d)
{
	memset(d, 0, sizeof(Deferred));
	d->render_mode = RENDER_MODE_SHADED;

	// Initialzie the skybox shader
	if(!(d->skybox_shader.program = utility_create_program("shaders/skybox.vert", "shaders/skybox.frag"))) {
		printf("Unable to load shader\n");
		return 1;
	}
	d->skybox_shader.pos_loc = glGetAttribLocation(d->skybox_shader.program, "position");
	d->skybox_shader.texcoord_loc = glGetAttribLocation(d->skybox_shader.program, "texcoord");
	d->skybox_shader.env_map_loc = glGetUniformLocation(d->skybox_shader.program, "SkyboxCube");
	d->skybox_shader.inv_vp_loc = glGetUniformLocation(d->skybox_shader.program, "InvViewProj");

	// Initialize box shader
	if(!(d->cube_shader.program = utility_create_program("shaders/box.vert", "shaders/box.frag"))) {
		printf("Unable to load shader\n");
		return 1;
	}

	d->cube_shader.pos_loc = glGetAttribLocation(d->cube_shader.program, "position");
	d->cube_shader.normal_loc = glGetAttribLocation(d->cube_shader.program, "normal");
	d->cube_shader.tangent_loc = glGetAttribLocation(d->cube_shader.program, "tangent");
	d->cube_shader.texcoord_loc = glGetAttribLocation(d->cube_shader.program, "texcoord");
	d->cube_shader.modelview_loc = glGetUniformLocation(d->cube_shader.program, "ModelView");
	d->cube_shader.invTModelview_loc = glGetUniformLocation(d->cube_shader.program, "invTModelView");
	d->cube_shader.view_loc = glGetUniformLocation(d->cube_shader.program, "ModelViewProj");
	d->cube_shader.diffuse_map_loc = glGetUniformLocation(d->cube_shader.program, "DiffuseMap");
	d->cube_shader.normal_map_loc = glGetUniformLocation(d->cube_shader.program, "NormalMap");
	d->cube_shader.specular_map_loc = glGetUniformLocation(d->cube_shader.program, "SpecularMap");
	d->cube_shader.ao_map_loc = glGetUniformLocation(d->cube_shader.program, "AOMap");

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
	d->lighting_shader.gbuffer_diffuse_loc = glGetUniformLocation(d->lighting_shader.program, "GBuffer_Diffuse");
	d->lighting_shader.gbuffer_specular_loc = glGetUniformLocation(d->lighting_shader.program, "GBuffer_Specular");
	d->lighting_shader.gbuffer_depth_loc = glGetUniformLocation(d->lighting_shader.program, "GBuffer_Depth");
	d->lighting_shader.env_map_loc = glGetUniformLocation(d->lighting_shader.program, "EnvCubemap");
	d->lighting_shader.inv_view_loc = glGetUniformLocation(d->lighting_shader.program, "InvView");

	if(gbuffer_initialize(&d->g_buffer)) {
		printf("Unable to create g-buffer.\n");
		return 1;
	}

	// Initialize passthrough
	if(!(d->debug_shader.program = utility_create_program("shaders/passthrough.vert", "shaders/passthrough.frag"))) {
		printf("Unable to load shader\n");
		return 1;
	}
	d->debug_shader.pos_loc = glGetAttribLocation(d->debug_shader.program, "position");
	d->debug_shader.texcoord_loc = glGetAttribLocation(d->debug_shader.program, "texcoord");
	d->debug_shader.gbuffer_render_loc = glGetUniformLocation(d->debug_shader.program, "GBuffer_Render");
	d->debug_shader.gbuffer_depth_loc = glGetUniformLocation(d->debug_shader.program, "GBuffer_Depth");

	if(!(d->cube_diffuse_map = utility_load_image(GL_TEXTURE_2D, "images/Medievil/Medievil Stonework - Color Map.png"))) {
	//if(!(d->cube_diffuse_map = utility_load_image(GL_TEXTURE_2D, "images/SciFiCube/Sci_Wall_Panel_01_basecolor.jpeg"))) {
		d->cube_diffuse_map = utility_load_texture_unknown();
	}

	if(!(d->cube_normal_map = utility_load_image(GL_TEXTURE_2D, "images/Medievil/Medievil Stonework - (Normal Map).png"))) {
	//if(!(d->cube_normal_map = utility_load_image(GL_TEXTURE_2D, "images/SciFiCube/Sci_Wall_Panel_01_normal.jpeg"))) {
		d->cube_normal_map = utility_load_texture_unknown();
	}

	if(!(d->cube_specular_map = utility_load_image(GL_TEXTURE_2D, "images/Medievil/Medievil Stonework - Specular Map.png"))) {
		d->cube_specular_map = utility_load_texture_unknown();
	}

	if(!(d->cube_ao_map = utility_load_image(GL_TEXTURE_2D, "images/Medievil/Medievil Stonework - AO Map.png"))) {
		d->cube_ao_map = utility_load_texture_unknown();
	}

	utility_sphere_tessellate(&d->sphere, 1.0f, 100, 100);
	return 0;
}

static void render_geometry(Deferred* d, Scene *s)
{
	gbuffer_bind(&d->g_buffer);

	utility_set_clear_color(0, 0, 0);
	glClearDepth(1.0f);
	glDisable(GL_BLEND);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (s->geo_mode == GeometryMode::NONE)
		return;

	glUseProgram(d->cube_shader.program);

	// bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, d->cube_diffuse_map);
	glUniform1i(d->cube_shader.diffuse_map_loc, 0);

	// bind normal map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, d->cube_normal_map);
	glUniform1i(d->cube_shader.normal_map_loc, 1);

	// bind normal map
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, d->cube_specular_map);
	glUniform1i(d->cube_shader.specular_map_loc, 2);

	// bind normal map
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, d->cube_ao_map);
	glUniform1i(d->cube_shader.ao_map_loc, 3);

	// calc model matrix
	mat4x4 model;
	mat4x4_identity(model);
	mat4x4_scale_aniso(model, model, 5.0f, 5.0f, 5.0f);

	// bind model-view matrix
	mat4x4 mv;
	mat4x4_mul(mv, s->camera.view, model);
	glUniformMatrix4fv(d->cube_shader.modelview_loc, 1, GL_FALSE, (const GLfloat*)mv);

	// bind inverse transpose model-view matrix
	mat4x4 inv_mv, invT_mv;
	mat4x4_invert(inv_mv, mv);
	mat4x4_transpose(invT_mv, inv_mv);
	glUniformMatrix4fv(d->cube_shader.invTModelview_loc, 1, GL_FALSE, (const GLfloat*)invT_mv);

	// bind model-view-projection matrix
	mat4x4 mvp;
	mat4x4_mul(mvp, s->camera.viewProj, model);
	glUniformMatrix4fv(d->cube_shader.view_loc, 1, GL_FALSE, (const GLfloat*)mvp);

	switch (s->geo_mode) {
		case GeometryMode::SPHERE:
			utility_sphere_draw(&d->sphere,
				d->cube_shader.texcoord_loc,
				d->cube_shader.normal_loc,
				d->cube_shader.tangent_loc,
				d->cube_shader.pos_loc );
			break;
		case GeometryMode::BOX: // intentional fallthrough
		default:
			utility_draw_cube(
				d->cube_shader.texcoord_loc,
				d->cube_shader.normal_loc,
				d->cube_shader.tangent_loc,
				d->cube_shader.pos_loc,
				-0.5f, 0.5f );
			break;
	}
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
	glBindTexture(GL_TEXTURE_CUBE_MAP, s->skybox.env_cubemap);
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

	utility_draw_fullscreen_quad(d->skybox_shader.texcoord_loc, d->skybox_shader.pos_loc);
}

static void render_debug(Deferred *d, Scene *s)
{
	glUseProgram(d->debug_shader.program);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);
	glDisable(GL_BLEND);

	GLuint render_buffer = 0;
	switch(d->render_mode) {
		case RENDER_MODE_POSITION: 	render_buffer = d->g_buffer.position_render_buffer; break;
		case RENDER_MODE_DIFFUSE: 	render_buffer = d->g_buffer.diffuse_render_buffer; break;
		case RENDER_MODE_NORMAL: 	render_buffer = d->g_buffer.normal_render_buffer; break;
		case RENDER_MODE_SPECULAR: 	render_buffer = d->g_buffer.specular_render_buffer; break;
		case RENDER_MODE_DEPTH: 	render_buffer = d->g_buffer.depth_render_buffer; break;
	}

	// bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, render_buffer);
	glUniform1i(d->debug_shader.gbuffer_render_loc, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, d->g_buffer.depth_render_buffer);
	glUniform1i(d->debug_shader.gbuffer_depth_loc, 1);

	utility_draw_fullscreen_quad(d->debug_shader.texcoord_loc, d->debug_shader.pos_loc);
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
