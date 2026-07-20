#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "transform"

#if UFBXT_IMPL

static ufbxt_noinline ufbxt_obj_file *ufbxt_load_obj_file(const char *file_name, const ufbxt_load_obj_opts *opts)
{
	char buf[512];
	snprintf(buf, sizeof(buf), "%s%s.obj", data_root, file_name);

	size_t obj_size = 0;
	void *obj_data = ufbxt_read_file(buf, &obj_size);
	ufbxt_obj_file *obj_file = obj_data ? ufbxt_load_obj(obj_data, obj_size, NULL) : NULL;
	ufbxt_assert(obj_file);
	free(obj_data);

	return obj_file;
}

typedef enum {
	UFBXT_CHECK_FRAME_SCALE_100 = 0x1,
	UFBXT_CHECK_FRAME_Z_TO_Y_UP = 0x2,
	UFBXT_CHECK_FRAME_NO_EXTRAPOLATE = 0x4,
} ufbxt_check_frame_flags;

void ufbxt_check_frame_imp(ufbx_scene *scene, ufbxt_diff_error *err, bool check_normals, const char *file_name, const char *anim_name, double time, uint32_t flags)
{
	ufbxt_hintf("Frame from '%s' %s time %.2fs",
		anim_name ? anim_name : "(implicit animation)",
		file_name, time);

	ufbx_evaluate_opts opts = { 0 };
	opts.evaluate_skinning = true;
	opts.evaluate_caches = true;
	opts.load_external_files = true;

	if (flags & UFBXT_CHECK_FRAME_NO_EXTRAPOLATE) {
		opts.evaluate_flags |= UFBX_EVALUATE_FLAG_NO_EXTRAPOLATION;
	}

	ufbxt_obj_file *obj_file = ufbxt_load_obj_file(file_name, NULL);

	if ((flags & UFBXT_CHECK_FRAME_SCALE_100) != 0) {
		obj_file->fbx_position_scale = 100.0;
	}

	if ((flags & UFBXT_CHECK_FRAME_Z_TO_Y_UP) != 0) {
		// -90deg rotation around X
		obj_file->fbx_rotation.w = 0.70710678f;
		obj_file->fbx_rotation.x = -0.70710678f;
	}

	ufbx_anim *anim = scene->anim;

	if (anim_name) {
		for (size_t i = 0; i < scene->anim_stacks.count; i++) {
			ufbx_anim_stack *stack = scene->anim_stacks.data[i];
			if (strstr(stack->name.data, anim_name)) {
				ufbxt_assert(stack->layers.count > 0);
				anim = stack->anim;
				break;
			}
		}
	}

	ufbx_scene *eval = ufbx_evaluate_scene(scene, anim, time, &opts, NULL);
	ufbxt_assert(eval);

	ufbxt_check_scene(eval);

	uint32_t diff_flags = 0;
	if (check_normals) diff_flags |= UFBXT_OBJ_DIFF_FLAG_CHECK_DEFORMED_NORMALS;
	ufbxt_diff_to_obj(eval, obj_file, err, diff_flags);

	ufbx_free_scene(eval);
	free(obj_file);
}
void ufbxt_check_frame(ufbx_scene *scene, ufbxt_diff_error *err, bool check_normals, const char *file_name, const char *anim_name, double time)
{
	ufbxt_check_frame_imp(scene, err, check_normals, file_name, anim_name, time, 0);
}
#endif

#if UFBXT_IMPL
enum {
	UFBXT_CHECK_TRANSFORM_CONSISTENCY_NO_BAKE = 0x1,
	UFBXT_CHECK_TRANSFORM_CONSISTENCY_NO_EXTRAPOLATION = 0x2,
};

static void ufbxt_check_transform_consistency(ufbxt_diff_error *err, ufbx_scene *scene, double sample_rate, uint32_t frame_count, uint32_t flags)
{
	uint32_t eval_flags = 0;
	uint32_t transform_flags = 0;
	if (flags & UFBXT_CHECK_TRANSFORM_CONSISTENCY_NO_EXTRAPOLATION) {
		eval_flags |= UFBX_EVALUATE_FLAG_NO_EXTRAPOLATION;
		transform_flags |= UFBX_TRANSFORM_FLAG_NO_EXTRAPOLATION;
	}

	ufbx_bake_opts bake_opts = { 0 };
	bake_opts.resample_rate = sample_rate;
	bake_opts.evaluate_flags = eval_flags;
	bake_opts.max_keyframe_segments = 4096;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbx_bake_anim(scene, scene->anim, &bake_opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	for (uint32_t frame = 0; frame <= frame_count; frame++) {
		ufbx_evaluate_opts eval_opts = { 0 };
		eval_opts.evaluate_flags = eval_flags;

		double time = (double)frame / sample_rate;
		ufbx_scene *state = ufbx_evaluate_scene(scene, scene->anim, time, &eval_opts, &error);
		if (!state) ufbxt_log_error(&error);
		ufbxt_assert(state);

		for (size_t i = 0; i < bake->nodes.count; i++) {
			ufbx_baked_node *bake_node = &bake->nodes.data[i];
			ufbx_node *scene_node = scene->nodes.data[bake_node->typed_id];
			ufbx_node *state_node = state->nodes.data[bake_node->typed_id];

			ufbx_transform state_transform = state_node->local_transform;

			ufbx_transform eval_transform = ufbx_evaluate_transform_flags(scene->anim, scene_node, time, transform_flags);

			ufbxt_assert_close_vec3(err, state_transform.translation, eval_transform.translation);
			ufbxt_assert_close_quat(err, state_transform.rotation, eval_transform.rotation);
			ufbxt_assert_close_vec3(err, state_transform.scale, eval_transform.scale);

			if ((flags & UFBXT_CHECK_TRANSFORM_CONSISTENCY_NO_BAKE) == 0) {
				ufbx_transform bake_transform = { 0 };
				bake_transform.translation = ufbx_evaluate_baked_vec3(bake_node->translation_keys, time);
				bake_transform.rotation = ufbx_evaluate_baked_quat(bake_node->rotation_keys, time);
				bake_transform.scale = ufbx_evaluate_baked_vec3(bake_node->scale_keys, time);

				ufbxt_assert_close_vec3(err, state_transform.translation, bake_transform.translation);
				ufbxt_assert_close_quat(err, state_transform.rotation, bake_transform.rotation);
				ufbxt_assert_close_vec3(err, state_transform.scale, bake_transform.scale);
			}
		}

		ufbx_free_scene(state);
	}

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST(maya_pivots)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_vec3 origin_ref = { (ufbx_real)0.7211236250, (ufbx_real)1.8317762500, (ufbx_real)-0.6038020000 };
	ufbxt_assert_close_vec3(err, node->local_transform.translation, origin_ref);
}
#endif

#if UFBXT_IMPL
static void ufbxt_check_rotation_order(ufbx_scene *scene, const char *name, ufbx_rotation_order order)
{
	ufbx_node *node = ufbx_find_node(scene, name);
	ufbxt_assert(node);
	ufbx_prop *prop = ufbx_find_prop(&node->props, "RotationOrder");
	ufbxt_assert(prop);
	ufbxt_assert((ufbx_rotation_order)prop->value_int == order);
}
#endif

UFBXT_FILE_TEST(maya_rotation_order)
#if UFBXT_IMPL
{
	ufbxt_check_rotation_order(scene, "XYZ", UFBX_ROTATION_ORDER_XYZ);
	ufbxt_check_rotation_order(scene, "XZY", UFBX_ROTATION_ORDER_XZY);
	ufbxt_check_rotation_order(scene, "YZX", UFBX_ROTATION_ORDER_YZX);
	ufbxt_check_rotation_order(scene, "YXZ", UFBX_ROTATION_ORDER_YXZ);
	ufbxt_check_rotation_order(scene, "ZXY", UFBX_ROTATION_ORDER_ZXY);
	ufbxt_check_rotation_order(scene, "ZYX", UFBX_ROTATION_ORDER_ZYX);
}
#endif

UFBXT_FILE_TEST(maya_post_rotate_order)
#if UFBXT_IMPL
{
	ufbxt_check_rotation_order(scene, "pCube1", UFBX_ROTATION_ORDER_XYZ);
	ufbxt_check_rotation_order(scene, "pCube2", UFBX_ROTATION_ORDER_ZYX);
}
#endif

UFBXT_FILE_TEST(synthetic_pre_post_rotate)
#if UFBXT_IMPL
{
	ufbxt_check_rotation_order(scene, "pCube1", UFBX_ROTATION_ORDER_XYZ);
	ufbxt_check_rotation_order(scene, "pCube2", UFBX_ROTATION_ORDER_ZYX);
}
#endif

UFBXT_FILE_TEST(maya_parented_cubes)
#if UFBXT_IMPL
{
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_geometric_squish, UFBXT_FILE_TEST_FLAG_OPT_HANDLING_IGNORE_NORMALS_IN_DIFF)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pSphere1");
	ufbxt_assert(node);
	ufbxt_assert_close_real(err, node->geometry_transform.scale.y, 0.01f);
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_geometric_transform, UFBXT_FILE_TEST_FLAG_OPT_HANDLING_IGNORE_NORMALS_IN_DIFF)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Parent");
	ufbxt_assert(node);
	ufbx_vec3 euler = ufbx_quat_to_euler(node->geometry_transform.rotation, UFBX_ROTATION_ORDER_XYZ);
	ufbxt_assert_close_real(err, node->geometry_transform.translation.x, -0.5f);
	ufbxt_assert_close_real(err, node->geometry_transform.translation.y, -1.0f);
	ufbxt_assert_close_real(err, node->geometry_transform.translation.z, -1.5f);
	ufbxt_assert_close_real(err, euler.x, 20.0f);
	ufbxt_assert_close_real(err, euler.y, 40.0f);
	ufbxt_assert_close_real(err, euler.z, 60.0f);
	ufbxt_assert_close_real(err, node->geometry_transform.scale.x, 2.0f);
	ufbxt_assert_close_real(err, node->geometry_transform.scale.y, 3.0f);
	ufbxt_assert_close_real(err, node->geometry_transform.scale.z, 4.0f);
}
#endif

