#pragma once
#include "common.h"
#include "scene.h"

typedef struct
{
  GLuint program;

  // shader vars
  GLint pos_loc;
  GLint model_view_loc;
  GLint model_view_proj_loc;
} DepthRenderShader;

struct ShadowDebugShader
{
  GLuint program;

  // shader vars
  GLint pos_loc;
  GLint texcoord_loc;
  GLint render_map_loc;
  GLint depth_map_loc;
  GLint z_near_loc;
  GLint z_far_loc;
};

struct ShadowMap
{
  // pixel dimensions
  int width, height;

  // frame buffer
  GLuint fbo;

  // depth texture attachment
  GLuint depth_buffer;

  // Depth render shaders
  DepthRenderShader depth_render_shader;

  // Debug view shader
  ShadowDebugShader debug_shader;
};

int shadow_map_initialize(ShadowMap* shadow_map, int width, int height);
void shadow_map_render(ShadowMap* shadow_map, Scene* s);
void shadow_map_render_debug(const ShadowMap *shadow_map, int x_off, int y_off, int width, int height);
