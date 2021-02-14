#include "physics_rigidbodies.h"

static void physics_rigidbody_transform_inertia_tensor(mat3x3 iit_world, const mat3x3 iit, const mat4x4 tform) {
  mat3x3 rot_mat;
  mat3x3_from_mat4x4(rot_mat, tform);
  mat3x3_mul(iit_world, rot_mat, iit);

  printf("inverse_inertia_tensor\n");
  mat3x3_printf(iit);

  printf("rot_mat\n");
  mat3x3_printf(rot_mat);

  printf("inverse_inertia_tensor_world\n");
  mat3x3_printf(iit_world);
}

void physics_rigidbody_initialize(PhysicsRigidBody* rb, float mass, mat3x3 inertia_tensor) {
  memset(rb, 0, sizeof(PhysicsRigidBody));
  quat_identity(rb->orientation);
  physics_rigidbody_set_mass(rb, mass);
  physics_rigidbody_set_inertia_tensor(rb, inertia_tensor);
  physics_rigidbody_calc_derived_data(rb);
}

void physics_rigidbody_set_mass(PhysicsRigidBody* rb, float mass) {
  if (mass <= 0.0f) {
    rb->inverse_mass = FLT_MAX;
  } else {
    rb->inverse_mass = 1.0f / mass;
  }
}

float physics_rigidbody_get_mass(const PhysicsRigidBody* rb) {
  if (rb->inverse_mass == FLT_MAX) {
    return 0.0f;
  } else {
    return 1.0f / rb->inverse_mass;
  }
}

void physics_rigidbody_set_inertia_tensor(PhysicsRigidBody* rb, const mat3x3 tensor) {
  mat3x3_inverse(rb->inverse_inertia_tensor, tensor);
}

void physics_rigidbody_get_inertia_tensor(PhysicsRigidBody* rb, mat3x3 out) {
  mat3x3_inverse(out, rb->inverse_inertia_tensor);
}

void physics_rigidbody_calc_derived_data(PhysicsRigidBody* rb) {
  mat4x4_make_transform(rb->transform, rb->orientation, rb->position);
  physics_rigidbody_transform_inertia_tensor(rb->inverse_inertia_tensor_world, rb->inverse_inertia_tensor, rb->transform);
}
