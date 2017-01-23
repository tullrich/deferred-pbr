#include "gbuffer.h"

// Syntax for actual OpenGL RenderBuffer objects.
//glGenRenderbuffers(1, &g_buffer->depth_render_buffer);
//glBindRenderbuffer(GL_RENDERBUFFER, g_buffer->depth_render_buffer);
//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, WINDOW_WIDTH, WINDOW_HEIGHT);
//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_COMPONENT, GL_RENDERBUFFER, g_buffer->depth_render_buffer);
//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, g_buffer->depth_render_buffer);

static GLuint generate_render_buffer()
{
	GLuint render_buffer = 0;
	glGenTextures(1, &render_buffer);
	glBindTexture(GL_TEXTURE_2D, render_buffer);
 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	return render_buffer;
}

static void initialize_depthbuffer(GBuffer *g_buffer)
{
	g_buffer->depth_render_buffer = generate_render_buffer();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_buffer->depth_render_buffer, 0);
	GL_CHECK_ERROR();
}

static void initialize_positionbuffer(GBuffer *g_buffer)
{
	g_buffer->position_render_buffer = generate_render_buffer();
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_buffer->position_render_buffer, 0);
	GL_CHECK_ERROR();
}

static void initialize_rendertexture(GBuffer *g_buffer)
{
	g_buffer->diffuse_render_buffer = generate_render_buffer();
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_buffer->diffuse_render_buffer, 0);
	GL_CHECK_ERROR();
}

static void initialize_normalbuffer(GBuffer *g_buffer)
{
	g_buffer->normal_render_buffer = generate_render_buffer();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	GL_CHECK_ERROR();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, g_buffer->normal_render_buffer, 0);
	GL_CHECK_ERROR();
}

static void initialize_specularbuffer(GBuffer *g_buffer)
{
	g_buffer->specular_render_buffer = generate_render_buffer();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
	GL_CHECK_ERROR();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, g_buffer->specular_render_buffer, 0);
	GL_CHECK_ERROR();
}

int gbuffer_initialize(GBuffer *g_buffer)
{
	memset(g_buffer, 0, sizeof(GBuffer));

	// Initialize Gbuffer.
	glGenFramebuffers(1, &g_buffer->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer->fbo);

	initialize_depthbuffer(g_buffer);
	initialize_rendertexture(g_buffer);
	initialize_positionbuffer(g_buffer);
	initialize_normalbuffer(g_buffer);
	initialize_specularbuffer(g_buffer);

	GLenum DrawBuffers[] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2,
		GL_COLOR_ATTACHMENT3
	};

	glDrawBuffers(STATIC_ELEMENT_COUNT(DrawBuffers), DrawBuffers);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return 1;

	return 0;
}

void gbuffer_bind(GBuffer *g_buffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer->fbo);
	glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);
}
