#include "deferred.h"

DEFINE_ENUM(RenderMode, render_mode_strings, ENUM_RenderMode);
DEFINE_ENUM(SkyboxMode, skybox_mode_strings, ENUM_SkyboxMode);
DEFINE_ENUM(TonemappingOperator, tonemapping_op_strings, ENUM_TonemappingOperator);

static int load_surface_shader(SurfaceShader* shader, const char** defines, int defines_count) {
  if(!(shader->program = utility_create_program_defines("shaders/mesh.vert", "shaders/mesh.frag",
                                 defines, defines_count))) {
    return 1;
  }
  GL_WRAP(glBindFragDataLocation(shader->program, 0, "AlbedoOut"));
  GL_WRAP(glBindFragDataLocation(shader->program, 1, "NormalOut"));
  GL_WRAP(glBindFragDataLocation(shader->program, 2, "RoughnessOut"));
  GL_WRAP(glBindFragDataLocation(shader->program, 3, "MetalnessOut"));
  GL_WRAP(glBindAttribLocation(shader->program, 0, "position"));
  GL_WRAP(glBindAttribLocation(shader->program, 1, "normal"));
  GL_WRAP(glBindAttribLocation(shader->program, 2, "tangent"));
  GL_WRAP(glBindAttribLocation(shader->program, 3, "texcoord"));
  if (utility_link_program(shader->program)) {
    return 1;
  }

  GL_WRAP(shader->pos_loc = glGetAttribLocation(shader->program, "position"));
  GL_WRAP(shader->normal_loc = glGetAttribLocation(shader->program, "normal"));
  GL_WRAP(shader->tangent_loc = glGetAttribLocation(shader->program, "tangent"));
  GL_WRAP(shader->texcoord_loc = glGetAttribLocation(shader->program, "texcoord"));
  GL_WRAP(shader->view_pos_loc = glGetUniformLocation(shader->program, "ViewPos"));
  GL_WRAP(shader->model_view_loc = glGetUniformLocation(shader->program, "ModelView"));
  GL_WRAP(shader->model_view_proj_loc = glGetUniformLocation(shader->program, "ModelViewProj"));
  GL_WRAP(shader->albedo_base_loc = glGetUniformLocation(shader->program, "AlbedoBase"));
  GL_WRAP(shader->metalness_base_loc = glGetUniformLocation(shader->program, "MetalnessBase"));
  GL_WRAP(shader->roughness_base_loc = glGetUniformLocation(shader->program, "RoughnessBase"));
  GL_WRAP(shader->emissive_base_loc = glGetUniformLocation(shader->program, "EmissiveBase"));
  GL_WRAP(shader->albedo_map_loc = glGetUniformLocation(shader->program, "AlbedoMap"));
  GL_WRAP(shader->normal_map_loc = glGetUniformLocation(shader->program, "NormalMap"));
  GL_WRAP(shader->height_map_loc = glGetUniformLocation(shader->program, "HeightMap"));
  GL_WRAP(shader->height_scale_loc = glGetUniformLocation(shader->program, "HeightScale"));
  GL_WRAP(shader->metalness_map_loc = glGetUniformLocation(shader->program, "MetalnessMap"));
  GL_WRAP(shader->roughness_map_loc = glGetUniformLocation(shader->program, "RoughnessMap"));
  GL_WRAP(shader->ao_map_loc = glGetUniformLocation(shader->program, "AOMap"));
  GL_WRAP(shader->emissive_map_loc = glGetUniformLocation(shader->program, "EmissiveMap"));
  return 0;
}

