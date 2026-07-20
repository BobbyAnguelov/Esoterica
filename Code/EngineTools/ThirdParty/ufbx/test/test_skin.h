#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "skin"

#if UFBXT_IMPL
void ufbxt_check_stack_times(ufbx_scene *scene, ufbxt_diff_error *err, const char *stack_name, double begin, double end)
{
	ufbx_anim_stack *stack = (ufbx_anim_stack*)ufbx_find_element(scene, UFBX_ELEMENT_ANIM_STACK, stack_name);
	ufbxt_assert(stack);
	ufbxt_assert(!strcmp(stack->name.data, stack_name));
	ufbxt_assert_close_real(err, (ufbx_real)stack->time_begin, (ufbx_real)begin);
	ufbxt_assert_close_real(err, (ufbx_real)stack->time_end, (ufbx_real)end);
}

void ufbxt_check_stack_timestamps(ufbx_scene *scene, ufbxt_diff_error *err, const char *stack_name, int64_t begin, int64_t end)
{
	ufbx_anim_stack *stack = (ufbx_anim_stack*)ufbx_find_element(scene, UFBX_ELEMENT_ANIM_STACK, stack_name);
	ufbxt_assert(stack);
	ufbxt_assert(!strcmp(stack->name.data, stack_name));

	ufbxt_assert(ufbx_find_int(&stack->props, "LocalStart", 0) == begin);
	ufbxt_assert(ufbx_find_int(&stack->props, "LocalStop", 0) == end);
	ufbxt_assert(ufbx_find_int(&stack->props, "ReferenceStart", 0) == begin);
	ufbxt_assert(ufbx_find_int(&stack->props, "ReferenceStop", 0) == end);
}
#endif

UFBXT_FILE_TEST(blender_279_sausage)
#if UFBXT_IMPL
{
	if (scene->metadata.ascii) {
		// ???: In the 6100 ASCII file Spin Take starts from -1 frames
		ufbxt_check_stack_times(scene, err, "Base", 0.0, 1.0/24.0);
		ufbxt_check_stack_times(scene, err, "Spin", -1.0/24.0, 18.0/24.0);
		ufbxt_check_stack_times(scene, err, "Wiggle", 0.0, 19.0/24.0);
	} else {
		ufbxt_check_stack_times(scene, err, "Skeleton|Base", 0.0, 1.0/24.0);
		ufbxt_check_stack_times(scene, err, "Skeleton|Spin", 0.0, 19.0/24.0);
		ufbxt_check_stack_times(scene, err, "Skeleton|Wiggle", 0.0, 19.0/24.0);
	}

	const char *cluster_names[][2] = {
		{ "Bottom", "Cluster Skin Bottom" },
		{ "Middle", "Cluster Skin Middle" },
		{ "Top", "Cluster Skin Top" },
	};
	const ufbx_matrix cluster_bind_ref[][2] = {
		{
			{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
		},
		{
			{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.9f, 0.0f },
			{ 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.9f, 0.0f, 0.0f },
		},
		{
			{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, -3.8f, 0.0f },
			{ 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, -3.8f, 0.0f, 0.0f },
		},
	};

	for (size_t i = 0; i < 3; i++) {
		ufbx_skin_cluster *cluster = (ufbx_skin_cluster*)ufbx_find_element(scene, UFBX_ELEMENT_SKIN_CLUSTER, cluster_names[i][scene->metadata.ascii]);
		ufbxt_assert(cluster);
		ufbxt_assert_close_vec3(err, cluster->geometry_to_bone.cols[0], cluster_bind_ref[i][scene->metadata.ascii].cols[0]);
		ufbxt_assert_close_vec3(err, cluster->geometry_to_bone.cols[1], cluster_bind_ref[i][scene->metadata.ascii].cols[1]);
		ufbxt_assert_close_vec3(err, cluster->geometry_to_bone.cols[2], cluster_bind_ref[i][scene->metadata.ascii].cols[2]);
		ufbxt_assert_close_vec3(err, cluster->geometry_to_bone.cols[3], cluster_bind_ref[i][scene->metadata.ascii].cols[3]);
	}

	ufbxt_check_frame(scene, err, true, "blender_279_sausage_base_0", "Base", 0.0);
	ufbxt_check_frame(scene, err, false, "blender_279_sausage_spin_15", "Spin", 15.0/24.0);
	ufbxt_check_frame(scene, err, false, "blender_279_sausage_wiggle_20", "Wiggle", 20.0/24.0);
}
#endif

UFBXT_FILE_TEST(maya_game_sausage)
#if UFBXT_IMPL
{
}
#endif

UFBXT_FILE_TEST_SUFFIX(maya_game_sausage, wiggle)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "maya_game_sausage_wiggle_10", NULL, 10.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_game_sausage_wiggle_18", NULL, 18.0/24.0);
}
#endif

UFBXT_FILE_TEST_SUFFIX(maya_game_sausage, spin)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, false, "maya_game_sausage_spin_7", NULL, 27.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_game_sausage_spin_15", NULL, 35.0/24.0);
}
#endif

UFBXT_FILE_TEST_SUFFIX(maya_game_sausage, deform)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, false, "maya_game_sausage_deform_8", NULL, 48.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_game_sausage_deform_15", NULL, 55.0/24.0);
}
#endif

UFBXT_FILE_TEST_SUFFIX(maya_game_sausage, combined)
#if UFBXT_IMPL
{
	ufbxt_check_stack_times(scene, err, "wiggle", 1.0/24.0, 20.0/24.0);
	ufbxt_check_stack_times(scene, err, "spin", 20.0/24.0, 40.0/24.0);
	ufbxt_check_stack_times(scene, err, "deform", 40.0/24.0, 60.0/24.0);

	ufbxt_check_stack_timestamps(scene, err, "wiggle", INT64_C(1924423250), INT64_C(38488465000));
	ufbxt_check_stack_timestamps(scene, err, "spin", INT64_C(38488465000), INT64_C(76976930000));
	ufbxt_check_stack_timestamps(scene, err, "deform", INT64_C(76976930000), INT64_C(115465395000));

	ufbxt_check_frame(scene, err, true, "maya_game_sausage_wiggle_10", "wiggle", 10.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_game_sausage_wiggle_18", "wiggle", 18.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_game_sausage_spin_7", "spin", 27.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_game_sausage_spin_15", "spin", 35.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_game_sausage_deform_8", "deform", 48.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_game_sausage_deform_15", "deform", 55.0/24.0);
}
#endif

UFBXT_FILE_TEST_ALT_SUFFIX(maya_game_sausage_combined_bake, maya_game_sausage, combined)
#if UFBXT_IMPL
{
	{
		ufbx_anim_stack *stack = ufbx_find_anim_stack(scene, "spin");
		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, stack->anim, NULL, NULL);
		ufbxt_assert(bake);

		ufbxt_assert(bake->key_time_min == 20.0/24.0);
		ufbxt_assert(bake->key_time_max == 40.0/24.0);

		ufbxt_assert(bake->playback_time_begin == 20.0/24.0);
		ufbxt_assert(bake->playback_time_end == 40.0/24.0);
		ufbxt_assert_close_double(err, bake->playback_duration, 40.0/24.0 - 20.0/24.0);

		ufbx_free_baked_anim(bake);
	}

	{
		ufbx_anim_stack *stack = ufbx_find_anim_stack(scene, "spin");

		ufbx_bake_opts opts = { 0 };
		opts.trim_start_time = true;

		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, stack->anim, &opts, NULL);
		ufbxt_assert(bake);

		ufbxt_assert(bake->key_time_min == 20.0/24.0);
		ufbxt_assert(bake->key_time_max == 40.0/24.0);

		ufbxt_assert(bake->playback_time_begin == 20.0/24.0);
		ufbxt_assert(bake->playback_time_end == 40.0/24.0);
		ufbxt_assert_close_double(err, bake->playback_duration, 40.0/24.0 - 20.0/24.0);

		ufbx_free_baked_anim(bake);
	}
}
#endif

