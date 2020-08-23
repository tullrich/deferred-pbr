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

static inline Bounds bounds_from_min_max(const vec3 min, const vec3 max) {
  Bounds b;
  for (int i = 0; i < 3; i++) {
    b.extents[i] = (max[i]-min[i])/2.0f;
    b.center[i] = min[i] + b.extents[i];
  }
  return b;
}

static inline void bounds_to_min_max(const Bounds* b, vec3 min, vec3 max) {
    for (int i = 0; i < 3; i++) {
      min[i] = b->center[i] - b->extents[i];
      max[i] = b->center[i] + b->extents[i];
    }
}

struct MeshDesc;

struct Mesh
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

  const MeshDesc* desc;
};

struct MeshDesc
{
  const char* name;
  const char* path;
  float base_scale;
  Mesh mesh;
};

void mesh_make_box(Mesh *out_mesh, float side_len);
void mesh_sphere_tessellate(Mesh *out_mesh, float radius, unsigned int rings, unsigned int sectors);
void mesh_make_quad(Mesh *out_mesh, float size_x, float size_z, float uv_scale);
void mesh_draw(const Mesh *mesh, GLint texcoord_loc, GLint normal_loc, GLint tangent_loc, GLint pos_loc);
void mesh_free(Mesh *out_mesh);
int mesh_load(Mesh *out_mesh, const MeshDesc* desc);
