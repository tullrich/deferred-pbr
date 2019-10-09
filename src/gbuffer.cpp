#include "gbuffer.h"

// Syntax for actual OpenGL RenderBuffer objects.
//	glGenRenderbuffers(1, &g_buffer->depth_render_buffer);
//	glBindRenderbuffer(GL_RENDERBUFFER, g_buffer->depth_render_buffer);
//	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, WINDOW_WIDTH, WINDOW_HEIGHT);
//	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_COMPONENT, GL_RENDERBUFFER, g_buffer->depth_render_buffer);
//	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, g_buffer->depth_render_buffer);

static GLuint generate_render_buffer() {
	GLuint render_buffer = 0;
	GL_WRAP(glGenTextures(1, &render_buffer));
	GL_WRAP(glBindTexture(GL_TEXTURE_2D, render_buffer));
 	GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
 	GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GL_WRAP(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	return render_buffer;
}

static void initialize_depthbuffer(GBuffer *g_buffer) {
	g_buffer->depth_render_buffer = generate_render_buffer();
	GL_WRAP(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0));
	GL_WRAP(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_buffer->depth_render_buffer, 0));
}

static GLuint initialize_attachment(GLenum attachment_slot, GLenum format) {
	GLuint attachment = generate_render_buffer();
	GL_WRAP(glTexImage2D(GL_TEXTURE_2D, 0, format, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, 0));
	GL_WRAP(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_slot, GL_TEXTURE_2D, attachment, 0));
	return attachment;
}

int gbuffer_initialize(GBuffer *g_buffer) {
	memset(g_buffer, 0, sizeof(GBuffer));

	// Initialize Gbuffer.
	GL_WRAP(glGenFramebuffers(1, &g_buffer->fbo));
	GL_WRAP(glBindFramebuffer(GL_FRAMEBUFFER, g_buffer->fbo));

	g_buffer->albedo_render_buffer = initialize_attachment(GL_COLOR_ATTACHMENT0, GL_RGBA32F);
	g_buffer->normal_render_buffer = initialize_attachment(GL_COLOR_ATTACHMENT1, GL_RGBA32F);
	g_buffer->roughness_render_buffer = initialize_attachment(GL_COLOR_ATTACHMENT2, GL_RGBA32F);
	g_buffer->metalness_render_buffer = initialize_attachment(GL_COLOR_ATTACHMENT3, GL_RGBA32F);
	initialize_depthbuffer(g_buffer);

	GLenum DrawBuffers[] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2,
		GL_COLOR_ATTACHMENT3
	};

	GL_WRAP(glDrawBuffers(STATIC_ELEMENT_COUNT(DrawBuffers), DrawBuffers));

	GLenum fbo_status;
	GL_WRAP(fbo_status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
	if (fbo_status != GL_FRAMEBUFFER_COMPLETE)
		return 1;

	return 0;
}

void gbuffer_bind(GBuffer *g_buffer) {
	GL_WRAP(glBindFramebuffer(GL_FRAMEBUFFER, g_buffer->fbo));
	GL_WRAP(glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT));
}
