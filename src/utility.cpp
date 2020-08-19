#include "utility.h"
#include "scene.h"

#include "imgui/ImGuizmo.h"
#include "gli/gli.hpp"

void utility_report_gl_err(const char * file, const char * func, int line) {
	GLenum e;
	while ((e = glGetError()) != GL_NO_ERROR) {
		printf("RH %s:%s:%d gl error %s(%d)\n", file, func, line, gluErrorString(e), e);
	}
}

void utility_gl_debug_cb(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
	printf("GL Error: %s", message);
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

	GLuint shader;
	GL_WRAP(shader = glCreateShader(shader_type));
	GL_WRAP(glShaderSource(shader, i+1, sources, lengths));
	GL_WRAP(glCompileShader(shader));
	free(file_contents);

	GLint compiled_result;
	GL_WRAP(glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled_result));
	if (compiled_result != GL_TRUE) {
		GLsizei log_length = 0;
		GLchar message[1024];
		GL_WRAP(glGetShaderInfoLog(shader, 1024, &log_length, message));
		printf("Shader '%s' Compile Error: %s\n", filename, message);
		GL_WRAP(glDeleteShader(shader));
		return 0;
	}

	return shader;
}

GLuint utility_link_program(GLuint program) {
	GL_WRAP(glLinkProgram(program));

	GLint program_linked;
	GL_WRAP(glGetProgramiv(program, GL_LINK_STATUS, &program_linked));
	if (program_linked != GL_TRUE) {
		GLsizei log_length = 0;
		GLchar message[1024];
		GL_WRAP(glGetProgramInfoLog(program, 1024, &log_length, message));
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
		GL_WRAP(glDeleteShader(vert_shader));
		return 0;
	}

	GLuint program;
	GL_WRAP(program = glCreateProgram());
	GL_WRAP(glAttachShader(program, vert_shader));
	GL_WRAP(glAttachShader(program, frag_shader));
	if (utility_link_program(program)) {
		GL_WRAP(glDeleteProgram(program));
		program = 0;
	}

	GL_WRAP(glDeleteShader(vert_shader));
	GL_WRAP(glDeleteShader(frag_shader));

	printf("Loaded Program -- Vertex: '%s' Fragment: '%s' Defines: %i\n", vert_filename, frag_filename, defines_count);
	return program;
}

GLuint utility_load_texture_constant(const vec4 value) {
	GLuint texture_id;
	GL_WRAP(glGenTextures(1, &texture_id));
	GL_WRAP(glBindTexture(GL_TEXTURE_2D, texture_id));
	GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT));
	GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT));
	GL_WRAP(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_FLOAT, value));
	return texture_id;
}

GLuint utility_load_texture_unknown() {
	GLuint texture_id;
	GL_WRAP(glGenTextures(1, &texture_id));
	GL_WRAP(glBindTexture(GL_TEXTURE_2D, texture_id));
	GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT));
	GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT));
	unsigned char pixel[] = {
		0, 255, 0, 255,
		255, 0, 0, 255,
		0, 0, 255, 255,
		0, 0, 0, 255 };
	GL_WRAP(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel));
	return texture_id;
}

void utility_draw_cube(GLint texcoord_loc, GLint normal_loc, GLint tangent_loc, GLint pos_loc, float min, float max) {
	GL_WRAP(
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
	);
}

void utility_draw_fullscreen_quad(GLint texcoord_loc, GLint pos_loc) {
	GL_WRAP(
		glBegin(GL_QUADS);
			glVertexAttrib2f(texcoord_loc, 1.0f, 0.0f); glVertexAttrib2f(pos_loc, 1.0f, -1.0f);
			glVertexAttrib2f(texcoord_loc, 1.0f, 1.0f); glVertexAttrib2f(pos_loc, 1.0f, 1.0f);
			glVertexAttrib2f(texcoord_loc, 0.0f, 1.0f); glVertexAttrib2f(pos_loc, -1.0f, 1.0f);
			glVertexAttrib2f(texcoord_loc, 0.0f, 0.0f); glVertexAttrib2f(pos_loc, -1.0f, -1.0f);
		glEnd();
	);
}

void utility_draw_fullscreen_quad2(GLint texcoord_loc, GLint pos_loc) {
	GL_WRAP(
		glBegin(GL_QUADS);
			glVertexAttrib2f(pos_loc, 1.0f, -1.0f);
			glVertexAttrib2f(pos_loc, 1.0f, 1.0f);
			glVertexAttrib2f(pos_loc, -1.0f, 1.0f);
			glVertexAttrib2f(pos_loc, -1.0f, -1.0f);
		glEnd()
	);
}

