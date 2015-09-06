#include <stdio.h>

#include <SDL.h>
#include <GL/glew.h>

#include "linmath.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 640

int deferred_buffer_file(const char *filename, unsigned char **buf, size_t *size) {
	FILE *fd;
	int ret = 1;
	if (fd = fopen(filename, "rb")) {
		if (!fseek(fd, 0, SEEK_END)) {
			size_t fsize = ftell(fd);
			rewind(fd);
			*buf = (unsigned char*)malloc(fsize);
			if (!fread(*buf, 1, fsize, fd) != fsize) {
				*size = fsize;
				ret = 0;
			} else {
				free(*buf);
			}
		}
		fclose(fd);
	}
	return ret;
}


// one of GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_GEOMETRY_SHADER
GLuint deferred_create_shader(const char *filename, GLenum shader_type) {
	unsigned char* buf;
	size_t s;
	if (deferred_buffer_file(filename, &buf, &s)) {
		printf("Unable to open shader file %s.\n", filename);
		return 0;
	}

	GLuint shader = glCreateShader(shader_type);
	glShaderSource(shader, 1, (const GLchar**)&buf, (GLint*)&s);
	glCompileShader(shader);
	free(buf);

	GLint compiled_result;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled_result);
	if (compiled_result != GL_TRUE)
	{
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetShaderInfoLog(shader, 1024, &log_length, message);
		printf("Shader Compile Error: %s\n", message);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint deferred_create_program(const char *vert_filename, const char *frag_filename) {
	GLint vert_shader;
	if (!(vert_shader = deferred_create_shader(vert_filename, GL_VERTEX_SHADER))) {
		return 0;
	}

	GLint frag_shader;
	if (!(frag_shader = deferred_create_shader(frag_filename, GL_FRAGMENT_SHADER))) {
		glDeleteShader(vert_shader);
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);

	GLint program_linked;
	glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
	if (program_linked != GL_TRUE)
	{
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetProgramInfoLog(program, 1024, &log_length, message);
		printf("Shader Link Error: %s\n", message);
		glDeleteProgram(program);
		program = 0;
	}

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	return program;
}

void deferred_draw_cube(GLint texcoord_loc, GLint pos_loc)
{
	glBegin(GL_QUADS);
	// back face
	glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, 0, 0, 0);
	glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, 0, 1, 0);
	glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, 1, 1, 0);
	glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, 1, 0, 0);

	// front face
	glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, 1, 1, 1);
	glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, 0, 1, 1);
	glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, 0, 0, 1);
	glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, 1, 0, 1);

	// left face
	glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, 0, 1, 1);
	glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, 0, 1, 0);
	glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, 0, 0, 0);
	glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, 0, 0, 1);

	// right face
	glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, 1, 1, 1);
	glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, 1, 0, 1);
	glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, 1, 0, 0);
	glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, 1, 1, 0);

	// bottom face
	glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, 1, 0, 1);
	glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, 0, 0, 1);
	glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, 0, 0, 0);
	glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, 1, 0, 0);

	// top face
	glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, 1, 1, 1);
	glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, 1, 1, 0);
	glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, 0, 1, 0);
	glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, 0, 1, 1);
	glEnd();
}

GLuint program;
GLint pos_loc, texcoord_loc;
GLint view_loc;

float accum;

int deferred_initialize()
{
	accum = 0.0f;

	glewExperimental = 1;
	glewInit();

	glEnable(GL_CULL_FACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glEnable(GL_DEPTH_TEST);

	if(!(program = deferred_create_program("shaders/box.vert", "shaders/box.frag"))) {
		printf("Unable to load shaders\n");
		return 1;
	}

	pos_loc = glGetAttribLocation(program, "position");
	texcoord_loc = glGetAttribLocation(program, "texcoord");
	view_loc = glGetUniformLocation(program, "view");

	uint32_t c = 0x606060;
	glClearColor((c&0xff)/255.0f, (c>>8&0xff)/255.0f, (c>>16&0xff)/255.0f, 1.0f);
	return 0;
}

int deferred_frame()
{
	accum += 0.005f;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(program);

	mat4x4 view;
	mat4x4_identity(view);
	mat4x4_translate_in_place(view, 0.0f, 0.0f, -2.0f);
	mat4x4_rotate_Y(view, view, sinf(accum) * M_PI);
	mat4x4_rotate_X(view, view, sinf(accum) * M_PI);
	mat4x4_translate_in_place(view, -0.5f, -0.5f, -0.5f);

/*	vec3 eye = { 0.0f, 0.0f, sinf(accum) };
	vec3 center = { 0.0f };
	vec3 up = { 0.0f, 1.0f, 0.0f };
	mat4x4_look_at(view, eye, center, up);*/

	mat4x4 proj;
	mat4x4_perspective(proj, 80.0f * M_PI/180.0f, (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 100.0f);

	mat4x4 camera;
	mat4x4_mul(camera, proj, view);
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, (GLfloat*)camera);

	deferred_draw_cube(texcoord_loc, pos_loc);
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
	int quit = 0;
	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {

			if (event.type == SDL_WINDOWEVENT) {
				switch (event.window.event) {
				case SDL_WINDOWEVENT_CLOSE:
					quit = 1;
					break;
				}
			}
		}

		if(!deferred_frame())
			SDL_GL_SwapWindow(window);
		else
			quit = 1;
	}

	// Close and destroy the window
	SDL_GL_DeleteContext(mainGLContext);
	SDL_DestroyWindow(window);

	// Clean up
	SDL_Quit();
	return 0;
}
