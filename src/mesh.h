#pragma once
#include "common.h"

// Bounds of mesh
typedef struct
{
	vec3 center;
	vec3 extents; // half of the size of the Bounds
} Bounds;

// Directy set the center and extents
static inline void bounds_set(Bounds *b, vec3 center, vec3 extents) {
	vec3_dup(b->center, center);
	vec3_dup(b->extents, extents);
}

static inline Bounds bounds_from_min_max(vec3 min, vec3 max) {
	Bounds b;
	for (int i = 0; i < 3; i++) {
		b.extents[i] = (max[i]-min[i])/2.0f;
		b.center[i] = min[i] + b.extents[i];
	}
	return b;
}

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

	Bounds bounds;
	float base_scale;
} Mesh;

void mesh_make_box(Mesh *out_mesh, float side_len);
void mesh_sphere_tessellate(Mesh *out_mesh, float radius, unsigned int rings, unsigned int sectors);
void mesh_draw(const Mesh *mesh, GLint texcoord_loc, GLint normal_loc, GLint tangent_loc, GLint pos_loc);
void mesh_free(Mesh *out_mesh);
int mesh_load_obj(Mesh *out_mesh, const char *filepath, float base_scale = 1.0f);


typedef struct
{
	const char* name;
	const char* path;
	float base_scale;
	Mesh mesh;
} MeshDesc;
