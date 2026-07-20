#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "convert"

#if UFBXT_IMPL
ufbx_load_opts ufbxt_geometry_transform_helper_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_HELPER_NODES;
	return opts;
}

ufbx_load_opts ufbxt_geometry_transform_modify_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY;
	return opts;
}

ufbx_load_opts ufbxt_geometry_transform_modify_no_fallback_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY_NO_FALLBACK;
	return opts;
}

ufbx_load_opts ufbxt_geometry_transform_helper_name_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_HELPER_NODES;
	opts.geometry_transform_helper_name.data = "(ufbxt helper)";
	opts.geometry_transform_helper_name.length = SIZE_MAX;
	return opts;
}

ufbx_load_opts ufbxt_y_up_meters_root_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_axes = ufbx_axes_right_handed_y_up;
	opts.target_unit_meters = 1.0f;
	opts.space_conversion = UFBX_SPACE_CONVERSION_TRANSFORM_ROOT;
	return opts;
}

ufbx_load_opts ufbxt_y_up_meters_adjust_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_axes = ufbx_axes_right_handed_y_up;
	opts.target_unit_meters = 1.0f;
	opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
	return opts;
}

ufbx_load_opts ufbxt_z_up_meters_root_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_axes = ufbx_axes_right_handed_z_up;
	opts.target_unit_meters = 1.0f;
	opts.space_conversion = UFBX_SPACE_CONVERSION_TRANSFORM_ROOT;
	return opts;
}

ufbx_load_opts ufbxt_z_up_meters_adjust_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_axes = ufbx_axes_right_handed_z_up;
	opts.target_unit_meters = 1.0f;
	opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
	return opts;
}

ufbx_load_opts ufbxt_z_up_meters_modify_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_axes = ufbx_axes_right_handed_z_up;
	opts.target_unit_meters = 1.0f;
	opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
	return opts;
}

ufbx_load_opts ufbxt_rh_y_camera_light_axes_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_camera_axes = ufbx_axes_right_handed_y_up;
	opts.target_light_axes = ufbx_axes_right_handed_y_up;
	return opts;
}

ufbx_load_opts ufbxt_blender_space_adjust_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_axes = ufbx_axes_right_handed_z_up;
	opts.target_unit_meters = 1.0f;
	opts.target_camera_axes = ufbx_axes_right_handed_y_up;
	opts.target_light_axes = ufbx_axes_right_handed_y_up;
	opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
	return opts;
}

#endif

