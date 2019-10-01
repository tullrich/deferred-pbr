#include "common.h"
#include "material.h"

int material_load(Material *out_material, const MaterialDesc *desc)
{
  if (!desc->albedo_map_path || !(out_material->albedo_map = utility_load_image(GL_TEXTURE_2D, desc->albedo_map_path)))
    out_material->albedo_map = utility_load_texture_unknown();
  if (!desc->normal_map_path || !(out_material->normal_map = utility_load_image(GL_TEXTURE_2D, desc->normal_map_path)))
    out_material->normal_map = utility_load_texture_unknown();
  if (!desc->metalness_map_path || !(out_material->metalness_map = utility_load_image(GL_TEXTURE_2D, desc->metalness_map_path)))
    out_material->metalness_map = utility_load_texture_unknown();
  if (!desc->roughness_map_path || !(out_material->roughness_map = utility_load_image(GL_TEXTURE_2D, desc->roughness_map_path)))
    out_material->roughness_map = utility_load_texture_unknown();
  if (!desc->ao_map_path || !(out_material->ao_map = utility_load_image(GL_TEXTURE_2D, desc->ao_map_path)))
    out_material->ao_map = utility_load_texture_unknown();
  if (!desc->emissive_map_path || !(out_material->emissive_map = utility_load_image(GL_TEXTURE_2D, desc->emissive_map_path)))
    out_material->emissive_map = utility_load_texture_unknown();
  vec3_dup(out_material->albedo_base, desc->albedo_base);
  vec3_dup(out_material->metalness_base, desc->metalness_base);
  vec3_dup(out_material->roughness_base, desc->roughness_base);
  return 0;
}

void material_gui(Material* material, int *material_idx, const MaterialDesc* mat_defs, int mat_defs_count) {
  ImGui::ColorEdit3("Albedo", material->albedo_base);
  ImGui::SliderFloat("Roughness", &material->roughness_base[0],  0.0f, 1.0f);
  ImGui::SliderFloat("Metalness", &material->metalness_base[0],  0.0f, 1.0f);
  if (ImGui::BeginCombo("Textures", mat_defs[*material_idx].name, 0))
  {
    for (int i = 0; i < mat_defs_count; i++)
    {
      if (ImGui::Selectable(mat_defs[i].name, (*material_idx == i)))
      {
        *material_idx = i;
        *material = mat_defs[i].material;
      }
      if ((*material_idx == i))
      {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
}
