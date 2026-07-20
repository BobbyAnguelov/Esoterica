#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "animation"

#if UFBXT_IMPL
typedef struct {
	int frame;
	double value;
} ufbxt_key_ref;
#endif

UFBXT_FILE_TEST(maya_interpolation_modes)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		-8.653366, // Start from zero time
		-8.653366,-8.602998,-8.464664,-8.257528,-8.00075,-7.713489,-7.414906,-7.124163,-6.86042,
		-6.642837,-6.490576,-6.388305,-6.306414,-6.242637,-6.19471,-6.160368,-6.137348,-6.123385,
		-6.116215,-6.113573,-6.113196,-5.969524,-5.825851,-5.682179,-5.538507,-5.394835,-5.251163,
		-5.107491,-4.963819,-4.820146,-4.676474,-4.532802,-4.38913,-4.245458,-4.101785,-3.958113,-4.1529,
		-4.347686,-4.542472,-4.737258,-4.932045,-5.126832,-5.321618,-5.516404,-5.71119,-5.905977,-5.767788,
		-5.315578,-4.954943,-4.83559,-4.856855,-4.960766,-5.118543,-4.976541,-4.885909,-4.865979,-4.93845,
		-5.099224,-5.270246,-5.359269,-5.349404,-5.261964,-5.118543,-5.264501,-5.33535,-5.285445,-5.058857,
		-4.69383,-4.357775,-4.124978,-3.981697,-3.904232,-3.875225,-3.875225,-3.875225,-3.875225,-3.875225,
		-3.875225,-3.875225,-2.942738,-2.942738,-2.942738,-2.942738,-2.942738,-2.942738,-2.942738,-2.942738,
		-2.942738,-1.243537,-1.243537,-1.243537,-1.243537,-1.243537,-1.243537,-1.243537,5.603338,5.603338,
		5.603338,5.603338,5.603338,5.603338,5.603338,5.603338,5.603338,5.603338,5.603338,5.603338,5.603338,
		5.603338,5.603338,5.603338,5.603338,5.603338,5.603338,5.603338,5.603338,5.603338,5.603338,5.603338,5.603338
	};

	ufbxt_assert(scene->anim_layers.count == 1);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	for (size_t i = 0; i < layer->anim_values.count; i++) {
		ufbx_anim_value *value = layer->anim_values.data[i];
		if (strcmp(value->name.data, "Lcl Translation")) continue;
		ufbx_anim_curve *curve = value->curves[0];

		size_t num_keys = 12;
		ufbxt_assert(curve->keyframes.count == num_keys);
		ufbx_keyframe *keys = curve->keyframes.data;

		static const ufbxt_key_ref key_ref[] = {
			{ 1, -8.653366 },
			{ 11, -6.490576 },
			{ 21, -6.113196 },
			{ 36, -3.958113 },
			{ 46, -5.905977 },
			{ 53, -5.118543 },
			{ 63, -5.118543 },
			{ 73, -3.875225 },
			{ 80, -2.942738 },
			{ 89, -1.927362 },
			{ 96, -1.243537 },
			{ 120, 5.603338 },
		};
		ufbxt_assert(ufbxt_arraycount(key_ref) == num_keys);

		for (size_t i = 0; i < num_keys; i++) {
			ufbxt_assert_close_double(err, keys[i].time, (double)key_ref[i].frame / 24.0);
			ufbxt_assert_close_real(err, keys[i].value, (ufbx_real)key_ref[i].value);
			if (i > 0) ufbxt_assert(keys[i].left.dx > 0.0f);
			if (i + 1 < num_keys) ufbxt_assert(keys[i].right.dx > 0.0f);
		}

		ufbxt_assert(keys[0].interpolation == UFBX_INTERPOLATION_CUBIC);
		ufbxt_assert(keys[0].right.dy == 0.0f);
		ufbxt_assert(keys[1].interpolation == UFBX_INTERPOLATION_CUBIC);
		ufbxt_assert_close_real(err, keys[1].left.dy/keys[1].left.dx, keys[1].right.dy/keys[1].left.dx);
		ufbxt_assert(keys[2].interpolation == UFBX_INTERPOLATION_LINEAR);
		ufbxt_assert_close_real(err, keys[3].left.dy/keys[3].left.dx, keys[2].right.dy/keys[2].right.dx);
		ufbxt_assert(keys[3].interpolation == UFBX_INTERPOLATION_LINEAR);
		ufbxt_assert_close_real(err, keys[4].left.dy/keys[4].left.dx, keys[3].right.dy/keys[3].right.dx);
		ufbxt_assert(keys[4].interpolation == UFBX_INTERPOLATION_CUBIC);
		ufbxt_assert(keys[4].right.dy == 0.0f);
		ufbxt_assert(keys[5].interpolation == UFBX_INTERPOLATION_CUBIC);
		ufbxt_assert(keys[5].left.dy < 0.0f);
		ufbxt_assert(keys[5].right.dy > 0.0f);
		ufbxt_assert(keys[6].interpolation == UFBX_INTERPOLATION_CUBIC);
		ufbxt_assert(keys[6].left.dy > 0.0f);
		ufbxt_assert(keys[6].right.dy < 0.0f);
		ufbxt_assert(keys[7].interpolation == UFBX_INTERPOLATION_CONSTANT_PREV);
		ufbxt_assert(keys[8].interpolation == UFBX_INTERPOLATION_CONSTANT_PREV);
		ufbxt_assert(keys[9].interpolation == UFBX_INTERPOLATION_CONSTANT_NEXT);
		ufbxt_assert(keys[10].interpolation == UFBX_INTERPOLATION_CONSTANT_NEXT);

		for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
			// Round up to the next frame to make stepped tangents consistent
			double time = (double)i * (1.0/24.0) + 0.000001;
			ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
			ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
		}

		size_t num_samples = 64 * 1024;
		for (size_t i = 0; i < num_samples; i++) {
			double time = (double)i * (5.0 / (double)num_samples);
			ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
			ufbxt_assert(value >= -16.0f && value <= 16.0f);
		}
	}
}
#endif

UFBXT_FILE_TEST(maya_auto_clamp)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000, 0.000, 0.273, 0.515, 0.718, 0.868, 0.945, 0.920, 0.779, 0.611,
		0.591, 0.747, 1.206, 2.059, 3.191, 4.489, 5.837, 7.121, 8.228, 9.042,
		9.449, 9.694, 10.128, 10.610, 10.873, 10.927, 10.854, 10.704, 10.502,
		10.264, 10.000,
	};

	ufbxt_assert(scene->anim_layers.count == 1);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	for (size_t i = 0; i < layer->anim_values.count; i++) {
		ufbx_anim_value *value = layer->anim_values.data[i];
		if (strcmp(value->name.data, "Lcl Translation")) continue;
		ufbx_anim_curve *curve = value->curves[0];
		ufbxt_assert(curve->keyframes.count == 4);

		for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
			double time = (double)i * (1.0/24.0);
			ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
			ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
		}
	}
}
#endif

UFBXT_FILE_TEST(maya_tangent_spline)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.00000, 3.50000, 6.00000, 6.51562, 6.37500, 6.04688, 6.00000,
		6.67313, 7.71167, 8.39438, 8.00000, 3.63667, -1.04000, -1.96474,
		-1.68059, -1.00000, 0.27700, 2.00000,
	};

	ufbxt_assert(scene->anim_layers.count == 1);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
		ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
	}
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_clamp_maya_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.key_clamp_threshold = 0.05;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS(maya_tangent_clamped, ufbxt_clamp_maya_opts)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.00000, 3.75000, 6.00000, 6.00000, 6.00000, 6.00000, 6.00000,
		6.53250, 7.58667, 8.34750, 8.00000, 3.18667, -1.04000, -1.02963,
		-1.01037, -1.00000, 0.12500, 2.00000,
	};

	ufbxt_assert(scene->anim_layers.count == 1);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
		ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
	}
}
#endif

UFBXT_FILE_TEST(motionbuilder_tangent_linear)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 0.000000, 3.333333, 6.666667, 10.000000, 8.750000, 7.500000,
		6.250000, 5.000000, 3.750000, 2.500000, 1.250000, 0.000000, 0.000000,
		1.333333, 2.666667, 4.000000, 0.000000, -4.000000, -4.000000, -4.000000,
		-4.000000, -4.000000, -1.000000, 2.000000, -1.000000, -4.000000, -4.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
		ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
	}
}
#endif

UFBXT_FILE_TEST(motionbuilder_tangent_auto)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 0.000000, 2.592592, 7.407408, 10.000000, 9.570312,
		8.437500, 6.835938, 5.000000, 3.164062, 1.562500, 0.429688,
		0.000000, 0.000000, 1.037037, 2.962963, 4.000000, 0.000000,
		-4.000000, -4.000000, -4.000000, -4.000000, -4.000000,
		-1.000000, 2.000000, -1.000000, -4.000000, -4.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
		ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
	}
}
#endif

UFBXT_FILE_TEST(motionbuilder_tangent_spline)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 0.000000, 3.703704, 7.962963, 10.000000, 9.691840,
		8.854167, 7.617188, 6.111111, 4.466146, 2.812500, 1.280382,
		0.000000, 0.000000, 1.659259, 3.540741, 4.000000, 0.133333,
		-4.000000, -4.937500, -5.166667, -4.812500, -4.000000,
		-0.750000, 2.000000, -0.625000, -4.000000, -4.375000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
		ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
	}
}
#endif

UFBXT_FILE_TEST(motionbuilder_tangent_spline_clamp)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 0.000000, 2.592592, 7.407408, 10.000000, 9.570312,
		8.437500, 6.835938, 5.000000, 3.164062, 1.562500, 0.429688,
		0.000000, 0.000000, 1.214815, 3.318519, 4.000000, -0.200000,
		-4.000000, -4.000000, -4.000000, -4.000000, -4.000000, -1.000000,
		2.000000, -1.000000, -4.000000, -4.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
		ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
	}
}
#endif

UFBXT_FILE_TEST(motionbuilder_tangent_smooth)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 2.851562, 5.937500, 8.554688, 10.000000, 10.117188,
		9.375000, 8.007812, 6.250000, 4.335938, 2.500000, 0.976562,
		0.000000, 0.000000, 1.148148, 2.518519, 2.000000, -3.041667,
		-8.000000, -8.500000, -7.000000, -5.000000, -4.000000, -4.906250,
		-6.750000, -8.218750, -8.000000, -4.625000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
		ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
	}
}
#endif

UFBXT_FILE_TEST(motionbuilder_tangent_smooth_clamp)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 0.000000, 2.361111, 6.944445, 10.000000, 10.367839, 9.609375,
		8.056641, 6.041667, 3.896484, 1.953125, 0.543620, 0.000000, 0.000000,
		1.333333, 3.555556, 4.000000, -0.333333, -4.000000, -4.000000, -4.000000,
		-4.000000, -4.000000, -1.000000, 2.000000, -1.000000, -4.000000, -4.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
		ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
	}
}
#endif

UFBXT_FILE_TEST(motionbuilder_tangent_weight)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 1.666723, 3.333501, 5.000291, 6.667066, 8.333776, 10.000000, 8.333776, 6.667066,
		5.000291, 3.333500, 1.666723, 0.000000, -3.286567, -7.675018, -9.056619, -9.661684, -9.928119,
		-10.000000, -9.928119, -9.661685, -9.056619, -7.675017, -3.286567, 0.000000, 2.594488, 6.547710,
		9.385767, 10.524116, 10.590028, 10.000000, 8.817327, 6.850233, 3.941248, -0.101893, -5.382658,
		-10.000000, -2.000851, 2.786134, 5.659585, 7.615819, 9.005583, 10.000000, 10.714197, 10.996990,
		6.856248, 0.950144, 0.174010, 0.000000, 0.106695, 0.495261, 1.353501, 3.215956, 8.157769,
		10.000000, 9.696111, 7.804129, 5.550874, 3.545988, 1.713910, 0.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
		if (i == 37) {
			// Hard Bezier solve, might be an inaccuracy in MotionBuilder
			ufbxt_assert_close_real_threshold(err, value, (ufbx_real)values[i], 0.01f);
		} else {
			ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
		}
	}
}
#endif

UFBXT_FILE_TEST(motionbuilder_tangent_weight_split)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 2.452671, 6.056693, 8.325726, 9.401590, 9.873834, 10.000000, 8.333776, 6.667066,
		5.000291, 3.333500, 1.666723, 0.000000, -1.848064, -4.097735, -6.603552, -8.662155, -9.723830,
		-10.000000, -9.925971, -9.635168, -8.897875, -6.736949, -2.281059, 0.000000, 2.105399, 4.761521,
		7.361103, 9.316892, 10.244871, 10.000000, 4.726504, -0.810048, -5.004114, -7.855899, -9.481855,
		-10.000000, -2.000851, 2.786134, 5.659585, 7.615819, 9.005583, 10.000000, 10.449002, 9.610980,
		6.124925, 2.117854, 0.408753, 0.000000, 0.106695, 0.495261, 1.353501, 3.215956, 8.157769,
		10.000000, 9.859852, 9.295445, 7.872536, 5.005719, 2.033110, 0.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
		if (i == 37) {
			// Hard Bezier solve, might be an inaccuracy in MotionBuilder
			ufbxt_assert_close_real_threshold(err, value, (ufbx_real)values[i], 0.01f);
		} else {
			ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
		}
	}
}
#endif

UFBXT_FILE_TEST(motionbuilder_tangent_velocity)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 1.306946, 3.600664, 6.350164, 8.505674, 9.667060, 10.000000, 9.640089, 8.020978,
		4.811217, 2.299784, 0.849270, 0.000000, -0.841173, -2.236577, -4.544493, -7.658146, -9.554997,
		-10.000000, -9.524388, -7.843781, -5.036027, -2.355318, -0.707333, 0.000000, 0.699153, 2.165492,
		4.372254, 7.037397, 9.356667, 10.000000, 5.273193, -2.875481, -6.854944, -8.828015, -9.745026,
		-10.000000, -9.747821, -8.858396, -7.006244, -3.480528, 3.549764, 10.000000, 8.909218, 6.263762,
		3.659487, 1.646342, 0.411071, 0.000000, 0.375149, 1.510146, 3.380960, 5.851594, 8.490248,
		10.000000, 7.836923, 4.849110, 2.974404, 1.677659, 0.723459, 0.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
		ufbxt_assert_close_real_threshold(err, value, (ufbx_real)values[i], 0.01f);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_tangent_velocity_split, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 1.306946, 3.600664, 6.350164, 8.505674, 9.667060, 10.000000, 9.715650, 8.870866,
		7.474936, 5.532619, 3.043366, 0.000000, -0.877294, -2.555327, -6.032461, -8.885489, -9.799076,
		-10.000000, -8.879360, -5.191613, -2.533277, -1.116049, -0.358573, 0.000000, 5.621936, 8.243435,
		9.649088, 10.286535, 10.363522, 10.000000, 8.181775, 5.144475, 1.450959, -2.605475, -6.752494,
		-10.000000, -9.440899, -7.391303, -3.113694, 3.258694, 8.264191, 10.000000, 10.449370, 9.756855,
		7.048309, 2.938391, 0.589423, 0.000000, 1.233396, 5.192722, 7.906461, 9.234742, 9.835874,
		10.000000, 8.548979, 5.938212, 3.775638, 2.145814, 0.920811, 0.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);

		// This file contains _negative_ velocities, which don't seem to be well supported,
		// so let's give it some leeway..
		ufbxt_assert_close_real_threshold(err, value, (ufbx_real)values[i], 0.01f);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_tangent_auto_bias, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 0.896991, 1.898148, 2.968750, 4.074074, 5.179398, 6.250000, 7.251157, 8.148148,
		8.906251, 9.490741, 9.866899, 10.000000, 8.125000, 5.000000, 3.190000, 2.586667, 2.930000,
		3.960000, 5.416667, 7.040000, 8.570000, 9.746667, 10.310000, 10.000000, 9.224537, 8.518518,
		7.812500, 7.037037, 6.122685, 5.000000, 3.146296, 1.014815, 0.000000, 1.855556, 5.927778,
		10.000000, 11.805555, 11.111111, 8.750000, 5.555555, 2.361110, 0.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);

		ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_tangent_auto_bias_split, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 0.896991, 1.898148, 2.968750, 4.074074, 5.179398, 6.250000, 7.251157, 8.148148,
		8.906251, 9.490741, 9.866899, 10.000000, 6.958333, 5.000000, 3.070000, 2.160000, 2.090000,
		2.680000, 3.750000, 5.120000, 6.610000, 8.039999, 9.230000, 10.000000, 8.935184, 7.592593,
		6.250000, 5.185185, 4.675926, 5.000000, 3.705556, 0.744444, 0.000000, 1.855556, 5.927778,
		10.000000, 6.597222, 4.444445, 3.125000, 2.222222, 1.319445, 0.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);

		ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_tangent_auto_bias_clamp, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 1.019097, 2.069444, 3.140625, 4.222222, 5.303820, 6.375000, 7.425347, 8.444445,
		9.421875, 10.347222, 11.210069, 12.000000, 12.160000, 12.179999, 12.120000, 12.040000, 12.000000,
		11.612245, 10.612245, 9.244898, 7.755102, 6.387755, 5.387755, 5.000000, 4.537037, 3.518518,
		2.500000, 2.037037, 2.685185, 5.000000, 4.816667, 1.300000, 0.000000, 1.855556, 5.927778,
		10.000000, 6.597222, 4.444445, 3.125000, 2.222222, 1.319445, 0.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);

		ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
	}
}
#endif

