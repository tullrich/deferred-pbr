#include "utility.h"
#include "deferred.h"
#include "common.h"

Scene 		gScene;
Deferred 	gDef;

float accum;

int initialize()
{
	accum = 0.0f;

	// Initialize AntTweakBar
	if (!TwInit(TW_OPENGL, NULL))
		return 1;

	TwWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);

    TwBar *bar;
	bar = TwNewBar("Ctrls");

	glewExperimental = 1;
	glewInit();

	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glEnable(GL_DEPTH_TEST);

	if(deferred_initialize(&gDef))
		return 1;

	memset(&gScene, 0, sizeof(Scene));

	return 0;
}

void update_scene()
{
/*	vec3 eye = { 0.0f, 0.0f, sinf(accum) };
	vec3 center = { 0.0f };
	vec3 up = { 0.0f, 1.0f, 0.0f };
	mat4x4_look_at(view, eye, center, up);*/

	mat4x4 view;
	mat4x4_identity(view);
	mat4x4_translate_in_place(view, 0.0f, 0.0f, -2.0f);
	mat4x4_rotate_X(view, view, 2.5f * gScene.rot[0]);
	mat4x4_rotate_Y(view, view, 2.5f * -gScene.rot[1]);
	mat4x4_translate_in_place(view, -0.5f, -0.5f, -0.5f);

	mat4x4 proj;
	mat4x4_perspective(proj, 80.0f * M_PI/180.0f, (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 100.0f);

	mat4x4_mul(gScene.camera, proj, view);
}

int frame()
{
	update_scene();
	deferred_render(&gDef, &gScene);
	TwDraw();
	SDL_GL_SwapBuffers();
	return 0;
}

int main() 
{
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
 
 	SDL_Surface *Surf_Display;
    if((Surf_Display = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 32, SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER | SDL_OPENGL)) == NULL) {
        return 1;
    }

	if(initialize())
		return 1;

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
					gScene.rot[1] += event.motion.xrel / (float)WINDOW_WIDTH;
					gScene.rot[0] += event.motion.yrel / (float)WINDOW_HEIGHT;
				}
			}
			else if (event.type == SDL_QUIT) {
				quit = 1;
				break;
			}
		}

		if(frame())
			quit = 1;
	}

	// Clean up
	SDL_Quit();
	return 0;
}
