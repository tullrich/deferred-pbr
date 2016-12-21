#include "forward.h"

static void draw_quad(const ParticleShader* shader,const vec3 translation, const vec3 rot, const vec3 scale, const vec4 color) {
	glBegin(GL_QUADS);
		if (shader->trans_loc >= 0) glVertexAttrib3f(shader->trans_loc, translation[0], translation[1], translation[2]);
		if (shader->rot_loc >= 0) glVertexAttrib3f(shader->rot_loc, rot[0], rot[1], rot[2]);
		if (shader->scale_loc >= 0) glVertexAttrib3f(shader->scale_loc, scale[0], scale[1], scale[2]);
		if (shader->color_loc >= 0) glVertexAttrib4f(shader->color_loc, color[0], color[1], color[2], color[3]);
		if (shader->uv_loc >= 0) glVertexAttrib2f(shader->uv_loc, 1, 0);
		glVertexAttrib3f(shader->vert_loc, 1.0f, -1.0f, 0.0f);
		if (shader->uv_loc >= 0) glVertexAttrib2f(shader->uv_loc, 1, 1.0f);
		glVertexAttrib3f(shader->vert_loc, 1.0f, 1.0f, 0.0f);
		if (shader->uv_loc >= 0) glVertexAttrib2f(shader->uv_loc, 0, 1.0f);
		glVertexAttrib3f(shader->vert_loc, -1.0f, 1.0f, 0.0f);
		if (shader->uv_loc >= 0) glVertexAttrib2f(shader->uv_loc, 0, 0);
		glVertexAttrib3f(shader->vert_loc, -1.0f, -1.0f, 0.0f);
	glEnd();
}

static void calculate_emitter_billboard_euler(vec3 euler, mat4x4 model2, const ParticleEmitter* emitter, const Scene *s) {
	vec3 cam_forward, cam_up;
	scene_camera_forward(s, cam_forward);
	scene_camera_up(s, cam_up);
	cam_forward[3] = cam_up[3] = 0.0f;

	mat4x4 invModel;
	mat4x4_invert(invModel, model2);

	vec4 model_cam_forward, model_cam_up;
	mat4x4_mul_vec4(model_cam_forward, invModel, cam_forward);
	vec4_negate_in_place(model_cam_forward);
	mat4x4_mul_vec4(model_cam_up, invModel, cam_up);
	vec4_negate_in_place(model_cam_up);

	mat4x4 lookAt;
	vec3 zero;
	vec3_zero(zero);
	mat4x4_look_at(lookAt, zero, model_cam_forward, model_cam_up);

	vec3_zero(euler);
	if (emitter->desc->billboard) {
		mat4x4_to_euler(euler, lookAt);
	}
}

static int load_particle_shader(ParticleShader* shader, const char* vert, const char* frag) {
	// initialize particle shader
	if (!(shader->program = utility_create_program(vert, frag))) {
		printf("Unable to load shader [%f. %f]\n", vert, frag);
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
	return 0;
}

int forward_initialize(Forward* f) {
	memset(f, 0, sizeof(Forward));
	if (load_particle_shader(&f->particle_shader_flat, "shaders/particle.vert", "shaders/particle_flat.frag")) {
		return 1;
	}
	if (load_particle_shader(&f->particle_shader_textured, "shaders/particle.vert", "shaders/particle_textured.frag")) {
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
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// draw emitters (nieve implementation)
	for (int i = 0; i < SCENE_EMITTERS_MAX, s->emitters[i]; i++) {
		const ParticleEmitter* emitter = s->emitters[i];
		const ParticleEmitterDesc* desc = emitter->desc;

		const ParticleShader* shader = NULL;
		switch (desc->shading_mode) {
			case PARTICLE_SHADING_TEXTURED: shader = &f->particle_shader_textured; break;
			case PARTICLE_SHADING_FLAT: //  fallthrough
			default: shader = &f->particle_shader_flat;
		}

		// bind particle program
		glUseProgram(shader->program);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0,0,WINDOW_WIDTH, WINDOW_HEIGHT);

		// calculate model matrix
		mat4x4 model;
		mat4x4_make_transform_uscale(model, emitter->scale, emitter->rot, emitter->pos);

		// bind model-view-projection matrix
		mat4x4 mvp;
		mat4x4_mul(mvp, s->camera.viewProj, model);
		glUniformMatrix4fv(shader->modelviewproj_loc, 1, GL_FALSE, (const GLfloat*)mvp);

		// bind texture
		if (shader->texture_loc >= 0 && desc->texture >= 0) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, desc->texture);
			glUniform1i(shader->texture_loc, 0);
		}

		// calc optional billboarding euler angles
		vec3 euler;
		if (desc->billboard) {
			calculate_emitter_billboard_euler(euler, model, emitter, s);
		}

		// draw particles
		for (int j = 0; j < emitter->count; j++) {
			const Particle* part = &emitter->particles[j];
			vec3 rot;
			if (desc->billboard) {
				vec3_dup(rot, euler);
			} else {
				vec3_dup(rot, part->rot);
			}

			draw_quad(shader, part->pos, rot, part->scale, part->color);
		}
	}

	glDepthMask(GL_TRUE);
}