UFBXT_FILE_TEST(maya_cube_hidden)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);
	ufbxt_assert(!node->visible);
}
#endif

UFBXT_TEST(root_transform)
#if UFBXT_IMPL
{

	ufbxt_diff_error err = { 0 };

	char path[512];
	ufbxt_file_iterator iter = { "maya_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_load_opts opts = { 0 };

		ufbx_vec3 euler = { { 90.0f, 0.0f, 0.0f } };

		opts.use_root_transform = true;
		opts.root_transform.translation.x = -1.0f;
		opts.root_transform.translation.y = -2.0f;
		opts.root_transform.translation.z = -3.0f;
		opts.root_transform.rotation = ufbx_euler_to_quat(euler, UFBX_ROTATION_ORDER_XYZ);
		opts.root_transform.scale.x = 2.0f;
		opts.root_transform.scale.y = 3.0f;
		opts.root_transform.scale.z = 4.0f;

		ufbx_scene *scene = ufbx_load_file(path, &opts, NULL);
		ufbxt_assert(scene);

		ufbxt_check_scene(scene);

		ufbxt_assert_close_vec3(&err, scene->root_node->local_transform.translation, opts.root_transform.translation);
		ufbxt_assert_close_quat(&err, scene->root_node->local_transform.rotation, opts.root_transform.rotation);
		ufbxt_assert_close_vec3(&err, scene->root_node->local_transform.scale, opts.root_transform.scale);

		ufbx_transform eval = ufbx_evaluate_transform(scene->anim, scene->root_node, 0.1);
		ufbxt_assert_close_vec3(&err, eval.translation, opts.root_transform.translation);
		ufbxt_assert_close_quat(&err, eval.rotation, opts.root_transform.rotation);
		ufbxt_assert_close_vec3(&err, eval.scale, opts.root_transform.scale);

		ufbx_scene *state = ufbx_evaluate_scene(scene, scene->anim, 0.1, NULL, NULL);
		ufbxt_assert(state);

		ufbxt_assert_close_vec3(&err, state->root_node->local_transform.translation, opts.root_transform.translation);
		ufbxt_assert_close_quat(&err, state->root_node->local_transform.rotation, opts.root_transform.rotation);
		ufbxt_assert_close_vec3(&err, state->root_node->local_transform.scale, opts.root_transform.scale);
		
		ufbx_free_scene(state);
		ufbx_free_scene(scene);
	}

	ufbxt_assert(iter.num_found >= 8);

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
}
#endif

