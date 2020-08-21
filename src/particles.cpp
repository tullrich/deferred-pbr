#include "particles.h"
#include "assets.h"

#define PARTICLE_GRAVITY -9.81f

DEFINE_ENUM(ParticleShadingMode, particle_shading_mode_strings, ENUM_ParticleShadingMode);
DEFINE_ENUM(ParticleOrientationMode, particle_orient_mode_strings, ENUM_ParticleOrientationMode);

static void random_direction(vec3 out) {
  float theta = (float)M_PI * utility_random_real11();
  float phi = acosf(utility_random_real11());
  out[0] = cosf(theta)*sinf(phi);
  out[1] = sinf(theta)*sinf(phi);
  out[2] = -cosf(phi);
}

static void random_conical_direction(vec3 out, const vec3 axis) {
  float phi = atan2f(axis[0], -axis[2] );
  float theta = atan2f(axis[1], axis[0] );
  out[0] = cosf(theta) * sinf(phi);
  out[1] = sinf(theta) * sinf(phi);
  out[2] = -cosf(phi);

}

void particle_update(const ParticleEmitterDesc* def, Particle* part, float dt) {
  if (def->simulate_gravity) {
    part->velocity[1] += PARTICLE_GRAVITY * dt;
  }

  vec3 deltaPos;
  vec3_scale(deltaPos, part->velocity, dt);
  vec3_add(part->pos, part->pos, deltaPos);
  part->ttl -= dt;

  float min_dt = fminf(part->ttl, dt);

  vec4 dt_color;
  vec4_scale(dt_color, part->delta_color, dt);
  vec4_add(part->color, part->color, dt_color);

  vec3 dt_scale;
  vec3_scale(dt_scale, part->delta_scale, dt);
  vec3_add(part->scale, part->scale, dt_scale);
}

int particle_emitter_initialize(ParticleEmitter *emitter, const ParticleEmitterDesc* def) {
  memset(emitter, 0, sizeof(ParticleEmitter));
  emitter->desc = def;
  if ((emitter->particles = (Particle*)calloc(def->max, sizeof(Particle))) == NULL) {
    return 1; // error: unable to allocate memory
  }
  emitter->max = def->max;
  quat_identity(emitter->rot);
  emitter->scale = 1.0f;
  return 0;
}

int particle_emitter_destroy(ParticleEmitter *emitter) {
  if (!emitter->desc || !emitter->particles) {
    return 1; // error: uninitialized emitter
  }
  free(emitter->particles);
  if (emitter->sort_records) {
    free(emitter->sort_records);
  }
  memset(emitter, 0, sizeof(ParticleEmitter));
  return 0;
}

int particle_emitter_refresh(ParticleEmitter *emitter) {
  const ParticleEmitterDesc* desc = emitter->desc;
  int ret;
  if ((ret = particle_emitter_destroy(emitter))) {
    return ret;
  }
  if ((ret = particle_emitter_initialize(emitter, desc))) {
    return ret;
  }
  return 0;
}

Particle* particle_emitter_emit_one(ParticleEmitter* emitter) {
  const ParticleEmitterDesc* desc = emitter->desc;
  if (emitter->count >= emitter->max) {
    return NULL; // error: not enough space
  }
  Particle* part = &emitter->particles[emitter->count++];
  memset(part, 0, sizeof(Particle));
  part->ttl = desc->life_time + utility_random_real11() * desc->life_time_variance;
  part->scale[0] = part->scale[1] = part->scale[2] = desc->start_scale;
  vec3_swizzle(part->delta_scale, (desc->end_scale - desc->start_scale) / part->ttl);
  random_direction(part->velocity);
  vec3_dup(part->rot, part->velocity);
  //quat_rotation_between(part->rot, part->velocity, Axis_Forward);
  vec3_scale(part->velocity, part->velocity, desc->speed + utility_random_real11() * desc->speed_variance);
  vec4_dup(part->color, desc->start_color);
  vec4_sub(part->delta_color, desc->end_color, desc->start_color);
  vec4_scale(part->delta_color, part->delta_color, 1.0f/part->ttl);
  return part;
}

void particle_emitter_burst(ParticleEmitter* emitter, int count) {
  for (int i = 0; i < count; i++) {
    particle_emitter_emit_one(emitter);
  }
}

void particle_emitter_destroy_at_index(ParticleEmitter* emitter, int index) {
  if (index >= 0 && index < emitter->count) {
    if (index != emitter->count-1) {
      // swap end into hole except for last element
      emitter->particles[index] = emitter->particles[emitter->count-1];
    }
    emitter->count--;
  }
}

void particle_emitter_update(ParticleEmitter* emitter, float dt) {
  const ParticleEmitterDesc* desc = emitter->desc;

  // spawn particles
  if (!emitter->muted && desc->spawn_rate > 0) {
    float rate = 1.0f / desc->spawn_rate;
    emitter->time_till_spawn += dt;
    while (emitter->time_till_spawn > rate) {
      particle_emitter_emit_one(emitter);
      emitter->time_till_spawn -= rate;
    }
  }

  // advance particle state
  for (int i = 0; i < emitter->count; i++) {
    Particle* part = &emitter->particles[i];
    particle_update(desc, part, dt);
  }

  // destroy particles
  for (int i = 0; i < emitter->count; i++) {
    Particle* part = &emitter->particles[i];
    if (part->ttl <= 0.0f) {
      particle_emitter_destroy_at_index(emitter, i);
    }
  }
}

