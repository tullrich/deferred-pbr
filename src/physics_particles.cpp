#include "physics_particles.h"
#include "debug_lines.h"

DEFINE_ENUM(ParticleForceGeneratorType, particle_force_generator_type_strings, ENUM_ParticleForceGeneratorType);

struct PhysicsDragForceGenerator
{
  PhysicsParticleForceGenerator base;
  // the velocity drag coefficient
  float k1;
  // the velocity squared drag coefficient
  float k2;
};

// Models drags as force = -norm(v)(k1 * |v| + k2 * |v|^2)
static void physics_drag_force_generate_update_force(const PhysicsParticleForceGenerator* generator, PhysicsParticle* p, float dt) {
  (void)(dt);
  vec3 force;
  const PhysicsDragForceGenerator* drag = (const PhysicsDragForceGenerator*)generator;
  vec3_dup(force, p->velocity);
  float dragCoeff = vec3_len(force);
  dragCoeff = drag->k1 * dragCoeff + drag->k2 * dragCoeff * dragCoeff;
  if (dragCoeff > 0.0f) {
    vec3_norm(force, force);
    vec3_scale(force, force, -dragCoeff);
    physics_particle_apply_force(p, force);
  }
}

struct PhysicsSpringForceGenerator
{
  PhysicsParticleForceGenerator base;
  // the world space position this spring is anchored at
  vec3 anchor;
  // the rest length of this spring
  float length;
  // the stiffness (linear force multiplier) of this spring
  float stiffness;
  // if set, this spring only exerts a pull force
  int is_bungie;
};

// Implement (dragless) spring force based on Hooke's law
static void physics_spring_force_generate_update_force(const PhysicsParticleForceGenerator* generator, PhysicsParticle* p, float dt) {
  (void)(dt);
  vec3 force;
  const PhysicsSpringForceGenerator* spring = (const PhysicsSpringForceGenerator*)generator;
  vec3_sub(force, p->position, spring->anchor);
  float d = vec3_len(force);
  if (spring->is_bungie && d <= spring->length) return;
  if (d > 0.0f) {
    float k = -spring->stiffness * (d - spring->length);
    vec3_norm(force, force);
    vec3_scale(force, force, k);
    physics_particle_apply_force(p, force);
  }
}

static void physics_spring_force_generate_debug_render(const PhysicsParticleForceGenerator* generator, const PhysicsParticle* p) {
  const PhysicsSpringForceGenerator* spring = (const PhysicsSpringForceGenerator*)generator;
  debug_lines_submit(spring->anchor, p->position, Red);
}

struct BuoyancyForceGenerator
{
  PhysicsParticleForceGenerator base;
  // the maximum depth in world space afterwhich the buoyancy force no longer increases
  float max_depth;
  // the volume of water displaced by the body
  float volume;
  // the water height plane in world space
  float water_height;
  // the liquid density in kg per cubic meter. (set to 1000 for water)
  float liquid_density;
};

// buoyancy based on the spring equation
static void physics_buoyancy_force_generate_update_force(const PhysicsParticleForceGenerator* generator, PhysicsParticle* p, float dt) {
  (void)(dt);
  const BuoyancyForceGenerator* buoyancy = (const BuoyancyForceGenerator*)generator;
  float d = std::min((buoyancy->water_height + buoyancy->max_depth - p->position[1]) / (2.0f * buoyancy->max_depth), 1.0f);
  if (d > 0.0f) {
    float magnitude = buoyancy->liquid_density * buoyancy->volume * d;
    vec3 force;
    vec3_scale(force, Axis_Y, magnitude);
    physics_particle_apply_force(p, force);
  }
}

static void physics_buoyancy_force_generate_debug_render(const PhysicsParticleForceGenerator* generator, const PhysicsParticle* p) {
  const BuoyancyForceGenerator* buoyancy = (const BuoyancyForceGenerator*)generator;
  vec3 mid, max, min;
  vec3_set(mid, p->position[0], buoyancy->water_height, p->position[2]);
  vec3_set(max, p->position[0], buoyancy->water_height + buoyancy->max_depth, p->position[2]);
  vec3_set(min, p->position[0], buoyancy->water_height - buoyancy->max_depth, p->position[2]);
  debug_lines_submit(max, min, Red);
  const float horizontal_len = 4.0f;
  const vec3* horizontals[3] = { &mid, &max, &min };
  for (int i = 0; i < 3; i++) {
    vec3 start, end;
    vec3_add_scaled(start, *horizontals[i], Axis_X, horizontal_len / 2.0f);
    vec3_add_scaled(end, *horizontals[i], Axis_X, -horizontal_len / 2.0f);
    debug_lines_submit(start, end, Blue);
  }
}

