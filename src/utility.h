#pragma once
#include "common.h"

void utility_report_gl_err(const char * file, const char * func, int line);
#define GL_CHECK_ERROR() utility_report_gl_err(__FILE__,__FUNCTION__,__LINE__)

int utility_buffer_file(const char *filename, unsigned char **buf, size_t *size);
GLuint utility_create_shader(const char *filename, GLenum shader_type);
GLuint utility_create_program(const char *vert_filename, const char *frag_filename);

void utility_draw_cube(GLint texcoord_loc, GLint normal_loc, GLint tangent_loc, GLint pos_loc);
void utility_draw_fullscreen_quad(GLint texcoord_loc, GLint pos_loc);

GLuint utility_set_clear_color(unsigned char r,  unsigned char g, unsigned b);

GLuint utility_load_texture_unknown();
GLuint utility_load_image(GLenum target, const char *filepath);

float utility_secs_since_launch();
float utility_mod_time(float modulus);

float utility_random01();
float utility_random_float(); //[-1, 1]
float utility_randomRange(float min, float max); // inclusive
