#pragma once
#include "common.h"

#include "deferred.h"
#include "forward.h"
#include "debug_lines.h"

typedef struct
{
  // deferred rendering state
  Deferred deferred;

  // forward rendering state
  Forward forward;

  // offscren shadow map render target + texture
  ShadowMap shadow_map;

  // if set, draws a picture-in-picture of the shadow map
  int debug_shadow_map;

  // if set, draws debug lines
  int render_debug_lines;
} Renderer;

int renderer_initialize(Renderer* r);
void renderer_render(Renderer* r, const Scene* scene);