UFBXT_FILE_TEST(motionbuilder_tangent_auto_bias_absolute_simple)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 12.731482, 46.296295, 93.750000, 148.148148, 202.546310, 250.000000, 283.564819, 296.296295,
		281.250000, 231.481476, 140.046295, 0.000000, -140.046310, -231.481476, -281.250000, -296.296295, -283.564819,
		-250.000000, -202.546310, -148.148148, -93.750008, -46.296303, -12.731483, 0.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);

		ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_tangent_auto_bias_absolute, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 30.833332, 41.111111, 37.500000, 26.666666, 15.277778, 10.000000, 10.856482, 12.962962,
		15.624999, 18.148148, 19.837963, 20.000000, 18.634037, 15.264155, 11.020745, 7.070707, 3.423960,
		0.000000, -3.454167, -7.205388, -11.360994, -15.907764, -19.289827, -20.000000, -19.982880, -19.254641,
		-16.822596, -12.495454, -6.726660, 0.000000, 6.599604, 11.988842, 15.768107, 17.791979, 18.772791,
		20.000000, 22.530613, 26.054420, 29.469387, 31.673466, 31.564625, 28.040813, 20.000000, 16.203705,
		20.407408, 24.000000, 24.065104, 23.562500, 22.695312, 21.666668, 20.679688, 19.937500, 19.643229,
		20.000000, 14.733797, 2.148147, -10.687501, -16.703705, -8.831019, 20.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);

		ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_tangent_auto_bias_absolute_split, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, -208.333328, -166.666672, 0.000000, 166.666672, 208.333328, 0.000000, 208.333328, 166.666672,
		0.000000, -166.666672, -208.333328, 0.000000, -1134.259277, -907.407410, 0.000000, 907.407410, 1134.259277,
		0.000000, -52.083332, -166.666672, -281.250000, -333.333344, -260.416656, 0.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);

		ufbxt_assert_close_real_threshold(err, value, (ufbx_real)values[i], 0.01f);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_tangent_auto_bias_absolute_clamp, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, -260.416656, -333.333344, -281.250000, -166.666672, -52.083328, 0.000000, -52.083336, -166.666672,
		-281.250000, -333.333344, -260.416656, 0.000000, -1417.824097, -1814.814819, -1531.250000, -907.407410, -283.564819,
		0.000000, -52.083332, -166.666672, -281.250000, -333.333344, -260.416656, 0.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);

		ufbxt_assert_close_real_threshold(err, value, (ufbx_real)values[i], 0.01f);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_tangent_auto_bias_absolute_sign, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, -1.145833, -4.166667, -8.437500, -13.333333, -18.229168, -22.500000, -25.520832, -26.666666,
		-25.312500, -20.833332, -12.604168, 0.000000, 13.750000, 25.000000, 33.750000, 40.000000, 43.750000,
		45.000000, 43.750000, 40.000000, 33.750000, 24.999998, 13.749999, 0.000000, -10.416666, -13.333333,
		-11.250000, -6.666667, -2.083333, 0.000000, -0.000001, -0.000002, 0.000000, 1.145844, 4.166698,
		8.437560, 13.333425, 18.229292, 22.500154, 25.521011, 26.666861, 25.312698, 20.833519, 12.604320,
		0.000100, -13.749959, -25.000004, -33.750038, -40.000061, -43.750072, -45.000076, -43.750076, -40.000069,
		-33.750053, -25.000038, -13.750021, 0.000000, 10.416676, 13.333332, 11.249978, 6.666616, 2.083255,
		-0.000100,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);

		ufbxt_assert_close_real_threshold(err, value, (ufbx_real)values[i], 0.01f);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_tangent_auto_bias_absolute_negative, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE|UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->metadata.has_warning[UFBX_WARNING_MISSING_GEOMETRY_DATA]);

	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, 11.652200, 16.689816, 16.484375, 12.407409, 5.830441, -1.874999, -9.337385, -15.185184,
		-18.046875, -16.550926, -9.325809, 5.000000, 21.818577, 35.590279, 46.289062, 53.888889, 58.363716,
		59.687500, 57.834198, 52.777775, 44.492184, 32.951385, 18.129337, 0.000000, -15.677082, -23.666668,
		-24.781250, -19.833334, -9.635417, 5.000000, 20.174999, 32.466667, 41.774998, 48.000000, 51.041664,
		50.800003, 47.174999, 40.066669, 29.375004, 15.000000, -0.205000, -13.040000, -23.085001, -29.920002,
		-33.125000, -32.279999, -26.965002, -16.760004, -1.245000, 20.000000, 37.912502, 44.800003, 43.137501,
		35.400002, 24.062502, 11.600001, 0.487498, -6.800000, -7.787500, 0.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 1);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);

		ufbxt_assert_close_real_threshold(err, value, (ufbx_real)values[i], 0.01f);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_tangent_tcb, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	// Curve evaluated values at 24fps
	static const double values[] = {
		0.000000, -0.839699, -2.648148, -4.546875, -5.657407, -5.101273, -2.000000, -1.805556, -1.222222,
		-0.250000, 1.111111, 2.861111, 5.000000, 6.862268, 7.870370, 8.156250, 7.851851, 7.089120,
		6.000000, 4.716435, 3.370370, 2.093750, 1.018518, 0.276620, 0.000000, 2.407407, 3.481481,
		4.000000, 4.740741, 6.481482, 10.000000, 7.193704, 5.340741, 4.290000, 3.890371, 3.990741,
		4.440001, 5.087037, 5.780741, 6.370000, 6.703704, 6.630741, 6.000000, 4.702963, 2.951704,
		1.076000, -0.594370, -1.729629, -2.000000, -0.186167, 2.875667, 4.000000, 3.947917, 2.680556,
		0.875000, -0.791667, -1.642361, -1.000000, -0.069444, 2.354167, 4.000000,
	};

	ufbxt_assert(scene->anim_layers.count == 2);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(layer, &node->element, UFBX_Lcl_Translation);
	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(values); i++) {
		ufbxt_hintf("i=%zu", i);
		double time = (double)i / 24.0;
		ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);

		ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
	}
}
#endif

UFBXT_FILE_TEST(maya_resampled)
#if UFBXT_IMPL
{
	static const double values6[] = {
		0,0,0,0,0,0,0,0,0,
		-0.004, -0.022, -0.056, -0.104, -0.166, -0.241, -0.328, -0.427, -0.536, -0.654, -0.783,
		-0.919, -1.063, -1.214, -1.371, -1.533, -1.700, -1.871, -2.044, -2.220, -2.398, -2.577,
		-2.755, -2.933, -3.109, -3.283, -3.454, -3.621, -3.784, -3.941, -4.093, -4.237, -4.374,
		-4.503, -4.623, -4.733, -4.832, -4.920, -4.996, -5.059, -5.108, -5.143, -5.168, -5.186,
		-5.200, -5.209, -5.215, -5.218, -5.220, -5.220, -5.216, -5.192, -5.151, -5.091, -5.013,
		-4.919, -4.810, -4.686,
	};

	static const double values7[] = {
		0,0,0,0,0,0,0,0,
		0.000, -0.004, -0.025, -0.061, -0.112, -0.176, -0.252, -0.337, -0.431, -0.533, -0.648, 
		-0.776, -0.915, -1.064, -1.219, -1.378, -1.539, -1.700, -1.865, -2.037, -2.216, -2.397, -2.580, 
		-2.761, -2.939, -3.111, -3.278, -3.447, -3.615, -3.782, -3.943, -4.098, -4.244, -4.379, 
		-4.500, -4.614, -4.722, -4.821, -4.911, -4.990, -5.056, -5.107, -5.143, -5.168, -5.186, -5.200, 
		-5.209, -5.215, -5.218, -5.220, -5.220, -5.215, -5.190, -5.145, -5.082, -5.002, -4.908, 
		-4.800, -4.680, -4.550, -4.403, -4.239, 
	};

	const double *values = scene->metadata.version >= 7000 ? values7 : values6;
	size_t num_values = scene->metadata.version >= 7000 ? ufbxt_arraycount(values7) : ufbxt_arraycount(values6);

	ufbxt_assert(scene->anim_layers.count == 1);
	ufbx_anim_layer *layer = scene->anim_layers.data[0];
	for (size_t i = 0; i < layer->anim_values.count; i++) {
		ufbx_anim_value *value = layer->anim_values.data[i];
		if (strcmp(value->name.data, "Lcl Translation")) continue;
		ufbx_anim_curve *curve = value->curves[0];

		for (size_t i = 0; i < num_values; i++) {
			double time = (double)i * (1.0/200.0);
			ufbx_real value = ufbx_evaluate_curve(curve, time, 0.0);
			ufbxt_assert_close_real(err, value, (ufbx_real)values[i]);
		}
	}
}
#endif

#if UFBXT_IMPL

typedef struct {
	int frame;
	ufbx_real intensity;
	ufbx_vec3 color;
} ufbxt_anim_light_ref;

typedef struct {
	int frame;
	ufbx_vec3 translation;
	ufbx_vec3 rotation_euler;
	ufbx_vec3 scale;
} ufbxt_anim_transform_ref;

#endif

UFBXT_FILE_TEST(maya_anim_light)
#if UFBXT_IMPL
{
	static const ufbxt_anim_light_ref refs[] = {
		{  0, 3.072f, { 0.148f, 0.095f, 0.440f } },
		{ 12, 1.638f, { 0.102f, 0.136f, 0.335f } },
		{ 24, 1.948f, { 0.020f, 0.208f, 0.149f } },
		{ 32, 3.676f, { 0.010f, 0.220f, 0.113f } },
		{ 40, 4.801f, { 0.118f, 0.195f, 0.115f } },
		{ 48, 3.690f, { 0.288f, 0.155f, 0.117f } },
		{ 56, 1.565f, { 0.421f, 0.124f, 0.119f } },
		{ 60, 1.145f, { 0.442f, 0.119f, 0.119f } },
	};

	for (size_t i = 0; i < ufbxt_arraycount(refs); i++) {
		const ufbxt_anim_light_ref *ref = &refs[i];

		double time = ref->frame * (1.0/24.0);
		ufbx_scene *state = ufbx_evaluate_scene(scene, scene->anim, time, NULL, NULL);
		ufbxt_assert(state);

		ufbxt_check_scene(state);

		ufbx_node *light_node = ufbx_find_node(state, "pointLight1");
		ufbxt_assert(light_node);
		ufbx_light *light = light_node->light;
		ufbxt_assert(light);

		ufbxt_assert_close_real(err, light->intensity, ref->intensity);
		ufbxt_assert_close_vec3(err, light->color, ref->color);

		ufbx_free_scene(state);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "pointLight1");
		ufbxt_assert(node && node->light);
		uint32_t element_id = node->light->element.element_id;

		ufbx_prop_override_desc overrides[] = {
			{ element_id, { "Intensity", SIZE_MAX }, { (ufbx_real)10.0 } },
			{ element_id, { "Color", SIZE_MAX }, { (ufbx_real)0.3, (ufbx_real)0.6, (ufbx_real)0.9 } },
			{ element_id, { "|NewProp", SIZE_MAX }, { 10, 20, 30 }, { "Test", SIZE_MAX } },
			{ element_id, { "IntProp", SIZE_MAX }, { 0, 0, 0 }, { "", SIZE_MAX }, 15 },
		};

		uint32_t node_id = node->typed_id;
		ufbx_transform_override transform_overrides[] = {
			{ node_id, { { 1.0f, 2.0f, 3.0f }, { 1.0f, 0.0f, 0.0f, 0.0f }, { 4.0f, 5.0f, 6.0f } } },
		};

		uint32_t layers[32];
		size_t num_layers = scene->anim->layers.count;
		for (size_t i = 0; i < num_layers; i++) {
			layers[i] = scene->anim->layers.data[i]->typed_id;
		}

		ufbx_anim_opts opts = { 0 };
		opts.layer_ids.data = layers;
		opts.layer_ids.count = num_layers;
		opts.prop_overrides.data = overrides;
		opts.prop_overrides.count = ufbxt_arraycount(overrides);
		opts.transform_overrides.data = transform_overrides;
		opts.transform_overrides.count = ufbxt_arraycount(transform_overrides);

		ufbx_error error;
		ufbx_anim *anim = ufbx_create_anim(scene, &opts, &error);
		if (!anim) {
			ufbxt_log_error(&error);
		}
		ufbxt_assert(anim);

		ufbxt_check_anim(scene, anim);

		ufbx_scene *state = ufbx_evaluate_scene(scene, anim, 1.0f, NULL, NULL);
		ufbxt_assert(state);

		ufbxt_check_scene(state);

		ufbx_node *light_node = ufbx_find_node(state, "pointLight1");
		ufbxt_assert(light_node);

		ufbxt_assert_close_vec3(err, light_node->local_transform.translation, transform_overrides[0].transform.translation);
		ufbxt_assert_close_quat(err, light_node->local_transform.rotation, transform_overrides[0].transform.rotation);
		ufbxt_assert_close_vec3(err, light_node->local_transform.scale, transform_overrides[0].transform.scale);

		ufbx_light *light = light_node->light;
		ufbxt_assert(light);

		ufbx_vec3 ref_color = { (ufbx_real)0.3, (ufbx_real)0.6, (ufbx_real)0.9 };
		ufbxt_assert_close_real(err, light->intensity, 0.1f);
		ufbxt_assert_close_vec3(err, light->color, ref_color);

		{
			ufbx_vec3 ref_new = { 10, 20, 30 };
			ufbx_prop *new_prop = ufbx_find_prop(&light->props, "|NewProp");
			ufbxt_assert(new_prop);
			ufbxt_assert((new_prop->flags & UFBX_PROP_FLAG_OVERRIDDEN) != 0);
			ufbxt_assert(!strcmp(new_prop->value_str.data, "Test"));
			ufbxt_assert(new_prop->value_int == 10);
			ufbxt_assert_close_vec3(err, new_prop->value_vec3, ref_new);

			ufbx_prop *int_prop = ufbx_find_prop(&light->props, "IntProp");
			ufbxt_assert(int_prop);
			ufbxt_assert((int_prop->flags & UFBX_PROP_FLAG_OVERRIDDEN) != 0);
			ufbxt_assert_close_real(err, int_prop->value_real, 15.0f);
			ufbxt_assert(int_prop->value_int == 15);
		}

		{
			ufbx_element *original_light = &node->light->element;

			ufbx_prop color = ufbx_evaluate_prop(anim, original_light, "Color", 1.0);
			ufbxt_assert((color.flags & UFBX_PROP_FLAG_OVERRIDDEN) != 0);
			ufbxt_assert_close_vec3(err, color.value_vec3, ref_color);

			ufbx_prop intensity = ufbx_evaluate_prop(anim, original_light, "Intensity", 1.0);
			ufbxt_assert((intensity.flags & UFBX_PROP_FLAG_OVERRIDDEN) != 0);
			ufbxt_assert_close_real(err, intensity.value_real, 10.0f);

			ufbx_vec3 ref_new = { 10, 20, 30 };
			ufbx_prop new_prop = ufbx_evaluate_prop(anim, original_light, "|NewProp", 1.0);
			ufbxt_assert((new_prop.flags & UFBX_PROP_FLAG_OVERRIDDEN) != 0);
			ufbxt_assert(!strcmp(new_prop.value_str.data, "Test"));
			ufbxt_assert(new_prop.value_int == 10);
			ufbxt_assert_close_vec3(err, new_prop.value_vec3, ref_new);

			ufbx_prop int_prop = ufbx_evaluate_prop(anim, original_light, "IntProp", 1.0);
			ufbxt_assert((int_prop.flags & UFBX_PROP_FLAG_OVERRIDDEN) != 0);
			ufbxt_assert_close_real(err, int_prop.value_real, 15.0f);
			ufbxt_assert(int_prop.value_int == 15);
		}

		ufbx_free_scene(state);

		ufbx_free_anim(anim);
	}

	{
		ufbx_anim_layer *layer = scene->anim_layers.data[0];
		ufbx_node *node = ufbx_find_node(scene, "pointLight1");
		ufbxt_assert(node && node->light);
		ufbx_light *light = node->light;

		{
			ufbx_anim_prop_list props = ufbx_find_anim_props(layer, &node->element);
			ufbxt_assert(props.count == 3);
			ufbxt_assert(!strcmp(props.data[0].prop_name.data, "Lcl Rotation"));
			ufbxt_assert(!strcmp(props.data[1].prop_name.data, "Lcl Scaling"));
			ufbxt_assert(!strcmp(props.data[2].prop_name.data, "Lcl Translation"));

			ufbx_anim_prop *prop;
			prop = ufbx_find_anim_prop(layer, &node->element, "Lcl Rotation");
			ufbxt_assert(prop && !strcmp(prop->prop_name.data, "Lcl Rotation"));
			prop = ufbx_find_anim_prop(layer, &node->element, "Lcl Scaling");
			ufbxt_assert(prop && !strcmp(prop->prop_name.data, "Lcl Scaling"));
			prop = ufbx_find_anim_prop(layer, &node->element, "Lcl Translation");
			ufbxt_assert(prop && !strcmp(prop->prop_name.data, "Lcl Translation"));
		}

		{
			ufbx_anim_prop_list props = ufbx_find_anim_props(layer, &light->element);
			ufbxt_assert(props.count == 2);
			ufbxt_assert(!strcmp(props.data[0].prop_name.data, "Color"));
			ufbxt_assert(!strcmp(props.data[1].prop_name.data, "Intensity"));

			ufbx_anim_prop *prop;
			prop = ufbx_find_anim_prop(layer, &light->element, "Color");
			ufbxt_assert(prop && !strcmp(prop->prop_name.data, "Color"));
			prop = ufbx_find_anim_prop(layer, &light->element, "Intensity");
			ufbxt_assert(prop && !strcmp(prop->prop_name.data, "Intensity"));

			prop = ufbx_find_anim_prop(layer, &light->element, "Nonexistent");
			ufbxt_assert(prop == NULL);
		}

		{
			ufbx_anim_prop_list props = ufbx_find_anim_props(layer, &layer->element);
			ufbxt_assert(props.count == 0);

			ufbx_anim_prop *prop = ufbx_find_anim_prop(layer, &layer->element, "Weight");
			ufbxt_assert(prop == NULL);
		}
	}
}
#endif

