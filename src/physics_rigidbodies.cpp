#include "physics_rigidbodies.h"
#include "debug_lines.h"

DEFINE_ENUM(PhysicsShapeType, physics_shape_type_strings, ENUM_PhysicsShapeType);
DEFINE_ENUM(ForceGeneratorType, force_generator_type_strings, ENUM_ForceGeneratorType);

struct PhysicsSphereShape
{
  PhysicsShape base;

  // the sphere radius
  float radius;

  // if set all mass is distributed on the sphere's surface
  int is_hollow;
};

void physics_shape_get_inertia_tensor_sphere(const PhysicsShape* shape, float mass, mat3x3 tensor_out) {
  const PhysicsSphereShape* sphere = (const PhysicsSphereShape*)shape;
  mat3x3_zero(tensor_out);
  if (sphere->is_hollow) {
    tensor_out[0][0] = tensor_out[1][1] = tensor_out[2][2] = (2.0f/3.0f) * mass * powf(sphere->radius, 2.0f);
  } else {
    tensor_out[0][0] = tensor_out[1][1] = tensor_out[2][2] = (2.0f/5.0f) * mass * powf(sphere->radius, 2.0f);
  }
}

PhysicsShape* physics_shape_allocate_sphere(float radius, int is_hollow /* = 0 */) {
  PhysicsSphereShape* sphere = (PhysicsSphereShape*)calloc(1, sizeof(PhysicsSphereShape));
  sphere->base.type = PHYSICS_SHAPE_TYPE_SPHERE;
  sphere->base.get_inertia_tensor_vfn = physics_shape_get_inertia_tensor_sphere;
  sphere->radius = radius;
  sphere->is_hollow = is_hollow;
  return &sphere->base;
}

struct PhysicsBoxShape
{
  PhysicsShape base;

  // the side lengths of the box (not halfed)
  vec3 extents;
};

void physics_shape_get_inertia_tensor_box(const PhysicsShape* shape, float mass, mat3x3 tensor_out) {
  const PhysicsBoxShape* box = (const PhysicsBoxShape*)shape;
  mat3x3_zero(tensor_out);
  tensor_out[0][0] = (1.0f/12.0f) * mass * (powf(box->extents[1], 2.0f) + powf(box->extents[2], 2.0f));
  tensor_out[1][1] = (1.0f/12.0f) * mass * (powf(box->extents[0], 2.0f) + powf(box->extents[2], 2.0f));
  tensor_out[2][2] = (1.0f/12.0f) * mass * (powf(box->extents[0], 2.0f) + powf(box->extents[1], 2.0f));
}

PhysicsShape* physics_shape_allocate_box(const vec3 extents) {
  PhysicsBoxShape* box = (PhysicsBoxShape*)calloc(1, sizeof(PhysicsBoxShape));
  box->base.type = PHYSICS_SHAPE_TYPE_BOX;
  box->base.get_inertia_tensor_vfn = physics_shape_get_inertia_tensor_box;
  vec3_dup(box->extents, extents);
  return &box->base;
}

void physics_shape_free(PhysicsShape* shape) {
  shape->type = PHYSICS_SHAPE_TYPE_NONE;
  shape->get_inertia_tensor_vfn = NULL;
  free(shape);
}

void physics_shape_get_inertia_tensor(const PhysicsShape* shape, float mass, mat3x3 tensor_out) {
  assert(shape->get_inertia_tensor_vfn);
  shape->get_inertia_tensor_vfn(shape, mass, tensor_out);
}

static void physics_rigid_body_transform_inertia_tensor(mat3x3 iit_world, const mat3x3 iit, const mat4x4 tform) {
  mat3x3 rot_mat;
  mat3x3_from_mat4x4(rot_mat, tform);
  mat3x3_mul(iit_world, rot_mat, iit);
}

void physics_rigid_body_initialize(PhysicsRigidBody* rb, const vec3 position, const quat orient, const vec3 acceleration, float mass, const PhysicsShape* shape) {
  memset(rb, 0, sizeof(PhysicsRigidBody));
  rb->shape = shape;
  vec3_dup(rb->position, position);
  quat_dup(rb->orientation, orient);
  vec3_dup(rb->acceleration, acceleration);
  rb->linear_damping = rb->angular_damping = 1.0f;
  physics_rigid_body_set_mass(rb, mass);
  physics_rigid_body_calc_derived_data(rb);
}

