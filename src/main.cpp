#include "common.h"
#include "particles.h"
#include "scene.h"
#include "deferred.h"
#include "forward.h"
#include "ibl.h"

#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/ImGuizmo.h"

static Scene gScene;
static Deferred gDeferred;
static Forward gForward;

static ParticleEmitterDesc gEmitterDesc;
static ParticleEmitter gEmitter;

static int rotate_cam = 0;
static int show_manipulator = 0;
static int skybox_idx = 0;
static int material_idx = 0;
static int mesh_idx = 0;

static SkyboxDesc gSkyboxes[] = {
	{
		.name = "Saint Peters Basilica",
		.paths = CUBEMAP_FILEPATHS("images/SaintPetersBasilica", ".jpg"),
		.low_paths = CUBEMAP_FILEPATHS("images/SaintPetersBasilica_low", ".jpg"),
		.irr_paths = CUBEMAP_FILEPATHS("images/SaintPetersBasilica_low", "_irradiance.png")
	},
	{
		.name = "San Francisco",
		.paths = CUBEMAP_FILEPATHS("images/SanFrancisco4", ".jpg"),
		.low_paths = CUBEMAP_FILEPATHS("images/SanFrancisco4_low", ".jpg"),
		.irr_paths = CUBEMAP_FILEPATHS("images/SanFrancisco4_low", "_irradiance.png")
	},
	{
		.name = "UV Debug",
		.paths = {
			"images/uv_map.png",
			"images/uv_map.png",
			"images/uv_map.png",
			"images/uv_map.png",
			"images/uv_map.png",
			"images/uv_map.png",
		}
	}
};

static MeshDesc gMeshes[] = {
	{ .name = "Box" },
	{ .name = "Sphere" },
	{ .name = "Buddha", .path = "meshes/buddha/buddha.obj" },
	{ .name = "Dragon", .path = "meshes/dragon/dragon.obj" },
	{ .name = "Bunny", .path = "meshes/bunny/bunny.obj" },
	{ .name = "Bunny UV", .path = "meshes/bunny_uv/bunny_uv.obj" },
	{ .name = "None" }
};

static MaterialDesc gMaterials[] = {
	{
		.name = "Sci-Fi Cube",
		.albedo_map_path = "images/SciFiCube/Sci_Wall_Panel_01_basecolor.jpeg",
		.normal_map_path = "images/SciFiCube/Sci_Wall_Panel_01_normal.jpeg",
		.metalness_map_path = "images/SciFiCube/Sci_Wall_Panel_01_metallic_rgb.png",
		.albedo_base = { 1.0f,  1.0f,  1.0f },
		.metalness_base = { 0.0f, 0.0f, 0.0f },
		.roughness_base = { 1.0f,  1.0f,  1.0f }
	},
	{
		.name = "Medievil",
		.albedo_map_path = "images/Medievil/Medievil Stonework - Color Map.png",
		.normal_map_path = "images/Medievil/Medievil Stonework - (Normal Map).png",
		.ao_map_path = "images/Medievil/Medievil Stonework - AO Map.png",
		.albedo_base = { 1.0f,  1.0f,  1.0f },
		.metalness_base = { 0.0f, 0.0f, 0.0f },
		.roughness_base = { 1.0f,  1.0f,  1.0f }
	},
	{
		.name = "Moorish Lattice",
		.albedo_map_path = "images/MoorishLattice/moorish_lattice_diffuse.png",
		.normal_map_path = "images/MoorishLattice/moorish_lattice_normal.png",
		.albedo_base = { 1.0f,  1.0f,  1.0f },
		.metalness_base = { 0.0f, 0.0f, 0.0f },
		.roughness_base = { 1.0f,  1.0f,  1.0f }
	},
	{
		.name = "Terracotta",
		.albedo_map_path = "images/Terracotta/terracotta_diffuse.png",
		.normal_map_path = "images/Terracotta/terracotta_normal.png",
		.albedo_base = { 1.0f,  1.0f,  1.0f },
		.metalness_base = { 0.0f, 0.0f, 0.0f },
		.roughness_base = { 1.0f,  1.0f,  1.0f }
	},
	{
		.name = "UV Debug",
		.albedo_map_path = "images/uv_map2.png",
		.albedo_base = { 1.0f,  1.0f,  1.0f },
		.metalness_base = { 0.0f, 0.0f, 0.0f },
		.roughness_base = { 1.0f,  1.0f,  1.0f }
	}
};

