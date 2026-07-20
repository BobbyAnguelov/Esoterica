#ifndef UMATH_H_INCLUDED
#define UMATH_H_INCLUDED

#include <math.h>
#include <float.h>
#include <stdbool.h>

#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable: 4201)
#endif

#define um_inline static inline

#if defined(__cplusplus)
	#define um_abi extern "C"
#else
	#define um_abi 
#endif

typedef struct um_vec2 {
	union {
		struct { float x, y; };
		struct { float v[2]; };
	};
} um_vec2;

typedef struct um_vec3 {
	union {
		struct { float x, y, z; };
		struct { um_vec2 xy; };
		struct { float v[3]; };
	};
} um_vec3;

typedef struct um_vec4 {
	union {
		struct { float x, y, z, w; };
		struct { um_vec3 xyz; };
		struct { um_vec2 xy; };
		struct { float v[4]; };
	};
} um_vec4;

typedef struct um_quat {
	union {
		struct { float x, y, z, w; };
		struct { um_vec4 xyzw; };
		struct { um_vec3 xyz; };
		struct { float v[4]; };
	};
} um_quat;

typedef struct um_mat {
	union {
		struct { float m[16]; };
		struct { um_vec4 cols[4]; };
		struct { float m11, m21, m31, m41, m12, m22, m32, m42, m13, m23, m33, m43, m14, m24, m34, m44; };
	};
} um_mat;

#define UM_PI (3.14159265358979323846f)
#define UM_2PI (6.28318530717958647692f)
#define UM_RCP_PI (1.0f / 3.14159265358979323846f)
#define UM_RCP_2PI (1.0f / 6.28318530717958647692f)
#define UM_RAD_TO_DEG (180.0f / UM_PI)
#define UM_DEG_TO_RAD (UM_PI / 180.0f)

#if defined(__cplusplus__)
	#define um_new(type) type
#else
	#define um_new(type) (type)
#endif

#define um_v2(x, y) (um_new(um_vec2){{{ (x), (y) }}})
#define um_v3(x, y, z) (um_new(um_vec3){{{ (x), (y), (z) }}})
#define um_v4(x, y, z, w) (um_new(um_vec4){{{ (x), (y), (z), (w) }}})

#define um_quat_xyzw(x, y, z, w) (um_new(um_quat){{{ (x), (y), (z), (w) }}})

#define um_mat_rows(m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34, m41, m42, m43, m44, ...) \
	(um_new(um_mat){{{ \
	(m11), (m21), (m31), (m41), \
	(m12), (m22), (m32), (m42), \
	(m13), (m23), (m33), (m43), \
	(m14), (m24), (m34), (m44), }} __VA_ARGS__ })

#define um_mat_cols(m11, m21, m31, m41, m12, m22, m32, m42, m13, m23, m33, m43, m14, m24, m34, m44, ...) \
	(um_new(um_mat){{{ \
	(m11), (m21), (m31), (m41), \
	(m12), (m22), (m32), (m42), \
	(m13), (m23), (m33), (m43), \
	(m14), (m24), (m34), (m44), }} __VA_ARGS__ })

#define um_zero2 (um_v2(0, 0))
#define um_zero3 (um_v3(0, 0, 0))
#define um_zero4 (um_v4(0, 0, 0, 0))

#define um_one2 (um_v2(1, 1))
#define um_one3 (um_v3(1, 1, 1))
#define um_one4 (um_v4(1, 1, 1, 1))

#define um_quat_identity um_quat_xyzw(0, 0, 0, 1)

extern const um_mat um_mat_identity;

um_inline float um_sqrt(float a) { return sqrtf(a); }
um_inline float um_abs(float a) { return fabsf(a); }
um_inline float um_min(float a, float b) { return a < b ? a : b; }
um_inline float um_max(float a, float b) { return b < a ? a : b; }
um_inline float um_clamp(float a, float minv, float maxv) { return um_min(um_max(a, minv), maxv); }
um_inline float um_lerp(float a, float b, float t) { return a*(1.0f-t) + b*t; }

um_inline float um_smoothstep(float a) { return a * a * (3.0f - 2.0f * a); }

um_inline um_vec2 um_dup2(float a) { return um_v2(a, a); }
um_inline um_vec2 um_add2(um_vec2 a, um_vec2 b) { return um_v2(a.x + b.x, a.y + b.y); }
um_inline um_vec2 um_sub2(um_vec2 a, um_vec2 b) { return um_v2(a.x - b.x, a.y - b.y); }
um_inline um_vec2 um_mul2(um_vec2 a, float b) { return um_v2(a.x * b, a.y * b); }
um_inline um_vec2 um_div2(um_vec2 a, float b) { float v = 1.0f / b; return um_v2(a.x * v, a.y * v); }
um_inline um_vec2 um_mad2(um_vec2 a, um_vec2 b, float c) { return um_v2(a.x + b.x*c, a.y + b.y*c); }
um_inline um_vec2 um_neg2(um_vec2 a) { return um_v2(-a.x, -a.y); }
um_inline um_vec2 um_rcp2(um_vec2 a) { return um_v2(1.0f / a.x, 1.0f / a.y); }
um_inline um_vec2 um_mulv2(um_vec2 a, um_vec2 b) { return um_v2(a.x * b.x, a.y * b.y); }
um_inline um_vec2 um_divv2(um_vec2 a, um_vec2 b) { return um_v2(a.x / b.x, a.y / b.y); }
um_inline float um_dot2(um_vec2 a, um_vec2 b) { return a.x*b.x + a.y*b.y; }
um_inline float um_length2(um_vec2 a) { return um_sqrt(a.x*a.x + a.y*a.y); }
um_inline um_vec2 um_min2(um_vec2 a, um_vec2 b) { return um_v2(um_min(a.x, b.x), um_min(a.y, b.y)); }
um_inline um_vec2 um_max2(um_vec2 a, um_vec2 b) { return um_v2(um_max(a.x, b.x), um_max(a.y, b.y)); }
um_inline um_vec2 um_clamp2(um_vec2 a, um_vec2 minv, um_vec2 maxv) { return um_v2(um_clamp(a.x, minv.x, maxv.x), um_clamp(a.y, minv.y, maxv.y)); }
um_inline um_vec2 um_lerp2(um_vec2 a, um_vec2 b, float t) { return um_v2(um_lerp(a.x, b.x, t), um_lerp(a.y, b.y, t)); }
um_inline um_vec2 um_normalize2(um_vec2 a) { float v = um_length2(a); v = v >= FLT_MIN ? 1.0f / v : 0.0f; return um_v2(a.x * v, a.y * v); }
um_inline bool um_equal2(um_vec2 a, um_vec2 b) { return (a.x == b.x) & (a.y == b.y); }