UFBXT_FILE_TEST_ALT(create_anim_alloc_fail, maya_anim_light_7500_binary)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pointLight1");
	ufbxt_assert(node && node->light);
	uint32_t element_id = node->light->element.element_id;

	ufbx_prop_override_desc overrides[] = {
		{ element_id, { "Intensity", SIZE_MAX }, { (ufbx_real)10.0 } },
		{ element_id, { "Color", SIZE_MAX }, { (ufbx_real)0.3, (ufbx_real)0.6, (ufbx_real)0.9 } },
		{ element_id, { "|NewProp", SIZE_MAX }, { 10, 20, 30 }, { "Test", SIZE_MAX } },
		{ element_id, { "IntProp", SIZE_MAX }, { 0, 0, 0 }, { "", SIZE_MAX }, 15 },
	};

	uint32_t node_id = node->typed_id;
	ufbx_transform_override transform_overrides[] = {
		{ node_id, { { 1.0f, 2.0f, 3.0f }, { 1.0f, 0.0f, 0.0f, 0.0f }, { 4.0f, 5.0f, 6.0f } } },
	};

	uint32_t layers[32];
	size_t num_layers = scene->anim->layers.count;
	for (size_t i = 0; i < num_layers; i++) {
		layers[i] = scene->anim->layers.data[i]->typed_id;
	}

	ufbx_anim_opts opts = { 0 };
	opts.layer_ids.data = layers;
	opts.layer_ids.count = num_layers;
	opts.prop_overrides.data = overrides;
	opts.prop_overrides.count = ufbxt_arraycount(overrides);
	opts.transform_overrides.data = transform_overrides;
	opts.transform_overrides.count = ufbxt_arraycount(transform_overrides);

	for (size_t max_result = 1; max_result < 10000; max_result++) {
		opts.result_allocator.huge_threshold = 1;
		opts.result_allocator.allocation_limit = max_result;

		ufbxt_hintf("Result limit: %zu", max_result);

		ufbx_error error;
		ufbx_anim *anim = ufbx_create_anim(scene, &opts, &error);
		if (anim) {
			ufbxt_logf(".. Tested up to %zu result allocations", max_result);
			ufbx_free_anim(anim);
			break;
		}
		ufbxt_assert(error.type == UFBX_ERROR_ALLOCATION_LIMIT);
	}
}
#endif

UFBXT_FILE_TEST(maya_transform_animation)
#if UFBXT_IMPL
{
	static const ufbxt_anim_transform_ref refs[] = {
		{  1, {  0.000f,  0.000f,  0.000f }, {   0.000f,   0.000f,   0.000f }, { 1.000f, 1.000f, 1.000f } },
		{  5, {  0.226f,  0.452f,  0.677f }, {   2.258f,   4.515f,   6.773f }, { 1.023f, 1.045f, 1.068f } },
		{ 14, {  1.000f,  2.000f,  3.000f }, {  10.000f,  20.000f,  30.000f }, { 1.100f, 1.200f, 1.300f } },
		{ 20, { -0.296f, -0.592f, -0.888f }, {  -2.960f,  -5.920f,  -8.880f }, { 0.970f, 0.941f, 0.911f } },
		{ 24, { -1.000f, -2.000f, -3.000f }, { -10.000f, -20.000f, -30.000f }, { 0.900f, 0.800f, 0.700f } },
	};

	ufbx_node *node = ufbx_find_node(scene, "pCube1");

	for (size_t i = 0; i < ufbxt_arraycount(refs); i++) {
		const ufbxt_anim_transform_ref *ref = &refs[i];
		double time = ref->frame * (1.0/24.0);

		ufbx_scene *state = ufbx_evaluate_scene(scene, scene->anim, time, NULL, NULL);
		ufbxt_assert(state);
		ufbxt_check_scene(state);

		ufbx_transform t1 = state->nodes.data[node->element.typed_id]->local_transform;
		ufbx_transform t2 = ufbx_evaluate_transform(scene->anim, node, time);

		ufbx_vec3 t1_euler = ufbx_quat_to_euler(t1.rotation, UFBX_ROTATION_ORDER_XYZ);
		ufbx_vec3 t2_euler = ufbx_quat_to_euler(t2.rotation, UFBX_ROTATION_ORDER_XYZ);

		ufbxt_assert_close_vec3(err, ref->translation, t1.translation);
		ufbxt_assert_close_vec3(err, ref->translation, t2.translation);
		ufbxt_assert_close_vec3(err, ref->rotation_euler, t1_euler);
		ufbxt_assert_close_vec3(err, ref->rotation_euler, t2_euler);
		ufbxt_assert_close_vec3(err, ref->scale, t1.scale);
		ufbxt_assert_close_vec3(err, ref->scale, t2.scale);

		ufbx_free_scene(state);
	}

	{
		uint32_t element_id = node->element.element_id;
		ufbxt_anim_transform_ref ref = refs[2];
		ref.translation.x -= 0.1f;
		ref.translation.y -= 0.2f;
		ref.translation.z -= 0.3f;
		ref.scale.x = 2.0f;
		ref.scale.y = 3.0f;
		ref.scale.z = 4.0f;

		ufbx_prop_override_desc overrides[] = {
			{ element_id, { "Color", SIZE_MAX }, { (ufbx_real)0.3, (ufbx_real)0.6, (ufbx_real)0.9 } },
			{ element_id, { "|NewProp", SIZE_MAX }, { 10, 20, 30 }, { "Test", SIZE_MAX }, },
			{ element_id, { "Lcl Scaling", SIZE_MAX }, { 2.0f, 3.0f, 4.0f } },
			{ element_id, { "RotationOffset", SIZE_MAX }, { -0.1f, -0.2f, -0.3f } },
		};

		uint32_t layers[32];
		size_t num_layers = scene->anim->layers.count;
		for (size_t i = 0; i < num_layers; i++) {
			layers[i] = scene->anim->layers.data[i]->typed_id;
		}

		ufbx_anim_opts opts = { 0 };
		opts.layer_ids.data = layers;
		opts.layer_ids.count = num_layers;
		opts.prop_overrides.data = overrides;
		opts.prop_overrides.count = ufbxt_arraycount(overrides);

		ufbx_error error;
		ufbx_anim *anim = ufbx_create_anim(scene, &opts, NULL);
		if (!anim) {
			ufbxt_log_error(&error);
		}
		ufbxt_assert(anim);
		ufbxt_check_anim(scene, anim);

		double time = 14.0/24.0;
		ufbx_scene *state = ufbx_evaluate_scene(scene, anim, time, NULL, NULL);
		ufbxt_assert(state);
		ufbxt_check_scene(state);

		ufbx_transform t1 = state->nodes.data[node->element.typed_id]->local_transform;
		ufbx_transform t2 = ufbx_evaluate_transform(anim, node, time);

		ufbx_vec3 t1_euler = ufbx_quat_to_euler(t1.rotation, UFBX_ROTATION_ORDER_XYZ);
		ufbx_vec3 t2_euler = ufbx_quat_to_euler(t2.rotation, UFBX_ROTATION_ORDER_XYZ);

		ufbxt_assert_close_vec3(err, ref.translation, t1.translation);
		ufbxt_assert_close_vec3(err, ref.translation, t2.translation);
		ufbxt_assert_close_vec3(err, ref.rotation_euler, t1_euler);
		ufbxt_assert_close_vec3(err, ref.rotation_euler, t2_euler);
		ufbxt_assert_close_vec3(err, ref.scale, t1.scale);
		ufbxt_assert_close_vec3(err, ref.scale, t2.scale);

		ufbx_free_scene(state);
		ufbx_free_anim(anim);
	}
}
#endif


UFBXT_FILE_TEST(maya_anim_layers)
#if UFBXT_IMPL
{
	ufbx_anim_layer *x = (ufbx_anim_layer*)ufbx_find_element(scene, UFBX_ELEMENT_ANIM_LAYER, "X");
	ufbx_anim_layer *y = (ufbx_anim_layer*)ufbx_find_element(scene, UFBX_ELEMENT_ANIM_LAYER, "Y");
	ufbxt_assert(x && y);
	ufbxt_assert(y->compose_rotation == false);
	ufbxt_assert(y->compose_scale == false);
}
#endif

UFBXT_FILE_TEST(maya_anim_layers_acc)
#if UFBXT_IMPL
{
	ufbx_anim_layer *x = (ufbx_anim_layer*)ufbx_find_element(scene, UFBX_ELEMENT_ANIM_LAYER, "X");
	ufbx_anim_layer *y = (ufbx_anim_layer*)ufbx_find_element(scene, UFBX_ELEMENT_ANIM_LAYER, "Y");
	ufbxt_assert(x && y);
	ufbxt_assert(y->compose_rotation == true);
	ufbxt_assert(y->compose_scale == true);
}
#endif

UFBXT_FILE_TEST(maya_anim_layers_over)
#if UFBXT_IMPL
{
	ufbx_anim_layer *x = (ufbx_anim_layer*)ufbx_find_element(scene, UFBX_ELEMENT_ANIM_LAYER, "X");
	ufbx_anim_layer *y = (ufbx_anim_layer*)ufbx_find_element(scene, UFBX_ELEMENT_ANIM_LAYER, "Y");
	ufbxt_assert(x && y);
	ufbxt_assert(y->compose_rotation == false);
	ufbxt_assert(y->compose_scale == false);
}
#endif

UFBXT_FILE_TEST(maya_anim_layers_over_acc)
#if UFBXT_IMPL
{
	ufbx_anim_layer *x = (ufbx_anim_layer*)ufbx_find_element(scene, UFBX_ELEMENT_ANIM_LAYER, "X");
	ufbx_anim_layer *y = (ufbx_anim_layer*)ufbx_find_element(scene, UFBX_ELEMENT_ANIM_LAYER, "Y");
	ufbxt_assert(x && y);
	ufbxt_assert(y->compose_rotation == true);
	ufbxt_assert(y->compose_scale == true);
}
#endif

#if UFBXT_IMPL
typedef struct {
	double time;
	bool visible;
} ufbxt_visibility_ref;
#endif

UFBXT_FILE_TEST(maya_cube_blinky)
#if UFBXT_IMPL
{
	ufbxt_visibility_ref refs[] = {
		{ 1.0, false },
		{ 9.5, false },
		{ 10.5, true },
		{ 11.5, false },
		{ 15.0, false },
		{ 19.5, false },
		{ 20.5, false },
		{ 25.0, false },
		{ 29.5, false },
		{ 30.5, true },
		{ 40.0, true },
		{ 50.0, true },
	};

	for (size_t i = 0; i < ufbxt_arraycount(refs); i++) {
		double time = refs[i].time / 24.0;
		ufbx_scene *state = ufbx_evaluate_scene(scene, scene->anim, time, NULL, NULL);
		ufbxt_assert(state);

		ufbxt_check_scene(state);

		ufbx_node *node = ufbx_find_node(state, "pCube1");
		ufbxt_assert(node);
		ufbxt_assert(node->visible == refs[i].visible);

		ufbx_free_scene(state);
	}
}
#endif

#if UFBXT_IMPL
typedef struct {
	double time;
	double value;
} ufbxt_anim_ref;
#endif

UFBXT_FILE_TEST(maya_anim_interpolation)
#if UFBXT_IMPL
{
	ufbxt_anim_ref anim_ref[] = {
		// Cubic
		{ 0.0 / 30.0, 0.0 },
		{ 1.0 / 30.0, -0.855245 },
		{ 2.0 / 30.0, -1.13344 },
		{ 3.0 / 30.0, -1.17802 },
		{ 4.0 / 30.0, -1.10882 },
		{ 5.0 / 30.0, -0.991537 },
		{ 6.0 / 30.0, -0.875223 },
		{ 7.0 / 30.0, -0.808958 },
		{ 8.0 / 30.0, -0.858419 },
		{ 9.0 / 30.0, -1.14293 },
		// Linear
		{ 10.0 / 30.0, -2.0 },
		// Constant previous
		{ 20.0 / 30.0, -4.0 },
		{ 25.0 / 30.0 - 0.001, -4.0 },
		// Constant next
		{ 25.0 / 30.0, -6.0 },
		{ 25.0 / 30.0 + 0.001, -8.0 },
		// Constant previous
		{ 30.0 / 30.0, -8.0 },
		{ 35.0 / 30.0 - 0.001, -8.0 },
		// Linear
		{ 35.0 / 30.0, -10.0 },
		// Constant next
		{ 40.0 / 30.0, -12.0 },
		{ 40.0 / 30.0 + 0.001, -14.0 },
		// Constant previous
		{ 45.0 / 30.0, -14.0 },
		{ 50.0 / 30.0 - 0.001, -14.0 },
		// Constant next
		{ 50.0 / 30.0, -16.0 },
		{ 50.0 / 30.0 + 0.001, -14.0 },
		// Final
		{ 55.0 / 30.0, -14.0, },
	};

	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	for (size_t i = 0; i < ufbxt_arraycount(anim_ref); i++) {
		ufbxt_anim_ref ref = anim_ref[i];
		ufbxt_hintf("%zu: %f (frame %.2f)", i, ref.time, ref.time * 30.0f);

		ufbx_prop p = ufbx_evaluate_prop(scene->anim, &node->element, "Lcl Translation", ref.time);
		ufbxt_assert_close_real(err, p.value_vec3.x, (ufbx_real)ref.value);
	}
}
#endif

#if UFBXT_IMPL
typedef struct {
	int64_t frame;
	ufbx_real value;
} ufbxt_frame_ref;
#endif

#if UFBXT_IMPL

// Clang x86 UBSAN emits an undefined reference to __mulodi4() for 64-bit
// multiplication and this is the only instance where we need it at the
// moment so just use a dumb implementation if necessary.
#if defined(__clang__) && defined(__i386__) && (defined(UFBX_UBSAN) || defined(__SANITIZE_UNDEFINED__))
static int64_t ufbxt_mul_i64(int64_t a, int64_t b)
{
	bool negative = false;
	if (a < 0) {
		negative = !negative;
		a = -a;
	}
	if (b < 0) {
		negative = !negative;
		b = -b;
	}

	int64_t result = 0;
	for (int32_t i = 0; i < 64; i++) {
		result += ((a >> i) & 1) ? (b << i) : 0;
	}
	return negative ? -result : result;
}
#else
static int64_t ufbxt_mul_i64(int64_t a, int64_t b)
{
	return a * b;
}
#endif

#endif

UFBXT_FILE_TEST(maya_long_keyframes)
#if UFBXT_IMPL
{
	ufbxt_frame_ref anim_ref[] = {
		{ -5000000, -50.0f },
		{ -2925270, -29.0f },
		{ -2925269, -28.0f },
		{ -2925268, -27.0f },
		{ -2925267, -26.0f },
		{ -2925266, -25.0f },
		{ -2925265, -24.0f },
		{ -2925264, -23.0f },
		{ -2925263, -22.0f },
		{ -2925262, -21.0f },
		{ -2000000, -20.0f },
		{ -599999, -5.9f },
		{ -500000, -5.0f },
		{ -49999, -4.9f },
		{ -40000, -4.0f },
		{ -3999, -3.9f },
		{ -3000, -3.0f },
		{ -299, -2.9f },
		{ -200, -2.0f },
		{ -10, -1.0f },
		{ 0, 0.0f },
		{ 10, 1.0f },
		{ 200, 2.0f },
		{ 299, 2.9f },
		{ 3000, 3.0f },
		{ 3999, 3.9f },
		{ 40000, 4.0f },
		{ 49999, 4.9f },
		{ 500000, 5.0f },
		{ 599999, 5.9f },
		{ 2000000, 20.0f },
		{ 2925262, 21.0f },
		{ 2925263, 22.0f },
		{ 2925264, 23.0f },
		{ 2925265, 24.0f },
		{ 2925266, 25.0f },
		{ 2925267, 26.0f },
		{ 2925268, 27.0f },
		{ 2925269, 28.0f },
		{ 2925270, 29.0f },
		{ 5000000, 50.0f },
	};

	ufbxt_assert(scene->metadata.ktime_second == 46186158000);

	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbxt_assert(scene->anim_layers.count > 0);

	ufbx_anim_prop *aprop = ufbx_find_anim_prop(scene->anim_layers.data[0], &node->element, "Lcl Translation");
	ufbxt_assert(aprop);

	ufbx_anim_curve *curve = aprop->anim_value->curves[0];
	ufbxt_assert(curve);

	for (size_t i = 0; i < ufbxt_arraycount(anim_ref); i++) {
		ufbxt_frame_ref ref = anim_ref[i];
		ufbxt_hintf("%zu: (frame %lld)", i, (long long)ref.frame);
		ufbxt_assert(i < curve->keyframes.count);
		ufbx_keyframe key = curve->keyframes.data[i];

		bool tick_accurate = true;
		if (llabs(ref.frame) > 2925270) {
			tick_accurate = false;
		}

		int64_t ref_tick = ufbxt_mul_i64(ref.frame, INT64_C(46186158000) / 30);
		int64_t tick = (int64_t)round(key.time * 46186158000.0);
		if (tick_accurate) {
			ufbxt_assert(ref_tick == tick);
		} else {
			ufbxt_assert_close_real(err, key.time, (double)ref.frame / 30.0);
		}
		ufbxt_assert_close_real(err, key.value, ref.value);
	}
}
#endif

