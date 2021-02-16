#pragma once
#include "physics.h"
#include "container.h"

class PhysicsWorld;

#define ENUM_PhysicsShapeType(D)					         \
  D(PHYSICS_SHAPE_TYPE_NONE, 		    "None")			   \
  D(PHYSICS_SHAPE_TYPE_PLANE, 		  "Plane")	     \
  D(PHYSICS_SHAPE_TYPE_SPHERE, 		  "Sphere")	     \
  D(PHYSICS_SHAPE_TYPE_BOX, 		    "Box")

DECLARE_ENUM(PhysicsShapeType, physics_shape_type_strings, ENUM_PhysicsShapeType);

struct PhysicsShape;
typedef void(*PhysicsShapeGetInertiaTensorVFN)(const PhysicsShape* shape, float mass, mat3x3 tensor_out);

struct PhysicsShape
{
  // the type of this physics shap
  PhysicsShapeType type;

  // the virtual get_inertia_tensor function to use
  PhysicsShapeGetInertiaTensorVFN get_inertia_tensor_vfn;
};

PhysicsShape* physics_shape_allocate_sphere(float radius, int is_hollow = 0);
PhysicsShape* physics_shape_allocate_box(const vec3 extents);
void physics_shape_free(PhysicsShape* shape);
void physics_shape_get_inertia_tensor(const PhysicsShape* shape, float mass, mat3x3 tensor_out);

struct PhysicsRigidBody
{
  DECLATE_INTRUSIVE_LL_MEMBERS(PhysicsRigidBody);

  // the collision geometry of this body
  const PhysicsShape* shape;

  // world space velocity pos
  vec3 position;

  // world space orientation
  quat orientation;

  // world space velocity
  vec3 velocity;

  // constant world space linear acceleration (used for Gravity)
  vec3 acceleration;

  // world space angular velocity
  vec3 rotation;

  // 1/mass of an object. 0 represents immovable.
  float inverse_mass;

  // local space inverse inertial tensor used to compute angular acceleration from applied torque
  mat3x3 inverse_inertia_tensor;

  // world space transform updated once per frame
  mat4x4 transform;

  // world space inverse inertial tensor computed once per frame
  mat3x3 inverse_inertia_tensor_world;

  // the sum of linear forces affecting this body. Cleared every frame.
  vec3 force;

  // the sum of angular forces affecting this body. Cleared every frame.
  vec3 torque;

  // linear drag force in m/s
  float linear_damping;

  // angular drag force rad/s
  float angular_damping;
};

void physics_rigid_body_initialize(PhysicsRigidBody* rb, const vec3 position, const quat orient, const vec3 acceleration, float mass, const PhysicsShape* shape);
void physics_rigid_body_set_mass(PhysicsRigidBody* rb, float mass);
float physics_rigid_body_get_mass(const PhysicsRigidBody* rb);
void physics_rigid_body_get_inertia_tensor(PhysicsRigidBody* rb, mat3x3 out);
void physics_rigid_body_calc_derived_data(PhysicsRigidBody* rb);
void physics_rigid_body_apply_force(PhysicsRigidBody* rb, const vec3 force);
void physics_rigid_body_apply_force_at_local_point(PhysicsRigidBody* rb, const vec3 force, const vec3 local_point);
void physics_rigid_body_apply_force_at_world_point(PhysicsRigidBody* rb, const vec3 force, const vec3 point);
void physics_rigid_body_apply_torque(PhysicsRigidBody* rb, const vec3 torque);
void physics_rigid_body_get_world_point(const PhysicsRigidBody* rb, vec3 out, const vec3 local_point);
void physics_rigid_body_integrate(PhysicsRigidBody* rb, float dt);
void physics_rigid_body_debug_render(const PhysicsRigidBody* rb);

#define ENUM_ForceGeneratorType(D)					        \
  D(FORCE_GENERATOR_TYPE_NONE, 		  "None")			    \
  D(FORCE_GENERATOR_TYPE_DRAG, 		  "Drag")			    \
  D(FORCE_GENERATOR_TYPE_BUOYANCY,  "Buoyancy")     \
  D(FORCE_GENERATOR_TYPE_SPRING, 		"Spring")

DECLARE_ENUM(ForceGeneratorType, force_generator_type_strings, ENUM_ForceGeneratorType);