void physics_particle_initialize(PhysicsParticle* p, const vec3 pos, float mass, const vec3 acceleration, float damping) {
    memset(p, 0, sizeof(PhysicsParticle));
    vec3_dup(p->position, pos);
    vec3_dup(p->acceleration, acceleration);
    p->damping = damping;
    physics_particle_set_mass(p, mass);
}

void physics_particle_integrate(PhysicsParticle* p, float dt) {
  assert(dt > 0.0f);

  // skip zero mass
  if (p->inverse_mass <= 0.0f) return;

  // integrate position
  PHYSICS_PRINT("before pos_y %f vel_y %f -> ", p->position[1], p->velocity[1]);
  vec3_add_scaled(p->position, p->position, p->velocity, dt);

  // integrate velocity
  vec3 acceleration;
  vec3_add_scaled(acceleration, p->acceleration, p->force, p->inverse_mass);
  vec3_add_scaled(p->velocity, p->velocity, acceleration, dt);

  // apply damping
  vec3_scale(p->velocity, p->velocity, powf(p->damping, dt));
  PHYSICS_PRINT("after pos_y %f vel_y %f\n", p->position[1], p->velocity[1]);

  // clear forces
  vec3_zero(p->force);
}

void physics_particle_debug_render(const PhysicsParticle* p) {
  (void)(p);
}

void physics_particle_set_mass(PhysicsParticle* p, float mass) {
  if (mass <= 0.0f) {
    p->inverse_mass = FLT_MAX;
  } else {
    p->inverse_mass = 1.0f / mass;
  }
}

float physics_particle_get_mass(const PhysicsParticle* p) {
  if (p->inverse_mass == FLT_MAX) {
    return 0.0f;
  } else {
    return 1.0f / p->inverse_mass;
  }
}

void physics_particle_apply_force(PhysicsParticle* p, const vec3 force) {
  vec3_add(p->force, p->force, force);
}

void physics_particle_apply_impulse(PhysicsParticle* p, const vec3 impulse) {
  vec3_add(p->velocity, p->velocity, impulse);
}

void physics_particle_move(PhysicsParticle* p, const vec3 delta_pos) {
  vec3_add(p->position, p->position, delta_pos);
}

PhysicsParticleForceGenerator* physics_particle_force_generator_allocate_drag(float k1, float k2) {
  PhysicsDragForceGenerator* drag = (PhysicsDragForceGenerator*)calloc(1, sizeof(PhysicsDragForceGenerator));
  drag->base.type = PARTICLE_FORCE_GENERATOR_TYPE_DRAG;
  drag->base.update_force_vfn = physics_drag_force_generate_update_force;
  drag->k1 = k1;
  drag->k2 = k2;
  return &drag->base;
}

PhysicsParticleForceGenerator* physics_particle_force_generator_allocate_spring(const vec3 anchor, float length, float stiffness, int is_bungie) {
  PhysicsSpringForceGenerator* spring = (PhysicsSpringForceGenerator*)calloc(1, sizeof(PhysicsSpringForceGenerator));
  spring->base.type = PARTICLE_FORCE_GENERATOR_TYPE_SPRING;
  spring->base.update_force_vfn = physics_spring_force_generate_update_force;
  spring->base.debug_render_vfn = physics_spring_force_generate_debug_render;
  vec3_dup(spring->anchor, anchor);
  spring->length = length;
  spring->stiffness = stiffness;
  spring->is_bungie = is_bungie;
  return &spring->base;
}

PhysicsParticleForceGenerator* physics_particle_force_generator_allocate_buoyancy(float max_depth, float volume, float water_height, float liquid_density) {
  BuoyancyForceGenerator* buoyancy = (BuoyancyForceGenerator*)calloc(1, sizeof(BuoyancyForceGenerator));
  buoyancy->base.type = PARTICLE_FORCE_GENERATOR_TYPE_BUOYANCY;
  buoyancy->base.update_force_vfn = physics_buoyancy_force_generate_update_force;
  buoyancy->base.debug_render_vfn = physics_buoyancy_force_generate_debug_render;
  buoyancy->max_depth = max_depth;
  buoyancy->volume = volume;
  buoyancy->water_height = water_height;
  buoyancy->liquid_density = liquid_density;
  return &buoyancy->base;
}

