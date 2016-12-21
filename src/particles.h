#pragma once
#include "common.h"

typedef enum
{
	PARTICLE_SHADING_FLAT,
	PARTICLE_SHADING_TEXTURED,
} ParticleShadingMode;

// single particle
typedef struct
{
	// position in emitter local space
	vec3 pos;

	// xyz-euler angles in emitter local space
	vec3 rot;

	// scale in emitter local space
	vec3 scale;

	// velocity
	vec3 velocity;

	// time to live (counts down to zero)
	float ttl;

	// current color
	vec4 color;

	// color change/second
	vec4 delta_color;

	// scale change/second
	vec3 delta_scale;
} Particle;

// definition of a particle emitter
typedef struct
{
	// max living particles at once
	int max;

	// spawn rate in particles/second
	float spawn_rate;

	// starting rgba
	vec4 start_color;

	// ending rgba
	vec4 end_color;

	// if true, particles will be oriented parallel to the near plane
	int billboard;

	// speed of the particles
	float speed;

	// speed variance
	float speed_variance;

	// amount of time each particle lives
	float life_time;

	// life time variance
	float life_time_variance;

	// particle shading mode
	ParticleShadingMode shading_mode;

	// particle texture used if render_mode is set to PARTICLE_SHADING_TEXTURED
	GLuint texture;

	// starting scale of the particles
	float start_scale;

	// ending scale of the particles
	float end_scale;
} ParticleEmitterDesc;

// instance of a ParticleEmitterDesc
typedef struct
{
	// definition that created this system
	const ParticleEmitterDesc* desc;

	// buffer of particle
	Particle* particles;

	// current count of living particles
	int count;

	// max living particles at once
	int max;

	// position in world space
	vec3 pos;

	// orientation in world space
	quat rot;

	// uniform scale in world space
	float scale;

	// time remaining until next emission
	float time_till_spawn;

	// stops automatic spawning when set
	int muted;
} ParticleEmitter;

void particle_update(Particle* part, float dt);

int particle_emitter_initialize(ParticleEmitter *emitter, const ParticleEmitterDesc* def);
int particle_emitter_destroy(ParticleEmitter *emitter);

Particle* particle_emitter_emit_one(ParticleEmitter* emitter);
void particle_emitter_burst(ParticleEmitter* emitter, int count);

void particle_emitter_destroy_at_index(ParticleEmitter* emitter, int index);
void particle_emitter_update(ParticleEmitter* emitter, float dt);