um_inline um_vec3 um_dup3(float a) { return um_v3(a, a, a); }
um_inline um_vec3 um_add3(um_vec3 a, um_vec3 b) { return um_v3(a.x + b.x, a.y + b.y, a.z + b.z); }
um_inline um_vec3 um_sub3(um_vec3 a, um_vec3 b) { return um_v3(a.x - b.x, a.y - b.y, a.z - b.z); }
um_inline um_vec3 um_mul3(um_vec3 a, float b) { return um_v3(a.x * b, a.y * b, a.z * b); }
um_inline um_vec3 um_div3(um_vec3 a, float b) { float v = 1.0f / b; return um_v3(a.x * v, a.y * v, a.z * v); }
um_inline um_vec3 um_mad3(um_vec3 a, um_vec3 b, float c) { return um_v3(a.x + b.x*c, a.y + b.y*c, a.z + b.z*c); }
um_inline um_vec3 um_neg3(um_vec3 a) { return um_v3(-a.x, -a.y, -a.z); }
um_inline um_vec3 um_rcp3(um_vec3 a) { return um_v3(1.0f / a.x, 1.0f / a.y, 1.0f / a.z); }
um_inline um_vec3 um_mulv3(um_vec3 a, um_vec3 b) { return um_v3(a.x * b.x, a.y * b.y, a.z * b.z); }
um_inline um_vec3 um_divv3(um_vec3 a, um_vec3 b) { return um_v3(a.x / b.x, a.y / b.y, a.z / b.z); }
um_inline float um_dot3(um_vec3 a, um_vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
um_inline float um_length3(um_vec3 a) { return um_sqrt(a.x*a.x + a.y*a.y + a.z*a.z); }
um_inline um_vec3 um_min3(um_vec3 a, um_vec3 b) { return um_v3(um_min(a.x, b.x), um_min(a.y, b.y), um_min(a.z, b.z)); }
um_inline um_vec3 um_max3(um_vec3 a, um_vec3 b) { return um_v3(um_max(a.x, b.x), um_max(a.y, b.y), um_max(a.z, b.z)); }
um_inline um_vec3 um_clamp3(um_vec3 a, um_vec3 minv, um_vec3 maxv) { return um_v3(um_clamp(a.x, minv.x, maxv.x), um_clamp(a.y, minv.y, maxv.y), um_clamp(a.z, minv.z, maxv.z)); }
um_inline um_vec3 um_lerp3(um_vec3 a, um_vec3 b, float t) { return um_v3(um_lerp(a.x, b.x, t), um_lerp(a.y, b.y, t), um_lerp(a.z, b.z, t)); }
um_inline um_vec3 um_normalize3(um_vec3 a) { float v = um_length3(a); v = v >= FLT_MIN ? 1.0f / v : 0.0f; return um_v3(a.x * v, a.y * v, a.z * v); }
um_inline um_vec3 um_cross3(um_vec3 a, um_vec3 b) { return um_v3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); }
um_inline bool um_equal3(um_vec3 a, um_vec3 b) { return (a.x == b.x) & (a.y == b.y) & (a.z == b.z); }

