#define SOKOL_IMPL

#if defined(__APPLE__)
	#define SOKOL_METAL
#elif defined(_WIN32)
	#define SOKOL_D3D11
#elif defined(__EMSCRIPTEN__)
	#define SOKOL_GLES2
#else
	#define SOKOL_GLCORE33
#endif

#define UMATH_IMPLEMENTATION

#if defined(TEST_VIEWER)
	#define DUMMY_SAPP_MAX_FRAMES 64
	#include "external/dummy_sokol_app.h"
	#include "external/dummy_sokol_time.h"
	#include "external/dummy_sokol_gfx.h"
#else
	#include "external/sokol_app.h"
	#include "external/sokol_time.h"
	#include "external/sokol_gfx.h"
#endif

#include "external/sokol_glue.h"
#include "external/umath.h"
