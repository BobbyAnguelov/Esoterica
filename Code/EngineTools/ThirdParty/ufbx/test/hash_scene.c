#define _CRT_SECURE_NO_WARNINGS

#include <string.h>

#include "../ufbx.h"
#include "hash_scene.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>

#if defined(USE_THREADS)
	#define UFBX_OS_IMPLEMENTATION
	#include "../extra/ufbx_os.h"
#endif

#if defined(USE_THREADS)
	ufbx_os_thread_pool *g_thread_pool;
#endif

typedef struct {
	size_t pos;
	size_t size;
	union {
		size_t size_and_align[2];
		bool reverse;
	};
	char data[];
} ufbxt_linear_alloactor;

static void *linear_alloc_fn(void *user, size_t size)
{
	ufbxt_linear_alloactor *ator = (ufbxt_linear_alloactor*)user;
	size = (size + 7u) & ~(size_t)7;

	size_t pos = ator->pos;
	if (ator->reverse) {
		assert(pos >= size);
		ator->pos = pos - size;
		return ator->data + (pos - size);
	} else {
		assert(ator->size - pos >= size);
		ator->pos = pos + size;
		return ator->data + pos;
	}
}

static void linear_free_allocator_fn(void *user)
{
	ufbxt_linear_alloactor *ator = (ufbxt_linear_alloactor*)user;
	free(ator);
}

static void linear_allocator_init(ufbx_allocator *allocator, size_t size, bool reverse)
{
	ufbxt_linear_alloactor *ator = (ufbxt_linear_alloactor*)malloc(sizeof(ufbxt_linear_alloactor) + size);
	assert(ator);
	ator->size = size;
	ator->reverse = reverse;
	if (reverse) {
		ator->pos = ator->size;
	} else {
		ator->pos = 0;
	}

	allocator->alloc_fn = &linear_alloc_fn;
	allocator->free_allocator_fn = &linear_free_allocator_fn;
	allocator->user = ator;
}

typedef struct {
	bool dedicated_allocs;
	bool no_read_buffer;
	const char *allocator;
	size_t linear_allocator_size;
} ufbxt_hasher_opts;

static void setup_allocator(ufbx_allocator_opts *ator_opts, ufbxt_hasher_opts *opts)
{
	if (opts->dedicated_allocs) {
		ator_opts->huge_threshold = 1;
	}

	size_t linear_size = opts->linear_allocator_size ? opts->linear_allocator_size : 128u*1024u*1024u;
	if (opts->allocator) {
		if (!strcmp(opts->allocator, "linear")) {
			linear_allocator_init(&ator_opts->allocator, linear_size, false);
		} else if (!strcmp(opts->allocator, "linear-reverse")) {
			linear_allocator_init(&ator_opts->allocator, linear_size, true);
		} else {
			fprintf(stderr, "Unknown allocator: %s\n", opts->allocator);
			exit(1);
		}
	}
}

static ufbx_scene *load_scene(const char *filename, int frame, ufbxt_hasher_opts *hasher_opts)
{
	ufbx_load_opts opts = { 0 };
	opts.load_external_files = true;
	opts.ignore_missing_external_files = true;
	opts.evaluate_caches = true;
	opts.evaluate_skinning = true;
	opts.target_axes = ufbx_axes_right_handed_y_up;
	opts.target_unit_meters = 1.0f;

	#if defined(USE_THREADS)
		ufbx_os_init_ufbx_thread_pool(&opts.thread_opts.pool, g_thread_pool);
	#endif

	if (hasher_opts->no_read_buffer) {
		opts.read_buffer_size = 1;
	}

	setup_allocator(&opts.temp_allocator, hasher_opts);
	setup_allocator(&opts.result_allocator, hasher_opts);

	ufbx_error error;
	ufbx_scene *scene = ufbx_load_file(filename, &opts, &error);
	if (!scene) {
		#if defined(USE_THREADS)
			if (error.type == UFBX_ERROR_THREADED_ASCII_PARSE) return NULL;
		#endif

		fprintf(stderr, "Failed to load scene: %s\n", error.description.data);
		exit(2);
	}

	if (frame > 0) {
		ufbx_evaluate_opts eval_opts = { 0 };
		eval_opts.evaluate_caches = true;
		eval_opts.evaluate_skinning = true;
		eval_opts.load_external_files = true;

		double time = scene->anim->time_begin + frame / scene->settings.frames_per_second;
		ufbx_scene *state = ufbx_evaluate_scene(scene, NULL, time, NULL, &error);
		if (!state) {
			fprintf(stderr, "Failed to evaluate scene: %s\n", error.description.data);
			exit(2);
		}
		ufbx_free_scene(scene);
		scene = state;
	}

	return scene;
}

