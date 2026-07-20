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

#include "../ufbx.c"
#include "../test/check_scene.h"
#include "../test/testing_fuzz.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char g_buffer[1024*1024];

int main(int argc, char **argv)
{
#if defined(NO_AFL)
	size_t size = (size_t)read(0, g_buffer, sizeof(g_buffer));
	for (size_t i = 0; i < 10000; i++) {
#else
	while (__AFL_LOOP(10000)) {
		size_t size = (size_t)read(0, g_buffer, sizeof(g_buffer));
#endif
		ufbx_load_opts opts = { 0 };

		#if defined(DISCRETE_ALLOCATIONS)
			opts.temp_allocator.huge_threshold = 1;
			opts.result_allocator.huge_threshold = 1;
		#endif

		#if defined(LOAD_OBJ)
			opts.file_format = UFBX_FILE_FORMAT_OBJ;
		#elif defined(LOAD_MTL)
			opts.file_format = UFBX_FILE_FORMAT_MTL;
		#elif defined(LOAD_GUESS)
		#else
			opts.file_format = UFBX_FILE_FORMAT_FBX;
		#endif

		char *data = g_buffer;

		#if defined(FUZZ_HEADER)
		{
			 if (size < sizeof(ufbxt_fuzz_header)) return 1;
			const ufbxt_fuzz_header *header = (ufbxt_fuzz_header*)data;
			if (memcmp(header->magic, "ufbxfuzz", 8)) return 1;
			if (header->version != 1) return 1;
			ufbxt_fuzz_apply_flags(&opts, header->flags);

			data += sizeof(ufbxt_fuzz_header);
			size -= sizeof(ufbxt_fuzz_header);
		}
		#endif

		ufbx_scene *scene = ufbx_load_memory(data, size, &opts, NULL);
		if (scene) {
			ufbxt_check_scene(scene);
		}
		ufbx_free_scene(scene);
	}

	return 0;
}