static int load_lighting_shader(LightingShader* shader, const char* tonemapping_define) {
  char buf[512];
  sprintf(buf, "#define %s\n", tonemapping_define);
  const char* defines[] = { &buf[0] };
  if(!(shader->program = utility_create_program_defines("shaders/passthrough.vert", "shaders/lighting.frag",
            defines, 1))) {
    printf("Unable to load shader\n");
    return 1;
  }
  GL_WRAP(shader->pos_loc = glGetAttribLocation(shader->program, "position"));
  GL_WRAP(shader->texcoord_loc = glGetAttribLocation(shader->program, "texcoord"));
  GL_WRAP(shader->ambient_term_loc = glGetUniformLocation(shader->program, "AmbientTerm"));
  GL_WRAP(shader->light_pos_loc = glGetUniformLocation(shader->program, "MainLightPosition"));
  GL_WRAP(shader->light_color_loc = glGetUniformLocation(shader->program, "MainLightColor"));
  GL_WRAP(shader->light_intensity_loc = glGetUniformLocation(shader->program, "MainLightIntensity"));
  GL_WRAP(shader->eye_pos_loc = glGetUniformLocation(shader->program, "EyePosition"));

  GL_WRAP(shader->gbuffer_normal_loc = glGetUniformLocation(shader->program, "GBuffer_Normal"));
  GL_WRAP(shader->gbuffer_albedo_loc = glGetUniformLocation(shader->program, "GBuffer_Albedo"));
  GL_WRAP(shader->gbuffer_roughness_loc = glGetUniformLocation(shader->program, "GBuffer_Roughness"));
  GL_WRAP(shader->gbuffer_metalness_loc = glGetUniformLocation(shader->program, "GBuffer_Metalness"));
  GL_WRAP(shader->gbuffer_depth_loc = glGetUniformLocation(shader->program, "GBuffer_Depth"));
  GL_WRAP(shader->env_irr_map_loc = glGetUniformLocation(shader->program, "EnvIrrMap"));
  GL_WRAP(shader->env_prefilter_map_loc = glGetUniformLocation(shader->program, "EnvPrefilterMap"));
  GL_WRAP(shader->env_brdf_lut_loc = glGetUniformLocation(shader->program, "EnvBrdfLUT"));
  GL_WRAP(shader->shadow_map_loc = glGetUniformLocation(shader->program, "ShadowMap"));
  GL_WRAP(shader->inv_view_loc = glGetUniformLocation(shader->program, "InvView"));
  GL_WRAP(shader->inv_proj_loc = glGetUniformLocation(shader->program, "InvProjection"));
  GL_WRAP(shader->light_space_loc = glGetUniformLocation(shader->program, "LightSpace"));
  GL_WRAP(shader->exposure_loc = glGetUniformLocation(shader->program, "Exposure"));
  GL_WRAP(shader->ao_strength_loc = glGetUniformLocation(shader->program, "AOStrength"));

  return 0;
}

static int load_debug_shader(DebugShader* shader, const char** defines, int defines_count) {
  if (!(shader->program = utility_create_program_defines("shaders/passthrough.vert", "shaders/passthrough.frag"
        , defines, defines_count))) {
    return 1;
  }
  GL_WRAP(shader->pos_loc = glGetAttribLocation(shader->program, "position"));
  GL_WRAP(shader->texcoord_loc = glGetAttribLocation(shader->program, "texcoord"));
  GL_WRAP(shader->gbuffer_render_loc = glGetUniformLocation(shader->program, "RenderMap"));
  GL_WRAP(shader->gbuffer_depth_loc = glGetUniformLocation(shader->program, "DepthMap"));
  GL_WRAP(shader->z_near_loc = glGetUniformLocation(shader->program, "ZNear"));
  GL_WRAP(shader->z_far_loc = glGetUniformLocation(shader->program, "ZFar"));
  return 0;
}