um_inline um_vec4 um_dup4(float a) { return um_v4(a, a, a, a); }
um_inline um_vec4 um_add4(um_vec4 a, um_vec4 b) { return um_v4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
um_inline um_vec4 um_sub4(um_vec4 a, um_vec4 b) { return um_v4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
um_inline um_vec4 um_mul4(um_vec4 a, float b) { return um_v4(a.x * b, a.y * b, a.z * b, a.w * b); }
um_inline um_vec4 um_div4(um_vec4 a, float b) { float v = 1.0f / b; return um_v4(a.x * v, a.y * v, a.z * v, a.w * v); }
um_inline um_vec4 um_mad4(um_vec4 a, um_vec4 b, float c) { return um_v4(a.x + b.x*c, a.y + b.y*c, a.z + b.z*c, a.w + b.w*c); }
um_inline um_vec4 um_neg4(um_vec4 a) { return um_v4(-a.x, -a.y, -a.z, -a.w); }
um_inline um_vec4 um_rcp4(um_vec4 a) { return um_v4(1.0f / a.x, 1.0f / a.y, 1.0f / a.z, 1.0f / a.w); }
um_inline um_vec4 um_mulv4(um_vec4 a, um_vec4 b) { return um_v4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }
um_inline um_vec4 um_divv4(um_vec4 a, um_vec4 b) { return um_v4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w); }
um_inline float um_dot4(um_vec4 a, um_vec4 b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
um_inline float um_length4(um_vec4 a) { return um_sqrt(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w); }
um_inline um_vec4 um_min4(um_vec4 a, um_vec4 b) { return um_v4(um_min(a.x, b.x), um_min(a.y, b.y), um_min(a.z, b.z), um_min(a.w, b.w)); }
um_inline um_vec4 um_max4(um_vec4 a, um_vec4 b) { return um_v4(um_max(a.x, b.x), um_max(a.y, b.y), um_max(a.z, b.z), um_max(a.w, b.w)); }
um_inline um_vec4 um_clamp4(um_vec4 a, um_vec4 minv, um_vec4 maxv) { return um_v4(um_clamp(a.x, minv.x, maxv.x), um_clamp(a.y, minv.y, maxv.y), um_clamp(a.z, minv.z, maxv.z), um_clamp(a.w, minv.w, maxv.w)); }
um_inline um_vec4 um_lerp4(um_vec4 a, um_vec4 b, float t) { return um_v4(um_lerp(a.x, b.x, t), um_lerp(a.y, b.y, t), um_lerp(a.z, b.z, t), um_lerp(a.w, b.w, t)); }
um_inline um_vec4 um_normalize4(um_vec4 a) { float v = um_length4(a); v = v >= FLT_MIN ? 1.0f / v : 0.0f; return um_v4(a.x * v, a.y * v, a.z * v, a.w * v); }
um_inline bool um_equal4(um_vec4 a, um_vec4 b) { return (a.x == b.x) & (a.y == b.y) & (a.z == b.z) & (a.w == b.w); }

um_inline um_quat um_quat_add(um_quat a, um_quat b) { return um_quat_xyzw(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
um_inline um_quat um_quat_sub(um_quat a, um_quat b) { return um_quat_xyzw(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
um_inline um_quat um_quat_mad(um_quat a, um_quat b, float c) { return um_quat_xyzw(a.x + b.x * c, a.y + b.y * c, a.z + b.z * c, a.w + b.w * c); }
um_inline um_quat um_quat_div(um_quat a, float b) { float v = 1.0f / b; return um_quat_xyzw(a.x * v, a.y * v, a.z * v, a.w * v); }
um_inline um_quat um_quat_neg(um_quat a) { return um_quat_xyzw(-a.x, -a.y, -a.z, -a.w); }
um_inline um_quat um_quat_inverse(um_quat a) { return um_quat_div(um_quat_xyzw(-a.x, -a.y, -a.z, a.w), (a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w)); }
um_inline um_quat um_quat_inverse_normalized(um_quat a) { return um_quat_xyzw(-a.x, -a.y, -a.z, a.w); }
um_inline float um_quat_dot(um_quat a, um_quat b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
um_inline float um_quat_length(um_quat a) { return um_sqrt(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w); }
um_inline um_quat um_quat_normalize(um_quat a) { float v = um_quat_length(a); v = v >= FLT_MIN ? 1.0f / v : 0.0f; return um_quat_xyzw(a.x * v, a.y * v, a.z * v, a.w * v); }
um_inline bool um_quat_equal(um_quat a, um_quat b) { return (a.x == b.x) & (a.y == b.y) & (a.z == b.z) & (a.w == b.w); }

um_abi um_quat um_quat_mul(um_quat a, um_quat b);
um_abi um_vec3 um_quat_rotate(um_quat a, um_vec3 b);
#define um_quat_mulrev(a, b) um_quat_mul((b), (a))

um_abi um_quat um_quat_lerp(um_quat a, um_quat b, float t);
um_abi um_quat um_quat_slerp(um_quat a, um_quat b, float t);

um_abi um_quat um_quat_axis_angle(um_vec3 axis, float radians);

#define um_mat_is_affine(a) um_equal4((a).cols[3], um_v4(0, 0, 0, 1))

um_abi um_mat um_mat_basis(um_vec3 x, um_vec3 y, um_vec3 z, um_vec3 origin);
um_abi um_mat um_mat_inverse_basis(um_vec3 x, um_vec3 y, um_vec3 z, um_vec3 origin);
um_abi um_mat um_mat_translate(um_vec3 offset);
um_abi um_mat um_mat_scale(um_vec3 scale);
um_abi um_mat um_mat_rotate(um_quat rotation);
um_abi um_mat um_mat_trs(um_vec3 translation, um_quat rotation, um_vec3 scale);
um_abi um_mat um_mat_rotate_x(float radians);
um_abi um_mat um_mat_rotate_y(float radians);
um_abi um_mat um_mat_rotate_z(float radians);
um_abi um_mat um_mat_look_at(um_vec3 eye, um_vec3 target, um_vec3 up_hint);
um_abi um_mat um_mat_perspective_gl(float fov, float aspect, float near_plane, float far_plane);
um_abi um_mat um_mat_perspective_d3d(float fov, float aspect, float near_plane, float far_plane);

um_abi float um_mat_determinant(um_mat a);
um_abi um_mat um_mat_inverse(um_mat a);
um_abi um_mat um_mat_transpose(um_mat a);

um_abi um_mat um_mat_mul(um_mat a, um_mat b);
um_abi um_vec4 um_mat_mull(um_vec4 a, um_mat b);
um_abi um_vec4 um_mat_mulr(um_mat a, um_vec4 b);
#define um_mat_mulrev(a, b) um_mat_mul((b), (a))

um_abi um_mat um_mat_add(um_mat a, um_mat b);
um_abi um_mat um_mat_sub(um_mat a, um_mat b);
um_abi um_mat um_mat_mad(um_mat a, um_mat b, float c);
um_abi um_mat um_mat_muls(um_mat a, float b);

um_abi um_vec3 um_transform_point(const um_mat *a, um_vec3 b);
um_abi um_vec3 um_transform_direction(const um_mat *a, um_vec3 b);
um_abi um_vec3 um_transform_extent(const um_mat *a, um_vec3 b);

#if defined(__cplusplus)
um_inline um_vec2 operator+(const um_vec2 &a, const um_vec2 &b) { return um_add2(a, b); }
um_inline um_vec2 operator-(const um_vec2 &a, const um_vec2 &b) { return um_sub2(a, b); }
um_inline um_vec2 operator*(const um_vec2 &a, const um_vec2 &b) { return um_mulv2(a, b); }
um_inline um_vec2 operator/(const um_vec2 &a, const um_vec2 &b) { return um_divv2(a, b); }
um_inline um_vec2 operator*(const um_vec2 &a, float b) { return um_mul2(a, b); }
um_inline um_vec2 operator/(const um_vec2 &a, float b) { return um_div2(a, b); }
um_inline um_vec2 operator-(const um_vec2 &a) { return um_neg2(a); }

um_inline um_vec3 operator+(const um_vec3 &a, const um_vec3 &b) { return um_add3(a, b); }
um_inline um_vec3 operator-(const um_vec3 &a, const um_vec3 &b) { return um_sub3(a, b); }
um_inline um_vec3 operator*(const um_vec3 &a, const um_vec3 &b) { return um_mulv3(a, b); }
um_inline um_vec3 operator/(const um_vec3 &a, const um_vec3 &b) { return um_divv3(a, b); }
um_inline um_vec3 operator*(const um_vec3 &a, float b) { return um_mul3(a, b); }
um_inline um_vec3 operator/(const um_vec3 &a, float b) { return um_div3(a, b); }
um_inline um_vec3 operator-(const um_vec3 &a) { return um_neg3(a); }

um_inline um_vec4 operator+(const um_vec4 &a, const um_vec4 &b) { return um_add4(a, b); }
um_inline um_vec4 operator-(const um_vec4 &a, const um_vec4 &b) { return um_sub4(a, b); }
um_inline um_vec4 operator*(const um_vec4 &a, const um_vec4 &b) { return um_mulv4(a, b); }
um_inline um_vec4 operator/(const um_vec4 &a, const um_vec4 &b) { return um_divv4(a, b); }
um_inline um_vec4 operator*(const um_vec4 &a, float b) { return um_mul4(a, b); }
um_inline um_vec4 operator/(const um_vec4 &a, float b) { return um_div4(a, b); }
um_inline um_vec4 operator-(const um_vec4 &a) { return um_neg4(a); }

um_inline um_quat operator+(const um_quat &a, const um_quat &b) { return um_quat_add(a, b); }
um_inline um_quat operator-(const um_quat &a, const um_quat &b) { return um_quat_sub(a, b); }
um_inline um_quat operator*(const um_quat &a, const um_quat &b) { return um_quat_mul(a, b); }

um_inline um_mat operator+(const um_mat &a, const um_mat &b) { return um_mat_add(a, b); }
um_inline um_mat operator-(const um_mat &a, const um_mat &b) { return um_mat_sub(a, b); }
um_inline um_mat operator*(const um_mat &a, const um_mat &b) { return um_mat_mul(a, b); }
#endif

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif

#endif

#if defined(UMATH_IMPLEMENTATION) || defined(__INTELLISENSE__)
#ifndef UMATH_H_IMPLEMENTED
#define UMATH_H_IMPLEMENTED

const um_mat um_mat_identity = {{{
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
}}};

um_abi um_quat um_quat_mul(um_quat a, um_quat b)
{
	return um_quat_xyzw(
		a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
		a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
		a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
		a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z);
}

um_abi um_vec3 um_quat_rotate(um_quat a, um_vec3 b)
{
	float xy = a.x*b.y - a.y*b.x;
	float xz = a.x*b.z - a.z*b.x;
	float yz = a.y*b.z - a.z*b.y;
	return um_v3(
		2.0f * (+ a.w*yz + a.y*xy + a.z*xz) + b.x,
		2.0f * (- a.x*xy - a.w*xz + a.z*yz) + b.y,
		2.0f * (- a.x*xz - a.y*yz + a.w*xy) + b.z);
}

um_abi um_quat um_quat_lerp(um_quat a, um_quat b, float t)
{
	float af = 1.0f - t, bf = t;
	float x = af*a.x + bf*b.x;
	float y = af*a.y + bf*b.y;
	float z = af*a.z + bf*b.z;
	float w = af*a.w + bf*b.w;
	return um_quat_xyzw(x, y, z, w);
}

um_abi um_quat um_quat_slerp(um_quat a, um_quat b, float t)
{
	float dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
	if (dot < 0.0f) {
		dot = -dot;
		b.x = -b.x; b.y = -b.y; b.z = -b.z; b.w = -b.w;
	}
	float omega = acosf(um_min(um_max(dot, 0.0f), 1.0f));
	if (omega <= FLT_MIN) return a;

	float rcp_so = 1.0f / sinf(omega);
	float af = sinf((1.0f - t) * omega) * rcp_so;
	float bf = sinf(t * omega) * rcp_so;

	float x = af*a.x + bf*b.x;
	float y = af*a.y + bf*b.y;
	float z = af*a.z + bf*b.z;
	float w = af*a.w + bf*b.w;
	return um_quat_normalize(um_quat_xyzw(x, y, z, w));
}

um_abi um_quat um_quat_axis_angle(um_vec3 axis, float radians)
{
	axis = um_normalize3(axis);
	float c = cosf(radians * 0.5f), s = sinf(radians * 0.5f);
	return um_quat_xyzw(axis.x * s, axis.y * s, axis.z * s, c);
}

um_abi um_mat um_mat_basis(um_vec3 x, um_vec3 y, um_vec3 z, um_vec3 origin)
{
	return um_mat_rows(
		x.x, y.x, z.x, origin.x,
		x.y, y.y, z.y, origin.y,
		x.z, y.z, z.z, origin.z,
		0, 0, 0, 1,
	);
}

um_abi um_mat um_mat_inverse_basis(um_vec3 x, um_vec3 y, um_vec3 z, um_vec3 origin)
{
	return um_mat_rows(
		x.x, x.y, x.z, -um_dot3(origin, x),
		y.x, y.y, y.z, -um_dot3(origin, y),
		z.x, z.y, z.z, -um_dot3(origin, z),
		0, 0, 0, 1);
}

um_abi um_mat um_mat_translate(um_vec3 offset)
{
	return um_mat_rows(
		1, 0, 0, offset.x,
		0, 1, 0, offset.y,
		0, 0, 1, offset.z,
		0, 0, 0, 1);
}

um_abi um_mat um_mat_scale(um_vec3 scale)
{
	return um_mat_rows(
		scale.x, 0, 0, 0,
		0, scale.y, 0, 0,
		0, 0, scale.z, 0,
		0, 0, 0, 1);
}

um_abi um_mat um_mat_rotate(um_quat rotation)
{
	um_quat q = rotation;
	float xx = q.x*q.x, xy = q.x*q.y, xz = q.x*q.z, xw = q.x*q.w;
	float yy = q.y*q.y, yz = q.y*q.z, yw = q.y*q.w;
	float zz = q.z*q.z, zw = q.z*q.w;
	return um_mat_rows(
		2.0f * (- yy - zz + 0.5f), 2.0f * (- zw + xy), 2.0f * (+ xz + yw), 0,
		2.0f * (+ xy + zw), 2.0f * (- xx - zz + 0.5f), 2.0f * (- xw + yz), 0,
		2.0f * (- yw + xz), 2.0f * (+ xw + yz), 2.0f * (- xx - yy + 0.5f), 0,
		0, 0, 0, 1,
	);
}

um_abi um_mat um_mat_trs(um_vec3 translation, um_quat rotation, um_vec3 scale)
{
	um_quat q = rotation;
	float xx = q.x*q.x, xy = q.x*q.y, xz = q.x*q.z, xw = q.x*q.w;
	float yy = q.y*q.y, yz = q.y*q.z, yw = q.y*q.w;
	float zz = q.z*q.z, zw = q.z*q.w;
	float sx = 2.0f * scale.x, sy = 2.0f * scale.y, sz = 2.0f * scale.z;
	return um_mat_rows(
		sx * (- yy - zz + 0.5f), sy * (- zw + xy), sz * (+ xz + yw), translation.x,
		sx * (+ xy + zw), sy * (- xx - zz + 0.5f), sz * (- xw + yz), translation.y,
		sx * (- yw + xz), sy * (+ xw + yz), sz * (- xx - yy + 0.5f), translation.z,
		0, 0, 0, 1,
	);
}

um_abi um_mat um_mat_rotate_x(float radians)
{
	float c = cosf(radians), s = sinf(radians);
	return um_mat_rows(
		1, 0, 0, 0,
		0, c, -s, 0,
		0, s, c, 0,
		0, 0, 0, 1,
	);
}

um_abi um_mat um_mat_rotate_y(float radians)
{
	float c = cosf(radians), s = sinf(radians);
	return um_mat_rows(
		c, 0, s, 0,
		0, 1, 0, 0,
		-s, 0, c, 0,
		0, 0, 0, 1,
	);
}

um_abi um_mat um_mat_rotate_z(float radians)
{
	float c = cosf(radians), s = sinf(radians);
	return um_mat_rows(
		c, -s, 0, 0,
		s, c, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	);
}

um_abi um_mat um_mat_look_at(um_vec3 eye, um_vec3 target, um_vec3 up_hint)
{
	um_vec3 dir = um_normalize3(um_sub3(target, eye));
	um_vec3 right = um_normalize3(um_cross3(dir, up_hint));
	um_vec3 up = um_normalize3(um_cross3(right, dir));
	return um_mat_inverse_basis(right, up, dir, eye);
}

um_abi um_mat um_mat_perspective_d3d(float fov, float aspect, float near_plane, float far_plane)
{
	float tan_fov = 1.0f / tanf(fov / 2.0f);
	float n = near_plane, f = far_plane;
	return um_mat_rows(
		tan_fov / aspect, 0, 0, 0,
		0, tan_fov, 0, 0,
		0, 0, f / (f-n), -(f*n)/(f-n),
		0, 0, 1, 0);
}

um_abi um_mat um_mat_perspective_gl(float fov, float aspect, float near_plane, float far_plane)
{
	float tan_fov = 1.0f / tanf(fov / 2.0f);
	float n = near_plane, f = far_plane;
	return um_mat_rows(
		tan_fov / aspect, 0, 0, 0,
		0, tan_fov, 0, 0,
		0, 0, (f+n) / (f-n), -2.0f * (f*n)/(f-n),
		0, 0, 1, 0);
}

um_abi float um_mat_determinant(um_mat a)
{
	if (um_mat_is_affine(a)) {
		return 
			- a.m14*a.m22*a.m41 + a.m12*a.m24*a.m41 + a.m14*a.m21*a.m42
			- a.m11*a.m24*a.m42 - a.m12*a.m21*a.m44 + a.m11*a.m22*a.m44;
	} else {
		return
			+ a.m14*a.m23*a.m32*a.m41 - a.m13*a.m24*a.m32*a.m41 - a.m14*a.m22*a.m33*a.m41 + a.m12*a.m24*a.m33*a.m41
			+ a.m13*a.m22*a.m34*a.m41 - a.m12*a.m23*a.m34*a.m41 - a.m14*a.m23*a.m31*a.m42 + a.m13*a.m24*a.m31*a.m42
			+ a.m14*a.m21*a.m33*a.m42 - a.m11*a.m24*a.m33*a.m42 - a.m13*a.m21*a.m34*a.m42 + a.m11*a.m23*a.m34*a.m42
			+ a.m14*a.m22*a.m31*a.m43 - a.m12*a.m24*a.m31*a.m43 - a.m14*a.m21*a.m32*a.m43 + a.m11*a.m24*a.m32*a.m43
			+ a.m12*a.m21*a.m34*a.m43 - a.m11*a.m22*a.m34*a.m43 - a.m13*a.m22*a.m31*a.m44 + a.m12*a.m23*a.m31*a.m44
			+ a.m13*a.m21*a.m32*a.m44 - a.m11*a.m23*a.m32*a.m44 - a.m12*a.m21*a.m33*a.m44 + a.m11*a.m22*a.m33*a.m44;
	}
}

um_abi um_mat um_mat_inverse(um_mat a)
{
	if (um_mat_is_affine(a)) {
		float det =
			- a.m13*a.m22*a.m31 + a.m12*a.m23*a.m31 + a.m13*a.m21*a.m32
			- a.m11*a.m23*a.m32 - a.m12*a.m21*a.m33 + a.m11*a.m22*a.m33;
		float rcp_det = 1.0f / det;

		return um_mat_rows(
			( - a.m23*a.m32 + a.m22*a.m33) * rcp_det,
			( + a.m13*a.m32 - a.m12*a.m33) * rcp_det,
			( - a.m13*a.m22 + a.m12*a.m23) * rcp_det,
			(a.m14*a.m23*a.m32 - a.m13*a.m24*a.m32 - a.m14*a.m22*a.m33 + a.m12*a.m24*a.m33 + a.m13*a.m22*a.m34 - a.m12*a.m23*a.m34) * rcp_det,

			( + a.m23*a.m31 - a.m21*a.m33) * rcp_det,
			( - a.m13*a.m31 + a.m11*a.m33) * rcp_det,
			( + a.m13*a.m21 - a.m11*a.m23) * rcp_det,
			(a.m13*a.m24*a.m31 - a.m14*a.m23*a.m31 + a.m14*a.m21*a.m33 - a.m11*a.m24*a.m33 - a.m13*a.m21*a.m34 + a.m11*a.m23*a.m34) * rcp_det,

			( - a.m22*a.m31 + a.m21*a.m32) * rcp_det,
			( + a.m12*a.m31 - a.m11*a.m32) * rcp_det,
			( - a.m12*a.m21 + a.m11*a.m22) * rcp_det,
			(a.m14*a.m22*a.m31 - a.m12*a.m24*a.m31 - a.m14*a.m21*a.m32 + a.m11*a.m24*a.m32 + a.m12*a.m21*a.m34 - a.m11*a.m22*a.m34) * rcp_det,

			0, 0, 0, 1
		);
	} else {
		float det =
			+ a.m14*a.m23*a.m32*a.m41 - a.m13*a.m24*a.m32*a.m41 - a.m14*a.m22*a.m33*a.m41 + a.m12*a.m24*a.m33*a.m41
			+ a.m13*a.m22*a.m34*a.m41 - a.m12*a.m23*a.m34*a.m41 - a.m14*a.m23*a.m31*a.m42 + a.m13*a.m24*a.m31*a.m42
			+ a.m14*a.m21*a.m33*a.m42 - a.m11*a.m24*a.m33*a.m42 - a.m13*a.m21*a.m34*a.m42 + a.m11*a.m23*a.m34*a.m42
			+ a.m14*a.m22*a.m31*a.m43 - a.m12*a.m24*a.m31*a.m43 - a.m14*a.m21*a.m32*a.m43 + a.m11*a.m24*a.m32*a.m43
			+ a.m12*a.m21*a.m34*a.m43 - a.m11*a.m22*a.m34*a.m43 - a.m13*a.m22*a.m31*a.m44 + a.m12*a.m23*a.m31*a.m44
			+ a.m13*a.m21*a.m32*a.m44 - a.m11*a.m23*a.m32*a.m44 - a.m12*a.m21*a.m33*a.m44 + a.m11*a.m22*a.m33*a.m44;
		float rcp_det = 1.0f / det;

		return um_mat_rows(
			(a.m23*a.m34*a.m42 - a.m24*a.m33*a.m42 + a.m24*a.m32*a.m43 - a.m22*a.m34*a.m43 - a.m23*a.m32*a.m44 + a.m22*a.m33*a.m44) * rcp_det,
			(a.m14*a.m33*a.m42 - a.m13*a.m34*a.m42 - a.m14*a.m32*a.m43 + a.m12*a.m34*a.m43 + a.m13*a.m32*a.m44 - a.m12*a.m33*a.m44) * rcp_det,
			(a.m13*a.m24*a.m42 - a.m14*a.m23*a.m42 + a.m14*a.m22*a.m43 - a.m12*a.m24*a.m43 - a.m13*a.m22*a.m44 + a.m12*a.m23*a.m44) * rcp_det,
			(a.m14*a.m23*a.m32 - a.m13*a.m24*a.m32 - a.m14*a.m22*a.m33 + a.m12*a.m24*a.m33 + a.m13*a.m22*a.m34 - a.m12*a.m23*a.m34) * rcp_det,

			(a.m24*a.m33*a.m41 - a.m23*a.m34*a.m41 - a.m24*a.m31*a.m43 + a.m21*a.m34*a.m43 + a.m23*a.m31*a.m44 - a.m21*a.m33*a.m44) * rcp_det,
			(a.m13*a.m34*a.m41 - a.m14*a.m33*a.m41 + a.m14*a.m31*a.m43 - a.m11*a.m34*a.m43 - a.m13*a.m31*a.m44 + a.m11*a.m33*a.m44) * rcp_det,
			(a.m14*a.m23*a.m41 - a.m13*a.m24*a.m41 - a.m14*a.m21*a.m43 + a.m11*a.m24*a.m43 + a.m13*a.m21*a.m44 - a.m11*a.m23*a.m44) * rcp_det,
			(a.m13*a.m24*a.m31 - a.m14*a.m23*a.m31 + a.m14*a.m21*a.m33 - a.m11*a.m24*a.m33 - a.m13*a.m21*a.m34 + a.m11*a.m23*a.m34) * rcp_det,

			(a.m22*a.m34*a.m41 - a.m24*a.m32*a.m41 + a.m24*a.m31*a.m42 - a.m21*a.m34*a.m42 - a.m22*a.m31*a.m44 + a.m21*a.m32*a.m44) * rcp_det,
			(a.m14*a.m32*a.m41 - a.m12*a.m34*a.m41 - a.m14*a.m31*a.m42 + a.m11*a.m34*a.m42 + a.m12*a.m31*a.m44 - a.m11*a.m32*a.m44) * rcp_det,
			(a.m12*a.m24*a.m41 - a.m14*a.m22*a.m41 + a.m14*a.m21*a.m42 - a.m11*a.m24*a.m42 - a.m12*a.m21*a.m44 + a.m11*a.m22*a.m44) * rcp_det,
			(a.m14*a.m22*a.m31 - a.m12*a.m24*a.m31 - a.m14*a.m21*a.m32 + a.m11*a.m24*a.m32 + a.m12*a.m21*a.m34 - a.m11*a.m22*a.m34) * rcp_det,

			(a.m23*a.m32*a.m41 - a.m22*a.m33*a.m41 - a.m23*a.m31*a.m42 + a.m21*a.m33*a.m42 + a.m22*a.m31*a.m43 - a.m21*a.m32*a.m43) * rcp_det,
			(a.m12*a.m33*a.m41 - a.m13*a.m32*a.m41 + a.m13*a.m31*a.m42 - a.m11*a.m33*a.m42 - a.m12*a.m31*a.m43 + a.m11*a.m32*a.m43) * rcp_det,
			(a.m13*a.m22*a.m41 - a.m12*a.m23*a.m41 - a.m13*a.m21*a.m42 + a.m11*a.m23*a.m42 + a.m12*a.m21*a.m43 - a.m11*a.m22*a.m43) * rcp_det,
			(a.m12*a.m23*a.m31 - a.m13*a.m22*a.m31 + a.m13*a.m21*a.m32 - a.m11*a.m23*a.m32 - a.m12*a.m21*a.m33 + a.m11*a.m22*a.m33) * rcp_det,
		);
	}
}

um_abi um_mat um_mat_transpose(um_mat a)
{
	return um_mat_rows(
		a.m11, a.m21, a.m31, a.m41,
		a.m12, a.m22, a.m32, a.m42,
		a.m13, a.m23, a.m33, a.m43,
		a.m14, a.m24, a.m34, a.m44,
	);
}

um_abi um_mat um_mat_mul(um_mat a, um_mat b)
{
	return um_mat_rows(
		a.m11*b.m11 + a.m12*b.m21 + a.m13*b.m31 + a.m14*b.m41,
		a.m11*b.m12 + a.m12*b.m22 + a.m13*b.m32 + a.m14*b.m42,
		a.m11*b.m13 + a.m12*b.m23 + a.m13*b.m33 + a.m14*b.m43,
		a.m11*b.m14 + a.m12*b.m24 + a.m13*b.m34 + a.m14*b.m44,
		a.m21*b.m11 + a.m22*b.m21 + a.m23*b.m31 + a.m24*b.m41,
		a.m21*b.m12 + a.m22*b.m22 + a.m23*b.m32 + a.m24*b.m42,
		a.m21*b.m13 + a.m22*b.m23 + a.m23*b.m33 + a.m24*b.m43,
		a.m21*b.m14 + a.m22*b.m24 + a.m23*b.m34 + a.m24*b.m44,
		a.m31*b.m11 + a.m32*b.m21 + a.m33*b.m31 + a.m34*b.m41,
		a.m31*b.m12 + a.m32*b.m22 + a.m33*b.m32 + a.m34*b.m42,
		a.m31*b.m13 + a.m32*b.m23 + a.m33*b.m33 + a.m34*b.m43,
		a.m31*b.m14 + a.m32*b.m24 + a.m33*b.m34 + a.m34*b.m44,
		a.m41*b.m11 + a.m42*b.m21 + a.m43*b.m31 + a.m44*b.m41,
		a.m41*b.m12 + a.m42*b.m22 + a.m43*b.m32 + a.m44*b.m42,
		a.m41*b.m13 + a.m42*b.m23 + a.m43*b.m33 + a.m44*b.m43,
		a.m41*b.m14 + a.m42*b.m24 + a.m43*b.m34 + a.m44*b.m44,
	);
}

um_abi um_mat um_mat_add(um_mat a, um_mat b)
{
	return um_mat_rows(
		a.m11 + b.m11, a.m12 + b.m12, a.m13 + b.m13, a.m14 + b.m14,
		a.m21 + b.m21, a.m22 + b.m22, a.m23 + b.m23, a.m24 + b.m24,
		a.m31 + b.m31, a.m32 + b.m32, a.m33 + b.m33, a.m34 + b.m34,
		a.m41 + b.m41, a.m42 + b.m42, a.m43 + b.m43, a.m44 + b.m44,
	);
}

um_abi um_mat um_mat_sub(um_mat a, um_mat b)
{
	return um_mat_rows(
		a.m11 - b.m11, a.m12 - b.m12, a.m13 - b.m13, a.m14 - b.m14,
		a.m21 - b.m21, a.m22 - b.m22, a.m23 - b.m23, a.m24 - b.m24,
		a.m31 - b.m31, a.m32 - b.m32, a.m33 - b.m33, a.m34 - b.m34,
		a.m41 - b.m41, a.m42 - b.m42, a.m43 - b.m43, a.m44 - b.m44,
	);
}

um_abi um_mat um_mat_mad(um_mat a, um_mat b, float c)
{
	return um_mat_rows(
		a.m11 + b.m11 * c, a.m12 + b.m12 * c, a.m13 + b.m13 * c, a.m14 + b.m14 * c,
		a.m21 + b.m21 * c, a.m22 + b.m22 * c, a.m23 + b.m23 * c, a.m24 + b.m24 * c,
		a.m31 + b.m31 * c, a.m32 + b.m32 * c, a.m33 + b.m33 * c, a.m34 + b.m34 * c,
		a.m41 + b.m41 * c, a.m42 + b.m42 * c, a.m43 + b.m43 * c, a.m44 + b.m44 * c,
	);
}

um_abi um_mat um_mat_muls(um_mat a, float b)
{
	return um_mat_rows(
		a.m11 * b, a.m12 * b, a.m13 * b, a.m14 * b,
		a.m21 * b, a.m22 * b, a.m23 * b, a.m24 * b,
		a.m31 * b, a.m32 * b, a.m33 * b, a.m34 * b,
		a.m41 * b, a.m42 * b, a.m43 * b, a.m44 * b,
	);
}

um_abi um_vec4 um_mat_mull(um_vec4 a, um_mat b)
{
	return um_v4(
		a.x*b.m11 + a.y*b.m21 + a.z*b.m31 + a.w*b.m41,
		a.x*b.m12 + a.y*b.m22 + a.z*b.m32 + a.w*b.m42,
		a.x*b.m13 + a.y*b.m23 + a.z*b.m33 + a.w*b.m43,
		a.x*b.m14 + a.y*b.m24 + a.z*b.m34 + a.w*b.m44);
}

um_abi um_vec4 um_mat_mulr(um_mat a, um_vec4 b)
{
	return um_v4(
		a.m11*b.x + a.m12*b.y + a.m13*b.z + a.m14*b.w,
		a.m21*b.x + a.m22*b.y + a.m23*b.z + a.m24*b.w,
		a.m31*b.x + a.m32*b.y + a.m33*b.z + a.m34*b.w,
		a.m41*b.x + a.m42*b.y + a.m43*b.z + a.m44*b.w);
}

um_abi um_vec3 um_transform_point(const um_mat *a, um_vec3 b)
{
	return um_v3(
		a->m11*b.x + a->m12*b.y + a->m13*b.z + a->m14,
		a->m21*b.x + a->m22*b.y + a->m23*b.z + a->m24,
		a->m31*b.x + a->m32*b.y + a->m33*b.z + a->m34);
}

um_abi um_vec3 um_transform_direction(const um_mat *a, um_vec3 b)
{
	return um_v3(
		a->m11*b.x + a->m12*b.y + a->m13*b.z,
		a->m21*b.x + a->m22*b.y + a->m23*b.z,
		a->m31*b.x + a->m32*b.y + a->m33*b.z);
}

um_abi um_vec3 um_transform_extent(const um_mat *a, um_vec3 b)
{
	return um_v3(
		um_abs(a->m11)*b.x + um_abs(a->m12)*b.y + um_abs(a->m13)*b.z,
		um_abs(a->m21)*b.x + um_abs(a->m22)*b.y + um_abs(a->m23)*b.z,
		um_abs(a->m31)*b.x + um_abs(a->m32)*b.y + um_abs(a->m33)*b.z);
}

#endif
#endif