static int partition(SortRecord* recs, int low, int high) {
  SortRecord pivot = recs[high];
  int i = low - 1;
  for (int j = low; j <= high - 1; j++) {
    if (recs[j].depth <= pivot.depth) {
      i++;

      SortRecord tmp = recs[i];
      recs[i] = recs[j];
      recs[j] = tmp;
    }
  }
  recs[high] = recs[i + 1];
  recs[i + 1] = pivot;
  return i + 1;
}

static void quicksort(SortRecord* recs, int low, int high) {
  if (low < high) {
    int pi = partition(recs, low, high);
    quicksort(recs, low, pi - 1);
    quicksort(recs, pi + 1, high);
  }
}

void particle_emitter_sort(ParticleEmitter* emitter, const vec3 cam_position) {
  // allocate sort records the first time we sort
  if (!emitter->sort_records) {
    if (emitter->max <= 0 || (emitter->sort_records = (SortRecord*)calloc(emitter->max, sizeof(SortRecord))) == NULL) {
      return; // unable to allocate sort records
    }
  }

  // calculate depth from camera
  vec3 diff;
  for (int i = 0; i < emitter->count; i++) {
    Particle* part = &emitter->particles[i];
    SortRecord* rec = &emitter->sort_records[i];
    vec3_sub(diff, part->pos, cam_position);
    rec->depth = vec3_len2(diff);
    rec->index = i;
  }

  // sort
  quicksort(emitter->sort_records, 0, emitter->count-1);
}

static int get_particle_texture_index(GLuint texId, const ParticleEmitterTextureDesc* texDefs, int texDefsCount) {
  for (int i = 0; i < texDefsCount; i++) {
    if (texId == texDefs[i].texture) {
      return i;
    }
  }
  return 0;
}

void particle_emitter_gui(ParticleEmitterDesc* desc, ParticleEmitter* emitter, const ParticleEmitterTextureDesc* tex_defs, int tex_defs_count) {
  if (ImGui::Button("Flare")) {
    *desc = gEmitterDescs[0];
    particle_emitter_refresh(emitter);
  }
  ImGui::SameLine();
  if (ImGui::Button("Particle")) {
    *desc = gEmitterDescs[1];
    particle_emitter_refresh(emitter);
  }
  ImGui::SameLine();
  if (ImGui::Button("Smoke")) {
    *desc = gEmitterDescs[2];
    particle_emitter_refresh(emitter);
  }
  if ( ImGui::Button("Refresh") ) {
    particle_emitter_refresh(emitter);
  }
  ImGui::SliderFloat( "Spawn Rate", &desc->spawn_rate, 0, 500.0f );
  ImGui::SliderInt( "Max Particles", &desc->max, 1, 2048 );

  ImGui::Combo( "Shading Mode", ( int* )&desc->shading_mode, particle_shading_mode_strings, particle_shading_mode_strings_count );

  if (desc->shading_mode == PARTICLE_SHADING_TEXTURED ) {
    int texture_idx = get_particle_texture_index(desc->texture, tex_defs, tex_defs_count);
    if (ImGui::BeginCombo("Texture", tex_defs[texture_idx].name, 0)) {
      for (int i = 0; i < tex_defs_count; i++) {
        if (ImGui::Selectable(tex_defs[i].name, (texture_idx == i))) {
          desc->texture = tex_defs[i].texture;
        }
        if ((texture_idx == i)) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
  }
  ImGui::SliderFloat( "Start Scale", &desc->start_scale, .01f, 10.0f );
  ImGui::SliderFloat( "End Scale", &desc->end_scale, .01f, 10.0f );
  ImGui::ColorEdit4( "Start Color", desc->start_color );
  ImGui::ColorEdit4( "End Color", desc->end_color );

  ImGui::Combo( "Orientation Mode", ( int* )&desc->orient_mode, particle_orient_mode_strings, particle_orient_mode_strings_count );
  ImGui::Checkbox( "Depth Sort", ( bool* )&desc->depth_sort_alpha_blend );
  ImGui::Checkbox( "Soft", ( bool* )&desc->soft );
  ImGui::Checkbox( "Gravity", ( bool* )&desc->simulate_gravity );
  ImGui::Checkbox( "Mute", ( bool* )&emitter->muted );
  ImGui::SliderFloat( "Life Time", &desc->life_time, 0.0f, 10.0f );
  ImGui::SliderFloat( "Life Time Variance", &desc->life_time_variance, 0.0f, 10.0f );
  ImGui::SliderFloat( "Speed", &desc->speed, 0.0f, 10.0f );
  ImGui::SliderFloat( "Speed Variance", &desc->speed_variance, 0.0f, 10.0f );

  ImGui::SliderFloat( "Local Scale", &emitter->scale, 0.0f, 10.0f );
  ImGui::InputFloat3( "Local Translation", emitter->pos );
  if ( ImGui::Button( "Burst" ) ) {
    particle_emitter_burst(emitter, desc->burst_count);
  }
  ImGui::SliderInt( "Burst Count", &desc->burst_count, 0, 1000 );
}
