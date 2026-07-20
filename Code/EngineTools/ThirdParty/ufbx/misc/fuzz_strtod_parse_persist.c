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
#include <sys/mman.h>

char g_buffer[1024*32];

__AFL_COVERAGE();
__AFL_COVERAGE_START_OFF();

int main(int argc, char **argv)
{
	size_t page_size = 64 * 1024;
	char *read_buffer = mmap(NULL, page_size * 2, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	ufbx_assert(read_buffer);
	char *read_end = read_buffer + page_size;
	int prot_result = mprotect(read_end, page_size, PROT_NONE);
	ufbx_assert(prot_result == 0);

	#if defined(NO_AFL)
	size_t size = (size_t)read(0, g_buffer, sizeof(g_buffer) - 1);
	for (size_t i = 0; i < 10000; i++) {
	#else
	while (__AFL_LOOP(100000)) {
	size_t size = (size_t)read(0, g_buffer, sizeof(g_buffer) - 1);
	#endif

		char *value = read_end - size;
		memcpy(value, g_buffer, size);

		__AFL_COVERAGE_ON();

		char *p_end = NULL;
		double ufbx_d = ufbxi_parse_double(value, size, &p_end, 0);

		__AFL_COVERAGE_OFF();
	}

	munmap(read_buffer, page_size * 2);
	return 0;
}