static GLint components_to_gl_format(int components) {
	switch( components ) {
		case 1: return GL_RED;
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

GLuint utility_load_texture(GLenum target, const char *filepath) {
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

	GL_WRAP(glGenTextures(1, &texture_id));
	GL_WRAP(glBindTexture(target, texture_id));

	GL_WRAP(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GL_WRAP(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GL_WRAP(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GL_WRAP(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	GL_WRAP(glTexImage2D(target, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data));
	stbi_image_free(data);

	GL_WRAP(glGenerateMipmap(target));

	printf("Loaded Image -- '%s' Width: %i Height %i Components %i\n", filepath, width, height, components);

	return texture_id;
}

GLuint utility_load_cubemap(const char* const* filepaths) {
	GLuint texture_id;
	GL_WRAP(glGenTextures(1, &texture_id));
	GL_WRAP(glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id));

	int width, height;
	int components;
	unsigned char* data;
	GLint format;
	stbi_set_flip_vertically_on_load(0);
	for (int i = 0; i < 6; i++) {
		if(!(data = stbi_load(filepaths[i], &width, &height, &components, 0))) {
			GL_WRAP(glDeleteTextures(1, &texture_id));
			printf("Error loading stb image '%s' with error: %s\n", filepaths[i], stbi_failure_reason());
			return 0;
		}

		if ((format = components_to_gl_format(components)) == 0) {
			printf("Unsupported image format '%s', with %i channels.\n", filepaths[i], components);
			GL_WRAP(glDeleteTextures(1, &texture_id));
			stbi_image_free(data);
			return 0;
		}

		GL_WRAP(glTexImage2D(gl_cubemap_targets[i], 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data));
		stbi_image_free(data);
	}

	GL_WRAP(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
	GL_WRAP(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GL_WRAP(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GL_WRAP(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_WRAP(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));

	printf("Loaded Cubemap -- '%s' Width: %i Height %i Components %i\n", filepaths[0], width, height, components);
	return texture_id;
}

void utility_set_clear_color(unsigned char r,  unsigned char g, unsigned b) {
	//uint32_t c = 0x606060;
	//glClearColor((c&0xff)/255.0f, (c>>8&0xff)/255.0f, (c>>16&0xff)/255.0f, 1.0f);
	GL_WRAP(glClearColor(r/255.0f, g/255.0f, b/255.0f, 1.0f));
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

void utility_translation_gizmo(vec3 out, const mat4x4 view, const mat4x4 proj) {
	mat4x4 manip_mat;
	mat4x4_identity(manip_mat);
	vec3_dup(manip_mat[3], out);
	ImGuizmo::Enable(true);
	ImGuizmo::SetRect((float)VIEWPORT_X_OFFSET, 0.0f, (float)VIEWPORT_WIDTH, (float)VIEWPORT_HEIGHT);
	ImGuizmo::Manipulate(
		&view[0][0],
		&proj[0][0],
		ImGuizmo::TRANSLATE,
		ImGuizmo::LOCAL,
		&manip_mat[0][0]
	);
	vec3_dup(out, manip_mat[3]);
}

GLuint utility_load_texture_dds(const char* filepath) {
	gli::texture Texture = gli::load(filepath);
	if (Texture.empty()) {
		return 0;
	}

	gli::gl GL(gli::gl::PROFILE_ES30);
	gli::gl::format format = GL.translate(Texture.format(), Texture.swizzles());
	GLenum target = GL.translate(Texture.target());
	assert(Texture.extent().z == 1 || Texture.target() != gli::TARGET_2D);

	GLuint texture_id = 0;
	GL_WRAP(glGenTextures(1, &texture_id));
	GL_WRAP(glBindTexture(target, texture_id));
	GL_WRAP(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GL_WRAP(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GL_WRAP(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GL_WRAP(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_WRAP(glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0));
	GL_WRAP(glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, (GLint)Texture.levels() - 1));
	GL_WRAP(glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, &format.Swizzles[0]));

	glm::tvec3<GLsizei> extent = Texture.extent();
	printf("DDS %i: Extents: <%i, %i, %i> Faces: %i Levels: %i IsCube %i\n", texture_id, extent.x, extent.y, extent.z, Texture.faces(), Texture.levels(), gli::is_target_cube(Texture.target()));

  for (std::size_t face = 0; face < Texture.faces(); face++) {
		for (std::size_t level = 0; level < Texture.levels(); level++) {
			GLenum face_target = gli::is_target_cube(Texture.target()) ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X + face) : target;
		  glm::tvec3<GLsizei> level_extent = Texture.extent(level);
			printf("\tFace: %i Mip: %i Extents: <%i, %i, %i> Format: %#010x External %#010x\n", face, level, level_extent.x, level_extent.y, level_extent.z, format.Internal, format.External);
			if (gli::is_compressed(Texture.format())) {
				printf("\t\tCompressed Size: %i\n", Texture.size(level));
				GL_WRAP(glCompressedTexImage2D(
					face_target, level,
					format.Internal,
					level_extent.x, level_extent.y,
					0, Texture.size(level),
					Texture.data(0, face, level)
				));
			} else {
				GL_WRAP(glTexImage2D(
		 			face_target, level,
		 			format.Internal,
		 			level_extent.x, level_extent.y,
          0, format.External, format.Type,
		 			Texture.data(0, face, level)
				));
			}
		}
	}

	if (Texture.levels() <= 1) {
		GL_WRAP(glGenerateMipmap(target));
	}
	GL_WRAP(glBindTexture(target, 0));

	return texture_id;
}
