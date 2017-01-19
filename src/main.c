#include "common.h"
#include "particles.h"
#include "scene.h"
#include "deferred.h"
#include "forward.h"

static Scene 				gScene;
static Deferred 			gDeferred;
static Forward 				gForward;

static ParticleEmitterDesc 	gEmitterDesc;
static ParticleEmitter 		gEmitter;

static int burst_count = 150;
static int rotate_cam = 0;

// box render modes Tw def
static const TwEnumVal render_mode_def[] = {
	{RENDER_MODE_SHADED, "Shaded"},
	{RENDER_MODE_POSITION, "Position"},
	{RENDER_MODE_DIFFUSE, "Diffuse"},
	{RENDER_MODE_NORMAL, "Normal"},
	{RENDER_MODE_SPECULAR, "Specular"},
	{RENDER_MODE_DEPTH, "Depth"},
};

// particle shading modes Tw def
static const TwEnumVal particle_shading_mode_def[] = {
	{PARTICLE_SHADING_FLAT, "Flat"},
	{PARTICLE_SHADING_TEXTURED, "Textured"},
};

// particle orientation mode Tw def
static const TwEnumVal particle_orient_mode_def[] = {
	{PARTICLE_ORIENT_FREE, "Free" },
	{PARTICLE_ORIENT_SCREEN_ALIGNED, "Screen Aligned" },
};

// particle texture index Tw def
static const TwEnumVal particle_texture_def[] = {
	{0, "Flare"},
	{1, "Particle"},
	{2, "Smoke"},
	{3, "Divine"},
	{4, "UV Debug"},
};

// particle texture index -> paths LUT
static const char* gTexturePaths[] = {
	"images/particles/flare.png",
	"images/particles/particle.png",
	"images/particles/smoke.png",
	"images/particles/divine.png",
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
};

static int initialize_particle_textures() {
	for (int i = 0; i < STATIC_ELEMENT_COUNT(gTexturePaths); i++) {
		if ((gTextures[i] = utility_load_image(GL_TEXTURE_2D, gTexturePaths[i])) < 0) {
			return 1;
		}
	}
	return 0;
}

static void get_particle_texture(void* value, void* user) {
	for (int i = 0; i < STATIC_ELEMENT_COUNT(gTexturePaths); i++) {
		if (gEmitterDesc.texture == gTextures[i]) {
			*((int*)value) = i;
			return;
		}
	}
	*((int*)value) = 0;
}

