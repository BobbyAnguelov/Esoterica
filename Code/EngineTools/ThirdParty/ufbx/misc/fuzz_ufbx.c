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
		fprintf(stderr, "Usage: fuzz_ufbx <file>\n");
		return 1;
	}

	ufbx_load_opts opts = { 0 };

	for (int i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "-d")) {
			opts.temp_huge_size = 1;
			opts.result_huge_size = 1;
		}
	}

	ufbx_scene *scene = ufbx_load_file(argv[1], &opts, NULL);
	ufbx_free_scene(scene);

	return 0;
}