UFBXT_FILE_TEST(max_geometry_transform)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->nodes.count == 3);

	{
		ufbx_node *node = ufbx_find_node(scene, "Box001");
		ufbxt_assert(node);
		ufbxt_assert(!node->is_geometry_transform_helper);
		ufbxt_assert(node->geometry_transform_helper == NULL);
		ufbxt_assert(node->mesh);

		ufbxt_ref_transform local_transform = {
			{ 0.0f, -10.0f, 0.0f }, { 0.0f, 0.0f, -90.0f }, { 1.0f, 2.0f, 1.0f },
		};
		ufbxt_ref_transform geometry_transform = {
			{ 0.0f, 0.0f, 10.0f }, { 0.0f, 90.0f, 0.0f }, { 1.0f, 1.0f, 2.0f },
		};

		ufbxt_check_transform(err, "Box001 local", node->local_transform, local_transform);
		ufbxt_check_transform(err, "Box001 geometry", node->geometry_transform, geometry_transform);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Box002");
		ufbxt_assert(node);
		ufbxt_assert(!node->is_geometry_transform_helper);
		ufbxt_assert(node->geometry_transform_helper == NULL);
		ufbxt_assert(node->mesh);

		ufbxt_ref_transform local_transform = {
			{ 0.0f, 0.0f, 20.0f }, { 0.0f, 0.0f, -180.0f }, { 1.0f, 0.5f, 1.0f },
		};
		ufbxt_ref_transform geometry_transform = {
			{ 10.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f },
		};

		ufbxt_check_transform(err, "Box002 local", node->local_transform, local_transform);
		ufbxt_check_transform(err, "Box002 geometry", node->geometry_transform, geometry_transform);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(max_geometry_transform_helper, max_geometry_transform, ufbxt_geometry_transform_helper_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->nodes.count == 5);

	{
		ufbx_node *node = ufbx_find_node(scene, "Box001");
		ufbxt_assert(node);
		ufbxt_assert(!node->is_geometry_transform_helper);
		ufbxt_assert(node->geometry_transform_helper != NULL);
		ufbxt_assert(!node->mesh);

		ufbxt_ref_transform local_transform = {
			{ 0.0f, -10.0f, 0.0f }, { 0.0f, 0.0f, -90.0f }, { 1.0f, 2.0f, 1.0f },
		};
		ufbxt_ref_transform geometry_transform = {
			{ 0.0f, 0.0f, 10.0f }, { 0.0f, 90.0f, 0.0f }, { 1.0f, 1.0f, 2.0f },
		};

		ufbxt_check_transform(err, "Box001 local", node->local_transform, local_transform);
		ufbxt_check_transform(err, "Box001 geometry", node->geometry_transform, ufbxt_ref_transform_identity);

		node = node->geometry_transform_helper;
		ufbxt_assert(node->is_geometry_transform_helper);
		ufbxt_assert(node->geometry_transform_helper == NULL);
		ufbxt_assert(node->mesh);

		ufbxt_check_transform(err, "Box001 helper local", node->local_transform, geometry_transform);
		ufbxt_check_transform(err, "Box001 helper geometry", node->geometry_transform, ufbxt_ref_transform_identity);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Box002");
		ufbxt_assert(node);
		ufbxt_assert(!node->is_geometry_transform_helper);
		ufbxt_assert(node->geometry_transform_helper != NULL);
		ufbxt_assert(!node->mesh);

		ufbxt_ref_transform local_transform = {
			{ 0.0f, 0.0f, 20.0f }, { 0.0f, 0.0f, -180.0f }, { 1.0f, 0.5f, 1.0f },
		};
		ufbxt_ref_transform geometry_transform = {
			{ 10.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f },
		};

		ufbxt_check_transform(err, "Box002 local", node->local_transform, local_transform);
		ufbxt_check_transform(err, "Box002 geometry", node->geometry_transform, ufbxt_ref_transform_identity);

		node = node->geometry_transform_helper;
		ufbxt_assert(node->is_geometry_transform_helper);
		ufbxt_assert(node->geometry_transform_helper == NULL);
		ufbxt_assert(node->mesh);

		ufbxt_check_transform(err, "Box002 helper local", node->local_transform, geometry_transform);
		ufbxt_check_transform(err, "Box002 helper geometry", node->geometry_transform, ufbxt_ref_transform_identity);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(max_geometry_transform_modify, max_geometry_transform, ufbxt_geometry_transform_modify_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->nodes.count == 3);

	{
		ufbx_node *node = ufbx_find_node(scene, "Box001");
		ufbxt_assert(node);
		ufbxt_assert(!node->is_geometry_transform_helper);
		ufbxt_assert(node->geometry_transform_helper == NULL);
		ufbxt_assert(node->mesh);

		ufbxt_ref_transform local_transform = {
			{ 0.0f, -10.0f, 0.0f }, { 0.0f, 0.0f, -90.0f }, { 1.0f, 2.0f, 1.0f },
		};

		ufbxt_check_transform(err, "Box001 local", node->local_transform, local_transform);
		ufbxt_check_transform(err, "Box001 geometry", node->geometry_transform, ufbxt_ref_transform_identity);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Box002");
		ufbxt_assert(node);
		ufbxt_assert(!node->is_geometry_transform_helper);
		ufbxt_assert(node->geometry_transform_helper == NULL);
		ufbxt_assert(node->mesh);

		ufbxt_ref_transform local_transform = {
			{ 0.0f, 0.0f, 20.0f }, { 0.0f, 0.0f, -180.0f }, { 1.0f, 0.5f, 1.0f },
		};

		ufbxt_check_transform(err, "Box002 local", node->local_transform, local_transform);
		ufbxt_check_transform(err, "Box002 geometry", node->geometry_transform, ufbxt_ref_transform_identity);
	}
}
#endif

UFBXT_TEST(max_geometry_transform_lefthanded)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	ufbxt_obj_file *obj_file = ufbxt_load_obj_file("max_geometry_transform_lefthanded", NULL);

	char path[512];
	ufbxt_file_iterator iter = { "max_geometry_transform" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t axis_ix = 0; axis_ix < UFBX_MIRROR_AXIS_COUNT; axis_ix++) {
			for (uint32_t handling_ix = 0; handling_ix < UFBX_GEOMETRY_TRANSFORM_HANDLING_COUNT; handling_ix++) {
				ufbxt_hintf("handedness_conversion_axis=%u geometry_transform_handling=%u", axis_ix, handling_ix);

				ufbx_load_opts opts = { 0 };

				opts.target_axes = ufbx_axes_left_handed_z_up;
				opts.handedness_conversion_axis = (ufbx_mirror_axis)axis_ix;
				opts.geometry_transform_handling = (ufbx_geometry_transform_handling)handling_ix;
				opts.reverse_winding = axis_ix == 0;

				ufbx_error error;
				ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
				if (!scene) ufbxt_log_error(&error);
				ufbxt_assert(scene);

				ufbxt_check_scene(scene);
				ufbxt_diff_to_obj(scene, obj_file, &err, 0);

				ufbx_free_scene(scene);
			}
		}
	}

	free(obj_file);
	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
}
#endif

