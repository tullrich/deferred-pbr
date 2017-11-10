#pragma once
#include "common.h"

void utility_report_gl_err(const char * file, const char * func, int line);
#define GL_CHECK_ERROR() utility_report_gl_err(__FILE__,__FUNCTION__,__LINE__)

int utility_buffer_file(const char *filename, unsigned char **buf, size_t *size);
GLuint utility_create_shader(const char *filename, GLenum shader_type, const char** defines, int defines_count);
GLuint utility_link_program(GLuint program);
GLuint utility_create_program(const char *vert_filename, const char *frag_filename);
GLuint utility_create_program_defines(const char *vert_filename, const char *frag_filename, const char** defines, int defines_count);

void utility_draw_cube(GLint texcoord_loc, GLint normal_loc, GLint tangent_loc, GLint pos_loc, float min, float max);
void utility_draw_fullscreen_quad(GLint texcoord_loc, GLint pos_loc);
void utility_draw_fullscreen_quad2( GLint texcoord_loc, GLint pos_loc );
void utility_set_clear_color(unsigned char r,  unsigned char g, unsigned b);

typedef enum
{
	CUBEMAP_FRONT,
	CUBEMAP_BACK,
	CUBEMAP_TOP,
	CUBEMAP_BOTTOM,
	CUBEMAP_LEFT,
	CUBEMAP_RIGHT
} CubeMapFaces;

GLuint utility_load_texture_unknown();
GLuint utility_load_image(GLenum target, const char *filepath);
GLuint utility_load_cubemap(const char** filepaths);

float utility_secs_since_launch();
float utility_mod_time(float modulus);

int utility_random_bool();
float utility_random_real01(); //[0, 1]
float utility_random_real11(); //[-1, 1]
float utility_random_range(float min, float max); // [min, max]

typedef struct
{
	float *vertices;
	float *normals;
	float *tangents;
	float *texcoords;
	unsigned short *indices;

	unsigned int vertex_count;
	unsigned int index_count;
} SphereMesh;

void utility_sphere_tessellate(SphereMesh *out_mesh, float radius, unsigned int rings, unsigned int sectors);
void utility_sphere_draw(const SphereMesh *mesh, GLint texcoord_loc, GLint normal_loc, GLint tangent_loc, GLint pos_loc);
void utility_sphere_free(SphereMesh *out_mesh);
