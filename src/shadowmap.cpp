#include "shadowmap.h"
#include "utility.h"

static int load_depth_render_shader(DepthRenderShader* shader, const char** defines, int defines_count) {
  if(!(shader->program = utility_create_program_defines("shaders/mesh.vert", "shaders/mesh.frag"
                                , defines, defines_count))) {
    return 1;
  }
  GL_WRAP(glBindAttribLocation(shader->program, 0, "position"));
  if (utility_link_program(shader->program)) {
    return 1;
  }
  GL_WRAP(shader->pos_loc = glGetAttribLocation(shader->program, "position"));
  GL_WRAP(shader->model_view_loc = glGetUniformLocation(shader->program, "ModelView"));
  GL_WRAP(shader->model_view_proj_loc = glGetUniformLocation(shader->program, "ModelViewProj"));
  return 0;
}

static int load_debug_shader(ShadowDebugShader* shader, const char** defines, int defines_count) {
  if (!(shader->program = utility_create_program_defines("shaders/passthrough.vert", "shaders/passthrough.frag"
                                , defines, defines_count))) {
    return 1;
  }
  GL_WRAP(shader->pos_loc = glGetAttribLocation(shader->program, "position"));
  GL_WRAP(shader->texcoord_loc = glGetAttribLocation(shader->program, "texcoord"));
  GL_WRAP(shader->render_map_loc = glGetUniformLocation(shader->program, "RenderMap"));
  GL_WRAP(shader->depth_map_loc = glGetUniformLocation(shader->program, "DepthMap"));
  GL_WRAP(shader->z_near_loc = glGetUniformLocation(shader->program, "ZNear"));
  GL_WRAP(shader->z_far_loc = glGetUniformLocation(shader->program, "ZFar"));
  return 0;
}

int shadow_map_initialize(ShadowMap* shadow_map, int width, int height) {
  memset(shadow_map, 0, sizeof(ShadowMap));
  shadow_map->width = width;
  shadow_map->height = height;

  if (load_depth_render_shader(&shadow_map->depth_render_shader, NULL, 0)) {
    printf("Unable to load depth render shader\n");
    return 1;
  }

  const char* debug_linearize_defines[] ={
    // "#define DEBUG_RENDER_LINEARIZE\n"
  };
  if (load_debug_shader(&shadow_map->debug_shader, debug_linearize_defines, STATIC_ELEMENT_COUNT(debug_linearize_defines))) {
    printf("Unable to load debug shader\n");
    return 1;
  }

  // Init framebuffer
  GL_WRAP(glGenFramebuffers(1, &shadow_map->fbo));
  GL_WRAP(glBindFramebuffer(GL_FRAMEBUFFER, shadow_map->fbo));

  // Init depth texture
  GL_WRAP(glGenTextures(1, &shadow_map->depth_buffer));
  GL_WRAP(glBindTexture(GL_TEXTURE_2D, shadow_map->depth_buffer));
  GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
  GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
  GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL_WRAP(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0));

  // Attach depth to fbo
  GL_WRAP(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map->depth_buffer, 0));
  GL_WRAP(glDrawBuffer(GL_NONE));
  GL_WRAP(glReadBuffer(GL_NONE));

  GLenum fbo_status;
  GL_WRAP(fbo_status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
  if (fbo_status != GL_FRAMEBUFFER_COMPLETE) {
    printf("glCheckFramebufferStatus failed\n");
    return 1;
  }

  // Cleanup
  GL_WRAP(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  return 0;
}

static void render_geometry(ShadowMap *shadow_map, Model* model, Scene *s) {
  if (!model->mesh->vertices)
    return;

  const DepthRenderShader* shader = &shadow_map->depth_render_shader;
  GL_WRAP(glUseProgram(shader->program));

  mat4x4 proj;
  mat4x4_ortho(proj, -50.0f, 50.0f, -50.0f, 50.0f, Z_NEAR, Z_FAR);

  mat4x4 view;
  vec3 center = { 0.0f, 0.0f, 0.0f };
  mat4x4_look_at(view, s->light->position, center, Axis_Up);

  mat4x4 vp;
  mat4x4_mul(vp, proj, view);

  // calc model matrix
  mat4x4 m;
  mat4x4_identity(m);
  mat4x4_rotate_Z(m, m, DEG_TO_RAD(model->rot[2]));
  mat4x4_rotate_Y(m, m, DEG_TO_RAD(model->rot[1]));
  mat4x4_rotate_X(m, m, DEG_TO_RAD(model->rot[0]));
  float scale = model->scale * model->mesh->base_scale;
  mat4x4_scale_aniso(m, m, scale, scale, scale);
  vec3_add(m[3], m[3], model->position);
  mat4x4_translate_in_place(m, -model->mesh->bounds.center[0], -model->mesh->bounds.center[1], -model->mesh->bounds.center[2]);

  // bind model-view matrix
  mat4x4 mv;
  mat4x4_mul(mv, view, m);
  GL_WRAP(glUniformMatrix4fv(shader->model_view_loc, 1, GL_FALSE, (const GLfloat*)mv));

  // bind model-view-projection matrix
  mat4x4 mvp;
  mat4x4_mul(mvp, vp, m);
  GL_WRAP(glUniformMatrix4fv(shader->model_view_proj_loc, 1, GL_FALSE, (const GLfloat*)mvp));

  mesh_draw(model->mesh, -1, -1, -1, shader->pos_loc);
}


void shadow_map_render(ShadowMap *shadow_map, Scene *s) {
  // Bind render target
  GL_WRAP(glBindFramebuffer(GL_FRAMEBUFFER, shadow_map->fbo));
  GL_WRAP(glViewport(0, 0, shadow_map->width, shadow_map->height));

  // Clear depth attachment
  GL_WRAP(glDisable(GL_BLEND));
  GL_WRAP(glEnable(GL_DEPTH_TEST));
  GL_WRAP(glDepthFunc(GL_LEQUAL));
  GL_WRAP(glClearDepth(1.0f));
  GL_WRAP(glClear(GL_DEPTH_BUFFER_BIT));

  for (int i = 0; i < SCENE_MODELS_MAX, s->models[i]; i++) {
    render_geometry(shadow_map, s->models[i], s);
  }

  // Cleanup
  GL_WRAP(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void shadow_map_render_debug(const ShadowMap *shadow_map, int x_off, int y_off, int width, int height) {
  GL_WRAP(glUseProgram(shadow_map->debug_shader.program));
  GL_WRAP(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  GL_WRAP(glViewport(x_off, y_off, width, height));
  GL_WRAP(glDisable(GL_BLEND));
  GL_WRAP(glDisable(GL_DEPTH_TEST));

  GL_WRAP(glActiveTexture(GL_TEXTURE0));
  GL_WRAP(glBindTexture(GL_TEXTURE_2D, shadow_map->depth_buffer));
  GL_WRAP(glUniform1i(shadow_map->debug_shader.render_map_loc, 0));

  GL_WRAP(glActiveTexture(GL_TEXTURE1));
  GL_WRAP(glBindTexture(GL_TEXTURE_2D, shadow_map->depth_buffer));
  GL_WRAP(glUniform1i(shadow_map->debug_shader.depth_map_loc, 1));

  GL_WRAP(glUniform1f(shadow_map->debug_shader.z_near_loc, Z_NEAR));
  GL_WRAP(glUniform1f(shadow_map->debug_shader.z_far_loc, Z_FAR));

  utility_draw_fullscreen_quad(shadow_map->debug_shader.texcoord_loc, shadow_map->debug_shader.pos_loc);
}
