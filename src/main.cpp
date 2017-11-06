#include "common.h"
#include "particles.h"
#include "scene.h"
#include "deferred.h"
#include "forward.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl_gl3.h"

static SDL_Window*			gSDLWindow;
static Scene 				gScene;
static Deferred 			gDeferred;
static Forward 				gForward;

static ParticleEmitterDesc 	gEmitterDesc;
static ParticleEmitter 		gEmitter;

static int burst_count = 150;
static int rotate_cam = 0;
static int skybox_idx = 0;

// box render modes

static const char* render_mode_def[] = {
	"Shaded",
	"Position",
	"Diffuse",
	"Normal",
	"Specular",
	"Depth",
};

// particle shading modes
static const char* particle_shading_mode_def[] = {
	"Flat",
	"Textured",
};

// particle orientation mode
static const char* particle_orient_mode_def[] = {
	"Free",
	"Screen Aligned",
};

// particle texture index
static const char* particle_texture_def[] = {
	"Flare",
	"Particle",
	"Smoke",
	"Divine",
	"UV Debug",
};

// particle texture index -> paths LUT
static const char* gTexturePaths[] = {
	"images/particles/flare.png",
	"images/particles/particle.png",
	"images/particles/smoke.png",
	"images/particles/divine.png",
	"images/uv_map.png",
};


static const char* skybox_def[] = {
	"Saint Peters Basilica",
	"San Francisco",
	"UV Debug",
};

static Skybox gSkyboxes[STATIC_ELEMENT_COUNT(skybox_def)];

// SanFrancisco skybox textures (must match CubeMapFaces enum)
static const char* skybox_SanFrancisco[] = {
	"images/SanFrancisco4/posz.jpg",
	"images/SanFrancisco4/negz.jpg",
	"images/SanFrancisco4/posy.jpg",
	"images/SanFrancisco4/negy.jpg",
	"images/SanFrancisco4/posx.jpg",
	"images/SanFrancisco4/negx.jpg",
};

// SaintPetersBasilica skybox textures (must match CubeMapFaces enum)
static const char* skybox_SaintPetersBasilica[] = {
	"images/SaintPetersBasilica/posz.jpg",
	"images/SaintPetersBasilica/negz.jpg",
	"images/SaintPetersBasilica/posy.jpg",
	"images/SaintPetersBasilica/negy.jpg",
	"images/SaintPetersBasilica/posx.jpg",
	"images/SaintPetersBasilica/negx.jpg",
};

static const char* skybox_UV_Debug[] = {
	"images/uv_map.png",
	"images/uv_map.png",
	"images/uv_map.png",
	"images/uv_map.png",
	"images/uv_map.png",
	"images/uv_map.png",
};

static GLuint gTextures[STATIC_ELEMENT_COUNT(gTexturePaths)];

static void emitter_desc_preset_flare(ParticleEmitterDesc* out) {
	memset(out, 0, sizeof(ParticleEmitterDesc));
	out->max = 1024;
	out->spawn_rate = 60;
	vec4_dup(out->start_color, Yellow);
	vec4_dup(out->end_color, Red); out->end_color[3] = 0.0f;
	out->orient_mode = PARTICLE_ORIENT_SCREEN_ALIGNED;
	out->depth_sort_alpha_blend = 0;
	out->speed = 8.0f; out->speed_variance = 5.0f;
	out->life_time = 2.5f; out->life_time_variance = 0.5f;
	out->shading_mode = PARTICLE_SHADING_TEXTURED;
	out->texture = gTextures[0];
	out->start_scale = 1.0f; out->end_scale = 0.01f;
	vec3_dup(out->emit_cone_axis, Axis_Right);
};

