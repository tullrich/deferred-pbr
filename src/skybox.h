#pragma once

// Skybox renderable
typedef struct
{
	// skybox cubemap
	GLuint env_cubemap;

	// irradiance map (pre-convolved env_cubemap)
	GLuint irr_cubemap;

	// prefiltered env map
  GLuint prefilter_cubemap;
} Skybox;

// Skybox load description
typedef struct
{
	const char* name;
  const char* env_path;
  const char* irr_path;
  const char* prefilter_path;
	Skybox skybox;
} SkyboxDesc;

int skybox_load(Skybox *out_skybox, const SkyboxDesc *desc);
