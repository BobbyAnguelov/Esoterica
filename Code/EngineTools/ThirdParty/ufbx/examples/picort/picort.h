#pragma once

#include <stddef.h>
#include <stdint.h>
#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include "../../ufbx.h"

// -- Configuration

#if defined(_MSC_VER)
	#define picort_forceinline inline __forceinline
#elif defined(__GNUC__)
	#define picort_forceinline inline __attribute__((always_inline))
#else
	#define picort_forceinline inline
#endif


// -- Math

using std::sin;
using std::cos;
using std::atan2;
using std::acos;
using std::asin;
using std::abs;

using Real = float;
static const constexpr Real Inf = INFINITY;
static const constexpr Real Pi = (Real)3.14159265359;

struct Vec2
{
	Real x = 0.0f, y = 0.0f;

	picort_forceinline Vec2 operator-() const { return { -x, -y }; };
	picort_forceinline Vec2 operator+(const Vec2 &b) const { return { x + b.x, y + b.y }; }
	picort_forceinline Vec2 operator-(const Vec2 &b) const { return { x - b.x, y - b.y }; }
	picort_forceinline Vec2 operator*(const Vec2 &b) const { return { x * b.x, y * b.y }; }
	picort_forceinline Vec2 operator/(const Vec2 &b) const { return { x / b.x, y / b.y }; }
	picort_forceinline Vec2 operator*(Real b) const { return { x * b, y * b }; }
	picort_forceinline Vec2 operator/(Real b) const { return { x / b, y / b }; }

	picort_forceinline Real operator[](int axis) const { return (&x)[axis]; }
};

struct Vec3
{
	Real x = 0.0f, y = 0.0f, z = 0.0f;

	picort_forceinline Vec3 operator-() const { return { -x, -y, -z }; };
	picort_forceinline Vec3 operator+(const Vec3 &b) const { return { x + b.x, y + b.y, z + b.z }; }
	picort_forceinline Vec3 operator-(const Vec3 &b) const { return { x - b.x, y - b.y, z - b.z }; }
	picort_forceinline Vec3 operator*(const Vec3 &b) const { return { x * b.x, y * b.y, z * b.z }; }
	picort_forceinline Vec3 operator/(const Vec3 &b) const { return { x / b.x, y / b.y, z / b.z }; }
	picort_forceinline Vec3 operator*(Real b) const { return { x * b, y * b, z * b }; }
	picort_forceinline Vec3 operator/(Real b) const { return { x / b, y / b, z / b }; }

	picort_forceinline Real operator[](int axis) const { return (&x)[axis]; }

	picort_forceinline bool operator==(const Vec3 &rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
	picort_forceinline bool operator!=(const Vec3 &rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z; }
};

struct Vec4
{
	Real x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;

	picort_forceinline Vec4 operator-() const { return { -x, -y, -z, -w }; };
	picort_forceinline Vec4 operator+(const Vec4 &b) const { return { x + b.x, y + b.y, z + b.z, w + b.w }; }
	picort_forceinline Vec4 operator-(const Vec4 &b) const { return { x - b.x, y - b.y, z - b.z, w - b.w }; }
	picort_forceinline Vec4 operator*(const Vec4 &b) const { return { x * b.x, y * b.y, z * b.z, w * b.w }; }
	picort_forceinline Vec4 operator/(const Vec4 &b) const { return { x / b.x, y / b.y, z / b.z, w / b.w }; }
	picort_forceinline Vec4 operator*(Real b) const { return { x * b, y * b, z * b, w * b }; }
	picort_forceinline Vec4 operator/(Real b) const { return { x / b, y / b, z / b, w / b }; }

	picort_forceinline Real operator[](int axis) const { return (&x)[axis]; }

