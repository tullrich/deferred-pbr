#include "assets.h"

SkyboxDesc gSkyboxes[] = {
	{
		.name = "Saint Peters Basilica",
    .env_path = "environments/SaintPetersBasilica/SaintPetersBasilica_Env_512.dds",
    .irr_path = "environments/SaintPetersBasilica/SaintPetersBasilica_Irr.dds",
    .prefilter_path = "environments/SaintPetersBasilica/SaintPetersBasilica_Prefilter.dds"
	},
	{
		.name = "San Francisco",
    .env_path = "environments/SanFrancisco4/SanFrancisco4_Env_512.dds",
    .irr_path = "environments/SanFrancisco4/SanFrancisco4_Irr.dds",
    .prefilter_path = "environments/SanFrancisco4/SanFrancisco4_Prefilter.dds"
	},
	{
		.name = "UV Debug",
    .env_path = "environments/UVDebug/UVDebug_Env.dds",
	}
};
const int gSkyboxesCount = STATIC_ELEMENT_COUNT(gSkyboxes);

MeshDesc gMeshes[] = {
  { .name = "Sphere" },
	{ .name = "Box" },
	{ .name = "Buddha", .path = "meshes/buddha/buddha.obj" },
	{ .name = "Dragon", .path = "meshes/dragon/dragon.obj" },
	{ .name = "Bunny", .path = "meshes/bunny/bunny.obj" },
	{ .name = "Bunny UV", .path = "meshes/bunny_uv/bunny_uv.obj", .base_scale = 50.0f },
	{ .name = "None" }
};
const int gMeshesCount = STATIC_ELEMENT_COUNT(gMeshes);

MaterialDesc gMaterials[] = {
	{
		.name = "Mirror",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 1.0f,
		.roughness_base = 0.05f,
		.emissive_base = { 0.0f, 0.0f, 0.0f }
	},
	{
		.name = "Sci-Fi Cube",
		.albedo_map_path = "materials/SciFiCube/Sci_Wall_Panel_01_basecolor.jpeg",
		.normal_map_path = "materials/SciFiCube/Sci_Wall_Panel_01_normal.jpeg",
		.height_map_path = "materials/SciFiCube/Sci_Wall_Panel_01_height.png",
		.metalness_map_path = "materials/SciFiCube/Sci_Wall_Panel_01_metallic_rgb.png",
		.roughness_map_path = "materials/SciFiCube/Sci_Wall_Panel_01_roughness.jpeg",
		.emissive_map_path = "materials/SciFiCube/Sci_Wall_Panel_01_emissive.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 1.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 1.0f, 1.0f, 1.0f },
    .height_map_scale = 0.015f
	},
	{
		.name = "Gold",
		.albedo_map_path = "materials/Gold/lightgold_albedo.png",
		.normal_map_path = "materials/Gold/lightgold_normal-dx.png",
		.metalness_map_path = "materials/Gold/lightgold_metallic.png",
		.roughness_map_path = "materials/Gold/lightgold_roughness.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 1.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 0.0f, 0.0f, 0.0f }
	},
	{
		.name = "Metal Ventilation",
		.albedo_map_path = "materials/MetalVentilation/metal-ventilation1-albedo.png",
		.normal_map_path = "materials/MetalVentilation/metal-ventilation1-normal-dx.png",
		.height_map_path = "materials/MetalVentilation/metal-ventilation1-height.png",
		.metalness_map_path = "materials/MetalVentilation/metal-ventilation1-metallic.png",
		.roughness_map_path = "materials/MetalVentilation/metal-ventilation1-roughness.png",
		.ao_map_path = "materials/MetalVentilation/metal-ventilation1-ao.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 1.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 0.0f, 0.0f, 0.0f },
    .height_map_scale = 0.1f
	},
  {
		.name = "Harsh Brick",
		.albedo_map_path = "materials/HarshBricks/harshbricks-albedo.png",
		.normal_map_path = "materials/HarshBricks/harshbricks-normal.png",
		.height_map_path = "materials/HarshBricks/harshbricks-height5-16.png",
		.metalness_map_path = "materials/HarshBricks/harshbricks-metalness.png",
		.roughness_map_path = "materials/HarshBricks/harshbricks-roughness.png",
		.ao_map_path = "materials/HarshBricks/harshbricks-ao2.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 1.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 0.0f, 0.0f, 0.0f },
    .height_map_scale = 0.1f
	},
  {
		.name = "Wrinkled Paper",
		.albedo_map_path = "materials/WrinkledPaper/wrinkled-paper-albedo.png",
		.normal_map_path = "materials/WrinkledPaper/wrinkled-paper-normal-dx.png",
		.height_map_path = "materials/WrinkledPaper/wrinkled-paper-height.png",
		.metalness_map_path = "materials/WrinkledPaper/wrinkled-paper-metalness.png",
		.roughness_map_path = "materials/WrinkledPaper/wrinkled-paper-roughness.png",
		.ao_map_path = "materials/WrinkledPaper/wrinkled-paper-ao.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 1.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 0.0f, 0.0f, 0.0f },
    .height_map_scale = 0.1f
	},
  {
		.name = "Snow Covered Path",
		.albedo_map_path = "materials/SnowCoveredPath/snowcoveredpath_albedo.png",
		.normal_map_path = "materials/SnowCoveredPath/snowcoveredpath_normal-dx.png",
		.height_map_path = "materials/SnowCoveredPath/snowcoveredpath_height.png",
		.metalness_map_path = "materials/SnowCoveredPath/snowcoveredpath_metallic.png",
		.roughness_map_path = "materials/SnowCoveredPath/snowcoveredpath_roughness.png",
		.ao_map_path = "materials/SnowCoveredPath/snowcoveredpath_ao.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 1.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 0.0f, 0.0f, 0.0f },
    .height_map_scale = 0.1f
	},
	{
		.name = "Medievil",
		.albedo_map_path = "materials/Medievil/Medievil Stonework - Color Map.png",
		.normal_map_path = "materials/Medievil/Medievil Stonework - (Normal Map).png",
		.ao_map_path = "materials/Medievil/Medievil Stonework - AO Map.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 0.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 0.0f, 0.0f, 0.0f },
    .height_map_scale = 1.0f
	},
	{
		.name = "Moorish Lattice",
		.albedo_map_path = "materials/MoorishLattice/moorish_lattice_diffuse.png",
		.normal_map_path = "materials/MoorishLattice/moorish_lattice_normal.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 0.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 0.0f, 0.0f, 0.0f }
	},
	{
		.name = "Terracotta",
		.albedo_map_path = "materials/Terracotta/terracotta_diffuse.png",
		.normal_map_path = "materials/Terracotta/terracotta_normal.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 0.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 0.0f, 0.0f, 0.0f }
	},
	{
		.name = "UV Debug",
		.albedo_map_path = "uv_map.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 0.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 0.0f, 0.0f, 0.0f }
	}
};
const int gMaterialsCount = STATIC_ELEMENT_COUNT(gMaterials);