UFBXT_FILE_TEST(synthetic_sausage_wiggle_no_link)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "maya_game_sausage_wiggle_10", NULL, 10.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_game_sausage_wiggle_18", NULL, 18.0/24.0);
}
#endif

UFBXT_FILE_TEST(synthetic_sausage_wiggle_no_bind)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "maya_game_sausage_wiggle_10", NULL, 10.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_game_sausage_wiggle_18", NULL, 18.0/24.0);
}
#endif

#if UFBXT_IMPL
static void ufbxt_check_bind_poses(ufbx_scene *scene, ufbxt_diff_error *err)
{
	ufbxt_assert(scene->poses.count == 1);
	ufbx_pose *pose = scene->poses.data[0];
	ufbxt_assert(pose->is_bind_pose);

	for (size_t i = 0; i < scene->skin_clusters.count; i++) {
		ufbx_skin_cluster *cluster = scene->skin_clusters.data[i];

		ufbx_bone_pose *bone_pose = NULL;
		for (size_t j = 0; j < pose->bone_poses.count; j++) {
			if (pose->bone_poses.data[j].bone_node == cluster->bone_node) {
				bone_pose = &pose->bone_poses.data[j];
				break;
			}
		}
		ufbxt_assert(bone_pose);

		ufbxt_assert_close_matrix(err, bone_pose->bone_to_world, cluster->bind_to_world);
		ufbxt_assert_close_matrix(err, bone_pose->bone_to_world, cluster->bone_node->node_to_world);
	}
}
#endif

UFBXT_TEST(synthetic_sausage_wiggle_no_link_conversion)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	char path[512];
	ufbxt_file_iterator iter = { "synthetic_sausage_wiggle_no_link" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t space_conversion_i = 0; space_conversion_i < UFBX_SPACE_CONVERSION_COUNT; space_conversion_i++) {
			ufbx_space_conversion space_conversion = (ufbx_space_conversion)space_conversion_i;
			ufbxt_hintf("space_conversion=%u", space_conversion_i);

			{
				ufbx_load_opts opts = { 0 };

				opts.space_conversion = space_conversion;
				opts.target_unit_meters = 1.0f;

				ufbx_error error;
				ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
				if (!scene) ufbxt_log_error(&error);
				ufbxt_assert(scene);

				ufbxt_check_scene(scene);
				ufbxt_check_bind_poses(scene, &err);

				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_10", NULL, 10.0/24.0, UFBXT_CHECK_FRAME_SCALE_100);
				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_18", NULL, 18.0/24.0, UFBXT_CHECK_FRAME_SCALE_100);

				ufbx_free_scene(scene);
			}

			{
				ufbx_load_opts opts = { 0 };

				opts.space_conversion = space_conversion;
				opts.target_axes = ufbx_axes_right_handed_z_up;
				opts.target_unit_meters = 1.0f;

				ufbx_error error;
				ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
				if (!scene) ufbxt_log_error(&error);
				ufbxt_assert(scene);

				ufbxt_check_scene(scene);
				ufbxt_check_bind_poses(scene, &err);

				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_10", NULL, 10.0/24.0, UFBXT_CHECK_FRAME_SCALE_100|UFBXT_CHECK_FRAME_Z_TO_Y_UP);
				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_18", NULL, 18.0/24.0, UFBXT_CHECK_FRAME_SCALE_100|UFBXT_CHECK_FRAME_Z_TO_Y_UP);

				ufbx_free_scene(scene);
			}

			for (uint32_t mirror_i = 0; mirror_i < UFBX_MIRROR_AXIS_COUNT; mirror_i++) {
				// Cannot convert handedness without mirroring if not transforming root
				if (space_conversion != UFBX_SPACE_CONVERSION_TRANSFORM_ROOT && mirror_i == 0) continue;

				ufbx_mirror_axis mirror_axis = (ufbx_mirror_axis)mirror_i;
				ufbx_load_opts opts = { 0 };

				opts.space_conversion = space_conversion;
				opts.target_axes = ufbx_axes_left_handed_y_up;
				opts.target_unit_meters = 1.0f;
				opts.handedness_conversion_axis = mirror_axis;
				opts.reverse_winding = (mirror_i == 0);

				ufbx_error error;
				ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
				if (!scene) ufbxt_log_error(&error);
				ufbxt_assert(scene);

				ufbxt_check_scene(scene);
				ufbxt_check_bind_poses(scene, &err);

				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_lefthanded_10", NULL, 10.0/24.0, UFBXT_CHECK_FRAME_SCALE_100);
				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_lefthanded_18", NULL, 18.0/24.0, UFBXT_CHECK_FRAME_SCALE_100);

				ufbx_free_scene(scene);
			}

			for (uint32_t mirror_i = 0; mirror_i < UFBX_MIRROR_AXIS_COUNT; mirror_i++) {
				// Cannot convert handedness without mirroring if not transforming root
				if (space_conversion != UFBX_SPACE_CONVERSION_TRANSFORM_ROOT && mirror_i == 0) continue;

				ufbx_mirror_axis mirror_axis = (ufbx_mirror_axis)mirror_i;
				ufbx_load_opts opts = { 0 };

				opts.space_conversion = space_conversion;
				opts.target_axes = ufbx_axes_left_handed_z_up;
				opts.target_unit_meters = 1.0f;
				opts.handedness_conversion_axis = mirror_axis;
				opts.reverse_winding = (mirror_i == 0);

				ufbx_error error;
				ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
				if (!scene) ufbxt_log_error(&error);
				ufbxt_assert(scene);

				ufbxt_check_scene(scene);
				ufbxt_check_bind_poses(scene, &err);

				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_lefthanded_10", NULL, 10.0/24.0, UFBXT_CHECK_FRAME_SCALE_100|UFBXT_CHECK_FRAME_Z_TO_Y_UP);
				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_lefthanded_18", NULL, 18.0/24.0, UFBXT_CHECK_FRAME_SCALE_100|UFBXT_CHECK_FRAME_Z_TO_Y_UP);

				ufbx_free_scene(scene);
			}
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
}
#endif