UFBXT_TEST(blender_axes)
#if UFBXT_IMPL
{
	char path[512], name[512];

	static const char *axis_names[] = {
		"px", "nx", "py", "ny", "pz", "nz",
	};

	for (uint32_t fwd_ix = 0; fwd_ix < 6; fwd_ix++) {
		for (uint32_t up_ix = 0; up_ix < 6; up_ix++) {

			// Don't allow collinear axes
			if ((fwd_ix >> 1) == (up_ix >> 1)) continue;

			ufbx_coordinate_axis axis_fwd = (ufbx_coordinate_axis)fwd_ix;
			ufbx_coordinate_axis axis_up = (ufbx_coordinate_axis)up_ix;

			snprintf(name, sizeof(name), "blender_axes/axes_%s%s", axis_names[fwd_ix], axis_names[up_ix]);

			ufbxt_file_iterator iter = { name };
			while (ufbxt_next_file(&iter, path, sizeof(path))) {
				ufbxt_diff_error err = { 0 };

				// Load normally and check axes
				{
					ufbx_scene *scene = ufbx_load_file(path, NULL, NULL);
					ufbxt_assert(scene);
					ufbxt_check_scene(scene);

					ufbx_coordinate_axis axis_front = axis_fwd ^ 1;

					ufbxt_assert(scene->settings.axes.front == axis_front);
					ufbxt_assert(scene->settings.axes.up == axis_up);

					ufbx_free_scene(scene);
				}

				// Axis conversion
				for (int mode = 0; mode < 4; mode++) {
					ufbxt_hintf("mode = %d", mode);
					ufbx_load_opts opts = { 0 };

					bool transform_root = (mode % 2) != 0;
					bool use_adjust = (mode / 2) != 0;

					opts.target_axes = ufbx_axes_right_handed_z_up;
					opts.target_unit_meters = 1.0f;

					if (transform_root) {
						opts.use_root_transform = true;
						opts.root_transform = ufbx_identity_transform;
						opts.root_transform.translation.z = 1.0f;
					}

					if (use_adjust) {
						opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
					} else {
						opts.space_conversion = UFBX_SPACE_CONVERSION_TRANSFORM_ROOT;
					}

					ufbx_scene *scene = ufbx_load_file(path, &opts, NULL);
					ufbxt_assert(scene);
					ufbxt_check_scene(scene);

					ufbx_node *plane = ufbx_find_node(scene, "Plane");
					ufbxt_assert(plane && plane->mesh);
					ufbx_mesh *mesh = plane->mesh;

					ufbxt_assert(mesh->num_faces == 1);
					ufbx_face face = mesh->faces.data[0];
					ufbxt_assert(face.num_indices == 3);

					if (use_adjust && !transform_root) {
						ufbx_node *root = scene->root_node;

						ufbx_vec3 identity_scale = { 1.0f, 1.0f, 1.0f };
						ufbxt_assert_close_quat(&err, root->local_transform.rotation, ufbx_identity_quat);
						ufbxt_assert_close_vec3(&err, root->local_transform.scale, identity_scale);
					}

					for (uint32_t i = 0; i < face.num_indices; i++) {
						ufbx_vec3 pos = ufbx_get_vertex_vec3(&mesh->vertex_position, face.index_begin + i);
						ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, face.index_begin + i);

						pos = ufbx_transform_position(&plane->geometry_to_world, pos);

						ufbx_vec3 ref;
						if (uv.x < 0.5f && uv.y < 0.5f) {
							ref.x = 1.0f;
							ref.y = 1.0f;
							ref.z = 3.0f;
						} else if (uv.x > 0.5f && uv.y < 0.5f) {
							ref.x = 2.0f;
							ref.y = 2.0f;
							ref.z = 3.0f;
						} else if (uv.x < 0.5f && uv.y > 0.5f) {
							ref.x = 1.0f;
							ref.y = 2.0f;
							ref.z = 4.0f;
						} else {
							ufbxt_assert(0 && "Shouldn't exist");
						}

						if (transform_root) {
							ref.z += 1.0f;
						}

						ufbxt_assert_close_vec3(&err, pos, ref);
					}

					ufbx_free_scene(scene);
				}

				ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
			}
		}
	}
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_scale_to_cm_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_unit_meters = 0.01f;
	return opts;
}
static ufbx_load_opts ufbxt_scale_to_cm_adjust_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_unit_meters = 0.01f;
	opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
	return opts;
}
static ufbx_load_opts ufbxt_scale_to_cm_helper_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_unit_meters = 0.01f;
	opts.inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_HELPER_NODES;
	return opts;
}
static ufbx_load_opts ufbxt_scale_to_cm_adjust_helper_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_unit_meters = 0.01f;
	opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
	opts.inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_HELPER_NODES;
	return opts;
}
static ufbx_load_opts ufbxt_scale_to_cm_compensate_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_unit_meters = 0.01f;
	opts.inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_COMPENSATE;
	return opts;
}
static ufbx_load_opts ufbxt_scale_to_cm_adjust_compensate_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_unit_meters = 0.01f;
	opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
	opts.inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_COMPENSATE;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS(maya_scale_no_inherit, ufbxt_scale_to_cm_opts)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "joint1");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);
		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 0.03f, 0.03f, 0.03f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 3.0f, 3.0f, 3.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 0.0f, 0.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint2");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_IGNORE_PARENT_SCALE);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);
		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 100.0f, 100.0f, 100.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 0.0f, 6.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_IGNORE_PARENT_SCALE);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);
		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 0.01f, 0.01f, 0.01f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 200.0f, 206.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint4");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_IGNORE_PARENT_SCALE);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);
		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 0.015f, 0.025f, 0.035f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 1.5f, 2.5f, 3.5f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 200.0f, 208.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");

		{
			ufbx_transform transform = ufbx_evaluate_transform(scene->anim, node, 1.0);
			ufbxt_assert_close_real(err, transform.scale.x * 100.0f, 0.3f);
			ufbxt_assert_close_real(err, transform.scale.y * 100.0f, 0.6f);
			ufbxt_assert_close_real(err, transform.scale.z * 100.0f, 0.9f);
		}

		{
			ufbx_transform transform = ufbx_evaluate_transform(scene->anim, node, 0.5);
			ufbxt_assert_close_real(err, transform.scale.x * 100.0f, 0.67281f);
			ufbxt_assert_close_real(err, transform.scale.y * 100.0f, 0.81304f);
			ufbxt_assert_close_real(err, transform.scale.z * 100.0f, 0.95326f);
		}
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_scale_no_inherit_adjust, maya_scale_no_inherit, ufbxt_scale_to_cm_adjust_opts)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "joint1");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_IGNORE_PARENT_SCALE);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);
		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 3.0f, 3.0f, 3.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 3.0f, 3.0f, 3.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 0.0f, 0.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint2");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_IGNORE_PARENT_SCALE);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);
		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 100.0f, 100.0f, 100.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 100.0f, 100.0f, 100.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 0.0f, 6.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_IGNORE_PARENT_SCALE);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);
		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 200.0f, 206.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint4");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_IGNORE_PARENT_SCALE);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);
		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 1.5f, 2.5f, 3.5f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 1.5f, 2.5f, 3.5f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 200.0f, 208.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");

		{
			ufbx_transform transform = ufbx_evaluate_transform(scene->anim, node, 1.0);
			ufbxt_assert_close_vec3_xyz(err, transform.scale, 0.3f, 0.6f, 0.9f);
		}

		{
			ufbx_transform transform = ufbx_evaluate_transform(scene->anim, node, 0.5);
			ufbxt_assert_close_vec3_xyz(err, transform.scale, 0.67281f, 0.81304f, 0.95326f);
		}
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_scale_no_inherit_helper, maya_scale_no_inherit, ufbxt_scale_to_cm_helper_opts)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "joint1");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbx_node *helper = node->scale_helper;
		ufbxt_assert(helper);
		ufbxt_assert(helper->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&helper->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, helper->local_transform.scale, 0.03f, 0.03f, 0.03f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 3.0f, 3.0f, 3.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 0.0f, 0.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint2");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbxt_assert(!node->scale_helper);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 100.0f, 100.0f, 100.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 0.0f, 6.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbx_node *helper = node->scale_helper;
		ufbxt_assert(helper);
		ufbxt_assert(helper->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&helper->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, helper->local_transform.scale, 0.01f, 0.01f, 0.01f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 200.0f, 206.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint4");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbxt_assert(!node->scale_helper);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 0.015f, 0.025f, 0.035f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 1.5f, 2.5f, 3.5f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 200.0f, 208.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");
		ufbx_node *helper = node->scale_helper;
		ufbxt_assert(helper);

		{
			ufbx_transform transform = ufbx_evaluate_transform(scene->anim, helper, 1.0);
			ufbxt_assert_close_real(err, transform.scale.x * 100.0f, 0.3f);
			ufbxt_assert_close_real(err, transform.scale.y * 100.0f, 0.6f);
			ufbxt_assert_close_real(err, transform.scale.z * 100.0f, 0.9f);
		}

		{
			ufbx_transform transform = ufbx_evaluate_transform(scene->anim, helper, 0.5);
			ufbxt_assert_close_real(err, transform.scale.x * 100.0f, 0.67281f);
			ufbxt_assert_close_real(err, transform.scale.y * 100.0f, 0.81304f);
			ufbxt_assert_close_real(err, transform.scale.z * 100.0f, 0.95326f);
		}
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_scale_no_inherit_adjust_helper, maya_scale_no_inherit, ufbxt_scale_to_cm_adjust_helper_opts)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "joint1");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbx_node *helper = node->scale_helper;
		ufbxt_assert(helper);
		ufbxt_assert(helper->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&helper->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 100.0f, 100.0f, 100.0f);
		ufbxt_assert_close_vec3_xyz(err, helper->local_transform.scale, 0.03f, 0.03f, 0.03f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 3.0f, 3.0f, 3.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 0.0f, 0.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint2");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbxt_assert(!node->scale_helper);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 100.0f, 100.0f, 100.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 0.0f, 6.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbx_node *helper = node->scale_helper;
		ufbxt_assert(helper);
		ufbxt_assert(helper->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&helper->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, helper->local_transform.scale, 0.01f, 0.01f, 0.01f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 200.0f, 206.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint4");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbxt_assert(!node->scale_helper);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 0.015f, 0.025f, 0.035f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 1.5f, 2.5f, 3.5f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 200.0f, 208.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");
		ufbx_node *helper = node->scale_helper;
		ufbxt_assert(helper);

		{
			ufbx_transform transform = ufbx_evaluate_transform(scene->anim, helper, 1.0);
			ufbxt_assert_close_real(err, transform.scale.x * 100.0f, 0.3f);
			ufbxt_assert_close_real(err, transform.scale.y * 100.0f, 0.6f);
			ufbxt_assert_close_real(err, transform.scale.z * 100.0f, 0.9f);
		}

		{
			ufbx_transform transform = ufbx_evaluate_transform(scene->anim, helper, 0.5);
			ufbxt_assert_close_real(err, transform.scale.x * 100.0f, 0.67281f);
			ufbxt_assert_close_real(err, transform.scale.y * 100.0f, 0.81304f);
			ufbxt_assert_close_real(err, transform.scale.z * 100.0f, 0.95326f);
		}
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_scale_no_inherit_compensate, maya_scale_no_inherit, ufbxt_scale_to_cm_compensate_opts)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "joint1");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbxt_assert(!node->scale_helper);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 0.03f, 0.03f, 0.03f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 3.0f, 3.0f, 3.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 0.0f, 0.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint2");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbxt_assert(!node->scale_helper);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 33.33333f, 33.33333f, 33.33333f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 100.0f, 100.0f, 100.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 0.0f, 6.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbx_node *helper = node->scale_helper;
		ufbxt_assert(helper);
		ufbxt_assert(helper->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&helper->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, helper->local_transform.scale, 0.01f, 0.01f, 0.01f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 200.0f, 206.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint4");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbxt_assert(!node->scale_helper);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 0.015f, 0.025f, 0.035f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 1.5f, 2.5f, 3.5f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 200.0f, 208.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");
		ufbx_node *helper = node->scale_helper;
		ufbxt_assert(helper);

		{
			ufbx_transform transform = ufbx_evaluate_transform(scene->anim, helper, 1.0);
			ufbxt_assert_close_real(err, transform.scale.x * 100.0f, 0.3f);
			ufbxt_assert_close_real(err, transform.scale.y * 100.0f, 0.6f);
			ufbxt_assert_close_real(err, transform.scale.z * 100.0f, 0.9f);
		}

		{
			ufbx_transform transform = ufbx_evaluate_transform(scene->anim, helper, 0.5);
			ufbxt_assert_close_real(err, transform.scale.x * 100.0f, 0.67281f);
			ufbxt_assert_close_real(err, transform.scale.y * 100.0f, 0.81304f);
			ufbxt_assert_close_real(err, transform.scale.z * 100.0f, 0.95326f);
		}
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_scale_no_inherit_adjust_compensate, maya_scale_no_inherit, ufbxt_scale_to_cm_adjust_compensate_opts)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "joint1");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbxt_assert(!node->scale_helper);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 3.0f, 3.0f, 3.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 3.0f, 3.0f, 3.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 0.0f, 0.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint2");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbxt_assert(!node->scale_helper);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 33.33333f, 33.33333f, 33.33333f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 100.0f, 100.0f, 100.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 0.0f, 6.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbx_node *helper = node->scale_helper;
		ufbxt_assert(helper);
		ufbxt_assert(helper->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&helper->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, helper->local_transform.scale, 0.01f, 0.01f, 0.01f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 1.0f, 1.0f, 1.0f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 200.0f, 206.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint4");
		ufbxt_assert(node);
		ufbxt_assert(node->inherit_mode == UFBX_INHERIT_MODE_NORMAL);
		ufbxt_assert(!node->scale_helper);
		ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3_xyz(err, node->local_transform.scale, 0.015f, 0.025f, 0.035f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.scale, 1.5f, 2.5f, 3.5f);
		ufbxt_assert_close_vec3_xyz(err, world_transform.translation, 200.0f, 208.0f, 0.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");
		ufbx_node *helper = node->scale_helper;
		ufbxt_assert(helper);

		{
			ufbx_transform transform = ufbx_evaluate_transform(scene->anim, helper, 1.0);
			ufbxt_assert_close_real(err, transform.scale.x * 100.0f, 0.3f);
			ufbxt_assert_close_real(err, transform.scale.y * 100.0f, 0.6f);
			ufbxt_assert_close_real(err, transform.scale.z * 100.0f, 0.9f);
		}

		{
			ufbx_transform transform = ufbx_evaluate_transform(scene->anim, helper, 0.5);
			ufbxt_assert_close_real(err, transform.scale.x * 100.0f, 0.67281f);
			ufbxt_assert_close_real(err, transform.scale.y * 100.0f, 0.81304f);
			ufbxt_assert_close_real(err, transform.scale.z * 100.0f, 0.95326f);
		}
	}
}
#endif