static ParticleEmitterTextureDesc gParticleTextures[] = {
	{ .name = "Flare", .path = "images/particles/flare.png" },
	{ .name = "Particle", .path = "images/particles/particle.png"},
	{ .name = "Smoke", .path = "images/particles/smoke.png" },
	{ .name = "Divine", .path = "images/particles/divine.png" },
	{ .name = "UV Debug", .path = "images/uv_map.png" }
};

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

static int initialize_particle_rendering() {
	for (int i = 0; i < STATIC_ELEMENT_COUNT(gParticleTextures); i++) {
		if ((gParticleTextures[i].texture = utility_load_image(GL_TEXTURE_2D, gParticleTextures[i].path)) < 0) {
			return 1;
		}
	}

	for (int i = 0; i < STATIC_ELEMENT_COUNT(gEmitterDescs); i++) {
		int idx = (i < STATIC_ELEMENT_COUNT(gParticleTextures)) ? i : 0;
		gEmitterDescs[i].texture = gParticleTextures[idx].texture;
	}
	return 0;
}

static int initialize_meshes() {
	mesh_make_box(&gMeshes[0].mesh, 5.0f);
	mesh_sphere_tessellate(&gMeshes[1].mesh, 2.5f, 100, 100);
	for (int i = 2; i < (STATIC_ELEMENT_COUNT(gMeshes) - 1); i++) {
		if (mesh_load_obj(&gMeshes[i].mesh, gMeshes[i].path)) {
			return 1;
		}
	}
	return 0;
}

static int initialize_materials() {
	for (int i = 0; i < STATIC_ELEMENT_COUNT(gMaterials); i++) {
		MaterialDesc* desc = &gMaterials[i];
		if (!desc->albedo_map_path || !(desc->material.albedo_map = utility_load_image(GL_TEXTURE_2D, desc->albedo_map_path)))
			desc->material.albedo_map = utility_load_texture_unknown();
		if (!desc->normal_map_path || !(desc->material.normal_map = utility_load_image(GL_TEXTURE_2D, desc->normal_map_path)))
			desc->material.normal_map = utility_load_texture_unknown();
		if (!desc->metalness_map_path || !(desc->material.metalness_map = utility_load_image(GL_TEXTURE_2D, desc->metalness_map_path)))
			desc->material.metalness_map = utility_load_texture_unknown();
		if (!desc->roughness_map_path || !(desc->material.roughness_map = utility_load_image(GL_TEXTURE_2D, desc->roughness_map_path)))
			desc->material.roughness_map = utility_load_texture_unknown();
		if (!desc->ao_map_path || !(desc->material.ao_map = utility_load_image(GL_TEXTURE_2D, desc->ao_map_path)))
			desc->material.ao_map = utility_load_texture_unknown();
		if (!desc->emissive_map_path || !(desc->material.emissive_map = utility_load_image(GL_TEXTURE_2D, desc->emissive_map_path)))
			desc->material.emissive_map = utility_load_texture_unknown();
		vec3_dup(desc->material.albedo_base, desc->albedo_base);
		vec3_dup(desc->material.metalness_base, desc->metalness_base);
		vec3_dup(desc->material.roughness_base, desc->roughness_base);
	}
	return 0;
}

