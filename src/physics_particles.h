#pragma once
#include "physics.h"
#include "container.h"

#define MAX_PARTICLES 256
#define MAX_FORCE_GENERATORS 256
#define MAX_CONTACTS 512
#define MAX_RESOLVE_ITERATIONS (1024 * 10)
#define COLLISION_FUDGE 0.01f

struct PhysicsParticleWorld;

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
  PhysicsParticleWorld* world;
};

void physics_particle_initialize(PhysicsParticle* p, const vec3 pos, float mass, const vec3 acceleration, float damping);
void physics_particle_integrate(PhysicsParticle* p, float dt);
void physics_particle_debug_render(const PhysicsParticle* p);
void physics_particle_set_mass(PhysicsParticle* p, float mass);
float physics_particle_get_mass(const PhysicsParticle* p);
void physics_particle_apply_force(PhysicsParticle* p, const vec3 force);
void physics_particle_apply_impulse(PhysicsParticle* p, const vec3 impulse);
void physics_particle_move(PhysicsParticle* p, const vec3 delta_pos);

#define ENUM_ParticleForceGeneratorType(D)							  	  \
  D(PARTICLE_FORCE_GENERATOR_TYPE_NONE, 		  "None")			    \
  D(PARTICLE_FORCE_GENERATOR_TYPE_DRAG, 		  "Drag")			    \
  D(PARTICLE_FORCE_GENERATOR_TYPE_BUOYANCY,  "Buoyancy")      \
  D(PARTICLE_FORCE_GENERATOR_TYPE_SPRING, 		"Spring")

DECLARE_ENUM(ParticleForceGeneratorType, force_generator_type_strings, ENUM_ParticleForceGeneratorType);

struct PhysicsParticleForceGenerator;
typedef void(*UpdateForceVFN)(const PhysicsParticleForceGenerator* generator, PhysicsParticle* p, float dt);
typedef void(*DebugRenderVFN)(const PhysicsParticleForceGenerator* generator, const PhysicsParticle* p);

struct PhysicsParticleForceGenerator
{
  // the force generator type
  ParticleForceGeneratorType type;

  // the virtual update_force function
  UpdateForceVFN update_force_vfn;

  // the virtual debug_render function
  DebugRenderVFN debug_render_vfn;
};

void physics_particle_force_generator_update_force(const PhysicsParticleForceGenerator* generator, PhysicsParticle* p, float dt);
void physics_particle_force_generator_debug_render(const PhysicsParticleForceGenerator* generator, const PhysicsParticle* p);
PhysicsParticleForceGenerator* physics_particle_force_generator_allocate_drag(float k1, float k2);
PhysicsParticleForceGenerator* physics_particle_force_generator_allocate_spring(const vec3 anchor, float length, float stiffness, int is_bungie);
PhysicsParticleForceGenerator* physics_particle_force_generator_allocate_buoyancy(float max_depth, float volume, float water_height, float liquid_density);

struct PhysicsParticleContact
{
  // the two participating particles
  PhysicsParticle* particles[2];

  // the penetration resolution amount
  vec3 delta_resolution[2];

  // the coefficient of restitution representing the interactions between the materials of both particles
  float restitution;

  // the world space contact normal
  vec3 normal;

  // the penetration of the two bodies. Positive means overlapping.
  float penetration;
};

void physics_particle_contact_initialize(PhysicsParticleContact* contact, PhysicsParticle* a, PhysicsParticle* b, float restitution, const vec3 contact_normal, float penetration);
float physics_particle_contact_get_separating_velocity(const PhysicsParticleContact* contact);
void physics_particle_contact_resolve(PhysicsParticleContact* contact, float dt);

struct PhysicsParticleForceGeneratorRegistration
{
  const PhysicsParticleForceGenerator* generator;
  PhysicsParticle* particle;
};

struct PhysicsParticleContactGenerator;
typedef int(*AddContactVFN)(const PhysicsParticleContactGenerator* generator, PhysicsParticleContact* next, int limit);

struct PhysicsParticleContactGenerator
{
  DECLATE_INTRUSIVE_LL_MEMBERS(PhysicsParticleContactGenerator);

  // the virtual add_contact function
  AddContactVFN add_contact_vfn;
};

int physics_particle_contact_generator_add_contact(const PhysicsParticleContactGenerator* generator, PhysicsParticleContact* next, int limit);
PhysicsParticleContactGenerator* physics_particle_contact_generator_allocate_simple(const PhysicsParticleWorld* world, float particle_radius, float contact_restitution);
PhysicsParticleContactGenerator* physics_particle_contact_generator_allocate_plane(const PhysicsParticleWorld* world, const vec3 normal, float d, float particle_radius, float contact_restitution);

DECLARE_LINKED_LIST_TYPE(PhysicsParticleContactGenerator, PhysicsParticleContactGeneratorList);

struct PhysicsParticleWorld
{
  // particles to integrate
  PhysicsParticle* particles[MAX_PARTICLES];

  // count of particles
  int particle_count;

  // registered forcegenerator + particle pairs
  PhysicsParticleForceGeneratorRegistration force_generators[MAX_FORCE_GENERATORS];

  // count of particles
  int force_generator_count;

  // found particle contacts
  PhysicsParticleContact contacts[MAX_CONTACTS];

  // The linked list of contact generators`
  PhysicsParticleContactGeneratorList contact_generators;
};

void physics_particle_world_initialize(PhysicsParticleWorld* world);
void physics_particle_world_run(PhysicsParticleWorld* world, float dt);
void physics_particle_world_debug_render(const PhysicsParticleWorld* world);

int physics_particle_world_add_particle(PhysicsParticleWorld* world, PhysicsParticle* p);
int physics_particle_world_remove_particle(PhysicsParticleWorld* world, PhysicsParticle* p);

int physics_particle_world_register_force_generator(PhysicsParticleWorld* world, const PhysicsParticleForceGenerator* generator, PhysicsParticle* p);
int physics_particle_world_unregister_force_generator(PhysicsParticleWorld* world, const PhysicsParticleForceGenerator* generator, PhysicsParticle* p);
void physics_particle_world_unregister_all_force_generators(PhysicsParticleWorld* world, PhysicsParticle* p);

void physics_particle_world_register_contact_generator(PhysicsParticleWorld* world, PhysicsParticleContactGenerator* cgen);
void physics_particle_world_unregister_contact_generators(PhysicsParticleWorld* world, PhysicsParticleContactGenerator* cgen);