UFBXT_FILE_TEST(synthetic_node_dag)
#if UFBXT_IMPL
{
	ufbx_node *root = scene->root_node;
	ufbx_node *a = ufbx_find_node(scene, "A");
	ufbx_node *b = ufbx_find_node(scene, "B");
	ufbx_node *c = ufbx_find_node(scene, "C");
	ufbx_node *d = ufbx_find_node(scene, "D");

	ufbxt_assert(root && a && b && c && d);
	ufbxt_assert(root->children.count == 1);
	ufbxt_assert(root->children.data[0] == a);

	ufbxt_assert(a->parent == root);
	ufbxt_assert(a->children.count == 1);
	ufbxt_assert(a->children.data[0] == b);

	ufbxt_assert(b->parent == a);
	ufbxt_assert(b->children.count == 1);
	ufbxt_assert(b->children.data[0] == c);

	ufbxt_assert(c->parent == b);
	ufbxt_assert(c->children.count == 1);
	ufbxt_assert(c->children.data[0] == d);

	ufbxt_assert(d->parent == c);
	ufbxt_assert(d->children.count == 0);
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_node_cycle_fail, UFBXT_FILE_TEST_FLAG_ALLOW_ERROR)
#if UFBXT_IMPL
{
	ufbxt_assert(!scene);
}
#endif

UFBXT_FILE_TEST(maya_static_no_inherit_scale)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "joint1");
		ufbxt_assert(node);

		ufbx_vec3 ref_translation = { 0.0f, 0.0f, 0.0f };
		ufbx_vec3 ref_scale = { 2.0f, 2.0f, 2.0f };
		ufbx_transform transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3(err, ref_translation, transform.translation);
		ufbxt_assert_close_vec3(err, ref_scale, transform.scale);
		ufbxt_assert_close_vec3(err, ref_scale, node->local_transform.scale);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint2");
		ufbxt_assert(node);

		ufbx_vec3 ref_translation = { 0.0f, 8.0f, 0.0f };
		ufbx_vec3 ref_scale = { 1.0f, 2.0f, 2.0f };
		ufbx_transform transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3(err, ref_translation, transform.translation);
		ufbxt_assert_close_vec3(err, ref_scale, transform.scale);
		ufbxt_assert_close_vec3(err, ref_scale, node->local_transform.scale);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");
		ufbxt_assert(node);

		ufbx_vec3 ref_translation = { 0.0f, 12.0f, 0.0f };
		ufbx_vec3 ref_scale = { 1.0f, 1.0f, 1.0f };
		ufbx_transform transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3(err, ref_translation, transform.translation);
		ufbxt_assert_close_vec3(err, ref_scale, transform.scale);
		ufbxt_assert_close_vec3(err, ref_scale, node->local_transform.scale);
	}
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_scale_helper_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_HELPER_NODES;
	return opts;
}
static ufbx_load_opts ufbxt_scale_compensate_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_COMPENSATE;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(maya_static_no_inherit_scale_helper, maya_static_no_inherit_scale, ufbxt_scale_helper_opts, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
	ufbx_vec3 ref_scale_one = { 1.0f, 1.0f, 1.0f };

	{
		ufbx_node *node = ufbx_find_node(scene, "joint1");
		ufbxt_assert(node);
		ufbxt_assert(node->scale_helper);

		ufbx_vec3 ref_translation = { 0.0f, 0.0f, 0.0f };
		ufbx_vec3 ref_scale = { 2.0f, 2.0f, 2.0f };
		ufbx_transform transform = ufbx_matrix_to_transform(&node->scale_helper->node_to_world);

		ufbxt_assert_close_vec3(err, ref_translation, transform.translation);
		ufbxt_assert_close_vec3(err, ref_scale, transform.scale);
		ufbxt_assert_close_vec3(err, ref_scale, node->scale_helper->local_transform.scale);
		ufbxt_assert_close_vec3(err, ref_scale_one, node->local_transform.scale);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint2");
		ufbxt_assert(node);
		ufbxt_assert(node->scale_helper);

		ufbx_vec3 ref_translation = { 0.0f, 8.0f, 0.0f };
		ufbx_vec3 ref_scale = { 1.0f, 2.0f, 2.0f };
		ufbx_transform transform = ufbx_matrix_to_transform(&node->scale_helper->node_to_world);

		ufbxt_assert_close_vec3(err, ref_translation, transform.translation);
		ufbxt_assert_close_vec3(err, ref_scale, transform.scale);
		ufbxt_assert_close_vec3(err, ref_scale, node->scale_helper->local_transform.scale);
		ufbxt_assert_close_vec3(err, ref_scale_one, node->local_transform.scale);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");
		ufbxt_assert(node);
		ufbxt_assert(!node->scale_helper);

		ufbx_vec3 ref_translation = { 0.0f, 12.0f, 0.0f };
		ufbx_vec3 ref_scale = { 1.0f, 1.0f, 1.0f };
		ufbx_transform transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3(err, ref_translation, transform.translation);
		ufbxt_assert_close_vec3(err, ref_scale, transform.scale);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(maya_static_no_inherit_scale_compensate, maya_static_no_inherit_scale, ufbxt_scale_compensate_opts, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
	ufbx_vec3 ref_scale_one = { 1.0f, 1.0f, 1.0f };

	{
		ufbx_node *node = ufbx_find_node(scene, "joint1");
		ufbxt_assert(node);

		ufbx_vec3 ref_translation = { 0.0f, 0.0f, 0.0f };
		ufbx_vec3 ref_scale = { 2.0f, 2.0f, 2.0f };
		ufbx_transform transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3(err, ref_translation, transform.translation);
		ufbxt_assert_close_vec3(err, ref_scale, transform.scale);
		ufbxt_assert_close_vec3(err, ref_scale, node->local_transform.scale);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint2");
		ufbxt_assert(node);
		ufbxt_assert(node->scale_helper);
		ufbxt_assert_close_real(err, node->adjust_post_scale, 0.5f);

		ufbx_vec3 ref_translation = { 0.0f, 8.0f, 0.0f };
		ufbx_vec3 ref_scale = { 1.0f, 2.0f, 2.0f };
		ufbx_vec3 local_scale = { 0.5f, 0.5f, 0.5f };
		ufbx_transform transform = ufbx_matrix_to_transform(&node->scale_helper->node_to_world);

		ufbxt_assert_close_vec3(err, ref_translation, transform.translation);
		ufbxt_assert_close_vec3(err, ref_scale, transform.scale);
		ufbxt_assert_close_vec3(err, ref_scale, node->scale_helper->local_transform.scale);
		ufbxt_assert_close_vec3(err, local_scale, node->local_transform.scale);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "joint3");
		ufbxt_assert(node);
		ufbxt_assert(!node->scale_helper);

		ufbx_vec3 ref_translation = { 0.0f, 12.0f, 0.0f };
		ufbx_vec3 ref_scale = { 1.0f, 1.0f, 1.0f };
		ufbx_transform transform = ufbx_matrix_to_transform(&node->node_to_world);

		ufbxt_assert_close_vec3(err, ref_translation, transform.translation);
		ufbxt_assert_close_vec3(err, ref_scale, transform.scale);
	}
}
#endif

UFBXT_FILE_TEST(synthetic_geometry_transform_inherit_mode)
#if UFBXT_IMPL
{
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(synthetic_geometry_transform_inherit_mode_scale_helper, synthetic_geometry_transform_inherit_mode, ufbxt_scale_helper_opts, UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
	for (size_t i = 0; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];
		ufbx_transform transform = ufbx_evaluate_transform(scene->anim, node, 0.0);
		ufbxt_assert_close_vec3(err, node->local_transform.translation, transform.translation);
		ufbxt_assert_close_quat(err, node->local_transform.rotation, transform.rotation);
		ufbxt_assert_close_vec3(err, node->local_transform.scale, transform.scale);
	}
}
#endif

UFBXT_FILE_TEST(maya_child_pivots)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "maya_child_pivots", NULL, 0.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_child_pivots_6", NULL, 6.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_child_pivots_12", NULL, 12.0/24.0);
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_lefthanded_y_up_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_axes = ufbx_axes_left_handed_y_up;
	opts.handedness_conversion_axis = UFBX_MIRROR_AXIS_X;
	return opts;
}
#endif

