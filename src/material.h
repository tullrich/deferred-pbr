#pragma once

struct MaterialDesc;

struct Material
{
  vec3 albedo_base;
  float metalness_base;
  float roughness_base;
  vec3 emissive_base;
  float height_map_scale;
  GLuint albedo_map;
  GLuint normal_map;
  GLuint height_map;
  GLuint metalness_map;
  GLuint roughness_map;
  GLuint emissive_map;
  GLuint ao_map;
  const MaterialDesc* desc;
};

struct MaterialDesc
{
  const char* name;
  const char* albedo_map_path;
  const char* normal_map_path;
  const char* height_map_path;
  const char* metalness_map_path;
  const char* roughness_map_path;
  const char* emissive_map_path;
  const char* ao_map_path;
  vec3 albedo_base;
  float metalness_base;
  float roughness_base;
  vec3 emissive_base;
  float height_map_scale;
  Material material;
  int use_point_sampling;
};

int material_initialize_default(Material *out);
int material_load(Material *out_material, const MaterialDesc *desc);
void material_gui(Material* material);