static void set_particle_texture(const void* value,void* user) {
	gEmitterDesc.texture = gTextures[*((int*)value)];
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

static void set_preset(void *clientData) {
	void(*preset_func)(ParticleEmitterDesc*) = clientData;
	preset_func(&gEmitterDesc);
	refresh_emitter( NULL );
}

static int initialize() {
	glewExperimental = 1;
	glewInit();

	// Initialize AntTweakBar
	if (!TwInit(TW_OPENGL_CORE, NULL))
		return 1;

	TwWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);

    TwBar *bar;
	bar = TwNewBar("Controls");

	TwAddSeparator(bar, "Renderer", "");
	TwAddVarRW(bar, "Cam Rotate", TW_TYPE_BOOL32, &rotate_cam, "");
	TwAddVarRW(bar, "Render Mode", TwDefineEnum("", render_mode_def
		, STATIC_ELEMENT_COUNT(render_mode_def)), &gDeferred.render_mode, "");
	TwAddVarRW(bar, "Show Box", TW_TYPE_BOOL32, &gScene.show_box, "");
	TwAddVarRW(bar, "Ambient Color", TW_TYPE_COLOR3F, &gScene.ambient_color, "");
	TwAddVarRW(bar, "Ambient Intensity", TW_TYPE_FLOAT, &gScene.ambient_intensity, "step='0.01' min=0 max=1");
	TwAddVarRW(bar, "Light Position", TW_TYPE_DIR3F, &gScene.main_light.position, "");
	TwAddVarRW(bar, "Light Color", TW_TYPE_COLOR3F, &gScene.main_light.color, "");
	TwAddVarRW(bar, "Light Intensity", TW_TYPE_FLOAT, &gScene.main_light.intensity, "step='0.01' min=0 max=1");
	TwAddSeparator(bar, "Emitter", "");
	TwAddButton(bar, "Refresh", (TwButtonCallback)&refresh_emitter, NULL, "");
	TwAddVarRW(bar, "Spawn Rate", TW_TYPE_FLOAT, &gEmitterDesc.spawn_rate, "");
	TwAddVarRW(bar, "Shading Mode", TwDefineEnum("", particle_shading_mode_def
		, STATIC_ELEMENT_COUNT(particle_shading_mode_def)), &gEmitterDesc.shading_mode, "");
	TwAddVarCB(bar, "Texture", TwDefineEnum("", particle_texture_def
		, STATIC_ELEMENT_COUNT(particle_texture_def)), (TwSetVarCallback)&set_particle_texture, (TwGetVarCallback)&get_particle_texture, NULL, "");
	TwAddVarRW(bar, "Start Scale", TW_TYPE_FLOAT, &gEmitterDesc.start_scale, "step='0.25' min=.01 max=10");
	TwAddVarRW(bar, "End Scale", TW_TYPE_FLOAT, &gEmitterDesc.end_scale, "step='0.25' min=.01 max=10");
	TwAddVarRW(bar, "Start Color", TW_TYPE_COLOR3F, &gEmitterDesc.start_color, "");
	TwAddVarRW(bar, "End Color", TW_TYPE_COLOR3F, &gEmitterDesc.end_color, "");
	TwAddVarRW(bar, "Start Alpha", TW_TYPE_FLOAT, &gEmitterDesc.start_color[3], "step='0.01' min=0 max=1");
	TwAddVarRW(bar, "End Alpha", TW_TYPE_FLOAT, &gEmitterDesc.end_color[3], "step='0.01' min=0 max=1");
	TwAddVarRW(bar, "Orientation Mode", TwDefineEnum("", particle_orient_mode_def
		, STATIC_ELEMENT_COUNT(particle_orient_mode_def)), &gEmitterDesc.orient_mode, "");
	TwAddVarRW( bar, "Depth Sort", TW_TYPE_BOOL32, &gEmitterDesc.depth_sort_alpha_blend, "" );
	TwAddVarRW( bar, "Soft", TW_TYPE_BOOL32, &gEmitterDesc.soft, "");
	TwAddVarRW(bar, "Life Time", TW_TYPE_FLOAT, &gEmitterDesc.life_time, "step='0.25' min=0 max=10");
	TwAddVarRW(bar, "Life Time Variance", TW_TYPE_FLOAT, &gEmitterDesc.life_time_variance, "step='0.25' min=0 max=10");
	TwAddVarRW(bar, "Speed", TW_TYPE_FLOAT, &gEmitterDesc.speed, "step='1' min=0 max=10");
	TwAddVarRW(bar, "Speed Variance", TW_TYPE_FLOAT, &gEmitterDesc.speed_variance, "step='1' min=0 max=10");
	TwAddVarRW(bar, "Max", TW_TYPE_INT32, &gEmitter.max, "step='1' min=1 max=2048");
	TwAddVarRW(bar, "Local Scale", TW_TYPE_FLOAT, &gEmitter.scale, "step='0.05' min=.01 max=5");
	TwAddVarRW(bar, "Local Rotate", TW_TYPE_QUAT4F, &gEmitter.rot, "");
	TwAddVarRW(bar, "Local Translation", TW_TYPE_DIR3F, &gEmitter.pos, "");
	TwAddVarRW(bar, "Mute", TW_TYPE_BOOL32, &gEmitter.muted, "");
	TwAddButton(bar, "Burst", (TwButtonCallback)burst, NULL, "");
	TwAddVarRW(bar, "Bust Count", TW_TYPE_INT32, &burst_count, "step='20' min=0 max=1000");
	TwAddSeparator(bar, "Presets", "");
	TwAddButton(bar, "Flare", (TwButtonCallback)&set_preset, &emitter_desc_preset_flare, "");
	TwAddButton(bar, "Particle", (TwButtonCallback)&set_preset, &emitter_desc_preset_particle, "");
	TwAddButton(bar, "Smoke", (TwButtonCallback)&set_preset, &emitter_desc_preset_smoke, "");

	srand((unsigned)time(0));

	if(deferred_initialize(&gDeferred))
		return 1;

	if(forward_initialize(&gForward))
		return 1;
	gForward.g_buffer = &gDeferred.g_buffer;

	initialize_particle_textures();

	memset(&gScene, 0, sizeof(Scene));
	gScene.camera.boomLen = 15.0f;
	init_main_light();

	memset(&gEmitterDesc, 0, sizeof(ParticleEmitterDesc));
	emitter_desc_preset_flare(&gEmitterDesc);
	refresh_emitter( NULL );

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
	mat4x4_perspective(gScene.camera.proj, 80.0f * (float)M_PI/180.0f, (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, Z_NEAR, Z_FAR);
	mat4x4_mul(gScene.camera.viewProj, gScene.camera.proj, gScene.camera.view);
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

	// render gui
	TwDraw();

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

	SDL_Window* sdlWindow;
    if(!(sdlWindow = SDL_CreateWindow("Particles", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
			, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL))) {
        return 1;
    }

	SDL_GLContext glcontext = SDL_GL_CreateContext(sdlWindow);
	if(initialize()) {
		printf("Failed to initialize. Exiting.\n");
		return 1;
	}

	// The window is open: enter program loop (see SDL_PollEvent)
	int quit = 0;
	int mouse_grabbed = 0;
	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {

			if (TwEventSDL(&event, SDL_MAJOR_VERSION, SDL_MINOR_VERSION)) {
				continue;
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN) {
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
			else if (event.type == SDL_QUIT) {
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
		SDL_GL_SwapWindow( sdlWindow );
	}

	// Clean up
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow( sdlWindow );
	SDL_Quit();
	return 0;
}