int main(int argc, char **argv)
{
	const char *filename = NULL;
	const char *dump_filename = NULL;
	int max_dump_errors = -1;
	bool do_check = false;
	bool dump_all = false;
	bool verbose = false;
	ufbxt_hasher_opts hasher_opts = { 0 };

	#if defined(USE_THREADS)
	 {
		ufbx_os_thread_pool_opts pool_opts = { 0 };
		pool_opts.max_threads = 4;
		g_thread_pool = ufbx_os_create_thread_pool(&pool_opts);
		assert(g_thread_pool);
	 }
	#endif

	int frame = -1;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--frame")) {
			if (++i < argc) frame = atoi(argv[i]);
		} else if (!strcmp(argv[i], "--verbose")) {
			verbose = true;
		} else if (!strcmp(argv[i], "--check")) {
			do_check = true;
		} else if (!strcmp(argv[i], "--dump")) {
			if (++i < argc) dump_filename = argv[i];
		} else if (!strcmp(argv[i], "--dump-all")) {
			dump_all = true;
		} else if (!strcmp(argv[i], "--dedicated-allocs")) {
			hasher_opts.dedicated_allocs = true;
		} else if (!strcmp(argv[i], "--no-read-buffer")) {
			hasher_opts.no_read_buffer = true;
		} else if (!strcmp(argv[i], "--allocator")) {
			if (++i < argc) hasher_opts.allocator = argv[i];
		} else if (!strcmp(argv[i], "--max-dump-errors")) {
			if (++i < argc) max_dump_errors = atoi(argv[i]);
		} else {
			if (filename) {
				if (argv[i][0] == '-') {
					fprintf(stderr, "Unknown flag: %s\n", argv[i]);
					exit(1);
				} else {
					fprintf(stderr, "Error: Multiple input files\n");
					exit(1);
				}
			}
			filename = argv[i];
		}
	}

	if (!filename) {
		fprintf(stderr, "Usage: hash_scene <file.fbx>\n");
		fprintf(stderr, "       hash_scene --check <hashes.txt>\n");
		return 1;
	}

	int num_fail = 0;
	int num_total = 0;

	FILE *dump_file = NULL;

	if (do_check) {
		FILE *f = fopen(filename, "rb");
		if (!f) {
			fprintf(stderr, "Failed to open hash file\n");
			return 1;
		}

		uint64_t fbx_hash = 0;
		char fbx_file[1024];
		while (fscanf(f, "%" SCNx64 " %d %[^\r\n]", &fbx_hash, &frame, fbx_file) == 3) {
			ufbx_scene *scene = load_scene(fbx_file, frame, &hasher_opts);
			if (!scene) continue;

			uint64_t hash = ufbxt_hash_scene(scene, NULL);
			if (hash != fbx_hash || dump_all) {
				if (num_fail < max_dump_errors || dump_all) {
					if (!dump_file) {
						dump_file = fopen(dump_filename, "wb");
						assert(dump_file);
					}

					fprintf(dump_file, "\n-- %d %s\n\n", frame, fbx_file);
					ufbxt_hash_scene(scene, dump_file);
				}

				if (!dump_all) {
					printf("%s: FAIL %" PRIx64 " (local) vs %" PRIx64 " (reference)\n",
						fbx_file, hash, fbx_hash);
					num_fail++;
				}
			} else {
				if (verbose) {
					printf("%s %d: OK\n", fbx_file, frame);
				}
			}
			num_total++;

			ufbx_free_scene(scene);
		}

		fclose(f);

		if (!dump_all) {
			if (verbose || num_fail > 0) {
				printf("\n");
			}
			printf("%d/%d hashes match\n", num_total - num_fail, num_total);
		}
	} else {
		ufbx_scene *scene = load_scene(filename, frame, &hasher_opts);

		if (dump_filename) {
			dump_file = fopen(dump_filename, "wb");
			assert(dump_file);
		}

		uint64_t hash = ufbxt_hash_scene(scene, dump_file);

		printf("%016" PRIx64 "\n", hash);
	}

	if (dump_file) {
		fclose(dump_file);
	}

	#if defined(UFBXT_THREADS)
		ufbx_os_free_thread_pool(g_thread_pool);
	#endif

	return num_fail > 0 ? 3 : 0;
}

#define UFBX_EXTERNAL_MATH

#include "../ufbx.c"

