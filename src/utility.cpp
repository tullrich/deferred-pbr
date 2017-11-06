#include "utility.h"

#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
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
		lengths[0] = skip_len;
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
	glLinkProgram(program);

	GLint program_linked;
	glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
	if (program_linked != GL_TRUE) {
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

void utility_draw_cube(GLint texcoord_loc, GLint normal_loc, GLint tangent_log, GLint pos_loc, float min, float max) {
	glBegin(GL_QUADS);
		// back face
		glVertexAttrib3f(tangent_log, -1, 0, 0);
		glVertexAttrib3f(normal_loc, 0, 0, -1);
		glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, min, min, min);
		glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, min, max, min);
		glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, max, max, min);
		glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, max, min, min);

		// front face
		glVertexAttrib3f(tangent_log, 1, 0, 0);
		glVertexAttrib3f(normal_loc, 0, 0, 1);
		glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, max, max, max);
		glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, min, max, max);
		glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, min, min, max);
		glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, max, min, max);

		// left face
		glVertexAttrib3f(tangent_log, 0, 0, 1);
		glVertexAttrib3f(normal_loc, -1, 0, 0);
		glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, min, max, max);
		glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, min, max, min);
		glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, min, min, min);
		glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, min, min, max);

		// right face
		glVertexAttrib3f(tangent_log, 0, 0, -1);
		glVertexAttrib3f(normal_loc, 1, 0, 0);
		glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, max, max, max);
		glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, max, min, max);
		glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, max, min, min);
		glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, max, max, min);

		// bottom face
		glVertexAttrib3f(tangent_log, -1, 0, 0);
		glVertexAttrib3f(normal_loc, 0, -1, 0);
		glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, max, min, max);
		glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, min, min, max);
		glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, min, min, min);
		glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, max, min, min);

		// top face
		glVertexAttrib3f(tangent_log, 1, 0, 0);
		glVertexAttrib3f(normal_loc, 0, 1, 0);
		glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, max, max, max);
		glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, max, max, min);
		glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, min, max, min);
		glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, min, max, max);
	glEnd();
}

void utility_draw_fullscreen_quad(GLint texcoord_loc, GLint pos_loc) {
	glBegin(GL_QUADS);
		glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib2f(pos_loc, 1.0f, -1.0f);
		glVertexAttrib2f(texcoord_loc, 1, 1.0f); glVertexAttrib2f(pos_loc, 1.0f, 1.0f);
		glVertexAttrib2f(texcoord_loc, 0, 1.0f); glVertexAttrib2f(pos_loc, -1.0f, 1.0f);
		glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib2f(pos_loc, -1.0f, -1.0f);
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
static GLenum gl_cubemap_targets[] = {
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
