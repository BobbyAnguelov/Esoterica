#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "lights"

UFBXT_FILE_TEST(motionbuilder_lights)
#if UFBXT_IMPL
{
	ufbx_vec3 white_color = { 1.0f, 1.0f, 1.0f };
	ufbx_vec3 ref_color = { 0.2f, 0.4f, 0.6f };

	{
		ufbx_node *node = ufbx_find_node(scene, "PointConstant");
		ufbxt_assert(node && node->light);
		ufbx_light *light = node->light;
		ufbxt_assert(light->type == UFBX_LIGHT_POINT);
		ufbxt_assert(light->decay == UFBX_LIGHT_DECAY_NONE);
		ufbxt_assert_close_vec3(err, light->color, white_color);
		ufbxt_assert_close_real(err, light->intensity, 1.0f);
		ufbxt_assert(light->cast_shadows);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "PointLinear");
		ufbxt_assert(node && node->light);
		ufbx_light *light = node->light;
		ufbxt_assert(light->type == UFBX_LIGHT_POINT);
		ufbxt_assert(light->decay == UFBX_LIGHT_DECAY_LINEAR);
		ufbxt_assert_close_vec3(err, light->color, ref_color);
		ufbxt_assert_close_real(err, light->intensity, 0.5f);
		ufbxt_assert(light->cast_shadows);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "PointQuadratic");
		ufbxt_assert(node && node->light);
		ufbx_light *light = node->light;
		ufbxt_assert(light->type == UFBX_LIGHT_POINT);
		ufbxt_assert(light->decay == UFBX_LIGHT_DECAY_QUADRATIC);
		ufbxt_assert_close_vec3(err, light->color, ref_color);
		ufbxt_assert_close_real(err, light->intensity, 1.5f);
		ufbxt_assert(light->cast_shadows);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "PointCubic");
		ufbxt_assert(node && node->light);
		ufbx_light *light = node->light;
		ufbxt_assert(light->type == UFBX_LIGHT_POINT);
		ufbxt_assert(light->decay == UFBX_LIGHT_DECAY_CUBIC);
		ufbxt_assert_close_vec3(err, light->color, ref_color);
		ufbxt_assert_close_real(err, light->intensity, 2.0f);
		ufbxt_assert(!light->cast_shadows);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "SpotConstant");
		ufbxt_assert(node && node->light);
		ufbx_light *light = node->light;
		ufbxt_assert(light->type == UFBX_LIGHT_SPOT);
		ufbxt_assert(light->decay == UFBX_LIGHT_DECAY_NONE);
		ufbxt_assert_close_vec3(err, light->color, white_color);
		ufbxt_assert_close_real(err, light->intensity, 1.0f);
		ufbxt_assert(light->cast_shadows);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "SpotLinear");
		ufbxt_assert(node && node->light);
		ufbx_light *light = node->light;
		ufbxt_assert(light->type == UFBX_LIGHT_SPOT);
		ufbxt_assert(light->decay == UFBX_LIGHT_DECAY_LINEAR);
		ufbxt_assert_close_vec3(err, light->color, ref_color);
		ufbxt_assert_close_real(err, light->intensity, 1.1f);
		ufbxt_assert(light->cast_shadows);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "SpotQuadratic");
		ufbxt_assert(node && node->light);
		ufbx_light *light = node->light;
		ufbxt_assert(light->type == UFBX_LIGHT_SPOT);
		ufbxt_assert(light->decay == UFBX_LIGHT_DECAY_QUADRATIC);
		ufbxt_assert_close_vec3(err, light->color, ref_color);
		ufbxt_assert_close_real(err, light->intensity, 1.2f);
		ufbxt_assert(light->cast_shadows);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "SpotCubic");
		ufbxt_assert(node && node->light);
		ufbx_light *light = node->light;
		ufbxt_assert(light->type == UFBX_LIGHT_SPOT);
		ufbxt_assert(light->decay == UFBX_LIGHT_DECAY_CUBIC);
		ufbxt_assert_close_vec3(err, light->color, ref_color);
		ufbxt_assert_close_real(err, light->intensity, 1.3f);
		ufbxt_assert(!light->cast_shadows);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "SpotLook");
		ufbxt_assert(node && node->light);
		ufbx_light *light = node->light;
		ufbxt_assert(light->type == UFBX_LIGHT_SPOT);
		ufbxt_assert(light->decay == UFBX_LIGHT_DECAY_QUADRATIC);
		ufbxt_assert_close_vec3(err, light->color, ref_color);
		ufbxt_assert_close_real(err, light->intensity, 1.0f);
		ufbxt_assert(light->cast_shadows);

		ufbx_node *look_at = (ufbx_node*)ufbx_find_prop_element(&node->element, "LookAtProperty", UFBX_ELEMENT_NODE);
		ufbxt_assert(look_at);
		ufbxt_assert(!strcmp(look_at->name.data, "Cube"));
	}
}
#endif