UFBXT_TEST(synthetic_sausage_wiggle_no_bind_conversion)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	char path[512];
	ufbxt_file_iterator iter = { "synthetic_sausage_wiggle_no_bind" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t space_conversion_i = 0; space_conversion_i < UFBX_SPACE_CONVERSION_COUNT; space_conversion_i++) {
			ufbx_space_conversion space_conversion = (ufbx_space_conversion)space_conversion_i;
			ufbxt_hintf("space_conversion=%u", space_conversion_i);

			{
				ufbx_load_opts opts = { 0 };

				opts.space_conversion = space_conversion;
				opts.target_unit_meters = 1.0f;

				ufbx_error error;
				ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
				if (!scene) ufbxt_log_error(&error);
				ufbxt_assert(scene);

				ufbxt_check_scene(scene);

				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_10", NULL, 10.0/24.0, UFBXT_CHECK_FRAME_SCALE_100);
				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_18", NULL, 18.0/24.0, UFBXT_CHECK_FRAME_SCALE_100);

				ufbx_free_scene(scene);
			}

			{
				ufbx_load_opts opts = { 0 };

				opts.space_conversion = space_conversion;
				opts.target_axes = ufbx_axes_right_handed_z_up;
				opts.target_unit_meters = 1.0f;

				ufbx_error error;
				ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
				if (!scene) ufbxt_log_error(&error);
				ufbxt_assert(scene);

				ufbxt_check_scene(scene);

				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_10", NULL, 10.0/24.0, UFBXT_CHECK_FRAME_SCALE_100|UFBXT_CHECK_FRAME_Z_TO_Y_UP);
				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_18", NULL, 18.0/24.0, UFBXT_CHECK_FRAME_SCALE_100|UFBXT_CHECK_FRAME_Z_TO_Y_UP);

				ufbx_free_scene(scene);
			}

			for (uint32_t mirror_i = 0; mirror_i < UFBX_MIRROR_AXIS_COUNT; mirror_i++) {
				// Cannot convert handedness without mirroring if not transforming root
				if (space_conversion != UFBX_SPACE_CONVERSION_TRANSFORM_ROOT && mirror_i == 0) continue;

				ufbx_mirror_axis mirror_axis = (ufbx_mirror_axis)mirror_i;
				ufbx_load_opts opts = { 0 };

				opts.space_conversion = space_conversion;
				opts.target_axes = ufbx_axes_left_handed_y_up;
				opts.target_unit_meters = 1.0f;
				opts.handedness_conversion_axis = mirror_axis;
				opts.reverse_winding = (mirror_i == 0);

				ufbx_error error;
				ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
				if (!scene) ufbxt_log_error(&error);
				ufbxt_assert(scene);

				ufbxt_check_scene(scene);

				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_lefthanded_10", NULL, 10.0/24.0, UFBXT_CHECK_FRAME_SCALE_100);
				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_lefthanded_18", NULL, 18.0/24.0, UFBXT_CHECK_FRAME_SCALE_100);

				ufbx_free_scene(scene);
			}

			for (uint32_t mirror_i = 0; mirror_i < UFBX_MIRROR_AXIS_COUNT; mirror_i++) {
				// Cannot convert handedness without mirroring if not transforming root
				if (space_conversion != UFBX_SPACE_CONVERSION_TRANSFORM_ROOT && mirror_i == 0) continue;

				ufbx_mirror_axis mirror_axis = (ufbx_mirror_axis)mirror_i;
				ufbx_load_opts opts = { 0 };

				opts.space_conversion = space_conversion;
				opts.target_axes = ufbx_axes_left_handed_z_up;
				opts.target_unit_meters = 1.0f;
				opts.handedness_conversion_axis = mirror_axis;
				opts.reverse_winding = (mirror_i == 0);

				ufbx_error error;
				ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
				if (!scene) ufbxt_log_error(&error);
				ufbxt_assert(scene);

				ufbxt_check_scene(scene);

				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_lefthanded_10", NULL, 10.0/24.0, UFBXT_CHECK_FRAME_SCALE_100|UFBXT_CHECK_FRAME_Z_TO_Y_UP);
				ufbxt_check_frame_imp(scene, &err, true, "maya_game_sausage_wiggle_lefthanded_18", NULL, 18.0/24.0, UFBXT_CHECK_FRAME_SCALE_100|UFBXT_CHECK_FRAME_Z_TO_Y_UP);

				ufbx_free_scene(scene);
			}
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
}
#endif

