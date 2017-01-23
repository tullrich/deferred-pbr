#include "scene.h"

void scene_camera_forward(const Scene* s, vec3 out) {
	vec4 temp;
	mat4x4 invView;
	mat4x4_invert(invView, s->camera.view);
	mat4x4_mul_vec4(temp, invView, Axis_Forward);
	vec3_dup(out, temp);
}

void scene_camera_up(const Scene* s, vec3 out) {
	vec4 temp;
	mat4x4 invView;
	mat4x4_invert(invView, s->camera.view);
	mat4x4_mul_vec4(temp, invView, Axis_Up);
	vec3_dup(out, temp);
}