UFBXT_FILE_TEST(maya_axes)
#if UFBXT_IMPL
{
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_modify_meter_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_unit_meters = 1.0f;
	opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(maya_axes_modify, maya_axes, ufbxt_modify_meter_opts, UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_DIFF_SCALE_100)
#if UFBXT_IMPL
{
}
#endif

UFBXT_TEST(maya_axes_lefthanded)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	ufbxt_obj_file *obj_file = ufbxt_load_obj_file("maya_axes_lefthanded", NULL);

	char path[512];
	ufbxt_file_iterator iter = { "maya_axes" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t i = 0; i < UFBX_MIRROR_AXIS_COUNT; i++) {
			ufbxt_hintf("mirror_axis=%u", i);

			ufbx_load_opts opts = { 0 };

			opts.target_axes = ufbx_axes_left_handed_y_up;
			opts.handedness_conversion_axis = (ufbx_mirror_axis)i;
			opts.reverse_winding = (i == 0);

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			if (!scene) ufbxt_log_error(&error);
			ufbxt_assert(scene);

			ufbxt_check_transform_consistency(&err, scene, 24.0, 1, 0);

			ufbxt_check_scene(scene);
			ufbxt_diff_to_obj(scene, obj_file, &err, 0);
			ufbx_free_scene(scene);
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
	free(obj_file);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(maya_axes_lefthanded_file, maya_axes, ufbxt_lefthanded_y_up_opts, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
}
#endif

UFBXT_TEST(maya_axes_lefthanded_adjust)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	ufbxt_obj_file *obj_file = ufbxt_load_obj_file("maya_axes_lefthanded", NULL);

	char path[512];
	ufbxt_file_iterator iter = { "maya_axes" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t i = 1; i < UFBX_MIRROR_AXIS_COUNT; i++) {
			ufbxt_hintf("mirror_axis=%u", i);

			ufbx_load_opts opts = { 0 };

			opts.target_axes = ufbx_axes_left_handed_y_up;
			opts.handedness_conversion_axis = (ufbx_mirror_axis)i;
			opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			if (!scene) ufbxt_log_error(&error);
			ufbxt_assert(scene);

			ufbxt_check_transform_consistency(&err, scene, 24.0, 1, 0);

			ufbxt_check_scene(scene);
			ufbxt_diff_to_obj(scene, obj_file, &err, 0);
			ufbx_free_scene(scene);
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
	free(obj_file);
}
#endif

UFBXT_TEST(maya_axes_lefthanded_modify)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	ufbxt_obj_file *obj_file = ufbxt_load_obj_file("maya_axes_lefthanded", NULL);
	obj_file->fbx_position_scale = 100.0;

	char path[512];
	ufbxt_file_iterator iter = { "maya_axes" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t i = 1; i < UFBX_MIRROR_AXIS_COUNT; i++) {
			ufbxt_hintf("mirror_axis=%u", i);

			ufbx_load_opts opts = { 0 };

			opts.target_axes = ufbx_axes_left_handed_y_up;
			opts.handedness_conversion_axis = (ufbx_mirror_axis)i;
			opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
			opts.target_unit_meters = 1.0f;

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			if (!scene) ufbxt_log_error(&error);
			ufbxt_assert(scene);

			ufbxt_check_transform_consistency(&err, scene, 24.0, 1, 0);

			ufbxt_check_scene(scene);
			ufbxt_diff_to_obj(scene, obj_file, &err, 0);
			ufbx_free_scene(scene);
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
	free(obj_file);
}
#endif

UFBXT_FILE_TEST(maya_axes_anim)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, false, "maya_axes_anim_0", NULL, 0.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_axes_anim_5", NULL, 5.0/24.0);
	ufbxt_check_frame(scene, err, false, "maya_axes_anim_8", NULL, 8.0/24.0);

	ufbxt_assert(scene->poses.count == 1);
	ufbx_pose *pose = scene->poses.data[0];
	ufbxt_assert(pose->is_bind_pose);
	ufbxt_assert(pose->bone_poses.count == 5);
	for (size_t i = 0; i < pose->bone_poses.count; i++) {
		ufbx_bone_pose *bone_pose = &pose->bone_poses.data[i];
		ufbx_node *bone_node = bone_pose->bone_node;
		ufbxt_assert(bone_node);

		ufbxt_assert_close_vec3(err, bone_node->node_to_world.cols[0], bone_pose->bone_to_world.cols[0]);
		ufbxt_assert_close_vec3(err, bone_node->node_to_world.cols[1], bone_pose->bone_to_world.cols[1]);
		ufbxt_assert_close_vec3(err, bone_node->node_to_world.cols[2], bone_pose->bone_to_world.cols[2]);
		ufbxt_assert_close_vec3(err, bone_node->node_to_world.cols[3], bone_pose->bone_to_world.cols[3]);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(maya_axes_anim_modify, maya_axes_anim, ufbxt_modify_meter_opts, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
	ufbxt_check_frame_imp(scene, err, false, "maya_axes_anim_0", NULL, 0.0/24.0, UFBXT_CHECK_FRAME_SCALE_100);
	ufbxt_check_frame_imp(scene, err, false, "maya_axes_anim_5", NULL, 5.0/24.0, UFBXT_CHECK_FRAME_SCALE_100);
	ufbxt_check_frame_imp(scene, err, false, "maya_axes_anim_8", NULL, 8.0/24.0, UFBXT_CHECK_FRAME_SCALE_100);
}
#endif

UFBXT_TEST(maya_axes_anim_lefthanded)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	char path[512];
	ufbxt_file_iterator iter = { "maya_axes_anim" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t i = 0; i < UFBX_MIRROR_AXIS_COUNT; i++) {
			ufbxt_hintf("mirror_axis=%u", i);

			ufbx_load_opts opts = { 0 };

			opts.target_axes = ufbx_axes_left_handed_y_up;
			opts.handedness_conversion_axis = (ufbx_mirror_axis)i;
			opts.load_external_files = true;
			opts.reverse_winding = (i == 0);

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			if (!scene) ufbxt_log_error(&error);
			ufbxt_assert(scene);

			ufbxt_check_scene(scene);
			ufbxt_check_frame(scene, &err, false, "maya_axes_anim_lefthanded_0", NULL, 0.0/24.0);
			ufbxt_check_frame(scene, &err, false, "maya_axes_anim_lefthanded_5", NULL, 5.0/24.0);
			ufbxt_check_frame(scene, &err, false, "maya_axes_anim_lefthanded_8", NULL, 8.0/24.0);

			ufbxt_check_transform_consistency(&err, scene, 24.0, 24, 0);

			ufbx_free_scene(scene);
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(maya_axes_anim_lefthanded_file, maya_axes_anim, ufbxt_lefthanded_y_up_opts, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
}
#endif

UFBXT_TEST(maya_axes_anim_lefthanded_adjust)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	char path[512];
	ufbxt_file_iterator iter = { "maya_axes_anim" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t i = 1; i < UFBX_MIRROR_AXIS_COUNT; i++) {
			ufbxt_hintf("mirror_axis=%u", i);

			ufbx_load_opts opts = { 0 };

			opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
			opts.target_axes = ufbx_axes_left_handed_y_up;
			opts.handedness_conversion_axis = (ufbx_mirror_axis)i;
			opts.load_external_files = true;

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			if (!scene) ufbxt_log_error(&error);
			ufbxt_assert(scene);

			ufbxt_check_scene(scene);
			ufbxt_check_frame(scene, &err, false, "maya_axes_anim_lefthanded_0", NULL, 0.0/24.0);
			ufbxt_check_frame(scene, &err, false, "maya_axes_anim_lefthanded_5", NULL, 5.0/24.0);
			ufbxt_check_frame(scene, &err, false, "maya_axes_anim_lefthanded_8", NULL, 8.0/24.0);

			ufbxt_check_transform_consistency(&err, scene, 24.0, 24, 0);

			ufbx_free_scene(scene);
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
}
#endif

UFBXT_TEST(maya_axes_anim_lefthanded_modify)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	char path[512];
	ufbxt_file_iterator iter = { "maya_axes_anim" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t i = 1; i < UFBX_MIRROR_AXIS_COUNT; i++) {
			ufbxt_hintf("mirror_axis=%u", i);

			ufbx_load_opts opts = { 0 };

			opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
			opts.target_unit_meters = 1.0f;
			opts.target_axes = ufbx_axes_left_handed_y_up;
			opts.handedness_conversion_axis = (ufbx_mirror_axis)i;
			opts.load_external_files = true;

			ufbx_error error;
			ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
			if (!scene) ufbxt_log_error(&error);
			ufbxt_assert(scene);

			ufbxt_check_scene(scene);
			ufbxt_check_frame_imp(scene, &err, false, "maya_axes_anim_lefthanded_0", NULL, 0.0/24.0, UFBXT_CHECK_FRAME_SCALE_100);
			ufbxt_check_frame_imp(scene, &err, false, "maya_axes_anim_lefthanded_5", NULL, 5.0/24.0, UFBXT_CHECK_FRAME_SCALE_100);
			ufbxt_check_frame_imp(scene, &err, false, "maya_axes_anim_lefthanded_8", NULL, 8.0/24.0, UFBXT_CHECK_FRAME_SCALE_100);

			ufbxt_check_transform_consistency(&err, scene, 24.0, 24, 0);

			ufbx_free_scene(scene);
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_pivot_adjust_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.pivot_handling = UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT;
	return opts;
}
#endif

UFBXT_FILE_TEST(maya_equal_pivot)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);
	ufbx_prop *prop = NULL;

	prop = ufbx_find_prop(&node->props, "RotationPivot");
	ufbxt_assert(prop);
	ufbxt_assert_close_vec3_xyz(err, prop->value_vec3, 2.0f, 0.0f, 0.0f);

	prop = ufbx_find_prop(&node->props, "ScalingPivot");
	ufbxt_assert(prop);
	ufbxt_assert_close_vec3_xyz(err, prop->value_vec3, 2.0f, 0.0f, 0.0f);

	prop = ufbx_find_prop(&node->props, "GeometricTranslation");
	ufbxt_assert_close_vec3_xyz(err, prop->value_vec3, 0.0f, 0.0f, 0.0f);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(maya_equal_pivot_adjust, maya_equal_pivot, ufbxt_pivot_adjust_opts, UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);
	ufbx_prop *prop = NULL;

	prop = ufbx_find_prop(&node->props, "RotationPivot");
	ufbxt_assert(prop);
	ufbxt_assert_close_vec3_xyz(err, prop->value_vec3, 0.0f, 0.0f, 0.0f);

	prop = ufbx_find_prop(&node->props, "ScalingPivot");
	ufbxt_assert(prop);
	ufbxt_assert_close_vec3_xyz(err, prop->value_vec3, 0.0f, 0.0f, 0.0f);

	prop = ufbx_find_prop(&node->props, "GeometricTranslation");
	ufbxt_assert(prop);
	ufbxt_assert_close_vec3_xyz(err, prop->value_vec3, -2.0f, 0.0f, 0.0f);
}
#endif

UFBXT_FILE_TEST(maya_equal_pivot_scale)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Middle");
	ufbxt_assert(node);
	ufbx_prop *prop = NULL;

	prop = ufbx_find_prop(&node->props, "RotationPivot");
	ufbxt_assert(prop);
	ufbxt_assert_close_vec3_xyz(err, prop->value_vec3, 2.0f, 0.0f, 0.0f);

	prop = ufbx_find_prop(&node->props, "ScalingPivot");
	ufbxt_assert(prop);
	ufbxt_assert_close_vec3_xyz(err, prop->value_vec3, 2.0f, 0.0f, 0.0f);

	prop = ufbx_find_prop(&node->props, "GeometricTranslation");
	ufbxt_assert_close_vec3_xyz(err, prop->value_vec3, 0.0f, 0.0f, 0.0f);

	ufbxt_check_frame_imp(scene, err, false, "maya_equal_pivot_scale_6", NULL, 6.0/24.0, 0);
	ufbxt_check_frame_imp(scene, err, false, "maya_equal_pivot_scale_12", NULL, 12.0/24.0, 0);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(maya_equal_pivot_scale_adjust, maya_equal_pivot_scale, ufbxt_pivot_adjust_opts, UFBXT_FILE_TEST_FLAG_DIFF_ALWAYS)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Middle");
	ufbxt_assert(node);
	ufbx_prop *prop = NULL;

	prop = ufbx_find_prop(&node->props, "RotationPivot");
	ufbxt_assert(prop);
	ufbxt_assert_close_vec3_xyz(err, prop->value_vec3, 0.0f, 0.0f, 0.0f);

	prop = ufbx_find_prop(&node->props, "ScalingPivot");
	ufbxt_assert(prop);
	ufbxt_assert_close_vec3_xyz(err, prop->value_vec3, 0.0f, 0.0f, 0.0f);

	prop = ufbx_find_prop(&node->props, "GeometricTranslation");
	ufbxt_assert(prop);
	ufbxt_assert_close_vec3_xyz(err, prop->value_vec3, -2.0f, 0.0f, 0.0f);

	ufbxt_check_frame_imp(scene, err, false, "maya_equal_pivot_scale_6", NULL, 6.0/24.0, 0);
	ufbxt_check_frame_imp(scene, err, false, "maya_equal_pivot_scale_12", NULL, 12.0/24.0, 0);
}
#endif

UFBXT_TEST(maya_equal_pivot_handling)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	ufbxt_obj_file *obj_file = ufbxt_load_obj_file("maya_equal_pivot", NULL);

	char path[512];
	ufbxt_file_iterator iter = { "maya_equal_pivot" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t pv = 0; pv < UFBX_PIVOT_HANDLING_COUNT; pv++) {
			for (uint32_t gh = 0; gh < UFBX_GEOMETRY_TRANSFORM_HANDLING_COUNT; gh++) {
				ufbxt_hintf("pivot_handling=%u, geometry_transform_handling=%u", pv, gh);

				ufbx_load_opts opts = { 0 };

				opts.pivot_handling = (ufbx_pivot_handling)pv;
				opts.geometry_transform_handling = (ufbx_geometry_transform_handling)gh;

				ufbx_error error;
				ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
				if (!scene) ufbxt_log_error(&error);
				ufbxt_assert(scene);
				ufbxt_check_scene(scene);

				ufbx_node *node = ufbx_find_node(scene, "pCube1");
				ufbxt_assert(node);
				if (pv == UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT) {
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&node->props, "RotationPivot", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&node->props, "ScalingPivot", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);
				}

				ufbxt_diff_to_obj(scene, obj_file, &err, 0);
				ufbx_free_scene(scene);
			}
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
	free(obj_file);
}
#endif

