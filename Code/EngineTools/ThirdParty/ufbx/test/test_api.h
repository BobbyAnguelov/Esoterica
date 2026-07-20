#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "api"

#if UFBXT_IMPL

static void ufbxt_close_memory(void *user, void *data, size_t data_size)
{
	free(data);
}

static bool ufbxt_open_file_memory_default(void *user, ufbx_stream *stream, const char *path, size_t path_len, const ufbx_open_file_info *info)
{
	++*(size_t*)user;

	size_t size;
	void *data = ufbxt_read_file(path, &size);
	if (!data) return false;

	bool ok = ufbx_open_memory(stream, data, size, NULL, NULL);
	free(data);
	return ok;
}

static bool ufbxt_open_file_memory_ctx(void *user, ufbx_stream *stream, const char *path, size_t path_len, const ufbx_open_file_info *info)
{
	++*(size_t*)user;

	size_t size;
	void *data = ufbxt_read_file(path, &size);
	if (!data) return false;

	bool ok = ufbx_open_memory_ctx(stream, info->context, data, size, NULL, NULL);
	free(data);
	return ok;
}

static bool ufbxt_open_file_memory_ref(void *user, ufbx_stream *stream, const char *path, size_t path_len, const ufbx_open_file_info *info)
{
	++*(size_t*)user;

	size_t size;
	void *data = ufbxt_read_file(path, &size);
	if (!data) return false;

	ufbx_open_memory_opts opts = { 0 };
	opts.no_copy = true;
	opts.close_cb.fn = &ufbxt_close_memory;
	return ufbx_open_memory(stream, data, size, &opts, NULL);
}

#endif

#if UFBXT_IMPL
static void ufbxt_do_open_memory_test(const char *filename, size_t expected_calls_fbx, size_t expected_calls_obj, ufbx_open_file_fn *open_file_fn)
{
	char path[512];
	ufbxt_file_iterator iter = { filename };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (size_t i = 0; i < 2; i++) {
			ufbx_load_opts opts = { 0 };
			size_t num_calls = 0;

			opts.open_file_cb.fn = open_file_fn;
			opts.open_file_cb.user = &num_calls;
			opts.load_external_files = true;
			if (i == 1) {
				opts.read_buffer_size = 1;
			}

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			if (!scene) ufbxt_log_error(&error);
			ufbxt_assert(scene);

			ufbxt_check_scene(scene);

			if (scene->metadata.file_format == UFBX_FILE_FORMAT_FBX) {
				ufbxt_assert(num_calls == expected_calls_fbx);
			} else if (scene->metadata.file_format == UFBX_FILE_FORMAT_OBJ) {
				ufbxt_assert(num_calls == expected_calls_obj);
			} else {
				ufbxt_assert(false);
			}

			ufbx_free_scene(scene);
		}
	}
}
#endif

UFBXT_TEST(open_memory_default)
#if UFBXT_IMPL
{
	ufbxt_do_open_memory_test("maya_cache_sine", 5, 0, ufbxt_open_file_memory_default);
}
#endif

UFBXT_TEST(open_memory_ctx)
#if UFBXT_IMPL
{
	ufbxt_do_open_memory_test("maya_cache_sine", 5, 0, ufbxt_open_file_memory_ctx);
}
#endif

UFBXT_TEST(open_memory_ref)
#if UFBXT_IMPL
{
	ufbxt_do_open_memory_test("maya_cache_sine", 5, 0, ufbxt_open_file_memory_ref);
}
#endif

UFBXT_TEST(obj_open_memory_default)
#if UFBXT_IMPL
{
	ufbxt_do_open_memory_test("blender_279_ball", 1, 2, ufbxt_open_file_memory_default);
}
#endif

UFBXT_TEST(obj_open_memory_ctx)
#if UFBXT_IMPL
{
	ufbxt_do_open_memory_test("blender_279_ball", 1, 2, ufbxt_open_file_memory_ctx);
}
#endif

UFBXT_TEST(obj_open_memory_ref)
#if UFBXT_IMPL
{
	ufbxt_do_open_memory_test("blender_279_ball", 1, 2, ufbxt_open_file_memory_ref);
}
#endif

UFBXT_TEST(retain_free_null)
#if UFBXT_IMPL
{
	ufbx_retain_scene(NULL);
	ufbx_free_scene(NULL);
	ufbx_retain_mesh(NULL);
	ufbx_free_mesh(NULL);
	ufbx_retain_line_curve(NULL);
	ufbx_free_line_curve(NULL);
	ufbx_retain_geometry_cache(NULL);
	ufbx_free_geometry_cache(NULL);
	ufbx_retain_anim(NULL);
	ufbx_free_anim(NULL);
	ufbx_retain_baked_anim(NULL);
	ufbx_free_baked_anim(NULL);
}
#endif

UFBXT_TEST(thread_memory)
#if UFBXT_IMPL
{
	ufbx_retain_scene(NULL);
	ufbx_free_scene(NULL);
	ufbx_retain_mesh(NULL);
	ufbx_free_mesh(NULL);
	ufbx_retain_line_curve(NULL);
	ufbx_free_line_curve(NULL);
	ufbx_retain_geometry_cache(NULL);
	ufbx_free_geometry_cache(NULL);
	ufbx_retain_anim(NULL);
	ufbx_free_anim(NULL);
	ufbx_retain_baked_anim(NULL);
	ufbx_free_baked_anim(NULL);
}
#endif

#if UFBXT_IMPL
typedef struct {
	bool immediate;
	bool initialized;
	bool freed;
	uint32_t wait_index;
	uint32_t dispatches;
} ufbxt_single_thread_pool;

static bool ufbxt_single_thread_pool_init_fn(void *user, ufbx_thread_pool_context ctx, const ufbx_thread_pool_info *info)
{
	ufbxt_single_thread_pool *pool = (ufbxt_single_thread_pool*)user;
	pool->initialized = true;

	return true;
}

static void ufbxt_single_thread_pool_run_fn(void *user, ufbx_thread_pool_context ctx, uint32_t group, uint32_t start_index, uint32_t count)
{
	ufbxt_single_thread_pool *pool = (ufbxt_single_thread_pool*)user;
	ufbxt_assert(pool->initialized);
	pool->dispatches++;
	if (!pool->immediate) return;

	for (uint32_t i = 0; i < count; i++) {
		ufbx_thread_pool_run_task(ctx, start_index + i);
	}
}