static void emitter_desc_preset_particle(ParticleEmitterDesc* out) {
	memset(out, 0, sizeof(ParticleEmitterDesc));
	out->max = 1024;
	out->spawn_rate = 60.0f;
	vec4_rgba(out->start_color, 100, 100, 255, 255);
	vec4_dup( out->end_color, White ); out->end_color[ 3 ] = 0.0f;
	out->orient_mode = PARTICLE_ORIENT_SCREEN_ALIGNED;
	out->depth_sort_alpha_blend = 0;
	out->speed = 4.0f;
	out->speed_variance = 1.0f;
	out->life_time = 2.5f;
	out->life_time_variance = 0.5f;
	out->shading_mode = PARTICLE_SHADING_TEXTURED;
	out->texture = gTextures[1];
	out->start_scale = 0.1f;
	out->end_scale = 0.5f;
	vec3_dup(out->emit_cone_axis, Axis_Up);
};

static void emitter_desc_preset_smoke(ParticleEmitterDesc* out) {
	memset(out, 0, sizeof(ParticleEmitterDesc));
	out->max = 1024;
	out->spawn_rate = 24.0f;
	vec4_dup(out->start_color, Green); out->start_color[3] = 0.4f;
	vec4_dup(out->end_color, White); out->end_color[3] = 0.0f;
	out->orient_mode = PARTICLE_ORIENT_SCREEN_ALIGNED;
	out->depth_sort_alpha_blend = 1;
	out->speed = 3.0f;
	out->speed_variance = 0.0f;
	out->life_time = 3.0f;
	out->life_time_variance = 0.0f;
	out->shading_mode = PARTICLE_SHADING_TEXTURED;
	out->texture = gTextures[2];
	out->start_scale = 2.0f;
	out->end_scale = 4.5f;
	vec3_dup(out->emit_cone_axis, Axis_Up);
};

static int initialize_particle_textures() {
	for (int i = 0; i < STATIC_ELEMENT_COUNT(gTexturePaths); i++) {
		if ((gTextures[i] = utility_load_image(GL_TEXTURE_2D, gTexturePaths[i])) < 0) {
			return 1;
		}
	}
	return 0;
}

static int get_particle_texture_index() {
	for (int i = 0; i < STATIC_ELEMENT_COUNT(gTexturePaths); i++) {
		if (gEmitterDesc.texture == gTextures[i]) {
			return i;
		}
	}
	return 0;
}

static int initialize_skybox_textures() {
	if(!(gSkyboxes[0].env_cubemap = utility_load_cubemap(skybox_SaintPetersBasilica)))
		return 1;

	if(!(gSkyboxes[1].env_cubemap = utility_load_cubemap(skybox_SanFrancisco)))
		return 1;

	if(!(gSkyboxes[2].env_cubemap = utility_load_cubemap(skybox_UV_Debug)))
		return 1;
	return 0;
}

static void init_main_light() {
	gScene.ambient_color[0] = 0.15f;
	gScene.ambient_color[1] = 0.60f;
	gScene.ambient_color[2] = 0.15f;
	gScene.ambient_intensity = 0.45f;

	gScene.main_light.position[0] = 5.0f;
	gScene.main_light.position[1] = 10.0f;
	gScene.main_light.position[2] = 5.0f;

	gScene.main_light.color[0] = 1.0f;
	gScene.main_light.color[1] = 0.15f;
	gScene.main_light.color[2] = 0.25f;

	gScene.main_light.intensity = 1.0f;
}

static void refresh_emitter( void *clientData ) {
	// delete existing
	if (gEmitter.desc) {
		particle_emitter_destroy(&gEmitter);
		gScene.emitters[0] = NULL;
	}

	particle_emitter_initialize(&gEmitter, &gEmitterDesc);
	gScene.emitters[0] = &gEmitter;
}

static void burst() {
	particle_emitter_burst(&gEmitter, burst_count);
}

static int init_scene() {
	memset(&gScene, 0, sizeof(Scene));
	gScene.show_box = true;

	// Init camera
	gScene.camera.boomLen = 15.0f;

	// Init skybox
	gScene.skybox = gSkyboxes[skybox_idx];

	// Init main directional light
	init_main_light();

	// Init particle system
	memset(&gEmitterDesc, 0, sizeof(ParticleEmitterDesc));
	emitter_desc_preset_flare(&gEmitterDesc);
	refresh_emitter( NULL );
	gEmitter.muted = true; // start muted

	return 0;
}

