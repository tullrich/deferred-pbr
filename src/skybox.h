#pragma once

// Skybox renderable
typedef struct
{
	// skybox cubemap
	GLuint env_cubemap;

	// irradiance map (pre-convolved env_cubemap)
	GLuint irr_cubemap;
} Skybox;

// Skybox load description
typedef struct
{
	const char* name;
  const char* env_path;
	const char* low_paths[6];
	const char* irr_paths[6];
	Skybox skybox;
} SkyboxDesc;

int skybox_load(Skybox *out_skybox, const SkyboxDesc *desc);