static void ufbxt_single_thread_pool_wait_fn(void *user, ufbx_thread_pool_context ctx, uint32_t group, uint32_t max_index)
{
	ufbxt_single_thread_pool *pool = (ufbxt_single_thread_pool*)user;
	ufbxt_assert(pool->initialized);

	if (!pool->immediate) {
		for (uint32_t i = pool->wait_index; i < max_index; i++) {
			ufbx_thread_pool_run_task(ctx, i);
		}
	}

	pool->wait_index = max_index;
}

static void ufbxt_single_thread_pool_free_fn(void *user, ufbx_thread_pool_context ctx)
{
	ufbxt_single_thread_pool *pool = (ufbxt_single_thread_pool*)user;
	pool->freed = true;
}

static void ufbxt_single_thread_pool_init(ufbx_thread_pool *dst, ufbxt_single_thread_pool *pool, bool immediate)
{
	memset(pool, 0, sizeof(ufbxt_single_thread_pool));
	pool->immediate = immediate;

	dst->init_fn = ufbxt_single_thread_pool_init_fn;
	dst->run_fn = ufbxt_single_thread_pool_run_fn;
	dst->wait_fn = ufbxt_single_thread_pool_wait_fn;
	dst->free_fn = ufbxt_single_thread_pool_free_fn;
	dst->user = pool;
}
#endif

#if UFBXT_IMPL
static bool ufbxt_is_big_endian()
{
	uint8_t buf[2];
	uint16_t val = 0xbbaa;
	memcpy(buf, &val, 2);
	return buf[0] == 0xbb;
}
#endif

UFBXT_TEST(single_thread_immediate_stream)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "blender_293_barbarian" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbxt_single_thread_pool pool;
		ufbx_load_opts opts = { 0 };
		ufbxt_single_thread_pool_init(&opts.thread_opts.pool, &pool, true);

		ufbx_error error;
		ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
		if (!scene) ufbxt_log_error(&error);
		ufbxt_assert(scene);

		ufbxt_assert(pool.initialized);
		ufbxt_assert(pool.freed);
		if (ufbxt_is_big_endian()) {
			ufbxt_assert(pool.wait_index == 0);
		} else {
			ufbxt_assert(pool.wait_index >= 100);
		}

		ufbxt_check_scene(scene);
		ufbx_free_scene(scene);
	}
}
#endif

UFBXT_TEST(single_thread_immediate_memory)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "blender_293_barbarian" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbxt_single_thread_pool pool;
		ufbx_load_opts opts = { 0 };
		ufbxt_single_thread_pool_init(&opts.thread_opts.pool, &pool, true);

		size_t size = 0;
		void *data = ufbxt_read_file(path, &size);
		ufbxt_assert(data);

		ufbx_error error;
		ufbx_scene *scene = ufbx_load_memory(data, size, &opts, &error);
		if (!scene) ufbxt_log_error(&error);
		ufbxt_assert(scene);

		ufbxt_assert(pool.initialized);
		ufbxt_assert(pool.freed);
		if (ufbxt_is_big_endian()) {
			ufbxt_assert(pool.wait_index == 0);
		} else {
			ufbxt_assert(pool.wait_index >= 100);
		}

		ufbxt_check_scene(scene);
		ufbx_free_scene(scene);
		free(data);
	}
}
#endif

UFBXT_TEST(single_thread_deferred_stream)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "blender_293_barbarian" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbxt_single_thread_pool pool;
		ufbx_load_opts opts = { 0 };
		ufbxt_single_thread_pool_init(&opts.thread_opts.pool, &pool, false);

		ufbx_error error;
		ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
		if (!scene) ufbxt_log_error(&error);
		ufbxt_assert(scene);

		ufbxt_assert(pool.initialized);
		ufbxt_assert(pool.freed);
		if (ufbxt_is_big_endian()) {
			ufbxt_assert(pool.wait_index == 0);
		} else {
			ufbxt_assert(pool.wait_index >= 100);
		}

		ufbxt_check_scene(scene);
		ufbx_free_scene(scene);
	}
}
#endif

UFBXT_TEST(single_thread_deferred_memory)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "blender_293_barbarian" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbxt_single_thread_pool pool;
		ufbx_load_opts opts = { 0 };
		ufbxt_single_thread_pool_init(&opts.thread_opts.pool, &pool, false);

		size_t size = 0;
		void *data = ufbxt_read_file(path, &size);
		ufbxt_assert(data);

		ufbx_error error;
		ufbx_scene *scene = ufbx_load_memory(data, size, &opts, &error);
		if (!scene) ufbxt_log_error(&error);
		ufbxt_assert(scene);

		ufbxt_assert(pool.initialized);
		ufbxt_assert(pool.freed);
		if (ufbxt_is_big_endian()) {
			ufbxt_assert(pool.wait_index == 0);
		} else {
			ufbxt_assert(pool.wait_index >= 100);
		}

		ufbxt_check_scene(scene);
		ufbx_free_scene(scene);
		free(data);
	}
}
#endif

UFBXT_TEST(thread_memory_limit)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "blender_293_barbarian" };
	size_t prev_dispatches = 0;
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (size_t i = 0; i < 24; i++) {
			ufbxt_single_thread_pool pool;
			ufbx_load_opts opts = { 0 };
			ufbxt_single_thread_pool_init(&opts.thread_opts.pool, &pool, true);
			opts.thread_opts.memory_limit = (size_t)1 << i;

			size_t size = 0;
			void *data = ufbxt_read_file(path, &size);
			ufbxt_assert(data);

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_memory(data, size, &opts, &error);
			if (!scene) ufbxt_log_error(&error);
			ufbxt_assert(scene);

			if (pool.dispatches != prev_dispatches) {
				ufbxt_logf("limit %zu dispatches: %u", opts.thread_opts.memory_limit, pool.dispatches);
				prev_dispatches = pool.dispatches;
			}

			ufbxt_assert(pool.initialized);
			ufbxt_assert(pool.freed);
			if (ufbxt_is_big_endian()) {
				ufbxt_assert(pool.wait_index == 0);
			} else {
				ufbxt_assert(pool.wait_index >= 100);
			}

			ufbxt_check_scene(scene);
			ufbx_free_scene(scene);
			free(data);
		}
	}
}
#endif

UFBXT_TEST(single_thread_file_not_found)
#if UFBXT_IMPL
{
	ufbxt_single_thread_pool pool;
	ufbx_load_opts opts = { 0 };
	ufbxt_single_thread_pool_init(&opts.thread_opts.pool, &pool, true);

	ufbx_error error;
	ufbx_scene *scene = ufbx_load_file("<doesnotexist>.fbx", &opts, &error);
	ufbxt_assert(!scene);
	ufbxt_assert(error.type == UFBX_ERROR_FILE_NOT_FOUND);
	ufbxt_assert(strstr(error.info, "<doesnotexist>.fbx"));

	ufbxt_assert(!pool.initialized);
	ufbxt_assert(!pool.freed);
	ufbxt_assert(pool.wait_index == 0);
}
#endif

