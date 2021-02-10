#include "renderer.h"

int renderer_initialize(Renderer* r) {
  memset(r, 0, sizeof(Renderer));
  r->render_debug_lines = 1;

  int err = 0;
  if (err = deferred_initialize(&r->deferred)) {
    printf("Deferred renderer init failed\n");
    return err;
  }

  printf("<-- Initializing forward renderer... -->\n");
  if (err = forward_initialize(&r->forward)) {
    printf("Forward renderer init failed\n");
    return err;
  }

  printf("<-- Initializing debug line renderer... -->\n");
  if (err = debug_lines_initialize()) {
    printf("Debug line renderer init failed\n");
  }

  printf("<-- Initializing shadow map... -->\n");
  if (err = shadow_map_initialize(&r->shadow_map, VIEWPORT_WIDTH, VIEWPORT_HEIGHT)) {
    printf("Shadow map init failed\n");
    return err;
  }

  return 0;
}

void renderer_render(Renderer* r, const Scene* scene) {
  // clear backbuffer
  utility_set_clear_color(0, 0, 0);
  GL_WRAP(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

  // render offscreen shadowmap
  shadow_map_render(&r->shadow_map, scene);

  // render opaque objects
  deferred_render(&r->deferred, scene, &r->shadow_map);

  // render transparent objects, particles, and billboarded icons
  forward_render(&r->forward, scene);

  // render debug lines
  if (r->render_debug_lines) {
    if (scene->models[0]) {
      OBB obb;
      model_get_obb(scene->models[0], &obb);
      debug_lines_submit_obb(&obb, Green);
    }
    debug_lines_render(scene);
  } else {
    debug_lines_clear();
  }

  // render debug shadow map picture-in-picture
  if (r->debug_shadow_map) {
    const int width = 300;
    const int height = width * r->shadow_map.height/(float)r->shadow_map.width;
    shadow_map_render_debug(&r->shadow_map, VIEWPORT_X_OFFSET, VIEWPORT_HEIGHT - height, width, height);
  }
}
