#include "physics_rigidbodies.h"
#include "debug_lines.h"

DEFINE_ENUM(PhysicsShapeType, physics_shape_type_strings, ENUM_PhysicsShapeType);
DEFINE_ENUM(ForceGeneratorType, force_generator_type_strings, ENUM_ForceGeneratorType);
DEFINE_ENUM(ContactGeneratorType, contact_generator_type_strings, ENUM_ContactGeneratorType);

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

  // the side lengths of the box
  vec3 extents;
};

void physics_shape_get_inertia_tensor_box(const PhysicsShape* shape, float mass, mat3x3 tensor_out) {
  const PhysicsBoxShape* box = (const PhysicsBoxShape*)shape;
  mat3x3_zero(tensor_out);
  tensor_out[0][0] = (1.0f/12.0f) * mass * (powf(box->extents[1] * 2.0f, 2.0f) + powf(box->extents[2] * 2.0f, 2.0f));
  tensor_out[1][1] = (1.0f/12.0f) * mass * (powf(box->extents[0] * 2.0f, 2.0f) + powf(box->extents[2] * 2.0f, 2.0f));
  tensor_out[2][2] = (1.0f/12.0f) * mass * (powf(box->extents[0] * 2.0f, 2.0f) + powf(box->extents[1] * 2.0f, 2.0f));
}

PhysicsShape* physics_shape_allocate_box(const vec3 sides) {
  PhysicsBoxShape* box = (PhysicsBoxShape*)calloc(1, sizeof(PhysicsBoxShape));
  box->base.type = PHYSICS_SHAPE_TYPE_BOX;
  box->base.get_inertia_tensor_vfn = physics_shape_get_inertia_tensor_box;
  vec3_dup(box->extents, sides);
  vec3_scale(box->extents, box->extents, 0.5f);
  return &box->base;
}

struct PhysicsPlaneShape
{
  PhysicsShape base;

  // the plane normal
  vec3 normal;

  // the plane constant
  float d;
};

void physics_shape_get_inertia_tensor_plane(const PhysicsShape* shape, float mass, mat3x3 tensor_out) {
  (void)(shape);
  (void)(mass);
  mat3x3_zero(tensor_out); // rotating plane not supported
}