struct PhysicsForceGenerator;
typedef void(*UpdateForceVFN)(const PhysicsForceGenerator* generator, PhysicsRigidBody* rb, float dt);
typedef void(*DebugRenderVFN)(const PhysicsForceGenerator* generator, const PhysicsRigidBody* rb);

struct PhysicsForceGenerator
{
  // the force generator implementation type
  ForceGeneratorType type;

  // the virtual update_force function
  UpdateForceVFN update_force_vfn;

  // the virtual debug_render function
  DebugRenderVFN debug_render_vfn;
};

PhysicsForceGenerator* physics_force_generator_allocate_spring(const vec3 world_anchor, const vec3 local_attach_point, float length, float stiffness, int is_bungie);
void physics_force_generator_update_force(const PhysicsForceGenerator* generator, PhysicsRigidBody* rb, float dt);
void physics_force_generator_debug_render(const PhysicsForceGenerator* generator, const PhysicsRigidBody* rb);

struct PhysicsForceGeneratorRegistration
{
  DECLATE_INTRUSIVE_LL_MEMBERS(PhysicsForceGeneratorRegistration);

  // the registered force generator
  const PhysicsForceGenerator* generator;

  // the body to apply force/torque
  PhysicsRigidBody* body;
};

struct PhysicsContact
{
  // the two participating particles
  PhysicsRigidBody* bodies[2];

  // the penetration resolution amount
  vec3 delta_resolution[2];

  // the coefficient of restitution representing the interactions between the materials of both particles
  float restitution;

  // the contact point in world space
  vec3 world_point;

  // the world space contact normal
  vec3 normal;

  // the penetration of the two bodies. Positive means overlapping.
  float penetration;
};

void physics_contact_initialize(PhysicsContact* contact, PhysicsRigidBody* a, PhysicsRigidBody* b, float restitution, const vec3 world_point, const vec3 normal, float penetration);

struct PhysicsContactGenerator;
typedef int(*AddContactsVFN)(const PhysicsContactGenerator* generator, const PhysicsWorld* world, PhysicsContact* next, int limit);

struct PhysicsContactGenerator
{
  DECLATE_INTRUSIVE_LL_MEMBERS(PhysicsContactGenerator);

  // the virtual add_contact function
  AddContactsVFN add_contacts_vfn;
};

int physics_contact_generator_add_contacts(const PhysicsContactGenerator* generator, const PhysicsWorld* world, PhysicsContact* next, int limit);
PhysicsContactGenerator* physics_contact_generator_allocate_plane(const vec3 normal, float d, float contact_restitution);

DECLARE_LINKED_LIST_TYPE(PhysicsRigidBody, PhysicsRigidBodyList);
DECLARE_LINKED_LIST_TYPE(PhysicsForceGeneratorRegistration, PhysicsForceGeneratorRegistrationList);
DECLARE_LINKED_LIST_TYPE(PhysicsContactGenerator, PhysicsContactGeneratorList);

struct PhysicsWorld {
  // The linked list of rigid bodies
  PhysicsRigidBodyList rigid_bodies;

  // The linked list of force generators registrations
  PhysicsForceGeneratorRegistrationList force_generator_regs;

  // the linked list of contact generators
  PhysicsContactGeneratorList contact_generators;

  // found particle contacts
  PhysicsContact contacts[MAX_CONTACTS];

  // the count of used contacts this frame
  int contacts_count;
};

void physics_world_initialize(PhysicsWorld* world);
void physics_world_run(PhysicsWorld* world, float dt);
void physics_world_debug_render(const PhysicsWorld* world);

void physics_world_add_rigid_body(PhysicsWorld* world, PhysicsRigidBody* rb);
void physics_world_remove_rigid_body(PhysicsWorld* world, PhysicsRigidBody* rb);

void physics_world_register_force_generator(PhysicsWorld* world, const PhysicsForceGenerator* generator, PhysicsRigidBody* rb);
void physics_world_unregister_force_generator(PhysicsWorld* world, const PhysicsForceGenerator* generator, const PhysicsRigidBody* rb);

void physics_world_register_contact_generator(PhysicsWorld* world, PhysicsContactGenerator* cgen);
void physics_world_unregister_contact_generators(PhysicsWorld* world, PhysicsContactGenerator* cgen);
