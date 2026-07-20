#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "legacy"

#if UFBXT_IMPL
static void ufbxt_diff_material_value(ufbxt_diff_error *err, const ufbx_material_map *color, const ufbx_material_map *factor, ufbx_vec3 value)
{
	ufbxt_assert_close_real(err, color->value_vec3.x * factor->value_vec3.x, value.x);
	ufbxt_assert_close_real(err, color->value_vec3.y * factor->value_vec3.x, value.y);
	ufbxt_assert_close_real(err, color->value_vec3.z * factor->value_vec3.x, value.z);
}
#endif

UFBXT_FILE_TEST(max7_cube)
#if UFBXT_IMPL
{
}
#endif

UFBXT_FILE_TEST(max7_cube_normals)
#if UFBXT_IMPL
{
}
#endif

UFBXT_FILE_TEST(max2009_blob)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->lights.count == 3);
	ufbxt_assert(scene->cameras.count == 1);

	{
		ufbx_node *node = ufbx_find_node(scene, "Box01");
		ufbxt_assert(node);
		ufbxt_assert(node->mesh);
		ufbxt_assert(node->children.count == 16);
		ufbx_mesh *mesh = node->mesh;

		size_t num_top = 0;
		size_t num_left = 0;
		size_t num_right = 0;
		size_t num_front = 0;

		for (size_t fi = 0; fi < mesh->num_faces; fi++) {
			ufbx_face face = mesh->faces.data[fi];
			ufbx_vec3 center = ufbx_zero_vec3;
			for (size_t i = 0; i < face.num_indices; i++) {
				ufbx_vec3 v = ufbx_get_vertex_vec3(&mesh->vertex_position, face.index_begin + i);
				center.x += v.x;
				center.y += v.y;
				center.z += v.z;
			}
			center.x /= (ufbx_real)face.num_indices;
			center.y /= (ufbx_real)face.num_indices;
			center.z /= (ufbx_real)face.num_indices;

			if (center.z >= 14.0f) {
				ufbx_material *mat = mesh->materials.data[mesh->face_material.data[fi]];
				ufbxt_assert(!strcmp(mat->name.data, "Top"));
				num_top++;
			}
			if (center.y <= -10.0f) {
				ufbx_material *mat = mesh->materials.data[mesh->face_material.data[fi]];
				ufbxt_assert(!strcmp(mat->name.data, "Right"));
				num_right++;
			}
			if (center.y >= 10.0f) {
				ufbx_material *mat = mesh->materials.data[mesh->face_material.data[fi]];
				ufbxt_assert(!strcmp(mat->name.data, "Left"));
				num_left++;
			}
			if (center.x >= 9.0f) {
				ufbx_material *mat = mesh->materials.data[mesh->face_material.data[fi]];
				ufbxt_assert(!strcmp(mat->name.data, "Front"));
				num_front++;
			}
		}

		ufbxt_assert(num_top >= 80);
		ufbxt_assert(num_left >= 80);
		ufbxt_assert(num_right >= 80);
		ufbxt_assert(num_front >= 80);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Omni01");
		ufbxt_assert(node && node->light);
		ufbx_light *light = node->light;
		ufbxt_assert(light->type == UFBX_LIGHT_POINT);
		if (scene->metadata.version < 6000) {
			ufbxt_assert(light->decay == UFBX_LIGHT_DECAY_NONE);
		}
		ufbx_vec3 color = { 0.172549024224281f, 0.364705890417099f, 1.0f };
		ufbxt_assert_close_vec3(err, light->color, color);
		ufbxt_assert_close_real(err, light->intensity, 1.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Fspot01");
		ufbxt_assert(node && node->light);
		ufbx_light *light = node->light;
		ufbxt_assert(light->type == UFBX_LIGHT_SPOT);
		if (scene->metadata.version < 6000) {
			ufbxt_assert(light->decay == UFBX_LIGHT_DECAY_NONE);
		}
		ufbx_vec3 color = { 0.972549080848694f ,0.0705882385373116f, 0.0705882385373116f };
		ufbxt_assert_close_vec3(err, light->color, color);
		ufbxt_assert_close_real(err, light->intensity, 1.0f);
		ufbxt_assert_close_real(err, light->outer_angle, 45.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "FDirect02");
		ufbxt_assert(node && node->light);
		ufbx_light *light = node->light;
		ufbxt_assert(light->type == UFBX_LIGHT_DIRECTIONAL);
		if (scene->metadata.version < 6000) {
			ufbxt_assert(light->decay == UFBX_LIGHT_DECAY_NONE);
		}
		ufbx_vec3 color = { 0.533333361148834f ,0.858823597431183f, 0.647058844566345f };
		ufbxt_assert_close_vec3(err, light->color, color);
		ufbxt_assert_close_real(err, light->intensity, 1.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Camera01");
		ufbxt_assert(node && node->camera);
		ufbx_camera *camera = node->camera;
		ufbxt_assert(camera->aspect_mode == UFBX_ASPECT_MODE_WINDOW_SIZE);
		ufbxt_assert(camera->aperture_mode == UFBX_APERTURE_MODE_HORIZONTAL);
		ufbx_vec2 aperture = { 1.41732287406921f ,1.06299209594727f };
		ufbxt_assert_close_real(err, camera->focal_length_mm, 43.4558439883016f);
		ufbxt_assert_close_vec2(err, camera->film_size_inch, aperture);
		ufbxt_assert_close_vec2(err, camera->aperture_size_inch, aperture);
	}

	{
		ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Left");
		ufbxt_assert(material);
		ufbx_vec3 ambient = { 0.588235318660736f, 0.588235318660736f, 0.588235318660736f };
		ufbx_vec3 diffuse = { 0.588235318660736f, 0.588235318660736f, 0.588235318660736f };
		ufbx_vec3 specular = { 0.179999984502793f, 0.179999984502793f, 0.179999984502793f };
		ufbx_vec3 emission = { 0.823529481887817f, 0.0f, 0.0f };
		ufbx_real shininess = 1.99999991737042f;
		ufbxt_diff_material_value(err, &material->fbx.ambient_color, &material->fbx.ambient_factor, ambient);
		ufbxt_diff_material_value(err, &material->fbx.diffuse_color, &material->fbx.diffuse_factor, diffuse);
		ufbxt_diff_material_value(err, &material->fbx.specular_color, &material->fbx.specular_factor, specular);
		ufbxt_diff_material_value(err, &material->fbx.emission_color, &material->fbx.emission_factor, emission);
		ufbxt_assert_close_real(err, material->fbx.specular_exponent.value_vec3.x, shininess);
	}

	{
		ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Front");
		ufbxt_assert(material);
		ufbx_vec3 ambient = { 0.588235318660736f, 0.921568691730499f, 0.925490260124207f };
		ufbx_vec3 diffuse = { 0.588235318660736f, 0.921568691730499f, 0.925490260124207f };
		ufbx_vec3 specular = { 0.0f, 0.0f, 0.0f };
		ufbx_vec3 emission = { 0.0f, 0.0f, 0.0f };
		ufbx_real shininess = 1.99999991737042f;
		ufbxt_diff_material_value(err, &material->fbx.ambient_color, &material->fbx.ambient_factor, ambient);
		ufbxt_diff_material_value(err, &material->fbx.diffuse_color, &material->fbx.diffuse_factor, diffuse);
		ufbxt_diff_material_value(err, &material->fbx.specular_color, &material->fbx.specular_factor, specular);
		ufbxt_diff_material_value(err, &material->fbx.emission_color, &material->fbx.emission_factor, emission);
		ufbxt_assert_close_real(err, material->fbx.specular_exponent.value_vec3.x, shininess);
	}

	{
		ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Right");
		ufbxt_assert(material);
		ufbx_vec3 ambient = { 0.588235318660736f, 0.588235318660736f, 0.588235318660736f };
		ufbx_vec3 diffuse = { 0.588235318660736f, 0.588235318660736f, 0.588235318660736f };
		ufbx_vec3 specular = { 0.0f, 0.0f, 0.0f };
		ufbx_vec3 emission = { 0.0f, 0.803921639919281f, 0.0f };
		ufbx_real shininess = 7.99999900844507f;
		ufbxt_diff_material_value(err, &material->fbx.ambient_color, &material->fbx.ambient_factor, ambient);
		ufbxt_diff_material_value(err, &material->fbx.diffuse_color, &material->fbx.diffuse_factor, diffuse);
		ufbxt_diff_material_value(err, &material->fbx.specular_color, &material->fbx.specular_factor, specular);
		ufbxt_diff_material_value(err, &material->fbx.emission_color, &material->fbx.emission_factor, emission);
		ufbxt_assert_close_real(err, material->fbx.specular_exponent.value_vec3.x, shininess);
	}

	{
		ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Top");
		ufbxt_assert(material);
		ufbx_vec3 ambient = { 0.564705908298492f, 0.603921592235565f, 0.890196144580841f };
		ufbx_vec3 diffuse = { 0.564705908298492f, 0.603921592235565f, 0.890196144580841f };
		ufbx_vec3 specular = { 0.0f, 0.0f, 0.0f };
		ufbx_vec3 emission = { 0.0f, 0.0f, 0.0f };
		ufbx_real shininess = 1.99999991737042f;
		ufbxt_diff_material_value(err, &material->fbx.ambient_color, &material->fbx.ambient_factor, ambient);
		ufbxt_diff_material_value(err, &material->fbx.diffuse_color, &material->fbx.diffuse_factor, diffuse);
		ufbxt_diff_material_value(err, &material->fbx.specular_color, &material->fbx.specular_factor, specular);
		ufbxt_diff_material_value(err, &material->fbx.emission_color, &material->fbx.emission_factor, emission);
		ufbxt_assert_close_real(err, material->fbx.specular_exponent.value_vec3.x, shininess);
	}

	ufbxt_check_frame(scene, err, false, "max2009_blob_8", NULL, 8.0/30.0);
	ufbxt_check_frame(scene, err, false, "max2009_blob_18", NULL, 18.0/30.0);
}
#endif

UFBXT_FILE_TEST(max7_skin)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, false, "max7_skin_5", NULL, 5.0/30.0);
	ufbxt_check_frame(scene, err, false, "max7_skin_15", NULL, 15.0/30.0);
}
#endif