PhysicsShape* physics_shape_allocate_plane(const vec3 normal, float d) {
  PhysicsPlaneShape* plane = (PhysicsPlaneShape*)calloc(1, sizeof(PhysicsPlaneShape));
  plane->base.type = PHYSICS_SHAPE_TYPE_PLANE;
  plane->base.get_inertia_tensor_vfn = physics_shape_get_inertia_tensor_plane;
  vec3_dup(plane->normal, normal);
  plane->d = d;
  return &plane->base;
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

struct CollisionParams
{
  PhysicsContact* next;
  int limit;
  float restitution;
  float friction;
};

int physics_collide_sphere_vs_plane(PhysicsRigidBody* a, PhysicsRigidBody* b, const CollisionParams* params) {
  const PhysicsSphereShape* sphere = (const PhysicsSphereShape*)(a->shape);
  const PhysicsPlaneShape* plane = (const PhysicsPlaneShape*)(b->shape);
  float distance = vec3_mul_inner(plane->normal, a->position) - sphere->radius - plane->d;
  if (params->limit <= 0 || distance >= 0) return 0;
  vec3 contact_point;
  vec3_scale(contact_point, plane->normal, distance + sphere->radius);
  vec3_sub(contact_point, a->position, contact_point);
  physics_contact_initialize(params->next, a, NULL, params->restitution, params->friction, contact_point, plane->normal, -distance);
  return 1;
}

int physics_collide_box_vs_plane(PhysicsRigidBody* a, PhysicsRigidBody* b, const CollisionParams* params) {
  const PhysicsBoxShape* box = (const PhysicsBoxShape*)(a->shape);
  const PhysicsPlaneShape* plane = (const PhysicsPlaneShape*)(b->shape);
  const float deltas[] = { -1.0f, 1.0f };
  int found = 0;
  for (int dx = 0; dx < 2; dx++) {
    for (int dy = 0; dy < 2; dy++) {
      for (int dz = 0; dz < 2; dz++) {
        if (found >= params->limit) return found;
        vec3 vert; vec4 offset;
        vec4_set(offset, box->extents[0] * deltas[dx], box->extents[1] * deltas[dy], box->extents[2] * deltas[dz], 1.0f);
        mat4x4_mul_vec4(vert, a->transform, offset);
        float dist = vec3_mul_inner(vert, plane->normal);
        if (dist <= plane->d) {
          vec3 contact_point;
          vec3_scale(contact_point, plane->normal, -(dist - plane->d));
          vec3_add(contact_point, contact_point, vert);
          physics_contact_initialize(params->next + found++, a, NULL, params->restitution, params->friction, contact_point, plane->normal, plane->d - dist);
        }
      }
    }
  }
  return found;
}

static float project_box_to_axis(const vec3 extents, const mat4x4 transform, const vec3 axis) {
  float out = 0.0f;
  for (int i = 0; i < 3; i++) {
    vec3 basis;
    mat4x4_get_basis_vector(basis, transform, i);
    out += fabs(vec3_mul_inner(basis, axis)) * extents[i];
  }
  return out;
}

static bool is_overlapping_on_axis(const vec3 a_to_b, const vec3 extents_a, const mat4x4 tform_a, const vec3 extents_b, const mat4x4 tform_b, const vec3 in_axis, int case_code, int* min_case, float* min_penetration) {
  if (vec3_len2(in_axis) < 0.0001f) return true;
  vec3 axis;
  vec3_norm(axis, in_axis);
  float proj_size_a = project_box_to_axis(extents_a, tform_a, axis);
  float proj_size_b = project_box_to_axis(extents_b, tform_b, axis);
  float proj_dist = fabs(vec3_mul_inner(a_to_b, axis));
  float penetration = proj_size_a + proj_size_b - proj_dist; // positive indicates overlap
  if (penetration < 0) return false;
  if (penetration < *min_penetration) {
    *min_case = case_code;
    *min_penetration = penetration;
  }
  return true;
}

static bool intersect_box_vs_box(const vec3 a_to_b, const vec3 extents_a, const mat4x4 tform_a, const vec3 extents_b, const mat4x4 tform_b, int* min_case, float* min_penetration, int* min_major_case) {
  // test 6x basis vectors (3 per cube)
  vec3 basis_a, basis_b;
  for (int i = 0; i < 3; i++) {
    mat4x4_get_basis_vector(basis_a, tform_a, i);
    mat4x4_get_basis_vector(basis_b, tform_b, i);
    if (!is_overlapping_on_axis(a_to_b, extents_a, tform_a, extents_b, tform_b, basis_a, i, min_case, min_penetration)) return false;
    if (!is_overlapping_on_axis(a_to_b, extents_a, tform_a, extents_b, tform_b, basis_b, i + 3, min_case, min_penetration)) return false;
  }

  *min_major_case = *min_case;

  // test 9x cross products of every basis vector pair
  vec3 basis_cross;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      mat4x4_get_basis_vector(basis_a, tform_a, i);
      mat4x4_get_basis_vector(basis_b, tform_b, j);
      vec3_mul_cross(basis_cross, basis_a, basis_b);
      if (!is_overlapping_on_axis(a_to_b, extents_a, tform_a, extents_b, tform_b, basis_cross, 6 + i * 3 + j, min_case, min_penetration)) return false;
    }
  }

  return true;
}

static void find_contact_from_edge_centers(const vec3 edge_center_a, const vec3 basis_a, float a_len, const vec3 edge_center_b, const vec3 basis_b, float b_len, int use_a, vec3 out_contact) {
  float sq_mag_a = vec3_len2(basis_a);
  float sq_mag_b = vec3_len2(basis_b);
  float a_dot_b = vec3_mul_inner(basis_a, basis_b);

  vec3 b_to_a;
  vec3_sub(b_to_a, edge_center_a, edge_center_b);
  float a_dot_dist = vec3_mul_inner(basis_a, b_to_a);
  float b_dot_dist = vec3_mul_inner(basis_b, b_to_a);
  float denom = sq_mag_a * sq_mag_b - a_dot_b * a_dot_b;

  // parrallel lines
  if (fabs(denom) < 0.0001f) {
    vec3_dup(out_contact, use_a ? edge_center_a : edge_center_b);
    return;
  }

  float mu_a = (a_dot_b * b_dot_dist - sq_mag_b * a_dot_dist) / denom;
  float mu_b = (sq_mag_a * b_dot_dist - a_dot_b * a_dot_dist) / denom;
  if (mu_a > a_len || mu_a < -a_len ||
      mu_b > b_len || mu_b < -b_len) {
      vec3_dup(out_contact, use_a ? edge_center_a : edge_center_b);
  } else {
    vec3 first, second;
    vec3_add_scaled(first, edge_center_a, basis_a, mu_a);
    vec3_add_scaled(second, edge_center_b, basis_b, mu_b);
    vec3_scale(first, first, 0.5f);
    vec3_scale(second, second, 0.5f);
    vec3_add(out_contact, first, second);
  }
}

