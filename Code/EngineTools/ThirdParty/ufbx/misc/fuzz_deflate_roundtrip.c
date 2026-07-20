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

#if !defined(_WIN32)
	#include <unistd.h>
#endif

#include "../build/libdeflate/libdeflate.h"

char g_buffer[65536];
char g_dst[65536];
char g_ref[65536];

#if !defined(NO_AFL)
__AFL_COVERAGE();
__AFL_COVERAGE_START_OFF();
#endif

int main(int argc, char **argv)
{
	ufbx_inflate_retain retain;
	retain.initialized = false;

	struct libdeflate_decompressor *dc = libdeflate_alloc_decompressor();

#if defined(NO_AFL)
	size_t size = fread(g_buffer, 1, sizeof(g_buffer), stdin);
	{
#else
	while (__AFL_LOOP(100000)) {
		size_t size = (size_t)read(0, g_buffer, sizeof(g_buffer));
#endif

		size_t length = 0;

#if !defined(NO_AFL)
		__AFL_COVERAGE_ON();
#endif

		size_t ref_length = 0;
		enum libdeflate_result ref_result = libdeflate_zlib_decompress(dc, g_buffer, size, g_ref, sizeof(g_ref), &ref_length);

		ufbx_inflate_input input = { 0 };
		input.data = g_buffer;
		input.data_size = size;
		input.total_size = size;
		input.no_checksum = true;
		ptrdiff_t result = ufbx_inflate(g_dst, sizeof(g_dst), &input, &retain);

#if !defined(NO_AFL)
		__AFL_COVERAGE_OFF();
#endif

		if (ref_result == LIBDEFLATE_SUCCESS) {
			if (result == -21 || result == -13 || result == -11) {
				// ufbx is more strict here
				#ifdef STRICT
					ufbxt_assert(0);
				#endif
			} else {
				ufbxt_assert((size_t)result == ref_length);
				ufbxt_assert(!memcmp(g_dst, g_ref, ref_length));
			}
		} else {
			// TODO: Be more strict
			#ifdef STRICT
				ufbxt_assert(result < 0);
			#endif
		}

		memset(g_buffer, 0, size);
	}

	return 0;
}
