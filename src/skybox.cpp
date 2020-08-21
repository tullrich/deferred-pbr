#include "common.h"
#include "skybox.h"

int skybox_load(Skybox *out_skybox, const SkyboxDesc *desc) {
	if(!(out_skybox->env_cubemap = utility_load_texture_dds(desc->env_path))) {
		return 1;
	}
  if (desc->irr_path) {
    if(!(out_skybox->irr_cubemap = utility_load_texture_dds(desc->irr_path))) {
  		return 1;
  	}
  } else {
		out_skybox->irr_cubemap = out_skybox->env_cubemap;
	}
  if (desc->prefilter_path) {
    if(!(out_skybox->prefilter_cubemap = utility_load_texture_dds(desc->prefilter_path))) {
  		return 1;
  	}
  } else {
		out_skybox->prefilter_cubemap = out_skybox->env_cubemap;
	}
  out_skybox->desc = desc;
	return 0;
}