UFBXT_TEST(max_geometry_transform_lefthanded_adjust)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	ufbxt_obj_file *obj_file = ufbxt_load_obj_file("max_geometry_transform_lefthanded", NULL);

	char path[512];
	ufbxt_file_iterator iter = { "max_geometry_transform" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t axis_ix = 1; axis_ix < UFBX_MIRROR_AXIS_COUNT; axis_ix++) {
			for (uint32_t handling_ix = 0; handling_ix < UFBX_GEOMETRY_TRANSFORM_HANDLING_COUNT; handling_ix++) {
				ufbxt_hintf("handedness_conversion_axis=%u geometry_transform_handling=%u", axis_ix, handling_ix);

				ufbx_load_opts opts = { 0 };

				opts.target_axes = ufbx_axes_left_handed_z_up;
				opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
				opts.handedness_conversion_axis = (ufbx_mirror_axis)axis_ix;
				opts.geometry_transform_handling = (ufbx_geometry_transform_handling)handling_ix;

				ufbx_error error;
				ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
				if (!scene) ufbxt_log_error(&error);
				ufbxt_assert(scene);

				ufbxt_check_scene(scene);
				ufbxt_diff_to_obj(scene, obj_file, &err, 0);

				ufbx_free_scene(scene);
			}
		}
	}

	free(obj_file);
	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(no_unnecessary_geometry_helpers, maya_cube, ufbxt_geometry_transform_helper_opts)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->nodes.count == 2);
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);
	ufbxt_assert(!node->is_geometry_transform_helper);
	ufbxt_assert(node->geometry_transform_helper == NULL);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(max_transformed_skin_helpers, max_transformed_skin, ufbxt_geometry_transform_helper_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Box001");
	ufbxt_assert(node);
	ufbxt_assert(!node->mesh);
	ufbxt_assert(!node->is_geometry_transform_helper);
	ufbxt_assert(node->geometry_transform_helper);
	ufbxt_assert(!node->has_geometry_transform);
	ufbx_node *geo_node = node->geometry_transform_helper;
	ufbxt_assert(geo_node->mesh);
	ufbxt_assert(geo_node->is_geometry_transform_helper);
	ufbxt_assert(!geo_node->geometry_transform_helper);
	ufbxt_assert(!geo_node->has_geometry_transform);

	ufbxt_check_frame(scene, err, false, "max_transformed_skin_5", NULL, 5.0/30.0);
	ufbxt_check_frame(scene, err, false, "max_transformed_skin_15", NULL, 15.0/30.0);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(max_transformed_skin_modify, max_transformed_skin, ufbxt_geometry_transform_modify_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Box001");
	ufbxt_assert(node);
	ufbxt_assert(node->mesh);
	ufbxt_assert(!node->is_geometry_transform_helper);
	ufbxt_assert(!node->geometry_transform_helper);
	ufbxt_assert(!node->has_geometry_transform);

	ufbxt_check_frame(scene, err, false, "max_transformed_skin_5", NULL, 5.0/30.0);
	ufbxt_check_frame(scene, err, false, "max_transformed_skin_15", NULL, 15.0/30.0);
}
#endif

UFBXT_FILE_TEST(max_geometry_transform_instances)
#if UFBXT_IMPL
{
	if (scene->metadata.version >= 7000) {
		ufbxt_assert(scene->meshes.count == 1);
	} else {
		ufbxt_assert(scene->meshes.count == 4);
	}

	ufbxt_assert(scene->nodes.count == 5);
	for (size_t i = 1; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];
		ufbxt_assert(node->has_geometry_transform);
		ufbxt_assert(node->mesh);
		ufbxt_assert(!node->is_geometry_transform_helper);
		ufbxt_assert(!node->geometry_transform_helper);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(max_geometry_transform_instances_helper, max_geometry_transform_instances, ufbxt_geometry_transform_helper_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	if (scene->metadata.version >= 7000) {
		ufbxt_assert(scene->meshes.count == 1);
	} else {
		ufbxt_assert(scene->meshes.count == 4);
	}

	ufbxt_assert(scene->nodes.count == 9);
	for (size_t i = 1; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];
		ufbxt_assert(!node->has_geometry_transform);
		if (node->name.length > 0) {
			ufbxt_assert(!node->mesh);
			ufbxt_assert(!node->is_geometry_transform_helper);
			ufbxt_assert(node->geometry_transform_helper);
		} else {
			ufbxt_assert(node->mesh);
			ufbxt_assert(node->is_geometry_transform_helper);
			ufbxt_assert(!node->geometry_transform_helper);
		}
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(max_geometry_transform_instances_modify, max_geometry_transform_instances, ufbxt_geometry_transform_modify_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	if (scene->metadata.version >= 7000) {
		ufbxt_assert(scene->meshes.count == 1);
	} else {
		ufbxt_assert(scene->meshes.count == 4);
	}

	if (scene->metadata.version >= 7000) {
		ufbxt_assert(scene->nodes.count == 9);
		for (size_t i = 1; i < scene->nodes.count; i++) {
			ufbx_node *node = scene->nodes.data[i];
			ufbxt_assert(!node->has_geometry_transform);
			if (node->name.length > 0) {
				ufbxt_assert(!node->mesh);
				ufbxt_assert(!node->is_geometry_transform_helper);
				ufbxt_assert(node->geometry_transform_helper);
			} else {
				ufbxt_assert(node->mesh);
				ufbxt_assert(node->is_geometry_transform_helper);
				ufbxt_assert(!node->geometry_transform_helper);
			}
		}
	} else {
		ufbxt_assert(scene->nodes.count == 5);
		for (size_t i = 1; i < scene->nodes.count; i++) {
			ufbx_node *node = scene->nodes.data[i];
			ufbxt_assert(!node->has_geometry_transform);
			ufbxt_assert(node->mesh);
			ufbxt_assert(!node->is_geometry_transform_helper);
			ufbxt_assert(!node->geometry_transform_helper);
		}
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(max_geometry_transform_instances_modify_no_fallback, max_geometry_transform_instances, ufbxt_geometry_transform_modify_no_fallback_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS|UFBXT_FILE_TEST_FLAG_DIFF_EXPECT_FAIL_POST_7000)
#if UFBXT_IMPL
{
	if (scene->metadata.version >= 7000) {
		ufbxt_assert(scene->meshes.count == 1);
	} else {
		ufbxt_assert(scene->meshes.count == 4);
	}

	ufbxt_assert(scene->nodes.count == 5);
	for (size_t i = 1; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];
		ufbxt_assert(!node->has_geometry_transform);
		ufbxt_assert(node->mesh);
		ufbxt_assert(!node->is_geometry_transform_helper);
		ufbxt_assert(!node->geometry_transform_helper);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(max_geometry_transform_helper_names, max_geometry_transform, ufbxt_geometry_transform_helper_name_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->nodes.count == 5);

	{
		ufbx_node *node = ufbx_find_node(scene, "Box001");
		ufbxt_assert(node->geometry_transform_helper != NULL);
		node = node->geometry_transform_helper;
		ufbxt_assert(!strcmp(node->name.data, "(ufbxt helper)"));
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Box002");
		ufbxt_assert(node->geometry_transform_helper != NULL);
		node = node->geometry_transform_helper;
		ufbxt_assert(!strcmp(node->name.data, "(ufbxt helper)"));
	}
}
#endif

UFBXT_FILE_TEST(max_geometry_transform_types)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "Plane001");
		ufbxt_assert(node);
		ufbxt_check_transform(err, "Plane001 local", node->local_transform, ufbxt_ref_transform_identity);
		ufbxt_assert(node->has_geometry_transform);
		ufbxt_assert(node->attrib_type == UFBX_ELEMENT_MESH);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Line001");
		ufbxt_assert(node);
		ufbxt_check_transform(err, "Line001 local", node->local_transform, ufbxt_ref_transform_identity);
		ufbxt_assert(node->has_geometry_transform);
		if (scene->metadata.version >= 7000) {
			ufbxt_assert(node->attrib_type == UFBX_ELEMENT_LINE_CURVE);
		} else {
			ufbxt_assert(node->attrib_type == UFBX_ELEMENT_EMPTY);
		}
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Curve001");
		ufbxt_assert(node);
		ufbxt_check_transform(err, "Curve001 local", node->local_transform, ufbxt_ref_transform_identity);
		ufbxt_assert(node->has_geometry_transform);
		ufbxt_assert(node->attrib_type == UFBX_ELEMENT_NURBS_CURVE);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Surface001");
		ufbxt_assert(node);
		ufbxt_check_transform(err, "Surface001 local", node->local_transform, ufbxt_ref_transform_identity);
		ufbxt_assert(node->has_geometry_transform);
		ufbxt_assert(node->attrib_type == UFBX_ELEMENT_NURBS_SURFACE);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(max_geometry_transform_types_helper, max_geometry_transform_types, ufbxt_geometry_transform_helper_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "Plane001");
		ufbxt_assert(node);
		ufbxt_check_transform(err, "Plane001 local", node->local_transform, ufbxt_ref_transform_identity);
		ufbxt_check_transform(err, "Plane001 geometry", node->geometry_transform, ufbxt_ref_transform_identity);
		ufbxt_assert(!node->has_geometry_transform);
		ufbxt_assert(!node->attrib);
		ufbxt_assert(node->geometry_transform_helper);
		node = node->geometry_transform_helper;
		ufbxt_assert(node->attrib_type == UFBX_ELEMENT_MESH);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Line001");
		ufbxt_assert(node);
		ufbxt_check_transform(err, "Line001 local", node->local_transform, ufbxt_ref_transform_identity);
		ufbxt_check_transform(err, "Line001 geometry", node->geometry_transform, ufbxt_ref_transform_identity);
		ufbxt_assert(!node->has_geometry_transform);
		ufbxt_assert(!node->attrib);
		ufbxt_assert(node->geometry_transform_helper);
		node = node->geometry_transform_helper;
		if (scene->metadata.version >= 7000) {
			ufbxt_assert(node->attrib_type == UFBX_ELEMENT_LINE_CURVE);
		} else {
			ufbxt_assert(node->attrib_type == UFBX_ELEMENT_EMPTY);
		}
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Curve001");
		ufbxt_assert(node);
		ufbxt_check_transform(err, "Curve001 local", node->local_transform, ufbxt_ref_transform_identity);
		ufbxt_check_transform(err, "Curve001 geometry", node->geometry_transform, ufbxt_ref_transform_identity);
		ufbxt_assert(!node->has_geometry_transform);
		ufbxt_assert(!node->attrib);
		ufbxt_assert(node->geometry_transform_helper);
		node = node->geometry_transform_helper;
		ufbxt_assert(node->attrib_type == UFBX_ELEMENT_NURBS_CURVE);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Surface001");
		ufbxt_assert(node);
		ufbxt_check_transform(err, "Surface001 local", node->local_transform, ufbxt_ref_transform_identity);
		ufbxt_check_transform(err, "Surface001 geometry", node->geometry_transform, ufbxt_ref_transform_identity);
		ufbxt_assert(!node->has_geometry_transform);
		ufbxt_assert(!node->attrib);
		ufbxt_assert(node->geometry_transform_helper);
		node = node->geometry_transform_helper;
		ufbxt_assert(node->attrib_type == UFBX_ELEMENT_NURBS_SURFACE);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(max_geometry_transform_types_modify, max_geometry_transform_types, ufbxt_geometry_transform_modify_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "Plane001");
		ufbxt_assert(node);
		ufbxt_check_transform(err, "Plane001 local", node->local_transform, ufbxt_ref_transform_identity);
		ufbxt_check_transform(err, "Plane001 geometry", node->geometry_transform, ufbxt_ref_transform_identity);
		ufbxt_assert(!node->has_geometry_transform);

		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh);

		ufbxt_assert(mesh->vertex_normal.exists);
		ufbxt_assert(mesh->vertex_tangent.exists);
		ufbxt_assert(mesh->vertex_bitangent.exists);

		ufbx_vec3 ref_positions[] = {
			{ 20.0f, 10.0f, 0.0f },
			{ 0.0f, 10.0f, 10.0f },
			{ 20.0f, -10.0f, 0.0f },
		};

		ufbx_vec3 ref_normal = { 0.447213f, 0.0f, 0.894427f };
		ufbx_vec3 ref_tangent = { 0.0f, -1.0f, 0.0f };
		ufbx_vec3 ref_bitangent = { 0.894427f, 0.0f, -0.447213f };

		ufbxt_assert(mesh->num_indices == 3);
		for (size_t i = 0; i < 3; i++) {
			ufbxt_hintf("i=%zu", i);

			ufbx_vec3 position = ufbx_get_vertex_vec3(&mesh->vertex_position, i);
			ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, i);
			ufbx_vec3 tangent = ufbx_get_vertex_vec3(&mesh->vertex_tangent, i);
			ufbx_vec3 bitangent = ufbx_get_vertex_vec3(&mesh->vertex_bitangent, i);

			ufbxt_assert_close_vec3(err, position, ref_positions[i]);
			ufbxt_assert_close_vec3(err, normal, ref_normal);
			ufbxt_assert_close_vec3(err, tangent, ref_tangent);
			ufbxt_assert_close_vec3(err, bitangent, ref_bitangent);
		}
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Line001");
		ufbxt_assert(node);
		ufbxt_check_transform(err, "Line001 local", node->local_transform, ufbxt_ref_transform_identity);
		ufbxt_check_transform(err, "Line001 geometry", node->geometry_transform, ufbxt_ref_transform_identity);
		ufbxt_assert(!node->has_geometry_transform);

		ufbx_line_curve *line = ufbx_as_line_curve(node->attrib);
		if (scene->metadata.version >= 7000) {
			ufbxt_assert(line);

			ufbx_vec3 ref_positions[] = {
				{ 20.0f, -10.0f, 0.0f },
				{ 20.0f, 10.0f, 0.0f },
				{ 0.0f, 10.0f, 0.0f },
			};

			ufbxt_assert(line->point_indices.count == 3);
			ufbxt_assert(line->segments.count == 1);
			ufbxt_assert(line->segments.data[0].index_begin == 0);
			ufbxt_assert(line->segments.data[0].num_indices == 3);
			for (size_t i = 0; i < 3; i++) {
				ufbxt_hintf("i=%zu", i);

				ufbx_vec3 position = line->control_points.data[line->point_indices.data[i]];
				ufbxt_assert_close_vec3(err, position, ref_positions[i]);
			}
		}
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Curve001");
		ufbxt_assert(node);
		ufbxt_check_transform(err, "Curve001 local", node->local_transform, ufbxt_ref_transform_identity);
		ufbxt_check_transform(err, "Curve001 geometry", node->geometry_transform, ufbxt_ref_transform_identity);
		ufbxt_assert(!node->has_geometry_transform);

		ufbx_nurbs_curve *curve = ufbx_as_nurbs_curve(node->attrib);
		ufbxt_assert(curve);

		ufbx_vec4 ref_points[] = {
			{ 0.0f, 10.0f, 0.0f, 1.0f },
			{ 6.666666f, 3.333333f, 0.0f, 1.0f },
			{ 13.333333f, -3.333333f, 0.0f, 1.0f },
			{ 20.0f, -10.0f, 0.0f, 1.0f },
		};

		ufbxt_assert(curve->control_points.count == 4);
		for (size_t i = 0; i < 4; i++) {
			ufbxt_hintf("i=%zu", i);

			ufbx_vec4 point = curve->control_points.data[i];
			ufbxt_assert_close_vec4(err, point, ref_points[i]);
		}
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Surface001");
		ufbxt_assert(node);
		ufbxt_check_transform(err, "Surface001 local", node->local_transform, ufbxt_ref_transform_identity);
		ufbxt_check_transform(err, "Surface001 geometry", node->geometry_transform, ufbxt_ref_transform_identity);
		ufbxt_assert(!node->has_geometry_transform);

		ufbx_nurbs_surface *surface = ufbx_as_nurbs_surface(node->attrib);
		ufbxt_assert(surface);

		ufbxt_assert(surface->num_control_points_u == 4);
		ufbxt_assert(surface->num_control_points_v == 4);
		for (size_t v = 0; v < 4; v++) {
			for (size_t u = 0; u < 4; u++) {
				ufbxt_hintf("u=%zu, v=%zu", u, v);

				ufbx_vec4 ref_point;
				ref_point.x = (ufbx_real)v * 6.666666f;
				ref_point.y = 10.0f - (ufbx_real)u * 6.666666f;
				ref_point.z = 0.0f;
				ref_point.w = 1.0f;

				ufbx_vec4 point = surface->control_points.data[v*4 + u];
				ufbxt_assert_close_vec4(err, point, ref_point);
			}
		}
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(blender_340_z_up, UFBXT_FILE_TEST_FLAG_DIFF_EXPECT_FAIL)
#if UFBXT_IMPL
{
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(blender_340_z_up_root, blender_340_z_up, ufbxt_y_up_meters_root_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	ufbx_node *root = scene->root_node;
	ufbxt_ref_transform root_transform = {
		{ 0.0f, 0.0f, 0.0f }, { -90.0f, 0.0f, 0.0f }, { 0.01f, 0.01f, 0.01f },
	};
	ufbxt_check_transform(err, "root", root->local_transform, root_transform);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(blender_340_z_up_adjust, blender_340_z_up, ufbxt_y_up_meters_adjust_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	ufbx_node *root = scene->root_node;
	ufbxt_check_transform(err, "root", root->local_transform, ufbxt_ref_transform_identity);

	size_t num_adjusted = 0;
	for (size_t i = 0; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];
		if (node->node_depth == 1) {
			ufbxt_assert(node->has_adjust_transform);
			ufbx_vec3 rotation = { -90.0f, 0.0f, 0.0f };
			ufbx_quat rotation_quat = ufbx_euler_to_quat(rotation, UFBX_ROTATION_ORDER_XYZ);
			ufbxt_assert_close_quat(err, node->adjust_post_rotation, ufbx_identity_quat);
			ufbxt_assert_close_quat(err, node->adjust_pre_rotation, rotation_quat);
			ufbxt_assert_close_real(err, node->adjust_pre_scale, 0.01f);
			num_adjusted++;
		} else {
			ufbxt_assert(!node->has_adjust_transform);
			ufbxt_assert_close_quat(err, node->adjust_post_rotation, ufbx_identity_quat);
			ufbxt_assert_close_quat(err, node->adjust_pre_rotation, ufbx_identity_quat);
			ufbxt_assert_close_real(err, node->adjust_pre_scale, 1.0f);
		}
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Light");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 4.0f, 5.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
		};
		ufbxt_check_transform(err, "Light", node->local_transform, ref_transform);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Camera");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 7.0f, 5.0f, -6.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
		};
		ufbxt_check_transform(err, "Camera", node->local_transform, ref_transform);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Cube");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 0.0f, 1.0f, -1.0f }, { -90.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
		};
		ufbxt_check_transform(err, "Camera", node->local_transform, ref_transform);
	}

	ufbxt_assert(scene->nodes.count == 5);
	ufbxt_assert(num_adjusted == 3);
}
#endif