UFBXT_FILE_TEST(maya_blend_shape_cube)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->blend_deformers.count == 1);
	ufbx_blend_deformer *deformer = mesh->blend_deformers.data[0];
	ufbxt_assert(deformer);

	ufbx_blend_channel *top[2] = { NULL, NULL };
	for (size_t i = 0; i < deformer->channels.count; i++) {
		ufbx_blend_channel *chan = deformer->channels.data[i];
		if (strstr(chan->name.data, "TopH")) top[0] = chan;
		if (strstr(chan->name.data, "TopV")) top[1] = chan;
	}
	ufbxt_assert(top[0] && top[1]);

	ufbxt_assert_close_real(err, top[0]->weight, 1.0);
	ufbxt_assert_close_real(err, top[1]->weight, 1.0);

	double keyframes[][3] = {
		{ 1.0/24.0, 1.0, 1.0 },
		{ 16.0/24.0, 0.279, 0.670 },
		{ 53.0/24.0, 0.901, 0.168 },
		{ 120.0/24.0, 1.0, 1.0 },
	};
	ufbx_vec3 ref_offsets[2][8] = {
		{ {0,0,0},{0,0,0},{0.317f,0,0},{-0.317f,0,0},{0.317f,0,0},{-0.317f,0,0},{0,0,0},{0,0,0}, },
		{ {0,0,0},{0,0,0},{0,0,-0.284f},{0,0,-0.284f},{0,0,0.284f},{0,0,0.284f},{0,0,0},{0,0,0}, },
	};

	for (size_t chan_ix = 0; chan_ix < 2; chan_ix++) {
		ufbx_blend_channel *chan = top[chan_ix];
		ufbxt_assert(chan->keyframes.count == 1);

		ufbxt_assert_close_real(err, chan->keyframes.data[0].target_weight, 1.0);
		ufbx_blend_shape *shape = chan->keyframes.data[0].shape;

		ufbx_vec3 offsets[8] = { 0 };
		ufbx_add_blend_shape_vertex_offsets(shape, offsets, 8, 1.0f);
		for (size_t i = 0; i < 8; i++) {
			ufbx_vec3 ref = ref_offsets[chan_ix][i];
			ufbx_vec3 off_a = offsets[i];
			ufbx_vec3 off_b = ufbx_get_blend_shape_vertex_offset(shape, i);
			ufbxt_assert_close_vec3(err, ref, off_a);
			ufbxt_assert_close_vec3(err, ref, off_b);
		}

		for (size_t key_ix = 0; key_ix < ufbxt_arraycount(keyframes); key_ix++) {
			double *frame = keyframes[key_ix];
			double time = frame[0];
			ufbx_real ref = (ufbx_real)frame[1 + chan_ix];

			ufbx_real weight = ufbx_evaluate_blend_weight(scene->anim, chan, time);
			ufbxt_assert_close_real(err, weight, ref);

			ufbx_prop prop = ufbx_evaluate_prop(scene->anim, &chan->element, "DeformPercent", time);
			ufbxt_assert_close_real(err, prop.value_real / 100.0f, ref);
		}
	}

	{
		ufbx_vec3 offsets[8] = { 0 };
		ufbx_add_blend_vertex_offsets(deformer, offsets, 8, 1.0f);
		for (size_t i = 0; i < 8; i++) {
			ufbx_vec3 ref = ufbxt_add3(ref_offsets[0][i], ref_offsets[1][i]);
			ufbxt_assert_close_vec3(err, ref, offsets[i]);
		}
	}

	for (int eval_skin = 0; eval_skin <= 1; eval_skin++) {
		for (size_t key_ix = 0; key_ix < ufbxt_arraycount(keyframes); key_ix++) {
			double *frame = keyframes[key_ix];
			double time = frame[0];

			ufbx_evaluate_opts opts = { 0 };

			opts.evaluate_skinning = eval_skin != 0;

			ufbx_scene *state = ufbx_evaluate_scene(scene, scene->anim, time, &opts, NULL);
			ufbxt_assert(state);

			ufbx_real weights[2];
			for (size_t chan_ix = 0; chan_ix < 2; chan_ix++) {
				ufbx_real ref = (ufbx_real)frame[1 + chan_ix];

				ufbx_blend_channel *chan = state->blend_channels.data[top[chan_ix]->typed_id];
				ufbxt_assert(chan);

				ufbx_prop prop = ufbx_evaluate_prop(scene->anim, &chan->element, "DeformPercent", time);

				ufbxt_assert_close_real(err, prop.value_real / 100.0f, ref);
				ufbxt_assert_close_real(err, chan->weight, ref);
				weights[chan_ix] = chan->weight;
			}

			if (eval_skin) {
				ufbx_blend_deformer *eval_deformer = state->blend_deformers.data[deformer->typed_id];
				ufbx_mesh *eval_mesh = state->meshes.data[mesh->typed_id];
				for (size_t i = 0; i < 8; i++) {
					ufbx_vec3 original_pos = eval_mesh->vertex_position.values.data[i];
					ufbx_vec3 skinned_pos = eval_mesh->skinned_position.values.data[i];
					ufbx_vec3 ref = original_pos;
					ufbx_vec3 blend_pos = ufbx_get_blend_vertex_offset(eval_deformer, i);
					ref = ufbxt_add3(ref, ufbxt_mul3(ref_offsets[0][i], weights[0]));
					ref = ufbxt_add3(ref, ufbxt_mul3(ref_offsets[1][i], weights[1]));
					blend_pos = ufbxt_add3(original_pos, blend_pos);
					ufbxt_assert_close_vec3(err, ref, skinned_pos);
					ufbxt_assert_close_vec3(err, ref, blend_pos);
				}
			}

			ufbxt_check_scene(state);

			ufbx_free_scene(state);
		}
	}
}
#endif

UFBXT_FILE_TEST(maya_blend_inbetween)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, false, "maya_blend_inbetween_1", NULL, 1.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_blend_inbetween_30", NULL, 30.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_blend_inbetween_60", NULL, 60.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_blend_inbetween_65", NULL, 65.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_blend_inbetween_71", NULL, 71.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_blend_inbetween_80", NULL, 80.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_blend_inbetween_89", NULL, 89.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_blend_inbetween_120", NULL, 120.0/24.0);
}
#endif

UFBXT_FILE_TEST(synthetic_blend_shape_order)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pPlatonic1");
	ufbxt_assert(node);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh);

	static const ufbx_vec3 ref[] = {
		{ -0.041956f, -0.004164f, -1.064113f },
		{  0.736301f,  0.734684f, -0.451944f },
		{ -0.263699f,  1.059604f, -0.451944f },
		{ -0.951595f, -0.168521f, -0.372847f },
		{ -0.333561f, -1.019172f, -0.372847f },
		{  1.004034f, -0.525731f, -0.447214f },
		{  1.019802f, -0.010427f,  0.466597f },
		{  0.289087f,  1.059604f,  0.442483f },
		{ -0.816271f,  0.564766f,  0.459767f },
		{ -0.870921f, -0.699814f,  0.400384f },
		{  0.514865f, -0.854815f,  0.383101f },
		{  0.238471f, -0.004164f,  0.935887f },
	};

	ufbx_blend_shape *shape = (ufbx_blend_shape*)ufbx_find_element(scene, UFBX_ELEMENT_BLEND_SHAPE, "Target");
	ufbxt_assert(shape);

	ufbx_vec3 pos[12];
	memcpy(pos, mesh->vertices.data, sizeof(pos));
	ufbx_add_blend_shape_vertex_offsets(shape, pos, 12, 0.5f);

	for (size_t i = 0; i < mesh->num_vertices; i++) {
		ufbx_vec3 vert = mesh->vertices.data[i];
		ufbx_vec3 off = ufbx_get_blend_shape_vertex_offset(shape, i);
		ufbx_vec3 res = { vert.x+off.x*0.5f, vert.y+off.y*0.5f, vert.z+off.z*0.5f };
		ufbxt_assert_close_vec3(err, res, ref[i]);
		ufbxt_assert_close_vec3(err, pos[i], ref[i]);
	}
}
#endif

