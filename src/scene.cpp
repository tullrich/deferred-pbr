#include "scene.h"

void model_initialize(Model *out, const Mesh *mesh, const Material *mat) {
	memset(out, 0, sizeof(Model));
	out->scale = 1.0f;
	out->mesh = *mesh;
	out->material = *mat;
}

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