int physics_collide_box_vs_box(PhysicsRigidBody* a, PhysicsRigidBody* b, const CollisionParams* params) {
  const PhysicsBoxShape* box_a = (const PhysicsBoxShape*)(a->shape);
  const PhysicsBoxShape* box_b = (const PhysicsBoxShape*)(b->shape);

  // Calculate A to B
  vec3 a_to_b;
  vec3_sub(a_to_b, b->position, a->position);

  // Perform SAT test on all 15 possible axes (6x major axis, and 9x cross axis)
  float min_penetration = FLT_MAX;
  int min_case = -1;
  int min_major_case = -1;
  bool is_intersecting = intersect_box_vs_box(a_to_b, box_a->extents, a->transform, box_b->extents, b->transform, &min_case, &min_penetration, &min_major_case);

  // Early out if no intersection was found
  if (!is_intersecting) return 0;

  if (min_case < 3) {
    // A face intersects B vert
    vec3 normal;
    mat4x4_get_basis_vector(normal, a->transform, min_case);
    if (vec3_mul_inner(normal, a_to_b) > 0.0f) {
      vec3_scale(normal, normal, -1.0f);
    }
    vec4 vert;
    vec3_dup(vert, box_b->extents);
    vert[3] = 1.0f;
    for (int i = 0; i < 3; i++) {
      vec3 axis;
      mat4x4_get_basis_vector(axis, b->transform, i);
      float dist = vec3_mul_inner(axis, normal);
      if (ALMOST_ZERO(dist)) {
        dist = -vec3_mul_inner(axis, a_to_b);
      }
      if (dist < 0) vert[i] = -vert[i];
    }
    vec4 contact_point;
    mat4x4_mul_vec4(contact_point, b->transform, vert);
    physics_contact_initialize(params->next, a, b, params->restitution, params->friction, contact_point, normal, min_penetration);
    return 1;
  }
  else if (min_case < 6) {
    // B face intersects A vert
    vec3 normal;
    mat4x4_get_basis_vector(normal, b->transform, min_case-3);
    if (vec3_mul_inner(normal, a_to_b) < 0.0f) {
      vec3_scale(normal, normal, -1.0f);
    }
    vec4 vert;
    vec3_dup(vert, box_a->extents);
    vert[3] = 1.0f;
    for (int i = 0; i < 3; i++) {
      vec3 axis;
      mat4x4_get_basis_vector(axis, a->transform, i);
      float dist = vec3_mul_inner(axis, normal);
      if (ALMOST_ZERO(dist)) {
        dist = vec3_mul_inner(axis, a_to_b);
      }
      if (dist < 0) vert[i] = -vert[i];
    }
    vec4 contact_point;
    mat4x4_mul_vec4(contact_point, a->transform, vert);
    physics_contact_initialize(params->next, b, a, params->restitution, params->friction, contact_point, normal, min_penetration);
    return 1;
  }
  else {
    // Edge to Edge contact
    int a_axis_idx = (min_case - 6) / 3;
    int b_axis_idx = (min_case - 6) % 3;
    vec3 a_axis, b_axis;
    mat4x4_get_basis_vector(a_axis, a->transform, a_axis_idx);
    mat4x4_get_basis_vector(b_axis, b->transform, b_axis_idx);
    vec3 normal;
    vec3_mul_cross(normal, a_axis, b_axis);
    if (vec3_mul_inner(normal, a_to_b) > 0) {
      vec3_scale(normal, normal, -1.0f);
    }

    // Workout the center point of each contacting edge
    vec4 edge_center_a, edge_center_b;
    edge_center_a[3] = edge_center_b[3] = 1.0f;
    for (int i = 0; i < 3; i++) {
      if (i != a_axis_idx) {
        vec3 basis_a;
        mat4x4_get_basis_vector(basis_a, a->transform, i);
        float dist = vec3_mul_inner(basis_a, normal);
        if (ALMOST_ZERO(dist)) {
          dist = -vec3_mul_inner(basis_a, a_to_b);
        }
        edge_center_a[i] = box_a->extents[i] * (dist > 0.0f ? -1.0f : 1.0f);
      } else {
        edge_center_a[i] = 0.0f;
      }
      if (i != b_axis_idx) {
        vec3 basis_b;
        mat4x4_get_basis_vector(basis_b, b->transform, i);
        float dist = vec3_mul_inner(basis_b, normal);
        if (ALMOST_ZERO(dist)) {
          dist = -vec3_mul_inner(basis_b, a_to_b);
        }
        edge_center_b[i] = box_b->extents[i] * (dist > 0.0f ? 1.0f : -1.0f);
      } else {
        edge_center_b[i] = 0.0f;
      }
    }
    vec4 w_edge_center_a, w_edge_center_b;
    mat4x4_mul_vec4(w_edge_center_a, a->transform, edge_center_a);
    mat4x4_mul_vec4(w_edge_center_b, b->transform, edge_center_b);

    // workout contact point
    vec3 contact_point;
    find_contact_from_edge_centers(
      w_edge_center_a, a_axis, box_a->extents[a_axis_idx],
      w_edge_center_b, b_axis, box_b->extents[b_axis_idx],
      min_major_case > 2,
      contact_point
    );
    physics_contact_initialize(params->next, a, b, params->restitution, params->friction, contact_point, normal, min_penetration);
    return 1;
  }
}