UFBXT_FILE_TEST(blender_293_half_skinned)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Plane");
	ufbxt_assert(node);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh);
	ufbxt_assert(mesh->num_vertices == 4);
	ufbxt_assert(mesh->skin_deformers.count == 1);
	ufbx_skin_deformer *skin = mesh->skin_deformers.data[0];
	ufbxt_assert(skin->vertices.count == 4);
	ufbxt_assert(skin->vertices.data[0].num_weights == 1);
	ufbxt_assert(skin->vertices.data[0].weight_begin == 0);
	ufbxt_assert(skin->vertices.data[1].num_weights == 1);
	ufbxt_assert(skin->vertices.data[1].weight_begin == 1);
	ufbxt_assert(skin->vertices.data[2].num_weights == 0);
	ufbxt_assert(skin->vertices.data[3].num_weights == 0);
}
#endif

UFBXT_FILE_TEST(maya_dual_quaternion)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->skin_deformers.count == 1);
	ufbx_skin_deformer *skin = mesh->skin_deformers.data[0];
	ufbxt_assert(skin->skinning_method == UFBX_SKINNING_METHOD_DUAL_QUATERNION);
}
#endif

UFBXT_FILE_TEST(maya_dual_quaternion_scale)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->skin_deformers.count == 1);
	ufbx_skin_deformer *skin = mesh->skin_deformers.data[0];
	ufbxt_assert(skin->skinning_method == UFBX_SKINNING_METHOD_DUAL_QUATERNION);
}
#endif

UFBXT_FILE_TEST(maya_dq_weights)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, false, "maya_dq_weights_10", NULL, 10.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_dq_weights_18", NULL, 18.0/24.0);
}
#endif

UFBXT_FILE_TEST(maya_bone_radius)
#if UFBXT_IMPL
{
	for (size_t i = 1; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];
		ufbxt_assert(strstr(node->name.data, "joint"));
		ufbx_bone *bone = ufbx_as_bone(node->attrib);
		ufbxt_assert(bone);
		ufbxt_assert_close_real(err, bone->radius, (ufbx_real)i * 0.2f);
		ufbxt_assert_close_real(err, bone->relative_length, 1.0f);
	}
}
#endif

UFBXT_FILE_TEST(blender_279_bone_radius)
#if UFBXT_IMPL
{
	ufbx_node *armature = ufbx_find_node(scene, "Armature");
	ufbx_node *node = armature;

	for (size_t i = 0; node->children.count > 0; i++) {
		node = node->children.data[0];

		ufbxt_assert(strstr(node->name.data, "Bone"));
		ufbx_bone *bone = ufbx_as_bone(node->attrib);
		ufbxt_assert(bone);

		// Blender end bones have some weird scaling factor
		// TODO: Add quirks mode for this?
		if (strstr(node->name.data, "end")) continue;

		if (scene->metadata.ascii) {
			ufbxt_assert_close_real(err, bone->radius, 1.0f);
		} else {
			ufbxt_assert_close_real(err, bone->radius, (ufbx_real)(i + 1) * 0.1f);
		}
		ufbxt_assert_close_real(err, bone->relative_length, 1.0f);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_broken_cluster, UFBXT_FILE_TEST_FLAG_ALLOW_STRICT_ERROR)
#if UFBXT_IMPL
{
	ufbx_node *cube = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(cube && cube->mesh);
	ufbx_mesh *mesh = cube->mesh;
	ufbxt_assert(mesh->skin_deformers.count == 1);
	ufbx_skin_deformer *skin = mesh->skin_deformers.data[0];
	ufbxt_assert(skin->clusters.count == 2);
	ufbxt_assert(!strcmp(skin->clusters.data[0]->bone_node->name.data, "joint3"));
	ufbxt_assert(!strcmp(skin->clusters.data[1]->bone_node->name.data, "joint1"));
}
#endif

UFBXT_TEST(synthetic_broken_cluster_connect)
#if UFBXT_IMPL
{
	char path[512];

	ufbxt_file_iterator iter = { "synthetic_broken_cluster" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_load_opts opts = { 0 };
		opts.connect_broken_elements = true;
		ufbx_scene *scene = ufbx_load_file(path, &opts, NULL);
		ufbxt_assert(scene);

		ufbx_node *cube = ufbx_find_node(scene, "pCube1");
		ufbxt_assert(cube && cube->mesh);
		ufbx_mesh *mesh = cube->mesh;
		ufbxt_assert(mesh->skin_deformers.count == 1);
		ufbx_skin_deformer *skin = mesh->skin_deformers.data[0];
		ufbxt_assert(skin->clusters.count == 3);
		ufbxt_assert(!strcmp(skin->clusters.data[0]->bone_node->name.data, "joint3"));
		ufbxt_assert(skin->clusters.data[1]->bone_node == NULL);
		ufbxt_assert(!strcmp(skin->clusters.data[2]->bone_node->name.data, "joint1"));

		// HACK: Patch the bone of `clusters.data[1]` to be valid for `ufbxt_check_scene()`...
		skin->clusters.data[1]->bone_node = skin->clusters.data[0]->bone_node;

		ufbxt_check_scene(scene);

		ufbx_free_scene(scene);
	}
}
#endif

UFBXT_TEST(synthetic_broken_cluster_safe)
#if UFBXT_IMPL
{
	char path[512];

	// Same test as above but check that the scene is valid if we don't
	// pass `connect_broken_elements`.
	ufbxt_file_iterator iter = { "synthetic_broken_cluster" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_load_opts opts = { 0 };
		ufbx_scene *scene = ufbx_load_file(path, &opts, NULL);
		ufbxt_assert(scene);
		ufbxt_check_scene(scene);

		ufbx_free_scene(scene);
	}
}
#endif

UFBXT_FILE_TEST(max_transformed_skin)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, false, "max_transformed_skin_5", NULL, 5.0/30.0);
	ufbxt_check_frame(scene, err, false, "max_transformed_skin_15", NULL, 15.0/30.0);
}
#endif

UFBXT_TEST(max_transformed_skin_lefthanded)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	char path[512];
	ufbxt_file_iterator iter = { "max_transformed_skin" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t i = 0; i < UFBX_MIRROR_AXIS_COUNT; i++) {
			ufbxt_hintf("mirror_axis=%u", i);

			ufbx_load_opts opts = { 0 };

			opts.target_axes = ufbx_axes_left_handed_z_up;
			opts.handedness_conversion_axis = (ufbx_mirror_axis)i;

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			if (!scene) ufbxt_log_error(&error);
			ufbxt_assert(scene);

			ufbxt_check_scene(scene);
			ufbxt_check_frame(scene, &err, false, "max_transformed_skin_lefthanded_5", NULL, 5.0/30.0);
			ufbxt_check_frame(scene, &err, false, "max_transformed_skin_lefthanded_15", NULL, 15.0/30.0);

			ufbxt_check_transform_consistency(&err, scene, 30.0, 30, 0);

			ufbx_free_scene(scene);
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
}
#endif

