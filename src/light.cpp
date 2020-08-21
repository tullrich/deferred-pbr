#include "light.h"

DEFINE_ENUM(LightType, light_type_strings, ENUM_LightType);

void light_initialize_point(Light *out, const vec3 pos, const vec3 color, float intensity) {
  memset(out, 0, sizeof(Light));
  out->type = LIGHT_TYPE_POINT;
  vec3_dup(out->position, pos);
  out->position[3] = 1.0f;
  vec3_dup(out->color, color);
  quat_identity(out->rot);
  out->intensity = intensity;
}

void light_initialize_directional(Light *out, const vec3 pos, const vec3 color, float intensity) {
  memset(out, 0, sizeof(Light));
  out->type = LIGHT_TYPE_DIRECTIONAL;
  vec3_dup(out->position, pos);
  out->position[3] = 0.0f;
  vec3_dup(out->color, color);
  quat_identity(out->rot);
  out->intensity = intensity;
}

void light_gui(Light *light) {
	if (ImGui::Combo("Type", (int*)&light->type, light_type_strings, light_type_strings_count)) {
    switch (light->type) {
    case LIGHT_TYPE_POINT: light->position[3] = 1.0f; break;
    case LIGHT_TYPE_DIRECTIONAL: light->position[3] = 0.0f; break;
    }
  }
  ImGui::InputFloat3("Position", light->position);
  ImGui::ColorEdit3("Color", light->color);
  ImGui::SliderFloat("Intensity", &light->intensity, 0, 1.0f);
}