void physics_particle_force_generator_update_force(const PhysicsParticleForceGenerator* generator, PhysicsParticle* p, float dt) {
  assert(generator->update_force_vfn);
  generator->update_force_vfn(generator, p, dt);
}

void physics_particle_force_generator_debug_render(const PhysicsParticleForceGenerator* generator, const PhysicsParticle* p) {
  if (generator->debug_render_vfn)
    generator->debug_render_vfn(generator, p);
}

void physics_particle_contact_initialize(PhysicsParticleContact* contact, PhysicsParticle* a, PhysicsParticle* b, float restitution, const vec3 normal, float penetration) {
  memset(contact, 0, sizeof(PhysicsParticleContact));
  contact->particles[0] = a;
  contact->particles[1] = b;
  contact->restitution = restitution;
  contact->penetration = penetration;
  vec3_dup(contact->normal, normal);
}
  // calculate separating velocity Vs = (Va - Vb) dot N
float physics_particle_contact_get_separating_velocity(const PhysicsParticleContact* contact) {
  vec3 velocity;
  vec3_dup(velocity, contact->particles[0]->velocity);
  if (contact->particles[1]) {
    vec3_sub(velocity, velocity, contact->particles[1]->velocity);
  }
  return vec3_mul_inner(velocity, contact->normal);
}

static void physics_particle_contact_resolve_interpenetration(PhysicsParticleContact* contact, float dt) {
  (void)(dt);
  if (contact->penetration <= 0.0f) return;

  // split the correction between both bodies proportionally to their respective mass
  // sum_inverse_mass = (m_a * m_b) / (m_a + m_b)
  float sum_inverse_mass = contact->particles[0]->inverse_mass;
  if (contact->particles[1]) {
    sum_inverse_mass += contact->particles[1]->inverse_mass;
  }

  // both bodies are immovable
  if (sum_inverse_mass <= 0) return;

  vec3 delta_p_per_mass;
  vec3_scale(delta_p_per_mass, contact->normal, contact->penetration / sum_inverse_mass);

  // apply correction to body a = delta_p * (m_b) / (m_a + m_b)
  vec3_scale(contact->delta_resolution[0], delta_p_per_mass, contact->particles[0]->inverse_mass);
  PHYSICS_PRINT("[moving part by %f from y %f to", contact->penetration, contact->particles[0]->position[1]);
  physics_particle_move(contact->particles[0], contact->delta_resolution[0]);
  PHYSICS_PRINT(" %f] ", contact->particles[0]->position[1]);

  // apply correction to body b = -delta_p * (m_a) / (m_a + m_b)
  if (contact->particles[1]) {
    vec3_scale(contact->delta_resolution[1], delta_p_per_mass, -contact->particles[1]->inverse_mass); // negated due to opposite direction
    physics_particle_move(contact->particles[1], contact->delta_resolution[1]);
  } else {
    vec3_zero(contact->delta_resolution[1]);
  }
}

