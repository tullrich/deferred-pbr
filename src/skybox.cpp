#include "common.h"
#include "skybox.h"

int skybox_load(Skybox *out_skybox, const SkyboxDesc *desc)
{
	if(!(out_skybox->env_cubemap = utility_load_texture_dds(desc->env_path))) {
		return 1;
	}
	if (desc->irr_paths[0]) {
		if (!(out_skybox->irr_cubemap = utility_load_cubemap(desc->irr_paths)))
			return 1;
	} else {
		out_skybox->irr_cubemap = out_skybox->env_cubemap;
	}
	return 0;
}
