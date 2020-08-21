#pragma once

struct SkyboxDesc;

// Skybox renderable
struct Skybox
{
  // skybox cubemap
  GLuint env_cubemap;

  // irradiance map (pre-convolved env_cubemap)
  GLuint irr_cubemap;

  // prefiltered env map
  GLuint prefilter_cubemap;

  // The definition this was created from
  const SkyboxDesc* desc;
};

// Skybox load description
struct SkyboxDesc
{
  const char* name;
  const char* env_path;
  const char* irr_path;
  const char* prefilter_path;
  Skybox skybox;
};

int skybox_load(Skybox *out_skybox, const SkyboxDesc *desc);