	picort_forceinline bool operator==(const Vec4 &rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
	picort_forceinline bool operator!=(const Vec4 &rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z || w == rhs.w; }
};

picort_forceinline Real min(Real a, Real b) { return a < b ? a : b; }
picort_forceinline Real max(Real a, Real b) { return b < a ? a : b; }
picort_forceinline Real floor_real(Real a) { return std::floor(a); }
picort_forceinline Real ceil_real(Real a) { return std::ceil(a); }

picort_forceinline Vec2 min(const Vec2 &a, const Vec2 &b) { return { min(a.x, b.x), min(a.y, b.y) }; }
picort_forceinline Vec2 max(const Vec2 &a, const Vec2 &b) { return { max(a.x, b.x), max(a.y, b.y) }; }

picort_forceinline Vec3 min(const Vec3 &a, const Vec3 &b) { return { min(a.x, b.x), min(a.y, b.y), min(a.z, b.z) }; }
picort_forceinline Vec3 max(const Vec3 &a, const Vec3 &b) { return { max(a.x, b.x), max(a.y, b.y), max(a.z, b.z) }; }

picort_forceinline Vec4 min(const Vec4 &a, const Vec4 &b) { return { min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w) }; }
picort_forceinline Vec4 max(const Vec4 &a, const Vec4 &b) { return { max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w) }; }

picort_forceinline Real min_component(const Vec2 &a) { return min(min(a.x, a.y), +Inf); }
picort_forceinline Real max_component(const Vec2 &a) { return max(max(a.x, a.y), -Inf); }

picort_forceinline Real min_component(const Vec3 &a) { return min(min(min(a.x, a.y), a.z), +Inf); }
picort_forceinline Real max_component(const Vec3 &a) { return max(max(max(a.x, a.y), a.z), -Inf); }

picort_forceinline Real min_component(const Vec4 &a) { return min(min(min(min(a.x, a.y), a.z), a.w), +Inf); }
picort_forceinline Real max_component(const Vec4 &a) { return max(max(max(max(a.x, a.y), a.z), a.w), -Inf); }

picort_forceinline Vec2 abs(const Vec2 &a) { return { abs(a.x), abs(a.y) }; }
picort_forceinline Vec3 abs(const Vec3 &a) { return { abs(a.x), abs(a.y), abs(a.z) }; }
picort_forceinline Vec4 abs(const Vec4 &a) { return { abs(a.x), abs(a.y), abs(a.z), abs(a.w) }; }

