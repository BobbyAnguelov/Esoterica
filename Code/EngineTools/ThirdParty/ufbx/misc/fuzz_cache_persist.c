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
#include <unistd.h>
#include <assert.h>

char g_buffer[1024*1024];
size_t g_pos = 0;
size_t g_size = 0;

static size_t read_file(void *user, void *data, size_t size)
{
    size_t to_read = g_size - g_pos;
    if (to_read > size) to_read = size;
    memcpy(data, g_buffer + g_pos, to_read);
    g_pos += to_read;
    return to_read;
}

static bool open_file(void *user, ufbx_stream *stream, const char *path, size_t path_len)
{
    if (!strcmp(path, "memory-cache")) {
        assert(g_pos == 0);
        stream->read_fn = &read_file;
        return true;
    } else {
        return false;
    }
}

int main(int argc, char **argv)
{
	ufbx_geometry_cache_opts opts = { 0 };
    opts.open_file_fn = &open_file;

    opts.temp_allocator.memory_limit = 0x4000000; // 64MB
    opts.result_allocator.memory_limit = 0x4000000; // 64MB

#if defined(DISCRETE_ALLOCATIONS)
	opts.temp_allocator.huge_threshold = 1;
	opts.result_allocator.huge_threshold = 1;
#endif

	while (__AFL_LOOP(10000)) {
		size_t size = (size_t)read(0, g_buffer, sizeof(g_buffer));
        g_size = size;
        g_pos = 0;

		ufbx_geometry_cache *cache = ufbx_load_geometry_cache("memory-cache", &opts, NULL);
        ufbx_free_geometry_cache(cache);
	}

	return 0;
}

