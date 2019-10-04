#include "light.h"

DEFINE_ENUM(LightType, light_type_strings, ENUM_LightType);

void light_init_point(Light *out, const vec3 pos, const vec3 color, float intensity) {
  memset(out, 0, sizeof(Light));
  out->type = LIGHT_TYPE_POINT;
  vec3_dup(out->position, pos);
  out->position[3] = 1.0f;
  vec3_dup(out->color, color);
  out->intensity = intensity;
}

void light_init_directional(Light *out, const vec3 pos, const vec3 color, float intensity) {
  memset(out, 0, sizeof(Light));
  out->type = LIGHT_TYPE_DIRECTIONAL;
  vec3_dup(out->position, pos);
  out->position[3] = 0.0f;
  vec3_dup(out->color, color);
  out->intensity = intensity;
}
