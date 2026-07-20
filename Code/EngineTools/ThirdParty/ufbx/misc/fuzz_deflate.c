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

#include "../ufbx.c"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: fuzz_deflate <file>\n");
		return 1;
	}
	FILE *f = fopen(argv[1], "rb");
	if (!f) {
		fprintf(stderr, "Failed to open file\n");
		return 1;
	}
	fseek(f, 0, SEEK_END);
	size_t src_size = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *src = malloc(src_size);
	size_t num_read = fread(src, 1, src_size, f);
	if (num_read != src_size) {
		fprintf(stderr, "Failed to read file\n");
		return 1;
	}
	fclose(f);

	size_t dst_size = 1024*src_size;
	if (argc >= 3) {
		dst_size = (size_t)atoi(argv[2]);
	}

	char *dst = malloc(dst_size);

	ufbx_inflate_retain retain;
	retain.initialized = false;
	ufbx_inflate_input input = { 0 };
	input.data = src;
	input.data_size = src_size;
	input.total_size = src_size;
	ptrdiff_t result = ufbx_inflate(dst, dst_size, &input, &retain);

	free(src);
	free(dst);

	printf("%td\n", result);
	return 0;
}