UFBXT_TEST(empty_file_memory)
#if UFBXT_IMPL
{
	{
		ufbx_error error;
		ufbx_scene *scene = ufbx_load_memory(NULL, 0, NULL, &error);
		ufbxt_assert(!scene);
		ufbxt_assert(error.type == UFBX_ERROR_EMPTY_FILE);
	}

	{
		ufbx_load_opts opts = { 0 };
		opts.file_format = UFBX_FILE_FORMAT_FBX;
		ufbx_error error;
		ufbx_scene *scene = ufbx_load_memory(NULL, 0, &opts, &error);
		ufbxt_assert(!scene);
		ufbxt_assert(error.type == UFBX_ERROR_EMPTY_FILE);
	}
}
#endif

#if UFBXT_IMPL
static size_t ufbxt_empty_stream_read_fn(void *user, void *data, size_t size)
{
	return 0;
}
#endif

UFBXT_TEST(empty_file_stream)
#if UFBXT_IMPL
{
	ufbx_stream stream = { 0 };
	stream.read_fn = &ufbxt_empty_stream_read_fn;

	{
		ufbx_error error;
		ufbx_scene *scene = ufbx_load_stream(&stream, NULL, &error);
		ufbxt_assert(!scene);
		ufbxt_assert(error.type == UFBX_ERROR_EMPTY_FILE);
	}

	{
		ufbx_load_opts opts = { 0 };
		opts.file_format = UFBX_FILE_FORMAT_FBX;
		ufbx_error error;
		ufbx_scene *scene = ufbx_load_stream(&stream, &opts, &error);
		ufbxt_assert(!scene);
		ufbxt_assert(error.type == UFBX_ERROR_EMPTY_FILE);
	}
}
#endif

UFBXT_TEST(file_format_lookahead)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (size_t i = 0; i <= 16; i++) {
			ufbxt_hintf("i=%zu", i);

			ufbx_load_opts opts = { 0 };
			opts.file_format_lookahead = i * i * i;

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			if (!scene) ufbxt_log_error(&error);
			ufbxt_assert(scene);

			ufbxt_check_scene(scene);
			ufbx_free_scene(scene);
		}
	}
}
#endif

#if UFBXT_IMPL
static void *ufbxt_multiuse_realloc(void *user, void *old_ptr, size_t old_size, size_t new_size)
{
	if (new_size == 0) {
		free(old_ptr);
		return NULL;
	} else if (old_size > 0) {
		return realloc(old_ptr, new_size);
	} else {
		return malloc(new_size);
	}
}
static void *ufbxt_null_realloc(void *user, void *old_ptr, size_t old_size, size_t new_size)
{
	return NULL;
}
static void *ufbxt_alloc_malloc(void *user, size_t size)
{
	return malloc(size);
}
static void ufbxt_free_free(void *user, void *ptr, size_t size)
{
	free(ptr);
}
#endif

UFBXT_TEST(multiuse_realloc)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_load_opts opts = { 0 };
		opts.temp_allocator.allocator.realloc_fn = &ufbxt_multiuse_realloc;
		opts.result_allocator.allocator.realloc_fn = &ufbxt_multiuse_realloc;

		ufbx_error error;
		ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
		if (!scene) ufbxt_log_error(&error);
		ufbxt_assert(scene);
		ufbxt_check_scene(scene);
		ufbx_free_scene(scene);
	}
}
#endif

UFBXT_TEST(null_realloc)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_load_opts opts = { 0 };
		opts.temp_allocator.allocator.alloc_fn = &ufbxt_alloc_malloc;
		opts.temp_allocator.allocator.realloc_fn = &ufbxt_null_realloc;
		opts.temp_allocator.allocator.free_fn = &ufbxt_free_free;

		ufbx_error error;
		ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
		ufbxt_assert(!scene);
		ufbxt_assert(error.type == UFBX_ERROR_OUT_OF_MEMORY);
	}
}
#endif

UFBXT_TEST(null_realloc_only)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		{
			ufbx_load_opts opts = { 0 };
			opts.temp_allocator.allocator.realloc_fn = &ufbxt_null_realloc;

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			ufbxt_assert(!scene);
			ufbxt_assert(error.type == UFBX_ERROR_OUT_OF_MEMORY);
		}
		{
			ufbx_load_opts opts = { 0 };
			opts.result_allocator.allocator.realloc_fn = &ufbxt_null_realloc;

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			ufbxt_assert(!scene);
			ufbxt_assert(error.type == UFBX_ERROR_OUT_OF_MEMORY);
		}
	}
}
#endif

#if UFBXT_IMPL
typedef struct {
	ufbx_element_type type;
	size_t size;
} ufbxt_element_type_size;
#endif

