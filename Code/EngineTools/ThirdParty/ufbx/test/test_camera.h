#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "camera"

#if UFBXT_IMPL

static void ufbxt_check_ortho_camera(ufbxt_diff_error *err, ufbx_scene *scene, const char *name, ufbx_gate_fit gate_fit, ufbx_real extent, ufbx_real width, ufbx_real height)
{
	ufbxt_hintf("Cameara %s", name);
	ufbx_node *node = ufbx_find_node(scene, name);
	ufbxt_assert(node && node->camera);
	ufbx_camera *camera = node->camera;

	ufbxt_assert(camera->projection_mode == UFBX_PROJECTION_MODE_ORTHOGRAPHIC);
	ufbxt_assert(camera->gate_fit == gate_fit);
	ufbxt_assert_close_real(err, camera->orthographic_extent, extent);
	ufbxt_assert_close_real(err, camera->orthographic_size.x, width);
	ufbxt_assert_close_real(err, camera->orthographic_size.y, height);
}

#endif

UFBXT_FILE_TEST(maya_ortho_camera_400x200)
#if UFBXT_IMPL
{
	ufbxt_check_ortho_camera(err, scene, "Fill", UFBX_GATE_FIT_FILL, 30.0f, 30.0f, 15.0f);
	ufbxt_check_ortho_camera(err, scene, "Horizontal", UFBX_GATE_FIT_HORIZONTAL, 30.0f, 30.0f, 15.0f);
	ufbxt_check_ortho_camera(err, scene, "Vertical", UFBX_GATE_FIT_VERTICAL, 30.0f, 60.0f, 30.0f);
	ufbxt_check_ortho_camera(err, scene, "Overscan", UFBX_GATE_FIT_OVERSCAN, 30.0f, 60.0f, 30.0f);
}
#endif

UFBXT_FILE_TEST(maya_ortho_camera_200x300)
#if UFBXT_IMPL
{
	ufbxt_check_ortho_camera(err, scene, "Fill", UFBX_GATE_FIT_FILL, 30.0f, 20.0f, 30.0f);
	ufbxt_check_ortho_camera(err, scene, "Horizontal", UFBX_GATE_FIT_HORIZONTAL, 30.0f, 30.0f, 45.0f);
	ufbxt_check_ortho_camera(err, scene, "Vertical", UFBX_GATE_FIT_VERTICAL, 30.0f, 20.0f, 30.0f);
	ufbxt_check_ortho_camera(err, scene, "Overscan", UFBX_GATE_FIT_OVERSCAN, 30.0f, 30.0f, 45.0f);
}
#endif

UFBXT_FILE_TEST(maya_ortho_camera_size)
#if UFBXT_IMPL
{
	ufbxt_check_ortho_camera(err, scene, "Ortho_10", UFBX_GATE_FIT_FILL, 10.0f, 10.0f, 10.0f);
	ufbxt_check_ortho_camera(err, scene, "Ortho_30", UFBX_GATE_FIT_FILL, 30.0f, 30.0f, 30.0f);
	ufbxt_check_ortho_camera(err, scene, "Ortho_35", UFBX_GATE_FIT_FILL, 35.0f, 35.0f, 35.0f);
	ufbxt_check_ortho_camera(err, scene, "Ortho_100", UFBX_GATE_FIT_FILL, 100.0f, 100.0f, 100.0f);
}
#endif

UFBXT_FILE_TEST(blender_402_ortho_scale_local)
#if UFBXT_IMPL
{
	ufbxt_check_ortho_camera(err, scene, "Ortho_1", UFBX_GATE_FIT_HORIZONTAL, 1.0f, 1.0f, 1.0f);
	ufbxt_check_ortho_camera(err, scene, "Ortho_2", UFBX_GATE_FIT_HORIZONTAL, 2.0f, 2.0f, 2.0f);
	ufbxt_check_ortho_camera(err, scene, "Ortho_3", UFBX_GATE_FIT_HORIZONTAL, 3.0f, 3.0f, 3.0f);
}
#endif

UFBXT_TEST(blender_402_ortho_scale_local_conversion)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	char path[512];
	ufbxt_file_iterator iter = { "blender_402_ortho_scale_local" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t i = 0; i < UFBX_SPACE_CONVERSION_COUNT; i++) {
			ufbx_load_opts opts = { 0 };
			opts.space_conversion = i;
			opts.target_unit_meters = 1.0f;

			ufbx_scene *scene = ufbx_load_file(path, &opts, NULL);
			ufbxt_assert(scene);
			ufbxt_check_ortho_camera(&err, scene, "Ortho_1", UFBX_GATE_FIT_HORIZONTAL, 1.0f, 1.0f, 1.0f);
			ufbxt_check_ortho_camera(&err, scene, "Ortho_2", UFBX_GATE_FIT_HORIZONTAL, 2.0f, 2.0f, 2.0f);
			ufbxt_check_ortho_camera(&err, scene, "Ortho_3", UFBX_GATE_FIT_HORIZONTAL, 3.0f, 3.0f, 3.0f);
			ufbx_free_scene(scene);
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
}
#endif

UFBXT_FILE_TEST(blender_402_ortho_scale_unit)
#if UFBXT_IMPL
{
	ufbxt_check_ortho_camera(err, scene, "Ortho_1", UFBX_GATE_FIT_HORIZONTAL, 1.0f, 1.0f, 1.0f);
	ufbxt_check_ortho_camera(err, scene, "Ortho_2", UFBX_GATE_FIT_HORIZONTAL, 2.0f, 2.0f, 2.0f);
	ufbxt_check_ortho_camera(err, scene, "Ortho_3", UFBX_GATE_FIT_HORIZONTAL, 3.0f, 3.0f, 3.0f);
}
#endif

UFBXT_TEST(blender_402_ortho_scale_unit_conversion)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	char path[512];
	ufbxt_file_iterator iter = { "blender_402_ortho_scale_unit" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t i = 0; i < UFBX_SPACE_CONVERSION_COUNT; i++) {
			ufbx_load_opts opts = { 0 };
			opts.space_conversion = i;
			opts.target_unit_meters = 1.0f;

			ufbx_scene *scene = ufbx_load_file(path, &opts, NULL);
			ufbxt_assert(scene);
			ufbxt_check_ortho_camera(&err, scene, "Ortho_1", UFBX_GATE_FIT_HORIZONTAL, 1.0f, 1.0f, 1.0f);
			ufbxt_check_ortho_camera(&err, scene, "Ortho_2", UFBX_GATE_FIT_HORIZONTAL, 2.0f, 2.0f, 2.0f);
			ufbxt_check_ortho_camera(&err, scene, "Ortho_3", UFBX_GATE_FIT_HORIZONTAL, 3.0f, 3.0f, 3.0f);
			ufbx_free_scene(scene);
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
}
#endif

