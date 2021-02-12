#pragma once
#include "common.h"
#include "container.h"

#define MAX_PARTICLES 256
#define MAX_FORCE_GENERATORS 256
#define MAX_CONTACTS 256
#define MAX_RESOLVE_ITERATIONS 10

struct PhysicsWorld;

extern const vec3 Gravity;

struct PhysicsParticle
{
  // world space velocity pos
  vec3 position;

  // world space velocity
  vec3 velocity;

  // world space acceleration
  vec3 acceleration;

  // the sum of forces affecting this particle
  vec3 force;

  // linear drag
  float damping;

  // 1/mass of an object. 0 represents immovable.
  float inverse_mass;

  // the world that simulates this particle
  PhysicsWorld* world;
};

void physics_particle_initialize(PhysicsParticle* p, const vec3 pos, float mass, const vec3 acceleration, float damping);
void physics_particle_integrate(PhysicsParticle* p, float dt);
void physics_particle_debug_render(const PhysicsParticle* p);
void physics_particle_set_mass(PhysicsParticle* p, float mass);
float physics_particle_get_mass(const PhysicsParticle* p);
void physics_particle_apply_force(PhysicsParticle* p, const vec3 force);
void physics_particle_apply_impulse(PhysicsParticle* p, const vec3 impulse);

#define ENUM_ForceGeneratorType(D)							  	\
  D(FORCE_GENERATOR_TYPE_NONE, 		  "None")			    \
  D(FORCE_GENERATOR_TYPE_DRAG, 		  "Drag")			    \
  D(FORCE_GENERATOR_TYPE_BUOYANCY,  "Buoyancy")     \
  D(FORCE_GENERATOR_TYPE_SPRING, 		"Spring")

DECLARE_ENUM(ForceGeneratorType, force_generator_type_strings, ENUM_ForceGeneratorType);

struct PhysicsForceGenerator;
typedef void(*UpdateForceVFN)(const PhysicsForceGenerator* generator, PhysicsParticle* p, float dt);
typedef void(*DebugRenderVFN)(const PhysicsForceGenerator* generator, const PhysicsParticle* p);

struct PhysicsForceGenerator
{
  // the force generator type
  ForceGeneratorType type;

  // the virtual update_force function
  UpdateForceVFN update_force_vfn;

  // the virtual debug_render function
  DebugRenderVFN debug_render_vfn;
};

void physics_force_generator_update_force(const PhysicsForceGenerator* generator, PhysicsParticle* p, float dt);
void physics_force_generator_debug_render(const PhysicsForceGenerator* generator, const PhysicsParticle* p);
PhysicsForceGenerator* physics_force_generator_allocate_drag(float k1, float k2);
PhysicsForceGenerator* physics_force_generator_allocate_spring(const vec3 anchor, float length, float stiffness, int is_bungie);
PhysicsForceGenerator* physics_force_generator_allocate_buoyancy(float max_depth, float volume, float water_height, float liquid_density);

struct PhysicsParticleContact
{
  // the two participating particles
  PhysicsParticle* particles[2];

  // the coefficient of restitution representing the interactions between the materials of both particles
  float restitution;

  // the world space contact normal
  vec3 normal;
};

void physics_particle_contact_initialize(PhysicsParticleContact* contact, PhysicsParticle* a, PhysicsParticle* b, float restitution, const vec3 contact_normal);
float physics_particle_contact_get_separating_velocity(const PhysicsParticleContact* contact);
void physics_particle_contact_resolve(PhysicsParticleContact* contact, float dt);

struct PhysicsForceGeneratorRegistration
{
  const PhysicsForceGenerator* generator;
  PhysicsParticle* particle;
};

struct PhysicsContactGenerator;
typedef int(*AddContactVFN)(const PhysicsContactGenerator* generator, PhysicsParticleContact* next, int limit);

struct PhysicsContactGenerator
{
  DECLATE_INTRUSIVE_LL_MEMBERS(PhysicsContactGenerator);

  // the virtual add_contact function
  AddContactVFN add_contact_vfn;
};

int physics_contact_generator_add_contact(const PhysicsContactGenerator* generator, PhysicsParticleContact* next, int limit);
PhysicsContactGenerator* physics_contact_generator_allocate_simple(const PhysicsWorld* world, float particle_radius, float contact_restitution);

DECLARE_LINKED_LIST_TYPE(PhysicsContactGenerator, PhysicsContactGeneratorList);

struct PhysicsWorld
{
  // particles to integrate
  PhysicsParticle* particles[MAX_PARTICLES];

  // count of particles
  int particle_count;

  // registered forcegenerator + particle pairs
  PhysicsForceGeneratorRegistration force_generators[MAX_FORCE_GENERATORS];

  // count of particles
  int force_generator_count;

  // found particle contacts
  PhysicsParticleContact contacts[MAX_CONTACTS];

  // The linked list of contact generators`
  PhysicsContactGeneratorList contact_generators;
};

void physics_world_initialize(PhysicsWorld* world);
void physics_world_run(PhysicsWorld* world, float dt);
void physics_world_debug_render(const PhysicsWorld* world);

int physics_world_add_particle(PhysicsWorld* world, PhysicsParticle* p);
int physics_world_remove_particle(PhysicsWorld* world, PhysicsParticle* p);

int physics_world_register_force_generator(PhysicsWorld* world, const PhysicsForceGenerator* generator, PhysicsParticle* p);
int physics_world_unregister_force_generator(PhysicsWorld* world, const PhysicsForceGenerator* generator, PhysicsParticle* p);
void physics_world_unregister_all_force_generators(PhysicsWorld* world, PhysicsParticle* p);

void physics_world_register_contact_generator(PhysicsWorld* world, PhysicsContactGenerator* cgen);
void physics_world_unregister_contact_generators(PhysicsWorld* world, PhysicsContactGenerator* cgen);
