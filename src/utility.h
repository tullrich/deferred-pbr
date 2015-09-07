#pragma once
#include "common.h"

void __report_gl_err(const char * file, const char * func, int line);
#define GL_CHECK_ERROR() __report_gl_err(__FILE__,__FUNCTION__,__LINE__)


int deferred_buffer_file(const char *filename, unsigned char **buf, size_t *size);
GLuint deferred_create_shader(const char *filename, GLenum shader_type);
GLuint deferred_create_program(const char *vert_filename, const char *frag_filename);

GLuint deferred_load_texture_unknown();
void deferred_draw_cube(GLint texcoord_loc, GLint pos_loc);