UFBXT_TEST(maya_equal_pivot_scale_handling)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	ufbxt_obj_file *obj_file = ufbxt_load_obj_file("maya_equal_pivot_scale", NULL);

	char path[512];
	ufbxt_file_iterator iter = { "maya_equal_pivot_scale" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t pv = 0; pv < UFBX_PIVOT_HANDLING_COUNT; pv++) {
			for (uint32_t gh = 0; gh < UFBX_GEOMETRY_TRANSFORM_HANDLING_COUNT; gh++) {
				ufbxt_hintf("pivot_handling=%u, geometry_transform_handling=%u", pv, gh);

				ufbx_load_opts opts = { 0 };

				opts.pivot_handling = (ufbx_pivot_handling)pv;
				opts.geometry_transform_handling = (ufbx_geometry_transform_handling)gh;

				ufbx_error error;
				ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
				if (!scene) ufbxt_log_error(&error);
				ufbxt_assert(scene);
				ufbxt_check_scene(scene);

				ufbx_node *node = ufbx_find_node(scene, "Middle");
				ufbxt_assert(node);
				if (pv == UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT) {
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&node->props, "RotationPivot", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&node->props, "ScalingPivot", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);
				}

				ufbxt_diff_to_obj(scene, obj_file, &err, 0);
				ufbxt_check_frame_imp(scene, &err, false, "maya_equal_pivot_scale_6", NULL, 6.0/24.0, 0);
				ufbxt_check_frame_imp(scene, &err, false, "maya_equal_pivot_scale_12", NULL, 12.0/24.0, 0);

				ufbx_free_scene(scene);
			}
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
	free(obj_file);
}
#endif

UFBXT_TEST(maya_instanced_pivots)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	ufbxt_obj_file *obj_file = ufbxt_load_obj_file("maya_instanced_pivots", NULL);

	char path[512];
	ufbxt_file_iterator iter = { "maya_instanced_pivots" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t pv = 0; pv < UFBX_PIVOT_HANDLING_COUNT; pv++) {
			for (uint32_t gh = 0; gh < UFBX_GEOMETRY_TRANSFORM_HANDLING_COUNT; gh++) {
				ufbxt_hintf("pivot_handling=%u, geometry_transform_handling=%u", pv, gh);

				ufbx_load_opts opts = { 0 };

				opts.pivot_handling = (ufbx_pivot_handling)pv;
				opts.geometry_transform_handling = (ufbx_geometry_transform_handling)gh;

				ufbx_error error;
				ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
				if (!scene) ufbxt_log_error(&error);
				ufbxt_assert(scene);

				ufbxt_check_scene(scene);
				ufbxt_diff_to_obj(scene, obj_file, &err, 0);

				ufbx_node *cube1 = ufbx_find_node(scene, "pCube1");
				ufbx_node *cube2 = ufbx_find_node(scene, "pCube2");
				ufbx_node *light = ufbx_find_node(scene, "pointLight1");
				ufbxt_assert(cube1);
				ufbxt_assert(cube2);
				ufbxt_assert(light);

				ufbx_prop *prop = NULL;

				ufbxt_assert(scene->lights.count == 1);
				ufbx_light *light_attr = scene->lights.data[0];
				ufbxt_assert(light_attr->instances.count == 1);
				ufbx_node *light_parent = light_attr->instances.data[0];
				ufbxt_assert_close_vec3_xyz(&err, light_parent->geometry_to_world.cols[3], 0.0f, 1.414f, 0.586f);

				if ((pv == UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT || pv == UFBX_PIVOT_HANDLING_ADJUST_TO_ROTATION_PIVOT) && gh != UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY_NO_FALLBACK) {
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&cube1->props, "RotationPivot", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&cube1->props, "ScalingPivot", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);
					if (gh == UFBX_GEOMETRY_TRANSFORM_HANDLING_PRESERVE) {
						ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&cube1->props, "GeometricTranslation", ufbx_zero_vec3), -2.0f, 0.0f, 0.0f);
					} else {
						ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&cube1->props, "GeometricTranslation", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);
						ufbxt_assert(cube1->geometry_transform_helper);
					}

					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&cube2->props, "RotationPivot", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&cube2->props, "ScalingPivot", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);
					if (gh == UFBX_GEOMETRY_TRANSFORM_HANDLING_PRESERVE) {
						ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&cube2->props, "GeometricTranslation", ufbx_zero_vec3), 2.0f, 0.0f, 0.0f);
					} else {
						ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&cube2->props, "GeometricTranslation", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);
						ufbxt_assert(cube2->geometry_transform_helper);
					}

					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&light->props, "RotationPivot", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&light->props, "ScalingPivot", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);
					if (gh == UFBX_GEOMETRY_TRANSFORM_HANDLING_PRESERVE) {
						ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&light->props, "GeometricTranslation", ufbx_zero_vec3), 0.0f, 0.0f, -2.0f);
					} else {
						ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&light->props, "GeometricTranslation", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);
						ufbxt_assert(cube2->geometry_transform_helper);
					}

				} else {
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&cube1->props, "RotationPivot", ufbx_zero_vec3), 2.0f, 0.0f, 0.0f);
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&cube1->props, "ScalingPivot", ufbx_zero_vec3), 2.0f, 0.0f, 0.0f);
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&cube1->props, "GeometricTranslation", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);

					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&cube2->props, "RotationPivot", ufbx_zero_vec3), -2.0f, 0.0f, 0.0f);
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&cube2->props, "ScalingPivot", ufbx_zero_vec3), -2.0f, 0.0f, 0.0f);
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&cube2->props, "GeometricTranslation", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);

					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&light->props, "RotationPivot", ufbx_zero_vec3), 0.0f, 0.0f, 2.0f);
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&light->props, "ScalingPivot", ufbx_zero_vec3), 0.0f, 0.0f, 2.0f);
					ufbxt_assert_close_vec3_xyz(&err, ufbx_find_vec3(&light->props, "GeometricTranslation", ufbx_zero_vec3), 0.0f, 0.0f, 0.0f);
				}

				ufbx_free_scene(scene);
			}
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
	free(obj_file);
}
#endif