static void physics_particle_contact_resolve_velocity(PhysicsParticleContact* contact, float dt) {
  // separating velocity will be negative if the bodies are closing, otherwise positive if they are separating
  float sep_velocity = physics_particle_contact_get_separating_velocity(contact);

  // no velocity change needed if the bodies are already separating
  if (sep_velocity > 0) return;

  // apply coefficient of restitution
  float target_velocity = -sep_velocity * contact->restitution;

  // identify resting contacts
  vec3 accCausedVelocity;
  vec3_dup(accCausedVelocity, contact->particles[0]->acceleration);
  if (contact->particles[1]) vec3_sub(accCausedVelocity, accCausedVelocity, contact->particles[1]->acceleration);
  float accCausedSepVelocity = vec3_mul_inner(accCausedVelocity, contact->normal) * dt;
  if (accCausedSepVelocity < 0) {
    PHYSICS_PRINT("[resting contact target %f accCaused %f] ", target_velocity, contact->restitution * accCausedSepVelocity);
    target_velocity += contact->restitution * accCausedSepVelocity * 2;
    if (target_velocity < 0) target_velocity = 0.0f;
  }

  // split the target_velocity between both bodies proportionally to their respective mass
  float delta_velocity = target_velocity - sep_velocity;
  float sum_inverse_mass = contact->particles[0]->inverse_mass;
  if (contact->particles[1]) {
    sum_inverse_mass += contact->particles[1]->inverse_mass;
  }

  // both bodies are immovable
  if (sum_inverse_mass <= 0) return;

  vec3 impulse_per_mass;
  vec3_scale(impulse_per_mass, contact->normal, delta_velocity / sum_inverse_mass);

  // apply impulse to body a = delta_v * (m_b) / (m_a + m_b)
  vec3 impulse_a;
  vec3_scale(impulse_a, impulse_per_mass, contact->particles[0]->inverse_mass);
  physics_particle_apply_impulse(contact->particles[0], impulse_a);

  // apply impulse to body b = -delta_v * (m_a) / (m_a + m_b)
  if (contact->particles[1]) {
    vec3 impulse_b;
    vec3_scale(impulse_b, impulse_per_mass, -contact->particles[1]->inverse_mass); // negated due to opposite direction
    physics_particle_apply_impulse(contact->particles[1], impulse_b);
  }
}

void physics_particle_contact_resolve(PhysicsParticleContact* contact, float dt) {
  physics_particle_contact_resolve_velocity(contact, dt);
  physics_particle_contact_resolve_interpenetration(contact, dt);
}

int physics_particle_contact_generator_add_contacts(const PhysicsParticleContactGenerator* generator, PhysicsParticleContact* next, int limit) {
  assert(generator->add_contacts_vfn);
  return generator->add_contacts_vfn(generator, next, limit);
}

struct PhysicsSimpleContactGenerator {
  PhysicsParticleContactGenerator base;
  const PhysicsParticleWorld* world;
  float particle_radius;
  float contact_restitution;
};

static int physics_simple_contact_generator_add_contacts(const PhysicsParticleContactGenerator* generator, PhysicsParticleContact* next, int limit) {
  const PhysicsSimpleContactGenerator* simple = (const PhysicsSimpleContactGenerator*)generator;
  float radius2 = powf(simple->particle_radius * 2.0f, 2.0f);
  const PhysicsParticleWorld* world = simple->world;
  PhysicsParticle* const * particles = world->particles;
  int particle_count = world->particle_count;
  int found = 0;
  for (int i = 0; i < particle_count; i++) {
    for (int j = i + 1; j < particle_count; j++) {
      PhysicsParticle* a = particles[i];
      PhysicsParticle* b = particles[j];
      vec3 bToA;
      vec3_sub(bToA, a->position, b->position);
      float distance = vec3_len2(bToA);
      if (distance < (radius2 + COLLISION_FUDGE)) {
        vec3 normal;
        if (distance > FLT_EPSILON || distance < -FLT_EPSILON) {
          vec3_norm(normal, bToA);
        } else {
          vec3_dup(normal, Axis_X);
        }
        float penetration = (simple->particle_radius * 2.0f) - sqrt(distance);
        PhysicsParticleContact* contact = &next[found++];
        physics_particle_contact_initialize(contact, a, b, simple->contact_restitution, normal, penetration);
         if (found >= limit) {
           return found;
         }
      }
    }
  }
  return found;
}

PhysicsParticleContactGenerator* physics_particle_contact_generator_allocate_simple(const PhysicsParticleWorld* world, float particle_radius, float contact_restitution) {
  PhysicsSimpleContactGenerator* simple = (PhysicsSimpleContactGenerator*)calloc(1, sizeof(PhysicsSimpleContactGenerator));
  simple->base.add_contacts_vfn = physics_simple_contact_generator_add_contacts;
  simple->world = world;
  simple->particle_radius = particle_radius;
  simple->contact_restitution = contact_restitution;
  return &simple->base;
}

struct PhysicsPlaneContactGenerator {
  PhysicsParticleContactGenerator base;
  const PhysicsParticleWorld* world;
  float particle_radius;
  float contact_restitution;
  vec3 normal;
  float d;
};