UFBXT_FILE_TEST(max7_blend_cube)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Box01");
	ufbxt_assert(node);
	ufbxt_assert(node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->blend_deformers.count == 1);
	ufbx_blend_deformer *blend = mesh->blend_deformers.data[0];
	ufbxt_assert(blend->channels.count == 2);

	ufbxt_check_frame(scene, err, false, "max7_blend_cube_8", NULL, 8.0/30.0);
	ufbxt_check_frame(scene, err, false, "max7_blend_cube_24", NULL, 24.0/30.0);
}
#endif

UFBXT_FILE_TEST(max6_teapot)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Teapot01");
	ufbxt_assert(node);
	ufbxt_assert(node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->vertex_normal.exists);
	ufbxt_assert(mesh->vertex_uv.exists);
}
#endif

UFBXT_FILE_TEST(synthetic_legacy_nonzero_material)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Box01");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->faces.count == 1);
	ufbxt_assert(mesh->num_indices == 3);
	ufbxt_assert(mesh->materials.count == 2);
	ufbxt_assert(!strcmp(mesh->materials.data[0]->name.data, "Right"));
	ufbxt_assert(mesh->material_parts.data[0].num_faces == 0);
	ufbxt_assert(!strcmp(mesh->materials.data[1]->name.data, "Left"));
	ufbxt_assert(mesh->material_parts.data[1].num_faces == 1);
	ufbxt_assert(mesh->face_material.count == 1);
	ufbxt_assert(mesh->face_material.data[0] == 1);
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_legacy_unquoted_child_fail, UFBXT_FILE_TEST_FLAG_ALLOW_ERROR)
#if UFBXT_IMPL
{
	ufbxt_assert(!scene);
	ufbxt_assert(load_error);
}
#endif