UFBXT_FILE_TEST_ALT(anim_override_utf8, blender_279_default)
#if UFBXT_IMPL
{
	ufbx_node *cube = ufbx_find_node(scene, "Cube");
	ufbxt_assert(cube);
	uint32_t cube_id = cube->element_id;

	ufbx_string good_strings[] = {
		{ NULL, 0 },
		{ "", 0 },
		{ "", SIZE_MAX },
		{ "a", 1 },
		{ "a", SIZE_MAX },
	};

	ufbx_string bad_strings[] = {
		{ "\0", 1 },
		{ "\xff", 1 },
		{ "\xff", SIZE_MAX },
		{ "a\xff", 2 },
		{ "a\xff", SIZE_MAX },
	};

	for (size_t i = 0; i < ufbxt_arraycount(good_strings); i++) {
		for (int do_value = 0; do_value <= 1; do_value++) {
			ufbxt_hintf("i=%zu, do_value=%d", i, do_value);

			ufbx_prop_override_desc over = { cube_id };

			over.prop_name.data = "prop";
			over.prop_name.length = 4;

			if (do_value) {
				over.value_str = good_strings[i];
			} else {
				over.prop_name = good_strings[i];
			}

			ufbx_anim_opts opts = { 0 };
			opts.prop_overrides.data = &over;
			opts.prop_overrides.count = 1;

			ufbx_error error;
			ufbx_anim *anim = ufbx_create_anim(scene, &opts, &error);
			if (!anim) {
				ufbxt_log_error(&error);
			}
			ufbxt_assert(anim);
			ufbx_free_anim(anim);
		}
	}

	for (size_t i = 0; i < ufbxt_arraycount(bad_strings); i++) {
		for (int do_value = 0; do_value <= 1; do_value++) {
			ufbxt_hintf("i=%zu, do_value=%d", i, do_value);

			ufbx_prop_override_desc over = { cube_id };

			over.prop_name.data = "prop";
			over.prop_name.length = 4;

			if (do_value) {
				over.value_str = bad_strings[i];
			} else {
				over.prop_name = bad_strings[i];
			}

			ufbx_anim_opts opts = { 0 };
			opts.prop_overrides.data = &over;
			opts.prop_overrides.count = 1;

			ufbx_error error;
			ufbx_anim *anim = ufbx_create_anim(scene, &opts, &error);
			ufbxt_assert(!anim);
			ufbxt_assert(error.type == UFBX_ERROR_INVALID_UTF8);
		}
	}
}
#endif

#if UFBXT_IMPL
typedef struct {
	ufbx_vec3 translation;
	ufbx_vec3 rotation_euler;
	ufbx_vec3 scale;
} ufbxt_ref_transform;

static void ufbxt_check_transform(ufbxt_diff_error *err, const char *name, ufbx_transform transform, ufbxt_ref_transform ref)
{
	ufbx_vec3 rotation_euler = ufbx_quat_to_euler(transform.rotation, UFBX_ROTATION_ORDER_XYZ);
	ufbxt_hintf("%s { { %.2f, %.2f, %.2f }, { %.2f, %.2f, %.2f }, { %.2f, %.2f, %.2f } }", name,
		transform.translation.x, transform.translation.y, transform.translation.z,
		rotation_euler.x, rotation_euler.y, rotation_euler.z,
		transform.scale.x, transform.scale.y, transform.scale.z);

	ufbxt_assert_close_vec3(err, transform.translation, ref.translation);
	ufbxt_assert_close_vec3(err, rotation_euler, ref.rotation_euler);
	ufbxt_assert_close_vec3(err, transform.scale, ref.scale);

	ufbxt_hintf("");
}

static const ufbxt_ref_transform ufbxt_ref_transform_identity = {
	{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f },
};
#endif

UFBXT_FILE_TEST_ALT(anim_multi_override, blender_293_instancing)
#if UFBXT_IMPL
{
	static const char *node_names[] = {
		"Suzanne",
		"Suzanne.001",
		"Suzanne.002",
		"Suzanne.003",
		"Suzanne.004",
		"Suzanne.005",
		"Suzanne.006",
		"Suzanne.007",
	};

	size_t num_nodes = ufbxt_arraycount(node_names);
	ufbx_prop_override_desc overrides[ufbxt_arraycount(node_names) * 3];
	memset(&overrides, 0, sizeof(overrides));

	for (size_t i = 0; i < num_nodes; i++) {
		ufbx_node *node = ufbx_find_node(scene, node_names[i]);
		ufbxt_assert(node);

		overrides[i*3 + 0].element_id = node->element_id;
		overrides[i*3 + 0].prop_name.data = "Lcl Translation";
		overrides[i*3 + 0].prop_name.length = SIZE_MAX;
		overrides[i*3 + 0].value.x = (ufbx_real)i;
		overrides[i*3 + 0].value.y = 0.0f;
		overrides[i*3 + 0].value.z = 0.0f;

		overrides[i*3 + 1].element_id = node->element_id;
		overrides[i*3 + 1].prop_name.data = "Lcl Rotation";
		overrides[i*3 + 1].prop_name.length = SIZE_MAX;
		overrides[i*3 + 1].value.x = 0.0f;
		overrides[i*3 + 1].value.y = 10.0f * (ufbx_real)i;
		overrides[i*3 + 1].value.z = 0.0f;

		overrides[i*3 + 2].element_id = node->element_id;
		overrides[i*3 + 2].prop_name.data = "Lcl Scaling";
		overrides[i*3 + 2].prop_name.length = SIZE_MAX;
		overrides[i*3 + 2].value.x = 1.0f;
		overrides[i*3 + 2].value.y = 1.0f;
		overrides[i*3 + 2].value.z = 1.0f + 0.1f * (ufbx_real)i;
	}

	ufbx_anim_opts opts = { 0 };
	opts.prop_overrides.data = overrides;
	opts.prop_overrides.count = ufbxt_arraycount(overrides);

	ufbx_error error;
	ufbx_anim *anim = ufbx_create_anim(scene, &opts, &error);
	if (!anim) ufbxt_log_error(&error);
	ufbxt_assert(anim);
	ufbxt_check_anim(scene, anim);

	ufbx_scene *state = ufbx_evaluate_scene(scene, anim, 0.0f, NULL, &error);
	if (!state) ufbxt_log_error(&error);
	ufbxt_assert(state);
	ufbxt_check_scene(state);

	for (size_t i = 0; i < num_nodes; i++) {
		ufbx_node *scene_node = ufbx_find_node(scene, node_names[i]);
		ufbx_node *state_node = ufbx_find_node(state, node_names[i]);

		ufbx_transform scene_transform = ufbx_evaluate_transform(anim, scene_node, 0.0f);
		ufbx_transform state_transform = state_node->local_transform;

		ufbxt_ref_transform ref_transform = ufbxt_ref_transform_identity;
		ref_transform.translation.x = (ufbx_real)i;
		ref_transform.rotation_euler.y = 10.0f * (ufbx_real)i;
		ref_transform.scale.z = 1.0f + 0.1f * (ufbx_real)i;

		ufbxt_check_transform(err, "scene_transform", scene_transform, ref_transform);
		ufbxt_check_transform(err, "state_transform", state_transform, ref_transform);
	}

	ufbx_free_scene(state);
	ufbx_free_anim(anim);
}
#endif

UFBXT_FILE_TEST_ALT(anim_override_duplicate, blender_293_instancing)
#if UFBXT_IMPL
{
	ufbx_prop_override_desc overrides[] = {
		{ 1, { "PropA", SIZE_MAX }, { 1.0f } },
		{ 1, { "PropB", SIZE_MAX }, { 1.0f } },
		{ 1, { "PropC", SIZE_MAX }, { 1.0f } },
		{ 2, { "PropA", SIZE_MAX }, { 1.0f } },
		{ 2, { "PropB", SIZE_MAX }, { 1.0f } },
		{ 2, { "PropB", SIZE_MAX }, { 1.0f } },
		{ 2, { "PropC", SIZE_MAX }, { 1.0f } },
	};

	ufbx_anim_opts opts = { 0 };
	opts.prop_overrides.data = overrides;
	opts.prop_overrides.count = ufbxt_arraycount(overrides);

	ufbx_error error;
	ufbx_anim *anim = ufbx_create_anim(scene, &opts, &error);
	ufbxt_assert(!anim);
	ufbxt_assert(error.type == UFBX_ERROR_DUPLICATE_OVERRIDE);
	ufbxt_assert(!strcmp(error.info, "element 2 prop \"PropB\""));
}
#endif

UFBXT_FILE_TEST(synthetic_anim_stack_no_props)
#if UFBXT_IMPL
{
	{
		ufbx_anim_stack *stack = ufbx_find_anim_stack(scene, "X");
		ufbxt_assert(stack);
		ufbxt_assert_close_double(err, stack->time_begin, 1.0 / 24.0);
		ufbxt_assert_close_double(err, stack->time_end, 12.0 / 24.0);

		ufbxt_assert(ufbx_find_int(&stack->props, "LocalStart", 0) == INT64_C(1924423250));
		ufbxt_assert(ufbx_find_int(&stack->props, "LocalStop", 0) == INT64_C(23093079000));
		ufbxt_assert(ufbx_find_int(&stack->props, "ReferenceStart", 0) == INT64_C(1924423250));
		ufbxt_assert(ufbx_find_int(&stack->props, "ReferenceStop", 0) == INT64_C(23093079000));
	}

	{
		ufbx_anim_stack *stack = ufbx_find_anim_stack(scene, "Y");
		ufbxt_assert(stack);
		ufbxt_assert_close_double(err, stack->time_begin, 13.0 / 24.0);
		ufbxt_assert_close_double(err, stack->time_end, 24.0 / 24.0);

		ufbxt_assert(ufbx_find_int(&stack->props, "LocalStart", 0) == INT64_C(25017502250));
		ufbxt_assert(ufbx_find_int(&stack->props, "LocalStop", 0) == INT64_C(46186158000));
		ufbxt_assert(ufbx_find_int(&stack->props, "ReferenceStart", 0) == INT64_C(25017502250));
		ufbxt_assert(ufbx_find_int(&stack->props, "ReferenceStop", 0) == INT64_C(46186158000));
	}

	{
		ufbx_anim_stack *stack = ufbx_find_anim_stack(scene, "Z");
		ufbxt_assert(stack);
		ufbxt_assert_close_double(err, stack->time_begin, 25.0 / 24.0);
		ufbxt_assert_close_double(err, stack->time_end, 36.0 / 24.0);

		ufbxt_assert(ufbx_find_int(&stack->props, "LocalStart", 0) == INT64_C(48110581250));
		ufbxt_assert(ufbx_find_int(&stack->props, "LocalStop", 0) == INT64_C(69279237000));
		ufbxt_assert(ufbx_find_int(&stack->props, "ReferenceStart", 0) == INT64_C(48110581250));
		ufbxt_assert(ufbx_find_int(&stack->props, "ReferenceStop", 0) == INT64_C(69279237000));
	}
}
#endif

UFBXT_FILE_TEST_ALT(anim_override_int64, maya_cube_7500_binary)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbx_prop *missing_prop = ufbx_find_prop(&node->props, "NewProperty");
	ufbxt_assert(missing_prop == NULL);

	ufbx_prop *existing_prop = ufbx_find_prop(&node->props, "DefaultAttributeIndex");
	ufbxt_assert(existing_prop);
	ufbxt_assert(existing_prop->type == UFBX_PROP_INTEGER);
	ufbxt_assert(existing_prop->flags & UFBX_PROP_FLAG_VALUE_INT);
	ufbxt_assert(existing_prop->value_int == 0);

	ufbx_prop_override_desc overrides[2] = { 0 };
	overrides[0].element_id = node->element_id;
	overrides[0].prop_name.data = "NewProperty";
	overrides[0].prop_name.length = SIZE_MAX;
	overrides[0].value_int = INT64_C(0x4000000000000001);
	overrides[1].element_id = node->element_id;
	overrides[1].prop_name.data = "DefaultAttributeIndex";
	overrides[1].prop_name.length = SIZE_MAX;
	overrides[1].value_int = INT64_C(0x5000000000000001);

	ufbx_anim_opts opts = { 0 };
	opts.prop_overrides.data = overrides;
	opts.prop_overrides.count = ufbxt_arraycount(overrides);

	ufbx_anim *anim = ufbx_create_anim(scene, &opts, NULL);
	ufbxt_assert(anim);

	ufbx_scene *state = ufbx_evaluate_scene(scene, anim, 0.0, NULL, NULL);
	ufbxt_assert(state);

	ufbx_node *node_state = state->nodes.data[node->typed_id];
	
	ufbx_prop *new_prop = ufbx_find_prop(&node_state->props, "NewProperty");
	ufbxt_assert(new_prop);
	ufbxt_assert(new_prop->flags & UFBX_PROP_FLAG_OVERRIDDEN);
	ufbxt_assert(new_prop->value_int == INT64_C(0x4000000000000001));
	ufbxt_assert_close_real(err, new_prop->value_real, (ufbx_real)INT64_C(0x4000000000000001));

	ufbx_prop *over_prop = ufbx_find_prop(&node_state->props, "DefaultAttributeIndex");
	ufbxt_assert(over_prop);
	ufbxt_assert(over_prop->flags & UFBX_PROP_FLAG_OVERRIDDEN);
	ufbxt_assert(over_prop->value_int == INT64_C(0x5000000000000001));
	ufbxt_assert_close_real(err, over_prop->value_real, (ufbx_real)INT64_C(0x5000000000000001));

	ufbx_free_scene(state);
	ufbx_free_anim(anim);
}
#endif

#if UFBXT_IMPL

static void ufbxt_diff_baked_vec3_imp(ufbxt_diff_error *err, const ufbx_baked_vec3 *refs, size_t num_refs, ufbx_baked_vec3_list list, const char *name)
{
	for (size_t i = 0; i < num_refs; i++) {
		ufbxt_hintf("%s i=%zu", name, i);
		ufbxt_assert(i < list.count);
		ufbx_baked_vec3 ref = refs[i];
		ufbx_baked_vec3 key = list.data[i];

		if (key.time != ref.time) ufbxt_logf("key.time=%f, ref.time=%f", key.time, ref.time);
		if (key.flags != ref.flags) ufbxt_logf("key.flags=0x%x, ref.flags=0x%x", key.flags, ref.flags);
		ufbxt_assert(key.time == ref.time);
		ufbxt_assert_close_vec3(err, key.value, ref.value);
		ufbxt_assert(key.flags == ref.flags);
	}
	ufbxt_assert(list.count == num_refs);
}

static void ufbxt_diff_baked_quat_imp(ufbxt_diff_error *err, const ufbx_baked_vec3 *refs, size_t num_refs, ufbx_baked_quat_list list, const char *name)
{
	for (size_t i = 0; i < num_refs; i++) {
		ufbxt_hintf("%s i=%zu", name, i);
		ufbxt_assert(i < list.count);
		ufbx_baked_vec3 ref = refs[i];
		ufbx_baked_quat key = list.data[i];

		ufbx_vec3 key_euler = ufbx_quat_to_euler(key.value, UFBX_ROTATION_ORDER_XYZ);

		if (key.time != ref.time) ufbxt_logf("key.time=%f, ref.time=%f", key.time, ref.time);
		ufbxt_assert(key.time == ref.time);
		ufbxt_assert_close_vec3(err, key_euler, ref.value);
		ufbxt_assert(key.flags == ref.flags);
	}
	ufbxt_assert(list.count == num_refs);
}

#define ufbxt_diff_baked_vec3(err, refs, list) ufbxt_diff_baked_vec3_imp((err), (refs), ufbxt_arraycount(refs), (list), #list)
#define ufbxt_diff_baked_quat(err, refs, list) ufbxt_diff_baked_quat_imp((err), (refs), ufbxt_arraycount(refs), (list), #list)

#endif

