#include "utility.h"

void __report_gl_err(const char * file, const char * func, int line)
{
	GLenum e;
	while ((e = glGetError()) != GL_NO_ERROR) {

		printf("RH %s:%s:%d gl error %s(%d)\n", file, func, line, gluErrorString(e), e);

	}
}


int deferred_buffer_file(const char *filename, unsigned char **buf, size_t *size)
{
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
GLuint deferred_create_shader(const char *filename, GLenum shader_type)
{
	unsigned char* buf;
	size_t s;
	if (deferred_buffer_file(filename, &buf, &s)) {
		printf("Unable to open shader file %s.\n", filename);
		return 0;
	}

	GLuint shader = glCreateShader(shader_type);
	glShaderSource(shader, 1, (const GLchar**)&buf, (GLint*)&s);
	glCompileShader(shader);
	free(buf);

	GLint compiled_result;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled_result);
	if (compiled_result != GL_TRUE)
	{
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetShaderInfoLog(shader, 1024, &log_length, message);
		printf("Shader Compile Error: %s\n", message);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint deferred_create_program(const char *vert_filename, const char *frag_filename)
{
	GLint vert_shader;
	if (!(vert_shader = deferred_create_shader(vert_filename, GL_VERTEX_SHADER))) {
		return 0;
	}

	GLint frag_shader;
	if (!(frag_shader = deferred_create_shader(frag_filename, GL_FRAGMENT_SHADER))) {
		glDeleteShader(vert_shader);
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);

	GLint program_linked;
	glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
	if (program_linked != GL_TRUE)
	{
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

GLuint deferred_load_texture_unknown()
{
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

void deferred_draw_cube(GLint texcoord_loc, GLint pos_loc)
{
	glBegin(GL_QUADS);
	// back face
	glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, 0, 0, 0);
	glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, 0, 1, 0);
	glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, 1, 1, 0);
	glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, 1, 0, 0);

	// front face
	glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, 1, 1, 1);
	glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, 0, 1, 1);
	glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, 0, 0, 1);
	glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, 1, 0, 1);

	// left face
	glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, 0, 1, 1);
	glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, 0, 1, 0);
	glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, 0, 0, 0);
	glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, 0, 0, 1);

	// right face
	glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, 1, 1, 1);
	glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, 1, 0, 1);
	glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, 1, 0, 0);
	glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, 1, 1, 0);

	// bottom face
	glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, 1, 0, 1);
	glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, 0, 0, 1);
	glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, 0, 0, 0);
	glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, 1, 0, 0);

	// top face
	glVertexAttrib2f(texcoord_loc, 1, 0); glVertexAttrib3f(pos_loc, 1, 1, 1);
	glVertexAttrib2f(texcoord_loc, 1, 1); glVertexAttrib3f(pos_loc, 1, 1, 0);
	glVertexAttrib2f(texcoord_loc, 0, 1); glVertexAttrib3f(pos_loc, 0, 1, 0);
	glVertexAttrib2f(texcoord_loc, 0, 0); glVertexAttrib3f(pos_loc, 0, 1, 1);
	glEnd();
}