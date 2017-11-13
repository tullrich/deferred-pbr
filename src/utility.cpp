#include "utility.h"

#ifdef _WIN32
#define M_PI_2 (M_PI/2.0)
#endif

void utility_report_gl_err(const char * file, const char * func, int line) {
	GLenum e;
	while ((e = glGetError()) != GL_NO_ERROR) {
		printf("RH %s:%s:%d gl error %s(%d)\n", file, func, line, gluErrorString(e), e);
	}
}

int utility_buffer_file(const char *filename, unsigned char **buf, size_t *size) {
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
GLuint utility_create_shader(const char *filename, GLenum shader_type, const char** defines, int defines_count) {
	if (defines_count > 30)
		return 0;

	const char* sources[32];
	GLint lengths[32];
	sources[0] = "#version 130\r\n";
	lengths[0] = (GLint)strlen(sources[0]);
	int i = 1;
	for (int j = 0; j < 30 && j < defines_count; j++) {
		sources[i] = (const GLchar*)defines[j];
		lengths[i] = (GLint)strlen(defines[j]);
		i++;
	}

	size_t file_len;
	char* file_contents;
	if (utility_buffer_file(filename, (unsigned char**)&file_contents, &file_len)) {
		printf( "Unable to open shader file %s.\n", filename );
		return 0;
	}

	// Find the version pragma
	size_t skip_len = 0;
	if (!strncmp(file_contents, "#version",8)) {
		for (size_t j = 0; j < file_len; j++) {
			if (file_contents[j] == '\n') {
				skip_len = j+1;
				break;
			}
		}
		sources[0] = file_contents;
		lengths[0] = (GLint)skip_len;
	}
	sources[i] = file_contents + skip_len;
	lengths[i] = (GLint)(file_len - skip_len);

	GLuint shader = glCreateShader(shader_type);
	glShaderSource(shader, i+1, sources, lengths);
	glCompileShader(shader);
	free(file_contents);

	GLint compiled_result;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled_result);
	if (compiled_result != GL_TRUE) {
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetShaderInfoLog(shader, 1024, &log_length, message);
		printf("Shader Compile Error: %s\n", message);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint utility_link_program(GLuint program) {
	glLinkProgram(program);

	GLint program_linked;
	glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
	if (program_linked != GL_TRUE) {
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetProgramInfoLog(program, 1024, &log_length, message);
		printf("Shader Link Error: %s\n", message);
		return 1;
	}
	return 0;
}

GLuint utility_create_program(const char *vert_filename, const char *frag_filename) {
	return utility_create_program_defines(vert_filename , frag_filename, NULL, 0);
}

GLuint utility_create_program_defines(const char *vert_filename, const char *frag_filename, const char** defines, int defines_count ) {
	GLint vert_shader;
	if (!(vert_shader = utility_create_shader(vert_filename, GL_VERTEX_SHADER, defines, defines_count))) {
		return 0;
	}

	GLint frag_shader;
	if (!(frag_shader = utility_create_shader(frag_filename, GL_FRAGMENT_SHADER, defines, defines_count))) {
		glDeleteShader(vert_shader);
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	if (utility_link_program(program)) {
		glDeleteProgram(program);
		program = 0;
	}

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	return program;
}

GLuint utility_load_texture_unknown() {
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

void utility_draw_cube(GLint texcoord_loc, GLint normal_loc, GLint tangent_loc, GLint pos_loc, float min, float max) {
	glBegin(GL_QUADS);
		// back face
		if (tangent_loc >= 0) glVertexAttrib4f(tangent_loc, -1, 0, 0, 1);
		glVertexAttrib3f(normal_loc, 0, 0, -1);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 1, 0);
		glVertexAttrib3f(pos_loc, min, min, min);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 1, 1);
		glVertexAttrib3f(pos_loc, min, max, min);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 0, 1);
		glVertexAttrib3f(pos_loc, max, max, min);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 0, 0);
		glVertexAttrib3f(pos_loc, max, min, min);

		// front face
		if (tangent_loc >= 0) glVertexAttrib4f(tangent_loc, 1, 0, 0, 1);
		glVertexAttrib3f(normal_loc, 0, 0, 1);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 1, 1);
		glVertexAttrib3f(pos_loc, max, max, max);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 0, 1);
		glVertexAttrib3f(pos_loc, min, max, max);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 0, 0);
		glVertexAttrib3f(pos_loc, min, min, max);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 1, 0);
		glVertexAttrib3f(pos_loc, max, min, max);

		// right face
		if (tangent_loc >= 0) glVertexAttrib4f(tangent_loc, 0, 0, 1, 1);
		glVertexAttrib3f(normal_loc, -1, 0, 0);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 1, 1);
		glVertexAttrib3f(pos_loc, min, max, max);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 0, 1);
		glVertexAttrib3f(pos_loc, min, max, min);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 0, 0);
		glVertexAttrib3f(pos_loc, min, min, min);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 1, 0);
		glVertexAttrib3f(pos_loc, min, min, max);

		// left face
		if (tangent_loc >= 0) glVertexAttrib4f(tangent_loc, 0, 0, -1, 1);
		glVertexAttrib3f(normal_loc, 1, 0, 0);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 1, 1);
		glVertexAttrib3f(pos_loc, max, max, min);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 0, 1);
		glVertexAttrib3f(pos_loc, max, max, max);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 0, 0);
		glVertexAttrib3f(pos_loc, max, min, max);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 1, 0);
		glVertexAttrib3f(pos_loc, max, min, min);

		// bottom face
		if (tangent_loc >= 0) glVertexAttrib4f(tangent_loc, 1, 0, 0, 1);
		glVertexAttrib3f(normal_loc, 0, -1, 0);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 0, 0);
		glVertexAttrib3f(pos_loc, min, min, min);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 1, 0);
		glVertexAttrib3f(pos_loc, max, min, min);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 1, 1);
		glVertexAttrib3f(pos_loc, max, min, max);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 0, 1);
		glVertexAttrib3f(pos_loc, min, min, max);

		// top face
		if (tangent_loc >= 0) glVertexAttrib4f(tangent_loc, 1, 0, 0, 1);
		glVertexAttrib3f(normal_loc, 0, 1, 0);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 0, 0);
		glVertexAttrib3f(pos_loc, min, max, max);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 1, 0);
		glVertexAttrib3f(pos_loc, max, max, max);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 1, 1);
		glVertexAttrib3f(pos_loc, max, max, min);
		if (texcoord_loc >= 0) glVertexAttrib2f(texcoord_loc, 0, 1);
		glVertexAttrib3f(pos_loc, min, max, min);
	glEnd();
}

