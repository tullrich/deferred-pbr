#include "common.h"
#include "deferred.h"
#include "forward.h"
#include "scene.h"
#include "ibl.h"

#include "imgui/imgui_custom_theme.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/ImGuizmo.h"

#define SPHERE_SCENE 1

static Scene gScene;
static Deferred gDeferred;
static Forward gForward;

static Light gLight;
static Model gModel;
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
	{ .name = "Bunny UV", .path = "meshes/bunny_uv/bunny_uv.obj", .base_scale = 50.0f },
	{ .name = "None" }
};

static MaterialDesc gMaterials[] = {
	{
		.name = "Sci-Fi Cube",
		.albedo_map_path = "images/SciFiCube/Sci_Wall_Panel_01_basecolor.jpeg",
		.normal_map_path = "images/SciFiCube/Sci_Wall_Panel_01_normal.jpeg",
		.metalness_map_path = "images/SciFiCube/Sci_Wall_Panel_01_metallic_rgb.png",
		.roughness_map_path = "images/SciFiCube/Sci_Wall_Panel_01_roughness.jpeg",
		.emissive_map_path = "images/SciFiCube/Sci_Wall_Panel_01_emissive.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 1.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 1.0f, 1.0f, 1.0f }
	},
	{
		.name = "Medievil",
		.albedo_map_path = "images/Medievil/Medievil Stonework - Color Map.png",
		.normal_map_path = "images/Medievil/Medievil Stonework - (Normal Map).png",
		.ao_map_path = "images/Medievil/Medievil Stonework - AO Map.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 0.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 0.0f, 0.0f, 0.0f }
	},
	{
		.name = "Moorish Lattice",
		.albedo_map_path = "images/MoorishLattice/moorish_lattice_diffuse.png",
		.normal_map_path = "images/MoorishLattice/moorish_lattice_normal.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 0.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 0.0f, 0.0f, 0.0f }
	},
	{
		.name = "Terracotta",
		.albedo_map_path = "images/Terracotta/terracotta_diffuse.png",
		.normal_map_path = "images/Terracotta/terracotta_normal.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 0.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 0.0f, 0.0f, 0.0f }
	},
	{
		.name = "UV Debug",
		.albedo_map_path = "images/uv_map.png",
		.albedo_base = { 1.0f, 1.0f, 1.0f },
		.metalness_base = 0.0f,
		.roughness_base = 1.0f,
		.emissive_base = { 0.0f, 0.0f, 0.0f }
	}
};

static ParticleEmitterTextureDesc gParticleTextures[] = {
	{ .name = "Flare", .path = "images/particles/flare.png" },
	{ .name = "Particle", .path = "images/particles/particle.png"},
	{ .name = "Smoke", .path = "images/particles/smoke.png" },
	{ .name = "Divine", .path = "images/particles/divine.png" },
	{ .name = "UV Debug", .path = "images/uv_map.png" }
};

static ParticleEmitterDesc gEmitterDescs[] = {
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
		if ((gParticleTextures[i].texture = utility_load_texture(GL_TEXTURE_2D, gParticleTextures[i].path)) < 0) {
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
		if (mesh_load_obj(&gMeshes[i].mesh, gMeshes[i].path, gMeshes[i].base_scale)) {
			return 1;
		}
	}
	return 0;
}

static int initialize_materials() {
	int ret;
	for (int i = 0; i < STATIC_ELEMENT_COUNT(gMaterials); i++) {
		if ((ret = material_initialize(&gMaterials[i].material, &gMaterials[i]))) {
			return ret;
		}
	}
	return 0;
}

static int initialize_skyboxes() {
	int ret;
	for (int i = 0; i < STATIC_ELEMENT_COUNT(gSkyboxes); i++) {
		//ibl_compute_irradiance_map(gSkyboxes[i].low_paths);
		if ((ret = skybox_load(&gSkyboxes[i].skybox, &gSkyboxes[i]))) {
			return ret;
		}
	}
	return 0;
}