static int physics_plane_contact_generator_add_contacts(const PhysicsParticleContactGenerator* generator, PhysicsParticleContact* next, int limit) {
  const PhysicsPlaneContactGenerator* plane = (const PhysicsPlaneContactGenerator*)generator;
  const PhysicsParticleWorld* world = plane->world;
  PhysicsParticle* const * particles = world->particles;
  int particle_count = world->particle_count;
  int found = 0;
  for (int i = 0; i < particle_count && found < limit; i++) {
    float penetration =  plane->d - (vec3_mul_inner(plane->normal, particles[i]->position) - plane->particle_radius);
    if ((penetration + COLLISION_FUDGE) > FLT_EPSILON) {
      PhysicsParticleContact* contact = &next[found++];
      physics_particle_contact_initialize(contact, particles[i], NULL, plane->contact_restitution, plane->normal, penetration);
    }
  }
  return found;
}

PhysicsParticleContactGenerator* physics_particle_contact_generator_allocate_plane(const PhysicsParticleWorld* world, const vec3 normal, float d, float particle_radius, float contact_restitution) {
  PhysicsPlaneContactGenerator* plane = (PhysicsPlaneContactGenerator*)calloc(1, sizeof(PhysicsPlaneContactGenerator));
  plane->base.add_contacts_vfn = physics_plane_contact_generator_add_contacts;
  plane->world = world;
  plane->d = d;
  plane->particle_radius = particle_radius;
  plane->contact_restitution = contact_restitution;
  vec3_dup(plane->normal, normal);
  return &plane->base;
}

void physics_particle_world_initialize(PhysicsParticleWorld* world) {
  memset(world, 0, sizeof(PhysicsParticleWorld));
}

static int physics_particle_world_generate_contacts(PhysicsParticleWorld* world) {
  int found = 0;
  const PhysicsParticleContactGenerator* head = LINKED_LIST_GET_HEAD(&world->contact_generators);
  while (head && found < MAX_CONTACTS) {
    found += physics_particle_contact_generator_add_contacts(head, world->contacts + found, MAX_CONTACTS - found);
    head = LINKED_LIST_GET_NEXT(head);
  }
  return found;
}

static void physics_particle_world_resolve_contacts(PhysicsParticleWorld* world,  PhysicsParticleContact* contacts, int count, float dt) {
  (void)(world);
  int iterations = 0;
  PHYSICS_PRINT("Resolving %i contacts\n", count);
  for (; iterations < MAX_RESOLVE_ITERATIONS; iterations++) {
    float max = FLT_MAX;
    int max_idx = count;
    for (int i = 0; i < count; i++) {
      float sep_velocity = physics_particle_contact_get_separating_velocity(&contacts[i]);
      if (sep_velocity < max && (sep_velocity < 0 || contacts[i].penetration > FLT_EPSILON)) {
        max = sep_velocity;
        max_idx = i;
      }
    }

    // all bodies are no longer penetrating
    if (max_idx == count) break;

    // resolve one
    float sep_velocity = physics_particle_contact_get_separating_velocity(&contacts[max_idx]);
    (void)(sep_velocity);
    PHYSICS_PRINT("\tcontact%s: sep %f penetration %f corrected to ", (contacts[max_idx].particles[1] == NULL) ? " (floor)" : "", sep_velocity, contacts[max_idx].penetration);
    physics_particle_contact_resolve(&contacts[max_idx], dt);

    // update penetration
    vec3* delta_resolution = contacts[max_idx].delta_resolution;
    for (int i = 0; i < count; i++) {
      if (contacts[i].particles[0] == contacts[max_idx].particles[0]) {
        contacts[i].penetration -= vec3_mul_inner(delta_resolution[0], contacts[i].normal);
      } else if (contacts[i].particles[0] == contacts[max_idx].particles[1]) {
        contacts[i].penetration -= vec3_mul_inner(delta_resolution[1], contacts[i].normal);
      }
      if (contacts[i].particles[1]) {
        if (contacts[i].particles[1] == contacts[max_idx].particles[0]) {
          contacts[i].penetration += vec3_mul_inner(delta_resolution[0], contacts[i].normal);
        } else if (contacts[i].particles[1] == contacts[max_idx].particles[1]) {
          contacts[i].penetration += vec3_mul_inner(delta_resolution[1], contacts[i].normal);
        }
      }
    }
    sep_velocity = physics_particle_contact_get_separating_velocity(&contacts[max_idx]);
    PHYSICS_PRINT("sep %f penetration %f\n", sep_velocity, contacts[max_idx].penetration);
  }

  if (iterations == MAX_RESOLVE_ITERATIONS) {
    PHYSICS_PRINT("WARNING: Max resolve iterations(%i) reached! Some contacts may not be resolved.\n", MAX_RESOLVE_ITERATIONS);
  } else {
    PHYSICS_PRINT("Iterations used: %i\n", iterations);
  }
}