void utility_draw_fullscreen_quad(GLint texcoord_loc, GLint pos_loc) {
	glBegin(GL_QUADS);
		glVertexAttrib2f(texcoord_loc, 1.0f, 0.0f); glVertexAttrib2f(pos_loc, 1.0f, -1.0f);
		glVertexAttrib2f(texcoord_loc, 1.0f, 1.0f); glVertexAttrib2f(pos_loc, 1.0f, 1.0f);
		glVertexAttrib2f(texcoord_loc, 0.0f, 1.0f); glVertexAttrib2f(pos_loc, -1.0f, 1.0f);
		glVertexAttrib2f(texcoord_loc, 0.0f, 0.0f); glVertexAttrib2f(pos_loc, -1.0f, -1.0f);
	glEnd();
}

void utility_draw_fullscreen_quad2( GLint texcoord_loc, GLint pos_loc ) {
	glBegin( GL_QUADS );
		glVertexAttrib2f(pos_loc, 1.0f, -1.0f);
		glVertexAttrib2f(pos_loc, 1.0f, 1.0f);
		glVertexAttrib2f(pos_loc, -1.0f, 1.0f);
		glVertexAttrib2f(pos_loc, -1.0f, -1.0f);
	glEnd();
}

static GLint components_to_gl_format(int components) {
	switch( components ) {
		case 1: return GL_ALPHA;
		case 3: return GL_RGB;
		case 4: return GL_RGBA;
		default: return 0;
	}
}

// Keep in sync with CubeMapFaces enum
static const GLenum gl_cubemap_targets[] = {
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
};

GLuint utility_load_image(GLenum target, const char *filepath) {
	int width, height;
	int components;
	unsigned char* data;
	GLuint texture_id;
	GLint format;

	stbi_set_flip_vertically_on_load(1);
	if(!(data = stbi_load(filepath, &width, &height, &components, 0))) {
		printf("Error loading stb image '%s' with error: %s\n", filepath, stbi_failure_reason());
		return 0;
	}

	if ((format = components_to_gl_format(components)) == 0) {
		printf("Unsupported image format '%s', with %i channels.\n", filepath, components);
		stbi_image_free(data);
		return 0;
	}

	glGenTextures(1, &texture_id);
	glBindTexture(target, texture_id);

	glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(target, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);

	glGenerateMipmap(target);

	printf("Loaded Image '%s': Width: %i, Height %i, Components %i\n", filepath, width, height, components);

	return texture_id;
}

