#include "common.h"
#include "assets.h"
#include "gui.h"
#include "scene.h"
#include "renderer.h"
#include "physics_particles.h"
#include "physics_rigidbodies.h"
#include "container.h"

#define MAX_DELTA_TIME (1/60.0f)

static SDL_Window* gWindow;
static Renderer gRenderer;
static Scene gScene;
static PhysicsWorld gPhysWorld;
static EditorState gEditor;
static PhysicsContactGenerator* gContactGenerators[2];

struct Entity {
  DECLATE_INTRUSIVE_LL_MEMBERS(Entity);
  Model model;
  PhysicsRigidBody body;
  PhysicsForceGenerator* generator;
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
  gScene.camera.exposure = 1.0f;

  // Setup ambient light
  vec3_dup(gScene.ambient_color, White);
  gScene.ambient_intensity = 1.0f;

  // Setup skybox
  gScene.skybox = &gSkyboxes[0].skybox;

  // Setup main directional light
  gScene.light = (Light*)malloc(sizeof(Light));
  vec3 lightPos = { 0.0f, 10.0f, 10.0f };
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
    model_initialize(gScene.models[0], &gMeshes[1].mesh, &gMaterials[0].material);
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
  Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
  mesh_make_quad(mesh, 200, 200, 6);
  gScene.models[1] = (Model*)malloc(sizeof(Model));
  model_initialize(gScene.models[1], mesh, &gMaterials[1].material);
  vec3_set(gScene.models[1]->position, 0, -3.0f, 0);

  // Setup physics
  physics_world_initialize(&gPhysWorld);
  gContactGenerators[0] = physics_contact_generator_allocate_plane(Axis_Up, -3.0f, gEditor.restitution, gEditor.friction);
  gContactGenerators[1] = physics_contact_generator_allocate_brute_force(gEditor.restitution, gEditor.friction);
  for (unsigned i = 0; i < STATIC_ELEMENT_COUNT(gContactGenerators); i++)
    physics_world_register_contact_generator(&gPhysWorld, gContactGenerators[i]);

  return 0;
}

static int initialize() {
  // seed not so random
  srand((unsigned)time(0));

  // init imgui
  gui_initialize(gWindow);

  // show loading screen
  update_loading_screen("Initializing renderer...", "", 0, 0);

  // setup editor state
  gEditor.show_floor = 1;
  gEditor.time_scale = 1.0f;
  gEditor.mass = 100.0f;
  gEditor.friction = 0.0f;
  gEditor.restitution = 0.2f;
  gEditor.linear_damping = 0.9f;
  gEditor.angular_damping = 0.9f;
  vec3_dup(gEditor.gravity, Gravity);

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
  physics_world_unregister_force_generator(&gPhysWorld, ent->generator, &ent->body);
  physics_world_remove_rigid_body(&gPhysWorld, &ent->body);
  scene_remove_model(&gScene, &ent->model);
  free(ent);
}

static void spawn_entity(ForceGeneratorType type) {
  Entity* ent = (Entity*)calloc(1, sizeof(Entity));

  quat rot;
  quat_from_euler(rot, gScene.models[0]->rot);
  const PhysicsShape* shape = physics_shape_allocate_box(Vec_One);
  // const PhysicsShape* shape = physics_shape_allocate_sphere(1.0f);
  physics_rigid_body_initialize(&ent->body, gScene.models[0]->position, rot, gEditor.gravity, gEditor.mass, shape);
  ent->body.linear_damping = gEditor.linear_damping;
  ent->body.angular_damping = gEditor.angular_damping;
  physics_world_add_rigid_body(&gPhysWorld, &ent->body);

  switch (type) {
    case FORCE_GENERATOR_TYPE_SPRING: {
      vec3 attach_point;
      vec3_set(attach_point, 0, 0.5f, 0);
      ent->generator = physics_force_generator_allocate_spring(Zero, attach_point, 5.0f, 50.0f, 1);
      break;
    }
    case FORCE_GENERATOR_TYPE_NONE: break;
    default: break;
  }

  if (ent->generator) {
    physics_world_register_force_generator(&gPhysWorld, ent->generator, &ent->body);
  }

  // vec3 torque;
  // vec3_set(torque, 0, 5, 5);
  // physics_rigid_body_apply_torque(&ent->body, torque);
  //
  // vec3 force;
  // vec3_set(force, 0, 5, 0);
  // physics_rigid_body_apply_force(&ent->body, force);

  model_initialize(&ent->model, gScene.models[0]->mesh, &gScene.models[0]->material);
  if (scene_add_model(&gScene, &ent->model)) {
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
    vec3_dup(head->model.position, head->body.position);
    quat_to_euler(head->model.rot, head->body.orientation);
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
          spawn_entity(FORCE_GENERATOR_TYPE_NONE);
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
        switch (event.key.keysym.sym) {
          case SDLK_c: destroy_all_entities(); break;
          case SDLK_b: gEditor.step_frame = 1; break;
          case SDLK_SPACE: gEditor.paused = !gEditor.paused; break;
          default: break;
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
  float dt = std::min(frameTime - prevFrameTime, MAX_DELTA_TIME) * gEditor.time_scale;
  prevFrameTime = frameTime;

  // pump window events
  if (process_input()) {
    return 1;
  }

  gScene.models[1]->hidden = !gEditor.show_floor;

  // update physics
  if (!gEditor.paused || gEditor.step_frame) {
    debug_lines_clear();

    for (unsigned i = 0; i < STATIC_ELEMENT_COUNT(gContactGenerators); i++)
      physics_contact_generator_allocate_set_restitution_friction(gContactGenerators[i], gEditor.restitution, gEditor.friction);

    physics_world_run(&gPhysWorld, dt);

    if (gEditor.pause_on_collision) {
      for (int i = 0; i < gPhysWorld.contacts_count; i++) {
        if (gPhysWorld.contacts[i].bodies[0] && gPhysWorld.contacts[i].bodies[1]) {
          gEditor.paused = 1;
        }
      }
      // if (gPhysWorld.contacts_count > 0) {
      //   gEditor.paused = 1;
      // }
    }

    // Draw debug lines
    if (gRenderer.render_debug_lines) {
      if (gScene.models[0] && !gScene.models[0]->hidden) {
        OBB obb;
        model_get_obb(gScene.models[0], &obb);
        debug_lines_submit_obb(&obb, Green);
      }
      physics_world_debug_render(&gPhysWorld);
    }
  }
  update_entities();

  // update camera and emitters
  scene_update(&gScene, dt);

  // render viewport
  renderer_render(&gRenderer, &gScene);

  // render gui
  gui_render(gWindow, &gRenderer, &gScene, dt, &gEditor);

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