UFBXT_TEST(element_type_sizes)
#if UFBXT_IMPL
{
	const ufbxt_element_type_size element_type_sizes[] = {
		{ UFBX_ELEMENT_UNKNOWN, sizeof(ufbx_unknown) },
		{ UFBX_ELEMENT_NODE, sizeof(ufbx_node) },
		{ UFBX_ELEMENT_MESH, sizeof(ufbx_mesh) },
		{ UFBX_ELEMENT_LIGHT, sizeof(ufbx_light) },
		{ UFBX_ELEMENT_CAMERA, sizeof(ufbx_camera) },
		{ UFBX_ELEMENT_BONE, sizeof(ufbx_bone) },
		{ UFBX_ELEMENT_EMPTY, sizeof(ufbx_empty) },
		{ UFBX_ELEMENT_LINE_CURVE, sizeof(ufbx_line_curve) },
		{ UFBX_ELEMENT_NURBS_CURVE, sizeof(ufbx_nurbs_curve) },
		{ UFBX_ELEMENT_NURBS_SURFACE, sizeof(ufbx_nurbs_surface) },
		{ UFBX_ELEMENT_NURBS_TRIM_SURFACE, sizeof(ufbx_nurbs_trim_surface) },
		{ UFBX_ELEMENT_NURBS_TRIM_BOUNDARY, sizeof(ufbx_nurbs_trim_boundary) },
		{ UFBX_ELEMENT_PROCEDURAL_GEOMETRY, sizeof(ufbx_procedural_geometry) },
		{ UFBX_ELEMENT_STEREO_CAMERA, sizeof(ufbx_stereo_camera) },
		{ UFBX_ELEMENT_CAMERA_SWITCHER, sizeof(ufbx_camera_switcher) },
		{ UFBX_ELEMENT_MARKER, sizeof(ufbx_marker) },
		{ UFBX_ELEMENT_LOD_GROUP, sizeof(ufbx_lod_group) },
		{ UFBX_ELEMENT_SKIN_DEFORMER, sizeof(ufbx_skin_deformer) },
		{ UFBX_ELEMENT_SKIN_CLUSTER, sizeof(ufbx_skin_cluster) },
		{ UFBX_ELEMENT_BLEND_DEFORMER, sizeof(ufbx_blend_deformer) },
		{ UFBX_ELEMENT_BLEND_CHANNEL, sizeof(ufbx_blend_channel) },
		{ UFBX_ELEMENT_BLEND_SHAPE, sizeof(ufbx_blend_shape) },
		{ UFBX_ELEMENT_CACHE_DEFORMER, sizeof(ufbx_cache_deformer) },
		{ UFBX_ELEMENT_CACHE_FILE, sizeof(ufbx_cache_file) },
		{ UFBX_ELEMENT_MATERIAL, sizeof(ufbx_material) },
		{ UFBX_ELEMENT_TEXTURE, sizeof(ufbx_texture) },
		{ UFBX_ELEMENT_VIDEO, sizeof(ufbx_video) },
		{ UFBX_ELEMENT_SHADER, sizeof(ufbx_shader) },
		{ UFBX_ELEMENT_SHADER_BINDING, sizeof(ufbx_shader_binding) },
		{ UFBX_ELEMENT_ANIM_STACK, sizeof(ufbx_anim_stack) },
		{ UFBX_ELEMENT_ANIM_LAYER, sizeof(ufbx_anim_layer) },
		{ UFBX_ELEMENT_ANIM_VALUE, sizeof(ufbx_anim_value) },
		{ UFBX_ELEMENT_ANIM_CURVE, sizeof(ufbx_anim_curve) },
		{ UFBX_ELEMENT_DISPLAY_LAYER, sizeof(ufbx_display_layer) },
		{ UFBX_ELEMENT_SELECTION_SET, sizeof(ufbx_selection_set) },
		{ UFBX_ELEMENT_SELECTION_NODE, sizeof(ufbx_selection_node) },
		{ UFBX_ELEMENT_CHARACTER, sizeof(ufbx_character) },
		{ UFBX_ELEMENT_CONSTRAINT, sizeof(ufbx_constraint) },
		{ UFBX_ELEMENT_AUDIO_LAYER, sizeof(ufbx_audio_layer) },
		{ UFBX_ELEMENT_AUDIO_CLIP, sizeof(ufbx_audio_clip) },
		{ UFBX_ELEMENT_POSE, sizeof(ufbx_pose) },
		{ UFBX_ELEMENT_METADATA_OBJECT, sizeof(ufbx_metadata_object) },
	};
	ufbxt_assert(ufbxt_arraycount(element_type_sizes) == UFBX_ELEMENT_TYPE_COUNT);
	ufbxt_assert(ufbxt_arraycount(ufbx_element_type_size) == UFBX_ELEMENT_TYPE_COUNT);

	for (size_t i = 0; i < ufbxt_arraycount(element_type_sizes); i++) {
		ufbxt_element_type_size ref = element_type_sizes[i];
		ufbxt_assert(ufbx_element_type_size[(size_t)ref.type] == ref.size);
	}
}
#endif

UFBXT_TEST(evaluate_anim_null)
#if UFBXT_IMPL
{
	ufbx_real r = ufbx_evaluate_anim_value_real(NULL, 0.0);
	ufbxt_assert(r == 0.0f);

	ufbx_vec3 v = ufbx_evaluate_anim_value_vec3(NULL, 0.0);
	ufbxt_assert(v.x == 0.0f && v.y == 0.0f && v.z == 0.0f);
}
#endif

UFBXT_TEST(catch_triangulate)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_scene *scene = ufbx_load_file(path, NULL, NULL);
		ufbxt_assert(scene);

		ufbx_node *node = ufbx_find_node(scene, "pCube1");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbx_face face = mesh->faces.data[0];
		ufbxt_assert(face.index_begin == 0);
		ufbxt_assert(face.num_indices == 4);

		ufbx_panic panic;

		{
			uint32_t indices[6];
			panic.did_panic = false;
			uint32_t num_tris = ufbx_catch_triangulate_face(&panic, indices, 6, mesh, face);
			ufbxt_assert(num_tris == 2);
			ufbxt_assert(!panic.did_panic);
		}

		{
			uint32_t indices[4];
			panic.did_panic = false;
			uint32_t num_tris = ufbx_catch_triangulate_face(&panic, indices, 4, mesh, face);
			ufbxt_assert(num_tris == 0);
			ufbxt_assert(panic.did_panic);
			ufbxt_assert(!strcmp(panic.message, "Face needs at least 6 indices for triangles, got space for 4"));
		}

		{
			ufbx_face bad_face = { 100, 4 };
			uint32_t indices[6];
			panic.did_panic = false;
			uint32_t num_tris = ufbx_catch_triangulate_face(&panic, indices, 6, mesh, bad_face);
			ufbxt_assert(num_tris == 0);
			ufbxt_assert(panic.did_panic);
			ufbxt_assert(!strcmp(panic.message, "Face index begin (100) out of bounds (24)"));
		}

		{
			ufbx_face bad_face = { 22, 4 };
			uint32_t indices[6];
			panic.did_panic = false;
			uint32_t num_tris = ufbx_catch_triangulate_face(&panic, indices, 6, mesh, bad_face);
			ufbxt_assert(num_tris == 0);
			ufbxt_assert(panic.did_panic);
			ufbxt_assert(!strcmp(panic.message, "Face index end (22 + 4) out of bounds (24)"));
		}

		ufbx_free_scene(scene);
	}
}
#endif

UFBXT_TEST(catch_face_normal)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_scene *scene = ufbx_load_file(path, NULL, NULL);
		ufbxt_assert(scene);

		ufbx_node *node = ufbx_find_node(scene, "pCube1");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbx_face face = mesh->faces.data[0];
		ufbxt_assert(face.index_begin == 0);
		ufbxt_assert(face.num_indices == 4);

		ufbx_panic panic;

		{
			panic.did_panic = false;
			ufbx_catch_get_weighted_face_normal(&panic, &mesh->vertex_position, face);
			ufbxt_assert(!panic.did_panic);
		}

		{
			ufbx_face bad_face = { 100, 4 };
			panic.did_panic = false;
			ufbx_catch_get_weighted_face_normal(&panic, &mesh->vertex_position, bad_face);
			ufbxt_assert(panic.did_panic);
			ufbxt_assert(!strcmp(panic.message, "Face index begin (100) out of bounds (24)"));
		}

		{
			ufbx_face bad_face = { 22, 4 };
			panic.did_panic = false;
			ufbx_catch_get_weighted_face_normal(&panic, &mesh->vertex_position, bad_face);
			ufbxt_assert(panic.did_panic);
			ufbxt_assert(!strcmp(panic.message, "Face index end (22 + 4) out of bounds (24)"));
		}

		ufbx_free_scene(scene);
	}
}
#endif

