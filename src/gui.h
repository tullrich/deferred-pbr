#pragma once
#include "common.h"
#include "scene.h"
#include "renderer.h"

void gui_initialize(SDL_Window* window);
void gui_destroy();
void gui_render_loading_screen(SDL_Window* window, const char* stage, const char* asset, int index, int total);

void gui_begin_frame(SDL_Window* window);
void gui_end_frame();
int gui_process_event(SDL_Event* event);

void gui_rotation_gizmo(quat out_euler, const vec3 pos, const mat4x4 view, const mat4x4 proj);
void gui_translation_gizmo(vec3 out_pos, const mat4x4 view, const mat4x4 proj);
void gui_render(SDL_Window* window, Renderer* gRenderer, Scene* scene, float dt);
