#include "scene.h"

void camera_update(Camera* camera, float dt) {
  if (camera->auto_rotate) {
    camera->rot[1] += dt * (float)M_PI/10.0f;
  }

  // calculate view matrix
  camera->pos[0] = camera->pos[1] = 0.0f;
  camera->pos[2] = camera->boom_len;
  mat4x4_identity(camera->view);
  mat4x4_rotate_X(camera->view, camera->view, 2.5f * camera->rot[0]);
  mat4x4_rotate_Y(camera->view, camera->view, 2.5f * camera->rot[1]);
  vec3_dup(camera->view[3], camera->pos);
  vec3_negate_in_place(camera->view[3]);

  // calculate view-projection matrix
  mat4x4_perspective(camera->proj, camera->fovy * (float)M_PI/180.0f, (float)VIEWPORT_WIDTH/(float)VIEWPORT_HEIGHT, Z_NEAR, Z_FAR);
  mat4x4_mul(camera->viewProj, camera->proj, camera->view);
}

void camera_forward(const Camera* camera, vec3 out) {
  vec4 temp;
  mat4x4 invView;
  mat4x4_invert(invView, camera->view);
  mat4x4_mul_vec4(temp, invView, Axis_Forward);
  vec3_dup(out, temp);
}

void camera_up(const Camera* camera, vec3 out) {
  vec4 temp;
  mat4x4 invView;
  mat4x4_invert(invView, camera->view);
  mat4x4_mul_vec4(temp, invView, Axis_Up);
  vec3_dup(out, temp);
}

void model_initialize(Model *out, const Mesh *mesh, const Material *mat) {
  memset(out, 0, sizeof(Model));
  out->scale = 1.0f;
  out->mesh = mesh;
  out->material = *mat;
}

void model_get_obb(const Model* model, OBB* out) {
  vec3_dup(out->center, model->position);
  if (model->mesh) {
    vec3_dup(out->extents, model->mesh->bounds.extents);
  } else {
    vec3_set(out->extents, 0.5f, 0.5f, 0.5f);
  }
  vec3_scale(out->extents, out->extents, model->scale * model->mesh->base_scale);
  mat4x4 m;
  mat4x4_identity(m);
  mat4x4_rotate_Z(m, m, DEG_TO_RAD(model->rot[2]));
  mat4x4_rotate_Y(m, m, DEG_TO_RAD(model->rot[1]));
  mat4x4_rotate_X(m, m, DEG_TO_RAD(model->rot[0]));
  mat4x4_mul_vec4(out->axes[0], m, Axis_X);
  mat4x4_mul_vec4(out->axes[1], m, Axis_Y);
  mat4x4_mul_vec4(out->axes[2], m, Axis_Z);
}

void scene_update(Scene* scene, float dt) {
  camera_update(&scene->camera, dt);

  // update emitters
  for (int i = 0; i < SCENE_EMITTERS_MAX; i++) {
    if (scene->emitters[i]) {
      particle_emitter_update(scene->emitters[i], dt);
    }
  }
}