int deferred_initialize(Deferred* d) {
  memset(d, 0, sizeof(Deferred));
  d->render_mode = RENDER_MODE_SHADED;
  d->skybox_mode = SKYBOX_MODE_ENV_MAP;
  d->prefilter_lod = 0.0f;
  d->tonemapping_op = TONEMAPPING_OP_UNCHARTED2;
  d->ao_strength = 1.0f;

  // Initialize default material
  if (material_initialize_default(&d->default_mat)) {
    printf("Unable to load default material\n");
    return 1;
  }

  // initialize the BRDF look-up table
  if (!(d->brdf_lut_tex = utility_load_texture(GL_TEXTURE_2D, "./environments/ibl_brdf_lut.png"))) {
    printf("Unable to load ibl_brdf_lut.png\n");
    return 1;
  }

  // Initialize the skybox shader
  if (!(d->skybox_shader.program = utility_create_program("shaders/skybox.vert", "shaders/skybox.frag"))) {
    printf("Unable to load shader\n");
    return 1;
  }
  GL_WRAP(glBindAttribLocation(d->skybox_shader.program, 0, "position" ));
  GL_WRAP(glBindAttribLocation(d->skybox_shader.program, 0, "texcoord" ));
  if (utility_link_program(d->skybox_shader.program)) {
    printf( "Unable to load shader\n" );
    return 1;
  }
  GL_WRAP(d->skybox_shader.pos_loc = glGetAttribLocation(d->skybox_shader.program, "position"));
  GL_WRAP(d->skybox_shader.texcoord_loc = glGetAttribLocation(d->skybox_shader.program, "texcoord"));
  GL_WRAP(d->skybox_shader.env_map_loc = glGetUniformLocation(d->skybox_shader.program, "SkyboxCube"));
  GL_WRAP(d->skybox_shader.lod_loc = glGetUniformLocation(d->skybox_shader.program, "Lod"));
  GL_WRAP(d->skybox_shader.inv_vp_loc = glGetUniformLocation(d->skybox_shader.program, "InvViewProj"));

  // Initialize box shader
  const char* uv_surf_shader_defines[] ={
    "#define MESH_VERTEX_UV1\n",
    "#define USE_NORMAL_MAP\n",
    "#define USE_HEIGHT_MAP\n",
    "#define USE_ALBEDO_MAP\n",
    "#define USE_ROUGHNESS_MAP\n",
    "#define USE_METALNESS_MAP\n",
    "#define USE_AO_MAP\n",
    "#define USE_EMISSIVE_MAP\n",
  };
  if(load_surface_shader(&d->surf_shader[0], uv_surf_shader_defines, STATIC_ELEMENT_COUNT(uv_surf_shader_defines))
      || load_surface_shader(&d->surf_shader[1], NULL, 0)) {
    printf("Unable to load shader\n");
    return 1;
  }

  if(load_lighting_shader(&d->lighting_shader[0], "TONE_MAPPING_REINHARD")
      || load_lighting_shader(&d->lighting_shader[1], "TONE_MAPPING_UNCHARTED2")) {
    printf("Unable to load shader\n");
    return 1;
  }

  if(gbuffer_initialize(&d->g_buffer, VIEWPORT_WIDTH, VIEWPORT_HEIGHT)) {
    printf("Unable to create g-buffer.\n");
    return 1;
  }

  // Initialize passthrough
  const char* debug_ndc_defines[] ={
    "#define DEBUG_RENDER_NORMALIZE\n"
  };
  const char* debug_linearize_defines[] ={
    "#define DEBUG_RENDER_LINEARIZE\n"
  };
  if(load_debug_shader(&d->debug_shader[0], NULL, 0)
     || load_debug_shader(&d->debug_shader[1], debug_ndc_defines, STATIC_ELEMENT_COUNT(debug_ndc_defines))
     || load_debug_shader(&d->debug_shader[2], debug_linearize_defines, STATIC_ELEMENT_COUNT(debug_linearize_defines))) {
    printf("Unable to load debug shader\n");
    return 1;
  }

  return 0;
}

