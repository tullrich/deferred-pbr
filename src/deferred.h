#pragma once

#include "common.h"
#include "gbuffer.h"
#include "shadowmap.h"
#include "scene.h"

#define ENUM_RenderMode(D)								\
  D(RENDER_MODE_SHADED, 		"Shaded")			\
  D(RENDER_MODE_ALBEDO, 		"Albedo")			\
  D(RENDER_MODE_NORMAL, 		"Normal")			\
  D(RENDER_MODE_ROUGHNESS, 	"Roughness")	\
  D(RENDER_MODE_METALNESS, 	"Metalness")	\
  D(RENDER_MODE_DEPTH, 			"Depth")

DECLARE_ENUM(RenderMode, render_mode_strings, ENUM_RenderMode);

#define ENUM_SkyboxMode(D)					                 \
  D(SKYBOX_MODE_ENV_MAP, 		      "Env Map")		 	   \
  D(SKYBOX_MODE_IRR_MAP, 		      "Irradiance Map")  \
  D(SKYBOX_MODE_PREFILTER_MAP, 		"Prefilter Map")

DECLARE_ENUM(SkyboxMode, skybox_mode_strings, ENUM_SkyboxMode);

#define ENUM_TonemappingOperator(D)						   \
  D(TONEMAPPING_OP_REINHARD, 		"Reinhard")		   \
  D(TONEMAPPING_OP_UNCHARTED2, 		"Uncharted 2")

DECLARE_ENUM(TonemappingOperator, tonemapping_op_strings, ENUM_TonemappingOperator);

typedef struct
{
  GLuint program;

  // shader vars
  GLint pos_loc;
  GLint texcoord_loc;

  GLint env_map_loc;
  GLint lod_loc;

  GLint inv_vp_loc;
} SkyboxShader;

typedef struct
{
  GLuint program;

  // shader vars
  GLint pos_loc;
  GLint normal_loc;
  GLint tangent_loc;
  GLint texcoord_loc;

  GLint albedo_map_loc;
  GLint normal_map_loc;
  GLint height_map_loc;
  GLint metalness_map_loc;
  GLint roughness_map_loc;
  GLint ao_map_loc;
  GLint emissive_map_loc;

  GLint view_pos_loc;
  GLint model_view_loc;
  GLint model_view_proj_loc;
  GLint albedo_base_loc;
  GLint roughness_base_loc;
  GLint metalness_base_loc;
  GLint emissive_base_loc;
  GLint height_scale_loc;
} SurfaceShader;

typedef struct
{
  GLuint program;

  // shader vars
  GLint pos_loc;
  GLint texcoord_loc;

  union
  {
    struct
    {
      GLint gbuffer_normal_loc;
      GLint gbuffer_albedo_loc;
      GLint gbuffer_roughness_loc;
      GLint gbuffer_metalness_loc;
      GLint gbuffer_depth_loc;
    };
    GLint gbuffer_locs[GBUFFER_ATTACHMENTS_COUNT];
  };
  GLint env_irr_map_loc;
  GLint env_prefilter_map_loc;
  GLint env_brdf_lut_loc;
  GLint shadow_map_loc;

  // shader vars
  GLint ambient_term_loc;
  GLint light_pos_loc;
  GLint light_color_loc;
  GLint light_intensity_loc;
  GLint eye_pos_loc;
  GLint inv_view_loc;
  GLint inv_proj_loc;
  GLint light_space_loc;
} LightingShader;

typedef struct
{
  GLuint program;

  // shader vars
  GLint pos_loc;
  GLint texcoord_loc;
  GLint gbuffer_render_loc;
  GLint gbuffer_depth_loc;
  GLint z_near_loc;
  GLint z_far_loc;

} DebugShader;

typedef struct
{
  RenderMode render_mode;
  SkyboxMode skybox_mode;
  TonemappingOperator tonemapping_op;
  float prefilter_lod;
  SkyboxShader skybox_shader;
  SurfaceShader surf_shader[2];
  LightingShader lighting_shader[2];
  DebugShader debug_shader[3];
  Material default_mat;
  GBuffer g_buffer;
  GLuint brdf_lut_tex;
} Deferred;

int deferred_initialize(Deferred* d);
void deferred_render(Deferred* d, Scene *s, ShadowMap* sm);
