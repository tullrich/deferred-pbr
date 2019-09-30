#include "forward.h"

static void draw_quad(const ParticleShader* shader, const vec3 translation, const vec3 rot, const vec3 scale, const vec4 color) {
	glBegin(GL_QUADS);
		if (shader->trans_loc >= 0) glVertexAttrib3f(shader->trans_loc, translation[0], translation[1], translation[2]);
		if (shader->rot_loc >= 0) glVertexAttrib3f(shader->rot_loc, rot[0], rot[1], rot[2]);
		if (shader->scale_loc >= 0) glVertexAttrib3f(shader->scale_loc, scale[0], scale[1], scale[2]);
		if (shader->color_loc >= 0) glVertexAttrib4f(shader->color_loc, color[0], color[1], color[2], color[3]);
		if (shader->uv_loc >= 0) glVertexAttrib2f(shader->uv_loc, 1, 0);
		glVertexAttrib3f(shader->vert_loc, 0.5f, -0.5f, 0.0f);
		if (shader->uv_loc >= 0) glVertexAttrib2f(shader->uv_loc, 1, 1.0f);
		glVertexAttrib3f(shader->vert_loc, 0.5f, 0.5f, 0.0f);
		if (shader->uv_loc >= 0) glVertexAttrib2f(shader->uv_loc, 0, 1.0f);
		glVertexAttrib3f(shader->vert_loc, -0.5f, 0.5f, 0.0f);
		if (shader->uv_loc >= 0) glVertexAttrib2f(shader->uv_loc, 0, 0);
		glVertexAttrib3f(shader->vert_loc, -0.5f, -0.5f, 0.0f);
	glEnd();
}

static void calculate_billboard_euler(vec3 euler, mat4x4 model, const Scene *s) {
	vec4 cam_forward, cam_up;
	cam_forward[3] = cam_up[3] = 0.0f;
	scene_camera_forward(s, cam_forward);
	scene_camera_up(s, cam_up);

	mat4x4 invModel;
	mat4x4_invert(invModel, model);

	vec4 model_cam_forward, model_cam_up;
	mat4x4_mul_vec4(model_cam_forward, invModel, cam_forward);
	vec4_negate_in_place(model_cam_forward);
	mat4x4_mul_vec4(model_cam_up, invModel, cam_up);

	mat4x4 lookAt;
	vec3 zero;
	vec3_zero(zero);
	mat4x4_look_at(lookAt, zero, model_cam_forward, model_cam_up);

	vec3_zero(euler);
	mat4x4_to_euler(euler, lookAt);
}

static int load_particle_shader(ParticleShader* shader, const char* vert, const char* frag, const char** defines, int defines_count) {
	// initialize particle shader
	if (!(shader->program = utility_create_program_defines(vert, frag, defines, defines_count))) {
		printf("Unable to load shader [%s. %s]\n", vert, frag);
		return 1;
	}
	glBindAttribLocation(shader->program, 0, "vert");
	glBindAttribLocation(shader->program, 1, "translation");
	glBindAttribLocation(shader->program, 2, "rotation");
	glBindAttribLocation(shader->program, 3, "scale");
	glBindAttribLocation(shader->program, 4, "texcoord");
	glBindAttribLocation(shader->program, 5, "color");
	if (utility_link_program(shader->program)) {
		printf( "Unable to load shader\n" );
		return 1;
	}
	shader->vert_loc = glGetAttribLocation(shader->program, "vert");
	shader->trans_loc = glGetAttribLocation(shader->program, "translation");
	shader->rot_loc = glGetAttribLocation(shader->program, "rotation");
	shader->scale_loc = glGetAttribLocation(shader->program, "scale");
	shader->uv_loc = glGetAttribLocation(shader->program, "texcoord");
	shader->color_loc = glGetAttribLocation(shader->program, "color");
	shader->modelviewproj_loc = glGetUniformLocation(shader->program, "ModelViewProj");
	shader->texture_loc = glGetUniformLocation(shader->program, "Texture");
	shader->gbuffer_depth_loc = glGetUniformLocation(shader->program, "GBuffer_Depth");
	return 0;
}