static void render_geometry(const Model* model, Deferred* d, const Scene *s) {
  if (!model->mesh->vertices)
    return;

  int shader_idx = (model->mesh->texcoords) ? 0:1;
  const SurfaceShader* shader = &d->surf_shader[shader_idx];
  GL_WRAP(glUseProgram(shader->program));

  // bind albedo base color
  GL_WRAP(glUniform3fv(shader->albedo_base_loc, 1, model->material.albedo_base));

  // bind metalness base color
  GL_WRAP(glUniform1f(shader->metalness_base_loc, model->material.metalness_base));

  // bind roughness base color
  GL_WRAP(glUniform1f(shader->roughness_base_loc, model->material.roughness_base));

  // bind emissive base color
  GL_WRAP(glUniform3fv(shader->emissive_base_loc, 1, model->material.emissive_base));

  // bind albedo map
  GL_WRAP(glActiveTexture(GL_TEXTURE0));
  if (model->material.albedo_map) {
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, model->material.albedo_map));
  } else {
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, d->default_mat.albedo_map));
  }
  GL_WRAP(glUniform1i(shader->albedo_map_loc, 0));

  // bind normal map
  GL_WRAP(glActiveTexture(GL_TEXTURE1));
  if (model->material.normal_map) {
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, model->material.normal_map));
  } else {
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, d->default_mat.normal_map));
  }
  GL_WRAP(glUniform1i(shader->normal_map_loc, 1));

  // bind height map
  GL_WRAP(glActiveTexture(GL_TEXTURE2));
  if (model->material.height_map) {
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, model->material.height_map));
  } else {
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, d->default_mat.height_map));
  }
  GL_WRAP(glUniform1i(shader->height_map_loc, 2));

  // bind metalness map
  GL_WRAP(glActiveTexture(GL_TEXTURE3));
  if (model->material.metalness_map) {
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, model->material.metalness_map));
  } else {
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, d->default_mat.metalness_map));
  }
  GL_WRAP(glUniform1i(shader->metalness_map_loc, 3));

  // bind roughness map
  GL_WRAP(glActiveTexture(GL_TEXTURE4));
  if (model->material.roughness_map) {
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, model->material.roughness_map));
  } else {
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, d->default_mat.roughness_map));
  }
  GL_WRAP(glUniform1i(shader->roughness_map_loc, 4));

  // bind ao map
  GL_WRAP(glActiveTexture(GL_TEXTURE5));
  if (model->material.ao_map) {
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, model->material.ao_map));
  } else {
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, d->default_mat.ao_map));
  }
  GL_WRAP(glUniform1i(shader->ao_map_loc, 5));

  // bind emissive map
  GL_WRAP(glActiveTexture(GL_TEXTURE6));
  if (model->material.emissive_map) {
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, model->material.emissive_map));
  } else {
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, d->default_mat.emissive_map));
  }
  GL_WRAP(glUniform1i(shader->emissive_map_loc, 6));

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
  mat4x4_mul(mv, s->camera.view, m);
  GL_WRAP(glUniformMatrix4fv(shader->model_view_loc, 1, GL_FALSE, (const GLfloat*)mv));

  // bind the direction to camera in model space position
  mat4x4 invMv;
  vec4 origin = { 0.0f, 0.0f, 0.0f, 1.0f };
  vec4 view_pos;
  mat4x4_invert(invMv, mv);
  mat4x4_mul_vec4(view_pos, invMv, origin);
  GL_WRAP(glUniform3fv(shader->view_pos_loc, 1, (const GLfloat*)view_pos));

  // bind model-view-projection matrix
  mat4x4 mvp;
  mat4x4_mul(mvp, s->camera.viewProj, m);
  GL_WRAP(glUniformMatrix4fv(shader->model_view_proj_loc, 1, GL_FALSE, (const GLfloat*)mvp));

  // bind the height map scale factor
  float height_scale = (model->material.height_map) ? model->material.height_map_scale : 0.0f;
  GL_WRAP(glUniform1fv(shader->height_scale_loc, 1, (const GLfloat*)&height_scale));

  mesh_draw(model->mesh,
        shader->texcoord_loc,
        shader->normal_loc,
        shader->tangent_loc,
        shader->pos_loc);
}