void physics_rigid_body_set_mass(PhysicsRigidBody* rb, float mass) {
  if (mass <= 0.0f) {
    rb->inverse_mass = FLT_MAX;
  } else {
    rb->inverse_mass = 1.0f / mass;
  }

  // update inertia tensor
  mat3x3 tensor;
  physics_shape_get_inertia_tensor(rb->shape, mass, tensor);
  mat3x3_inverse(rb->inverse_inertia_tensor, tensor);
}

float physics_rigid_body_get_mass(const PhysicsRigidBody* rb) {
  if (rb->inverse_mass == FLT_MAX) {
    return 0.0f;
  } else {
    return 1.0f / rb->inverse_mass;
  }
}

void physics_rigid_body_get_inertia_tensor(PhysicsRigidBody* rb, mat3x3 out) {
  mat3x3_inverse(out, rb->inverse_inertia_tensor);
}

void physics_rigid_body_calc_derived_data(PhysicsRigidBody* rb) {
  quat_norm(rb->orientation, rb->orientation);
  mat4x4_make_transform(rb->transform, rb->orientation, rb->position);
  physics_rigid_body_transform_inertia_tensor(rb->inverse_inertia_tensor_world, rb->inverse_inertia_tensor, rb->transform);
}

void physics_rigid_body_apply_force(PhysicsRigidBody* rb, const vec3 force) {
  vec3_add(rb->force, rb->force, force);
}

void physics_rigid_body_apply_force_at_local_point(PhysicsRigidBody* rb, const vec3 force, const vec3 local_point) {
  vec3 world_point;
  physics_rigid_body_get_world_point(rb, world_point, local_point);
  physics_rigid_body_apply_force_at_world_point(rb, force, world_point);
}

void physics_rigid_body_apply_force_at_world_point(PhysicsRigidBody* rb, const vec3 force, const vec3 point) {
  vec3 pt;
  vec3_sub(pt, point, rb->position); // convert to center of mass
  physics_rigid_body_apply_force(rb, force);

  vec3 torque;
  vec3_mul_cross(torque, pt, force);
  physics_rigid_body_apply_torque(rb, torque);
}


void physics_rigid_body_apply_torque(PhysicsRigidBody* rb, const vec3 torque) {
  vec3_add(rb->torque, rb->torque, torque);
}

void physics_rigid_body_get_world_point(const PhysicsRigidBody* rb, vec3 out, const vec3 local_point) {
  vec4 in, out4;
  vec3_dup(in, local_point); in[3] = 1.0f;
  mat4x4_mul_vec4(out4, rb->transform, in);
  vec3_dup(out, out4);
}

void physics_rigid_body_integrate(PhysicsRigidBody* rb, float dt) {
  (void)(dt);

  // calc linear acceleration
  vec3 acceleration;
  vec3_add_scaled(acceleration, rb->acceleration, rb->force, rb->inverse_mass);

  // calc angular acceleration
  vec3 angular_acceleration;
  mat3x3_mul_vec3(angular_acceleration, rb->inverse_inertia_tensor_world, rb->torque);

  // integrate velocity
  vec3_add_scaled(rb->velocity, rb->velocity, acceleration, dt);

  // integrate angular velocity
  vec3_add_scaled(rb->rotation, rb->rotation, angular_acceleration, dt);
  // printf("rotation <%f, %f, %f>\n", FORMAT_VEC3(rb->rotation));

  // apply drag
  vec3_scale(rb->velocity, rb->velocity, powf(rb->linear_damping, dt));
  vec3_scale(rb->rotation, rb->rotation, powf(rb->angular_damping, dt));

  // integrate position
  vec3_add_scaled(rb->position, rb->position, rb->velocity, dt);

  // integrate orientation
  quat_add_scaled_vec3(rb->orientation, rb->orientation, rb->rotation, dt);

  // recalc world space matrices
  physics_rigid_body_calc_derived_data(rb);

  // clear forces
  vec3_zero(rb->force);
  vec3_zero(rb->torque);
}

void physics_rigid_body_debug_render(const PhysicsRigidBody* rb) {
  (void)(rb);
}

struct PhysicsSpringForceGenerator
{
  PhysicsForceGenerator base;
  // the world space position this spring is anchored at
  vec3 world_anchor;
  // the local space attachment point on the rigid body
  vec3 local_attach_point;
  // the rest length of this spring
  float length;
  // the stiffness (linear force multiplier) of this spring
  float stiffness;
  // if set, this spring only exerts a pull force
  int is_bungie;
};

