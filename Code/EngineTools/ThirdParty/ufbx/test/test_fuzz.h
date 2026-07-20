#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "fuzz"

UFBXT_TEST(fuzz_files)
#if UFBXT_IMPL
{
	size_t ok = 0;
	size_t i = g_fuzz_step < SIZE_MAX ? g_fuzz_step : 0;
	for (; i < 10000; i++) {
		if (g_fuzz_file != SIZE_MAX && i != g_fuzz_file) continue;

		char name[512];
		char buf[512];
		snprintf(name, sizeof(name), "fuzz_%04zu", i);
		snprintf(buf, sizeof(buf), "%sfuzz/fuzz_%04zu.fbx", data_root, i);

		size_t size;
		void *data = ufbxt_read_file(buf, &size);
		if (!data) break;

		ufbx_error error;

		ufbx_load_opts load_opts = { 0 };
		load_opts.temp_allocator.memory_limit = 0x4000000; // 64MB
		load_opts.result_allocator.memory_limit = 0x4000000; // 64MB
		load_opts.file_format = UFBX_FILE_FORMAT_FBX;

		ufbx_scene *scene = ufbx_load_memory(data, size, &load_opts, &error);
		if (scene) {
			ufbxt_check_scene(scene);
			ok++;
		}
		bool allow_error = scene == NULL;

		ufbx_free_scene(scene);

		ufbxt_progress_ctx stream_progress_ctx = { 0 };

		bool temp_freed = false, result_freed = false;

		ufbx_load_opts stream_opts = load_opts;
		ufbxt_init_allocator(&stream_opts.temp_allocator, &temp_freed);
		ufbxt_init_allocator(&stream_opts.result_allocator, &result_freed);
		stream_opts.read_buffer_size = 1;
		stream_opts.temp_allocator.huge_threshold = 1;
		stream_opts.result_allocator.huge_threshold = 1;
		stream_opts.progress_cb.fn = &ufbxt_measure_progress;
		stream_opts.progress_cb.user = &stream_progress_ctx;
		stream_opts.progress_interval_hint = 1;
		ufbx_scene *streamed_scene = ufbx_load_file(buf, &stream_opts, &error);
		if (streamed_scene) {
			ufbxt_check_scene(streamed_scene);
			ufbxt_assert(scene);
		} else {
			ufbxt_assert(!scene);
		}
		ufbx_free_scene(streamed_scene);

		ufbxt_assert(temp_freed);
		ufbxt_assert(result_freed);

		int prev_fuzz_quality = g_fuzz_quality;
		g_fuzz_quality = g_heavy_fuzz_quality;
		ufbxt_do_fuzz(name, data, size, buf, allow_error, UFBX_FILE_FORMAT_FBX, NULL);
		g_fuzz_quality = prev_fuzz_quality;

		free(data);
	}

	ufbxt_logf(".. Loaded fuzz files: %zu (%zu non-errors)", i, ok);
}
#endif

UFBXT_TEST(fuzz_cache_xml)
#if UFBXT_IMPL
{
	size_t ok = 0;
	size_t i = g_fuzz_step < SIZE_MAX ? g_fuzz_step : 0;
	for (; i < 10000; i++) {
		if (g_fuzz_file != SIZE_MAX && i != g_fuzz_file) continue;

		char buf[512];
		snprintf(buf, sizeof(buf), "%scache_fuzz/xml/fuzz_%04zu.xml", data_root, i);

		ufbx_geometry_cache_opts cache_opts = { 0 };
		cache_opts.temp_allocator.memory_limit = 0x4000000; // 64MB
		cache_opts.result_allocator.memory_limit = 0x4000000; // 64MB

		if (g_dedicated_allocs) {
			cache_opts.temp_allocator.huge_threshold = 1;
			cache_opts.result_allocator.huge_threshold = 1;
		}

		size_t size;
		void *data = ufbxt_read_file(buf, &size);
		if (!data) break;

		// TODO: Read memory?

		ufbx_error error;
		ufbx_geometry_cache *cache = ufbx_load_geometry_cache(buf, &cache_opts, &error);
		if (cache) {
			ufbxt_check_string(cache->root_filename);
			ok++;
		}
		ufbx_free_geometry_cache(cache);

		free(data);
	}

	ufbxt_logf(".. Loaded fuzz files: %zu (%zu non-errors)", i, ok);
}
#endif

UFBXT_TEST(fuzz_cache_mcx)
#if UFBXT_IMPL
{
	size_t ok = 0;
	size_t i = g_fuzz_step < SIZE_MAX ? g_fuzz_step : 0;
	for (; i < 10000; i++) {
		if (g_fuzz_file != SIZE_MAX && i != g_fuzz_file) continue;

		char buf[512];
		snprintf(buf, sizeof(buf), "%scache_fuzz/mcx/fuzz_%04zu.mcx", data_root, i);

		ufbx_geometry_cache_opts cache_opts = { 0 };
		cache_opts.temp_allocator.memory_limit = 0x4000000; // 64MB
		cache_opts.result_allocator.memory_limit = 0x4000000; // 64MB

		if (g_dedicated_allocs) {
			cache_opts.temp_allocator.huge_threshold = 1;
			cache_opts.result_allocator.huge_threshold = 1;
		}

		size_t size;
		void *data = ufbxt_read_file(buf, &size);
		if (!data) break;

		// TODO: Read memory?

		ufbx_error error;
		ufbx_geometry_cache *cache = ufbx_load_geometry_cache(buf, &cache_opts, &error);
		if (cache) {
			ufbxt_check_string(cache->root_filename);
			ok++;
		}
		ufbx_free_geometry_cache(cache);

		free(data);
	}

	ufbxt_logf(".. Loaded fuzz files: %zu (%zu non-errors)", i, ok);
}
#endif

