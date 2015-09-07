#include "utility.h"
#include "deferred.h"

int deferred_initialize(Deferred* d)
{
	memset(d, 0, sizeof(Deferred));

	// Initialize box shader
	if(!(d->program = deferred_create_program("shaders/box.vert", "shaders/box.frag"))) {
		printf("Unable to load shaders\n");
		return 1;
	}

	d->pos_loc = glGetAttribLocation(d->program, "position");
	d->texcoord_loc = glGetAttribLocation(d->program, "texcoord");
	d->view_loc = glGetUniformLocation(d->program, "view");
	d->tex_loc = glGetUniformLocation(d->program, "tex");


	if(gbuffer_initialize(&d->g_buffer)) {
		printf("Unable to create g-buffer.\n");
		return 1;
	}

	d->default_texture = deferred_load_texture_unknown();

	return 0;
}

static void render_scene(Deferred* d, Scene *s, GLuint texture)
{
	glUseProgram(d->program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(d->tex_loc, 0);
	glUniformMatrix4fv(d->view_loc, 1, GL_FALSE, (GLfloat*)s->camera);
	deferred_draw_cube(d->texcoord_loc, d->pos_loc);
}

static void render_geometry(Deferred* d, Scene *s)
{
	gbuffer_bind(&d->g_buffer);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	render_scene(d, s, d->default_texture);
}

static void render_shading(Deferred* d, Scene *s)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);

	uint32_t c = 0x606060;
	glClearColor((c&0xff)/255.0f, (c>>8&0xff)/255.0f, (c>>16&0xff)/255.0f, 1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	render_scene(d, s, d->g_buffer.rendered_texture);
}

void deferred_render(Deferred *d, Scene *s)
{
	render_geometry(d, s);
	render_shading(d, s);
}