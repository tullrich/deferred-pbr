#include "mesh.h"

static const float box_vertices[] = {
	// back face
	-0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f,
	0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f,

	// front face
	0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
	-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,

	// right face
	-0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f,
	-0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,

	// left face
	0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,
	0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f,

	// bottom face
	-0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f,
	0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f,

	// top face
	-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
	0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f
};

static const float box_tangents[] = {
	-1, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 1,
	1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1,
	0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1,
	0, 0, -1, 1, 0, 0, -1, 1, 0, 0, -1, 1, 0, 0, -1, 1,
	1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1,
	1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1,
};

static const float box_normals[] = {
	0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1,
	0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
	-1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0,
	1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,
	0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0,
	0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0
};

static const float box_texcoords[] = {
	1, 0, 1, 1, 0, 1, 0, 0,
	1, 1, 0, 1, 0, 0, 1, 0,
	1, 1, 0, 1, 0, 0, 1, 0,
	1, 1, 0, 1, 0, 0, 1, 0,
	0, 0, 1, 0, 1, 1, 0, 1,
	0, 0, 1, 0, 1, 1, 0, 1
};

void mesh_make_box(Mesh *out_mesh, float side_len) {
	memset(out_mesh, 0, sizeof(Mesh));
	out_mesh->mode = GL_QUADS;
	out_mesh->vertex_count = 24;
	out_mesh->index_count = 24;
	vec3_swizzle(out_mesh->bounds.extents, 0.5f*side_len);
	out_mesh->vertices = (float*)malloc(sizeof(box_vertices));
	for (int i = 0; i < STATIC_ELEMENT_COUNT(box_vertices); i++) {
		out_mesh->vertices[i] = box_vertices[i]*side_len;
	}
	out_mesh->normals = (float*)malloc(sizeof(box_normals));
	memcpy(out_mesh->normals, box_normals, sizeof(box_normals));
	out_mesh->tangents = (float*)malloc(sizeof(box_tangents));
	memcpy(out_mesh->tangents, box_tangents, sizeof(box_tangents));
	out_mesh->texcoords = (float*)malloc(sizeof(box_texcoords));
	memcpy(out_mesh->texcoords, box_texcoords, sizeof(box_texcoords));
}

// Adapted from https://stackoverflow.com/questions/7946770/calculating-a-sphere-in-opengl
void mesh_sphere_tessellate(Mesh *out_mesh, float radius, unsigned int rings, unsigned int sectors) {
    const float R = 1.f/(float)(rings-1);
    const float S = 1.f/(float)(sectors-1);

	float* v = (float*)malloc(rings * sectors * 3 * sizeof(float));
	float* n = (float*)malloc(rings * sectors * 3 * sizeof(float));
	float* ta = (float*)malloc(rings * sectors * 4 * sizeof(float));
	float* t = (float*)malloc(rings * sectors * 2 * sizeof(float));
	unsigned int* indices = (unsigned int*)malloc((rings-1) * (sectors-1) * 4 * sizeof(unsigned int));

	memset(out_mesh, 0, sizeof(Mesh));
	out_mesh->vertices = v;
	out_mesh->normals = n;
	out_mesh->tangents = ta;
	out_mesh->texcoords = t;
	out_mesh->indices = indices;
	out_mesh->vertex_count = rings * sectors;
	out_mesh->index_count = (rings-1) * (sectors-1) * 4;
	out_mesh->mode = GL_QUADS;
	vec3_swizzle(out_mesh->bounds.extents, radius);

	unsigned int r, s;
    for(r = 0; r < rings; r++) {
		for(s = 0; s < sectors; s++) {
			const float y = (float)(sin( -M_PI_2 + M_PI * r * R ));
            const float x = (float)(cos(2*M_PI * s * S) * sin( M_PI * r * R ));
            const float z = (float)(sin(2*M_PI * s * S) * sin( M_PI * r * R ));
			*t++ = s*S;
			*t++ = r*R;
			*v++ = x * radius;
            *v++ = y * radius;
            *v++ = z * radius;
            *n++ = x;
            *n++ = y;
			*n++ = z;

			ta[0] = -z; ta[1] = 0; ta[2] = x;
			if (vec3_len(ta) != 0.0f) {
				vec3_norm(ta, ta);
			} else {
				ta[0] = 0; ta[1] = 0; ta[2] = 1;
			}
			ta[3] = -1.0f;
			ta += 4;
		}
	}

    for(r = 0; r < rings-1; r++) {
		for(s = 0; s < sectors-1; s++) {
			*indices++ = (r+1) * sectors + s;
			*indices++ = (r+1) * sectors + (s+1);
			*indices++ = r * sectors + (s+1);
			*indices++ = r * sectors + s;
		}
    }
}

