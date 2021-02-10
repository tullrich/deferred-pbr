#pragma once

#include "common.h"
#include "scene.h"

extern SkyboxDesc gSkyboxes[];
extern const int gSkyboxesCount;

extern MeshDesc gMeshes[];
extern const int gMeshesCount;

extern MaterialDesc gMaterials[];
extern const int gMaterialsCount;

extern ParticleEmitterTextureDesc gParticleTextures[];
extern const int gParticleTexturesCount;

extern ParticleEmitterDesc gEmitterDescs[];
extern const int gEmitterDescsCount;

typedef void(*LoadingCallback)(const char* stage, const char* asset, int index, int total);
int initialize_assets(LoadingCallback update_loading_cb);