#if UFBXT_IMPL
static bool ufbxt_open_file_fail(void *user, ufbx_stream *stream, const char *path, size_t path_len, const ufbx_open_file_info *info)
{
	return false;
}

static bool ufbxt_fail_skip(void *user, size_t size)
{
	return false;
}

static bool ufbxt_open_file_bad_skip(void *user, ufbx_stream *stream, const char *path, size_t path_len, const ufbx_open_file_info *info)
{
	bool ok = ufbx_open_file(stream, path, path_len, NULL, NULL);
	ufbxt_assert(ok);
	stream->skip_fn = &ufbxt_fail_skip;
	return true;
}
#endif

UFBXT_TEST(cache_fail_skip)
#if UFBXT_IMPL
{
	const char *path = "caches/sine_mxsf_regular/cache.xml";
	char buf[512];
	snprintf(buf, sizeof(buf), "%s%s", data_root, path);

	ufbx_geometry_cache *cache = ufbx_load_geometry_cache(buf, NULL, NULL);
	ufbxt_assert(cache);
	ufbx_retain_geometry_cache(cache);
	ufbx_free_geometry_cache(cache);

	ufbxt_assert(cache->frames.count > 2);
	ufbx_cache_frame *frame = &cache->frames.data[1];
	ufbx_real data[256];

	{
		size_t num_read = ufbx_read_geometry_cache_real(frame, data, ufbxt_arraycount(data), NULL);
		ufbxt_assert(num_read == 108);
	}

	{
		ufbx_geometry_cache_data_opts opts = { 0 };
		opts.open_file_cb.fn = &ufbxt_open_file_fail;
		size_t num_read = ufbx_read_geometry_cache_real(frame, data, ufbxt_arraycount(data), &opts);
		ufbxt_assert(num_read == 0);
	}

	{
		ufbx_geometry_cache_data_opts opts = { 0 };
		opts.open_file_cb.fn = &ufbxt_open_file_bad_skip;
		size_t num_read = ufbx_read_geometry_cache_real(frame, data, ufbxt_arraycount(data), &opts);
		ufbxt_assert(num_read == 0);
	}

	ufbx_free_geometry_cache(cache);
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_retain_w_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.retain_vertex_attrib_w = true;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(catch_get_vertex_vec, maya_color_sets, ufbxt_retain_w_opts)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbx_panic panic;
	for (size_t i = 0; i < mesh->num_indices; i++) {
		panic.did_panic = false;
		ufbx_catch_get_vertex_vec3(&panic, &mesh->vertex_position, i);
		ufbxt_assert(!panic.did_panic);
		ufbx_catch_get_vertex_vec2(&panic, &mesh->vertex_uv, i);
		ufbxt_assert(!panic.did_panic);
		ufbx_catch_get_vertex_vec4(&panic, &mesh->vertex_color, i);
		ufbxt_assert(!panic.did_panic);
		ufbx_real w = ufbx_catch_get_vertex_w_vec3(&panic, &mesh->vertex_tangent, i);
		ufbxt_assert(!panic.did_panic);
		if (scene->metadata.version >= 7000) {
			ufbxt_assert(w == 1.0f);
		} else {
			ufbxt_assert(w == 0.0f);
		}
	}

	{
		size_t index = 24;
		panic.did_panic = false;
		ufbx_catch_get_vertex_vec3(&panic, &mesh->vertex_position, index);
		ufbxt_assert(panic.did_panic);
		ufbxt_assert(!strcmp(panic.message, "index (24) out of range (24)"));
		panic.did_panic = false;
		ufbx_catch_get_vertex_vec2(&panic, &mesh->vertex_uv, index);
		ufbxt_assert(panic.did_panic);
		ufbxt_assert(!strcmp(panic.message, "index (24) out of range (24)"));
		panic.did_panic = false;
		ufbx_catch_get_vertex_vec4(&panic, &mesh->vertex_color, index);
		ufbxt_assert(panic.did_panic);
		ufbxt_assert(!strcmp(panic.message, "index (24) out of range (24)"));
		ufbx_catch_get_vertex_w_vec3(&panic, &mesh->vertex_tangent, index);
		ufbxt_assert(panic.did_panic);
		ufbxt_assert(!strcmp(panic.message, "index (24) out of range (24)"));
	}
}
#endif

UFBXT_FILE_TEST_ALT(catch_get_vertex_real, maya_vertex_crease)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbx_panic panic;
	panic.did_panic = false;
	for (size_t i = 0; i < mesh->num_indices; i++) {
		ufbx_catch_get_vertex_real(&panic, &mesh->vertex_crease, i);
		ufbxt_assert(!panic.did_panic);
	}

	{
		size_t index = 24;
		panic.did_panic = false;
		ufbx_catch_get_vertex_real(&panic, &mesh->vertex_crease, index);
		ufbxt_assert(panic.did_panic);
		ufbxt_assert(!strcmp(panic.message, "index (24) out of range (24)"));
	}
}
#endif

UFBXT_FILE_TEST_ALT(tessellate_surface_overflow, maya_nurbs_surface_plane)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "nurbsPlane1");
	ufbxt_assert(node && node->attrib_type == UFBX_ELEMENT_NURBS_SURFACE);
	ufbx_nurbs_surface *surface = ufbx_as_nurbs_surface(node->attrib);
	ufbxt_assert(surface);
	ufbx_error error;

	{
		ufbx_mesh *mesh = ufbx_tessellate_nurbs_surface(surface, NULL, NULL);
		ufbxt_assert(mesh);
		ufbx_retain_mesh(mesh);
		ufbx_free_mesh(mesh);
		ufbx_free_mesh(mesh);
	}

	for (size_t i = 1; i <= sizeof(ufbx_real); i++) {
		ufbx_tessellate_surface_opts opts = { 0 };
		opts.span_subdivision_u = SIZE_MAX / i;
		ufbx_mesh *mesh = ufbx_tessellate_nurbs_surface(surface, &opts, &error);
		ufbxt_assert(!mesh);
		ufbxt_assert(error.type == UFBX_ERROR_UNKNOWN);
	}

	for (size_t i = 1; i <= sizeof(ufbx_real); i++) {
		ufbx_tessellate_surface_opts opts = { 0 };
		opts.span_subdivision_v = SIZE_MAX / i;
		ufbx_mesh *mesh = ufbx_tessellate_nurbs_surface(surface, &opts, &error);
		ufbxt_assert(!mesh);
		ufbxt_assert(error.type == UFBX_ERROR_UNKNOWN);
	}

	for (size_t i = 1; i <= sizeof(ufbx_real) / 2; i++) {
		ufbx_tessellate_surface_opts opts = { 0 };
		opts.span_subdivision_u = opts.span_subdivision_v = (size_t)sqrt((double)SIZE_MAX) / i;
		ufbx_mesh *mesh = ufbx_tessellate_nurbs_surface(surface, &opts, &error);
		ufbxt_assert(!mesh);
		ufbxt_assert(error.type == UFBX_ERROR_UNKNOWN);
	}

}
#endif