void mesh_draw(const Mesh *mesh, GLint texcoord_loc, GLint normal_loc, GLint tangent_loc, GLint pos_loc) {
	assert(mesh->vertices && pos_loc >= 0);
	glEnableVertexAttribArray(pos_loc);
	glVertexAttribPointer(pos_loc, 3, GL_FLOAT, GL_FALSE, 0, mesh->vertices);
	if (normal_loc >= 0) {
		if (mesh->normals) {
			glEnableVertexAttribArray(normal_loc);
			glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, 0, mesh->normals);
		} else {
			glDisableVertexAttribArray(normal_loc);
			glVertexAttrib4f(normal_loc, 0.f, 0.f, 0.f, 0.f);
		}
	}
	if (tangent_loc >= 0) {
		if (mesh->tangents) {
			glEnableVertexAttribArray(tangent_loc);
			glVertexAttribPointer(tangent_loc, 4, GL_FLOAT, GL_FALSE, 0, mesh->tangents);
		}
		else {
			glDisableVertexAttribArray(tangent_loc);
			glVertexAttrib4f(tangent_loc, 0.f, 0.f, 0.f, 0.f);
		}
	}
	if (texcoord_loc >= 0) {
		if (mesh->texcoords) {
			glEnableVertexAttribArray(texcoord_loc);
			glVertexAttribPointer(texcoord_loc, 2, GL_FLOAT, GL_FALSE, 0, mesh->texcoords);
		}
		else {
			glDisableVertexAttribArray(texcoord_loc);
			glVertexAttrib4f(texcoord_loc, 0.f, 0.f, 0.f, 0.f);
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	if (mesh->indices) {
		glDrawElements(mesh->mode, mesh->index_count, GL_UNSIGNED_INT, mesh->indices);
	} else {
		glDrawArrays(mesh->mode, 0, mesh->vertex_count);
	}

	glDisableVertexAttribArray(pos_loc);
	if (normal_loc >= 0) glDisableVertexAttribArray(normal_loc);
	if (tangent_loc >= 0) glDisableVertexAttribArray(tangent_loc);
	if (texcoord_loc >= 0) glDisableVertexAttribArray(texcoord_loc);
}

void mesh_free(Mesh *out_mesh) {
	free(out_mesh->vertices);
	free(out_mesh->normals);
	free(out_mesh->tangents);
	free(out_mesh->texcoords);
	free(out_mesh->indices);
	memset(out_mesh, 0, sizeof(Mesh));
}

static void calc_tangent(const vec3 v1, const vec3 v2, const vec3 v3,
						 const vec2 n1, const vec2 n2, const vec2 n3,
						 const vec2 tc1, const vec2 tc2, const vec2 tc3,
						 vec4 ta1, vec4 ta2, vec4 ta3 ) {
	// D = F.s * T + F.t * U
	// E = G.s * T + G.t * U
	vec3 D, E;
	vec3_sub(D, v2, v1);
	vec3_sub(E, v3, v1);

	vec2 F, G;
	vec2_sub(F, tc2, tc1);
	vec2_sub(G, tc3, tc1);

	// | T.x T.y T.z |           1         |  G.t  -F.t | | D.x D.y D.z |
	// |             | = ----------------- |            | |             |
	// | U.x U.y U.z |   F.s G.t - F.t G.s | -G.s   F.s | | E.x E.y E.z |
	vec3 T, U;
	float r = 1.0f / (F[0] * G[1] - G[0] * F[1]);
	T[0] = (G[1] * D[0] - F[1] * E[0]) * r;
	T[1] = (G[1] * D[1] - F[1] * E[1]) * r;
	T[2] = (G[1] * D[2] - F[1] * E[2]) * r;
	U[0] = (F[0] * E[0] - G[0] * D[0]) * r;
	U[1] = (F[0] * E[1] - G[0] * D[1]) * r;
	U[2] = (F[0] * E[2] - G[0] * D[2]) * r;

	// T' = T - (N�T) N
	vec3 temp;
#define V_TANGENT(nX, taX)										\
	vec3_scale(temp, nX, vec3_mul_inner(nX, T));				\
	vec3_sub(temp, T, temp);									\
	vec3_norm(taX, temp);										\
	vec3_mul_cross(temp, nX, T);								\
	taX[3] =  (vec3_mul_inner(temp, U) < 0.0f) ? -1.0f : 1.0f;

	V_TANGENT(n1, ta1);
	V_TANGENT(n2, ta2);
	V_TANGENT(n3, ta3);
}

static void mesh_split_verts(Mesh *out_mesh, const tinyobj_attrib_t *attrib) {
	assert(attrib->num_vertices > 0 && attrib->num_normals > 0); // required vertex attributes

	out_mesh->vertex_count = attrib->num_faces;
	out_mesh->vertices = (float*)malloc(attrib->num_faces*3*sizeof(float));;
	out_mesh->normals = (float*)malloc(attrib->num_faces*3*sizeof(float));

	if (attrib->num_texcoords > 0) {
		out_mesh->texcoords = (float*)malloc(attrib->num_faces*2*sizeof(float));
	}

	// Copy faces
	float *vert_next = out_mesh->vertices;
	float *norm_next = out_mesh->normals;
	float *texcoords_next = out_mesh->texcoords;
	for (unsigned int i = 0; i < attrib->num_faces; i+=3) {
		tinyobj_vertex_index_t *idx0 = &attrib->faces[i];
		tinyobj_vertex_index_t *idx1 = &attrib->faces[i+1];
		tinyobj_vertex_index_t *idx2 = &attrib->faces[i+2];
		tinyobj_vertex_index_t* face[3] ={idx0, idx1, idx2};
		for (int v = 0; v < 3; v++) {
			vec3_dup(vert_next+v*3, &attrib->vertices[face[v]->v_idx*3]);
			vec3_dup(norm_next+v*3, &attrib->normals[face[v]->vn_idx*3]);

			if (texcoords_next)
				vec2_dup(texcoords_next+v*2, &attrib->texcoords[face[v]->vt_idx*2]);
		}

		vert_next += 9;
		norm_next += 9;
		if (texcoords_next) texcoords_next += 6;
	}
}

#if 0
static void build_adjacency() {
	typedef struct
	{
		unsigned int face_indices[8];
		unsigned char count;
	} vertex_adjacency;

	vertex_adjacency *adjacency_list = (vertex_adjacency*)calloc(1, out_mesh->vertex_count*sizeof(vertex_adjacency));
	// Copy faces and build adjacency list
	for (unsigned int i = 0; i < attrib.num_faces; i+=3) {
		tinyobj_vertex_index_t *idx0 = &attrib.faces[i];
		tinyobj_vertex_index_t *idx1 = &attrib.faces[i+1];
		tinyobj_vertex_index_t *idx2 = &attrib.faces[i+2];

		vertex_adjacency* adj0 = &adjacency_list[idx0->v_idx];
		assert(adj0->count<8);
		adj0->face_indices[adj0->count++] = i;

		vertex_adjacency* adj1 = &adjacency_list[idx1->v_idx];
		assert(adj1->count<8);;
		adj1->face_indices[adj1->count++] = i+1;

		vertex_adjacency* adj2 = &adjacency_list[idx2->v_idx];
		assert(adj2->count<8);
		adj2->face_indices[adj2->count++] = i+2;
	}
}
#endif

static void compute_mesh_normals(tinyobj_attrib_t *out_attrib) {
	assert(out_attrib->num_vertices > 0 && out_attrib->num_faces > 0);
	tinyobj_vertex_index_t *indices = out_attrib->faces;
	const unsigned int face_count = out_attrib->num_faces;
	const float *vertices = out_attrib->vertices;

	float *normals = (float*)calloc(1, out_attrib->num_vertices*3*sizeof(float));

	//  For each face, compute normal and aggregate them for each shared vertex
	for (unsigned int i = 0; i < face_count; i+=3) {
		int v_idx0 = out_attrib->faces[i].v_idx*3;
		int v_idx1 = out_attrib->faces[i+1].v_idx*3;
		int v_idx2 = out_attrib->faces[i+2].v_idx*3;

		vec3 edge1, edge2;
		vec3_sub(edge1, &vertices[v_idx1], &vertices[v_idx0]);
		vec3_sub(edge2, &vertices[v_idx2], &vertices[v_idx0]);

		vec3 face_normal;
		vec3_mul_cross(face_normal, edge1, edge2);

		// Add face contribution to each vertex
		vec3_add(&normals[v_idx0], &normals[v_idx0], face_normal);
		vec3_add(&normals[v_idx1], &normals[v_idx1], face_normal);
		vec3_add(&normals[v_idx2], &normals[v_idx2], face_normal);

		// Assign vertex normals to the face
		out_attrib->faces[i].vn_idx = v_idx0/3;
		out_attrib->faces[i+1].vn_idx = v_idx1/3;
		out_attrib->faces[i+2].vn_idx = v_idx2/3;
	}

	// Smooth normals (normalize)
	for (unsigned int i = 0; i < out_attrib->num_vertices; i++) {
		vec3_norm(&normals[i*3], &normals[i*3]);
	}

	out_attrib->num_normals = out_attrib->num_vertices;
	out_attrib->normals = normals;
}

static Bounds compute_mesh_bounds(const Mesh* mesh) {
	const float *vertices = mesh->vertices;
	const unsigned int vertex_count = mesh->vertex_count;

	vec3 min = {0.0f}, max = {0.0f};
	if (vertex_count) {
		vec3_dup(min, vertices);
		vec3_dup(max, vertices);
		for (unsigned int i = 0; i < vertex_count; i++) {
			const float *vert = &vertices[i*3];
			for (int j = 0; j < 3; j++) {
				if (min[j] > vert[j]) min[j] = vert[j];
				if (max[j] < vert[j]) max[j] = vert[j];
			}
		}
	}
	return bounds_from_min_max(min, max);;
}

static float* compute_mesh_tangents(const tinyobj_attrib_t *attrib) {
	assert(attrib->num_texcoords > 0 && attrib->num_normals > 0);
	const tinyobj_vertex_index_t *indices = attrib->faces;
	const unsigned int face_count = attrib->num_faces;
	const float *vertices = attrib->vertices;
	const float *normals = attrib->normals;
	const float *texcoords = attrib->texcoords;

	//  For each face, compute tangents and aggregate them for each shared vertex
	float *tangents = (float*)malloc(attrib->num_faces*4*sizeof(float));
	for (unsigned int i = 0; i < attrib->num_faces; i+=3) {
		tinyobj_vertex_index_t *idx0 = &attrib->faces[i];
		tinyobj_vertex_index_t *idx1 = &attrib->faces[i+1];
		tinyobj_vertex_index_t *idx2 = &attrib->faces[i+2];
		calc_tangent(&vertices[idx0->v_idx*3], &vertices[idx1->v_idx*3], &vertices[idx2->v_idx*3],
					 &normals[idx0->vn_idx*3], &normals[idx1->vn_idx*3], &normals[idx2->vn_idx*3],
					 &texcoords[idx0->vt_idx*2], &texcoords[idx1->vt_idx*2], &texcoords[idx2->vt_idx*2],
					 &tangents[i*4], &tangents[(i+1)*4], &tangents[(i+2)*4]);
	}

	return tangents;
}

int mesh_load_obj(Mesh *out_mesh, const char *filepath) {
	int ret = 0;

	// Read mesh file
	size_t file_len;
	char* file_contents;
	if (utility_buffer_file(filepath, (unsigned char**)&file_contents, &file_len)) {
		printf("Unable to open mesh file %s.\n", filepath);
		return 1;
	}

	memset(out_mesh, 0, sizeof(Mesh));

	// Parse contents
	tinyobj_attrib_t attrib;
	tinyobj_shape_t* shapes;
	tinyobj_material_t* materials;
	size_t num_shapes, num_materials;
	if (tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials, &num_materials, file_contents, file_len, TINYOBJ_FLAG_TRIANGULATE) != TINYOBJ_SUCCESS) {
		ret = 1;
		goto free_file_contents_and_exit;
	}

	// Compute smooth normals if not provided (pre-split)
	if (attrib.num_normals <= 0) {
		compute_mesh_normals(&attrib);
	}

	// Split shared verts
	mesh_split_verts(out_mesh, &attrib);
	out_mesh->mode = GL_TRIANGLES;

	// Compute optional tangents (if texcoords are provided, post-split)
	if (attrib.num_normals > 0 && attrib.num_texcoords > 0) {
		out_mesh->tangents = compute_mesh_tangents(&attrib);
	}

	// Compute bounds from vertex positions
	out_mesh->bounds = compute_mesh_bounds(out_mesh);

	// Free tinyobj data
	tinyobj_shapes_free(shapes, num_shapes);
	tinyobj_materials_free(materials, num_materials);
	tinyobj_attrib_free(&attrib);

	printf("Loaded Mesh -- '%s' Vertices: %i UVs: %s\n", filepath, out_mesh->vertex_count, BOOL_TO_STRING(out_mesh->texcoords != NULL));

free_file_contents_and_exit:
	free(file_contents);
	return ret;
}
