#include <stdio.h>

#include <SDL.h>
#include <GL/glew.h>
#include "AntTweakBar.h"

#include "linmath.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 640


void __report_gl_err(const char * file, const char * func, int line) {
	GLenum e;
	while ((e = glGetError()) != GL_NO_ERROR) {

		printf("RH %s:%s:%d gl error %s(%d)\n", file, func, line, gluErrorString(e), e);

	}
}
#define GL_CHECK_ERROR() __report_gl_err(__FILE__,__FUNCTION__,__LINE__)


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

GLuint deferred_load_texture_unknown() {
	GLuint texture_id;
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	unsigned char pixel[] = {
		0, 255, 0, 255,
		255, 0, 0, 255,
		0, 0, 255, 255,
		0, 0, 0, 255 };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	return texture_id;
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
GLint pos_loc, texcoord_loc, tex_loc;
GLint view_loc;

GLuint default_texture;

GLuint alt_fbo;
GLuint rendered_texture;
GLuint depth_render_buffer;

float accum;
vec3 rot = { 0.0, 0.0f, 0.0f };

int deferred_initialize()
{
	accum = 0.0f;

    TwBar *bar;
	bar = TwNewBar("Camera/Model Ctrls");

	glewExperimental = 1;
	glewInit();

	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);

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
	tex_loc = glGetUniformLocation(program, "tex");

	uint32_t c = 0x606060;
	glClearColor((c&0xff)/255.0f, (c>>8&0xff)/255.0f, (c>>16&0xff)/255.0f, 1.0f);

	glGenFramebuffers(1, &alt_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, alt_fbo);

	glGenTextures(1, &rendered_texture);
	glBindTexture(GL_TEXTURE_2D, rendered_texture);
 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA8, WINDOW_WIDTH, WINDOW_HEIGHT, 0,GL_RGBA, GL_UNSIGNED_BYTE, 0);
	printf("Val %i\n", rendered_texture);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rendered_texture, 0);
	GL_CHECK_ERROR();


	glGenRenderbuffers(1, &depth_render_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_render_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WINDOW_WIDTH, WINDOW_HEIGHT);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_render_buffer);

	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return 1;

	default_texture = deferred_load_texture_unknown();

	return 0;
}

void render_scene(GLuint texture)
{
	glUseProgram(program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(tex_loc, 0);

	mat4x4 view;
	mat4x4_identity(view);
	mat4x4_translate_in_place(view, 0.0f, 0.0f, -2.0f);
	mat4x4_rotate_X(view, view, 2.5f * rot[0]);
	mat4x4_rotate_Y(view, view, 2.5f * -rot[1]);
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
}

int deferred_frame()
{
	accum += 0.005f;
	{
		glBindFramebuffer(GL_FRAMEBUFFER, alt_fbo);
		glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
		glClearDepth(1.0f);
		glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		render_scene(default_texture);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);

	uint32_t c = 0x606060;
	glClearColor((c&0xff)/255.0f, (c>>8&0xff)/255.0f, (c>>16&0xff)/255.0f, 1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	render_scene(rendered_texture);
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

	// Initialize AntTweakBar
	if (!TwInit(TW_OPENGL, NULL))
		return 1;

	TwWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);

	if(deferred_initialize())
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
					rot[1] += event.motion.xrel / (float)WINDOW_WIDTH;
					rot[0] += event.motion.yrel / (float)WINDOW_HEIGHT;
				}
			}
			else if (event.type == SDL_QUIT) {
				quit = 1;
				break;
			}
		}

		if(!deferred_frame()) {
			TwDraw();
			SDL_GL_SwapBuffers();
		}
		else
			quit = 1;
	}

	// Clean up
	SDL_Quit();
	return 0;
}