typedef int(*CollideShapeFN)(PhysicsRigidBody* a, PhysicsRigidBody* b, const CollisionParams* params);
CollideShapeFN gCollisionFNTable[PHYSICS_SHAPE_TYPE_MAX][PHYSICS_SHAPE_TYPE_MAX];

void physics_init_collision_fn_table() {
  gCollisionFNTable[PHYSICS_SHAPE_TYPE_SPHERE][PHYSICS_SHAPE_TYPE_BOX] = NULL;
  gCollisionFNTable[PHYSICS_SHAPE_TYPE_SPHERE][PHYSICS_SHAPE_TYPE_PLANE] = physics_collide_sphere_vs_plane;
  gCollisionFNTable[PHYSICS_SHAPE_TYPE_SPHERE][PHYSICS_SHAPE_TYPE_SPHERE] = NULL;
  gCollisionFNTable[PHYSICS_SHAPE_TYPE_BOX][PHYSICS_SHAPE_TYPE_PLANE] = physics_collide_box_vs_plane;
  gCollisionFNTable[PHYSICS_SHAPE_TYPE_BOX][PHYSICS_SHAPE_TYPE_BOX] = physics_collide_box_vs_box;
}

int physics_collide(PhysicsRigidBody* a, PhysicsRigidBody* b, const CollisionParams* params) {
  if ((int)a->shape->type > (int)b->shape->type) {
    PhysicsRigidBody* tmp = a;
    a = b; b = tmp;
  }
  CollideShapeFN collide_fn = gCollisionFNTable[a->shape->type][b->shape->type];
  if (collide_fn) {
    return collide_fn(a, b, params);
  }
  return 0;
}