UFBXT_TEST(fuzz_obj_files)
#if UFBXT_IMPL
{
	size_t ok = 0;
	size_t i = g_fuzz_step < SIZE_MAX ? g_fuzz_step : 0;
	for (; i < 10000; i++) {
		if (g_fuzz_file != SIZE_MAX && i != g_fuzz_file) continue;

		char name[512];
		char buf[512];
		snprintf(name, sizeof(name), "obj_fuzz_%04zu", i);
		snprintf(buf, sizeof(buf), "%sobj_fuzz/fuzz_%04zu.obj", data_root, i);

		size_t size;
		void *data = ufbxt_read_file(buf, &size);
		if (!data) break;

		ufbx_error error;

		ufbx_load_opts load_opts = { 0 };
		load_opts.temp_allocator.memory_limit = 0x4000000; // 64MB
		load_opts.result_allocator.memory_limit = 0x4000000; // 64MB
		load_opts.file_format = UFBX_FILE_FORMAT_OBJ;

		ufbx_scene *scene = ufbx_load_memory(data, size, &load_opts, &error);
		if (scene) {
			ufbxt_check_scene(scene);
			ok++;
		}

		bool allow_error = scene == NULL;

		ufbx_free_scene(scene);

		ufbxt_progress_ctx stream_progress_ctx = { 0 };

		bool temp_freed = false, result_freed = false;

		ufbx_load_opts stream_opts = load_opts;
		ufbxt_init_allocator(&stream_opts.temp_allocator, &temp_freed);
		ufbxt_init_allocator(&stream_opts.result_allocator, &result_freed);
		stream_opts.read_buffer_size = 1;
		stream_opts.temp_allocator.huge_threshold = 1;
		stream_opts.result_allocator.huge_threshold = 1;
		stream_opts.progress_cb.fn = &ufbxt_measure_progress;
		stream_opts.progress_cb.user = &stream_progress_ctx;
		stream_opts.progress_interval_hint = 1;
		ufbx_scene *streamed_scene = ufbx_load_file(buf, &stream_opts, &error);
		if (streamed_scene) {
			ufbxt_check_scene(streamed_scene);
			ufbxt_assert(scene);
		} else {
			ufbxt_assert(!scene);
		}
		ufbx_free_scene(streamed_scene);

		ufbxt_assert(temp_freed);
		ufbxt_assert(result_freed);

		ufbxt_do_fuzz(name, data, size, buf, allow_error, UFBX_FILE_FORMAT_OBJ, NULL);

		free(data);
	}

	ufbxt_logf(".. Loaded fuzz files: %zu (%zu non-errors)", i, ok);
}
#endif

UFBXT_TEST(fuzz_mtl_files)
#if UFBXT_IMPL
{
	size_t ok = 0;
	size_t i = g_fuzz_step < SIZE_MAX ? g_fuzz_step : 0;
	for (; i < 10000; i++) {
		if (g_fuzz_file != SIZE_MAX && i != g_fuzz_file) continue;

		char name[512];
		char buf[512];
		snprintf(name, sizeof(name), "mtl_fuzz_%04zu", i);
		snprintf(buf, sizeof(buf), "%smtl_fuzz/fuzz_%04zu.mtl", data_root, i);

		size_t size;
		void *data = ufbxt_read_file(buf, &size);
		if (!data) break;

		ufbx_error error;

		ufbx_load_opts load_opts = { 0 };
		load_opts.temp_allocator.memory_limit = 0x4000000; // 64MB
		load_opts.result_allocator.memory_limit = 0x4000000; // 64MB
		load_opts.file_format = UFBX_FILE_FORMAT_MTL;

		ufbx_scene *scene = ufbx_load_memory(data, size, &load_opts, &error);
		if (scene) {
			ufbxt_check_scene(scene);
			ok++;
		}

		bool allow_error = scene == NULL;

		ufbx_free_scene(scene);

		ufbxt_progress_ctx stream_progress_ctx = { 0 };

		bool temp_freed = false, result_freed = false;

		ufbx_load_opts stream_opts = load_opts;
		ufbxt_init_allocator(&stream_opts.temp_allocator, &temp_freed);
		ufbxt_init_allocator(&stream_opts.result_allocator, &result_freed);
		stream_opts.read_buffer_size = 1;
		stream_opts.temp_allocator.huge_threshold = 1;
		stream_opts.result_allocator.huge_threshold = 1;
		stream_opts.progress_cb.fn = &ufbxt_measure_progress;
		stream_opts.progress_cb.user = &stream_progress_ctx;
		stream_opts.progress_interval_hint = 1;
		ufbx_scene *streamed_scene = ufbx_load_file(buf, &stream_opts, &error);
		if (streamed_scene) {
			ufbxt_check_scene(streamed_scene);
			ufbxt_assert(scene);
		} else {
			ufbxt_assert(!scene);
		}
		ufbx_free_scene(streamed_scene);

		ufbxt_assert(temp_freed);
		ufbxt_assert(result_freed);

		ufbxt_do_fuzz(name, data, size, buf, allow_error, UFBX_FILE_FORMAT_MTL, NULL);

		free(data);
	}

	ufbxt_logf(".. Loaded fuzz files: %zu (%zu non-errors)", i, ok);
}
#endif