UFBXT_FILE_TEST_FLAGS(blender_340_y_up, UFBXT_FILE_TEST_FLAG_DIFF_EXPECT_FAIL)
#if UFBXT_IMPL
{
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(blender_340_y_up_root, blender_340_y_up, ufbxt_z_up_meters_root_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	ufbx_node *root = scene->root_node;
	ufbxt_ref_transform root_transform = {
		{ 0.0f, 0.0f, 0.0f }, { 90.0f, 0.0f, 0.0f }, { 0.01f, 0.01f, 0.01f },
	};
	ufbxt_check_transform(err, "root", root->local_transform, root_transform);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(blender_340_y_up_adjust, blender_340_y_up, ufbxt_z_up_meters_adjust_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	ufbx_node *root = scene->root_node;
	ufbxt_check_transform(err, "root", root->local_transform, ufbxt_ref_transform_identity);

	size_t num_adjusted = 0;
	for (size_t i = 0; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];
		if (node->node_depth == 1) {
			ufbxt_assert(node->has_adjust_transform);
			ufbx_vec3 rotation = { 90.0f, 0.0f, 0.0f };
			ufbx_quat rotation_quat = ufbx_euler_to_quat(rotation, UFBX_ROTATION_ORDER_XYZ);
			ufbxt_assert_close_quat(err, node->adjust_post_rotation, ufbx_identity_quat);
			ufbxt_assert_close_quat(err, node->adjust_pre_rotation, rotation_quat);
			ufbxt_assert_close_real(err, node->adjust_pre_scale, 0.01f);
			num_adjusted++;
		} else {
			ufbxt_assert(!node->has_adjust_transform);
			ufbxt_assert_close_quat(err, node->adjust_post_rotation, ufbx_identity_quat);
			ufbxt_assert_close_quat(err, node->adjust_pre_rotation, ufbx_identity_quat);
			ufbxt_assert_close_real(err, node->adjust_pre_scale, 1.0f);
		}
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Light");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 4.0f, 1.0f, 5.0f }, { 90.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
		};
		ufbxt_check_transform(err, "Light", node->local_transform, ref_transform);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Camera");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 7.0f, 6.0f, 5.0f }, { 90.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
		};
		ufbxt_check_transform(err, "Camera", node->local_transform, ref_transform);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Cube");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
		};
		ufbxt_check_transform(err, "Camera", node->local_transform, ref_transform);
	}

	ufbxt_assert(scene->nodes.count == 5);
	ufbxt_assert(num_adjusted == 3);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(blender_340_y_up_modify, blender_340_y_up, ufbxt_z_up_meters_modify_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	ufbx_node *root = scene->root_node;
	ufbxt_check_transform(err, "root", root->local_transform, ufbxt_ref_transform_identity);

	size_t num_adjusted = 0;
	for (size_t i = 0; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];
		if (node->is_root) {
			ufbxt_assert(!node->has_adjust_transform);
			ufbxt_assert_close_quat(err, node->adjust_post_rotation, ufbx_identity_quat);
			ufbxt_assert_close_quat(err, node->adjust_pre_rotation, ufbx_identity_quat);
			ufbxt_assert_close_real(err, node->adjust_translation_scale, 1.0f);
		} else if (node->node_depth == 1) {
			ufbxt_assert(node->has_adjust_transform);
			ufbx_vec3 rotation = { 90.0f, 0.0f, 0.0f };
			ufbx_quat rotation_quat = ufbx_euler_to_quat(rotation, UFBX_ROTATION_ORDER_XYZ);
			ufbxt_assert_close_quat(err, node->adjust_post_rotation, ufbx_identity_quat);
			ufbxt_assert_close_quat(err, node->adjust_pre_rotation, rotation_quat);
			ufbxt_assert_close_real(err, node->adjust_translation_scale, 0.01f);
			num_adjusted++;
		} else {
			ufbxt_assert(node->has_adjust_transform);
			ufbxt_assert_close_quat(err, node->adjust_post_rotation, ufbx_identity_quat);
			ufbxt_assert_close_quat(err, node->adjust_pre_rotation, ufbx_identity_quat);
			ufbxt_assert_close_real(err, node->adjust_translation_scale, 0.01f);
		}
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Light");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 4.0f, 1.0f, 5.0f }, { 90.0f, 0.0f, 0.0f }, { 100.0f, 100.0f, 100.0f }
		};
		ufbxt_check_transform(err, "Light", node->local_transform, ref_transform);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Camera");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 7.0f, 6.0f, 5.0f }, { 90.0f, 0.0f, 0.0f }, { 100.0f, 100.0f, 100.0f }
		};
		ufbxt_check_transform(err, "Camera", node->local_transform, ref_transform);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Cube");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 100.0f, 100.0f, 100.0f }
		};
		ufbxt_check_transform(err, "Camera", node->local_transform, ref_transform);
	}

	ufbxt_assert(scene->nodes.count == 5);
	ufbxt_assert(num_adjusted == 3);
}
#endif

