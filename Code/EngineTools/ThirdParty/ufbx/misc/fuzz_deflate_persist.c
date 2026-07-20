#define _CRT_SECURE_NO_WARNINGS

#if defined(_WIN32)
#define ufbx_assert(cond) do { \
		if (!(cond)) __debugbreak(); \
	} while (0)
#else
#define ufbx_assert(cond) do { \
		if (!(cond)) __builtin_trap(); \
	} while (0)
#endif

#define ufbxt_assert_fail(file, line, msg) ufbx_assert(false)
#define ufbxt_assert(m_cond) ufbx_assert(m_cond)
#define UFBX_DEV

#include "../ufbx.c"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <zlib.h>

char g_buffer[1024*32];
#ifdef FUZZ_SMALL
	char g_dst[128];
#else
	char g_dst[65536];
#endif

__AFL_COVERAGE();
__AFL_COVERAGE_START_OFF();

int main(int argc, char **argv)
{
	ufbx_inflate_retain retain;
	retain.initialized = false;

#if defined(NO_AFL)
	size_t size = (size_t)read(0, g_buffer, sizeof(g_buffer));
	for (size_t i = 0; i < 10000; i++) {
#else
	while (__AFL_LOOP(100000)) {
		size_t size = (size_t)read(0, g_buffer, sizeof(g_buffer));
#endif

		size_t length = 0;

		__AFL_COVERAGE_ON();

		ufbx_inflate_input input = { 0 };
		input.data = g_buffer;
		input.data_size = size;
		input.total_size = size;
		ptrdiff_t result = ufbx_inflate(g_dst, sizeof(g_dst), &input, &retain);

		__AFL_COVERAGE_OFF();

		if (result < 0) {
			result = -1;
		}

		uLongf zlib_len = sizeof(g_dst);
		int ret = uncompress((Bytef*)g_dst, &zlib_len, (const Bytef*)g_buffer, size);
		if (ret == Z_OK) {
			ufbxt_assert(result == zlib_len);
		} else {
			ufbxt_assert(result < 0);
		}
	}

	return 0;
}