UFBXT_TEST(max_transformed_skin_lefthanded_adjust)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	char path[512];
	ufbxt_file_iterator iter = { "max_transformed_skin" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t i = 1; i < UFBX_MIRROR_AXIS_COUNT; i++) {
			ufbxt_hintf("mirror_axis=%u", i);

			ufbx_load_opts opts = { 0 };

			opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
			opts.target_axes = ufbx_axes_left_handed_z_up;
			opts.handedness_conversion_axis = (ufbx_mirror_axis)i;

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			if (!scene) ufbxt_log_error(&error);
			ufbxt_assert(scene);

			ufbxt_check_scene(scene);
			ufbxt_check_frame(scene, &err, false, "max_transformed_skin_lefthanded_5", NULL, 5.0/30.0);
			ufbxt_check_frame(scene, &err, false, "max_transformed_skin_lefthanded_15", NULL, 15.0/30.0);

			ufbxt_check_transform_consistency(&err, scene, 30.0, 30, 0);

			ufbx_free_scene(scene);
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_bind_to_root, UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbxt_check_warning(scene, UFBX_WARNING_BAD_ELEMENT_CONNECTED_TO_ROOT, UFBX_ELEMENT_SKIN_DEFORMER, "", SIZE_MAX, NULL);
	// Some unknown exporter is exporting skin deformers being parented to root
	// This test exists to check that it is handled gracefully if quirks are enabled
}
#endif

