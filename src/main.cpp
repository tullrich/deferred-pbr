#include <iostream>

/* SDL */
#include "SDL.h"
#include <GL/glew.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 640

int deferred_initialize()
{
	glewExperimental = true;
	glewInit();

	glClearColor(64 /255.0f, 87 / 255.0f, 170 / 255.0f, 1.0f);
	return 0;
}

int deferred_frame()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	return 0;

}

int main() 
{
	SDL_Window *window;                    // Declare a pointer
	SDL_GLContext mainGLContext;

	SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL2
										   // Create an application window with the following settings:
	window = SDL_CreateWindow(
		"Deferred",                  // window title
		SDL_WINDOWPOS_UNDEFINED,           // initial x position
		SDL_WINDOWPOS_UNDEFINED,           // initial y position
		WINDOW_WIDTH,						// width, in pixels
		WINDOW_HEIGHT,						// height, in pixels
		SDL_WINDOW_OPENGL                  // flags - see below
		);


	// Check that the window was successfully made
	if (window == NULL) {
		// In the event that the window could not be made...
		printf("Could not create window: %s\n", SDL_GetError());
		return 1;
	}

	mainGLContext = SDL_GL_CreateContext(window);

	if(deferred_initialize())
		return 1;

	// The window is open: enter program loop (see SDL_PollEvent)
	bool quit = false;
	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {

			if (event.type == SDL_WINDOWEVENT) {
				switch (event.window.event) {
				case SDL_WINDOWEVENT_CLOSE:
					quit = true;
					break;
				}
			}
		}

		if(!deferred_frame())
			SDL_GL_SwapWindow(window);
		else
			quit = true;
	}

	// Close and destroy the window
	SDL_GL_DeleteContext(mainGLContext);
	SDL_DestroyWindow(window);

	// Clean up
	SDL_Quit();
	return 0;
}