UFBXT_FILE_TEST(maya_camera_light_axes_y_up)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "forwardCamera");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 0.0f, 0.0f, 0.0f }, { 0.0f, 90.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
		};
		ufbxt_check_transform(err, "forwardCamera", node->local_transform, ref_transform);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "forwardLight");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 0.0f, 0.0f, 0.0f }, { 90.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
		};
		ufbxt_check_transform(err, "forwardLight", node->local_transform, ref_transform);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(maya_camera_light_axes_y_up_rh_y, maya_camera_light_axes_y_up, ufbxt_rh_y_camera_light_axes_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "forwardCamera");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
		};
		ufbxt_check_transform(err, "forwardCamera", node->local_transform, ref_transform);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "forwardLight");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
		};
		ufbxt_check_transform(err, "forwardLight", node->local_transform, ref_transform);
	}
}
#endif

UFBXT_FILE_TEST_OPTS(blender_340_default_unscaled, ufbxt_blender_space_adjust_opts)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "Cube");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
		};
		ufbxt_check_transform(err, "Cube", node->local_transform, ref_transform);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Camera");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 7.3589f, -6.9258f, 4.9583f }, { 63.5593f, 0.0f, 46.6919f }, { 1.0f, 1.0f, 1.0f }
		};
		ufbxt_check_transform(err, "Camera", node->local_transform, ref_transform);

		ufbx_camera *camera = node->camera;
		ufbxt_assert(camera);
		ufbxt_assert(camera->projection_axes.right == UFBX_COORDINATE_AXIS_POSITIVE_X);
		ufbxt_assert(camera->projection_axes.up == UFBX_COORDINATE_AXIS_POSITIVE_Y);
		ufbxt_assert(camera->projection_axes.front == UFBX_COORDINATE_AXIS_POSITIVE_Z);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Light");
		ufbxt_assert(node);
		ufbxt_ref_transform ref_transform = {
			{ 4.0762f, 1.0055f, 5.9039f }, { 37.261f, 3.16371f, 106.936f }, { 1.0f, 1.0f, 1.0f },
		};
		ufbxt_check_transform(err, "Light", node->local_transform, ref_transform);

		ufbx_light *light = node->light;
		ufbxt_assert(light);

		ufbx_vec3 ref_dir = { 0.0f, 0.0f, -1.0f };
		ufbxt_assert_close_vec3(err, light->local_direction, ref_dir);
	}
}
#endif