static int initialize_skybox_textures() {
	for (int i = 0; i < STATIC_ELEMENT_COUNT(gSkyboxes); i++)
	{
		//ibl_compute_irradiance_map(gSkyboxes[i].low_paths);
		if(!(gSkyboxes[i].skybox.env_cubemap = utility_load_cubemap(gSkyboxes[i].paths)))
			return 1;
		if (gSkyboxes[i].irr_paths[0]) {
			if (!(gSkyboxes[i].skybox.irr_cubemap = utility_load_cubemap(gSkyboxes[i].irr_paths)))
				return 1;
		} else {
			gSkyboxes[i].skybox.irr_cubemap = gSkyboxes[i].skybox.env_cubemap;
		}
	}
	return 0;
}

static int init_scene() {
	memset(&gScene, 0, sizeof(Scene));

	// Setup camera
	gScene.camera.boomLen = 30.0f;
	gScene.camera.fovy = 90.0f;

	// Setup ambient light
	vec3_dup(gScene.ambient_color, White);
	gScene.ambient_intensity = 1.f;

	// Setup main directional light
	vec3_dup(gScene.main_light.color, White);
	gScene.main_light.intensity = 1.0f;
	gScene.main_light.position[0] = 5.0f;
	gScene.main_light.position[1] = 10.0f;
	gScene.main_light.position[2] = 5.0f;

	// Setup skybox
	gScene.skybox = gSkyboxes[skybox_idx].skybox;

	// Setup start mesh
	gScene.mesh = gMeshes[mesh_idx].mesh;
	gScene.model_scale = 1.0f;

	// Setup start material
	gScene.material = gMaterials[material_idx].material;

	// Setup particle system
	memset(&gEmitter, 0, sizeof(ParticleEmitter));
	memset(&gEmitterDesc, 0, sizeof(ParticleEmitterDesc));
	gEmitterDesc = gEmitterDescs[0];
	particle_emitter_initialize(&gEmitter, &gEmitterDesc);
	gScene.emitters[0] = &gEmitter;
	gEmitter.muted = true; // start muted

	return 0;
}

static int initialize() {
	int err = 0;
	if (err = deferred_initialize(&gDeferred)) {
		printf("deferred rendering init failed\n");
		return err;
	}
	printf("deferred initialized\n");

	if (err = initialize_skybox_textures()) {
		printf("skyboxes init failed\n");
		return err;
	}
	printf("skyboxes initialized\n");

	if (err = forward_initialize(&gForward)) {
		printf("forward rendering init failed\n");
		return err;
	}
	gForward.g_buffer = &gDeferred.g_buffer;
	printf("forward initialized\n");

	if (err = initialize_particle_rendering()) {
		printf("particle rendering init failed\n");
		return err;
	}
	printf("particle rendering initialized\n");

	if (err = initialize_meshes()) {
		printf("mesh loading failed\n");
		return err;
	}
	printf("meshes loaded\n");

	if (err = initialize_materials()) {
		printf("material loading failed\n");
		return err;
	}
	printf("materials loaded\n");

	if (err = init_scene()) {
		printf("scene init failed\n");
		return err;
	}
	printf("scene initialized\n");

	GL_CHECK_ERROR();
	return 0;
}