UFBXT_FILE_TEST(max2009_cube_texture)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->videos.count == 1);
	ufbx_video *video = scene->videos.data[0];
	ufbxt_assert(!strcmp(video->relative_filename.data, "..\\..\\tiny_clouds.png"));
	ufbxt_assert(!strcmp(video->absolute_filename.data, "C:\\Documents and Settings\\XP3\\My Documents\\tiny_clouds.png"));
	if (scene->metadata.version >= 6000) {
		ufbxt_assert(!strcmp(video->name.data, "Clouds"));
	} else {
		ufbxt_assert(!strcmp(video->name.data, "tiny_clouds"));
	}
	if (!scene->metadata.ascii) {
		ufbxt_check_blob_content(video->content, "textures/tiny_clouds.png");
	}

	if (scene->metadata.version >= 6000) {
		ufbxt_assert(scene->textures.count == 1);
		ufbx_texture *texture = scene->textures.data[0];
		ufbxt_assert(!strcmp(texture->name.data, "Clouds"));
		if (!scene->metadata.ascii) {
			ufbxt_check_blob_content(texture->content, "textures/tiny_clouds.png");
		}
	}

	ufbx_node *node = ufbx_find_node(scene, "Box01");
	ufbxt_assert(node);
	ufbxt_assert(node->mesh);
	ufbxt_assert(node->materials.count == 1);

	ufbx_material *material = node->materials.data[0];
	ufbxt_assert(!strcmp(material->name.data, "Clouds"));

	// The 5800 file does not seem to link the video to the material in any way??
	if (scene->metadata.version >= 6000) {
		ufbxt_assert(material->fbx.diffuse_color.texture);
	}
}
#endif

UFBXT_FILE_TEST(max2009_cube_anim)
#if UFBXT_IMPL
{
	ufbxt_check_frame(scene, err, false, "max2009_cube_anim_15", NULL, 15.0/30.0);
	ufbxt_check_frame(scene, err, false, "max2009_cube_anim_45", NULL, 45.0/30.0);

	ufbxt_assert(scene->settings.frames_per_second == 30.0);

	{
		ufbx_anim_stack *stack = ufbx_find_anim_stack(scene, "Take 001");
		ufbxt_assert(stack);

		ufbxt_assert_close_double(err, stack->time_begin, 0.0 / 30.0);
		ufbxt_assert_close_double(err, stack->time_end, 60.0 / 30.0);

		ufbxt_assert(ufbx_find_int(&stack->props, "LocalStart", INT64_MIN) == INT64_C(0));
		ufbxt_assert(ufbx_find_int(&stack->props, "LocalStop", INT64_MIN) == INT64_C(92372316000));
		ufbxt_assert(ufbx_find_int(&stack->props, "ReferenceStart", INT64_MIN) == INT64_C(0));
		ufbxt_assert(ufbx_find_int(&stack->props, "ReferenceStop", INT64_MIN) == INT64_C(92372316000));
	}
}
#endif