GLuint utility_load_cubemap(const char** filepaths) {
	GLuint texture_id;
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

	int width, height;
	int components;
	unsigned char* data;
	GLint format;
	stbi_set_flip_vertically_on_load(0);
	for (int i = 0; i < 6; i++) {
		if(!(data = stbi_load(filepaths[i], &width, &height, &components, 0))) {
			glDeleteTextures(1, &texture_id);
			printf("Error loading stb image '%s' with error: %s\n", filepaths[i], stbi_failure_reason());
			return 0;
		}

		if ((format = components_to_gl_format(components)) == 0) {
			printf("Unsupported image format '%s', with %i channels.\n", filepaths[i], components);
			glDeleteTextures(1, &texture_id);
			stbi_image_free(data);
			return 0;
		}

		glTexImage2D(gl_cubemap_targets[i], 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		stbi_image_free(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	printf("Loaded Cubemap '%s': Width: %i, Height %i, Components %i\n", filepaths[0], width, height, components);
	return texture_id;
}

void utility_set_clear_color(unsigned char r,  unsigned char g, unsigned b) {
	//uint32_t c = 0x606060;
	//glClearColor((c&0xff)/255.0f, (c>>8&0xff)/255.0f, (c>>16&0xff)/255.0f, 1.0f);
	glClearColor(r/255.0f, g/255.0f, b/255.0f, 1.0f);
}

float utility_secs_since_launch() {
	return SDL_GetTicks() * 0.001f; // in seconds
}

float utility_mod_time(float modulus) {
	return fmodf(utility_secs_since_launch(), modulus); // in seconds
}

int utility_random_bool() {
	return rand() >= (float)RAND_MAX/2;
}

float utility_random_real01() {
	return (float)rand() / (float)RAND_MAX;
}

float utility_random_real11() {
	return -1.0f + 2.0f * rand() / (float)RAND_MAX;
}

float utility_random_range(float min, float max) {
	return min + rand() / (float)RAND_MAX * (max - min);
}

// Adapted from https://stackoverflow.com/questions/7946770/calculating-a-sphere-in-opengl
void utility_mesh_sphere_tessellate(Mesh *out_mesh, float radius, unsigned int rings, unsigned int sectors)
{
    const float R = 1.f/(float)(rings-1);
    const float S = 1.f/(float)(sectors-1);

	float* v = (float*)malloc(rings * sectors * 3 * sizeof(float));
	float* n = (float*)malloc(rings * sectors * 3 * sizeof(float));
	float* ta = (float*)malloc(rings * sectors * 4 * sizeof(float));
	float* t = (float*)malloc(rings * sectors * 2 * sizeof(float));
	unsigned int* indices = (unsigned int*)malloc((rings-1) * (sectors-1) * 4 * sizeof(unsigned int));

	out_mesh->vertices = v;
	out_mesh->normals = n;
	out_mesh->tangents = ta;
	out_mesh->texcoords = t;
	out_mesh->indices = indices;
	out_mesh->vertex_count = rings * sectors;
	out_mesh->index_count = (rings-1) * (sectors-1) * 4;

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

void utility_mesh_draw(const Mesh *mesh, GLenum mode, GLint texcoord_loc, GLint normal_loc, GLint tangent_loc, GLint pos_loc) {
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
		glDrawElements(mode, mesh->index_count, GL_UNSIGNED_INT, mesh->indices);
	} else {
		glDrawArrays(mode, 0, mesh->vertex_count);
	}

	glDisableVertexAttribArray(pos_loc);
	if (normal_loc >= 0) glDisableVertexAttribArray(normal_loc);
	if (tangent_loc >= 0) glDisableVertexAttribArray(tangent_loc);
	if (texcoord_loc >= 0) glDisableVertexAttribArray(texcoord_loc);
}

void utility_mesh_free(Mesh *out_mesh) {
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

int utility_mesh_load(Mesh *out_mesh, const char *filepath) {
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

	// Compute optional tangents (if texcoords are provided, post-split)
	if (attrib.num_normals > 0 && attrib.num_texcoords > 0) {
		out_mesh->tangents = compute_mesh_tangents(&attrib);
	}

	// Free tinyobj data
	tinyobj_shapes_free(shapes, num_shapes);
	tinyobj_materials_free(materials, num_materials);
	tinyobj_attrib_free(&attrib);

free_file_contents_and_exit:
	free(file_contents);
	return ret;
}