UFBXT_TEST(motionbuilder_pivot)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	ufbxt_obj_file *obj_file = ufbxt_load_obj_file("motionbuilder_pivot", NULL);

	char path[512];
	ufbxt_file_iterator iter = { "motionbuilder_pivot" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t pv = 0; pv < UFBX_PIVOT_HANDLING_COUNT; pv++) {
			for (uint32_t gh = 0; gh < UFBX_GEOMETRY_TRANSFORM_HANDLING_COUNT; gh++) {
				ufbxt_hintf("pivot_handling=%u, geometry_transform_handling=%u", pv, gh);

				ufbx_load_opts opts = { 0 };

				opts.pivot_handling = (ufbx_pivot_handling)pv;
				opts.geometry_transform_handling = (ufbx_geometry_transform_handling)gh;

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

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
	free(obj_file);
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_adjust_to_pivot_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.pivot_handling = UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS(maya_skinned_pivot, ufbxt_adjust_to_pivot_opts)
#if UFBXT_IMPL
{
}
#endif

UFBXT_TEST(maya_skinned_pivot_opts)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	ufbxt_obj_file *obj_file = ufbxt_load_obj_file("maya_skinned_pivot", NULL);

	char path[512];
	ufbxt_file_iterator iter = { "maya_skinned_pivot" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t pv = 0; pv < UFBX_PIVOT_HANDLING_COUNT; pv++) {
			for (uint32_t gh = 0; gh < UFBX_GEOMETRY_TRANSFORM_HANDLING_COUNT; gh++) {
				ufbxt_hintf("pivot_handling=%u, geometry_transform_handling=%u", pv, gh);

				ufbx_load_opts opts = { 0 };
				opts.evaluate_skinning = true;

				opts.pivot_handling = (ufbx_pivot_handling)pv;
				opts.geometry_transform_handling = (ufbx_geometry_transform_handling)gh;

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

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
	free(obj_file);
}
#endif

UFBXT_FILE_TEST_OPTS(maya_advanced_skinned_pivot, ufbxt_adjust_to_pivot_opts)
#if UFBXT_IMPL
{
}
#endif

UFBXT_TEST(maya_advanced_skinned_pivot_opts)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	ufbxt_obj_file *obj_file = ufbxt_load_obj_file("maya_advanced_skinned_pivot", NULL);

	char path[512];
	ufbxt_file_iterator iter = { "maya_advanced_skinned_pivot" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t pv = 0; pv < UFBX_PIVOT_HANDLING_COUNT; pv++) {
			for (uint32_t gh = 0; gh < UFBX_GEOMETRY_TRANSFORM_HANDLING_COUNT; gh++) {
				ufbxt_hintf("pivot_handling=%u, geometry_transform_handling=%u", pv, gh);

				ufbx_load_opts opts = { 0 };
				opts.evaluate_skinning = true;

				opts.pivot_handling = (ufbx_pivot_handling)pv;
				opts.geometry_transform_handling = (ufbx_geometry_transform_handling)gh;

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

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
	free(obj_file);
}
#endif

UFBXT_FILE_TEST(blender_340_mirrored_normals)
#if UFBXT_IMPL
{
}
#endif

UFBXT_FILE_TEST(maya_extrapolate_cube)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "maya_extrapolate_cube_26", NULL, 26.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_extrapolate_cube_60", NULL, 60.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_extrapolate_cube_80", NULL, 80.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_extrapolate_cube_100", NULL, 100.0/24.0);

	ufbxt_check_frame_imp(scene, err, true, "maya_extrapolate_cube_noex_28", NULL, 28.0/24.0, UFBXT_CHECK_FRAME_NO_EXTRAPOLATE);
	ufbxt_check_frame_imp(scene, err, true, "maya_extrapolate_cube_noex_34", NULL, 34.0/24.0, UFBXT_CHECK_FRAME_NO_EXTRAPOLATE);
	ufbxt_check_frame_imp(scene, err, true, "maya_extrapolate_cube_noex_44", NULL, 44.0/24.0, UFBXT_CHECK_FRAME_NO_EXTRAPOLATE);
	ufbxt_check_frame_imp(scene, err, true, "maya_extrapolate_cube_noex_68", NULL, 68.0/24.0, UFBXT_CHECK_FRAME_NO_EXTRAPOLATE);
	ufbxt_check_frame_imp(scene, err, true, "maya_extrapolate_cube_noex_68", NULL, 100.0/24.0, UFBXT_CHECK_FRAME_NO_EXTRAPOLATE);

	ufbxt_check_transform_consistency(err, scene, 24.0, 120, UFBXT_CHECK_TRANSFORM_CONSISTENCY_NO_BAKE);
	ufbxt_check_transform_consistency(err, scene, 24.0, 120, UFBXT_CHECK_TRANSFORM_CONSISTENCY_NO_EXTRAPOLATION);
}
#endif

UFBXT_FILE_TEST(maya_split_pivot)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_vec3 rotation_pivot = ufbx_find_vec3(&node->props, UFBX_RotationPivot, ufbx_zero_vec3);
	ufbx_vec3 scaling_pivot = ufbx_find_vec3(&node->props, UFBX_ScalingPivot, ufbx_zero_vec3);

	ufbxt_assert_vec3_equal(rotation_pivot, 2.0f, 0.0f, 0.0f);
	ufbxt_assert_vec3_equal(scaling_pivot, -0.5f, 0.0f, 0.0f);

	ufbxt_check_frame(scene, err, true, "maya_split_pivot", NULL, 1.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_split_pivot_6", NULL, 6.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_split_pivot_12", NULL, 12.0/24.0);
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_adjust_to_rotation_pivot_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.pivot_handling = UFBX_PIVOT_HANDLING_ADJUST_TO_ROTATION_PIVOT;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_split_pivot_adjust, maya_split_pivot, ufbxt_adjust_to_rotation_pivot_opts)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	{
		ufbx_transform transform = node->local_transform;
		ufbx_vec3 ref = { 2.0f, 0.0f, 0.0f };
		ufbxt_assert_close_vec3(err, transform.translation, ref);
	}

	{
		ufbx_transform transform = ufbx_evaluate_transform(scene->anim, node, 1.0/24.0);
		ufbx_vec3 ref = { 2.0f, 0.0f, 0.0f };
		ufbxt_assert_close_vec3(err, transform.translation, ref);
	}

	{
		ufbx_transform transform = ufbx_evaluate_transform(scene->anim, node, 6.0/24.0);
		ufbx_vec3 ref = { 2.0f, 0.0f, 0.0f };
		ufbxt_assert_close_vec3(err, transform.translation, ref);
	}

	{
		ufbx_transform transform = ufbx_evaluate_transform(scene->anim, node, 12.0/24.0);
		ufbx_vec3 ref = { 2.0f, 3.0f, 0.0f };
		ufbxt_assert_close_vec3(err, transform.translation, ref);
	}

	ufbxt_check_frame(scene, err, true, "maya_split_pivot", NULL, 1.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_split_pivot_6", NULL, 6.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_split_pivot_12", NULL, 12.0/24.0);
}
#endif

UFBXT_FILE_TEST(maya_pivot_offset)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "maya_pivot_offset", NULL, 1.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_pivot_offset_8", NULL, 8.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_pivot_offset_12", NULL, 12.0/24.0);

	{
		ufbx_node *node = ufbx_find_node(scene, "ScalingOffset");
		ufbxt_assert(node);

		ufbx_vec3 ref = { 20.0f, 0.0f, 0.0f };
		ufbxt_assert_close_vec3(err, node->local_transform.translation, ref);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "RotationOffset");
		ufbxt_assert(node);

		ufbx_vec3 ref = { 10.0f, 0.0f, 10.0f };
		ufbxt_assert_close_vec3(err, node->local_transform.translation, ref);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "BothOffsets");
		ufbxt_assert(node);

		ufbx_vec3 ref = { 20.0f, 0.0f, 00.0f };
		ufbxt_assert_close_vec3(err, node->local_transform.translation, ref);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "PivotOffsets");
		ufbxt_assert(node);

		ufbx_vec3 ref = { 10.0f, 0.0f, 30.0f };
		ufbxt_assert_close_vec3(err, node->local_transform.translation, ref);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_pivot_offset_legacy_adjust, maya_pivot_offset, ufbxt_adjust_to_pivot_opts)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "maya_pivot_offset", NULL, 1.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_pivot_offset_8", NULL, 8.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_pivot_offset_12", NULL, 12.0/24.0);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_pivot_offset_rotation_adjust, maya_pivot_offset, ufbxt_adjust_to_rotation_pivot_opts)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "maya_pivot_offset", NULL, 1.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_pivot_offset_8", NULL, 8.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_pivot_offset_12", NULL, 12.0/24.0);

	{
		ufbx_node *node = ufbx_find_node(scene, "ScalingOffset");
		ufbxt_assert(node);

		ufbx_vec3 ref = { 0.0f, 0.0f, 0.0f };
		ufbxt_assert_close_vec3(err, node->local_transform.translation, ref);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "RotationOffset");
		ufbxt_assert(node);

		ufbx_vec3 ref = { 10.0f, 0.0f, 10.0f };
		ufbxt_assert_close_vec3(err, node->local_transform.translation, ref);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "BothOffsets");
		ufbxt_assert(node);

		ufbx_vec3 ref = { 20.0f, 0.0f, 20.0f };
		ufbxt_assert_close_vec3(err, node->local_transform.translation, ref);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "PivotOffsets");
		ufbxt_assert(node);

		ufbx_vec3 ref = { 10.0f, 0.0f, 20.0f };
		ufbxt_assert_close_vec3(err, node->local_transform.translation, ref);
	}
}
#endif