UFBXT_FILE_TEST(maya_mixed_inherit_mode)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_0", NULL, 0.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_1", NULL, 1.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_2", NULL, 2.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_3", NULL, 3.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_4", NULL, 4.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_5", NULL, 5.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_6", NULL, 6.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_7", NULL, 7.0/24.0);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_mixed_inherit_mode_helper, maya_mixed_inherit_mode, ufbxt_scale_helper_opts)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_0", NULL, 0.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_1", NULL, 1.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_2", NULL, 2.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_3", NULL, 3.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_4", NULL, 4.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_5", NULL, 5.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_6", NULL, 6.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_mixed_inherit_mode_7", NULL, 7.0/24.0);

	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, NULL, NULL);
	ufbxt_assert(bake);

	for (int frame = 0; frame <= 7; frame++) {
		double time = (double)frame / 24.0;
		ufbx_scene *state = ufbx_evaluate_scene(scene, NULL, time, NULL, NULL);
		ufbxt_assert(state);

		for (size_t i = 0; i < scene->nodes.count; i++) {
			ufbx_node *scene_node = scene->nodes.data[i];
			ufbx_node *state_node = state->nodes.data[i];

			ufbx_transform transform = ufbx_evaluate_transform(scene->anim, scene_node, time);
			ufbxt_assert_close_vec3(err, transform.translation, state_node->local_transform.translation);
			ufbxt_assert_close_quat(err, transform.rotation, state_node->local_transform.rotation);
			ufbxt_assert_close_vec3(err, transform.scale, state_node->local_transform.scale);

			ufbx_matrix ref_matrix = state_node->node_to_world;
			ufbx_matrix bake_matrix = ufbxt_evaluate_baked_matrix(bake, scene_node, time);

			ufbxt_assert_close_vec3(err, ref_matrix.cols[0], bake_matrix.cols[0]);
			ufbxt_assert_close_vec3(err, ref_matrix.cols[1], bake_matrix.cols[1]);
			ufbxt_assert_close_vec3(err, ref_matrix.cols[2], bake_matrix.cols[2]);
			ufbxt_assert_close_vec3(err, ref_matrix.cols[3], bake_matrix.cols[3]);
		}

		ufbx_free_scene(state);
	}

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_sausage_rrss, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, false, "motionbuilder_sausage_rrss", NULL, 0.0/24.0);
	ufbxt_check_frame(scene, err, false, "motionbuilder_sausage_rrss_12", NULL, 12.0/24.0);
	ufbxt_check_frame(scene, err, false, "motionbuilder_sausage_rrss_24", NULL, 24.0/24.0);
	ufbxt_check_frame(scene, err, false, "motionbuilder_sausage_rrss_36", NULL, 36.0/24.0);
	ufbxt_check_frame(scene, err, false, "motionbuilder_sausage_rrss_48", NULL, 48.0/24.0);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(motionbuilder_sausage_rrss_helper, motionbuilder_sausage_rrss, ufbxt_scale_helper_opts, UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS|UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE|UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, false, "motionbuilder_sausage_rrss", NULL, 0.0/24.0);
	ufbxt_check_frame(scene, err, false, "motionbuilder_sausage_rrss_12", NULL, 12.0/24.0);
	ufbxt_check_frame(scene, err, false, "motionbuilder_sausage_rrss_24", NULL, 24.0/24.0);
	ufbxt_check_frame(scene, err, false, "motionbuilder_sausage_rrss_36", NULL, 36.0/24.0);
	ufbxt_check_frame(scene, err, false, "motionbuilder_sausage_rrss_48", NULL, 48.0/24.0);

	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, NULL, NULL);
	ufbxt_assert(bake);

	for (int frame = 0; frame <= 48; frame += 12) {
		double time = (double)frame / 24.0;
		ufbx_scene *state = ufbx_evaluate_scene(scene, NULL, time, NULL, NULL);
		ufbxt_assert(state);

		for (size_t i = 0; i < scene->nodes.count; i++) {
			ufbx_node *scene_node = scene->nodes.data[i];
			ufbx_node *state_node = state->nodes.data[i];

			ufbx_transform transform = ufbx_evaluate_transform(scene->anim, scene_node, time);
			ufbxt_assert_close_vec3(err, transform.translation, state_node->local_transform.translation);
			ufbxt_assert_close_quat(err, transform.rotation, state_node->local_transform.rotation);
			ufbxt_assert_close_vec3(err, transform.scale, state_node->local_transform.scale);

			ufbx_matrix ref_matrix = state_node->node_to_world;
			ufbx_matrix bake_matrix = ufbxt_evaluate_baked_matrix(bake, scene_node, time);

			ufbxt_assert_close_vec3(err, ref_matrix.cols[0], bake_matrix.cols[0]);
			ufbxt_assert_close_vec3(err, ref_matrix.cols[1], bake_matrix.cols[1]);
			ufbxt_assert_close_vec3(err, ref_matrix.cols[2], bake_matrix.cols[2]);
			ufbxt_assert_close_vec3(err, ref_matrix.cols[3], bake_matrix.cols[3]);
		}

		ufbx_free_scene(state);
	}

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST(maya_poses)
#if UFBXT_IMPL
{
	{
		ufbx_pose *pose = ufbx_as_pose(ufbx_find_element(scene, UFBX_ELEMENT_POSE, "skinCluster1"));
		ufbxt_assert(pose);

		ufbx_transform_override transform_overrides[5];
		ufbxt_assert(pose->bone_poses.count == ufbxt_arraycount(transform_overrides));

		for (size_t i = 0; i < pose->bone_poses.count; i++) {
			ufbx_bone_pose *bone = &pose->bone_poses.data[i];
			ufbx_transform transform = ufbx_matrix_to_transform(&bone->bone_to_parent);
			transform_overrides[i].node_id = bone->bone_node->typed_id;
			transform_overrides[i].transform = transform;
		}

		ufbx_anim_opts opts = { 0 };
		opts.transform_overrides.data = transform_overrides;
		opts.transform_overrides.count = ufbxt_arraycount(transform_overrides);
		ufbx_anim *anim = ufbx_create_anim(scene, &opts, NULL);
		ufbxt_assert(anim);

		ufbx_evaluate_opts eval_opts = { 0 };
		eval_opts.evaluate_skinning = true;

		ufbx_scene *state = ufbx_evaluate_scene(scene, anim, 0.0, &eval_opts, NULL);
		ufbxt_assert(state);

		ufbxt_obj_file *obj_file = ufbxt_load_obj_file("maya_poses_bind1", NULL);

		ufbxt_diff_to_obj(state, obj_file, err, 0);

		free(obj_file);
		ufbx_free_scene(state);
		ufbx_free_anim(anim);
	}

	{
		ufbx_pose *pose = ufbx_as_pose(ufbx_find_element(scene, UFBX_ELEMENT_POSE, "skinCluster2"));
		ufbxt_assert(pose);

		ufbx_transform_override transform_overrides[5];
		ufbxt_assert(pose->bone_poses.count == ufbxt_arraycount(transform_overrides));

		for (size_t i = 0; i < pose->bone_poses.count; i++) {
			ufbx_bone_pose *bone = &pose->bone_poses.data[i];
			ufbx_transform transform = ufbx_matrix_to_transform(&bone->bone_to_parent);
			transform_overrides[i].node_id = bone->bone_node->typed_id;
			transform_overrides[i].transform = transform;
		}

		ufbx_anim_opts opts = { 0 };
		opts.transform_overrides.data = transform_overrides;
		opts.transform_overrides.count = ufbxt_arraycount(transform_overrides);
		ufbx_anim *anim = ufbx_create_anim(scene, &opts, NULL);
		ufbxt_assert(anim);

		ufbx_evaluate_opts eval_opts = { 0 };
		eval_opts.evaluate_skinning = true;

		ufbx_scene *state = ufbx_evaluate_scene(scene, anim, 0.0, &eval_opts, NULL);
		ufbxt_assert(state);

		ufbxt_obj_file *obj_file = ufbxt_load_obj_file("maya_poses_bind2", NULL);

		ufbxt_diff_to_obj(state, obj_file, err, 0);

		free(obj_file);
		ufbx_free_scene(state);
		ufbx_free_anim(anim);
	}
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_lefthanded_y_up_z_flip_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_axes = ufbx_axes_left_handed_y_up;
	opts.handedness_conversion_axis = UFBX_MIRROR_AXIS_Z;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(maya_poses_lefthanded, maya_poses, ufbxt_lefthanded_y_up_z_flip_opts, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
	{
		ufbx_pose *pose = ufbx_as_pose(ufbx_find_element(scene, UFBX_ELEMENT_POSE, "skinCluster1"));
		ufbxt_assert(pose);

		ufbx_transform_override transform_overrides[5];
		ufbxt_assert(pose->bone_poses.count == ufbxt_arraycount(transform_overrides));

		for (size_t i = 0; i < pose->bone_poses.count; i++) {
			ufbx_bone_pose *bone = &pose->bone_poses.data[i];
			ufbx_transform transform = ufbx_matrix_to_transform(&bone->bone_to_parent);
			transform_overrides[i].node_id = bone->bone_node->typed_id;
			transform_overrides[i].transform = transform;
		}

		ufbx_anim_opts opts = { 0 };
		opts.transform_overrides.data = transform_overrides;
		opts.transform_overrides.count = ufbxt_arraycount(transform_overrides);
		ufbx_anim *anim = ufbx_create_anim(scene, &opts, NULL);
		ufbxt_assert(anim);

		ufbx_evaluate_opts eval_opts = { 0 };
		eval_opts.evaluate_skinning = true;

		ufbx_scene *state = ufbx_evaluate_scene(scene, anim, 0.0, &eval_opts, NULL);
		ufbxt_assert(state);

		ufbxt_obj_file *obj_file = ufbxt_load_obj_file("maya_poses_bind1_lefthanded", NULL);

		ufbxt_diff_to_obj(state, obj_file, err, 0);

		free(obj_file);
		ufbx_free_scene(state);
		ufbx_free_anim(anim);
	}

	{
		ufbx_pose *pose = ufbx_as_pose(ufbx_find_element(scene, UFBX_ELEMENT_POSE, "skinCluster2"));
		ufbxt_assert(pose);

		ufbx_transform_override transform_overrides[5];
		ufbxt_assert(pose->bone_poses.count == ufbxt_arraycount(transform_overrides));

		for (size_t i = 0; i < pose->bone_poses.count; i++) {
			ufbx_bone_pose *bone = &pose->bone_poses.data[i];
			ufbx_transform transform = ufbx_matrix_to_transform(&bone->bone_to_parent);
			transform_overrides[i].node_id = bone->bone_node->typed_id;
			transform_overrides[i].transform = transform;
		}

		ufbx_anim_opts opts = { 0 };
		opts.transform_overrides.data = transform_overrides;
		opts.transform_overrides.count = ufbxt_arraycount(transform_overrides);
		ufbx_anim *anim = ufbx_create_anim(scene, &opts, NULL);
		ufbxt_assert(anim);

		ufbx_evaluate_opts eval_opts = { 0 };
		eval_opts.evaluate_skinning = true;

		ufbx_scene *state = ufbx_evaluate_scene(scene, anim, 0.0, &eval_opts, NULL);
		ufbxt_assert(state);

		ufbxt_obj_file *obj_file = ufbxt_load_obj_file("maya_poses_bind2_lefthanded", NULL);

		ufbxt_diff_to_obj(state, obj_file, err, 0);

		free(obj_file);
		ufbx_free_scene(state);
		ufbx_free_anim(anim);
	}
}
#endif

UFBXT_FILE_TEST(blender_331_static_blend_shape)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->anim_stacks.count == 0);
}
#endif