UFBXT_FILE_TEST(maya_anim_linear)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.step_handling = UFBX_BAKE_STEP_HANDLING_ADJACENT_DOUBLE;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0/24.0, { 0.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 2.0/24.0, { 0.5f, 2.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 8.0/24.0, { 2.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ nextafter(12.0/24.0, -INFINITY), { 2.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 12.0/24.0, { 2.0f, 0.0f, 2.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ nextafter(12.0/24.0, INFINITY), { 2.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 14.0/24.0, { 2.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_ALT(maya_anim_linear_default, maya_anim_linear)
#if UFBXT_IMPL
{
	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, NULL, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0/24.0, { 0.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 2.0/24.0, { 0.5f, 2.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 8.0/24.0, { 2.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 12.0/24.0 - 0.001, { 2.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 12.0/24.0, { 2.0f, 0.0f, 2.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 12.0/24.0 + 0.001, { 2.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 14.0/24.0, { 2.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbxt_assert(ufbx_evaluate_baked_vec3(node->translation_keys, nextafter(12.0/24.0, -INFINITY)).z == 0.0f);
	ufbxt_assert(ufbx_evaluate_baked_vec3(node->translation_keys, 12.0/24.0).z == 2.0f);
	ufbxt_assert(ufbx_evaluate_baked_vec3(node->translation_keys, nextafter(12.0/24.0, INFINITY)).z == 0.0f);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_ALT(maya_anim_linear_ignore, maya_anim_linear)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.step_handling = UFBX_BAKE_STEP_HANDLING_IGNORE;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0/24.0, { 0.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 2.0/24.0, { 0.5f, 2.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 8.0/24.0, { 2.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 12.0/24.0, { 2.0f, 0.0f, 2.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 14.0/24.0, { 2.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST(maya_huge_stepped_tangents)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.step_handling = UFBX_BAKE_STEP_HANDLING_ADJACENT_DOUBLE;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0, { 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ nextafter(10.0, -INFINITY), { 0.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 10.0, { 1.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ nextafter(10.0, +INFINITY), { 2.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 20.0, { 2.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ nextafter(100.0, -INFINITY), { 2.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 100.0, { 3.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ nextafter(100.0, +INFINITY), { 4.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 200.0, { 4.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ nextafter(1000.0, -INFINITY), { 4.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 1000.0, { 5.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ nextafter(1000.0, +INFINITY), { 6.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 2000.0, { 6.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ nextafter(10000.0, -INFINITY), { 6.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 10000.0, { 7.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ nextafter(10000.0, +INFINITY), { 8.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 20000.0, { 8.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ nextafter(100000.0, -INFINITY), { 8.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 100000.0, { 9.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ nextafter(100000.0, +INFINITY), { 10.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 200000.0, { 10.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbxt_assert(ufbx_evaluate_baked_vec3(node->translation_keys, nextafter(10.0, -INFINITY)).x == 0.0f);
	ufbxt_assert(ufbx_evaluate_baked_vec3(node->translation_keys, 10.0).x == 1.0f);
	ufbxt_assert(ufbx_evaluate_baked_vec3(node->translation_keys, nextafter(10.0, INFINITY)).x == 2.0f);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_ALT(maya_huge_stepped_tangents_identical, maya_huge_stepped_tangents)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.step_handling = UFBX_BAKE_STEP_HANDLING_IDENTICAL_TIME;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0, { 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 10.0, { 0.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 10.0, { 1.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0, { 2.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 20.0, { 2.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 100.0, { 2.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 100.0, { 3.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 100.0, { 4.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 200.0, { 4.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 1000.0, { 4.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 1000.0, { 5.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 1000.0, { 6.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 2000.0, { 6.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 10000.0, { 6.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 10000.0, { 7.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10000.0, { 8.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 20000.0, { 8.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 100000.0, { 8.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 100000.0, { 9.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 100000.0, { 10.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 200000.0, { 10.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbxt_assert(ufbx_evaluate_baked_vec3(node->translation_keys, nextafter(10.0, -INFINITY)).x == 0.0f);
	ufbxt_assert(ufbx_evaluate_baked_vec3(node->translation_keys, 10.0).x == 1.0f);
	ufbxt_assert(ufbx_evaluate_baked_vec3(node->translation_keys, nextafter(10.0, INFINITY)).x == 2.0f);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_ALT(maya_huge_stepped_tangents_default, maya_huge_stepped_tangents)
#if UFBXT_IMPL
{
	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, NULL, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	const double step = 0.001;
	const double epsilon = 1.0 + (double)FLT_EPSILON * 4.0;
	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0, { 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 10.0 - step, { 0.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 10.0, { 1.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0 + step, { 2.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 20.0, { 2.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 100.0 - step, { 2.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 100.0, { 3.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 100.0 + step, { 4.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 200.0, { 4.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 1000.0 - step, { 4.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 1000.0, { 5.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 1000.0 + step, { 6.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 2000.0, { 6.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 10000.0 / epsilon, { 6.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 10000.0, { 7.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10000.0 * epsilon, { 8.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 20000.0, { 8.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 100000.0 / epsilon, { 8.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 100000.0, { 9.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 100000.0 * epsilon, { 10.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 200000.0, { 10.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbxt_assert(ufbx_evaluate_baked_vec3(node->translation_keys, nextafter(10.0, -INFINITY)).x == 0.0f);
	ufbxt_assert(ufbx_evaluate_baked_vec3(node->translation_keys, 10.0).x == 1.0f);
	ufbxt_assert(ufbx_evaluate_baked_vec3(node->translation_keys, nextafter(10.0, INFINITY)).x == 2.0f);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_ALT(maya_huge_stepped_tangents_custom, maya_huge_stepped_tangents)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.step_handling = UFBX_BAKE_STEP_HANDLING_CUSTOM_DURATION;
	opts.step_custom_duration = 0.1;
	opts.step_custom_epsilon = 0.0005;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	const double step = 0.1;
	const double epsilon = 1.0 + 0.0005;
	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0, { 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 10.0 - step, { 0.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 10.0, { 1.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0 + step, { 2.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 20.0, { 2.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 100.0 - step, { 2.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 100.0, { 3.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 100.0 + step, { 4.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 200.0, { 4.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 1000.0 / epsilon, { 4.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 1000.0, { 5.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 1000.0 * epsilon, { 6.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 2000.0, { 6.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 10000.0 / epsilon, { 6.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 10000.0, { 7.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10000.0 * epsilon, { 8.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 20000.0, { 8.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 100000.0 / epsilon, { 8.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 100000.0, { 9.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 100000.0 * epsilon, { 10.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 200000.0, { 10.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_ALT(maya_huge_stepped_tangents_huge_custom, maya_huge_stepped_tangents)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.step_handling = UFBX_BAKE_STEP_HANDLING_CUSTOM_DURATION;
	opts.step_custom_duration = 100.0;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	const double step = 100.0;
	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0, { 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 10.0, { 1.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 20.0, { 2.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 100.0, { 3.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 200.0, { 4.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 1000.0 - step, { 4.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 1000.0, { 5.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 1000.0 + step, { 6.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 2000.0, { 6.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 10000.0 - step, { 6.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 10000.0, { 7.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10000.0 + step, { 8.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 20000.0, { 8.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 100000.0 - step, { 8.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 100000.0, { 9.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 100000.0 + step, { 10.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 200000.0, { 10.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_ALT(maya_huge_stepped_tangents_huge_resample, maya_huge_stepped_tangents)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.maximum_sample_rate = 100.0;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	const double step = 1.0 / 100.0;
	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0, { 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 10.0 - step, { 0.0f }, UFBX_BAKED_KEY_REDUCED },
		{ 10.0, { 1.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0 + step, { 2.0f }, UFBX_BAKED_KEY_REDUCED },
		{ 20.0, { 2.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 100.0 - step, { 2.0f }, UFBX_BAKED_KEY_REDUCED },
		{ 100.0, { 3.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 100.0 + step, { 4.0f }, UFBX_BAKED_KEY_REDUCED },
		{ 200.0, { 4.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 1000.0 - step, { 4.0f }, UFBX_BAKED_KEY_REDUCED },
		{ 1000.0, { 5.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 1000.0 + step, { 6.0f }, UFBX_BAKED_KEY_REDUCED },
		{ 2000.0, { 6.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 10000.0 - step, { 6.0f }, UFBX_BAKED_KEY_REDUCED },
		{ 10000.0, { 7.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10000.0 + step, { 8.0f }, UFBX_BAKED_KEY_REDUCED },
		{ 20000.0, { 8.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 100000.0 - step, { 8.0f }, UFBX_BAKED_KEY_REDUCED },
		{ 100000.0, { 9.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 100000.0 + step, { 10.0f }, UFBX_BAKED_KEY_REDUCED },
		{ 200000.0, { 10.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST(maya_stepped_tangent_sign)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.step_handling = UFBX_BAKE_STEP_HANDLING_ADJACENT_DOUBLE;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_baked_vec3 translation_ref[] = {
		{ -2.0, { -3.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ nextafter(-1.0, -INFINITY), { -3.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ -1.0, { -2.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ nextafter(-1.0, +INFINITY), { -1.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ -0.1, { -1.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 0.1, { 1.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ nextafter(1.0, -INFINITY), { 1.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 1.0, { 2.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ nextafter(1.0, +INFINITY), { 3.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 2.0, { 3.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_ALT(maya_stepped_tangent_sign_default, maya_stepped_tangent_sign)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	double step = 0.001;
	ufbx_baked_vec3 translation_ref[] = {
		{ -2.0, { -3.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ -1.0 - step, { -3.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ -1.0, { -2.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ -1.0 + step, { -1.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ -0.1, { -1.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 0.1, { 1.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 1.0 - step, { 1.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 1.0, { 2.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 1.0 + step, { 3.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 2.0, { 3.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_ALT(maya_stepped_tangent_sign_epsilon, maya_stepped_tangent_sign)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.step_handling = UFBX_BAKE_STEP_HANDLING_CUSTOM_DURATION;
	opts.step_custom_epsilon = 0.05;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	const double epsilon = 1.0 + 0.05;
	ufbx_baked_vec3 translation_ref[] = {
		{ -2.0, { -3.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ -1.0 * epsilon, { -3.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ -1.0, { -2.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ -1.0 / epsilon, { -1.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ -0.1, { -1.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 0.1, { 1.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 1.0 / epsilon, { 1.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 1.0, { 2.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 1.0 * epsilon, { 3.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 2.0, { 3.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_ALT(maya_stepped_tangent_sign_huge_step, maya_stepped_tangent_sign)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.step_handling = UFBX_BAKE_STEP_HANDLING_CUSTOM_DURATION;
	opts.step_custom_duration = 0.25;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	double step = 0.25;
	ufbx_baked_vec3 translation_ref[] = {
		{ -2.0, { -3.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ -1.0 - step, { -3.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ -1.0, { -2.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ -1.0 + step, { -1.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ -0.1, { -1.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 0.1, { 1.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 1.0 - step, { 1.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 1.0, { 2.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 1.0 + step, { 3.0f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 2.0, { 3.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST(maya_late_stepped_tangents)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.step_handling = UFBX_BAKE_STEP_HANDLING_ADJACENT_DOUBLE;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_baked_vec3 translation_ref[] = {
		{ 80.0/24.0, { -3.0 }, UFBX_BAKED_KEY_KEYFRAME },
		{ nextafter(85.0/24.0, -INFINITY), { -3.0 }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 85.0/24.0, { -1.0 }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ nextafter(85.0/24.0, INFINITY), { 0.0 }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 90.0/24.0, { 0.0 },  UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ nextafter(90.0/24.0, INFINITY), { 1.0 },  UFBX_BAKED_KEY_STEP_RIGHT },
		{ 95.0/24.0, { 1.0 },  UFBX_BAKED_KEY_KEYFRAME },
		{ nextafter(100.0/24.0, -INFINITY), { 1.0 }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 100.0/24.0, { 2.0 }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_ALT(maya_late_stepped_tangents_trim, maya_late_stepped_tangents)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.step_handling = UFBX_BAKE_STEP_HANDLING_ADJACENT_DOUBLE;
	opts.trim_start_time = true;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0/24.0, { -3.0 }, UFBX_BAKED_KEY_KEYFRAME },
		{ nextafter(5.0/24.0, -INFINITY), { -3.0 }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 5.0/24.0, { -1.0 }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ nextafter(5.0/24.0, INFINITY), { 0.0 }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 10.0/24.0, { 0.0 },  UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ nextafter(10.0/24.0, INFINITY), { 1.0 },  UFBX_BAKED_KEY_STEP_RIGHT },
		{ 15.0/24.0, { 1.0 },  UFBX_BAKED_KEY_KEYFRAME },
		{ nextafter(20.0/24.0, -INFINITY), { 1.0 }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 20.0/24.0, { 2.0 }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST(maya_anim_diffuse_curve)
#if UFBXT_IMPL
{
	ufbx_error error;

	ufbx_bake_opts opts = { 0 };
	opts.resample_rate = 24.0;

	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbxt_assert(bake->nodes.count == 0);
	ufbxt_assert(bake->elements.count == 1);

	ufbx_baked_element *elem = &bake->elements.data[0];
	ufbx_element *ref_elem = scene->elements.data[elem->element_id];
	ufbxt_assert(ref_elem->type == UFBX_ELEMENT_MATERIAL);
	ufbxt_assert(!strcmp(ref_elem->name.data, "lambert1"));

	ufbxt_assert(elem->props.count == 1);
	ufbx_baked_prop *prop = &elem->props.data[0];

	ufbxt_check_string(prop->name);
	ufbxt_assert(!strcmp(prop->name.data, "DiffuseColor"));

	ufbx_baked_vec3 diffuse_ref[] = {
		{ 0.0/24.0, { 0.0f, 0.5f, 0.5f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 4.0/24.0, { 1.0f, 0.5f, 0.5f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 6.0/24.0 - 0.001, { 1.0f, 0.5f, 0.5f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 6.0/24.0, { 0.75f, 0.5f, 0.5f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 7.0/24.0, { 0.415319f, 0.5f, 0.5f }, 0 },
		{ 8.0/24.0, { 0.795f, 0.5f, 0.5f }, 0 },
		{ 9.0/24.0, { 1.152243f, 0.5f, 0.5f }, 0 },
		{ 10.0/24.0, { 0.75f, 0.5f, 0.5f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0 + 0.001, { 0.25f, 0.5f, 0.5f }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 12.0/24.0, { 0.25f, 0.5f, 0.5f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 13.0/24.0, { 0.75f, 0.5f, 0.5f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 14.0/24.0, { 0.0f, 0.5f, 0.5f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_diff_baked_vec3(err, diffuse_ref, prop->keys);

	ufbx_retain_baked_anim(bake);
	ufbx_free_baked_anim(bake);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_ALT(anim_bake_alloc_fail, maya_anim_diffuse_curve)
#if UFBXT_IMPL
{
	for (size_t max_temp = 1; max_temp < 10000; max_temp++) {
		ufbx_bake_opts opts = { 0 };
		opts.temp_allocator.huge_threshold = 1;
		opts.temp_allocator.allocation_limit = max_temp;

		ufbxt_hintf("Temp limit: %zu", max_temp);

		ufbx_error error;
		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
		if (bake) {
			ufbxt_logf(".. Tested up to %zu temporary allocations", max_temp);
			ufbx_free_baked_anim(bake);
			break;
		}
		ufbxt_assert(error.type == UFBX_ERROR_ALLOCATION_LIMIT);
	}

	for (size_t max_result = 1; max_result < 10000; max_result++) {
		ufbx_bake_opts opts = { 0 };
		opts.result_allocator.huge_threshold = 1;
		opts.result_allocator.allocation_limit = max_result;

		ufbxt_hintf("Result limit: %zu", max_result);

		ufbx_error error;
		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
		if (bake) {
			ufbxt_logf(".. Tested up to %zu result allocations", max_result);
			ufbx_free_baked_anim(bake);
			break;
		}
		ufbxt_assert(error.type == UFBX_ERROR_ALLOCATION_LIMIT);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(anim_bake_alloc_fail_alt, motionbuilder_sausage_rrss, ufbxt_scale_helper_opts, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	for (size_t max_temp = 1; max_temp < 10000; max_temp++) {
		ufbx_bake_opts opts = { 0 };
		opts.temp_allocator.huge_threshold = 1;
		opts.temp_allocator.allocation_limit = max_temp;

		ufbxt_hintf("Temp limit: %zu", max_temp);

		ufbx_error error;
		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
		if (bake) {
			ufbxt_logf(".. Tested up to %zu temporary allocations", max_temp);
			ufbx_free_baked_anim(bake);
			break;
		}
		ufbxt_assert(error.type == UFBX_ERROR_ALLOCATION_LIMIT);
	}

	for (size_t max_result = 1; max_result < 10000; max_result++) {
		ufbx_bake_opts opts = { 0 };
		opts.result_allocator.huge_threshold = 1;
		opts.result_allocator.allocation_limit = max_result;

		ufbxt_hintf("Result limit: %zu", max_result);

		ufbx_error error;
		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
		if (bake) {
			ufbxt_logf(".. Tested up to %zu result allocations", max_result);
			ufbx_free_baked_anim(bake);
			break;
		}
		ufbxt_assert(error.type == UFBX_ERROR_ALLOCATION_LIMIT);
	}
}
#endif

UFBXT_FILE_TEST(maya_anim_pivot_rotate)
#if UFBXT_IMPL
{
	ufbx_error error;

	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0/24.0, { 0.0f, 0.0f, 0.0f } },
		{ 1.0/24.0, { 0.0f, 0.066987f, -0.250f } },
		{ 2.0/24.0, { 0.0f, 0.250f, -0.433f } },
		{ 3.0/24.0, { 0.0f, 0.5f, -0.5f } },
		{ 4.0/24.0, { 0.0f, 0.75f, -0.433f } },
		{ 5.0/24.0, { 0.0f, 0.933013f, -0.250f } },
		{ 6.0/24.0, { 0.0f, 1.0f, 0.0f } },
	};

	{
		ufbx_bake_opts opts = { 0 };
		opts.resample_rate = 24.0;
		opts.key_reduction_enabled = true;
		opts.key_reduction_rotation = true;

		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
		if (!bake) ufbxt_log_error(&error);
		ufbxt_assert(bake);

		ufbxt_assert(bake->nodes.count == 1);
		ufbx_baked_node *node = &bake->nodes.data[0];

		ufbx_node *ref_node = scene->nodes.data[node->typed_id];
		ufbxt_assert(ref_node->element_id == node->element_id);
		ufbxt_assert(!strcmp(ref_node->name.data, "pCube1"));

		ufbx_baked_vec3 rotation_ref[] = {
			{ 0.0/24.0, { 0.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
			{ 6.0/24.0, { 180.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		};

		ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);
		ufbxt_diff_baked_quat(err, rotation_ref, node->rotation_keys);

		ufbx_free_baked_anim(bake);
	}

	{
		ufbx_bake_opts opts = { 0 };
		opts.resample_rate = 24.0;
		opts.key_reduction_enabled = true;

		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
		if (!bake) ufbxt_log_error(&error);
		ufbxt_assert(bake);

		ufbxt_assert(bake->nodes.count == 1);
		ufbx_baked_node *node = &bake->nodes.data[0];

		ufbx_node *ref_node = scene->nodes.data[node->typed_id];
		ufbxt_assert(ref_node->element_id == node->element_id);
		ufbxt_assert(!strcmp(ref_node->name.data, "pCube1"));

		ufbx_baked_vec3 rotation_ref[] = {
			{ 0.0/24.0, { 0.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
			{ 1.0/24.0, { 30.0f, 0.0f, 0.0f } },
			{ 2.0/24.0, { 60.0f, 0.0f, 0.0f } },
			{ 3.0/24.0, { 90.0f, 0.0f, 0.0f } },
			{ 4.0/24.0, { 120.0f, 0.0f, 0.0f } },
			{ 5.0/24.0, { 150.0f, 0.0f, 0.0f } },
			{ 6.0/24.0, { 180.0f, 0.0f, 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		};

		ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);
		ufbxt_diff_baked_quat(err, rotation_ref, node->rotation_keys);

		ufbx_free_baked_anim(bake);
	}
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_anim_scale_helper_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_HELPER_NODES;
	opts.scale_helper_name.data = "(scale helper)";
	opts.scale_helper_name.length = SIZE_MAX;
	return opts;
}

typedef struct {
	ufbx_real scale;
	ufbx_real global_y;
} ufbxt_no_inherit_scale_node_ref;

typedef struct {
	double frame;
	ufbxt_no_inherit_scale_node_ref nodes[4];
} ufbxt_no_inherit_scale_key_ref;

const ufbxt_no_inherit_scale_key_ref ufbxt_no_inherit_scale_ref[] = {
	{ 0.0, { { 1.0f, 0.0f }, { 2.0f, 4.0f }, { 1.0f, 12.0f }, { 1.0f, 16.0f } } },
	{ 6.0, { { 1.5f, 0.0f }, { 1.5f, 6.0f }, { 1.0f, 12.0f }, { 1.0f, 16.0f } } },
	{ 12.0, { { 2.0f, 0.0f }, { 1.0f, 8.0f }, { 1.0f, 12.0f }, { 1.0f, 16.0f } } },
	{ 18.0, { { 0.5f, 0.0f }, { 1.5f, 2.0f }, { 1.0f, 8.0f }, { 1.0f, 12.0f } } },
	{ 24.0, { { 0.5f, 0.0f }, { 2.0f, 2.0f }, { 1.5f, 10.0f }, { 1.0f, 16.0f } } },
};

static ufbx_matrix ufbxt_evaluate_baked_matrix(ufbx_baked_anim *bake, ufbx_node *node, double time)
{
	ufbx_transform transform = node->local_transform;
	for (size_t i = 0; i < bake->nodes.count; i++) {
		const ufbx_baked_node *baked_node = &bake->nodes.data[i];
		if (baked_node->typed_id == node->typed_id) {
			if (baked_node->translation_keys.count > 0) {
				transform.translation = ufbx_evaluate_baked_vec3(baked_node->translation_keys, time);
			}
			if (baked_node->rotation_keys.count > 0) {
				transform.rotation = ufbx_evaluate_baked_quat(baked_node->rotation_keys, time);
			}
			if (baked_node->scale_keys.count > 0) {
				transform.scale = ufbx_evaluate_baked_vec3(baked_node->scale_keys, time);
			}
		}
	}

	ufbx_matrix node_to_parent = ufbx_transform_to_matrix(&transform);
	if (node->parent) {
		ufbx_matrix parent_to_world = ufbxt_evaluate_baked_matrix(bake, node->parent, time);
		node_to_parent = ufbx_matrix_mul(&parent_to_world, &node_to_parent);
	}
	return node_to_parent;
}

#endif

UFBXT_FILE_TEST(maya_anim_no_inherit_scale)
#if UFBXT_IMPL
{
	ufbx_node *nodes[4];
	nodes[0] = ufbx_find_node(scene, "joint1");
	nodes[1] = ufbx_find_node(scene, "joint2");
	nodes[2] = ufbx_find_node(scene, "joint3");
	nodes[3] = ufbx_find_node(scene, "joint4");
	ufbxt_assert(nodes[0] && nodes[1] && nodes[2] && nodes[3]);

	for (size_t key_ix = 0; key_ix < ufbxt_arraycount(ufbxt_no_inherit_scale_ref); key_ix++) {
		ufbxt_no_inherit_scale_key_ref key = ufbxt_no_inherit_scale_ref[key_ix];
		double time = key.frame / 24.0;

		ufbx_scene *state = ufbx_evaluate_scene(scene, NULL, time, NULL, NULL);
		ufbxt_assert(state);

		for (size_t node_ix = 0; node_ix < 4; node_ix++) {
			ufbxt_no_inherit_scale_node_ref ref = key.nodes[node_ix];
			ufbxt_hintf("key_ix=%zu node_ix=%zu", key_ix, node_ix);

			ufbx_node *node = state->nodes.data[nodes[node_ix]->typed_id];
			if (node->scale_helper) {
				node = node->scale_helper;
			}
			ufbx_matrix node_to_world = node->node_to_world;
			ufbx_transform world_transform = ufbx_matrix_to_transform(&node_to_world);

			ufbx_vec3 ref_translation = { 0.0f, ref.global_y, 0.0f };
			ufbx_vec3 ref_scale = { ref.scale, ref.scale, ref.scale };

			ufbxt_assert_close_vec3(err, world_transform.translation, ref_translation);
			ufbxt_assert_close_vec3(err, world_transform.scale, ref_scale);
		}

		ufbx_free_scene(state);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_anim_no_inherit_scale_helper, maya_anim_no_inherit_scale, ufbxt_anim_scale_helper_opts)
#if UFBXT_IMPL
{
	ufbx_node *nodes[4];
	nodes[0] = ufbx_find_node(scene, "joint1");
	nodes[1] = ufbx_find_node(scene, "joint2");
	nodes[2] = ufbx_find_node(scene, "joint3");
	nodes[3] = ufbx_find_node(scene, "joint4");
	ufbxt_assert(nodes[0] && nodes[1] && nodes[2] && nodes[3]);

	for (size_t key_ix = 0; key_ix < ufbxt_arraycount(ufbxt_no_inherit_scale_ref); key_ix++) {
		ufbxt_no_inherit_scale_key_ref key = ufbxt_no_inherit_scale_ref[key_ix];
		double time = key.frame / 24.0;

		ufbx_scene *state = ufbx_evaluate_scene(scene, NULL, time, NULL, NULL);
		ufbxt_assert(state);

		for (size_t node_ix = 0; node_ix < 4; node_ix++) {
			ufbxt_no_inherit_scale_node_ref ref = key.nodes[node_ix];
			ufbxt_hintf("key_ix=%zu node_ix=%zu", key_ix, node_ix);

			ufbx_node *node = state->nodes.data[nodes[node_ix]->typed_id];
			if (node->scale_helper) {
				node = node->scale_helper;
			}
			ufbx_matrix node_to_world = node->node_to_world;
			ufbx_transform world_transform = ufbx_matrix_to_transform(&node_to_world);

			ufbx_vec3 ref_translation = { 0.0f, ref.global_y, 0.0f };
			ufbx_vec3 ref_scale = { ref.scale, ref.scale, ref.scale };

			ufbxt_assert_close_vec3(err, world_transform.translation, ref_translation);
			ufbxt_assert_close_vec3(err, world_transform.scale, ref_scale);
		}

		ufbx_free_scene(state);
	}

	ufbx_error error;
	ufbx_bake_opts opts = { 0 };
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	for (size_t key_ix = 0; key_ix < ufbxt_arraycount(ufbxt_no_inherit_scale_ref); key_ix++) {
		ufbxt_no_inherit_scale_key_ref key = ufbxt_no_inherit_scale_ref[key_ix];
		double time = key.frame / 24.0;

		for (size_t node_ix = 0; node_ix < 4; node_ix++) {
			ufbxt_no_inherit_scale_node_ref ref = key.nodes[node_ix];
			ufbxt_hintf("key_ix=%zu node_ix=%zu", key_ix, node_ix);

			ufbx_node *node = nodes[node_ix];
			if (node->scale_helper) {
				node = node->scale_helper;
			}
			ufbx_matrix node_to_world = ufbxt_evaluate_baked_matrix(bake, node, time);
			ufbx_transform world_transform = ufbx_matrix_to_transform(&node_to_world);

			ufbx_vec3 ref_translation = { 0.0f, ref.global_y, 0.0f };
			ufbx_vec3 ref_scale = { ref.scale, ref.scale, ref.scale };

			ufbxt_assert_close_vec3(err, world_transform.translation, ref_translation);
			ufbxt_assert_close_vec3(err, world_transform.scale, ref_scale);
		}
	}

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST(maya_anim_layer_anim)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.resample_rate = 24.0;

	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, NULL);
	ufbxt_assert(bake);

	ufbxt_assert(bake->nodes.count == 1);

	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_assert(!strcmp(scene->nodes.data[node->typed_id]->name.data, "pCube1"));

	static const ufbxt_ref_transform ref[] = {
		{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -0.305556f, 0.0f }, { 3.75f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -0.555556f, 0.0f }, { 7.5f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -0.75f, 0.0f }, { 11.25f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -0.888889f, 0.0f }, { 15.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -0.972222f, 0.0f }, { 18.75f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -1.0f, 0.0f }, { 22.5f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -0.972222f, 0.0f }, { 26.25f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -0.888889f, 0.0f }, { 30.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -0.75f, 0.0f }, { 33.75f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -0.555556f, 0.0f }, { 37.5f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -0.305556f, 0.0f }, { 41.25f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, 0.0f, 0.0f }, { 45.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -0.333333f, 0.0f }, { 41.25f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -0.666667f, 0.0f }, { 37.5f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -1.0f, 0.0f }, { 33.75f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -1.333333f, 0.0f }, { 30.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -1.666667f, 0.0f }, { 26.25f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -2.0f, 0.0f }, { 22.5f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -2.333333f, 0.0f }, { 18.75f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -2.666667f, 0.0f }, { 15.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -3.0f, 0.0f }, { 11.25f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -3.333333f, 0.0f }, { 7.5f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -3.666667f, 0.0f }, { 3.75f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.0f, -4.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
	};

	size_t num_keys = ufbxt_arraycount(ref);
	ufbxt_assert(node->translation_keys.count == num_keys);
	ufbxt_assert(node->rotation_keys.count == num_keys);
	ufbxt_assert(node->scale_keys.count == num_keys);

	for (size_t i = 0; i < num_keys; i++) {
		double time = (double)i / 24.0;
		ufbxt_hintf("i=%zu time=%f\n", i, time);

		// Times are exact
		ufbxt_assert(node->translation_keys.data[i].time == time);
		ufbxt_assert(node->rotation_keys.data[i].time == time);
		ufbxt_assert(node->scale_keys.data[i].time == time);

		ufbx_quat rotation_quat = ufbx_euler_to_quat(ref[i].rotation_euler, UFBX_ROTATION_ORDER_XYZ);

		ufbxt_assert_close_vec3(err, node->translation_keys.data[i].value, ref[i].translation);
		ufbxt_assert_close_quat(err, node->rotation_keys.data[i].value, rotation_quat);
		ufbxt_assert_close_vec3(err, node->scale_keys.data[i].value, ref[i].scale);

		ufbxt_assert_close_vec3(err, ufbx_evaluate_baked_vec3(node->translation_keys, time), ref[i].translation);
		ufbxt_assert_close_quat(err, ufbx_evaluate_baked_quat(node->rotation_keys, time), rotation_quat);
		ufbxt_assert_close_vec3(err, ufbx_evaluate_baked_vec3(node->scale_keys, time), ref[i].scale);
	}

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST(maya_keyframe_spacing)
#if UFBXT_IMPL
{
	{
		ufbx_bake_opts opts = { 0 };
		opts.maximum_sample_rate = 10.0;
		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, NULL);
		ufbxt_assert(bake);

		static const double ref_times[] = {
			0.0/10.0,
			1.0/10.0,
			2.0/10.0,
			3.0/10.0,
			4.0/10.0,
			6.0/10.0,
			8.0/10.0,
		};

		ufbxt_assert(bake->nodes.count == 1);
		ufbx_baked_vec3_list list = bake->nodes.data[0].translation_keys;
		for (size_t i = 0; i < list.count; i++) {
			ufbxt_hintf("i=%zu", i);
			ufbxt_assert(i < ufbxt_arraycount(ref_times));
			ufbxt_hintf("i=%zu: %f (ref %f)", i, list.data[i].time, ref_times[i]);
			ufbxt_assert(list.data[i].time == ref_times[i]);
		}
		ufbxt_assert(list.count == ufbxt_arraycount(ref_times));

		ufbx_free_baked_anim(bake);
	}

	{
		ufbx_bake_opts opts = { 0 };
		opts.maximum_sample_rate = 20.0;
		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, NULL);
		ufbxt_assert(bake);

		static const double ref_times[] = {
			0.0/20.0,
			1.0/20.0,
			2.0/20.0,
			3.0/20.0,
			4.0/20.0,
			5.0/20.0,
			44.0/120.0,
			12.0/20.0,
			15.0/20.0,
			16.0/20.0,
		};

		ufbxt_assert(bake->nodes.count == 1);
		ufbx_baked_vec3_list list = bake->nodes.data[0].translation_keys;
		for (size_t i = 0; i < list.count; i++) {
			ufbxt_hintf("i=%zu", i);
			ufbxt_assert(i < ufbxt_arraycount(ref_times));
			ufbxt_hintf("i=%zu: %f (ref %f)", i, list.data[i].time, ref_times[i]);
			ufbxt_assert(list.data[i].time == ref_times[i]);
		}
		ufbxt_assert(list.count == ufbxt_arraycount(ref_times));

		ufbx_free_baked_anim(bake);
	}

	{
		ufbx_bake_opts opts = { 0 };
		opts.maximum_sample_rate = 30.0;
		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, NULL);
		ufbxt_assert(bake);

		static const double ref_times[] = {
			0.0/30.0,
			1.0/30.0,
			2.0/30.0,
			3.0/30.0,
			4.0/30.0,
			5.0/30.0,
			7.0/30.0,
			8.0/30.0,
			44.0/120.0,
			18.0/30.0,
			23.0/30.0,
		};

		ufbxt_assert(bake->nodes.count == 1);
		ufbx_baked_vec3_list list = bake->nodes.data[0].translation_keys;
		for (size_t i = 0; i < list.count; i++) {
			ufbxt_hintf("i=%zu", i);
			ufbxt_assert(i < ufbxt_arraycount(ref_times));
			ufbxt_hintf("i=%zu: %f (ref %f)", i, list.data[i].time, ref_times[i]);
			ufbxt_assert(list.data[i].time == ref_times[i]);
		}
		ufbxt_assert(list.count == ufbxt_arraycount(ref_times));

		ufbx_free_baked_anim(bake);
	}
}
#endif

#if UFBXT_IMPL
static ufbx_baked_node *ufbxt_find_baked_node(ufbx_baked_anim *bake, ufbx_node *node)
{
	for (size_t i = 0; i < bake->nodes.count; i++) {
		if (bake->nodes.data[i].typed_id == node->typed_id) {
			return &bake->nodes.data[i];
		}
	}
	return NULL;
}
#endif

UFBXT_FILE_TEST_OPTS(maya_scale_no_inherit_step, ufbxt_scale_helper_opts)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_baked_vec3 scale_ref[] = {
		{ 0.0/24.0, { 2.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME },
		{ 5.0/24.0 - 0.001, { 2.0, 1.0, 1.0 }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 5.0/24.0, { 3.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0, { 4.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0 + 0.001, { 5.0, 1.0, 1.0 }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 15.0/24.0, { 5.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0/24.0, { 10.0 }, 0 },
		{ 5.0/24.0 - 0.001, { 10.0 }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 5.0/24.0, { 15.0 }, UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0, { 20.0 }, UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0 + 0.001, { 25.0 }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 15.0/24.0, { 25.0 }, 0 },
	};

	ufbx_node *joint1_node = ufbx_find_node(scene, "joint1");
	ufbx_node *joint2_node = ufbx_find_node(scene, "joint2");
	ufbxt_assert(joint1_node);
	ufbxt_assert(joint1_node->scale_helper);
	ufbxt_assert(joint2_node);

	ufbx_baked_node *joint1_helper = ufbxt_find_baked_node(bake, joint1_node->scale_helper);
	ufbx_baked_node *joint2 = ufbxt_find_baked_node(bake, joint2_node);
	ufbxt_assert(joint1_helper);
	ufbxt_assert(joint2);
	ufbxt_diff_baked_vec3(err, scale_ref, joint1_helper->scale_keys);
	ufbxt_diff_baked_vec3(err, translation_ref, joint2->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_scale_no_inherit_step_identical, maya_scale_no_inherit_step, ufbxt_scale_helper_opts)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.step_handling = UFBX_BAKE_STEP_HANDLING_IDENTICAL_TIME;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_baked_vec3 scale_ref[] = {
		{ 0.0/24.0, { 2.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME },
		{ 5.0/24.0, { 2.0, 1.0, 1.0 }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 5.0/24.0, { 3.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0, { 4.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0, { 5.0, 1.0, 1.0 }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 15.0/24.0, { 5.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0/24.0, { 10.0 }, 0 },
		{ 5.0/24.0, { 10.0 }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 5.0/24.0, { 15.0 }, UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0, { 20.0 }, UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0, { 25.0 }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 15.0/24.0, { 25.0 }, 0 },
	};

	ufbx_node *joint1_node = ufbx_find_node(scene, "joint1");
	ufbx_node *joint2_node = ufbx_find_node(scene, "joint2");
	ufbxt_assert(joint1_node);
	ufbxt_assert(joint1_node->scale_helper);
	ufbxt_assert(joint2_node);

	ufbx_baked_node *joint1_helper = ufbxt_find_baked_node(bake, joint1_node->scale_helper);
	ufbx_baked_node *joint2 = ufbxt_find_baked_node(bake, joint2_node);
	ufbxt_assert(joint1_helper);
	ufbxt_assert(joint2);
	ufbxt_diff_baked_vec3(err, scale_ref, joint1_helper->scale_keys);
	ufbxt_diff_baked_vec3(err, translation_ref, joint2->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_scale_no_inherit_step_adjacent, maya_scale_no_inherit_step, ufbxt_scale_helper_opts)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.step_handling = UFBX_BAKE_STEP_HANDLING_ADJACENT_DOUBLE;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_baked_vec3 scale_ref[] = {
		{ 0.0/24.0, { 2.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME },
		{ nextafter(5.0/24.0, -INFINITY), { 2.0, 1.0, 1.0 }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 5.0/24.0, { 3.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0, { 4.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ nextafter(10.0/24.0, INFINITY), { 5.0, 1.0, 1.0 }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 15.0/24.0, { 5.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0/24.0, { 10.0 }, 0 },
		{ nextafter(5.0/24.0, -INFINITY), { 10.0 }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 5.0/24.0, { 15.0 }, UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0, { 20.0 }, UFBX_BAKED_KEY_STEP_KEY },
		{ nextafter(10.0/24.0, INFINITY), { 25.0 }, UFBX_BAKED_KEY_STEP_RIGHT },
		{ 15.0/24.0, { 25.0 }, 0 },
	};

	ufbx_node *joint1_node = ufbx_find_node(scene, "joint1");
	ufbx_node *joint2_node = ufbx_find_node(scene, "joint2");
	ufbxt_assert(joint1_node);
	ufbxt_assert(joint1_node->scale_helper);
	ufbxt_assert(joint2_node);

	ufbx_baked_node *joint1_helper = ufbxt_find_baked_node(bake, joint1_node->scale_helper);
	ufbx_baked_node *joint2 = ufbxt_find_baked_node(bake, joint2_node);
	ufbxt_assert(joint1_helper);
	ufbxt_assert(joint2);
	ufbxt_diff_baked_vec3(err, scale_ref, joint1_helper->scale_keys);
	ufbxt_diff_baked_vec3(err, translation_ref, joint2->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_scale_no_inherit_step_resmaple, maya_scale_no_inherit_step, ufbxt_scale_helper_opts)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.maximum_sample_rate = 100.0;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	double step = 1.0 / 100.0;
	ufbx_baked_vec3 scale_ref[] = {
		{ 0.0/24.0, { 2.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME },
		{ 5.0/24.0 - step, { 2.0, 1.0, 1.0 }, UFBX_BAKED_KEY_REDUCED },
		{ 5.0/24.0, { 3.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0, { 4.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0 + step, { 5.0, 1.0, 1.0 }, UFBX_BAKED_KEY_REDUCED },
		{ 15.0/24.0, { 5.0, 1.0, 1.0 }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0/24.0, { 10.0 }, 0 },
		{ 5.0/24.0 - step, { 10.0 }, 0 },
		{ 5.0/24.0, { 15.0 }, UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0, { 20.0 }, UFBX_BAKED_KEY_STEP_KEY },
		{ 10.0/24.0 + step, { 25.0 }, 0 },
		{ 15.0/24.0, { 25.0 }, 0 },
	};

	ufbx_node *joint1_node = ufbx_find_node(scene, "joint1");
	ufbx_node *joint2_node = ufbx_find_node(scene, "joint2");
	ufbxt_assert(joint1_node);
	ufbxt_assert(joint1_node->scale_helper);
	ufbxt_assert(joint2_node);

	ufbx_baked_node *joint1_helper = ufbxt_find_baked_node(bake, joint1_node->scale_helper);
	ufbx_baked_node *joint2 = ufbxt_find_baked_node(bake, joint2_node);
	ufbxt_assert(joint1_helper);
	ufbxt_assert(joint2);
	ufbxt_diff_baked_vec3(err, scale_ref, joint1_helper->scale_keys);
	ufbxt_diff_baked_vec3(err, translation_ref, joint2->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST(maya_keyframe_offset)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.resample_rate = 30.0;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_baked_vec3 rotation_ref[] = {
		{ 0.0/30.0, { 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 1.0/30.0, { 0.0f, 0.0f, -3.867188f } },
		{ 2.0/30.0, { 0.0f, 0.0f, -14.0625f } },
		{ 3.0/30.0, { 0.0f, 0.0f, -28.476563f } },
		{ 4.0/30.0, { 0.0f, 0.0f, -45.0f } },
		{ 5.0/30.0, { 0.0f, 0.0f, -61.523438f } },
		{ 6.0/30.0, { 0.0f, 0.0f, -75.9375f }, },
		{ 7.0/30.0, { 0.0f, 0.0f, -86.132813f }, },
		{ 8.0/30.0, { 0.0f, 0.0f, -90.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	// FBX representation of 6401/48000
	double mid_time = 6159116611.0/46186158000.0;
	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0/30.0, { 0.000000f, 2.000000f, 0.000000f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 1.0/30.0, { 0.134888f, 1.995446f, 0.249960f }, 0 },
		{ 2.0/30.0, { 0.485960f, 1.940062f, 0.499921f }, 0 },
		{ 3.0/30.0, { 0.953598f, 1.758024f, 0.749882f }, 0 },
		{ mid_time, { 1.414473f, 1.413953f, 1.000000f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 5.0/30.0, { 1.758024f, 0.953598f, 1.499765f }, 0 },
		{ 6.0/30.0, { 1.940062f, 0.485960f, 1.999843f }, 0 },
		{ 7.0/30.0, { 1.995446f, 0.134888f, 2.499921f }, 0 },
		{ 8.0/30.0, { 2.000000f, 0.000000f, 3.000000f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_quat(err, rotation_ref, node->rotation_keys);
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST(maya_keyframe_offset_step)
#if UFBXT_IMPL
{
	ufbx_bake_opts opts = { 0 };
	opts.resample_rate = 30.0;

	ufbx_error error;
	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_baked_vec3 rotation_ref[] = {
		{ 0.0/30.0, { 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 4.0/30.0 - 0.001, { 0.0f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 4.0/30.0, { 0.0f, 0.0f, -45.0f }, UFBX_BAKED_KEY_KEYFRAME|UFBX_BAKED_KEY_STEP_KEY },
		{ 5.0/30.0, { 0.0f, 0.0f, -52.253807f } },
		{ 6.0/30.0, { 0.0f, 0.0f, -67.697828f }, },
		{ 7.0/30.0, { 0.0f, 0.0f, -83.042936f }, },
		{ 8.0/30.0, { 0.0f, 0.0f, -90.0f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	// FBX representation of 6401/48000
	double mid_time = 6159116611.0/46186158000.0;
	ufbx_baked_vec3 translation_ref[] = {
		{ 0.0/30.0, { 0.000000f, 2.000000f, 0.000000f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 4.0/30.0 - 0.001, { 0.000000f, 2.000000f, 0.999843f }, UFBX_BAKED_KEY_STEP_LEFT },
		{ 4.0/30.0, { 1.414213f, 1.414213f, 0.999843f }, UFBX_BAKED_KEY_STEP_KEY },
		{ mid_time, { 1.414219f, 1.414207f, 1.000000f }, UFBX_BAKED_KEY_KEYFRAME },
		{ 5.0/30.0, { 1.581460f, 1.224329f, 1.499765f }, 0 },
		{ 6.0/30.0, { 1.850390f, 0.758982f, 1.999843f }, 0 },
		{ 7.0/30.0, { 1.985274f, 0.242251f, 2.499921f }, 0 },
		{ 8.0/30.0, { 2.000000f, 0.000000f, 3.000000f }, UFBX_BAKED_KEY_KEYFRAME },
	};

	ufbxt_assert(bake->nodes.count == 1);
	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_diff_baked_quat(err, rotation_ref, node->rotation_keys);
	ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST(maya_linear_spline)
#if UFBXT_IMPL
{
	{
		ufbx_bake_opts opts = { 0 };
		opts.resample_rate = 24.0;
		opts.bake_transform_props = true;

		ufbx_error error;
		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
		if (!bake) ufbxt_log_error(&error);
		ufbxt_assert(bake);

		ufbx_baked_vec3 translation_ref[] = {
			{ 0.0/24.0, { 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
			{ 1.0/24.0, { 0.333333f }, 0 },
			{ 2.0/24.0, { 0.666666f }, 0 },
			{ 3.0/24.0, { 1.0f }, 0 },
			{ 4.0/24.0, { 1.333333f }, 0 },
			{ 5.0/24.0, { 1.666666f }, 0 },
			{ 6.0/24.0, { 2.0f }, 0 },
			{ 7.0/24.0, { 2.333333f }, 0 },
			{ 8.0/24.0, { 2.666666f }, 0 },
			{ 9.0/24.0, { 3.0f }, 0 },
			{ 10.0/24.0, { 3.333333f }, 0 },
			{ 11.0/24.0, { 3.666666f }, 0 },
			{ 12.0/24.0, { 4.0f }, UFBX_BAKED_KEY_KEYFRAME },
		};

		ufbxt_assert(bake->nodes.count == 1);
		ufbx_baked_node *node = &bake->nodes.data[0];
		ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

		ufbxt_assert(bake->elements.count == 1);
		ufbx_baked_element *elem = &bake->elements.data[0];
		ufbxt_assert(elem->element_id == node->element_id);
		ufbx_baked_prop *prop = NULL;
		for (size_t i = 0; i < elem->props.count; i++) {
			if (!strcmp(elem->props.data[i].name.data, UFBX_Lcl_Translation)) {
				prop = &elem->props.data[i];
				break;
			}
		}
		ufbxt_assert(prop);
		ufbxt_diff_baked_vec3(err, translation_ref, prop->keys);

		ufbx_free_baked_anim(bake);
	}

	{
		ufbx_bake_opts opts = { 0 };
		opts.resample_rate = 24.0;
		opts.bake_transform_props = true;
		opts.key_reduction_enabled = true;
		opts.key_reduction_passes = 16;

		ufbx_error error;
		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
		if (!bake) ufbxt_log_error(&error);
		ufbxt_assert(bake);

		ufbx_baked_vec3 translation_ref[] = {
			{ 0.0/24.0, { 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
			{ 12.0/24.0, { 4.0f }, UFBX_BAKED_KEY_KEYFRAME },
		};

		ufbxt_assert(bake->nodes.count == 1);
		ufbx_baked_node *node = &bake->nodes.data[0];
		ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

		ufbxt_assert(bake->elements.count == 1);
		ufbx_baked_element *elem = &bake->elements.data[0];
		ufbxt_assert(elem->element_id == node->element_id);
		ufbx_baked_prop *prop = NULL;
		for (size_t i = 0; i < elem->props.count; i++) {
			if (!strcmp(elem->props.data[i].name.data, UFBX_Lcl_Translation)) {
				prop = &elem->props.data[i];
				break;
			}
		}
		ufbxt_assert(prop);
		ufbxt_diff_baked_vec3(err, translation_ref, prop->keys);

		ufbx_free_baked_anim(bake);
	}

	{
		ufbx_bake_opts opts = { 0 };
		opts.resample_rate = 24.0;
		opts.bake_transform_props = true;
		opts.key_reduction_enabled = true;
		opts.key_reduction_passes = 1;

		ufbx_error error;
		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
		if (!bake) ufbxt_log_error(&error);
		ufbxt_assert(bake);

		ufbx_baked_vec3 translation_ref[] = {
			{ 0.0/24.0, { 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
			{ 2.0/24.0, { 0.666666f }, 0 },
			{ 4.0/24.0, { 1.333333f }, 0 },
			{ 6.0/24.0, { 2.0f }, 0 },
			{ 8.0/24.0, { 2.666666f }, 0 },
			{ 10.0/24.0, { 3.333333f }, 0 },
			{ 12.0/24.0, { 4.0f }, UFBX_BAKED_KEY_KEYFRAME },
		};

		ufbxt_assert(bake->nodes.count == 1);
		ufbx_baked_node *node = &bake->nodes.data[0];
		ufbxt_diff_baked_vec3(err, translation_ref, node->translation_keys);

		ufbxt_assert(bake->elements.count == 1);
		ufbx_baked_element *elem = &bake->elements.data[0];
		ufbxt_assert(elem->element_id == node->element_id);
		ufbx_baked_prop *prop = NULL;
		for (size_t i = 0; i < elem->props.count; i++) {
			if (!strcmp(elem->props.data[i].name.data, UFBX_Lcl_Translation)) {
				prop = &elem->props.data[i];
				break;
			}
		}
		ufbxt_assert(prop);
		ufbxt_diff_baked_vec3(err, translation_ref, prop->keys);

		ufbx_free_baked_anim(bake);
	}

	{
		ufbx_bake_opts opts = { 0 };
		opts.resample_rate = 24.0;
		opts.bake_transform_props = true;
		opts.key_reduction_enabled = true;
		opts.skip_node_transforms = true;
		opts.key_reduction_passes = 16;

		ufbx_error error;
		ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, &opts, &error);
		if (!bake) ufbxt_log_error(&error);
		ufbxt_assert(bake);

		ufbx_baked_vec3 translation_ref[] = {
			{ 0.0/24.0, { 0.0f }, UFBX_BAKED_KEY_KEYFRAME },
			{ 12.0/24.0, { 4.0f }, UFBX_BAKED_KEY_KEYFRAME },
		};

		ufbxt_assert(bake->nodes.count == 0);

		ufbxt_assert(bake->elements.count == 1);
		ufbx_baked_element *elem = &bake->elements.data[0];
		ufbx_baked_prop *prop = NULL;
		for (size_t i = 0; i < elem->props.count; i++) {
			if (!strcmp(elem->props.data[i].name.data, UFBX_Lcl_Translation)) {
				prop = &elem->props.data[i];
				break;
			}
		}
		ufbxt_assert(prop);
		ufbxt_diff_baked_vec3(err, translation_ref, prop->keys);

		ufbx_free_baked_anim(bake);
	}
}
#endif

UFBXT_FILE_TEST_ALT(bake_search_node, maya_anim_linear)
#if UFBXT_IMPL
{
	ufbx_error error;

	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, NULL, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_node *scene_node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(scene_node);

	ufbx_baked_node *node = ufbx_find_baked_node(bake, scene_node);
	ufbxt_assert(node);
	ufbxt_assert(node->typed_id == scene_node->typed_id);
	ufbxt_assert(node->element_id == scene_node->element_id);

	ufbxt_assert(ufbx_find_baked_node_by_typed_id(bake, node->typed_id) == node);

	ufbxt_assert(ufbx_find_baked_node(NULL, NULL) == NULL);
	ufbxt_assert(ufbx_find_baked_node(bake, NULL) == NULL);
	ufbxt_assert(ufbx_find_baked_node_by_typed_id(bake, UINT32_MAX) == NULL);
	ufbxt_assert(ufbx_find_baked_node(bake, scene->root_node) == NULL);

	ufbx_free_baked_anim(bake);
}
#endif

UFBXT_FILE_TEST_ALT(bake_search_element, maya_anim_diffuse_curve)
#if UFBXT_IMPL
{
	ufbx_error error;

	ufbx_baked_anim *bake = ufbxt_bake_anim(scene, NULL, NULL, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbx_material *material = ufbx_find_material(scene, "lambert1");
	ufbxt_assert(material);

	ufbx_baked_element *element = ufbx_find_baked_element(bake, &material->element);
	ufbxt_assert(element);
	ufbxt_assert(element->element_id == material->element_id);

	ufbxt_assert(ufbx_find_baked_element_by_element_id(bake, material->element_id) == element);

	ufbxt_assert(ufbx_find_baked_element(NULL, NULL) == NULL);
	ufbxt_assert(ufbx_find_baked_element(bake, NULL) == NULL);
	ufbxt_assert(ufbx_find_baked_element_by_element_id(bake, UINT32_MAX) == NULL);
	ufbxt_assert(ufbx_find_baked_element(bake, &scene->root_node->element) == NULL);

	ufbx_free_baked_anim(bake);
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_ignore_animation_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.ignore_animation = true;
	return opts;
}
#endif

#if UFBXT_IMPL
static void ufbxt_assert_close_extrapolation(ufbxt_diff_error *p_err, ufbx_real value, ufbx_real ref)
{
	#if UFBX_REAL_IS_FLOAT
	double abs_error = (ufbx_real)100.0;
	double rel_error = fmax(fabs(ref), 1.0f) * 1e-4;
	#else
	double abs_error = (ufbx_real)0.001;
	double rel_error = fmax(fabs(ref), 1.0f) * 1e-4;
	#endif

	ufbx_real threshold = (ufbx_real)fmin(abs_error, rel_error);
	ufbxt_assert_close_real_threshold(p_err, value, ref, threshold);
}
#endif

UFBXT_FILE_TEST(maya_anim_extrapolation)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbxt_assert(scene->anim_layers.count == 1);
	ufbx_anim_layer *anim_layer = scene->anim_layers.data[0];

	ufbx_anim_prop *anim_prop = ufbx_find_anim_prop(anim_layer, &node->element, UFBX_Lcl_Translation);
	ufbxt_assert(anim_prop);

	ufbx_anim_curve *anim_curve = anim_prop->anim_value->curves[0];
	ufbxt_assert(anim_curve);

	if (scene->metadata.version >= 7200) {
		ufbxt_assert(anim_curve->pre_extrapolation.mode == UFBX_EXTRAPOLATION_REPEAT_RELATIVE);
	} else {
		ufbxt_assert(anim_curve->pre_extrapolation.mode == UFBX_EXTRAPOLATION_CONSTANT);
	}
	ufbxt_assert(anim_curve->pre_extrapolation.repeat_count == -1);
	ufbxt_assert(anim_curve->post_extrapolation.mode == UFBX_EXTRAPOLATION_REPEAT);
	ufbxt_assert(anim_curve->post_extrapolation.repeat_count == -1);
	ufbxt_assert(anim_curve->min_time == 1.0 / 24.0);
	ufbxt_assert(anim_curve->max_time == 1.0);
	ufbxt_assert(anim_curve->keyframes.count == 2);

	static const ufbxt_key_ref refs[] = {
		{ 1, 0.0 },
		{ 12, 4.674119 },
		{ 24, 10.0 },
		{ 25, 0.055067 },
		{ 37, 5.972713 },
		#if FLT_EVAL_METHOD == 0 // Too precise for x86
			{ 47, 0.0 },
			{ 5820, 0.0 },
			{ 4459471, 0.0 },
		#endif
		{ 4459472, 0.055067 },
		{ 9514493, 5.972713 },
		{ 100109698, 5.325881 },
		{ 0, -0.055067 },
		{ -12, -5.972713 },
		{ -70, -30.213693 },
		{ -4645, -2020.0 },
		{ -100509, -43700 },
		{ -100000181, -43478340 },
		{ -500000000, -217391304.674119 },
	};

	for (size_t i = 0; i < ufbxt_arraycount(refs); i++) {
		ufbxt_hintf("i=%zu", i);
		int ref_frame = refs[i].frame;
		ufbx_real ref_value = (ufbx_real)refs[i].value;

		// Pre-7200 versions don't support relative repeat
		if (ref_frame <= 0 && scene->metadata.version < 7200) {
			ref_value = anim_curve->keyframes.data[0].value;
		}

		double time = (double)ref_frame / 24.0;

		ufbx_real value = ufbx_evaluate_curve(anim_curve, time, 0.0);
		ufbxt_assert_close_extrapolation(err, value, ref_value);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_anim_extrapolation_ignore, maya_anim_extrapolation, ufbxt_ignore_animation_opts)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbxt_assert(scene->anim_layers.count == 1);
	ufbx_anim_layer *anim_layer = scene->anim_layers.data[0];

	ufbx_anim_prop *anim_prop = ufbx_find_anim_prop(anim_layer, &node->element, UFBX_Lcl_Translation);
	ufbxt_assert(anim_prop);

	if (scene->metadata.version >= 7000) {
		ufbx_anim_curve *anim_curve = anim_prop->anim_value->curves[0];
		ufbxt_assert(anim_curve);

		if (scene->metadata.version >= 7200) {
			ufbxt_assert(anim_curve->pre_extrapolation.mode == UFBX_EXTRAPOLATION_REPEAT_RELATIVE);
		} else {
			ufbxt_assert(anim_curve->pre_extrapolation.mode == UFBX_EXTRAPOLATION_CONSTANT);
		}
		ufbxt_assert(anim_curve->pre_extrapolation.repeat_count == -1);
		ufbxt_assert(anim_curve->post_extrapolation.mode == UFBX_EXTRAPOLATION_REPEAT);
		ufbxt_assert(anim_curve->post_extrapolation.repeat_count == -1);
		ufbxt_assert(anim_curve->keyframes.count == 0);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_extrapolation_slope, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbxt_assert(scene->anim_layers.count == 1);
	ufbx_anim_layer *anim_layer = scene->anim_layers.data[0];

	ufbx_anim_prop *anim_prop = ufbx_find_anim_prop(anim_layer, &node->element, UFBX_Lcl_Translation);
	ufbxt_assert(anim_prop);

	ufbx_anim_curve *anim_curve = anim_prop->anim_value->curves[0];
	ufbxt_assert(anim_curve);

	ufbxt_assert(anim_curve->post_extrapolation.mode == UFBX_EXTRAPOLATION_SLOPE);
	ufbxt_assert(anim_curve->pre_extrapolation.repeat_count == -1);
	ufbxt_assert(anim_curve->post_extrapolation.mode == UFBX_EXTRAPOLATION_SLOPE);
	ufbxt_assert(anim_curve->post_extrapolation.repeat_count == -1);
	ufbxt_assert(anim_curve->min_time == 1.0 / 24.0);
	ufbxt_assert(anim_curve->max_time == 1.0);
	ufbxt_assert(anim_curve->keyframes.count == 2);

	static const ufbxt_key_ref refs[] = {
		{ 5, -2.051765 },
		{ 20, 11.633479 },
		{ 30, 3.016101 },
		{ 61, -33.067375 },
		{ 14614, -16972.513672 },
		{ -5, 7.743417 },
		{ -33, 43.879364 },
		{ -94, 122.604103 },
		{ -242, 313.608398 },
		{ -14301, 18457.726562 },
	};

	for (size_t i = 0; i < ufbxt_arraycount(refs); i++) {
		ufbxt_hintf("i=%zu", i);
		int ref_frame = refs[i].frame;
		ufbx_real ref_value = (ufbx_real)refs[i].value;

		// Pre-7200 versions don't support relative repeat
		if (ref_frame <= 0 && scene->metadata.version < 7200) {
			ref_value = anim_curve->keyframes.data[0].value;
		}

		double time = (double)ref_frame / 24.0;

		ufbx_real value = ufbx_evaluate_curve(anim_curve, time, 0.0);
		ufbxt_assert_close_extrapolation(err, value, ref_value);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_extrapolation_mirror, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbxt_assert(scene->anim_layers.count == 1);
	ufbx_anim_layer *anim_layer = scene->anim_layers.data[0];

	ufbx_anim_prop *anim_prop = ufbx_find_anim_prop(anim_layer, &node->element, UFBX_Lcl_Translation);
	ufbxt_assert(anim_prop);

	ufbx_anim_curve *anim_curve = anim_prop->anim_value->curves[0];
	ufbxt_assert(anim_curve);

	ufbxt_assert(anim_curve->post_extrapolation.mode == UFBX_EXTRAPOLATION_MIRROR);
	ufbxt_assert(anim_curve->pre_extrapolation.repeat_count == -1);
	ufbxt_assert(anim_curve->post_extrapolation.mode == UFBX_EXTRAPOLATION_MIRROR);
	ufbxt_assert(anim_curve->post_extrapolation.repeat_count == -1);
	ufbxt_assert(anim_curve->min_time == 1.0 / 24.0);
	ufbxt_assert(anim_curve->max_time == 1.0);
	ufbxt_assert(anim_curve->keyframes.count == 2);

	static const ufbxt_key_ref refs[] = {
		{ 18, 10.635811 },
		{ 30, 10.635811 },
		{ 42, -1.749766 },
		{ 52, -1.749766 },
		{ 70, 10.000000 },
		{ 77, 9.815335 },
		{ 355, 7.719610 },
		{ 1706, -2.065491 },
		{ 17357, 7.719610 },
		{ 44874, 10.000000 },
		{ 44897, 0.000000 },
		{ -24, 11.522083 },
		{ -52, -0.431755 },
		{ -414, -1.077311 },
		{ -2207, 0.000000 },
		{ -27612, 6.519766 },
		{ -36132, 10.000000 },
		{ -36155, 0.000000 },
	};

	for (size_t i = 0; i < ufbxt_arraycount(refs); i++) {
		ufbxt_hintf("i=%zu", i);
		int ref_frame = refs[i].frame;
		ufbx_real ref_value = (ufbx_real)refs[i].value;

		// Pre-7200 versions don't support relative repeat
		if (ref_frame <= 0 && scene->metadata.version < 7200) {
			ref_value = anim_curve->keyframes.data[0].value;
		}

		double time = (double)ref_frame / 24.0;

		ufbx_real value = ufbx_evaluate_curve(anim_curve, time, 0.0);
		ufbxt_assert_close_extrapolation(err, value, ref_value);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_extrapolation_mirror_count, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbxt_assert(scene->anim_layers.count == 1);
	ufbx_anim_layer *anim_layer = scene->anim_layers.data[0];

	ufbx_anim_prop *anim_prop = ufbx_find_anim_prop(anim_layer, &node->element, UFBX_Lcl_Translation);
	ufbxt_assert(anim_prop);

	ufbx_anim_curve *anim_curve = anim_prop->anim_value->curves[0];
	ufbxt_assert(anim_curve);

	ufbxt_assert(anim_curve->post_extrapolation.mode == UFBX_EXTRAPOLATION_MIRROR);
	ufbxt_assert(anim_curve->pre_extrapolation.repeat_count == 2);
	ufbxt_assert(anim_curve->post_extrapolation.mode == UFBX_EXTRAPOLATION_MIRROR);
	ufbxt_assert(anim_curve->post_extrapolation.repeat_count == 3);
	ufbxt_assert(anim_curve->min_time == 1.0 / 24.0);
	ufbxt_assert(anim_curve->max_time == 1.0);
	ufbxt_assert(anim_curve->keyframes.count == 2);

	static const ufbxt_key_ref refs[] = {
		{ 7, -1.197194 },
		{ -16, 10.635811 },
		{ -29, 9.815335 },
		{ -42, -2.065491 },
		{ -45, 0.000000 },
		{ -73, 0.000000 },
		{ -100000, 0.000000 },
		{ 29, 11.254428 },
		{ 42, -1.749766 },
		{ 65, 11.254428 },
		{ 74, 11.633479 },
		{ 89, -2.051765 },
		{ 93, 0.000000 },
		{ 100, 0.000000 },
		{ 100000, 0.000000 },
	};

	for (size_t i = 0; i < ufbxt_arraycount(refs); i++) {
		ufbxt_hintf("i=%zu", i);
		int ref_frame = refs[i].frame;
		ufbx_real ref_value = (ufbx_real)refs[i].value;

		// Pre-7200 versions don't support relative repeat
		if (ref_frame <= 0 && scene->metadata.version < 7200) {
			ref_value = anim_curve->keyframes.data[0].value;
		}

		double time = (double)ref_frame / 24.0;

		ufbx_real value = ufbx_evaluate_curve(anim_curve, time, 0.0);
		ufbxt_assert_close_extrapolation(err, value, ref_value);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_extrapolation_repeat_count, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbxt_assert(scene->anim_layers.count == 1);
	ufbx_anim_layer *anim_layer = scene->anim_layers.data[0];

	ufbx_anim_prop *anim_prop = ufbx_find_anim_prop(anim_layer, &node->element, UFBX_Lcl_Translation);
	ufbxt_assert(anim_prop);

	ufbx_anim_curve *anim_curve = anim_prop->anim_value->curves[0];
	ufbxt_assert(anim_curve);

	ufbxt_assert(anim_curve->post_extrapolation.mode == UFBX_EXTRAPOLATION_REPEAT);
	ufbxt_assert(anim_curve->pre_extrapolation.repeat_count == 2);
	ufbxt_assert(anim_curve->post_extrapolation.mode == UFBX_EXTRAPOLATION_REPEAT);
	ufbxt_assert(anim_curve->post_extrapolation.repeat_count == 0);
	ufbxt_assert(anim_curve->min_time == 1.0 / 24.0);
	ufbxt_assert(anim_curve->max_time == 1.0);
	ufbxt_assert(anim_curve->keyframes.count == 2);

	static const ufbxt_key_ref refs[] = {
		{ 16, 8.830700 },
		{ -5, 10.635811 },
		{ -26, 11.633479 },
		{ -43, -1.753241 },
		{ -44, -1.077311 },
		{ -45, 0.000000 },
		{ -46, 0.000000 },
		{ -100, 0.000000 },
		{ -10000, 0.000000 },
		{ 24, 10.000000 },
		{ 25, 10.000000 },
		{ 100, 10.000000 },
		{ 10000, 10.000000 },
	};

	for (size_t i = 0; i < ufbxt_arraycount(refs); i++) {
		ufbxt_hintf("i=%zu", i);
		int ref_frame = refs[i].frame;
		ufbx_real ref_value = (ufbx_real)refs[i].value;

		// Pre-7200 versions don't support relative repeat
		if (ref_frame <= 0 && scene->metadata.version < 7200) {
			ref_value = anim_curve->keyframes.data[0].value;
		}

		double time = (double)ref_frame / 24.0;

		ufbx_real value = ufbx_evaluate_curve(anim_curve, time, 0.0);
		ufbxt_assert_close_extrapolation(err, value, ref_value);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_extrapolation_relative_count, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbxt_assert(scene->anim_layers.count == 1);
	ufbx_anim_layer *anim_layer = scene->anim_layers.data[0];

	ufbx_anim_prop *anim_prop = ufbx_find_anim_prop(anim_layer, &node->element, UFBX_Lcl_Translation);
	ufbxt_assert(anim_prop);

	ufbx_anim_curve *anim_curve = anim_prop->anim_value->curves[0];
	ufbxt_assert(anim_curve);

	ufbxt_assert(anim_curve->post_extrapolation.mode == UFBX_EXTRAPOLATION_REPEAT_RELATIVE);
	ufbxt_assert(anim_curve->pre_extrapolation.repeat_count == 2);
	ufbxt_assert(anim_curve->post_extrapolation.mode == UFBX_EXTRAPOLATION_REPEAT_RELATIVE);
	ufbxt_assert(anim_curve->post_extrapolation.repeat_count == 1);
	ufbxt_assert(anim_curve->min_time == 1.0 / 24.0);
	ufbxt_assert(anim_curve->max_time == 1.0);
	ufbxt_assert(anim_curve->keyframes.count == 2);

	static const ufbxt_key_ref refs[] = {
		{ 12, 4.004632 },
		{ -8, -2.280390 },
		{ -24, -8.477917 },
		{ -41, -22.051765 },
		{ -44, -21.077311 },
		{ -45, -20.000000 },
		{ -46, -20.000000 },
		{ -50, -20.000000 },
		{ -100, -20.000000 },
		{ -10000, -20.000000 },
		{ 25, 8.922688 },
		{ 30, 8.802806 },
		{ 43, 21.633480 },
		{ 46, 20.956230 },
		{ 47, 20.000000 },
		{ 50, 20.000000 },
		{ 100, 20.000000 },
		{ 10000, 20.000000 },
	};

	for (size_t i = 0; i < ufbxt_arraycount(refs); i++) {
		ufbxt_hintf("i=%zu", i);
		int ref_frame = refs[i].frame;
		ufbx_real ref_value = (ufbx_real)refs[i].value;

		// Pre-7200 versions don't support relative repeat
		if (ref_frame <= 0 && scene->metadata.version < 7200) {
			ref_value = anim_curve->keyframes.data[0].value;
		}

		double time = (double)ref_frame / 24.0;

		ufbx_real value = ufbx_evaluate_curve(anim_curve, time, 0.0);
		ufbxt_assert_close_extrapolation(err, value, ref_value);
	}
}
#endif

UFBXT_FILE_TEST(synthetic_unknown_extrapolation)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node);

	ufbxt_assert(scene->anim_layers.count == 1);
	ufbx_anim_layer *anim_layer = scene->anim_layers.data[0];

	ufbx_anim_prop *anim_prop = ufbx_find_anim_prop(anim_layer, &node->element, UFBX_Lcl_Translation);
	ufbxt_assert(anim_prop);

	ufbx_anim_curve *anim_curve = anim_prop->anim_value->curves[0];
	ufbxt_assert(anim_curve);

	ufbxt_assert(anim_curve->pre_extrapolation.mode == UFBX_EXTRAPOLATION_CONSTANT);
	ufbxt_assert(anim_curve->pre_extrapolation.repeat_count == -1);
	ufbxt_assert(anim_curve->pre_extrapolation.mode == UFBX_EXTRAPOLATION_CONSTANT);
	ufbxt_assert(anim_curve->post_extrapolation.repeat_count == -1);
}
#endif

UFBXT_FILE_TEST_ALT(bake_constant_rotation, maya_anim_interpolation)
#if UFBXT_IMPL
{
	ufbx_node *scene_node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(scene_node);

	ufbx_error error;
	ufbx_baked_anim *bake = ufbx_bake_anim(scene, scene->anim, NULL, &error);
	if (!bake) ufbxt_log_error(&error);
	ufbxt_assert(bake);

	ufbxt_assert(bake->nodes.count == 1);

	ufbx_baked_node *node = &bake->nodes.data[0];
	ufbxt_assert(node->typed_id == scene_node->typed_id);
	ufbxt_assert(!node->constant_translation);
	ufbxt_assert(node->constant_rotation);
	ufbxt_assert(node->constant_scale);

	ufbx_free_baked_anim(bake);
}
#endif