static void physics_spring_force_generate_update_force(const PhysicsForceGenerator* generator, PhysicsRigidBody* rb, float dt) {
  (void)(dt);
  const PhysicsSpringForceGenerator* spring = (const PhysicsSpringForceGenerator*)generator;
  vec3 world_attach_point;
  physics_rigid_body_get_world_point(rb, world_attach_point, spring->local_attach_point);
  vec3 force;
  vec3_sub(force, world_attach_point, spring->world_anchor);
  float d = vec3_len(force);
  if (spring->is_bungie && d <= spring->length) return;
  if (d > 0.0f) {
    float k = -spring->stiffness * (d - spring->length);
    vec3_norm(force, force);
    vec3_scale(force, force, k);
    physics_rigid_body_apply_force_at_world_point(rb, force, world_attach_point);
  }
}

static void physics_spring_force_generate_debug_render(const PhysicsForceGenerator* generator, const PhysicsRigidBody* rb) {
  const PhysicsSpringForceGenerator* spring = (const PhysicsSpringForceGenerator*)generator;
  vec3 world_attach_point;
  physics_rigid_body_get_world_point(rb, world_attach_point, spring->local_attach_point);
  debug_lines_submit(spring->world_anchor, world_attach_point, Red);
}

PhysicsForceGenerator* physics_force_generator_allocate_spring(const vec3 world_anchor, const vec3 local_attach_point, float length, float stiffness, int is_bungie) {
  PhysicsSpringForceGenerator* spring = (PhysicsSpringForceGenerator*)malloc(sizeof(PhysicsSpringForceGenerator));
  spring->base.type = FORCE_GENERATOR_TYPE_SPRING;
  spring->base.update_force_vfn = physics_spring_force_generate_update_force;
  spring->base.debug_render_vfn = physics_spring_force_generate_debug_render;
  vec3_dup(spring->world_anchor, world_anchor);
  vec3_dup(spring->local_attach_point, local_attach_point);
  spring->length = length;
  spring->stiffness = stiffness;
  spring->is_bungie = is_bungie;
  return &spring->base;
}

void physics_force_generator_update_force(const PhysicsForceGenerator* generator, PhysicsRigidBody* rb, float dt) {
  assert(generator->update_force_vfn);
  generator->update_force_vfn(generator, rb, dt);
}

void physics_force_generator_debug_render(const PhysicsForceGenerator* generator, const PhysicsRigidBody* rb) {
  if (generator->debug_render_vfn) {
    generator->debug_render_vfn(generator, rb);
  }
}

void physics_contact_initialize(PhysicsContact* contact, PhysicsRigidBody* a, PhysicsRigidBody* b, float restitution, const vec3 world_point, const vec3 normal, float penetration) {
    memset(contact, 0, sizeof(PhysicsContact));
    contact->bodies[0] = a;
    contact->bodies[1] = b;
    contact->restitution = restitution;
    contact->penetration = penetration;
    vec3_dup(contact->world_point, world_point);
    vec3_dup(contact->normal, normal);
}


int physics_contact_generator_add_contacts(const PhysicsContactGenerator* generator, const PhysicsWorld* world, PhysicsContact* next, int limit) {
  assert(generator->add_contacts_vfn);
  return generator->add_contacts_vfn(generator, world, next, limit);
}

struct PhysicsPlaneContactGenerator {
  PhysicsContactGenerator base;

  // the plane normal
  vec3 normal;

  // the plane constant
  float d;

  // the contact restitution
  float contact_restitution;
};

int physics_plane_contact_generator_add_contacts(const PhysicsContactGenerator* generator, const PhysicsWorld* world, PhysicsContact* next, int limit) {
  const PhysicsPlaneContactGenerator* plane = (const PhysicsPlaneContactGenerator*)generator;
  int found = 0;
  PhysicsRigidBody* body = LINKED_LIST_GET_HEAD(&world->rigid_bodies);
  while (body && found < limit) {
    (void)(plane);
    (void)(next);
    // float penetration =  plane->d - (vec3_mul_inner(plane->normal, body->position) - plane->particle_radius);
    // if ((penetration + COLLISION_FUDGE) > FLT_EPSILON) {
    //   PhysicsContact* contact = &next[found++];
    //   physics_contact_initialize(contact, body, NULL, plane->contact_restitution, Zero, plane->normal, penetration);
    // }
    body = LINKED_LIST_GET_NEXT(body);
  }
  return found;
}

