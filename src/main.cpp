#include "common.h"
#include "assets.h"
#include "gui.h"
#include "scene.h"
#include "renderer.h"
#include "physics.h"
#include "container.h"

static SDL_Window* gWindow;
static Renderer gRenderer;
static Scene gScene;
static PhysicsWorld gPhysWorld;

struct Entity {
  DECLATE_INTRUSIVE_LL_MEMBERS(Entity);
  Model model;
  PhysicsParticle particle;
  PhysicsForceGenerator* force_generator;
};

DECLARE_LINKED_LIST_TYPE(Entity, EntityList);
EntityList gEntities;

static void update_loading_screen(const char* stage, const char* asset, int index, int total) {
   // clear window during load
  SDL_GL_SwapWindow(gWindow);

  // pump events
  SDL_Event event;
  while (SDL_PollEvent(&event)) { }

  // Draw loading
  GL_WRAP(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  utility_set_clear_color(0, 0, 0);
  GL_WRAP(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
  gui_render_loading_screen(gWindow, stage, asset, index, total);

  // swap
  SDL_GL_SwapWindow(gWindow);
  GL_CHECK_ERROR();
}

static int setup_scene(int sphere_scene) {
  memset(&gScene, 0, sizeof(Scene));

  // Setup camera
  gScene.camera.boom_len = 30.0f;
  gScene.camera.fovy = 72.0f;
  gScene.camera.rot[0] = gScene.camera.rot[1] = -30.0f;

  // Setup ambient light
  vec3_dup(gScene.ambient_color, White);
  gScene.ambient_intensity = 0.03f;

  // Setup skybox
  gScene.skybox = &gSkyboxes[0].skybox;

  // Setup main directional light
  gScene.light = (Light*)malloc(sizeof(Light));
  vec3 lightPos = { 0.0f, 0.0f, 10.0f };
  light_initialize_point(gScene.light, lightPos, White, 100.0f);

  // Setup particle system
  ParticleEmitterDesc* desc = (ParticleEmitterDesc*)calloc(1, sizeof(ParticleEmitterDesc));
  gScene.emitters[0] = (ParticleEmitter*)malloc(sizeof(ParticleEmitter));
  particle_emitter_initialize(gScene.emitters[0], desc);
  gScene.emitters[0]->muted = true; // start muted

  // Setup model(s)
  if (!sphere_scene) {
    printf("Creating single model scene\n");
    gScene.models[0] = (Model*)malloc(sizeof(Model));
    model_initialize(gScene.models[0], &gMeshes[0].mesh, &gMaterials[0].material);
  } else {
    const int kSphereRows = 7;
    const int kSphereCols = 7;
    const int kSphereSpacing = 8.0f;
    printf("Creating %ix%i spheres scene\n", kSphereRows, kSphereCols);
    for (int i = 0; i < kSphereRows; i++) {
      for (int j = 0; j < kSphereCols; j++) {
        Model* m = (Model*)malloc(sizeof(Model));
        model_initialize(m, &gMeshes[0].mesh, &gMaterials[0].material);
        m->position[0] = (j-(kSphereCols/2)) * kSphereSpacing;
        m->position[1] = (i-(kSphereRows/2)) * kSphereSpacing;
        vec3_dup(m->material.albedo_base, White);
        m->material.roughness_base = std::max(j/((float)kSphereCols), 0.05f);
        m->material.metalness_base = i/((float)kSphereRows);
        int center_idx = kSphereRows * kSphereCols / 2;
        int idx = i * kSphereCols + j;
        idx = (idx == center_idx || idx == 0) ? center_idx - idx : idx;
        gScene.models[idx] = m;
      }
    }
  }

  // Setup floor
  // Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
  // mesh_make_quad(mesh, 200, 200, 3);
  // gScene.models[1] = (Model*)malloc(sizeof(Model));
  // model_initialize(gScene.models[1], mesh, &gMaterials[1].material);
  // vec3_set(gScene.models[1]->position, 0, -10.0f, 0);

  // Setup physics
  physics_world_initialize(&gPhysWorld);
  PhysicsContactGenerator* contact_gen = physics_contact_generator_allocate_simple(&gPhysWorld, 1.0f, .9f);
  physics_world_register_contact_generator(&gPhysWorld, contact_gen);

  return 0;
}

static int initialize() {
  // seed not so random
  srand((unsigned)time(0));

  // init imgui
  gui_initialize(gWindow);

  // show loading screen
  update_loading_screen("Initializing renderer...", "", 0, 0);

  int err = 0;
  printf("<-- Initializing renderer... -->\n");
  if ((err = renderer_initialize(&gRenderer))) {
    printf("Deferred renderer init failed\n");
    return err;
  }

  if ((err = initialize_assets(&update_loading_screen))) {
    printf("assets init failed\n");
    return err;
  }

  printf("<-- Initializing scene... -->\n");
  if ((err = setup_scene(false))) {
    printf("Scene init failed\n");
    return err;
  }

  printf("<-- Initialization complete -->\n");
  return 0;
}

static void destroy_entity(Entity* ent) {
  LINKED_LIST_REMOVE(&gEntities, ent);
  if (ent->force_generator) {
    physics_world_unregister_force_generator(&gPhysWorld, ent->force_generator, &ent->particle);
    free(ent->force_generator);
  }
  physics_world_remove_particle(&gPhysWorld, &ent->particle);
  scene_remove_model(&gScene, &ent->model);
  free(ent);
}

static void spawn_entity(ForceGeneratorType type) {
  Entity* ent = (Entity*)calloc(1, sizeof(Entity));
  physics_particle_initialize(&ent->particle, gScene.models[0]->position, 4, Gravity, .5f);
  model_initialize(&ent->model, gScene.models[0]->mesh, &gScene.models[0]->material);
  switch (type) {
    case FORCE_GENERATOR_TYPE_DRAG: ent->force_generator = physics_force_generator_allocate_drag(.9f, .9f); break;
    case FORCE_GENERATOR_TYPE_SPRING: ent->force_generator  = physics_force_generator_allocate_spring(Zero, 5.0f, 50.0f, 1); break;
    case FORCE_GENERATOR_TYPE_BUOYANCY: ent->force_generator  = physics_force_generator_allocate_buoyancy(1.0f, 1, -10, 350.0f); break;
    case FORCE_GENERATOR_TYPE_NONE: // intentional fall-through
    default: break;
  }
  if (physics_world_add_particle(&gPhysWorld, &ent->particle) ||
      scene_add_model(&gScene, &ent->model) ||
      (ent->force_generator && physics_world_register_force_generator(&gPhysWorld, ent->force_generator, &ent->particle))
    ) {
    destroy_entity(ent);
    return;
  }
  LINKED_LIST_APPEND(&gEntities, ent);
}

static void destroy_all_entities() {
  Entity* head = LINKED_LIST_GET_HEAD(&gEntities);
  while (head) {
    Entity* cur = head;
    head = LINKED_LIST_GET_NEXT(head);
    destroy_entity(cur);
  }
}


static void update_entities() {
  Entity* head = LINKED_LIST_GET_HEAD(&gEntities);
  while (head) {
    vec3_dup(head->model.position, head->particle.position);
    head = LINKED_LIST_GET_NEXT(head);
  }
}

static int process_input() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
      return 1;
    } else if (!gui_process_event(&event)) {
      switch (event.type) {
      case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
          SDL_SetWindowGrab(gWindow, SDL_TRUE);
        }
        break;
      case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT) {
          SDL_SetWindowGrab(gWindow, SDL_FALSE);
        } else if (event.button.button == SDL_BUTTON_RIGHT) {
          spawn_entity(FORCE_GENERATOR_TYPE_SPRING);
        }
        break;
      case SDL_MOUSEMOTION:
        if (event.motion.state & SDL_BUTTON_LMASK) {
          gScene.camera.rot[1] += event.motion.xrel / (float)WINDOW_WIDTH;
          gScene.camera.rot[0] += event.motion.yrel / (float)WINDOW_HEIGHT;
        }
        break;
      case SDL_MOUSEWHEEL:
        if (event.wheel.y == 1)  {
          gScene.camera.boom_len -= 2.f;
        } else if (event.wheel.y == -1) {
          gScene.camera.boom_len +=  2.f;
        }
        break;
       case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_c) {
          destroy_all_entities();
        }
        break;
      case SDL_QUIT:
        return 1;
      }
    }
  }
  return 0;
}