UFBXT_FILE_TEST(max_geometry_transform_order)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->nodes.count == 3);
	ufbx_node **nodes = scene->nodes.data;

	ufbxt_assert(!strcmp(nodes[0]->name.data, ""));
	ufbxt_assert(nodes[0]->is_root);
	ufbxt_assert(nodes[0]->node_depth == 0);
	ufbxt_assert(nodes[0]->parent == NULL);

	ufbxt_assert(!strcmp(nodes[1]->name.data, "Box001"));
	ufbxt_assert(nodes[1]->has_geometry_transform);
	ufbxt_assert(nodes[1]->node_depth == 1);
	ufbxt_assert(nodes[1]->parent == nodes[0]);

	ufbxt_assert(!strcmp(nodes[2]->name.data, "Box002"));
	ufbxt_assert(nodes[2]->has_geometry_transform);
	ufbxt_assert(nodes[2]->node_depth == 2);
	ufbxt_assert(nodes[2]->parent == nodes[1]);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(max_geometry_transform_order_helper, max_geometry_transform_order, ufbxt_geometry_transform_helper_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->nodes.count == 5);
	ufbx_node **nodes = scene->nodes.data;

	ufbxt_assert(!strcmp(nodes[0]->name.data, ""));
	ufbxt_assert(nodes[0]->is_root);
	ufbxt_assert(nodes[0]->node_depth == 0);
	ufbxt_assert(nodes[0]->parent == NULL);

	ufbxt_assert(!strcmp(nodes[1]->name.data, "Box001"));
	ufbxt_assert(nodes[1]->geometry_transform_helper);
	ufbxt_assert(nodes[1]->node_depth == 1);
	ufbxt_assert(nodes[1]->parent == nodes[0]);

	ufbxt_assert(!strcmp(nodes[2]->name.data, ""));
	ufbxt_assert(nodes[2]->is_geometry_transform_helper);
	ufbxt_assert(nodes[2]->node_depth == 2);
	ufbxt_assert(nodes[2]->parent == nodes[1]);

	ufbxt_assert(!strcmp(nodes[3]->name.data, "Box002"));
	ufbxt_assert(nodes[3]->geometry_transform_helper);
	ufbxt_assert(nodes[3]->node_depth == 2);
	ufbxt_assert(nodes[3]->parent == nodes[1]);

	ufbxt_assert(!strcmp(nodes[4]->name.data, ""));
	ufbxt_assert(nodes[4]->is_geometry_transform_helper);
	ufbxt_assert(nodes[4]->node_depth == 3);
	ufbxt_assert(nodes[4]->parent == nodes[3]);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(max_geometry_transform_order_modify, max_geometry_transform_order, ufbxt_geometry_transform_modify_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->nodes.count == 5);
	ufbx_node **nodes = scene->nodes.data;

	ufbxt_assert(!strcmp(nodes[0]->name.data, ""));
	ufbxt_assert(nodes[0]->is_root);
	ufbxt_assert(nodes[0]->node_depth == 0);
	ufbxt_assert(nodes[0]->parent == NULL);

	ufbxt_assert(!strcmp(nodes[1]->name.data, "Box001"));
	ufbxt_assert(nodes[1]->geometry_transform_helper);
	ufbxt_assert(nodes[1]->node_depth == 1);
	ufbxt_assert(nodes[1]->parent == nodes[0]);

	ufbxt_assert(!strcmp(nodes[2]->name.data, ""));
	ufbxt_assert(nodes[2]->is_geometry_transform_helper);
	ufbxt_assert(nodes[2]->node_depth == 2);
	ufbxt_assert(nodes[2]->parent == nodes[1]);

	ufbxt_assert(!strcmp(nodes[3]->name.data, "Box002"));
	ufbxt_assert(nodes[3]->geometry_transform_helper);
	ufbxt_assert(nodes[3]->node_depth == 2);
	ufbxt_assert(nodes[3]->parent == nodes[1]);

	ufbxt_assert(!strcmp(nodes[4]->name.data, ""));
	ufbxt_assert(nodes[4]->is_geometry_transform_helper);
	ufbxt_assert(nodes[4]->node_depth == 3);
	ufbxt_assert(nodes[4]->parent == nodes[3]);
}
#endif


