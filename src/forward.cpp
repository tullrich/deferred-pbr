#include "forward.h"

static void draw_quad(const ParticleShader* shader, const vec3 translation, const vec3 rot, const vec3 scale, const vec4 color) {
  GL_WRAP(
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
  );
}

static void calculate_billboard_euler(vec3 euler, mat4x4 model, const Scene *s) {
  vec4 cam_forward, cam_up;
  cam_forward[3] = cam_up[3] = 0.0f;
  camera_forward(&s->camera, cam_forward);
  camera_up(&s->camera, cam_up);

  mat4x4 invModel;
  mat4x4_invert(invModel, model);

  vec4 model_cam_forward, model_cam_up;
  mat4x4_mul_vec4(model_cam_forward, invModel, cam_forward);
  vec4_negate_in_place(model_cam_forward);
  mat4x4_mul_vec4(model_cam_up, invModel, cam_up);

  mat4x4 lookAt, invLookAt;
  mat4x4_look_at(lookAt, Zero, model_cam_forward, model_cam_up);
  mat4x4_invert(invLookAt, lookAt);

  vec3_zero(euler);
  mat4x4_to_euler(euler, invLookAt);
}

static int load_particle_shader(ParticleShader* shader, const char* vert, const char* frag, const char** defines, int defines_count) {
  // initialize particle shader
  if (!(shader->program = utility_create_program_defines(vert, frag, defines, defines_count))) {
    printf("Unable to load shader [%s. %s]\n", vert, frag);
    return 1;
  }
  GL_WRAP(glBindAttribLocation(shader->program, 0, "vert"));
  GL_WRAP(glBindAttribLocation(shader->program, 1, "translation"));
  GL_WRAP(glBindAttribLocation(shader->program, 2, "rotation"));
  GL_WRAP(glBindAttribLocation(shader->program, 3, "scale"));
  GL_WRAP(glBindAttribLocation(shader->program, 4, "texcoord"));
  GL_WRAP(glBindAttribLocation(shader->program, 5, "color"));
  if (utility_link_program(shader->program)) {
    printf( "Unable to load shader\n" );
    return 1;
  }
  GL_WRAP(shader->vert_loc = glGetAttribLocation(shader->program, "vert"));
  GL_WRAP(shader->trans_loc = glGetAttribLocation(shader->program, "translation"));
  GL_WRAP(shader->rot_loc = glGetAttribLocation(shader->program, "rotation"));
  GL_WRAP(shader->scale_loc = glGetAttribLocation(shader->program, "scale"));
  GL_WRAP(shader->uv_loc = glGetAttribLocation(shader->program, "texcoord"));
  GL_WRAP(shader->color_loc = glGetAttribLocation(shader->program, "color"));
  GL_WRAP(shader->modelviewproj_loc = glGetUniformLocation(shader->program, "ModelViewProj"));
  GL_WRAP(shader->texture_loc = glGetUniformLocation(shader->program, "Texture"));
  GL_WRAP(shader->gbuffer_depth_loc = glGetUniformLocation(shader->program, "GBuffer_Depth"));
  return 0;
}

static void draw_billboard(Forward* f, GLuint texture, const vec3 position, float scale, const Scene *s) {
    const ParticleShader* shader = &f->particle_shader_textured;

    // bind shader
    GL_WRAP(glUseProgram(shader->program));
    GL_WRAP(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    GL_WRAP(glViewport(VIEWPORT_X_OFFSET, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT));
    GL_WRAP(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)); // alpha blend

    mat4x4 model;
    mat4x4_identity(model);
    vec3_dup(model[3], position);

    // bind model-view-projection matrix
    mat4x4 mvp;
    mat4x4_mul(mvp, s->camera.viewProj, model);
    GL_WRAP(glUniformMatrix4fv(shader->modelviewproj_loc, 1, GL_FALSE, (const GLfloat*)mvp));

    // bind texture
    GL_WRAP(glActiveTexture(GL_TEXTURE0));
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, texture));
    GL_WRAP(glUniform1i(shader->texture_loc, 0));

    // calculate billboard factor
    vec3 rot;
    calculate_billboard_euler(rot, model, s);

    // submit
    vec4 color; color[3] = 1.0f;
    vec3_dup(color, s->light->color);
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

  if ((f->light_icon = utility_load_texture(GL_TEXTURE_2D, "icons/lightbulb.png")) == 0) {
    return 1;
  }

  return 0;
}

void forward_render(Forward* f, const Scene *s) {
  // bind default backbuffer
  GL_WRAP(glDepthMask(GL_FALSE));

  // setup render state
  GL_WRAP(glDisable(GL_CULL_FACE));
  GL_WRAP(glEnable(GL_BLEND));
  GL_WRAP(glBlendEquation(GL_FUNC_ADD));

  // draw emitters
  for (int i = 0; i < SCENE_EMITTERS_MAX; i++) {
    if (!s->emitters[i])
      continue;

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
    GL_WRAP(glUseProgram(shader->program));
    GL_WRAP(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    GL_WRAP(glViewport(VIEWPORT_X_OFFSET, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT));

    // depth sort and select blend mode
    if (desc->depth_sort_alpha_blend) {
      particle_emitter_sort(emitter, s->camera.pos);
      GL_WRAP(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)); // alpha blend
    } else {
      GL_WRAP(glBlendFunc(GL_SRC_ALPHA, GL_ONE)); // additive blend
    }

    // calculate model matrix
    mat4x4 model;
    mat4x4_make_transform_uscale(model, emitter->scale, emitter->rot, emitter->pos);

    // bind model-view-projection matrix
    mat4x4 mvp;
    mat4x4_mul(mvp, s->camera.viewProj, model);
    GL_WRAP(glUniformMatrix4fv(shader->modelviewproj_loc, 1, GL_FALSE, (const GLfloat*)mvp));

    // bind depth texture for 'soft' particles
    if (desc->soft && f->g_buffer && shader->gbuffer_depth_loc != -1) {
      GL_WRAP(glActiveTexture(GL_TEXTURE0));
      GL_WRAP(glBindTexture(GL_TEXTURE_2D, f->g_buffer->depth_render_buffer));
      GL_WRAP(glUniform1i(shader->gbuffer_depth_loc, 0));
    }

    // bind texture
    if (shader->texture_loc >= 0 && desc->texture > 0) {
      GL_WRAP(glActiveTexture(GL_TEXTURE1));
      GL_WRAP(glBindTexture(GL_TEXTURE_2D, desc->texture));
      GL_WRAP(glUniform1i(shader->texture_loc, 1));
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
  draw_billboard(f, f->light_icon, s->light->position, 2.0f, s);

  GL_WRAP(glDepthMask(GL_TRUE));
}
