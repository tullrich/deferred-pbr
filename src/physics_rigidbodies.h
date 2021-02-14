#pragma once
#include "physics.h"
#include "container.h"

struct PhysicsRigidBody
{
  DECLATE_INTRUSIVE_LL_MEMBERS(PhysicsRigidBody);

  // world space velocity pos
  vec3 position;

  // world space orientation
  quat orientation;

  // world space velocity
  vec3 velocity;

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
};

void physics_rigidbody_initialize(PhysicsRigidBody* rb, float mass, mat3x3 inertia_tensor);
void physics_rigidbody_set_mass(PhysicsRigidBody* rb, float mass);
float physics_rigidbody_get_mass(const PhysicsRigidBody* rb);
void physics_rigidbody_set_inertia_tensor(PhysicsRigidBody* rb, const mat3x3 tensor);
void physics_rigidbody_get_inertia_tensor(PhysicsRigidBody* rb, mat3x3 out);
void physics_rigidbody_calc_derived_data(PhysicsRigidBody* rb);

DECLARE_LINKED_LIST_TYPE(PhysicsRigidBody, PhysicsRigidBodyList);