static void physics_rigid_body_transform_inertia_tensor(mat3x3 iit_world, const mat3x3 iit, const mat4x4 tform) {
  mat3x3 rot_mat, trans_rot_mat;
  mat3x3_from_mat4x4(rot_mat, tform);
  mat3x3_transpose(trans_rot_mat, rot_mat);

  // https://hepweb.ucsd.edu/ph110b/110b_notes/node24.html
  mat3x3 tmp;
  mat3x3_mul(tmp, iit, trans_rot_mat);
  mat3x3_mul(iit_world, rot_mat, tmp);
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
  vec3_zero(rb->last_frame_accel);
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

void physics_rigid_body_move(PhysicsRigidBody* rb, const vec3 delta) {
  vec3_add(rb->position, rb->position, delta);
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

void physics_rigid_body_apply_impulse_at_world_point(PhysicsRigidBody* rb, const vec3 impulse, const vec3 point) {
  vec3 pt;
  vec3_sub(pt, point, rb->position); // convert to center of mass

  //  apply impulse to velocity
  vec3_add_scaled(rb->velocity, rb->velocity, impulse, rb->inverse_mass);

  // modify rotation of b
  vec3 impulse_torque, delta_rotation;
  vec3_mul_cross(impulse_torque, pt, impulse);
  mat3x3_mul_vec3(delta_rotation, rb->inverse_inertia_tensor_world, impulse_torque);
  vec3_add(rb->rotation, rb->rotation, delta_rotation);
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

// velocity of point q: v_q = a_v cross (q - p) + v;
void physics_rigid_body_get_velocity_at_world_point(const PhysicsRigidBody* rb, const vec3 world_point, vec3 out_velocity) {
  vec3 pq; // local space position
  vec3_sub(pq, world_point, rb->position);
  vec3 angular_v; // angular component of the velocity at q
  vec3_mul_cross(angular_v, rb->rotation, pq);
  vec3_add(out_velocity, angular_v, rb->velocity);
}

void physics_rigid_body_integrate(PhysicsRigidBody* rb, float dt) {
  (void)(dt);

  // calc linear acceleration
  vec3_add_scaled(rb->last_frame_accel, rb->acceleration, rb->force, rb->inverse_mass);

  // calc angular acceleration
  vec3 angular_acceleration;
  mat3x3_mul_vec3(angular_acceleration, rb->inverse_inertia_tensor_world, rb->torque);

  // integrate velocity
  vec3_add_scaled(rb->velocity, rb->velocity, rb->last_frame_accel, dt);

  // integrate angular velocity
  vec3_add_scaled(rb->rotation, rb->rotation, angular_acceleration, dt);

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

void physics_contact_initialize(PhysicsContact* contact, PhysicsRigidBody* a, PhysicsRigidBody* b, float restitution, float friction, const vec3 world_point, const vec3 normal, float penetration) {
  memset(contact, 0, sizeof(PhysicsContact));
  contact->bodies[0] = a;
  contact->bodies[1] = b;
  contact->restitution = restitution;
  contact->friction = friction;
  contact->penetration = penetration;
  vec3_dup(contact->world_point, world_point);
  vec3_dup(contact->normal, normal);
}

// velocity of point q: v_q = a_v cross (q - p) + v
// < 0 is a closing velocity
void physics_contact_get_contact_velocity(const PhysicsContact* contact, vec3 contact_vel) {
  physics_rigid_body_get_velocity_at_world_point(contact->bodies[0], contact->world_point, contact_vel);
  if (contact->bodies[1]) {
    vec3 vel_b; // initial velocity of contact point on b
    physics_rigid_body_get_velocity_at_world_point(contact->bodies[1], contact->world_point, vel_b);
    vec3_sub(contact_vel, contact_vel, vel_b);
  }
}

float physics_contact_get_separating_velocity(const PhysicsContact* contact) {
  vec3 contact_vel;
  physics_contact_get_contact_velocity(contact, contact_vel);
  return vec3_mul_inner(contact_vel, contact->normal);
}

static void make_orthonormal_basis(const vec3 x, vec3 y_out, vec3 z_out) {
  vec3 up;
  float up_len = abs(vec3_mul_inner(x, Axis_Up)) - 1.0f;
  if (ALMOST_ZERO(up_len)) {
    vec3_dup(up, Axis_Right);
  } else {
    vec3_dup(up, Axis_Up);
  }
  vec3_mul_cross(z_out, x, up);
  float len = vec3_len2(z_out);
  if (ALMOST_ZERO(len)) {
    vec3_dup(y_out, up);
  } else {
    vec3_mul_cross(y_out, z_out, x);
  }
  vec3_norm(z_out, z_out);
  vec3_norm(y_out, y_out);

}
void physics_make_contact_to_world_mat(const PhysicsContact* contact, mat3x3 mat_out) {
  vec3 y_axis, z_axis;
  make_orthonormal_basis(contact->normal, y_axis, z_axis);
  mat3x3_from_basis(mat_out, contact->normal, y_axis, z_axis);
}

static float rotation_j_per_impulse_at_point(const PhysicsRigidBody* rb, const vec3 world_point, const vec3 impulse_dir) {
  vec3 contact_pos;
  vec3_sub(contact_pos, world_point, rb->position); // q_rel = q - p
  vec3 torque_per_impulse;
  vec3_mul_cross(torque_per_impulse, contact_pos, impulse_dir); // t_per_i = (q_rel X D)   d is impulse direction
  vec3 rotation_per_impulse;
  mat3x3_mul_vec3(rotation_per_impulse, rb->inverse_inertia_tensor_world, torque_per_impulse); // v_per_i =  I_inv * t_per_i
  vec3 rotation_at_contact_per_impulse;
  vec3_mul_cross(rotation_at_contact_per_impulse, rotation_per_impulse, contact_pos); // vel_at_q = v_per_i X q_rel
  return vec3_mul_inner(rotation_at_contact_per_impulse, impulse_dir);
}

void physics_contact_get_frictionless_impulse(const PhysicsContact* contact, float dt, float v_sep, vec3 out_impulse) {
  float delta_vel_per_impulse = rotation_j_per_impulse_at_point(contact->bodies[0], contact->world_point, contact->normal);
  delta_vel_per_impulse += contact->bodies[0]->inverse_mass;
  if (contact->bodies[1]) {
    delta_vel_per_impulse += rotation_j_per_impulse_at_point(contact->bodies[1], contact->world_point, contact->normal);
    delta_vel_per_impulse += contact->bodies[1]->inverse_mass;
  }

  float restitution = contact->restitution;
  const float MIN_VELOCITY = 0.1f;
  if (fabs(v_sep) < MIN_VELOCITY)
  {
      restitution = 0.0f;
  }

  float velocity_from_accel = vec3_mul_inner(contact->bodies[0]->last_frame_accel, contact->normal) * dt;
  if (contact->bodies[1]) {
    velocity_from_accel -= vec3_mul_inner(contact->bodies[1]->last_frame_accel, contact->normal) * dt;
  }
  // float desired_delta_vel = -velocity_from_accel - (v_sep - velocity_from_accel) * (1 + restitution);
  // -velocity_from_accel - (v_sep + v_sep*restitution - velocity_from_accel - velocity_from_accel*restitution)
  // -v_sep - v_sep*restitution + velocity_from_accel*restitution)
   float desired_delta_vel = -v_sep - restitution * (v_sep - velocity_from_accel);
  vec3_scale(out_impulse, contact->normal, desired_delta_vel / delta_vel_per_impulse);
  PHYSICS_PRINT("restitution %f target velocity %f = delta velocity %f / vel_per_impulse %f = impulse: <%f, %f, %f> \n"
    , restitution, desired_delta_vel + v_sep, desired_delta_vel, delta_vel_per_impulse, FORMAT_VEC3(out_impulse));
}

void physics_contact_get_friction_impulse(const PhysicsContact* contact, float max_j, vec3 contact_vel, vec3 out_impulse) {
  vec3 tangent_vel;
  vec3_scale(tangent_vel, contact->normal, vec3_mul_inner(contact_vel, contact->normal));
  vec3_sub(tangent_vel, contact_vel, tangent_vel);

  float mag = vec3_len(tangent_vel);
  if (mag > FLT_EPSILON) {
    vec3 tangent;
    vec3_norm(tangent, tangent_vel);
    float delta_vel_per_impulse = rotation_j_per_impulse_at_point(contact->bodies[0], contact->world_point, tangent);
    delta_vel_per_impulse += contact->bodies[0]->inverse_mass;

    if (contact->bodies[1]) {
      delta_vel_per_impulse += rotation_j_per_impulse_at_point(contact->bodies[1], contact->world_point, tangent);
      delta_vel_per_impulse += contact->bodies[1]->inverse_mass;
    }

    float jt = fmin(fmax(mag / delta_vel_per_impulse, -contact->friction * max_j), contact->friction * max_j);
    // printf("contact_vel <%f, %f, %f>\nnormal <%f, %f, %f>\ntangent <%f, %f, %f> mag %f delta_vel_per_impulse %f jt %f\n"
      // , FORMAT_VEC3(contact_vel), FORMAT_VEC3(contact->normal), FORMAT_VEC3(tangent_vel), mag, delta_vel_per_impulse, jt);
    vec3_scale(out_impulse, tangent, -jt);
  }
  else {
    vec3_zero(out_impulse);
  }
}

static void physics_contact_resolve_interpenetration(PhysicsContact* contact, float dt) {
  (void)(dt);
  if (contact->penetration <= 0.0f) return;

  // split the correction between both bodies proportionally to their respective mass
  // sum_inverse_mass = (m_a * m_b) / (m_a + m_b)
  float sum_inverse_mass = contact->bodies[0]->inverse_mass;
  if (contact->bodies[1]) {
    sum_inverse_mass += contact->bodies[1]->inverse_mass;
  }

  // both bodies are immovable
  if (sum_inverse_mass <= 0) return;

  vec3 delta_p_per_mass;
  vec3_scale(delta_p_per_mass, contact->normal, contact->penetration / sum_inverse_mass);

  // apply correction to body a = delta_p * (m_b) / (m_a + m_b)
  vec3_scale(contact->delta_resolution[0], delta_p_per_mass, contact->bodies[0]->inverse_mass);
  PHYSICS_PRINT("[moving part by %f from y %f to", contact->penetration, contact->bodies[0]->position[1]);
  physics_rigid_body_move(contact->bodies[0], contact->delta_resolution[0]);
  physics_rigid_body_calc_derived_data(contact->bodies[0]);
  PHYSICS_PRINT(" %f]\n", contact->bodies[0]->position[1]);

  // apply correction to body b = -delta_p * (m_a) / (m_a + m_b)
  if (contact->bodies[1]) {
    vec3_scale(contact->delta_resolution[1], delta_p_per_mass, -contact->bodies[1]->inverse_mass); // negated due to opposite direction
    physics_rigid_body_move(contact->bodies[1], contact->delta_resolution[1]);
    physics_rigid_body_calc_derived_data(contact->bodies[1]);
  } else {
    vec3_zero(contact->delta_resolution[1]);
  }
}

static void physics_contact_resolve_velocity(PhysicsContact* contact, float dt) {
  (void)(dt);

  // the separating velocity at the contact points relative to the collision normal.
  float v_sep = physics_contact_get_separating_velocity(contact);

  // bodies are already moving apart
  if (v_sep > 0) return;

  {
    vec3 point_velocity;
    physics_rigid_body_get_velocity_at_world_point(contact->bodies[0], contact->world_point, point_velocity);
    PHYSICS_PRINT("BEFORE linear_velocity <%f, %f, %f> angular_velocity <%f, %f, %f> point_velocity <%f, %f, %f> start sep: %f\n"
      , FORMAT_VEC3(contact->bodies[0]->velocity), FORMAT_VEC3(contact->bodies[0]->rotation), FORMAT_VEC3(point_velocity)
      , v_sep);
  }

  vec3 impulse;
  physics_contact_get_frictionless_impulse(contact, dt, v_sep, impulse);
  physics_rigid_body_apply_impulse_at_world_point(contact->bodies[0], impulse, contact->world_point);
  if (contact->bodies[1]) {
    vec3_scale(impulse, impulse, -1.0f);
    physics_rigid_body_apply_impulse_at_world_point(contact->bodies[1], impulse, contact->world_point);
  }

  vec3 tangent_impulse = { 0.0f };
  if (!ALMOST_ZERO(contact->friction)) {
    vec3 contact_vel;
    physics_contact_get_contact_velocity(contact, contact_vel);
    float max_j = vec3_len(impulse);
    physics_contact_get_friction_impulse(contact, max_j, contact_vel, tangent_impulse);
    physics_rigid_body_apply_impulse_at_world_point(contact->bodies[0], tangent_impulse, contact->world_point);
    if (contact->bodies[1]) {
      vec3_scale(tangent_impulse, tangent_impulse, -1.0f);
      physics_rigid_body_apply_impulse_at_world_point(contact->bodies[1], tangent_impulse, contact->world_point);
    }
  }

  {
    vec3 point_velocity;
    physics_rigid_body_get_velocity_at_world_point(contact->bodies[0], contact->world_point, point_velocity);
    PHYSICS_PRINT("AFTER linear_velocity <%f, %f, %f> angular_velocity <%f, %f, %f> point_velocity <%f, %f, %f> new sep: %f\n"
      , FORMAT_VEC3(contact->bodies[0]->velocity), FORMAT_VEC3(contact->bodies[0]->rotation), FORMAT_VEC3(point_velocity)
      , physics_contact_get_separating_velocity(contact));
    vec3 debug_impulse;
    vec3_add(debug_impulse, impulse, contact->world_point);
    debug_lines_submit(contact->world_point, debug_impulse, Red);
    vec3_add(debug_impulse, tangent_impulse, contact->world_point);
    debug_lines_submit(contact->world_point, debug_impulse, Yellow);
  }
}

void physics_contact_resolve(PhysicsContact* contact, float dt) {
  physics_contact_resolve_interpenetration(contact, dt);
  physics_contact_resolve_velocity(contact, dt);
}

void physics_contact_debug_render(const PhysicsContact* contact) {
  const float box_size = .05f;
  const float normal_len = 1.0f;
  debug_lines_submit_cube(contact->world_point, box_size, Red);
  vec3 end;
  vec3_add_scaled(end, contact->world_point, contact->normal, normal_len);
  debug_lines_submit(contact->world_point, end, Green);
}

struct PhysicsPlaneContactGenerator {
  PhysicsContactGenerator base;

  // the body that represents this plane
  PhysicsRigidBody* body;

  // the plane shape to collide with
  PhysicsShape* shape;

  // the contact restitution
  float restitution;

  // the contact friction coefficient
  float friction;
};

static int physics_plane_contact_generator_add_contacts(const PhysicsContactGenerator* generator, const PhysicsWorld* world, PhysicsContact* next, int limit) {
  const PhysicsPlaneContactGenerator* plane = (const PhysicsPlaneContactGenerator*)generator;
  int found = 0;
  PhysicsRigidBody* body = LINKED_LIST_GET_HEAD(&world->rigid_bodies);
  while (body && found < limit) {
    CollisionParams params;
    params.next = next + found;
    params.limit = limit - found;
    params.restitution = plane->restitution;
    params.friction = plane->friction;
    found += physics_collide(plane->body, body, &params);
    body = LINKED_LIST_GET_NEXT(body);
  }
  return found;
}

PhysicsContactGenerator* physics_contact_generator_allocate_plane(const vec3 normal, float d, float restitution, float friction) {
  PhysicsPlaneContactGenerator* plane = (PhysicsPlaneContactGenerator*)calloc(1, sizeof(PhysicsPlaneContactGenerator));
  plane->base.type = CONTACT_GENERATOR_TYPE_PLANE;
  plane->base.add_contacts_vfn = physics_plane_contact_generator_add_contacts;
  plane->shape = physics_shape_allocate_plane(normal, d);
  plane->body = (PhysicsRigidBody*)calloc(1, sizeof(PhysicsRigidBody));
  physics_rigid_body_initialize(plane->body, Zero, Quat_Identity, Zero, 0, plane->shape);
  plane->restitution = restitution;
  plane->friction = friction;
  return &plane->base;
}

struct PhysicsBruteForceContactGenerator {
  PhysicsContactGenerator base;

  // the contact restitution
  float restitution;

  // the contact friction coefficient
  float friction;
};

static int physics_brute_force_contact_generator_add_contacts(const PhysicsContactGenerator* generator, const PhysicsWorld* world, PhysicsContact* next, int limit) {
  const PhysicsBruteForceContactGenerator* gen = (const PhysicsBruteForceContactGenerator*)generator;
  int found = 0;
  PhysicsRigidBody* body_a = LINKED_LIST_GET_HEAD(&world->rigid_bodies);
  while (body_a && found < limit) {
    PhysicsRigidBody* body_b = LINKED_LIST_GET_NEXT(body_a);
    while (body_b && found < limit) {
      CollisionParams params;
      params.next = next + found;
      params.limit = limit - found;
      params.restitution = gen->restitution;
      params.friction = gen->friction;
      found += physics_collide(body_a, body_b, &params);
      body_b = LINKED_LIST_GET_NEXT(body_b);
    }
    body_a = LINKED_LIST_GET_NEXT(body_a);
  }
  return found;
}

PhysicsContactGenerator* physics_contact_generator_allocate_brute_force(float restitution, float friction) {
  PhysicsBruteForceContactGenerator* gen = (PhysicsBruteForceContactGenerator*)calloc(1, sizeof(PhysicsBruteForceContactGenerator));
  gen->base.type = CONTACT_GENERATOR_TYPE_BRUTE_FORCE;
  gen->base.add_contacts_vfn = physics_brute_force_contact_generator_add_contacts;
  gen->restitution = restitution;
  gen->friction = friction;
  return &gen->base;
}

void physics_contact_generator_allocate_set_restitution_friction(PhysicsContactGenerator* generator, float restitution, float friction) {
  switch(generator->type) {
    case CONTACT_GENERATOR_TYPE_PLANE: {
      PhysicsPlaneContactGenerator* plane = (PhysicsPlaneContactGenerator*)generator;
      plane->restitution = restitution;
      plane->friction = friction;
      break;
    }
    case CONTACT_GENERATOR_TYPE_BRUTE_FORCE: {
      PhysicsBruteForceContactGenerator* gen = (PhysicsBruteForceContactGenerator*)generator;
      gen->restitution = restitution;
      gen->friction = friction;
      break;
    }
    default: break;
  }
}

int physics_contact_generator_add_contacts(const PhysicsContactGenerator* generator, const PhysicsWorld* world, PhysicsContact* next, int limit) {
  assert(generator->add_contacts_vfn);
  return generator->add_contacts_vfn(generator, world, next, limit);
}

void physics_world_initialize(PhysicsWorld* world) {
  memset(world, 0, sizeof(PhysicsWorld));
  physics_init_collision_fn_table();
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
  PHYSICS_PRINT("Got %i contacts\n", count);
  for (int i = 0; i < count; i++) {
    physics_contact_resolve_interpenetration(&contacts[i], dt);
    int max_idx = i;
    vec3* delta_resolution = contacts[max_idx].delta_resolution;
    for (int j = 0; j < count; j++) {
      if (contacts[j].bodies[0] == contacts[max_idx].bodies[0]) {
        contacts[j].penetration -= vec3_mul_inner(delta_resolution[0], contacts[j].normal);
      } else if (contacts[j].bodies[0] == contacts[max_idx].bodies[1]) {
        contacts[j].penetration -= vec3_mul_inner(delta_resolution[1], contacts[j].normal);
      }
      if (contacts[j].bodies[1]) {
        if (contacts[j].bodies[1] == contacts[max_idx].bodies[0]) {
          contacts[j].penetration += vec3_mul_inner(delta_resolution[0], contacts[j].normal);
        } else if (contacts[j].bodies[1] == contacts[max_idx].bodies[1]) {
          contacts[j].penetration += vec3_mul_inner(delta_resolution[1], contacts[j].normal);
        }
     }
   }
  }
  int iterations = 0;
  PHYSICS_PRINT("===START RESOLVE===\n");
  while (iterations < count * 10)
  {
    float min_sep = -COLLISION_FUDGE;
    int index = count;
    for (int i = 0; i < count; i++)
    {
        float v_sep = physics_contact_get_separating_velocity(&contacts[i]);
        if (v_sep < min_sep)
        {
          min_sep = v_sep;
          index = i;
        }
    }
    if (index == count) break;

    PHYSICS_PRINT("RESOLVING CONTACT %i\n", index);
    physics_contact_resolve_velocity(&contacts[index], dt);

    iterations++;
  }
  PHYSICS_PRINT("===END RESOLVE (%i iterations)===\n", iterations);
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
  world->contacts_count = physics_world_generate_contacts(world);

  // Resolve contacts
  if (world->contacts_count > 0) {
    if (world->contacts_count >= MAX_CONTACTS) {
      printf("WARNING: Max contacts(%i) reached! Some contacts may be missed.\n", MAX_CONTACTS);
    }
    physics_world_resolve_contacts(world, world->contacts, world->contacts_count, dt);
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
   for (int i = 0; i < world->contacts_count; i++) {
     physics_contact_debug_render(&world->contacts[i]);
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
