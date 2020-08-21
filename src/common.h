#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <algorithm>

#include "SDL.h"
#include <GL/glew.h>

#include "stb_image.h"
#include "tinyobj_loader_c/tinyobj_loader_c.h"
#include "imgui/imgui.h"

#include "linmath.h"

static inline void vec2_set(vec3 r, float x, float y) {
	r[0] = x; r[1] = y;
}
static inline void vec2_dup(vec2 r, vec2 const v) {
	for (int i=0; i<2; ++i)
		r[i] = v[i];
}
static inline void vec2_print(vec2 const v) {
  printf("<%f, %f>\n", v[0], v[1]);
}
static inline void vec3_set(vec3 r, float x, float y, float z) {
	r[0] = x; r[1] = y; r[2] = z;
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
static inline void vec3_print(vec3 const v) {
  printf("<%f, %f, %f>\n", v[0], v[1], v[2]);
}
static inline void vec4_set(vec3 r, float x, float y, float z, float w) {
	r[0] = x; r[1] = y; r[2] = z; r[3] = w;
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
static inline void vec4_print(vec4 const v) {
  printf("<%f, %f, %f, %f>\n", v[0], v[1], v[2], v[3]);
}
static inline void quat_dup(quat r, quat const v) {
	for(int i=0; i<4; ++i)
		r[i] = v[i];
}

static inline void quat_from_euler(quat out, const vec3 euler) {
  quat_identity(out);
  vec3 axis;
  quat xrot, yrot, zrot;
  axis[0] = 1.0f; axis[1] = axis[2] = 0.0f;
  quat_rotate(xrot, euler[0], axis);
  axis[1] = 1.0f; axis[0] = axis[2] = 0.0f;
  quat_rotate(yrot, euler[1], axis);
  axis[2] = 1.0f; axis[0] = axis[1] = 0.0f;
  quat_rotate(zrot, euler[2], axis);
  quat tmp;
  quat_mul(tmp, xrot, yrot);
  quat_mul(out, tmp, zrot);
}
static inline void quat_to_axis_angle(float* angle, vec3 axis, const quat q) {
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
static inline void quat_print(const quat q) {
  printf("<%f, %f, %f, %f>\n", q[0], q[1], q[2], q[3]);
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
	if (m[0][2] == 1.0f) {
		euler[0] = atan2f(-m[1][0], -m[2][0]);
 		euler[1] = -(float)M_PI/2.0f;
		euler[2] = 0.0f;
  } else if (m[0][2] == -1.0f) {
		euler[0] = atan2f(m[1][0], m[2][0]);
 		euler[1] = (float)M_PI/2.0f;
		euler[2] = 0.0f;
	} else {
 		euler[1] = -asinf(m[0][2]);
		float cosTheta = cosf(euler[1]);
		euler[0] = atan2f(m[1][2]/cosTheta,m[2][2]/cosTheta);
		euler[2] = atan2f(m[0][1]/cosTheta,m[0][0]/cosTheta);
	}
}

static inline void mat4x4_make_transform(mat4x4 r, const quat rotation, const vec3 translation) {
 mat4x4_identity(r);
 mat4x4_from_quat(r, rotation);
 vec3_dup(r[3], translation);
 r[3][3] = 1.0f;
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

static inline void mat4x4_inf_perspective(mat4x4 m, float y_fov, float aspect, float n, float f) {
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

#define SIDEBAR_WIDTH 350
#define VIEWPORT_WIDTH 1150
#define WINDOW_WIDTH (VIEWPORT_WIDTH + (SIDEBAR_WIDTH * 2))
#define WINDOW_HEIGHT 1000
#define VIEWPORT_HEIGHT 1000
#define VIEWPORT_X_OFFSET SIDEBAR_WIDTH
#define Z_NEAR 0.1f
#define Z_FAR 100.0f

#define STATIC_ELEMENT_COUNT(arr) sizeof(arr)/sizeof(arr[0])
#define RAD_TO_DEG(rad) (rad*180.0f/(float)M_PI)
#define DEG_TO_RAD(deg) (deg*(float)M_PI/180.0f)
#define BOOL_TO_STRING(b) ((b) ? "true" : "false")

#ifdef _WIN32
#define M_PI_2 (M_PI/2.0)
#endif

#define ENUM_ORD_VALUE(V, S) V,
#define ENUM_STR_VALUE(V, S) S,
#define DECLARE_ENUM(name, strTable, values) 										\
	typedef enum {																								\
		values(ENUM_ORD_VALUE)																			\
	} name;																												\
	extern const char* strTable[];																\
	extern const int strTable##_count;
#define DEFINE_ENUM(name, strTable, values)											\
	const char* strTable[] = {																		\
	  values(ENUM_STR_VALUE)																			\
	};																														\
	const int strTable##_count = STATIC_ELEMENT_COUNT(strTable);

#include "utility.h"
