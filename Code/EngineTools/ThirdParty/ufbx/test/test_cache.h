#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "cache"

#if UFBXT_IMPL
static void ufbxt_test_sine_cache(ufbxt_diff_error *err, const char *path, double begin, double end, double err_threshold)
{
	char buf[512];
	snprintf(buf, sizeof(buf), "%s%s", data_root, path);

	ufbx_geometry_cache *cache = ufbx_load_geometry_cache(buf, NULL, NULL);
	ufbxt_assert(cache);
	ufbxt_assert(cache->channels.count == 2);

	bool found_cube1 = false;
	for (size_t i = 0; i < cache->channels.count; i++) {
		ufbx_cache_channel *channel = &cache->channels.data[i];
		ufbxt_assert(channel->interpretation == UFBX_CACHE_INTERPRETATION_VERTEX_POSITION);

		if (!strcmp(channel->name.data, "pCubeShape1")) {
			found_cube1 = true;

			ufbx_vec3 pos[64];
			for (double time = begin; time <= end + 0.0001; time += 0.1/24.0) {
				size_t num_verts = ufbx_sample_geometry_cache_vec3(channel, time, pos, ufbxt_arraycount(pos), NULL);
				ufbxt_assert(num_verts == 36);

				size_t dry_verts = ufbx_sample_geometry_cache_vec3(channel, time, pos, SIZE_MAX, NULL);
				ufbxt_assert(num_verts == dry_verts);

				double t = (time - 1.0/24.0) / (29.0/24.0) * 4.0;
				double pi2 = 3.141592653589793*2.0;
				double err_scale = 0.001 / err_threshold;

				for (size_t i = 0; i < num_verts; i++) {
					ufbx_vec3 v = pos[i];
					double sx = sin((v.y + t * 0.5f)*pi2) * 0.25;
					double vx = v.x;
					vx += vx > 0.0 ? -0.5 : 0.5;
					ufbxt_assert_close_double(err, vx*err_scale, sx*err_scale);
				}
			}
		}
	}

	ufbxt_assert(found_cube1);

	ufbx_free_geometry_cache(cache);
}
#endif

UFBXT_FILE_TEST(maya_cache_sine)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->cache_deformers.count == 2);
	for (size_t i = 0; i < 2; i++) {
		ufbx_cache_deformer *deformer = mesh->cache_deformers.data[i];
		ufbxt_assert(deformer->file);
		ufbxt_assert(deformer->file->format == UFBX_CACHE_FILE_FORMAT_MC);
	}

	ufbxt_check_frame(scene, err, false, "maya_cache_sine_12", NULL, 12.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_cache_sine_18", NULL, 18.0/24.0);
}
#endif

UFBXT_TEST(maya_cache_sine_caches)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	ufbxt_test_sine_cache(&err, "caches/sine_mcmf_undersample/cache.xml", 1.0/24.0, 29.0/24.0, 0.04);
	ufbxt_test_sine_cache(&err, "caches/sine_mcsd_oversample/cache.xml", 1.0/24.0, 29.0/24.0, 0.003);
	ufbxt_test_sine_cache(&err, "caches/sine_mxmd_oversample/cache.xml", 11.0/24.0, 19.0/24.0, 0.003);
	ufbxt_test_sine_cache(&err, "caches/sine_mxsf_regular/cache.xml", 1.0/24.0, 29.0/24.0, 0.008);

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
}
#endif

UFBXT_FILE_TEST(max_cache_box)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Box001");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->cache_deformers.count == 1);
	ufbx_cache_deformer *deformer = mesh->cache_deformers.data[0];
	ufbxt_assert(deformer->file);
	ufbxt_assert(deformer->file->format == UFBX_CACHE_FILE_FORMAT_PC2);
	ufbxt_assert(deformer->external_cache);
	ufbxt_assert(deformer->external_channel);
	ufbxt_assert(deformer->external_channel->frames.count == 11);
	for (size_t i = 0; i < deformer->external_channel->frames.count; i++) {
		ufbx_cache_frame *frame = &deformer->external_channel->frames.data[i];
		ufbxt_assert(frame->file_format == UFBX_CACHE_FILE_FORMAT_PC2);
	}

	ufbxt_check_frame(scene, err, false, "max_cache_box_44", NULL, 44.0/30.0);
	ufbxt_check_frame(scene, err, false, "max_cache_box_48", NULL, 48.0/30.0);
}
#endif