static void render_shading(Deferred* d, const Scene *s, const ShadowMap* sm) {
  LightingShader* shader = &d->lighting_shader[(int)d->tonemapping_op];

  GL_WRAP(glUseProgram(shader->program));

  GL_WRAP(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  GL_WRAP(glViewport(VIEWPORT_X_OFFSET, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT));

  GL_WRAP(glEnable(GL_BLEND));
  GL_WRAP(glBlendEquation(GL_FUNC_ADD));
  GL_WRAP(glBlendFunc(GL_ONE, GL_ONE));

  // bind gbuffer
  int i;
  for (i = 0; i < GBUFFER_ATTACHMENTS_COUNT; i++) {
    GL_WRAP(glActiveTexture(GL_TEXTURE0 + i));
    GL_WRAP(glBindTexture(GL_TEXTURE_2D, d->g_buffer.attachments[i]));
    GL_WRAP(glUniform1i(shader->gbuffer_locs[i], i));
  }

  // bind env irradiance map
  GL_WRAP(glActiveTexture(GL_TEXTURE0+i));
  GL_WRAP(glBindTexture(GL_TEXTURE_CUBE_MAP, s->skybox->irr_cubemap));
  GL_WRAP(glUniform1i(shader->env_irr_map_loc, i));

  // bind env prefiltered map
  GL_WRAP(glActiveTexture(GL_TEXTURE0+i+1));
  GL_WRAP(glBindTexture(GL_TEXTURE_CUBE_MAP, s->skybox->prefilter_cubemap));
  GL_WRAP(glUniform1i(shader->env_prefilter_map_loc, i+1));

  // bind the brdf lut
  GL_WRAP(glActiveTexture(GL_TEXTURE0+i+2));
  GL_WRAP(glBindTexture(GL_TEXTURE_2D, d->brdf_lut_tex));
  GL_WRAP(glUniform1i(shader->env_brdf_lut_loc, i+2));

  // bind the shadow map
  GL_WRAP(glActiveTexture(GL_TEXTURE0+i+3));
  GL_WRAP(glBindTexture(GL_TEXTURE_2D, sm->depth_buffer));
  GL_WRAP(glUniform1i(shader->shadow_map_loc, i+3));

  // light setup
  vec4 view_light_pos_in;
  if (s->light->type == LIGHT_TYPE_POINT) {
    vec4_dup(view_light_pos_in, s->light->position);
  } else {
    mat4x4 m;
    vec4 tmp;
    mat4x4_identity(m);
    mat4x4_rotate_Z(m, m, DEG_TO_RAD(s->light->rot[2]));
    mat4x4_rotate_Y(m, m, DEG_TO_RAD(s->light->rot[1]));
    mat4x4_rotate_X(m, m, DEG_TO_RAD(s->light->rot[0]));
    vec4_set(tmp, 0.0f, 1.0f, 0.0f, 0.0f);
    mat4x4_mul_vec4(view_light_pos_in, m, tmp);
  }

  vec4 view_light_pos;
  mat4x4_mul_vec4(view_light_pos, s->camera.view, view_light_pos_in);

  vec3 ambient_term;
  vec3_scale(ambient_term, s->ambient_color, s->ambient_intensity);
  GL_WRAP(glUniform3fv(shader->ambient_term_loc, 1, (const GLfloat*)ambient_term));
  GL_WRAP(glUniform4fv(shader->light_pos_loc, 1, (const GLfloat*)view_light_pos));
  GL_WRAP(glUniform3fv(shader->light_color_loc, 1, (const GLfloat*)s->light->color));
  GL_WRAP(glUniform1f(shader->light_intensity_loc, s->light->intensity));

  // Inverse view
  mat4x4 inv_view;
  mat4x4_invert(inv_view, s->camera.view);
  GL_WRAP(glUniformMatrix4fv(shader->inv_view_loc, 1, GL_FALSE, (const GLfloat*)inv_view));

  // Inverse proj
  mat4x4 inv_proj;
  mat4x4_invert(inv_proj, s->camera.proj);
  GL_WRAP(glUniformMatrix4fv(shader->inv_proj_loc, 1, GL_FALSE, (const GLfloat*)inv_proj));

  // Light-space matrix for shadowmap
  mat4x4 light;
  mat4x4_mul(light, sm->vp, inv_view);
  GL_WRAP(glUniformMatrix4fv(shader->light_space_loc, 1, GL_FALSE, (const GLfloat*)light));

  // HDR Exposure value
  GL_WRAP(glUniform1f(shader->exposure_loc, s->camera.exposure));

  // AO strength
  GL_WRAP(glUniform1f(shader->ao_strength_loc, d->ao_strength));

  // Render every pixel
  utility_draw_fullscreen_quad(shader->texcoord_loc, shader->pos_loc);
}

static void render_skybox(Deferred *d, const Scene *s) {
  GL_WRAP(glUseProgram(d->skybox_shader.program));

  GL_WRAP(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  GL_WRAP(glViewport(VIEWPORT_X_OFFSET, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT));

  GL_WRAP(glDisable(GL_BLEND));

  // Bind environment map
  GL_WRAP(glActiveTexture(GL_TEXTURE0));
  switch(d->skybox_mode) {
    case SKYBOX_MODE_ENV_MAP: {
      GL_WRAP(glBindTexture(GL_TEXTURE_CUBE_MAP, s->skybox->env_cubemap));
      break;
    }
    case SKYBOX_MODE_IRR_MAP: {
      GL_WRAP(glBindTexture(GL_TEXTURE_CUBE_MAP, s->skybox->irr_cubemap));
      break;
    }
    case SKYBOX_MODE_PREFILTER_MAP: {
      GL_WRAP(glBindTexture(GL_TEXTURE_CUBE_MAP, s->skybox->prefilter_cubemap));
      break;
    }
  }
  GL_WRAP(glUniform1i(d->skybox_shader.env_map_loc, 0));
  GL_WRAP(glUniform1f(d->skybox_shader.lod_loc, (d->skybox_mode == SKYBOX_MODE_PREFILTER_MAP) ? d->prefilter_lod : 0.0f));

  mat4x4 view_rot, vp, inv_vp;
  mat4x4_dup(view_rot, s->camera.view);
  vec3_zero(view_rot[3]);
  mat4x4_mul(vp, s->camera.proj, view_rot);
  mat4x4_invert(inv_vp, vp);

  GL_WRAP(glUniformMatrix4fv(d->skybox_shader.inv_vp_loc, 1, GL_FALSE, (const GLfloat*)inv_vp));

  utility_draw_fullscreen_quad2(d->skybox_shader.texcoord_loc, d->skybox_shader.pos_loc);
}

static void render_debug(Deferred *d) {
  int program_idx = 0;
  GLuint render_buffer = 0;
  switch(d->render_mode) {
    case RENDER_MODE_ALBEDO: 	render_buffer = d->g_buffer.albedo_render_buffer; break;
    case RENDER_MODE_NORMAL: 	render_buffer = d->g_buffer.normal_render_buffer; program_idx = 1; break;
    case RENDER_MODE_ROUGHNESS: 	render_buffer = d->g_buffer.roughness_render_buffer; break;
    case RENDER_MODE_METALNESS: 	render_buffer = d->g_buffer.metalness_render_buffer; break;
    case RENDER_MODE_DEPTH: 	render_buffer = d->g_buffer.depth_render_buffer; program_idx = 2; break;
    default: return;
  }

  GL_WRAP(glUseProgram(d->debug_shader[program_idx].program));
  GL_WRAP(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  GL_WRAP(glViewport(VIEWPORT_X_OFFSET, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT));
  GL_WRAP(glDisable(GL_BLEND));

  GL_WRAP(glActiveTexture(GL_TEXTURE0));
  GL_WRAP(glBindTexture(GL_TEXTURE_2D, render_buffer));
  GL_WRAP(glUniform1i(d->debug_shader[program_idx].gbuffer_render_loc, 0));

  GL_WRAP(glActiveTexture(GL_TEXTURE1));
  GL_WRAP(glBindTexture(GL_TEXTURE_2D, d->g_buffer.depth_render_buffer));
  GL_WRAP(glUniform1i(d->debug_shader[program_idx].gbuffer_depth_loc, 1));

  GL_WRAP(glUniform1f(d->debug_shader[program_idx].z_near_loc, Z_NEAR));
  GL_WRAP(glUniform1f(d->debug_shader[program_idx].z_far_loc, Z_FAR));

  utility_draw_fullscreen_quad(d->debug_shader[program_idx].texcoord_loc, d->debug_shader[program_idx].pos_loc);
}

void deferred_render(Deferred *d, const Scene *s, const ShadowMap* sm) {
  GL_WRAP(glEnable(GL_CULL_FACE));
  GL_WRAP(glEnable(GL_TEXTURE_2D));
  GL_WRAP(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));
  GL_WRAP(glEnable(GL_DEPTH_TEST));
  GL_WRAP(glDepthFunc(GL_LEQUAL));

  gbuffer_bind(&d->g_buffer);

  utility_set_clear_color(0, 0, 0);
  GL_WRAP(glClearDepth(1.0f));
  GL_WRAP(glDisable(GL_BLEND));
  GL_WRAP(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

  for (int i = 0; i < SCENE_MODELS_MAX; i++) {
    if (s->models[i] && !s->models[i]->hidden)
      render_geometry(s->models[i], d, s);
  }

  if (d->render_mode == RENDER_MODE_SHADED) {
    render_shading(d, s, sm);
    render_skybox(d, s);
  } else {
    render_debug(d);
  }
}