void physics_particle_world_run(PhysicsParticleWorld* world, float dt) {
  // Apply forces
  for (int i = 0; i < world->force_generator_count; i++) {
    PhysicsParticleForceGeneratorRegistration* reg = &world->force_generators[i];
    physics_particle_force_generator_update_force(reg->generator, reg->particle, dt);
  }

  // Advance particles
  for (int i = 0; i < world->particle_count; i++) {
    physics_particle_integrate(world->particles[i], dt);
  }

  // Find contacts
  int found = physics_particle_world_generate_contacts(world);

  // Resolve contacts
  if (found > 0) {
    if (found >= MAX_CONTACTS) {
      printf("WARNING: Max contacts(%i) reached! Some contacts may be missed.\n", MAX_CONTACTS);
    }
    physics_particle_world_resolve_contacts(world, world->contacts, found, dt);
  }
}

void physics_particle_world_debug_render(const PhysicsParticleWorld* world) {
  for (int i = 0; i < world->force_generator_count; i++) {
    const PhysicsParticleForceGeneratorRegistration* reg = &world->force_generators[i];
    physics_particle_force_generator_debug_render(reg->generator, reg->particle);
  }
  for (int i = 0; i < world->particle_count; i++) {
    physics_particle_debug_render(world->particles[i]);
  }
}

int physics_particle_world_add_particle(PhysicsParticleWorld* world, PhysicsParticle* p) {
  if (world->particle_count >= MAX_PARTICLES || p->world)
    return 1;

  world->particles[world->particle_count++] = p;
  p->world = world;
  return 0;
}

int physics_particle_world_remove_particle(PhysicsParticleWorld* world, PhysicsParticle* p) {
  if (p->world != world)
    return 1;

  for (int i = 0; i < world->particle_count; i++) {
    if (world->particles[i] == p) {
      int last = --world->particle_count;
      world->particles[i] = world->particles[last];
      world->particles[last] = NULL;
      p->world = NULL;
      physics_particle_world_unregister_all_force_generators(world, p);
      return 0;
    }
  }
  return 1;
}

int physics_particle_world_register_force_generator(PhysicsParticleWorld* world, const PhysicsParticleForceGenerator* generator, PhysicsParticle* p) {
  if (world->force_generator_count >= MAX_FORCE_GENERATORS)
    return 1;
  PhysicsParticleForceGeneratorRegistration* reg = &world->force_generators[world->force_generator_count++];
  reg->generator = generator;
  reg->particle = p;
  return 0;
}

int physics_particle_world_unregister_force_generator(PhysicsParticleWorld* world, const PhysicsParticleForceGenerator* generator, PhysicsParticle* p) {
  const int count = world->force_generator_count;
  for (int i = 0; i < count; i++) {
    PhysicsParticleForceGeneratorRegistration* reg = &world->force_generators[i];
    if (reg->generator == generator && reg->particle == p) {
      int last = --world->force_generator_count;
      (*reg) = world->force_generators[last];
      world->force_generators[last].generator = NULL;
      world->force_generators[last].particle = NULL;
      return 0;
    }
  }
  return 1;
}

void physics_particle_world_unregister_all_force_generators(PhysicsParticleWorld* world, PhysicsParticle* p) {
    int i = 0;
    while (i < world->force_generator_count) {
      PhysicsParticleForceGeneratorRegistration* reg = &world->force_generators[i];
      if (reg->particle == p) {
        int last = --world->force_generator_count;
        (*reg) = world->force_generators[last];
        world->force_generators[last].generator = NULL;
        world->force_generators[last].particle = NULL;
      } else {
        i++;
      }
    }
}

void physics_particle_world_register_contact_generator(PhysicsParticleWorld* world, PhysicsParticleContactGenerator* cgen) {
  LINKED_LIST_APPEND(&world->contact_generators, cgen);
}

void physics_particle_world_unregister_contact_generators(PhysicsParticleWorld* world, PhysicsParticleContactGenerator* cgen) {
  LINKED_LIST_REMOVE(&world->contact_generators, cgen);
}