UFBXT_TEST(cache_xml_parse)
#if UFBXT_IMPL
{
	char buf[512];
	snprintf(buf, sizeof(buf), "%s%s", data_root, "caches/sine_xml_parse/cache.xml");

	ufbx_geometry_cache *cache = ufbx_load_geometry_cache(buf, NULL, NULL);
	ufbxt_assert(cache);

	ufbxt_assert(cache->extra_info.count == 2);
	ufbxt_check_string(cache->extra_info.data[0]);
	ufbxt_check_string(cache->extra_info.data[1]);

	const char *ex0 = "cdata! \"'&<><--&lt;&#x61;<tag></tag>-->!CDATA[]>\x61\xce\xb2\xe3\x82\xab\xf0\x9f\x98\x82]]";
	ufbxt_assert(!strcmp(cache->extra_info.data[0].data, ex0));

	const char *ex1 = "\"'&<>\x61\x61\x61\xce\xb2\xce\xb2\xce\xb2\xe3\x82\xab\xe3\x82\xab\xe3\x82\xab\xf0\x9f\x98\x82\xf0\x9f\x98\x82\xf0\x9f\x98\x82";
	ufbxt_assert(!strcmp(cache->extra_info.data[1].data, ex1));

	ufbxt_assert(cache->channels.count == 2);
	ufbxt_check_string(cache->channels.data[0].name);
	ufbxt_check_string(cache->channels.data[1].name);
	ufbxt_check_string(cache->channels.data[0].interpretation_name);
	ufbxt_check_string(cache->channels.data[1].interpretation_name);
	ufbxt_assert(cache->channels.data[0].interpretation == UFBX_CACHE_INTERPRETATION_UNKNOWN);
	ufbxt_assert(cache->channels.data[1].interpretation == UFBX_CACHE_INTERPRETATION_UNKNOWN);

	ufbxt_assert(!strcmp(cache->channels.data[0].interpretation_name.data, "<!--\"positions\"-->"));
	ufbxt_assert(!strcmp(cache->channels.data[1].interpretation_name.data, "<![CDATA[<positions>]]>"));

	ufbx_free_geometry_cache(cache);
}
#endif

UFBXT_TEST(cache_xml_depth)
#if UFBXT_IMPL
{
	char buf[512];
	snprintf(buf, sizeof(buf), "%s%s", data_root, "cache_xml_depth.xml");

	ufbx_error error;
	ufbx_geometry_cache *cache = ufbx_load_geometry_cache(buf, NULL, &error);
	ufbxt_assert(!cache);
	ufbxt_assert(error.type == UFBX_ERROR_UNKNOWN);
}
#endif

UFBXT_FILE_TEST_OPTS(marvelous_quad, ufbxt_scale_to_cm_opts)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "marvelous_quad");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh->materials.count == 1);
		ufbx_material *material = mesh->materials.data[0];

		// What? Marvelous writes relative filenames as absolute.
		// TODO: Quirk mode to fix this?
		const char *prefix = "D:\\Dev\\clean\\ufbx\\data\\";

		ufbxt_check_material_texture_ex(scene, material->fbx.ambient_color.texture, prefix, "marvelous_quad_diffuse_100-1.png", true);
		ufbxt_check_material_texture_ex(scene, material->fbx.diffuse_color.texture, prefix, "marvelous_quad_diffuse_100-1.png", true);
		ufbxt_check_material_texture_ex(scene, material->fbx.normal_map.texture, prefix, "marvelous_quad_normal_100-1.png", true);
		ufbxt_check_material_texture_ex(scene, material->pbr.base_color.texture, prefix, "marvelous_quad_diffuse_100-1.png", true);
		ufbxt_check_material_texture_ex(scene, material->pbr.normal_map.texture, prefix, "marvelous_quad_normal_100-1.png", true);
	}

	ufbxt_check_frame(scene, err, false, "marvelous_quad_12", NULL, 12.0/24.0);
	ufbxt_check_frame(scene, err, false, "marvelous_quad_22", NULL, 22.0/24.0);
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_missing_cache_fail, UFBXT_FILE_TEST_FLAG_ALLOW_ERROR)
#if UFBXT_IMPL
{
	ufbxt_assert(load_error->type == UFBX_ERROR_EXTERNAL_FILE_NOT_FOUND);
	ufbxt_assert(!strcmp(load_error->info, "missing_cache.xml"));
}
#endif

#if UFBXT_IMPL
static bool ufbxt_open_file_no_skip(void *user, ufbx_stream *stream, const char *path, size_t path_len, const ufbx_open_file_info *info)
{
	if (!ufbx_open_file(stream, path, path_len, NULL, NULL)) return false;
	stream->skip_fn = NULL;
	return true;
}
#endif

UFBXT_TEST(cache_skip_read)
#if UFBXT_IMPL
{
	char buffer[512];
	snprintf(buffer, sizeof(buffer), "%s%s", data_root, "max_cache_box_7500_binary_fpc/max_cache_box.pc2");

	ufbx_geometry_cache_opts opts = { 0 };
	opts.open_file_cb.fn = &ufbxt_open_file_no_skip;

	ufbx_error error;
	ufbx_geometry_cache *cache = ufbx_load_geometry_cache(buffer, &opts, &error);
	ufbxt_assert(cache);
	ufbxt_assert(cache->frames.count == 11);

	for (size_t i = 0; i < cache->frames.count; i++) {
		ufbx_cache_frame *frame = &cache->frames.data[0];
		ufbxt_assert(frame->file_format == UFBX_CACHE_FILE_FORMAT_PC2);

		size_t num_vertices = frame->data_count;
		ufbxt_assert(num_vertices == 770);

		ufbx_vec3 *vertices = calloc(num_vertices, sizeof(ufbx_vec3));
		ufbxt_assert(vertices);

		ufbx_geometry_cache_data_opts data_opts = { 0 };
		data_opts.open_file_cb.fn = &ufbxt_open_file_no_skip;
		size_t num_read = ufbx_read_geometry_cache_vec3(frame, vertices, num_vertices, &data_opts);
		ufbxt_assert(num_read == num_vertices);

		free(vertices);
	}

	// Retain and free for fun
	ufbx_retain_geometry_cache(cache);
	ufbx_free_geometry_cache(cache);

	ufbx_free_geometry_cache(cache);
}
#endif

