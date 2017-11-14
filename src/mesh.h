#pragma once
#include "common.h"

typedef struct
{
	float *vertices;
	float *normals;
	float *tangents;
	float *texcoords;
	unsigned int *indices;

	GLenum mode;
	unsigned int vertex_count;
	unsigned int index_count;
} Mesh;

void mesh_sphere_tessellate(Mesh *out_mesh, float radius, unsigned int rings, unsigned int sectors);
void mesh_draw(const Mesh *mesh, GLint texcoord_loc, GLint normal_loc, GLint tangent_loc, GLint pos_loc);
void mesh_free(Mesh *out_mesh);
int mesh_load_obj(Mesh *out_mesh, const char *filepath);
