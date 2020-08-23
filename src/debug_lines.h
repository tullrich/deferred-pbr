#pragma once
#include "common.h"
#include "scene.h"

void debug_lines_submit_aabb(const Bounds* b, const vec3 rgb);
void debug_lines_submit_obb(const OBB* obb, const vec3 rgb);
void debug_lines_submit(const vec3 start, const vec3 end, const vec3 rgb);
void debug_lines_submit(float start_x, float start_y, float start_z
  , float end_x, float end_y, float end_z
  , const vec3 rgb);

int debug_lines_initialize();
void debug_lines_clear();
void debug_lines_render(const Scene *s);