ParticleEmitterTextureDesc gParticleTextures[] = {
	{ .name = "Flare", .path = "particles/flare.png" },
	{ .name = "Particle", .path = "particles/particle.png"},
	{ .name = "Smoke", .path = "particles/smoke.png" },
	{ .name = "Divine", .path = "particles/divine.png" },
	{ .name = "UV Debug", .path = "uv_map.png" }
};
const int gParticleTexturesCount = STATIC_ELEMENT_COUNT(gParticleTextures);

ParticleEmitterDesc gEmitterDescs[] = {
	{
		.max = 1024,
		.spawn_rate = 60.0f,
		.start_color = { Yellow[0], Yellow[1], Yellow[2], Yellow[3] },
		.end_color = { Red[0], Red[1], Red[2], Red[3] },
		.orient_mode = PARTICLE_ORIENT_SCREEN_ALIGNED,
		.speed = 8.0f, .speed_variance = 5.0f,
		.life_time = 2.5f, .life_time_variance = 0.5f,
		.burst_count = 150,
		.shading_mode = PARTICLE_SHADING_TEXTURED,
		.start_scale = 1.0f, .end_scale = 0.1f,
		.depth_sort_alpha_blend = 0,
		.soft = 0,
		.simulate_gravity = 0,
		.emit_cone_axis = { Axis_Right[0], Axis_Right[1], Axis_Right[2] }
	},
	{
		.max = 1024,
		.spawn_rate = 60.0f,
		.start_color = { 100/255.0f, 100/255.0f, 1.0f, 1.0f },
		.end_color = { 1.0f, 1.0f, 1.0f, 0.0f },
		.orient_mode = PARTICLE_ORIENT_SCREEN_ALIGNED,
		.speed = 4.0f, .speed_variance = 1.0f,
		.life_time = 2.5f, .life_time_variance = 0.5f,
		.burst_count = 150,
		.shading_mode = PARTICLE_SHADING_TEXTURED,
		.start_scale = 0.1f, .end_scale = 0.5,
		.depth_sort_alpha_blend = 0,
		.soft = 0,
		.simulate_gravity = 0,
		.emit_cone_axis = { Axis_Up[0], Axis_Up[1], Axis_Up[2] }
	},
	{
		.max = 1024,
		.spawn_rate = 24.0f,
		.start_color = { Green[0], Green[1], Green[2], 0.4f },
		.end_color = { 1.0f, 1.0f, 1.0f, 0.0f },
		.orient_mode = PARTICLE_ORIENT_SCREEN_ALIGNED,
		.speed = 3.0f, .speed_variance = 0.0f,
		.life_time = 3.0f, .life_time_variance = 0.0f,
		.burst_count = 150,
		.shading_mode = PARTICLE_SHADING_TEXTURED,
		.start_scale = 2.0f, .end_scale = 4.5f,
		.depth_sort_alpha_blend = 1,
		.soft = 0,
		.simulate_gravity = 0,
		.emit_cone_axis = { Axis_Up[0], Axis_Up[1], Axis_Up[2] }
	}
};
const int gEmitterDescsCount = STATIC_ELEMENT_COUNT(gEmitterDescs);