static int init_scene() {
	memset(&gScene, 0, sizeof(Scene));

	// Setup camera
	gScene.camera.boomLen = 30.0f;
	gScene.camera.fovy = 72.0f;

	// Setup ambient light
	vec3_dup(gScene.ambient_color, White);
	gScene.ambient_intensity = 0.03f;

	// Setup skybox
	gScene.skybox = gSkyboxes[skybox_idx].skybox;

	// Setup main directional light
	vec3 lightPos = { 0.0f, 0.0f, 10.0f };
	light_initialize_directional(&gLight, lightPos, White, 3.0f);
	gScene.light = &gLight;

	// Setup particle system
	memset(&gEmitter, 0, sizeof(ParticleEmitter));
	memset(&gEmitterDesc, 0, sizeof(ParticleEmitterDesc));
	gEmitterDesc = gEmitterDescs[0];
	particle_emitter_initialize(&gEmitter, &gEmitterDesc);
	gEmitter.muted = true; // start muted

#ifndef SPHERE_SCENE
	printf("Creating single model scene\n")
	model_initialize(&gModel, &gMeshes[mesh_idx].mesh, &gMaterials[material_idx].material);
	gScene.models[0] = &gModel;
	gScene.emitters[0] = &gEmitter;
#else
#define SPHERE_ROWS 7
#define SPHERE_COLUMNS 7
#define SPHERE_SPACING 8.0F
	printf("Creating %ix%i spheres scene\n", SPHERE_ROWS, SPHERE_COLUMNS);
	mesh_idx = 1;
	material_idx = 0;
	for (int i = 0; i < SPHERE_ROWS; i++) {
		for (int j = 0; j < SPHERE_COLUMNS; j++) {
			Model* m = (Model*)malloc(sizeof(Model));
			model_initialize(m, &gMeshes[1].mesh, &gMaterials[0].material);
			m->position[0] = (j-(SPHERE_COLUMNS/2)) * SPHERE_SPACING;
			m->position[1] = (i-(SPHERE_ROWS/2)) * SPHERE_SPACING;
			vec3_dup(m->material.albedo_base, White);
			m->material.roughness_base = std::max(j/((float)SPHERE_COLUMNS), 0.05f);
			m->material.metalness_base = i/((float)SPHERE_ROWS);
			gScene.models[i * SPHERE_COLUMNS + j] = m;
		}
	}
	int center_idx = SPHERE_ROWS * SPHERE_COLUMNS / 2;
	gModel = *gScene.models[center_idx];
	gScene.models[center_idx] = &gModel;
#endif

	return 0;
}

static int initialize() {
	int err = 0;

	printf("<-- Initializing deferred renderer... -->\n");
	if (err = deferred_initialize(&gDeferred)) {
		printf("Deferred renderer init failed\n");
		return err;
	}

	printf("<-- Initializing forward renderer... -->\n");
	if (err = forward_initialize(&gForward)) {
		printf("Forward renderer init failed\n");
		return err;
	}

	printf("<-- Initializing skyboxes... -->\n");
	if (err = initialize_skyboxes()) {
		printf("Skyboxes init failed\n");
		return err;
	}

	printf("<-- Initializing particle rendering... -->\n");
	if (err = initialize_particle_rendering()) {
		printf("Particle rendering init failed\n");
		return err;
	}

	printf("<-- Initializing meshes... -->\n");
	if (err = initialize_meshes()) {
		printf("Mesh loading failed\n");
		return err;
	}

	printf("<-- Initializing materials... -->\n");
	if (err = initialize_materials()) {
		printf("Material loading failed\n");
		return err;
	}

	printf("<-- Initializing scene... -->\n");
	if (err = init_scene()) {
		printf("Scene init failed\n");
		return err;
	}

	printf("<-- Initialization complete -->\n");
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

	// update emitter
	particle_emitter_update(&gEmitter, dt);
}