static void draw_billboard(Forward* f, GLuint texture, const vec3 position, float scale, const Scene *s) {
		const ParticleShader* shader = &f->particle_shader_textured;

		// bind shader
		glUseProgram(shader->program);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0,0,WINDOW_WIDTH, WINDOW_HEIGHT);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // alpha blend

		mat4x4 model;
		mat4x4_identity(model);
		vec3_dup(model[3], position);

		// bind model-view-projection matrix
		mat4x4 mvp;
		mat4x4_mul(mvp, s->camera.viewProj, model);
		glUniformMatrix4fv(shader->modelviewproj_loc, 1, GL_FALSE, (const GLfloat*)mvp);

		// bind texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(shader->texture_loc, 0);

		// calculate billboard factor
		vec3 rot;
		calculate_billboard_euler(rot, model, s);

		// submit
		vec4 color; color[3] = 1.0f;
		vec3_dup(color, s->main_light.color);
		vec3 scale3;
		vec3_swizzle(scale3, scale);
		draw_quad(shader, Vec_Zero, rot, scale3, color);
}

int forward_initialize(Forward* f) {
	memset(f, 0, sizeof(Forward));
	if (load_particle_shader(&f->particle_shader_flat, "shaders/particle.vert", "shaders/particle_flat.frag", NULL, 0)) {
		return 1;
	}
	if (load_particle_shader(&f->particle_shader_textured, "shaders/particle.vert", "shaders/particle_textured.frag", NULL, 0 )) {
		return 1;
	}

	const char* soft_defines[] = {
		"#define SOFT_PARTICLES\n"
	};
	if (load_particle_shader(&f->particle_shader_textured_soft, "shaders/particle.vert", "shaders/particle_textured.frag"
			, soft_defines, STATIC_ELEMENT_COUNT(soft_defines))) {
		return 1;
	}

	if ((f->light_icon = utility_load_image(GL_TEXTURE_2D, "icons/lightbulb.png")) < 0) {
		return 1;
	}

	return 0;
}

void forward_render(Forward* f, Scene *s) {
	// bind default backbuffer
	glDepthMask(GL_FALSE);

	// setup render state
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);

	// draw emitters
	for (int i = 0; i < SCENE_EMITTERS_MAX, s->emitters[i]; i++) {
		ParticleEmitter* emitter = s->emitters[i];
		const ParticleEmitterDesc* desc = emitter->desc;

		const ParticleShader* shader = NULL;
		switch (desc->shading_mode) {
			case PARTICLE_SHADING_TEXTURED: {
				if (desc->soft) {
					shader = &f->particle_shader_textured_soft;
				}
				else {
					shader = &f->particle_shader_textured;
				}
				break;
			}
			case PARTICLE_SHADING_FLAT: //  fallthrough
			default: shader = &f->particle_shader_flat;
		}

		// bind particle program
		glUseProgram(shader->program);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0,0,WINDOW_WIDTH, WINDOW_HEIGHT);

		// depth sort and select blend mode
		if (desc->depth_sort_alpha_blend) {
			particle_emitter_sort(emitter, s->camera.pos);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // alpha blend
		} else {
			glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive blend
		}

		// calculate model matrix
		mat4x4 model;
		mat4x4_make_transform_uscale(model, emitter->scale, emitter->rot, emitter->pos);

		// bind model-view-projection matrix
		mat4x4 mvp;
		mat4x4_mul(mvp, s->camera.viewProj, model);
		glUniformMatrix4fv(shader->modelviewproj_loc, 1, GL_FALSE, (const GLfloat*)mvp);

		// bind depth texture for 'soft' particles
		if (desc->soft && f->g_buffer && shader->gbuffer_depth_loc != -1) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, f->g_buffer->depth_render_buffer);
			glUniform1i(shader->gbuffer_depth_loc, 0);
		}

		// bind texture
		if (shader->texture_loc >= 0 && desc->texture >= 0) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, desc->texture);
			glUniform1i(shader->texture_loc, 1);
		}

		// calc optional billboarding euler angles
		vec3 euler;
		if (desc->orient_mode == PARTICLE_ORIENT_SCREEN_ALIGNED) {
			calculate_billboard_euler(euler, model, s);
		}

		// draw particles
		for (int j = 0; j < emitter->count; j++) {
			const Particle* part;
			if (desc->depth_sort_alpha_blend) {
				part = &emitter->particles[emitter->sort_records[j].index];
			} else {
				part = &emitter->particles[j];
			}

			vec3 rot;
			if (desc->orient_mode == PARTICLE_ORIENT_SCREEN_ALIGNED) {
				vec3_dup(rot, euler);
			} else {
				vec3_dup(rot, part->rot);
			}

			draw_quad(shader, part->pos, rot, part->scale, part->color);
		}
	}

	// Draw main light icon
	draw_billboard(f, f->light_icon, s->main_light.position, 2.0f, s);

	glDepthMask(GL_TRUE);

	GL_CHECK_ERROR();
}
