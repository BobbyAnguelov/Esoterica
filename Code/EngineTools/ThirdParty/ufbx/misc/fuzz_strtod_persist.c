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

char g_buffer[1024*32];

__AFL_COVERAGE();
__AFL_COVERAGE_START_OFF();

int main(int argc, char **argv)
{
#if defined(NO_AFL)
	size_t size = (size_t)read(0, g_buffer, sizeof(g_buffer));
	for (size_t i = 0; i < 10000; i++) {
#else
	while (__AFL_LOOP(100000)) {
		size_t size = (size_t)read(0, g_buffer, sizeof(g_buffer));
#endif

		size_t length = 0;

		#if defined(BINARY)
		{
			typedef struct {
				int64_t a;
				uint64_t b;
				int16_t exponent;
			} binary_header;

			memset(g_buffer + size, 0, sizeof(binary_header));

			binary_header header;
			memcpy(&header, g_buffer, sizeof(binary_header));

			length = (size_t)snprintf(g_buffer, sizeof(g_buffer), "%lld.%llue%d",
				(long long)header.a, (unsigned long long)header.b, (int)header.exponent);
		}
		#else
		{
			for (; length < size && length < 4096; length++) {
				char c = g_buffer[length];
				if (c >= '0' && c <= '9') {
					continue;
				} else if (c == '.' || c == '+' || c == '-' || c == 'e' || c == 'E') {
					continue;
				} else {
					break;
				}
			}
			g_buffer[length] = '\0';
		}
		#endif

		__AFL_COVERAGE_ON();

		char *ufbx_end;
		double ufbx_d = ufbxi_parse_double(g_buffer, SIZE_MAX, &ufbx_end, 0);
		float ufbx_f = (float)ufbxi_parse_double(g_buffer, SIZE_MAX, &ufbx_end, UFBXI_PARSE_DOUBLE_AS_BINARY32);

		__AFL_COVERAGE_OFF();

		double ref_d = strtod(g_buffer, NULL);
		float ref_f = strtof(g_buffer, NULL);

		if (length <= 100) {
			ufbx_assert(ufbx_d == ref_d);
			ufbx_assert(ufbx_f == ref_f);
		} else {
			typedef union { double d; int64_t b; } dbits;
			typedef union { float f; int32_t b; } fbits;

			dbits ufbx_db, ref_db;
			ufbx_db.d = ufbx_d;
			ref_db.d = ref_d;
			int64_t err_d = ufbx_db.b - ref_db.b;
			ufbx_assert(ufbx_d == ref_d || (err_d >= -1 && err_d <= 1));

			fbits ufbx_fb, ref_fb;
			ufbx_fb.f = ufbx_f;
			ref_fb.f = ref_f;
			int32_t err_f = ufbx_fb.b - ref_fb.b;
			ufbx_assert(ufbx_f == ref_f || (err_f >= -1 && err_f <= 1));
		}
	}

	return 0;
}