UFBXT_FILE_TEST_FLAGS(synthetic_geometry_transform_camera, UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Camera001");
	ufbxt_assert(node);

	ufbxt_ref_transform geo_transform = {
		{ 10.0f, 20.0f, 30.0f }, { 90.0f, 0.0f, 0.0f }, { 1.0f, 2.0f, 3.0f },
	};
	ufbxt_check_transform(err, "Camera geometry", node->geometry_transform, geo_transform);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(synthetic_geometry_transform_camera_helper, synthetic_geometry_transform_camera, ufbxt_geometry_transform_helper_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS|UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Camera001");
	ufbxt_assert(node);
	ufbxt_assert(!node->has_geometry_transform);

	ufbx_node *geo_node = node->geometry_transform_helper;
	ufbxt_assert(geo_node);

	ufbxt_ref_transform geo_transform = {
		{ 10.0f, 20.0f, 30.0f }, { 90.0f, 0.0f, 0.0f }, { 1.0f, 2.0f, 3.0f },
	};
	ufbxt_check_transform(err, "Camera geometry", geo_node->local_transform, geo_transform);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(synthetic_geometry_transform_camera_modify, synthetic_geometry_transform_camera, ufbxt_geometry_transform_modify_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS|UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Camera001");
	ufbxt_assert(node);
	ufbxt_assert(!node->has_geometry_transform);

	ufbx_node *geo_node = node->geometry_transform_helper;
	ufbxt_assert(geo_node);

	ufbxt_ref_transform geo_transform = {
		{ 10.0f, 20.0f, 30.0f }, { 90.0f, 0.0f, 0.0f }, { 1.0f, 2.0f, 3.0f },
	};
	ufbxt_check_transform(err, "Camera geometry", geo_node->local_transform, geo_transform);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(synthetic_geometry_transform_camera_modify_no_fallback, synthetic_geometry_transform_camera, ufbxt_geometry_transform_modify_no_fallback_opts,
	UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS|UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Camera001");
	ufbxt_assert(node);
	ufbxt_assert(!node->has_geometry_transform);
}
#endif

UFBXT_TEST(synthetic_unnormalized)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_diff_error err = { 0 };

	ufbxt_file_iterator iter = { "synthetic_unnormalized" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (int normalize_normals = 0; normalize_normals <= 1; normalize_normals++)
		for (int normalize_tangents = 0; normalize_tangents <= 1; normalize_tangents++) {
			ufbxt_hintf("normalize_normals=%d normalize_tangets=%d", normalize_normals, normalize_tangents);
			ufbx_load_opts opts = { 0 };
			opts.normalize_normals = normalize_normals != 0;
			opts.normalize_tangents = normalize_tangents != 0;
			ufbx_scene *scene = ufbx_load_file(path, &opts, NULL);
			ufbxt_assert(scene);
			ufbxt_check_scene(scene);

			ufbx_node *node = ufbx_find_node(scene, "pPlane1");
			ufbxt_assert(node && node->mesh);
			ufbx_mesh *mesh = node->mesh;

			ufbxt_assert(mesh->vertex_normal.values.count == 4);
			for (size_t i = 0; i < 4; i++) {
				ufbx_vec3 ref = ufbx_zero_vec3;
				if (normalize_normals) {
					ref.y = i > 0 ? 1.0f : 0.0f;
				} else {
					ref.y = (ufbx_real)i;
				}
				ufbxt_assert_close_vec3(&err, mesh->vertex_normal.values.data[i], ref);
			}

			ufbxt_assert(mesh->vertex_tangent.values.count == 4);
			for (size_t i = 0; i < 4; i++) {
				ufbx_vec3 ref = ufbx_zero_vec3;
				if (normalize_tangents) {
					ref.x = i > 0 ? 1.0f : 0.0f;
				} else {
					ref.x = (ufbx_real)i;
				}
				ufbxt_assert_close_vec3(&err, mesh->vertex_tangent.values.data[i], ref);
			}

			ufbxt_assert(mesh->vertex_bitangent.values.count == 4);
			for (size_t i = 0; i < 4; i++) {
				ufbx_vec3 ref = ufbx_zero_vec3;
				if (normalize_tangents) {
					ref.z = i > 0 ? -1.0f : 0.0f;
				} else {
					ref.z = -(ufbx_real)i;
				}
				ufbxt_assert_close_vec3(&err, mesh->vertex_bitangent.values.data[i], ref);
			}

			ufbx_free_scene(scene);
		}
	}

	if (err.num > 0) {
		double avg = err.sum / (double)err.num;
		ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", avg, err.max, err.num);
	}
}
#endif