static int initialize() {
	glewExperimental = 1;
	glewInit();

	// Initialize Imgui
	ImGui_ImplSdlGL3_Init(gSDLWindow);

	srand((unsigned)time(0));

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
	printf("forward initialized\n");

	gForward.g_buffer = &gDeferred.g_buffer;

	if (err = initialize_particle_textures()) {
		printf("particle rendering init failed\n");
		return err;
	}
	printf("particle rendering initialized\n");

	if (err = init_scene()) {
		printf("scene init failed\n");
		return err;
	}

	GL_CHECK_ERROR();
	printf("scene initialized\n");

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
	mat4x4_perspective(gScene.camera.proj, 80.0f * (float)M_PI/180.0f, (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, Z_NEAR, Z_FAR);
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

	ImGui::SetNextWindowSize( ImVec2( 200, 100 ), ImGuiSetCond_FirstUseEver );
	ImGui::Begin( "Controls", 0);

	if ( ImGui::CollapsingHeader( "Renderer", ImGuiTreeNodeFlags_DefaultOpen ) ) {
		ImGui::Combo( "Render Mode", ( int* )&gDeferred.render_mode, render_mode_def, STATIC_ELEMENT_COUNT( render_mode_def ) );
	}
	if ( ImGui::CollapsingHeader( "Scene", ImGuiTreeNodeFlags_DefaultOpen ) ) {
		ImGui::Checkbox( "Cam Rotate", ( bool* )&rotate_cam );

		ImGui::Checkbox( "Show Box", ( bool* )&gScene.show_box );
		ImGui::ColorEdit3( "Ambient Color", gScene.ambient_color );
		ImGui::SliderFloat( "Ambient Intensity", &gScene.ambient_intensity, 0, 1.0f );

		ImGui::InputFloat3( "Light Position", gScene.main_light.position );
		ImGui::ColorEdit3( "Light Color", gScene.main_light.color );
		ImGui::SliderFloat( "Light Intensity", &gScene.main_light.intensity, 0, 1.0f );

		if(ImGui::Combo( "Skybox", ( int* )&skybox_idx, skybox_def, STATIC_ELEMENT_COUNT( skybox_def ) )) {
			gScene.skybox = gSkyboxes[skybox_idx];
		}
	}
	if ( ImGui::CollapsingHeader( "Emitter", ImGuiTreeNodeFlags_DefaultOpen ) ) {
		if ( ImGui::Button("Refresh") ) {
			refresh_emitter(NULL);
		}
		ImGui::SliderFloat( "Spawn Rate", &gEmitterDesc.spawn_rate, 0, 500.0f );
		ImGui::SliderInt( "Max Particles", &gEmitterDesc.max, 1, 2048 );

		ImGui::Combo( "Shading Mode", ( int* )&gEmitterDesc.shading_mode, particle_shading_mode_def, STATIC_ELEMENT_COUNT( particle_shading_mode_def ) );

		if ( gEmitterDesc.shading_mode == PARTICLE_SHADING_TEXTURED ) {
			int texture_idx = get_particle_texture_index();
			if ( ImGui::Combo( "Texture", ( int* )&texture_idx, particle_texture_def, STATIC_ELEMENT_COUNT( particle_texture_def ) ) ) {
				gEmitterDesc.texture = gTextures[ texture_idx ];
			}
		}
		ImGui::SliderFloat( "Start Scale", &gEmitterDesc.start_scale, .01, 10.0f );
		ImGui::SliderFloat( "End Scale", &gEmitterDesc.end_scale, .01, 10.0f );
		ImGui::ColorEdit4( "Start Color", gEmitterDesc.start_color );
		ImGui::ColorEdit4( "End Color", gEmitterDesc.end_color );

		ImGui::Combo( "Orientation Mode", ( int* )&gEmitterDesc.orient_mode, particle_orient_mode_def, STATIC_ELEMENT_COUNT( particle_orient_mode_def ) );
		ImGui::Checkbox( "Depth Sort", ( bool* )&gEmitterDesc.depth_sort_alpha_blend );
		ImGui::Checkbox( "Soft", ( bool* )&gEmitterDesc.soft );
		ImGui::Checkbox( "Gravity", ( bool* )&gEmitterDesc.simulate_gravity );
		ImGui::Checkbox( "Mute", ( bool* )&gEmitter.muted );
		//TwAddVarRW( bar, "Cone Axis", TW_TYPE_DIR3F, &gEmitterDesc.emit_cone_axis, "" );
		ImGui::SliderFloat( "Life Time", &gEmitterDesc.life_time, 0.0f, 10.0f );
		ImGui::SliderFloat( "Life Time Variance", &gEmitterDesc.life_time_variance, 0.0f, 10.0f );
		ImGui::SliderFloat( "Speed", &gEmitterDesc.speed, 0.0f, 10.0f );
		ImGui::SliderFloat( "Speed Variance", &gEmitterDesc.speed_variance, 0.0f, 10.0f );

		ImGui::SliderFloat( "Local Scale", &gEmitter.scale, 0.0f, 10.0f );
		//TwAddVarRW( bar, "Local Rotate", TW_TYPE_QUAT4F, &gEmitter.rot, "" );
		ImGui::InputFloat3( "Local Translation", gEmitter.pos );
		if ( ImGui::Button( "Burst" ) ) {
			burst();
		}
		ImGui::SliderInt( "Burst Count", &burst_count, 0, 1000 );

		if ( ImGui::CollapsingHeader( "Presets", ImGuiTreeNodeFlags_DefaultOpen ) ) {
			if ( ImGui::Button( "Flare" ) ) {
				emitter_desc_preset_flare( &gEmitterDesc );
				refresh_emitter( NULL );
			}
			if ( ImGui::Button( "Particle" ) ) {
				emitter_desc_preset_particle( &gEmitterDesc );
				refresh_emitter( NULL );
			}
			if ( ImGui::Button( "Smoke" ) ) {
				emitter_desc_preset_smoke( &gEmitterDesc );
				refresh_emitter( NULL );
			}
		}
	}
	ImGui::End();
	ImGui::Render();

	GL_CHECK_ERROR();
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

    if(!(gSDLWindow = SDL_CreateWindow("Particles", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
			, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL))) {
        return 1;
    }

	SDL_GLContext glcontext = SDL_GL_CreateContext(gSDLWindow);
	if(initialize()) {
		printf("Failed to initialize. Exiting.\n");
		return 1;
	}

	// Enable vsync
	SDL_GL_SetSwapInterval(1);

	// The window is open: enter program loop (see SDL_PollEvent)
	int quit = 0;
	int mouse_grabbed = 0;
	while (!quit) {
		ImGui_ImplSdlGL3_NewFrame(gSDLWindow);
		bool imguiCaptureMouse = ImGui::GetIO().WantCaptureMouse;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (ImGui_ImplSdlGL3_ProcessEvent(&event)) {
				continue;
			}

			if (!imguiCaptureMouse) {
				if (event.type == SDL_MOUSEBUTTONDOWN) {
					if (event.button.button == SDL_BUTTON_LEFT) {
						// hide mouse, report deltas at screen edge
						//SDL_SetRelativeMouseMode(SDL_TRUE);
						mouse_grabbed = 1;
					}
				}
				else if (event.type == SDL_MOUSEBUTTONUP) {
					if (event.button.button == SDL_BUTTON_LEFT) {
						//SDL_SetRelativeMouseMode(SDL_FALSE);
						mouse_grabbed = 0;
					}
				}
				else if (event.type == SDL_MOUSEMOTION) {
					if (event.motion.state & SDL_BUTTON_LMASK) {
						gScene.camera.rot[1] += event.motion.xrel / (float)WINDOW_WIDTH;
						gScene.camera.rot[0] += event.motion.yrel / (float)WINDOW_HEIGHT;
					}
				}
			}
			if (event.type == SDL_QUIT) {
				quit = 1;
				break;
			}
		}

		if ( frame() )
		{
			quit = 1;
			break;
		}

		// swap
		SDL_GL_SwapWindow(gSDLWindow);
	}

	// Clean up
	ImGui_ImplSdlGL3_Shutdown();
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(gSDLWindow);
	SDL_Quit();
	return 0;
}