static int frame() {
  // calc dt
  float frameTime = utility_secs_since_launch();
  static float prevFrameTime = frameTime;
  float dt = frameTime - prevFrameTime;
  prevFrameTime = frameTime;

  // pump window events
  if (process_input()) {
    return 1;
  }

  // update physics
  physics_world_run(&gPhysWorld, dt);
  update_entities();

  // update camera and emitters
  scene_update(&gScene, dt);

  // Draw debug lines
  if (gRenderer.render_debug_lines) {
    if (gScene.models[0]) {
      OBB obb;
      model_get_obb(gScene.models[0], &obb);
      debug_lines_submit_obb(&obb, Green);
    }
    physics_world_debug_render(&gPhysWorld);
  }

  // render viewport
  renderer_render(&gRenderer, &gScene);

  // render gui
  gui_render(gWindow, &gRenderer, &gScene, dt);

  return 0;
}

int main(int argc, char* argv[]) {
  (void)(argc);
  (void)(argv);

  // init SDL
  SDL_Init(SDL_INIT_VIDEO);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,            8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,          8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,           8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,          8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,          16);
  SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,         32);
  SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,      8);
  SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,    8);
  SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,     8);
  SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,    8);

  // init platform window
  if(!(gWindow = SDL_CreateWindow("PBR Renderer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
    , WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL))) {
      printf("Failed to create window. Exiting.\n");
      return -1;
  }

  // init gl
  SDL_GLContext glcontext = SDL_GL_CreateContext(gWindow);
  SDL_GL_SetSwapInterval(1); // Enable vsync
  glewExperimental = 1;
  glewInit();

  // main init
  if(initialize()) {
    printf("Failed to initialize. Exiting.\n");
    return -1;
  }

  // while the window is open: enter program loop
  while (!frame()) {
    // swap
    SDL_GL_SwapWindow(gWindow);
    GL_CHECK_ERROR();
  }

  // clean up
  gui_destroy();
  SDL_GL_DeleteContext(glcontext);
  SDL_DestroyWindow(gWindow);
  SDL_Quit();
  return 0;
}