static void update_scene(float dt) {
	if (rotate_cam) {
		gScene.camera.rot[1] += dt * (float)M_PI/10.0f;
	}

	// calculate view matrix
	gScene.camera.pos[0] = gScene.camera.pos[1] = 0.0f;
	gScene.camera.pos[2] = gScene.camera.boomLen;
	mat4x4_identity(gScene.camera.view);
	mat4x4_rotate_X(gScene.camera.view, gScene.camera.view, 2.5f * gScene.camera.rot[0]);
	mat4x4_rotate_Y(gScene.camera.view, gScene.camera.view, 2.5f * -gScene.camera.rot[1]);
	vec3_dup(gScene.camera.view[3], gScene.camera.pos);
	vec3_negate_in_place(gScene.camera.view[3]);

	// calculate view-projection matrix
	mat4x4_perspective(gScene.camera.proj, gScene.camera.fovy * (float)M_PI/180.0f, (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, Z_NEAR, Z_FAR);
	mat4x4_mul(gScene.camera.viewProj, gScene.camera.proj, gScene.camera.view);

	vec3 cone_axis;
	vec3_dup(cone_axis, gEmitterDesc.emit_cone_axis);
	vec3_norm(gEmitterDesc.emit_cone_axis, cone_axis);
}

static int frame() {
	// calc dt
	static float prevFrameTime = 0.0f;
	float frameTime = utility_secs_since_launch();
	float dt = frameTime - prevFrameTime;
	prevFrameTime = frameTime;

	// update camera
	update_scene(dt);

	// update emitter
	particle_emitter_update(&gEmitter, dt);

	// clear backbuffer
	utility_set_clear_color(0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// render
	deferred_render(&gDeferred, &gScene);
	forward_render(&gForward, &gScene);

	ImGui::SetNextWindowSize( ImVec2( 200, 100 ), ImGuiCond_FirstUseEver );
	ImGui::Begin( "Controls", 0);

	if ( ImGui::CollapsingHeader( "Renderer", ImGuiTreeNodeFlags_DefaultOpen ) ) {
		ImGui::Combo( "Render Mode", ( int* )&gDeferred.render_mode, render_mode_strings, render_mode_strings_count );
	}
	if ( ImGui::CollapsingHeader( "Scene", ImGuiTreeNodeFlags_DefaultOpen ) ) {
		ImGui::Checkbox( "Cam Rotate", ( bool* )&rotate_cam );
		ImGui::SliderFloat("Cam Zoom", (float*)&gScene.camera.boomLen, 0.0f, 150.0f);
		ImGui::SliderFloat("FOVy", (float*)&gScene.camera.fovy, 0.0f, 180.0f);

		if (ImGui::BeginCombo("Geometry", gMeshes[mesh_idx].name, 0))
    {
      for (int i = 0; i < STATIC_ELEMENT_COUNT(gMeshes); i++)
      {
        if (ImGui::Selectable(gMeshes[i].name, (mesh_idx == i)))
				{
					mesh_idx = i;
					gScene.mesh = gMeshes[i].mesh;
				}
        if ((mesh_idx == i))
				{
          ImGui::SetItemDefaultFocus();
				}
      }
      ImGui::EndCombo();
    }
		ImGui::ColorEdit3( "Ambient Color", gScene.ambient_color );
		ImGui::SliderFloat( "Ambient Intensity", &gScene.ambient_intensity, 0, 1.0f );

		ImGui::InputFloat3( "Light Position", gScene.main_light.position );
		ImGui::ColorEdit3( "Light Color", gScene.main_light.color );
		ImGui::SliderFloat( "Light Intensity", &gScene.main_light.intensity, 0, 1.0f );
		ImGui::Checkbox("Show Manipulator", (bool*)&show_manipulator);
		if (ImGui::BeginCombo("Skybox", gSkyboxes[skybox_idx].name, 0))
		{
			for (int i = 0; i < STATIC_ELEMENT_COUNT(gSkyboxes); i++)
			{
				if (ImGui::Selectable(gSkyboxes[i].name, (skybox_idx == i)))
				{
					skybox_idx = i;
					gScene.skybox = gSkyboxes[i].skybox;
				}
				if ((skybox_idx == i))
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SliderFloat3("Model Position", gScene.model_translation, 0.0f, 25.0f, "%.0f");
		ImGui::SliderFloat("Model Rotation (Deg)", &gScene.model_rot[1], 0.0f, 360.0f, "%.0f");
		ImGui::SliderFloat("Model Scale", &gScene.model_scale, .01f, 25.0f );
	}
	if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::ColorEdit3("Albedo", gScene.material.albedo_base);
		ImGui::SliderFloat("Roughness", &gScene.material.roughness_base[0],  0.0f, 1.0f);
		ImGui::SliderFloat("Metalness", &gScene.material.metalness_base[0],  0.0f, 1.0f);
		if (ImGui::BeginCombo("Textures", gMaterials[material_idx].name, 0))
		{
			for (int i = 0; i < STATIC_ELEMENT_COUNT(gMaterials); i++)
			{
				if (ImGui::Selectable(gMaterials[i].name, (material_idx == i)))
				{
					material_idx = i;
					gScene.material = gMaterials[i].material;
				}
				if ((material_idx == i))
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}
	if ( ImGui::CollapsingHeader("Emitter", ImGuiTreeNodeFlags_DefaultOpen)) {
		particle_emitter_gui(&gEmitterDesc, &gEmitter, gParticleTextures, STATIC_ELEMENT_COUNT( gParticleTextures ));
		if ( ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
			if ( ImGui::Button("Flare") ) {
				gEmitterDesc = gEmitterDescs[0];
				particle_emitter_refresh(&gEmitter);
			}
			if ( ImGui::Button("Particle") ) {
				gEmitterDesc = gEmitterDescs[1];
				particle_emitter_refresh(&gEmitter);
			}
			if ( ImGui::Button("Smoke") ) {
				gEmitterDesc = gEmitterDescs[2];
				particle_emitter_refresh(&gEmitter);
			}
		}
	}
	ImGuizmo::SetDrawlist();
	ImGui::End();

	// Render light manipulator gizmo
	if (show_manipulator) {
		mat4x4 light_mat;
		mat4x4_identity(light_mat);
		vec3_dup(light_mat[3], gScene.main_light.position);
		ImGuizmo::Enable(true);
		ImGuizmo::SetRect(0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
		ImGuizmo::Manipulate(
			&gScene.camera.view[0][0],
			&gScene.camera.proj[0][0],
			ImGuizmo::TRANSLATE,
			ImGuizmo::LOCAL,
			&light_mat[0][0]);
		vec3_dup(gScene.main_light.position, light_mat[3]);
	}

	return 0;
}

int main(int argc, char* argv[]) {

	SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,            8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,          8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,           8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,          8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,          16);
  SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,         32);
  SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,      8);
  SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,    8);
  SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,     8);
  SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,    8);


	SDL_Window*	window;
  if(!(window = SDL_CreateWindow("Renderer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
		, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL))) {
      return 1;
  }

	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	glewExperimental = 1;
	glewInit();

	// Initialize Imgui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplSDL2_InitForOpenGL(window, glcontext);
  ImGui_ImplOpenGL3_Init("#version 130");

	srand((unsigned)time(0));

	SDL_GL_SwapWindow(window); // clear window

	if(initialize()) {
		printf("Failed to initialize. Exiting.\n");
		return 1;
	}

	// Enable vsync
	SDL_GL_SetSwapInterval(1);

	// The window is open: enter program loop (see SDL_PollEvent)
	int quit = 0;
	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {

			if (ImGui::GetIO().WantCaptureMouse) {
				if (ImGui_ImplSDL2_ProcessEvent(&event)) {
					continue;
				}
			}
			else {
				if (event.type == SDL_MOUSEBUTTONDOWN) {
					if (event.button.button == SDL_BUTTON_LEFT) {
						SDL_SetWindowGrab(window, SDL_TRUE);
					}
				}
				else if (event.type == SDL_MOUSEBUTTONUP) {
					if (event.button.button == SDL_BUTTON_LEFT) {
						SDL_SetWindowGrab(window, SDL_FALSE);
					}
				}
				else if (event.type == SDL_MOUSEMOTION) {
					if (event.motion.state & SDL_BUTTON_LMASK) {
						gScene.camera.rot[1] += event.motion.xrel / (float)WINDOW_WIDTH;
						gScene.camera.rot[0] += event.motion.yrel / (float)WINDOW_HEIGHT;
					}
				}
				else if (event.type == SDL_MOUSEWHEEL) {
					if (event.wheel.y == 1)  {
						gScene.camera.boomLen -= 2.f;
					}
					else if (event.wheel.y == -1) {
						gScene.camera.boomLen +=  2.f;
					}
				}
			}
			if (event.type == SDL_QUIT) {
				quit = 1;
				break;
			}
		}

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

		if (frame())
		{
			quit = 1;
			break;
		}

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		GL_CHECK_ERROR();

		// swap
		SDL_GL_SwapWindow(window);
	}

	// Clean up
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