UFBXT_FILE_TEST_ALT(tessellate_curve_overflow, max_nurbs_to_line)
#if UFBXT_IMPL
{
	ufbx_node *nurbs_node = ufbx_find_node(scene, "Nurbs");
	ufbxt_assert(nurbs_node);
	ufbx_nurbs_curve *nurbs = ufbx_as_nurbs_curve(nurbs_node->attrib);
	ufbxt_assert(nurbs);
	ufbx_error error;

	{
		ufbx_line_curve *tess_line = ufbx_tessellate_nurbs_curve(nurbs, NULL, NULL);
		ufbxt_assert(tess_line);
		ufbx_retain_line_curve(tess_line);
		ufbx_free_line_curve(tess_line);
		ufbx_free_line_curve(tess_line);
	}

	for (size_t i = 1; i <= sizeof(ufbx_real); i++) {
		ufbx_tessellate_curve_opts opts = { 0 };
		opts.span_subdivision = SIZE_MAX / i;
		ufbx_line_curve *line = ufbx_tessellate_nurbs_curve(nurbs, &opts, &error);
		ufbxt_assert(!line);
		ufbxt_assert(error.type == UFBX_ERROR_UNKNOWN);
	}
}
#endif

UFBXT_TEST(coordinate_axes_valid)
#if UFBXT_IMPL
{
	{
		ufbx_coordinate_axes axes;
		axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
		axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Y;
		axes.front = UFBX_COORDINATE_AXIS_POSITIVE_Z;
		ufbxt_assert(ufbx_coordinate_axes_valid(axes));
	}

	{
		ufbx_coordinate_axes axes;
		axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
		axes.up = UFBX_COORDINATE_AXIS_POSITIVE_X;
		axes.front = UFBX_COORDINATE_AXIS_POSITIVE_Z;
		ufbxt_assert(!ufbx_coordinate_axes_valid(axes));
	}

	{
		ufbx_coordinate_axes axes;
		axes.right = (ufbx_coordinate_axis)-1;
		axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Y;
		axes.front = UFBX_COORDINATE_AXIS_POSITIVE_Z;
		ufbxt_assert(!ufbx_coordinate_axes_valid(axes));
	}

	{
		ufbx_coordinate_axes axes;
		axes.right = (ufbx_coordinate_axis)100;
		axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Y;
		axes.front = UFBX_COORDINATE_AXIS_POSITIVE_Z;
		ufbxt_assert(!ufbx_coordinate_axes_valid(axes));
	}

	{
		ufbx_coordinate_axes axes;
		axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
		axes.up = (ufbx_coordinate_axis)-1;
		axes.front = UFBX_COORDINATE_AXIS_POSITIVE_Z;
		ufbxt_assert(!ufbx_coordinate_axes_valid(axes));
	}

	{
		ufbx_coordinate_axes axes;
		axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
		axes.up = (ufbx_coordinate_axis)100;
		axes.front = UFBX_COORDINATE_AXIS_POSITIVE_Z;
		ufbxt_assert(!ufbx_coordinate_axes_valid(axes));
	}

	{
		ufbx_coordinate_axes axes;
		axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
		axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Y;
		axes.front = (ufbx_coordinate_axis)-1;
		ufbxt_assert(!ufbx_coordinate_axes_valid(axes));
	}

	{
		ufbx_coordinate_axes axes;
		axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
		axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Y;
		axes.front = (ufbx_coordinate_axis)100;
		ufbxt_assert(!ufbx_coordinate_axes_valid(axes));
	}
}
#endif

#if UFBXT_IMPL
static size_t ufbxt_fail_read_fn(void *user, void *data, size_t size)
{
	return SIZE_MAX;
}
#endif

UFBXT_TEST(io_error)
#if UFBXT_IMPL
{
	ufbx_stream stream = { 0 };
	stream.read_fn = &ufbxt_fail_read_fn;

	ufbx_error error;
	ufbx_scene *scene = ufbx_load_stream(&stream, NULL, &error);
	ufbxt_assert(!scene);
	ufbxt_assert(error.type == UFBX_ERROR_IO);
}
#endif

UFBXT_TEST(huge_filename)
#if UFBXT_IMPL
{
	#if defined(__APPLE__)
		char filename[1024];
	#else
		char filename[2048];
	#endif
	size_t pos = 0;

	size_t root_len = strlen(data_root);
	ufbxt_assert(root_len < sizeof(filename) / 2);
	memcpy(filename + pos, data_root, root_len);
	pos += root_len;

	const char *path = "maya_cube_7500_ascii.fbx";
	size_t path_len = strlen(path);
	while (pos + path_len < sizeof(filename) - 8) {
		filename[pos + 0] = '.';
		filename[pos + 1] = '/';
		pos += 2;
	}

	memcpy(filename + pos, path, path_len + 1);

	ufbx_error error;
	ufbx_scene *scene = ufbx_load_file(filename, NULL, &error);
	if (!scene) ufbxt_log_error(&error);

	// Windows would need UNC paths here, but \\?\ does not support the relative path spam
	// we are doing here. Would need an actual long path for testing but that causes
	// portability issues as Git cannot handle it natively and creating files is not great.
	#if !defined(_WIN32)
		ufbxt_assert(scene);
	#endif

	if (scene) {
		ufbxt_check_scene(scene);
		ufbx_free_scene(scene);
	}
}
#endif