static int frame() {
	// calc dt
	static float prevFrameTime = 0.0f;
	float frameTime = utility_secs_since_launch();
	float dt = frameTime - prevFrameTime;
	prevFrameTime = frameTime;

	// update camera and emitters
	update_scene(dt);

	// clear backbuffer
	utility_set_clear_color(0, 0, 0);
	GL_WRAP(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	// render
	deferred_render(&gDeferred, &gScene);
	forward_render(&gForward, &gScene);

	ImGui::SetNextWindowSize(ImVec2( 200, 100 ), ImGuiCond_FirstUseEver);
	ImGui::Begin("Controls", 0);

	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Combo("Render Mode", (int*)&gDeferred.render_mode, render_mode_strings, render_mode_strings_count);
		ImGui::Separator();
		ImGui::Checkbox( "Cam Rotate", (bool*)&rotate_cam );
		ImGui::SliderFloat("Cam Zoom", (float*)&gScene.camera.boomLen, 0.0f, 150.0f);
		ImGui::SliderFloat("FOVy", (float*)&gScene.camera.fovy, 0.0f, 180.0f);
	}
	if (ImGui::CollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::BeginCombo("Skybox", gSkyboxes[skybox_idx].name, 0)) {
			for (int i = 0; i < STATIC_ELEMENT_COUNT(gSkyboxes); i++) {
				if (ImGui::Selectable(gSkyboxes[i].name, (skybox_idx == i))) {
					skybox_idx = i;
					gScene.skybox = gSkyboxes[i].skybox;
				}
				if ((skybox_idx == i)) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::ColorEdit3("Ambient Color", gScene.ambient_color);
		ImGui::SliderFloat("Ambient Intensity", &gScene.ambient_intensity, 0, 6.0f);
	}
	if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID("light");
		bool show_light_manipulator = (show_manipulator == 1);
		if (ImGui::Checkbox("Show Manipulator##light", &show_light_manipulator)) {
			show_manipulator = (show_light_manipulator) ? 1 : 0;
		}
		light_gui(&gLight);
		ImGui::PopID();
	}
	if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID("model");
		bool show_model_manipulator = (show_manipulator == 2);
		if (ImGui::Checkbox("Show Manipulator", &show_model_manipulator)) {
			show_manipulator = (show_model_manipulator) ? 2 : 0;
		}
		if (ImGui::BeginCombo("Geometry", gMeshes[mesh_idx].name, 0)) {
      for (int i = 0; i < STATIC_ELEMENT_COUNT(gMeshes); i++) {
        if (ImGui::Selectable(gMeshes[i].name, (mesh_idx == i))) {
					mesh_idx = i;
					gModel.mesh = gMeshes[i].mesh;
				}
        if ((mesh_idx == i)) {
          ImGui::SetItemDefaultFocus();
				}
      }
      ImGui::EndCombo();
    }
		ImGui::InputFloat3("Position", gModel.position);
		ImGui::SliderFloat("Rotation (Deg)", &gModel.rot[1], 0.0f, 360.0f, "%.0f");
		ImGui::SliderFloat("Scale", &gModel.scale, .01f, 25.0f );
#ifdef SPHERE_SCENE
		if (ImGui::Button("Refresh Scene")) {
			for (int i = 0; i < SPHERE_ROWS; i++) {
				for (int j = 0; j < SPHERE_COLUMNS; j++) {
					Model *m = gScene.models[i * SPHERE_COLUMNS + j];
					m->scale = gModel.scale;
					m->mesh = gModel.mesh;
					m->material = gModel.material;
    			m->material.roughness_base = std::max(j/((float)SPHERE_COLUMNS), 0.05f);
    			m->material.metalness_base = i/((float)SPHERE_ROWS);
				}
			}
		}
#endif
		ImGui::PopID();
	}
	if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
		material_gui(&gModel.material, &material_idx, gMaterials, STATIC_ELEMENT_COUNT(gMaterials));
	}
	if ( ImGui::CollapsingHeader("Emitter", 0)) {
		particle_emitter_gui(&gEmitterDesc, &gEmitter, gParticleTextures, STATIC_ELEMENT_COUNT(gParticleTextures));
		if (ImGui::Button("Flare")) {
			gEmitterDesc = gEmitterDescs[0];
			particle_emitter_refresh(&gEmitter);
		}
		ImGui::SameLine();
		if (ImGui::Button("Particle")) {
			gEmitterDesc = gEmitterDescs[1];
			particle_emitter_refresh(&gEmitter);
		}
		ImGui::SameLine();
		if (ImGui::Button("Smoke")) {
			gEmitterDesc = gEmitterDescs[2];
			particle_emitter_refresh(&gEmitter);
		}
	}
	ImGui::End();

	// render translation gizmo
	switch (show_manipulator) {
		case 1: utility_translation_gizmo(gLight.position, gScene.camera.view, gScene.camera.proj); break;
		case 2: utility_translation_gizmo(gModel.position, gScene.camera.view, gScene.camera.proj); break;
	}

	return 0;
}

int main(int argc, char* argv[]) {
  // init SDL
	SDL_Init(SDL_INIT_VIDEO);
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
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

  // init platform window
	SDL_Window*	window;
  if(!(window = SDL_CreateWindow("Renderer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
		, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL))) {
      return 1;
  }

	// init gl
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1); // Enable vsync
	glewExperimental = 1;
	glewInit();

	if (GLEW_EXT_texture_compression_s3tc) {
		printf("texture_compression_s3tc found\n");
	}

	// init Imgui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplSDL2_InitForOpenGL(window, glcontext);
  ImGui_ImplOpenGL3_Init("#version 130");
	StyleImguiCustom();

	// seed random
	srand((unsigned)time(0));

 	// clear window during load
	SDL_GL_SwapWindow(window);

	// main init
	if(initialize()) {
		printf("Failed to initialize. Exiting.\n");
		return 1;
	}

	// while the window is open: enter program loop
	int quit = 0;
	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
	  	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
				quit = 1;
				break;
			} else if (ImGui::GetIO().WantCaptureMouse) {
				if (ImGui_ImplSDL2_ProcessEvent(&event)) {
					continue; // ImGui handled this event
				}
			} else {
				switch (event.type) {
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT) {
						SDL_SetWindowGrab(window, SDL_TRUE);
					}
					break;
				case SDL_MOUSEBUTTONUP:
					if (event.button.button == SDL_BUTTON_LEFT) {
						SDL_SetWindowGrab(window, SDL_FALSE);
					}
					break;
				case SDL_MOUSEMOTION:
					if (event.motion.state & SDL_BUTTON_LMASK) {
						gScene.camera.rot[1] += event.motion.xrel / (float)WINDOW_WIDTH;
						gScene.camera.rot[0] += event.motion.yrel / (float)WINDOW_HEIGHT;
					}
					break;
				case SDL_MOUSEWHEEL:
					if (event.wheel.y == 1)  {
						gScene.camera.boomLen -= 2.f;
					} else if (event.wheel.y == -1) {
						gScene.camera.boomLen +=  2.f;
					}
					break;
				case SDL_QUIT:
					quit = 1;
					break;
				}
			}
		}

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		if (frame()) {
			quit = 1;
			break;
		}

		// render ImGui
		// ImGui::ShowDemoWindow();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// swap
		SDL_GL_SwapWindow(window);
		GL_CHECK_ERROR();
	}

	// clean up
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