PhysicsContactGenerator* physics_contact_generator_allocate_plane(const vec3 normal, float d, float contact_restitution) {
  PhysicsPlaneContactGenerator* plane = (PhysicsPlaneContactGenerator*)calloc(1, sizeof(PhysicsPlaneContactGenerator));
  plane->base.add_contacts_vfn = physics_plane_contact_generator_add_contacts;
  vec3_dup(plane->normal, normal);
  plane->d = d;
  plane->contact_restitution = contact_restitution;
  return &plane->base;
}

void physics_world_initialize(PhysicsWorld* world) {
  memset(world, 0, sizeof(PhysicsWorld));
}

static int physics_world_generate_contacts(PhysicsWorld* world) {
  int found = 0;
  const PhysicsContactGenerator* head = LINKED_LIST_GET_HEAD(&world->contact_generators);
  while (head && found < MAX_CONTACTS) {
    found += physics_contact_generator_add_contacts(head, world, world->contacts + found, MAX_CONTACTS - found);
    head = LINKED_LIST_GET_NEXT(head);
  }
  return found;
}

static void physics_world_resolve_contacts(PhysicsWorld* world, PhysicsContact* contacts, int count, float dt) {
  (void)(world);
  (void)(contacts);
  (void)(count);
  (void)(dt);
}

void physics_world_run(PhysicsWorld* world, float dt) {
  // Apply forces
  PhysicsForceGeneratorRegistration* reg = LINKED_LIST_GET_HEAD(&world->force_generator_regs);
  while (reg) {
    physics_force_generator_update_force(reg->generator, reg->body, dt);
    reg = LINKED_LIST_GET_NEXT(reg);
  }

  // Advance bodies
  PhysicsRigidBody* body = LINKED_LIST_GET_HEAD(&world->rigid_bodies);
  while (body) {
    physics_rigid_body_integrate(body, dt);
    body = LINKED_LIST_GET_NEXT(body);
  }

  // Find contacts
  int found = physics_world_generate_contacts(world);

  // Resolve contacts
  if (found > 0) {
    if (found >= MAX_CONTACTS) {
      printf("WARNING: Max contacts(%i) reached! Some contacts may be missed.\n", MAX_CONTACTS);
    }
    physics_world_resolve_contacts(world, world->contacts, found, dt);
  }
}

void physics_world_debug_render(const PhysicsWorld* world) {
  PhysicsForceGeneratorRegistration* reg = LINKED_LIST_GET_HEAD(&world->force_generator_regs);
  while (reg) {
    physics_force_generator_debug_render(reg->generator, reg->body);
    reg = LINKED_LIST_GET_NEXT(reg);
  }
  PhysicsRigidBody* body = LINKED_LIST_GET_HEAD(&world->rigid_bodies);
  while (body) {
    physics_rigid_body_debug_render(body);
    body = LINKED_LIST_GET_NEXT(body);
  }
}

void physics_world_add_rigid_body(PhysicsWorld* world, PhysicsRigidBody* rb) {
  LINKED_LIST_APPEND(&world->rigid_bodies, rb);
}

void physics_world_remove_rigid_body(PhysicsWorld* world, PhysicsRigidBody* rb) {
  LINKED_LIST_REMOVE(&world->rigid_bodies, rb);
}

void physics_world_register_force_generator(PhysicsWorld* world, const PhysicsForceGenerator* generator, PhysicsRigidBody* rb) {
  PhysicsForceGeneratorRegistration* reg = (PhysicsForceGeneratorRegistration*)calloc(1, sizeof(PhysicsForceGeneratorRegistration));
  reg->generator = generator;
  reg->body = rb;
  LINKED_LIST_APPEND(&world->force_generator_regs, reg);
}

void physics_world_unregister_force_generator(PhysicsWorld* world, const PhysicsForceGenerator* generator, const PhysicsRigidBody* rb) {
  PhysicsForceGeneratorRegistration* head = LINKED_LIST_GET_HEAD(&world->force_generator_regs);
  while (head && !(head->generator == generator && head->body == rb)) {
    head = LINKED_LIST_GET_NEXT(head);
  }
  if (head) {
    LINKED_LIST_REMOVE(&world->force_generator_regs, head);
    free(head);
  }
}

void physics_world_register_contact_generator(PhysicsWorld* world, PhysicsContactGenerator* cgen) {
  LINKED_LIST_APPEND(&world->contact_generators, cgen);
}

void physics_world_unregister_contact_generators(PhysicsWorld* world, PhysicsContactGenerator* cgen) {
  LINKED_LIST_REMOVE(&world->contact_generators, cgen);
}