UFBXT_TEST(huge_filename_not_found)
#if UFBXT_IMPL
{
	#if defined(__APPLE__)
		char filename[1024];
	#else
		char filename[2048];
	#endif
	size_t pos = 0;

	size_t root_len = strlen(data_root);
	ufbxt_assert(root_len < sizeof(filename) / 2);
	memcpy(filename + pos, data_root, root_len);
	pos += root_len;

	const char *path = "does_not_exist.fbx";
	size_t path_len = strlen(path);
	while (pos + path_len < sizeof(filename) - 8) {
		filename[pos + 0] = '.';
		filename[pos + 1] = '/';
		pos += 2;
	}

	memcpy(filename + pos, path, path_len + 1);

	ufbx_error error;
	ufbx_scene *scene = ufbx_load_file(filename, NULL, &error);
	ufbxt_assert(!scene);

	ufbxt_assert(error.type == UFBX_ERROR_FILE_NOT_FOUND);
	ufbxt_assert(error.info_length == strlen(error.info));
	ufbxt_assert(error.info_length == UFBX_ERROR_INFO_LENGTH - 1);
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_missing_obj_mtl_path_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.obj_mtl_path.data = "nonexistent_path.mtl";
	opts.obj_mtl_path.length = SIZE_MAX;
	opts.ignore_missing_external_files = true;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(missing_explicit_mtl, blender_279_ball, ufbxt_missing_obj_mtl_path_opts, UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	if (scene->metadata.file_format == UFBX_FILE_FORMAT_OBJ) {
		ufbxt_check_warning(scene, UFBX_WARNING_MISSING_EXTERNAL_FILE, UFBX_ELEMENT_UNKNOWN, NULL, 1, "nonexistent_path.mtl");
	}
}
#endif

UFBXT_TEST(load_scene_bad_opts)
#if UFBXT_IMPL
{
#if defined(UFBX_NO_ASSERT)

	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_error error;
		ufbx_load_opts opts;
		memset(&opts, 0xff, sizeof(opts));
		ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
		ufbxt_assert(!scene);
		ufbxt_assert(error.type == UFBX_ERROR_UNINITIALIZED_OPTIONS);
	}
#endif
}
#endif

UFBXT_TEST(subdivide_bad_opts)
#if UFBXT_IMPL
{
#if defined(UFBX_NO_ASSERT)

	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_scene *scene = ufbx_load_file(path, NULL, NULL);
		ufbxt_assert(scene);
		ufbxt_assert(scene->meshes.count > 0);
		ufbx_mesh *mesh = scene->meshes.data[0];

		ufbx_error error;
		ufbx_subdivide_opts opts;
		memset(&opts, 0xff, sizeof(opts));
		ufbx_mesh *sub_mesh = ufbx_subdivide_mesh(mesh, 1, &opts, &error);
		ufbxt_assert(!sub_mesh);
		ufbxt_assert(error.type == UFBX_ERROR_UNINITIALIZED_OPTIONS);
		ufbx_free_scene(scene);
	}
#endif
}
#endif

UFBXT_TEST(evaluate_bad_opts)
#if UFBXT_IMPL
{
#if defined(UFBX_NO_ASSERT)

	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_scene *scene = ufbx_load_file(path, NULL, NULL);
		ufbxt_assert(scene);

		ufbx_error error;
		ufbx_evaluate_opts opts;
		memset(&opts, 0xff, sizeof(opts));
		ufbx_scene *state = ufbx_evaluate_scene(scene, scene->anim, 0.0, &opts, &error);
		ufbxt_assert(!state);
		ufbxt_assert(error.type == UFBX_ERROR_UNINITIALIZED_OPTIONS);
		ufbx_free_scene(scene);
	}
#endif
}
#endif

UFBXT_TEST(bake_bad_opts)
#if UFBXT_IMPL
{
#if defined(UFBX_NO_ASSERT)

	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_scene *scene = ufbx_load_file(path, NULL, NULL);
		ufbxt_assert(scene);

		ufbx_error error;
		ufbx_bake_opts opts;
		memset(&opts, 0xff, sizeof(opts));
		ufbx_baked_anim *bake = ufbx_bake_anim(scene, scene->anim, &opts, &error);
		ufbxt_assert(!bake);
		ufbxt_assert(error.type == UFBX_ERROR_UNINITIALIZED_OPTIONS);
		ufbx_free_scene(scene);
	}
#endif
}
#endif

UFBXT_TEST(tessellate_curve_bad_opts)
#if UFBXT_IMPL
{
#if defined(UFBX_NO_ASSERT)

	char path[512];
	ufbxt_file_iterator iter = { "max_nurbs_to_line" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_scene *scene = ufbx_load_file(path, NULL, NULL);
		ufbxt_assert(scene);
		ufbxt_assert(scene->nurbs_curves.count > 0);
		ufbx_nurbs_curve *nurbs = scene->nurbs_curves.data[0];

		ufbx_error error;
		ufbx_tessellate_curve_opts opts;
		memset(&opts, 0xff, sizeof(opts));
		ufbx_line_curve *line = ufbx_tessellate_nurbs_curve(nurbs, &opts, &error);
		ufbxt_assert(!line);
		ufbxt_assert(error.type == UFBX_ERROR_UNINITIALIZED_OPTIONS);
		ufbx_free_scene(scene);
	}
#endif
}
#endif

UFBXT_TEST(tessellate_surface_bad_opts)
#if UFBXT_IMPL
{
#if defined(UFBX_NO_ASSERT)

	char path[512];
	ufbxt_file_iterator iter = { "maya_nurbs_surface_plane" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_scene *scene = ufbx_load_file(path, NULL, NULL);
		ufbxt_assert(scene);
		ufbxt_assert(scene->nurbs_surfaces.count > 0);
		ufbx_nurbs_surface *nurbs = scene->nurbs_surfaces.data[0];

		ufbx_error error;
		ufbx_tessellate_surface_opts opts;
		memset(&opts, 0xff, sizeof(opts));
		ufbx_mesh *mesh = ufbx_tessellate_nurbs_surface(nurbs, &opts, &error);
		ufbxt_assert(!mesh);
		ufbxt_assert(error.type == UFBX_ERROR_UNINITIALIZED_OPTIONS);
		ufbx_free_scene(scene);
	}
#endif
}
#endif

UFBXT_TEST(load_cache_bad_opts)
#if UFBXT_IMPL
{
#if defined(UFBX_NO_ASSERT)
	const char *path = "caches/sine_mxsf_regular/cache.xml";
	char buf[512];
	snprintf(buf, sizeof(buf), "%s%s", data_root, path);

	ufbx_error error;
	ufbx_geometry_cache_opts opts;
	memset(&opts, 0xff, sizeof(opts));
	ufbx_geometry_cache *cache = ufbx_load_geometry_cache(buf, &opts, &error);
	ufbxt_assert(!cache);
	ufbxt_assert(error.type == UFBX_ERROR_UNINITIALIZED_OPTIONS);
#endif
}
#endif

