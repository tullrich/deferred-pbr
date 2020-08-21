#include "common.h"
#include "assets.h"
#include "deferred.h"
#include "forward.h"
#include "scene.h"

#include "imgui/imgui_custom_theme.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/ImGuizmo.h"

static SDL_Window* gWindow;
static Deferred gDeferred;
static Forward gForward;
static Scene gScene;

static ParticleEmitterDesc gEmitterDesc;

static int rotate_cam = 0;
static int show_manipulator = 0;
static float prevFrameTime = 0.0f;
static float fps = 0.0f;

static void update_loading_screen(const char* stage, const char* asset, int index, int total) {
  // pump events
	SDL_Event event;
	while (SDL_PollEvent(&event)) { }

	GL_WRAP(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	utility_set_clear_color(0, 0, 0);
	GL_WRAP(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(gWindow);
  ImGui::NewFrame();

  ImGui::SetNextWindowPosCenter();
	ImGui::SetNextWindowSize(ImVec2(400, 100));
	ImGui::Begin("PBR Renderer - Loading", 0,
    ImGuiWindowFlags_NoScrollbar
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
  );
    if (total > 0) {
      ImGui::Text("%s...    %s (%i / %i)", stage, asset, index, total);
    } else {
      ImGui::Text("%s...", stage);
    }
    ImGui::ProgressBar((float)index / (float)total, ImVec2(-1, 8), " ");
  ImGui::End();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  // swap
  SDL_GL_SwapWindow(gWindow);
  GL_CHECK_ERROR();
}

static int initialize_particle_rendering() {
	for (int i = 0; i < gParticleTexturesCount; i++) {
    update_loading_screen("Initializing particle", gParticleTextures[i].name, i, gParticleTexturesCount);
		if ((gParticleTextures[i].texture = utility_load_texture(GL_TEXTURE_2D, gParticleTextures[i].path)) < 0) {
			return 1;
		}
	}

	for (int i = 0; i < gEmitterDescsCount; i++) {
		int idx = (i < gParticleTexturesCount) ? i : 0;
		gEmitterDescs[i].texture = gParticleTextures[idx].texture;
	}
	return 0;
}

static int initialize_meshes() {
  update_loading_screen("Initializing mesh", "sphere", 0, gMeshesCount);
	mesh_sphere_tessellate(&gMeshes[0].mesh, 2.5f, 100, 100);
  gMeshes[0].mesh.desc = &gMeshes[0];
  update_loading_screen("Initializing mesh", "box", 1, gMeshesCount);
	mesh_make_box(&gMeshes[1].mesh, 5.0f);
  gMeshes[1].mesh.desc = &gMeshes[1];
	for (int i = 2; i < (gMeshesCount - 1); i++) {
    update_loading_screen("Initializing mesh", gMeshes[i].name, i, gMeshesCount);
		if (mesh_load(&gMeshes[i].mesh, &gMeshes[i])) {
			return 1;
		}
	}
	return 0;
}

static int initialize_materials() {
	int ret;
	for (int i = 0; i < gMaterialsCount; i++) {
    update_loading_screen("Initializing material", gMaterials[i].name, i, gMaterialsCount);
		if ((ret = material_load(&gMaterials[i].material, &gMaterials[i]))) {
			return ret;
		}
	}
	return 0;
}

static int initialize_skyboxes() {
	int ret;
	for (int i = 0; i < gSkyboxesCount; i++) {
    update_loading_screen("Initializing skybox", gSkyboxes[i].name, i, gSkyboxesCount);
		if ((ret = skybox_load(&gSkyboxes[i].skybox, &gSkyboxes[i]))) {
			return ret;
		}
	}
	return 0;
}

static int initialize_scene(int sphere_scene) {
	memset(&gScene, 0, sizeof(Scene));

	// Setup camera
	gScene.camera.boomLen = 30.0f;
	gScene.camera.fovy = 72.0f;
  gScene.camera.rot[0] = gScene.camera.rot[1] = -30.0f;

	// Setup ambient light
	vec3_dup(gScene.ambient_color, White);
	gScene.ambient_intensity = 0.03f;

	// Setup skybox
	gScene.skybox = &gSkyboxes[0].skybox;

	// Setup main directional light
	gScene.light = (Light*)malloc(sizeof(Light));
	vec3 lightPos = { 0.0f, 0.0f, 10.0f };
	light_initialize_point(gScene.light, lightPos, White, 3.0f);

	// Setup particle system
	gScene.emitters[0] = (ParticleEmitter*)malloc(sizeof(ParticleEmitter));
	memset(&gEmitterDesc, 0, sizeof(ParticleEmitterDesc));
	gEmitterDesc = gEmitterDescs[0];
	particle_emitter_initialize(gScene.emitters[0], &gEmitterDesc);
	gScene.emitters[0]->muted = true; // start muted

  // Setup model(s)
  if (!sphere_scene) {
  	printf("Creating single model scene\n");
    gScene.models[0] = (Model*)malloc(sizeof(Model));
  	model_initialize(gScene.models[0], &gMeshes[0].mesh, &gMaterials[0].material);
  } else {
    const int kSphereRows = 7;
    const int kSphereCols = 7;
    const int kSphereSpacing = 8.0f;
  	printf("Creating %ix%i spheres scene\n", kSphereRows, kSphereCols);
  	for (int i = 0; i < kSphereRows; i++) {
  		for (int j = 0; j < kSphereCols; j++) {
  			Model* m = (Model*)malloc(sizeof(Model));
  			model_initialize(m, &gMeshes[0].mesh, &gMaterials[0].material);
  			m->position[0] = (j-(kSphereCols/2)) * kSphereSpacing;
  			m->position[1] = (i-(kSphereRows/2)) * kSphereSpacing;
  			vec3_dup(m->material.albedo_base, White);
  			m->material.roughness_base = std::max(j/((float)kSphereCols), 0.05f);
  			m->material.metalness_base = i/((float)kSphereRows);
      	int center_idx = kSphereRows * kSphereCols / 2;
        int idx = i * kSphereCols + j;
        idx = (idx == center_idx || idx == 0) ? center_idx - idx : idx;
  			gScene.models[idx] = m;
  		}
    }
	}

  // Setup floor
  Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
  mesh_make_quad(mesh, 100, 100, 5);
  gScene.models[1] = (Model*)malloc(sizeof(Model));
	model_initialize(gScene.models[1], mesh, &gMaterials[6].material);
  vec3_set(gScene.models[1]->position, 0, -10.0f, 0);
	return 0;
}

static int initialize() {
	int err = 0;

  update_loading_screen("Initializing renderer...", "", 0, 0);

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
	if (err = initialize_scene(false)) {
		printf("Scene init failed\n");
		return err;
	}

	printf("<-- Initialization complete -->\n");
	return 0;
}

static void update_scene(float dt) {
  camera_update(&gScene.camera, dt, rotate_cam);

	// update emitters
  for (int i = 0; i < SCENE_EMITTERS_MAX; i++) {
    if (gScene.emitters[i]) {
	    particle_emitter_update(gScene.emitters[i], dt);
    }
  }
}

static int frame() {
	// calc dt
	float frameTime = utility_secs_since_launch();
	float dt = frameTime - prevFrameTime;
  fps = 1.0f / dt;
	prevFrameTime = frameTime;

	// update camera and emitters
	update_scene(dt);

	// clear backbuffer
	utility_set_clear_color(0, 0, 0);
	GL_WRAP(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	// render
	deferred_render(&gDeferred, &gScene);
	forward_render(&gForward, &gScene);

  ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(SIDEBAR_WIDTH, WINDOW_HEIGHT));
	ImGui::Begin("Renderer Controls", 0,
    ImGuiWindowFlags_NoScrollbar
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
  );
	ImGui::Combo("Render Mode", (int*)&gDeferred.render_mode, render_mode_strings, render_mode_strings_count);
  ImGui::Combo("Tonemapping Operator", (int*)&gDeferred.tonemapping_op, tonemapping_op_strings, tonemapping_op_strings_count);
	ImGui::Separator();
  if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Cam Rotate", (bool*)&rotate_cam);
		ImGui::SliderFloat("Cam Zoom", (float*)&gScene.camera.boomLen, 0.0f, 150.0f);
		ImGui::SliderFloat("FOVy", (float*)&gScene.camera.fovy, 0.0f, 180.0f);
  }
  if (ImGui::CollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Combo("Skybox Render Mode", (int*)&gDeferred.skybox_mode, skybox_mode_strings, skybox_mode_strings_count);
    if (gDeferred.skybox_mode == SKYBOX_MODE_PREFILTER_MAP) {
  		ImGui::SliderFloat("LoD", (float*)&gDeferred.prefilter_lod, 0.0f, 10.0f);
    }
    const SkyboxDesc* sd = gScene.skybox->desc;
		if (ImGui::BeginCombo("Skybox", sd->name, 0)) {
			for (int i = 0; i < gSkyboxesCount; i++) {
				if (ImGui::Selectable(gSkyboxes[i].name, (&gSkyboxes[i] == sd))) {
					gScene.skybox = &gSkyboxes[i].skybox;
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
		light_gui(gScene.light);
		ImGui::PopID();
	}
  if (ImGui::Button("Save Screenshot")) {
    if (!utility_save_screenshot("./test.png", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT)) {
      printf("Wrote screenshot to './test.png'\n");
    } else {
      printf("Error saving screenshot\n");
    }
  }
	ImGui::End();

  ImGui::SetNextWindowPos(ImVec2(SIDEBAR_WIDTH + VIEWPORT_WIDTH, 0));
	ImGui::SetNextWindowSize(ImVec2(SIDEBAR_WIDTH, WINDOW_HEIGHT));
	ImGui::Begin("Model Controls", 0,
    ImGuiWindowFlags_NoScrollbar
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
  );
  if (ImGui::BeginTabBar("ModelTabBar", ImGuiTabBarFlags_None))
  {
    if (ImGui::BeginTabItem("Model"))
    {
      if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
    		ImGui::PushID("model");
        const MeshDesc* md = gScene.models[0]->mesh->desc;
    		if (ImGui::BeginCombo("Geometry", md->name, 0)) {
          for (int i = 0; i < gMeshesCount; i++) {
            if (ImGui::Selectable(gMeshes[i].name, (&gMeshes[i] == md))) {
    					gScene.models[0]->mesh = &gMeshes[i].mesh;
    				}
          }
          ImGui::EndCombo();
        }
    		ImGui::SliderFloat("Rotation (Deg)", &gScene.models[0]->rot[1], 0.0f, 360.0f, "%.0f");
    		ImGui::SliderFloat("Scale", &gScene.models[0]->scale, .01f, 25.0f );
        bool show_model_trans_manip = (show_manipulator == 2);
        if (ImGui::Checkbox("Translation Manipulator", &show_model_trans_manip)) {
          show_manipulator = (show_model_trans_manip) ? 2 : 0;
        }
        bool show_model_rot_manip = (show_manipulator == 3);
        if (ImGui::Checkbox("Rotation Show Manipulator", &show_model_rot_manip)) {
          show_manipulator = (show_model_rot_manip) ? 3 : 0;
        }
    		ImGui::PopID();
    	}
    	if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
    		material_gui(&gScene.models[0]->material);
    	}
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Emitter")) {
  		particle_emitter_gui(&gEmitterDesc, gScene.emitters[0], gParticleTextures, gParticleTexturesCount);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
	ImGui::End();

  ImGui::SetNextWindowPos(ImVec2(SIDEBAR_WIDTH + 10, VIEWPORT_HEIGHT - 10), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
  ImGui::SetNextWindowBgAlpha(0.35f);
	if (ImGui::Begin("FPS", 0,
    ImGuiWindowFlags_NoScrollbar
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoDecoration
    | ImGuiWindowFlags_AlwaysAutoResize
    | ImGuiWindowFlags_NoSavedSettings
    | ImGuiWindowFlags_NoFocusOnAppearing
    | ImGuiWindowFlags_NoNav
  )) {
    ImGui::Text("FPS: %i", (int)fps);
  }
	ImGui::End();

	// render translation gizmo
	switch (show_manipulator) {
		case 1: {
      if (gScene.light->type == LIGHT_TYPE_POINT) {
        utility_translation_gizmo(gScene.light->position, gScene.camera.view, gScene.camera.proj);
      } else {
        utility_rotation_gizmo(gScene.light->rot, gScene.light->position, gScene.camera.view, gScene.camera.proj);
      }
      break;
    }
		case 2: utility_translation_gizmo(gScene.models[0]->position, gScene.camera.view, gScene.camera.proj); break;
		case 3: utility_rotation_gizmo(gScene.models[0]->rot, gScene.models[0]->position, gScene.camera.view, gScene.camera.proj); break;
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

  // init platform window
  if(!(gWindow = SDL_CreateWindow("PBR Renderer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
		, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL))) {
      return 1;
  }

	// init gl
	SDL_GLContext glcontext = SDL_GL_CreateContext(gWindow);
	SDL_GL_SetSwapInterval(1); // Enable vsync
	glewExperimental = 1;
	glewInit();

	// init Imgui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplSDL2_InitForOpenGL(gWindow, glcontext);
  ImGui_ImplOpenGL3_Init("#version 130");
	StyleImguiCustom();

	// seed random
	srand((unsigned)time(0));

 	// clear window during load
	SDL_GL_SwapWindow(gWindow);

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
						SDL_SetWindowGrab(gWindow, SDL_TRUE);
					}
					break;
				case SDL_MOUSEBUTTONUP:
					if (event.button.button == SDL_BUTTON_LEFT) {
						SDL_SetWindowGrab(gWindow, SDL_FALSE);
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
    ImGui_ImplSDL2_NewFrame(gWindow);
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
		SDL_GL_SwapWindow(gWindow);
		GL_CHECK_ERROR();
	}

	// clean up
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(gWindow);
	SDL_Quit();
	return 0;
}
