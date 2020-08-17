#pragma once

typedef struct
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

} Material;

typedef struct
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
} MaterialDesc;

int material_initialize_default(Material *out);
int material_initialize(Material *out_material, const MaterialDesc *desc);
void material_gui(Material* material, int *material_idx, const MaterialDesc* mat_defs, int mat_defs_count);