UFBXT_FILE_TEST(maya_transformed_skin)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "maya_transformed_skin_0", NULL, 0.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_transformed_skin_4", NULL, 4.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_transformed_skin_8", NULL, 8.0/24.0);
}
#endif

UFBXT_FILE_TEST(maya_instanced_skin)
#if UFBXT_IMPL
{
	// TODO: This is broken on 6100 files currently, as the skin is attached
	// only to pCube1, but somehow it should be attached to pCube2 as well..
	// Probably somehow hacked through `NodeAttributeName: "Geometry::pCube1_ncl1_1"`
	if (scene->metadata.version >= 7000) {
		ufbxt_check_frame(scene, err, true, "maya_instanced_skin_0", NULL, 0.0/24.0);
		ufbxt_check_frame(scene, err, true, "maya_instanced_skin_4", NULL, 4.0/24.0);
		ufbxt_check_frame(scene, err, true, "maya_instanced_skin_8", NULL, 8.0/24.0);
	}
}
#endif

UFBXT_FILE_TEST(blender_279_shape_weights)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Cube");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->blend_deformers.count == 1);
	ufbx_blend_deformer *deformer = mesh->blend_deformers.data[0];
	ufbxt_assert(deformer->channels.count == 1);
	ufbx_blend_channel *channel = deformer->channels.data[0];
	ufbxt_assert(channel->keyframes.count == 1);
	ufbx_blend_keyframe *keyframe = &channel->keyframes.data[0];
	ufbx_blend_shape *shape = keyframe->shape;

	ufbxt_assert(!strcmp(channel->name.data, "Key"));

	if (scene->metadata.version >= 7000) {
		ufbxt_assert(shape->num_offsets == 4);
		ufbxt_assert(shape->offset_weights.count == 4);

		typedef struct {
			ufbx_vec3 position;
			ufbx_real weight;
		} ufbxt_shape_weight_ref;

		const ufbxt_shape_weight_ref weight_ref[] = {
			{ { -1.0f, -1.0f, +1.0f }, 0.0f },
			{ { +1.0f, -1.0f, +1.0f }, 0.25f },
			{ { +1.0f, +1.0f, +1.0f }, 0.75f },
			{ { -1.0f, +1.0f, +1.0f }, 1.0f },
		};

		for (size_t off_i = 0; off_i < shape->num_offsets; off_i++) {
			ufbxt_assert_vec3_equal(shape->position_offsets.data[off_i], 0.0f, 0.0f, 2.0f);

			ufbx_real weight = shape->offset_weights.data[off_i];
			uint32_t index = shape->offset_vertices.data[off_i];
			ufbxt_assert(index < mesh->num_vertices);
			ufbx_vec3 vertex = mesh->vertices.data[index];

			uint32_t num_found = 0;
			for (size_t ref_i = 0; ref_i < ufbxt_arraycount(weight_ref); ref_i++) {
				ufbxt_shape_weight_ref ref = weight_ref[ref_i];
				if (ref.position.x == vertex.x && ref.position.y == vertex.y && ref.position.z == vertex.z) {
					ufbxt_assert(weight == ref.weight);
					num_found++;
				}
			}
			ufbxt_assert(num_found == 1);
		}
	} else {
		ufbxt_assert(shape->offset_weights.count == 0);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(blender_279_shape_weights_dom, blender_279_shape_weights, ufbxt_retain_dom_opts)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Cube");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->blend_deformers.count == 1);
	ufbx_blend_deformer *deformer = mesh->blend_deformers.data[0];
	ufbxt_assert(deformer->channels.count == 1);
	ufbx_blend_channel *channel = deformer->channels.data[0];
	ufbxt_assert(channel->keyframes.count == 1);
	ufbx_blend_keyframe *keyframe = &channel->keyframes.data[0];
	ufbx_blend_shape *shape = keyframe->shape;

	ufbxt_assert(!strcmp(channel->name.data, "Key"));

	if (scene->metadata.version >= 7000) {
		ufbxt_assert(shape->num_offsets == 4);
		ufbxt_assert(shape->offset_weights.count == 4);

		typedef struct {
			ufbx_vec3 position;
			ufbx_real weight;
		} ufbxt_shape_weight_ref;

		const ufbxt_shape_weight_ref weight_ref[] = {
			{ { -1.0f, -1.0f, +1.0f }, 0.0f },
			{ { +1.0f, -1.0f, +1.0f }, 0.25f },
			{ { +1.0f, +1.0f, +1.0f }, 0.75f },
			{ { -1.0f, +1.0f, +1.0f }, 1.0f },
		};

		ufbx_dom_node *dom_channel = channel->element.dom_node;
		ufbxt_assert(dom_channel);

		ufbx_dom_node *dom_full_weights = ufbx_dom_find(dom_channel, "FullWeights");
		ufbxt_assert(dom_full_weights);

		ufbx_real_list full_weights = ufbx_dom_as_real_list(dom_full_weights);
		ufbxt_assert(full_weights.count == 4);

		for (size_t off_i = 0; off_i < shape->num_offsets; off_i++) {
			ufbxt_assert_vec3_equal(shape->position_offsets.data[off_i], 0.0f, 0.0f, 2.0f);

			ufbx_real weight = shape->offset_weights.data[off_i];
			ufbx_real dom_weight = full_weights.data[off_i];
			uint32_t index = shape->offset_vertices.data[off_i];
			ufbxt_assert(index < mesh->num_vertices);
			ufbx_vec3 vertex = mesh->vertices.data[index];

			ufbxt_assert(weight == dom_weight / 100.0);

			uint32_t num_found = 0;
			for (size_t ref_i = 0; ref_i < ufbxt_arraycount(weight_ref); ref_i++) {
				ufbxt_shape_weight_ref ref = weight_ref[ref_i];
				if (ref.position.x == vertex.x && ref.position.y == vertex.y && ref.position.z == vertex.z) {
					ufbxt_assert(weight == ref.weight);
					num_found++;
				}
			}
			ufbxt_assert(num_found == 1);
		}

	} else {
		ufbxt_assert(shape->offset_weights.count == 0);
	}
}
#endif

UFBXT_FILE_TEST(blender440_shape_weight_anim)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "blender440_shape_weight_anim_5", NULL, 5.0/24.0);
	ufbxt_check_frame(scene, err, true, "blender440_shape_weight_anim_15", NULL, 15.0/24.0);
	ufbxt_check_frame(scene, err, true, "blender440_shape_weight_anim_25", NULL, 25.0/24.0);
}
#endif