picort_forceinline Real dot(const Vec2 &a, const Vec2 &b) { return a.x*b.x + a.y*b.y; }
picort_forceinline Real dot(const Vec3 &a, const Vec3 &b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
picort_forceinline Real dot(const Vec4 &a, const Vec4 &b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
picort_forceinline Vec3 cross(const Vec3 &a, const Vec3 &b) {
	return { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
}

picort_forceinline Real length(const Vec2 &a) { return sqrt(dot(a, a)); }
picort_forceinline Real length(const Vec3 &a) { return sqrt(dot(a, a)); }
picort_forceinline Real length(const Vec4 &a) { return sqrt(dot(a, a)); }

picort_forceinline Vec2 normalize(const Vec2 &a) { return a / length(a); }
picort_forceinline Vec3 normalize(const Vec3 &a) { return a / length(a); }
picort_forceinline Vec4 normalize(const Vec4 &a) { return a / length(a); }

picort_forceinline Real lerp(Real a, Real b, Real t) { return a * (1.0f - t) + b * t; }
picort_forceinline Vec2 lerp(const Vec2 &a, const Vec2 &b, Real t) { return a * (1.0f - t) + b * t; }
picort_forceinline Vec3 lerp(const Vec3 &a, const Vec3 &b, Real t) { return a * (1.0f - t) + b * t; }
picort_forceinline Vec4 lerp(const Vec4 &a, const Vec4 &b, Real t) { return a * (1.0f - t) + b * t; }

picort_forceinline int largest_axis(const Vec3 &v) {
	Real m = max_component(v);
	return v.x == m ? 0 : v.y == m ? 1 : 2;
}

picort_forceinline Vec3 reflect(const Vec3 &n, const Vec3 &v) { return -v + n * (2.0f * dot(n, v)); }

picort_forceinline Vec2 vec2(float v) { return Vec2{ v, v }; }
picort_forceinline Vec3 vec3(float v) { return Vec3{ v, v, v }; }
picort_forceinline Vec4 vec4(float v) { return Vec4{ v, v, v, v }; }

picort_forceinline Real sqrt_safe(Real a) { return sqrt(a); }

picort_forceinline Real clamp(Real a, Real min_v, Real max_v) { return min(max(a, min_v), max_v); }

struct Bounds
{
	Vec3 min = { +Inf, +Inf, +Inf };
	Vec3 max = { -Inf, -Inf, -Inf };

	Vec3 center() const { return (min + max) * 0.5f; }
	Vec3 diagonal() const { return max - min; }
	Real area() const {
		Vec3 d = max - min;
		return 2.0f * (d.x*d.y + d.y*d.z + d.z*d.x);
	}

	void add(const Bounds &b) {
		min = ::min(min, b.min);
		max = ::max(max, b.max);
	}
};

// -- Color spaces

inline Real linear_to_srgb(Real a) {
	a = clamp(a, 0.0f, 1.0f);
	return a <= 0.0031308f ? 12.92f * a : 1.055f*pow(a, (1.0f / 2.4f)) - 0.055f;
}

inline Real srgb_to_linear(Real a) {
	a = clamp(a, 0.0f, 1.0f);
	return a <= 0.04045f ? 0.0773993808f * a : pow(a*0.947867298578f+0.0521327014218f, 2.4f);
}

inline Vec3 linear_to_srgb(Vec3 a) { return Vec3{ linear_to_srgb(a.x), linear_to_srgb(a.y), linear_to_srgb(a.z) }; }
inline Vec3 srgb_to_linear(Vec3 a) { return Vec3{ srgb_to_linear(a.x), srgb_to_linear(a.y), srgb_to_linear(a.z) }; }
inline Vec4 linear_to_srgb(Vec4 a) { return Vec4{ linear_to_srgb(a.x), linear_to_srgb(a.y), linear_to_srgb(a.z), a.w }; }
inline Vec4 srgb_to_linear(Vec4 a) { return Vec4{ srgb_to_linear(a.x), srgb_to_linear(a.y), srgb_to_linear(a.z), a.w }; }

// -- Random series

// *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)
struct Random
{
	uint64_t state = 1, inc = 1;

	uint32_t next() {
		uint64_t oldstate = state;
		// Advance internal state
		state = oldstate * 6364136223846793005ULL + inc;
		// Calculate output function (XSH RR), uses old state for max ILP
		uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
		uint32_t rot = oldstate >> 59u;
		return (xorshifted >> rot) | (xorshifted << ((0-rot) & 31));
	}
};

inline Real uniform_real(Random &rng) { return (Real)rng.next() * (Real)2.3283064e-10; }
inline Vec2 uniform_vec2(Random &rng) { return { uniform_real(rng), uniform_real(rng) }; }
inline Vec3 uniform_vec3(Random &rng) { return { uniform_real(rng), uniform_real(rng), uniform_real(rng) }; }

// -- Images

struct alignas(4) Pixel {
	uint8_t r=0, g=0, b=0, a=0xff;
};

struct alignas(8) Pixel16 {
	uint16_t r=0, g=0, b=0, a=0xffff;

	bool operator==(const Pixel16 &p) const { return r==p.r && g==p.g && b==p.b && a==p.a; }
	bool operator!=(const Pixel16 &p) const { return !(*this == p); }
};

struct Image
{
	uint32_t width = 0, height = 0;
	std::vector<Pixel16> pixels;
	bool srgb = false;
};

struct ImageResult
{
	const char *error;
	Image image;
};


// Fetch a single pixel from `image` at pixel coordinate `{x, y}`
picort_forceinline Vec4 fetch_image(const Image &image, int32_t x, int32_t y)
{
	if (((image.width & (image.width - 1)) | (image.height & (image.height - 1))) == 0) {
		x &= image.width - 1;
		y &= image.height - 1;
	} else {
		x = ((x % image.width) + image.width) % image.width;
		y = ((y % image.height) + image.height) % image.height;
	}
	Pixel16 p = image.pixels[y * image.width + x];
	Vec4 v { (Real)p.r, (Real)p.g, (Real)p.b, (Real)p.a };
	v = v * (Real)(1.0 / 65535.0);
	if (image.srgb) {
		v = srgb_to_linear(v);
	}
	return v;
}

// Bilinearly sample `image` at normalized coordinate `uv`
inline Vec4 sample_image(const Image &image, const Vec2 &uv)
{
	if (image.width == 0 || image.height == 0) return { };
	Vec2 px = uv * Vec2{ (Real)image.width, -(Real)image.height };
	Real x = floor_real(px.x), y = floor_real(px.y), dx = px.x - x, dy = px.y - y;
	Vec4 p00 = fetch_image(image, (int32_t)x + 0, (int32_t)y + 0);
	Vec4 p10 = fetch_image(image, (int32_t)x + 1, (int32_t)y + 0);
	Vec4 p01 = fetch_image(image, (int32_t)x + 0, (int32_t)y + 1);
	Vec4 p11 = fetch_image(image, (int32_t)x + 1, (int32_t)y + 1);
	Vec4 v = lerp(lerp(p00, p10, dx), lerp(p01, p11, dx), dy);
	return v;
}

ImageResult read_png(const void *data, size_t size);
ImageResult load_png(const char *filename);
std::vector<uint8_t> write_png(const void *data, uint32_t width, uint32_t height);
void save_png(const char *filename, const void *data, uint32_t width, uint32_t height, int frame);

// -- General utilities

inline bool ends_with(const std::string &str, const char *suffix)
{
	size_t len = strlen(suffix);
	return str.size() >= len && !str.compare(str.size() - len, len, suffix);
}

// -- Ray tracing

struct Triangle
{
	Vec3 v[3];
	size_t index = 0;

	Bounds bounds() const { return { min(min(v[0], v[1]), v[2]), max(max(v[0], v[1]), v[2]) }; }
};

struct BVH
{
	Bounds child_bounds[2];
	std::unique_ptr<BVH[]> child_nodes;
	std::vector<Triangle> triangles;
};

struct Ray
{
	Vec3 origin;
	Vec3 direction;
};

struct RayHit
{
	Real t = Inf, u = 0.0f, v = 0.0f;
	size_t index = SIZE_MAX;
	size_t steps = 0;
};

struct RayTrace
{
	struct Axes { int x = -1, y = -1, z = -1; };

	Ray ray;
	Vec3 rcp_direction;

	Axes axes;
	Vec3 shear;

	Real max_t = Inf;
	RayHit hit;
};

struct Framebuffer
{
	std::vector<Pixel> pixels;
	uint32_t width = 0, height = 0;

	Framebuffer() { }
	Framebuffer(uint32_t width, uint32_t height)
		: pixels(width * height), width(width), height(height) { }
};

BVH build_bvh(Triangle *triangles, size_t count);
RayTrace setup_trace(const Ray &ray, Real max_t=Inf);
void intersect_bvh(RayTrace &trace, const BVH &root);

// -- Sampling

template <int Base>
inline Real radical_inverse(uint64_t a, Real offset) {
    const Real inv_base = 1.0f / (Real)Base;
    uint64_t rev_digits = 0;
    Real inv_base_n = 1.0f;
    while (a) {
        uint64_t next  = a / Base;
        uint64_t digit = a - next * Base;
        rev_digits = rev_digits * Base + digit;
        inv_base_n *= inv_base;
        a = next;
    }
    return fmod((Real)(rev_digits * inv_base_n + offset), 1.0f);
}

inline Vec2 halton_vec2(uint64_t a, const Vec2 &offset=Vec2{})
{
	return {
		radical_inverse<2>(a, offset.x),
		radical_inverse<3>(a, offset.y),
	};
}

inline Vec3 halton_vec3(uint64_t a, const Vec3 &offset=Vec3{})
{
	return {
		radical_inverse<2>(a, offset.x),
		radical_inverse<3>(a, offset.y),
		radical_inverse<5>(a, offset.z),
	};
}

inline Vec3 cosine_hemisphere_sample(const Vec2 &uv)
{
	Real r = sqrt(uv.x);
	Real t = 2.0f*Pi*uv.y;
	return { r * cos(t), r * sin(t), sqrt_safe(1.0f - r) };
}

inline Vec2 uniform_disk_sample(const Vec2 &uv)
{
	Real r = sqrt(uv.x);
	Real t = 2.0f*Pi*uv.y;
	return { r * cos(t), r * sin(t) };
}

// Probability distribution for `cosine_hemisphere_sample()`
inline Real cosine_hemisphere_pdf(const Vec3 &wi)
{
	return wi.z / Pi;
}

inline Vec3 uniform_sphere_sample(const Vec2 &uv)
{
	Real t = 2.0f*Pi*uv.x;
	Real p = 2.0f*uv.y - 1.0f, q = sqrt_safe(1.0f - p*p);
	return { q * cos(t), q * sin(t), p };
}

struct Basis
{
	Vec3 x, y, z;

	Vec3 to_basis(const Vec3 &v) const { return { dot(x, v), dot(y, v), dot(z, v) }; }
	Vec3 to_world(const Vec3 &v) const { return x*v.x + y*v.y + z*v.z; }
};

inline Basis basis_normal(const Vec3 &axis_z)
{
	Vec3 z = normalize(axis_z);
	Real t = sqrt(z.x*z.x + z.y*z.y);
	Vec3 x = t > 0.0f ? Vec3{-z.y/t, z.x/t, 0.0f} : Vec3{1.0f, 0.0f, 0.0f};
	Vec3 y = normalize(cross(z, x));
	return { x, y, z };
}

// -- GUI

struct DebugTracerBase
{
	virtual ~DebugTracerBase() { }
	virtual Vec3 trace(Vec2 uv) = 0;
};

void enable_gui(const Framebuffer *fb);
void disable_gui(Framebuffer &&fb, std::unique_ptr<DebugTracerBase> tracer);
void close_gui();
void wait_gui();

// -- Options

struct alignas(8) OptBase
{
	const char *name, *desc, *alias;
	uint32_t num_args, size;
	bool defined = false;
	bool from_arg = false;

	OptBase(const char *name, const char *desc, const char *alias, uint32_t num_args, uint32_t size)
		: name(name), desc(desc), alias(alias), num_args(num_args), size(size) { }

	virtual void parse(const char **args) = 0;
	virtual int print(char *buf, size_t size) = 0;
};

template <typename T>
struct OptTraits { };

template <> struct OptTraits<bool> {
	enum { num_args = 0 };
	static bool parse(const char **args) { return true; }
	static int print(bool v, char *buf, size_t size) { return snprintf(buf, size, "%s", v ? "true" : "false"); }
};

template <> struct OptTraits<Real> {
	enum { num_args = 1 };
	static Real parse(const char **args) { return (Real)strtod(args[0], NULL); }
	static int print(Real v, char *buf, size_t size) { return snprintf(buf, size, "%g", v); }
};

template <> struct OptTraits<int> {
	enum { num_args = 1 };
	static int parse(const char **args) { return atoi(args[0]); }
	static int print(int v, char *buf, size_t size) { return snprintf(buf, size, "%d", v); }
};

template <> struct OptTraits<double> {
	enum { num_args = 1 };
	static double parse(const char **args) { return (double)strtod(args[0], NULL); }
	static int print(double v, char *buf, size_t size) { return snprintf(buf, size, "%g", v); }
};

template <> struct OptTraits<Vec2> {
	enum { num_args = 2 };
	static Vec2 parse(const char **args) {
		return { (Real)strtod(args[0], NULL), (Real)strtod(args[1], NULL) };
	}
	static int print(const Vec2 &v, char *buf, size_t size) { return snprintf(buf, size, "(%g, %g)", v.x, v.y); }
};

template <> struct OptTraits<Vec3> {
	enum { num_args = 3 };
	static Vec3 parse(const char **args) {
		return { (Real)strtod(args[0], NULL), (Real)strtod(args[1], NULL), (Real)strtod(args[2], NULL) };
	}
	static int print(const Vec3 &v, char *buf, size_t size) { return snprintf(buf, size, "(%g, %g, %g)", v.x, v.y, v.z); }
};

template <> struct OptTraits<std::string> {
	enum { num_args = 1 };
	static std::string parse(const char **args) { return { args[0] }; }
	static int print(const std::string &v, char *buf, size_t size) { return snprintf(buf, size, "%s", v.c_str()); }
};

struct StringPair { std::string v[2]; };

template <> struct OptTraits<StringPair> {
	enum { num_args = 2 };
	static StringPair parse(const char **args) { return { { { args[0] }, { args[1] } } }; }
	static int print(const StringPair &v, char *buf, size_t size) { return snprintf(buf, size, "(%s, %s)", v.v[0].c_str(), v.v[1].c_str()); }
};

template <typename T, typename Traits=OptTraits<T>>
struct Opt : OptBase
{
	T value;
	Opt(const char *name, const char *desc, T def = T{}, const char *alias=nullptr) : OptBase(name, desc, alias, Traits::num_args, sizeof(*this)), value(def) { }
	virtual void parse(const char **args) override {
		value = Traits::parse(args);
	}
	virtual int print(char *buf, size_t size) override {
		return Traits::print(value, buf, size);
	}
};

struct OptIter
{
	OptBase *ptr;
	OptIter(void *ptr) : ptr((OptBase*)ptr) { }
	OptIter &operator++() { ptr = (OptBase*)((char*)ptr + ptr->size); return *this; }
	OptIter operator++(int) { OptIter it = *this; ptr = (OptBase*)((char*)ptr + ptr->size); return it; }
	OptBase &operator*() const { return *ptr; }
	OptBase *operator->() const { return ptr; }
	bool operator==(const OptIter &rhs) const { return ptr == rhs.ptr; }
	bool operator!=(const OptIter &rhs) const { return ptr != rhs.ptr; }
};

struct Opts
{
	Opt<bool> help { "help", "Display this help", false, "h" };
	Opt<std::string> input { "input", "Input .fbx file (does not need -i if the path doesn't start with a '-')", "", "i" };
	Opt<std::string> output { "output", "Output .bmp file (defaults to 'picort-output.bmp')", "picort-output.bmp", "o" };
	Opt<bool> verbose { "verbose", "Enable verbose output", false, "v" };
	Opt<std::string> camera { "camera", "Camera name (defaults to 'picort_camera')", "picort_camera" };
	Opt<int> samples { "samples", "Samples to use while rendering", 64 };
	Opt<int> bounces { "bounces", "Number of indirect bounces", 1 };
	Opt<int> threads { "threads", "Threads to use while rendering (0 for auto-detect)" };
	Opt<std::string> animation { "animation", "Name of the animation to use" };
	Opt<double> frame { "frame", "Time in frames (can be fractional)", 1 };
	Opt<int> num_frames { "num-frames", "Number of frames to render (starting from --frame)", 1 };
	Opt<int> frame_offset { "frame-offset", "Frame number to start rendering from", 0 };
	Opt<double> fps { "fps", "Frames per second for --num-frames (overrides one set in FBX file)" };
	Opt<Vec2> resolution { "resolution", "Resolution", { 512.0f, 512.0f } };
	Opt<double> resolution_scale { "resolution-scale", "Scale factor to resolution", 1.0 };
	Opt<bool> gui { "gui", "Show a GUI window" };
	Opt<bool> keep_open { "keep-open", "Keep the GUI open" };
	Opt<double> time { "time", "Time in seconds" };
	Opt<Real> scene_scale { "scene-scale", "Global scene scale", 1.0f };
	Opt<Vec3> camera_position { "camera-position", "World space camera position", { 0.0f, 0.0f, 10.0f } };
	Opt<Vec3> camera_direction { "camera-direction", "World space camera direction", { 0.0f, 0.0f, -1.0f } };
	Opt<Vec3> camera_target { "camera-target", "World space camera target", { 0.0f, 0.0f, 0.0f } };
	Opt<Vec3> camera_up { "camera-direction", "World space camera up direction", { 0.0f, 1.0f, 0.0f } };
	Opt<Real> camera_fov { "camera-fov", "Camera field of view in degrees", 60.0f };
	Opt<std::string> sky { "sky", "Sky .png file" };
	Opt<Real> sky_exposure { "sky-exposure", "Exposure of the sky", -Inf };
	Opt<Real> sky_rotation { "sky-rotation", "Rotation of the sky in degrees" };
	Opt<Real> exposure { "exposure", "Exposure for the camera", 3.0f };
	Opt<Real> indirect_clamp { "indirect-clamp", "Clamping for indirect samples", 10.0f };
	Opt<std::string> base_path { "base-path", "Base directory to look up files from" };
	Opt<StringPair> compare { "compare", "Compare two result images" };
	Opt<std::string> reference { "reference", "Compare the resulting image to a reference" };
	Opt<double> error_threshold { "error-threshold", "Error threshold (MSE) for comparison", 0.05 };
	Opt<bool> bvh_heatmap { "bvh-heatmap", "Visualize BVH heatmap" };

	OptIter begin() { return this; }
	OptIter end() { return this + 1; }
};

Opts setup_opts(int argc, char **argv);
std::string get_path(const Opts &opts, const Opt<std::string> &opt);
void compare_images(Opts &opts, const char *path_a, const char *path_b);

extern bool g_verbose;

static void verbosef(const char *fmt, ...)
{
	if (!g_verbose) return;

	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}
