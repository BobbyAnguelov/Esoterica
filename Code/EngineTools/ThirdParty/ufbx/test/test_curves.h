#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "curves"

UFBXT_FILE_TEST(max_curve_line)
#if UFBXT_IMPL
{
	ufbx_node *polyline_node = ufbx_find_node(scene, "Polyline");
	ufbx_node *bezier_node = ufbx_find_node(scene, "Bezier");
	ufbxt_assert(polyline_node);
	ufbxt_assert(bezier_node);
	ufbxt_assert(polyline_node->attrib_type == UFBX_ELEMENT_LINE_CURVE);
	ufbxt_assert(bezier_node->attrib_type == UFBX_ELEMENT_LINE_CURVE);
	ufbxt_assert(polyline_node->attrib);
	ufbxt_assert(bezier_node->attrib);

	static const ufbx_vec3 ref_points[] = {
		{ -1.0f, 0.0f, -1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{ 1.0f, 0.0f, 1.0f },
		{ 0.0f, -1.0f, 2.0f },
		{ -2.0f, 0.0f, 2.0f },
		{ -1.0f, 0.0f, 1.0f },
		{ -1.0f, 0.0f, -1.0f },
	};

	ufbx_line_curve *polyline = (ufbx_line_curve*)polyline_node->attrib;
	ufbxt_assert(polyline->point_indices.count == 7);
	ufbxt_assert(polyline->segments.count == 1);
	ufbxt_assert(polyline->segments.data[0].index_begin == 0);
	ufbxt_assert(polyline->segments.data[0].num_indices == 7);
	for (size_t i = 0; i < polyline->point_indices.count; i++) {
		ufbx_vec3 p = polyline->control_points.data[polyline->point_indices.data[i]];
		p = ufbx_transform_position(&polyline_node->geometry_to_world, p);
		p.x *= 0.1f;
		p.y *= 0.1f;
		p.z *= 0.1f;
		ufbxt_assert_close_vec3(err, p, ref_points[i]);
	}

	ufbx_vec3 polyline_color = { 0.223529411764706f ,0.0313725490196078f, 0.533333333333333f };
	ufbxt_assert_close_vec3(err, polyline->color, polyline_color);

	ufbx_line_curve *bezier = (ufbx_line_curve*)bezier_node->attrib;
	ufbxt_assert(bezier->point_indices.count == 15);
	ufbxt_assert(bezier->segments.count == 1);
	ufbxt_assert(bezier->segments.data[0].index_begin == 0);
	ufbxt_assert(bezier->segments.data[0].num_indices == 15);
	for (size_t i = 0; i < bezier->point_indices.count; i++) {
		ufbx_vec3 p = bezier->control_points.data[bezier->point_indices.data[i]];
		p = ufbx_transform_position(&bezier_node->geometry_to_world, p);
		p.x *= 0.1f;
		p.y *= 0.1f;
		p.z *= 0.1f;
		ufbxt_assert(p.y >= -0.0001f);
		ufbxt_assert_close_real(err, p.z, 0.0f);
	}

	ufbx_vec3 bezier_color = { 0.603921568627451f ,0.725490196078431f, 0.898039215686275f };
	ufbxt_assert_close_vec3(err, bezier->color, bezier_color);
}
#endif
