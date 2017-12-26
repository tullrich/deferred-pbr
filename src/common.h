#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "SDL.h"
#include <GL/glew.h>

#include "stb_image.h"
#include "FreeImage.h"
#include "tinyobj_loader_c/tinyobj_loader_c.h"

#include "linmath.h"

static inline void vec2_dup(vec2 r, vec2 const v) {
	for (int i=0; i<2; ++i)
		r[i] = v[i];
}
static inline void vec3_dup(vec3 r, vec3 const v) {
	for(int i=0; i<3; ++i)
		r[i] = v[i];
}
static inline void vec3_swizzle(vec3 r, float val) {
	r[0] = r[1] = r[2] = val;
}
static inline void vec3_zero(vec3 r) {
	r[0] = r[1] = r[2] = 0.0f;
}
static inline void vec3_negate_in_place(vec3 r) {
	for(int i=0; i<3; ++i)
		r[i] = -r[i];
}
static inline float vec3_angle(vec3 a, vec3 b) {
	float cosAngle = vec3_mul_inner(a, b);
	return acosf(cosAngle);
}
static inline void vec4_dup(vec4 r, vec4 const v) {
	for(int i=0; i<4; ++i)
		r[i] = v[i];
}
static inline void vec4_swizzle(vec4 r, float val) {
	r[0] = r[1] = r[2] = r[3] = val;
}
static inline void vec4_zero(vec4 r) {
	r[0] = r[1] = r[2] = r[3] = 0.0f;
}
static inline void vec4_negate_in_place(vec4 r) {
	for(int i=0; i<4; ++i)
		r[i] = -r[i];
}
static inline void vec4_rgba(vec4 out, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	out[0] = r/255.0f; out[1] = g/255.0f;
	out[2] = b/255.0f; out[3] = a/255.0f;
}
static inline void quat_dup(quat r, quat const v) {
	for(int i=0; i<4; ++i)
		r[i] = v[i];
}
static inline void quat_to_axis_angle(float* angle, vec3 axis, quat const q) {
	*angle = acosf(q[3]) * 2.0f;
	float sin_half_angle = sqrtf(1.0f-q[3]*q[3]);
	for(int i=0; i<3; ++i) {
		if(sin_half_angle > 0.0f) {
			axis[i] = q[i] / sin_half_angle;
		} else {
			axis[i] = 0.0f;
		}
	}
}
static inline void quat_rotation_between(quat out, const vec3 a, const vec3 b) {
	quat rot;
	vec3_mul_cross(rot, a, b);
	rot[3] = sqrtf(vec3_len2(a)* vec3_len2(b)) + vec3_mul_inner(a, b);
	quat_norm(out, rot);
}
static inline void mat4x4_printf(mat4x4 m) {
	printf("[ %f, %f, %f, %f\n", m[0][0], m[0][1], m[0][2], m[0][3]);
	printf("  %f, %f, %f, %f\n", m[1][0], m[1][1], m[1][2], m[1][3]);
	printf("  %f, %f, %f, %f\n", m[2][0], m[2][1], m[2][2], m[2][3]);
	printf("  %f, %f, %f, %f ]\n", m[3][0], m[3][1], m[3][2], m[3][3]);
}
static inline void mat4x4_zero(mat4x4 m) {
	for (int i=0; i<4; i++)
		vec4_zero(m[i]);
}
static inline void mat4x4_to_euler(vec3 euler, mat4x4 const m) {
	float r31 = m[2][0];
	 if (r31 == 1.0f) {
		euler[0] = atan2f(-m[0][1], -m[0][2]);
   		euler[1] = -(float)M_PI/2.0f;
		euler[2] = 0.0f;
	 } else if (r31 == -1.0f) {
		euler[0] = atan2f(m[0][1], m[0][2]);
   		euler[1] = (float)M_PI/2.0f;
		euler[2] = 0.0f;
	 } else {
   		euler[1] = -asinf(r31);
		float cosTheta = cosf(euler[1]);

		euler[0] = atan2f(m[2][1]/cosTheta,m[2][2]/cosTheta);
		euler[2] = atan2f(m[1][0]/cosTheta,m[0][0]/cosTheta);
	 }
 }
 static inline void mat4x4_make_transform(mat4x4 r, const vec3 scale, const quat rotation, const vec3 translation) {
	 mat4x4_identity(r);
	 mat4x4_from_quat(r, rotation);
	 mat4x4_scale_aniso(r, r, scale[0], scale[1], scale[2]);
	 vec3_dup(r[3], translation);
	 r[3][3] = 1.0f;
 }

 static inline void mat4x4_make_transform_uscale(mat4x4 r, float scale, const quat rotation, const vec3 translation) {
	 vec3 scaleVec;
	 vec3_swizzle(scaleVec, scale);
	 mat4x4_make_transform(r, scaleVec, rotation, translation);
 }

 static inline void mat4x4_inf_perspective(mat4x4 m, float y_fov, float aspect, float n, float f)
 {
 	float const e = 1.f / tanf(y_fov / 2.f);

 	m[0][0] = e / aspect;
 	m[0][1] = 0.f;
 	m[0][2] = 0.f;
 	m[0][3] = 0.f;

 	m[1][0] = 0.f;
 	m[1][1] = e;
 	m[1][2] = 0.f;
 	m[1][3] = 0.f;

 	m[2][0] = 0.f;
 	m[2][1] = 0.f;
 	m[2][2] = -1.f;
 	m[2][3] = -1.f;

 	m[3][0] = 0.f;
 	m[3][1] = 0.f;
 	m[3][2] = -2.0f * n;
 	m[3][3] = 0.f;
 }

 #define FORMAT_VEC3(v) v[0], v[1], v[2]
 #define FORMAT_VEC4(v) v[0], v[1], v[2], v[3]

extern const vec4 Black;
extern const vec4 White;
extern const vec4 Red;
extern const vec4 Green;
extern const vec4 Blue;
extern const vec4 Yellow;
extern const vec4 Zero;
extern const vec4 Axis_Forward;
extern const vec4 Axis_Up;
extern const vec4 Axis_Right;
extern const vec4 Vec_Zero;
extern const vec4 Vec_One;
extern const quat Quat_Identity;

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 640
#define Z_NEAR 0.1f
#define Z_FAR 100.0f

#define STATIC_ELEMENT_COUNT(arr) sizeof(arr)/sizeof(arr[0])
#define RAD_TO_DEG(rad) (deg*180.0f/(float)M_PI)
#define DEG_TO_RAD(deg) (deg*(float)M_PI/180.0f)

#ifdef _WIN32
#define M_PI_2 (M_PI/2.0)
#endif

typedef enum
{
	RENDER_MODE_SHADED,
	RENDER_MODE_ALBEDO,
	RENDER_MODE_NORMAL,
	RENDER_MODE_ROUGHNESS,
	RENDER_MODE_METALNESS,
	RENDER_MODE_DEPTH
} RenderMode;

#include "utility.h"