UFBXT_TEST(read_cache_bad_opts)
#if UFBXT_IMPL
{
#if defined(UFBX_NO_ASSERT)
	const char *path = "caches/sine_mxsf_regular/cache.xml";
	char buf[512];
	snprintf(buf, sizeof(buf), "%s%s", data_root, path);

	ufbx_geometry_cache *cache = ufbx_load_geometry_cache(buf, NULL, NULL);
	ufbxt_assert(cache);
	ufbxt_assert(cache->frames.count > 0);
	ufbx_vec3 data[512];

	size_t result_ok = ufbx_read_geometry_cache_vec3(&cache->frames.data[0], data, ufbxt_arraycount(data), NULL);
	ufbxt_assert(result_ok == 36);

	ufbx_geometry_cache_data_opts opts;
	memset(&opts, 0xff, sizeof(opts));
	size_t result_bad = ufbx_read_geometry_cache_vec3(&cache->frames.data[0], data, ufbxt_arraycount(data), &opts);
	ufbxt_assert(result_bad == 0);

	ufbx_free_geometry_cache(cache);
#endif
}
#endif

UFBXT_TEST(sample_cache_bad_opts)
#if UFBXT_IMPL
{
#if defined(UFBX_NO_ASSERT)
	const char *path = "caches/sine_mxsf_regular/cache.xml";
	char buf[512];
	snprintf(buf, sizeof(buf), "%s%s", data_root, path);

	ufbx_geometry_cache *cache = ufbx_load_geometry_cache(buf, NULL, NULL);
	ufbxt_assert(cache);
	ufbxt_assert(cache->channels.count > 0);
	ufbx_vec3 data[512];

	size_t result_ok = ufbx_sample_geometry_cache_vec3(&cache->channels.data[0], 0.0, data, ufbxt_arraycount(data), NULL);
	ufbxt_assert(result_ok == 36);

	ufbx_geometry_cache_data_opts opts;
	memset(&opts, 0xff, sizeof(opts));
	size_t result_bad = ufbx_sample_geometry_cache_vec3(&cache->channels.data[0], 0.0, data, ufbxt_arraycount(data), &opts);
	ufbxt_assert(result_bad == 0);

	ufbx_free_geometry_cache(cache);
#endif
}
#endif

UFBXT_TEST(retain_free_garbage)
#if UFBXT_IMPL
{
	size_t buffer_size = 16*1024;
	void *data = malloc(buffer_size);
	memset(data, 0xff, buffer_size);
	ufbxt_assert(buffer_size >= sizeof(ufbx_scene) * 2);

#if defined(UFBX_NO_ASSERT)
	ufbx_retain_scene((ufbx_scene*)data);
	ufbx_free_scene((ufbx_scene*)data);
	ufbx_retain_anim((ufbx_anim*)data);
	ufbx_free_anim((ufbx_anim*)data);
	ufbx_retain_baked_anim((ufbx_baked_anim*)data);
	ufbx_free_baked_anim((ufbx_baked_anim*)data);
	ufbx_retain_mesh((ufbx_mesh*)data);
	ufbx_free_mesh((ufbx_mesh*)data);
	ufbx_retain_line_curve((ufbx_line_curve*)data);
	ufbx_free_line_curve((ufbx_line_curve*)data);
	ufbx_retain_geometry_cache((ufbx_geometry_cache*)data);
	ufbx_free_geometry_cache((ufbx_geometry_cache*)data);
#endif

	free(data);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(dom_helper_null, maya_cube, ufbxt_retain_dom_opts, UFBXT_FILE_TEST_FLAG_NO_FUZZ)
#if UFBXT_IMPL
{
	ufbxt_assert(ufbx_dom_is_array(NULL) == false);
	ufbxt_assert(ufbx_dom_array_size(NULL) == 0);
	ufbxt_assert(ufbx_dom_as_int32_list(NULL).count == 0);
	ufbxt_assert(ufbx_dom_as_int64_list(NULL).count == 0);
	ufbxt_assert(ufbx_dom_as_float_list(NULL).count == 0);
	ufbxt_assert(ufbx_dom_as_double_list(NULL).count == 0);
	ufbxt_assert(ufbx_dom_as_real_list(NULL).count == 0);
	ufbxt_assert(ufbx_dom_as_blob_list(NULL).count == 0);

	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_dom_node *dom_node = node->element.dom_node;
	ufbxt_assert(dom_node);

	ufbxt_assert(ufbx_dom_is_array(dom_node) == false);
	ufbxt_assert(ufbx_dom_array_size(dom_node) == 0);
	ufbxt_assert(ufbx_dom_as_int32_list(dom_node).count == 0);
	ufbxt_assert(ufbx_dom_as_int64_list(dom_node).count == 0);
	ufbxt_assert(ufbx_dom_as_float_list(dom_node).count == 0);
	ufbxt_assert(ufbx_dom_as_double_list(dom_node).count == 0);
	ufbxt_assert(ufbx_dom_as_real_list(dom_node).count == 0);
	ufbxt_assert(ufbx_dom_as_blob_list(dom_node).count == 0);

	ufbx_dom_node *dom_version = ufbx_dom_find(dom_node, "Version");
	ufbxt_assert(dom_version);

	ufbxt_assert(ufbx_dom_is_array(dom_version) == false);
	ufbxt_assert(ufbx_dom_array_size(dom_version) == 0);
	ufbxt_assert(ufbx_dom_as_int32_list(dom_version).count == 0);
	ufbxt_assert(ufbx_dom_as_int64_list(dom_version).count == 0);
	ufbxt_assert(ufbx_dom_as_float_list(dom_version).count == 0);
	ufbxt_assert(ufbx_dom_as_double_list(dom_version).count == 0);
	ufbxt_assert(ufbx_dom_as_real_list(dom_version).count == 0);
	ufbxt_assert(ufbx_dom_as_blob_list(dom_version).count == 0);

	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh);

	ufbx_dom_node *dom_mesh = mesh->element.dom_node;
	ufbxt_assert(dom_mesh);

	if (scene->metadata.version < 7000) {
		ufbxt_assert(dom_mesh == dom_node);
	} else {
		ufbxt_assert(dom_mesh != dom_node);
	}

	ufbx_dom_node *dom_vertices = ufbx_dom_find(dom_mesh, "Vertices");
	ufbxt_assert(ufbx_dom_is_array(dom_vertices) == true);
	ufbxt_assert(ufbx_dom_array_size(dom_vertices) == 24);
}
#endif

