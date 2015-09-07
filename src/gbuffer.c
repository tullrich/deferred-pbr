#include "gbuffer.h"


static void initialize_rendertexture(GBuffer *g_buffer)
{
	glGenTextures(1, &g_buffer->rendered_texture);
	glBindTexture(GL_TEXTURE_2D, g_buffer->rendered_texture);
 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA8, WINDOW_WIDTH, WINDOW_HEIGHT, 0,GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, WINDOW_WIDTH, WINDOW_HEIGHT, 0,GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_buffer->rendered_texture, 0);
}

static void initialize_depthbuffer(GBuffer *g_buffer)
{
	glGenRenderbuffers(1, &g_buffer->depth_render_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, g_buffer->depth_render_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, WINDOW_WIDTH, WINDOW_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, g_buffer->depth_render_buffer);
	//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, g_buffer->depth_render_buffer);

	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers);
}

int gbuffer_initialize(GBuffer *g_buffer)
{
	memset(g_buffer, 0, sizeof(GBuffer));

	// Initialize Gbuffer.
	glGenFramebuffers(1, &g_buffer->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer->fbo);

	initialize_rendertexture(g_buffer);
	initialize_depthbuffer(g_buffer);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return 1;

	return 0;
}

void gbuffer_bind(GBuffer *g_buffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer->fbo);
	glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);
}