UFBXT_FILE_TEST(maya_zero_scale_pivot)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "maya_zero_scale_pivot", NULL, 1.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_zero_scale_pivot_6", NULL, 6.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_zero_scale_pivot_12", NULL, 12.0/24.0);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_zero_scale_pivot_legacy_adjust, maya_zero_scale_pivot, ufbxt_adjust_to_pivot_opts)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "maya_zero_scale_pivot", NULL, 1.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_zero_scale_pivot_6", NULL, 6.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_zero_scale_pivot_12", NULL, 12.0/24.0);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_zero_scale_pivot_rotation_adjust, maya_zero_scale_pivot, ufbxt_adjust_to_rotation_pivot_opts)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "maya_zero_scale_pivot", NULL, 1.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_zero_scale_pivot_6", NULL, 6.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_zero_scale_pivot_12", NULL, 12.0/24.0);

	{
		ufbx_node *node = ufbx_find_node(scene, "Ortho_Parent");
		ufbxt_assert(node);

		ufbx_vec3 begin_ref = { 10.0f, 0.0f, 0.0f };
		ufbx_vec3 end_ref = { 10.0f, 0.0f, 0.0f };
		ufbx_vec3 begin_pos = node->local_transform.translation;
		ufbx_vec3 end_pos = ufbx_evaluate_transform(scene->anim, node, 1.0).translation;
		ufbxt_assert_close_vec3(err, begin_pos, begin_ref);
		ufbxt_assert_close_vec3(err, end_pos, end_ref);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Parallel_Parent");
		ufbxt_assert(node);

		ufbx_vec3 begin_ref = { 10.0f, 0.0f, 0.0f }; // incorrect due to zero scale
		ufbx_vec3 end_ref = { -10.0f, 0.0f, 0.0f };
		ufbx_vec3 begin_pos = node->local_transform.translation;
		ufbx_vec3 end_pos = ufbx_evaluate_transform(scene->anim, node, 1.0).translation;
		ufbxt_assert_close_vec3_threshold(err, begin_pos, begin_ref, 0.01f);
		ufbxt_assert_close_vec3(err, end_pos, end_ref);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Align_Parent");
		ufbxt_assert(node);

		ufbx_vec3 begin_ref = { 0.0f, 0.0f, 10.0f };
		ufbx_vec3 end_ref = { 0.0f, 0.0f, 10.0f };
		ufbx_vec3 begin_pos = node->local_transform.translation;
		ufbx_vec3 end_pos = ufbx_evaluate_transform(scene->anim, node, 1.0).translation;
		ufbxt_assert_close_vec3(err, begin_pos, begin_ref);
		ufbxt_assert_close_vec3(err, end_pos, end_ref);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Offset_Parent");
		ufbxt_assert(node);

		ufbx_vec3 begin_ref = { 0.0f, 0.0f, 0.0f }; // incorrect due to zero scale
		ufbx_vec3 end_ref = { 0.0f, 0.0f, -10.0f };
		ufbx_vec3 begin_pos = node->local_transform.translation;
		ufbx_vec3 end_pos = ufbx_evaluate_transform(scene->anim, node, 1.0).translation;
		ufbxt_assert_close_vec3(err, begin_pos, begin_ref);
		ufbxt_assert_close_vec3(err, end_pos, end_ref);
	}
}
#endif

UFBXT_FILE_TEST(maya_null_pivots)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "maya_null_pivots", NULL, 1.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_null_pivots_6", NULL, 6.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_null_pivots_12", NULL, 12.0/24.0);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_null_pivots_adjust, maya_null_pivots, ufbxt_adjust_to_rotation_pivot_opts)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, true, "maya_null_pivots", NULL, 1.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_null_pivots_6", NULL, 6.0/24.0);
	ufbxt_check_frame(scene, err, true, "maya_null_pivots_12", NULL, 12.0/24.0);
}
#endif

UFBXT_TEST(maya_null_pivots_opts)
#if UFBXT_IMPL
{
	ufbxt_diff_error err = { 0 };

	ufbxt_obj_file *obj_file = ufbxt_load_obj_file("maya_null_pivots", NULL);

	char path[512];
	ufbxt_file_iterator iter = { "maya_null_pivots" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (uint32_t pv = 0; pv < UFBX_PIVOT_HANDLING_COUNT; pv++) {
			for (uint32_t gh = 0; gh < UFBX_GEOMETRY_TRANSFORM_HANDLING_COUNT; gh++) {
				for (uint32_t re = 0; re < 2; re++) {
					ufbxt_hintf("pivot_handling=%u, geometry_transform_handling=%u, pivot_handling_retain_empties=%u", pv, gh, re);

					ufbx_load_opts opts = { 0 };
					opts.pivot_handling = (ufbx_pivot_handling)pv;
					opts.pivot_handling_retain_empties = re != 0;
					opts.geometry_transform_handling = (ufbx_geometry_transform_handling)gh;

					bool top_adjusted = false;
					bool mid_adjusted = false;
					bool top_helper = false;
					bool mid_helper = false;
					switch (opts.pivot_handling) {
					case UFBX_PIVOT_HANDLING_RETAIN:
						break;

					case UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT:
						switch (opts.geometry_transform_handling) {
						case UFBX_GEOMETRY_TRANSFORM_HANDLING_PRESERVE:
							top_adjusted = true;
							break;
						case UFBX_GEOMETRY_TRANSFORM_HANDLING_HELPER_NODES:
						case UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY:
							top_adjusted = true;
							top_helper = true;
							break;
						case UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY_NO_FALLBACK:
							break;
						}
						break;

					case UFBX_PIVOT_HANDLING_ADJUST_TO_ROTATION_PIVOT:
						if (!opts.pivot_handling_retain_empties) {
							top_adjusted = true;
							mid_adjusted = true;
						}
						break;
					}

					ufbx_error error;
					ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
					if (!scene) ufbxt_log_error(&error);
					ufbxt_assert(scene);
					ufbxt_check_scene(scene);

					{
						ufbx_node *node = ufbx_find_node(scene, "Top");
						ufbxt_assert(node);
						if (top_helper) {
							ufbxt_assert(node->attrib_type == UFBX_ELEMENT_UNKNOWN);
							ufbxt_assert(node->geometry_transform_helper);
						} else {
							ufbxt_assert(node->attrib_type == UFBX_ELEMENT_EMPTY);
							ufbxt_assert(!node->geometry_transform_helper);
						}

						if (top_adjusted) {
							ufbx_vec3 ref = { 10.0f, 0.0f, 0.0f };
							ufbxt_assert_close_vec3(&err, node->local_transform.translation, ref);
						} else {
							ufbx_vec3 ref = { 0.0f, 0.0f, 0.0f };
							ufbxt_assert_close_vec3(&err, node->local_transform.translation, ref);
						}
					}

					{
						ufbx_node *node = ufbx_find_node(scene, "Mid");
						ufbxt_assert(node);
						if (mid_helper) {
							ufbxt_assert(node->attrib_type == UFBX_ELEMENT_UNKNOWN);
							ufbxt_assert(node->geometry_transform_helper);
						} else {
							ufbxt_assert(node->attrib_type == UFBX_ELEMENT_EMPTY);
							ufbxt_assert(!node->geometry_transform_helper);
						}

						if (mid_adjusted) {
							ufbx_vec3 ref = { top_adjusted ? -20.0f : -10.0f, 0.0f, 0.0f };
							ufbxt_assert_close_vec3(&err, node->local_transform.translation, ref);
						} else {
							ufbx_vec3 ref = { top_adjusted ? -5.0f : 5.0f, 0.0f, 0.0f };
							ufbxt_assert_close_vec3(&err, node->local_transform.translation, ref);
						}
					}

					ufbxt_diff_to_obj(scene, obj_file, &err, 0);
					ufbxt_check_frame(scene, &err, true, "maya_null_pivots", NULL, 1.0/24.0);
					ufbxt_check_frame(scene, &err, true, "maya_null_pivots_6", NULL, 6.0/24.0);
					ufbxt_check_frame(scene, &err, true, "maya_null_pivots_12", NULL, 12.0/24.0);

					ufbx_free_scene(scene);
				}
			}
		}
	}

	ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", err.sum / (ufbx_real)err.num, err.max, err.num);
	free(obj_file);
}
#endif
