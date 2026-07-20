#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "mesh"

UFBXT_FILE_TEST(blender_279_default)
#if UFBXT_IMPL
{
	if (scene->metadata.ascii) {
		ufbxt_assert(scene->metadata.exporter == UFBX_EXPORTER_BLENDER_ASCII);
		ufbxt_assert(scene->metadata.exporter_version == ufbx_pack_version(2, 79, 0));
	} else {
		ufbxt_assert(scene->metadata.exporter == UFBX_EXPORTER_BLENDER_BINARY);
		ufbxt_assert(scene->metadata.exporter_version == ufbx_pack_version(3, 7, 13));
	}

	ufbx_node *node = ufbx_find_node(scene, "Lamp");
	ufbxt_assert(node);
	ufbx_light *light = node->light;
	ufbxt_assert(light);

	// Light attribute properties
	ufbx_vec3 color_ref = { 1.0, 1.0, 1.0 };
	ufbx_prop *color = ufbx_find_prop(&light->props, "Color");
	ufbxt_assert(color && color->type == UFBX_PROP_COLOR);
	ufbxt_assert_close_vec3(err, color->value_vec3, color_ref);

	ufbx_prop *intensity = ufbx_find_prop(&light->props, "Intensity");
	ufbxt_assert(intensity && intensity->type == UFBX_PROP_NUMBER);
	ufbxt_assert_close_real(err, intensity->value_real, 100.0);

	// Model properties
	ufbx_vec3 translation_ref = { 4.076245307922363, 5.903861999511719, -1.0054539442062378 };
	ufbx_prop *translation = ufbx_find_prop(&node->props, "Lcl Translation");
	ufbxt_assert(translation && translation->type == UFBX_PROP_TRANSLATION);
	ufbxt_assert_close_vec3(err, translation->value_vec3, translation_ref);

	// Model defaults
	ufbx_vec3 scaling_ref = { 1.0, 1.0, 1.0 };
	ufbx_prop *scaling = ufbx_find_prop(&node->props, "GeometricScaling");
	ufbxt_assert(scaling && scaling->type == UFBX_PROP_VECTOR);
	ufbxt_assert_close_vec3(err, scaling->value_vec3, scaling_ref);
}
#endif

UFBXT_FILE_TEST(blender_282_suzanne)
#if UFBXT_IMPL
{
}
#endif

UFBXT_FILE_TEST(blender_282_suzanne_and_transform)
#if UFBXT_IMPL
{
}
#endif

UFBXT_FILE_TEST(maya_cube)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->metadata.exporter == UFBX_EXPORTER_FBX_SDK);
	ufbxt_assert(scene->metadata.exporter_version == ufbx_pack_version(2019, 2, 0));

	ufbxt_assert(!strcmp(scene->metadata.original_application.vendor.data, "Autodesk"));
	ufbxt_assert(!strcmp(scene->metadata.original_application.name.data, "Maya"));
	ufbxt_assert(!strcmp(scene->metadata.original_application.version.data, "201900"));
	ufbxt_assert(!strcmp(scene->metadata.latest_application.vendor.data, "Autodesk"));
	ufbxt_assert(!strcmp(scene->metadata.latest_application.name.data, "Maya"));
	ufbxt_assert(!strcmp(scene->metadata.latest_application.version.data, "201900"));

	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(!mesh->generated_normals);

	for (size_t face_i = 0; face_i < mesh->num_faces; face_i++) {
		ufbx_face face = mesh->faces.data[face_i];
		for (size_t i = face.index_begin; i < face.index_begin + face.num_indices; i++) {
			ufbx_vec3 n = ufbx_get_vertex_vec3(&mesh->vertex_normal, i);
			ufbx_vec3 b = ufbx_get_vertex_vec3(&mesh->vertex_bitangent, i);
			ufbx_vec3 t = ufbx_get_vertex_vec3(&mesh->vertex_tangent, i);
			ufbxt_assert_close_real(err, ufbxt_dot3(n, n), 1.0);
			ufbxt_assert_close_real(err, ufbxt_dot3(b, b), 1.0);
			ufbxt_assert_close_real(err, ufbxt_dot3(t, t), 1.0);
			ufbxt_assert_close_real(err, ufbxt_dot3(n, b), 0.0);
			ufbxt_assert_close_real(err, ufbxt_dot3(n, t), 0.0);
			ufbxt_assert_close_real(err, ufbxt_dot3(b, t), 0.0);

			for (size_t j = 0; j < face.num_indices; j++) {
				ufbx_vec3 p0 = ufbx_get_vertex_vec3(&mesh->vertex_position, face.index_begin + j);
				ufbx_vec3 p1 = ufbx_get_vertex_vec3(&mesh->vertex_position, face.index_begin + (j + 1) % face.num_indices);
				ufbx_vec3 edge;
				edge.x = p1.x - p0.x;
				edge.y = p1.y - p0.y;
				edge.z = p1.z - p0.z;
				ufbxt_assert_close_real(err, ufbxt_dot3(edge, edge), 1.0);
				ufbxt_assert_close_real(err, ufbxt_dot3(n, edge), 0.0);
			}

		}
	}
}
#endif

UFBXT_FILE_TEST(maya_color_sets)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->color_sets.count == 4);
	ufbxt_assert(!strcmp(mesh->color_sets.data[0].name.data, "RGBCube"));
	ufbxt_assert(!strcmp(mesh->color_sets.data[1].name.data, "White"));
	ufbxt_assert(!strcmp(mesh->color_sets.data[2].name.data, "Black"));
	ufbxt_assert(!strcmp(mesh->color_sets.data[3].name.data, "Alpha"));

	for (size_t i = 0; i < mesh->num_indices; i++) {
		ufbx_vec3 pos = ufbx_get_vertex_vec3(&mesh->vertex_position, i);
		ufbx_vec4 refs[4] = {
			{ 0.0, 0.0, 0.0, 1.0 },
			{ 1.0, 1.0, 1.0, 1.0 },
			{ 0.0, 0.0, 0.0, 1.0 },
			{ 1.0, 1.0, 1.0, 0.0 },
		};

		refs[0].x = pos.x + 0.5f;
		refs[0].y = pos.y + 0.5f;
		refs[0].z = pos.z + 0.5f;
		refs[3].w = (pos.x + 0.5f) * 0.1f + (pos.y + 0.5f) * 0.2f + (pos.z + 0.5f) * 0.4f;

		for (size_t set_i = 0; set_i < 4; set_i++) {
			ufbx_vec4 color = ufbx_get_vertex_vec4(&mesh->color_sets.data[set_i].vertex_color, i);
			ufbxt_assert_close_vec4(err, color, refs[set_i]);
		}
	}
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_flip_winding_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.reverse_winding = true;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(maya_color_sets_winding, maya_color_sets, ufbxt_flip_winding_opts, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->color_sets.count == 4);
	ufbxt_assert(!strcmp(mesh->color_sets.data[0].name.data, "RGBCube"));
	ufbxt_assert(!strcmp(mesh->color_sets.data[1].name.data, "White"));
	ufbxt_assert(!strcmp(mesh->color_sets.data[2].name.data, "Black"));
	ufbxt_assert(!strcmp(mesh->color_sets.data[3].name.data, "Alpha"));

	for (size_t i = 0; i < mesh->num_indices; i++) {
		ufbx_vec3 pos = ufbx_get_vertex_vec3(&mesh->vertex_position, i);
		ufbx_vec4 refs[4] = {
			{ 0.0, 0.0, 0.0, 1.0 },
			{ 1.0, 1.0, 1.0, 1.0 },
			{ 0.0, 0.0, 0.0, 1.0 },
			{ 1.0, 1.0, 1.0, 0.0 },
		};

		refs[0].x = pos.x + 0.5f;
		refs[0].y = pos.y + 0.5f;
		refs[0].z = pos.z + 0.5f;
		refs[3].w = (pos.x + 0.5f) * 0.1f + (pos.y + 0.5f) * 0.2f + (pos.z + 0.5f) * 0.4f;

		for (size_t set_i = 0; set_i < 4; set_i++) {
			ufbx_vec4 color = ufbx_get_vertex_vec4(&mesh->color_sets.data[set_i].vertex_color, i);
			ufbxt_assert_close_vec4(err, color, refs[set_i]);
		}
	}
}
#endif

UFBXT_FILE_TEST(maya_uv_sets)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->uv_sets.count == 3);
	ufbxt_assert(!strcmp(mesh->uv_sets.data[0].name.data, "Default"));
	ufbxt_assert(!strcmp(mesh->uv_sets.data[1].name.data, "PerFace"));
	ufbxt_assert(!strcmp(mesh->uv_sets.data[2].name.data, "Row"));

	size_t counts1[2][2] = { 0 };
	size_t counts2[7][2] = { 0 };

	for (size_t i = 0; i < mesh->num_indices; i++) {
		ufbx_vec2 uv0 = ufbx_get_vertex_vec2(&mesh->uv_sets.data[0].vertex_uv, i);
		ufbx_vec2 uv1 = ufbx_get_vertex_vec2(&mesh->uv_sets.data[1].vertex_uv, i);
		ufbx_vec2 uv2 = ufbx_get_vertex_vec2(&mesh->uv_sets.data[2].vertex_uv, i);

		ufbxt_assert(uv0.x > 0.05f && uv0.y > 0.05f && uv0.x < 0.95f && uv0.y < 0.95f);
		int x1 = (int)(uv1.x + 0.5f), y1 = (int)(uv1.y + 0.5f);
		int x2 = (int)(uv2.x + 0.5f), y2 = (int)(uv2.y + 0.5f);
		ufbxt_assert_close_real(err, uv1.x - (ufbx_real)x1, 0.0);
		ufbxt_assert_close_real(err, uv1.y - (ufbx_real)y1, 0.0);
		ufbxt_assert_close_real(err, uv2.x - (ufbx_real)x2, 0.0);
		ufbxt_assert_close_real(err, uv2.y - (ufbx_real)y2, 0.0);
		ufbxt_assert(x1 >= 0 && x1 <= 1 && y1 >= 0 && y1 <= 1);
		ufbxt_assert(x2 >= 0 && x2 <= 6 && y2 >= 0 && y2 <= 1);
		counts1[x1][y1]++;
		counts2[x2][y2]++;
	}

	ufbxt_assert(counts1[0][0] == 6);
	ufbxt_assert(counts1[0][1] == 6);
	ufbxt_assert(counts1[1][0] == 6);
	ufbxt_assert(counts1[1][1] == 6);

	for (size_t i = 0; i < 7; i++) {
		size_t n = (i == 0 || i == 6) ? 1 : 2;
		ufbxt_assert(counts2[i][0] == n);
		ufbxt_assert(counts2[i][1] == n);
	}
}
#endif

UFBXT_FILE_TEST(blender_279_color_sets)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Cube");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->color_sets.count == 3);
	ufbxt_assert(!strcmp(mesh->color_sets.data[0].name.data, "RGBCube"));
	ufbxt_assert(!strcmp(mesh->color_sets.data[1].name.data, "White"));
	ufbxt_assert(!strcmp(mesh->color_sets.data[2].name.data, "Black"));

	for (size_t i = 0; i < mesh->num_indices; i++) {
		ufbx_vec3 pos = ufbx_get_vertex_vec3(&mesh->vertex_position, i);
		ufbx_vec4 refs[3] = {
			{ 0.0, 0.0, 0.0, 1.0 },
			{ 1.0, 1.0, 1.0, 1.0 },
			{ 0.0, 0.0, 0.0, 1.0 },
		};

		refs[0].x = pos.x + 0.5f;
		refs[0].y = pos.y + 0.5f;
		refs[0].z = pos.z + 0.5f;

		for (size_t set_i = 0; set_i < 3; set_i++) {
			ufbx_vec4 color = ufbx_get_vertex_vec4(&mesh->color_sets.data[set_i].vertex_color, i);
			ufbxt_assert_close_vec4(err, color, refs[set_i]);
		}
	}
}
#endif

UFBXT_FILE_TEST(blender_279_uv_sets)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Cube");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->uv_sets.count == 3);
	ufbxt_assert(!strcmp(mesh->uv_sets.data[0].name.data, "Default"));
	ufbxt_assert(!strcmp(mesh->uv_sets.data[1].name.data, "PerFace"));
	ufbxt_assert(!strcmp(mesh->uv_sets.data[2].name.data, "Row"));

	size_t counts1[2][2] = { 0 };
	size_t counts2[7][2] = { 0 };

	for (size_t i = 0; i < mesh->num_indices; i++) {
		ufbx_vec2 uv0 = ufbx_get_vertex_vec2(&mesh->uv_sets.data[0].vertex_uv, i);
		ufbx_vec2 uv1 = ufbx_get_vertex_vec2(&mesh->uv_sets.data[1].vertex_uv, i);
		ufbx_vec2 uv2 = ufbx_get_vertex_vec2(&mesh->uv_sets.data[2].vertex_uv, i);

		ufbxt_assert(uv0.x > 0.05f && uv0.y > 0.05f && uv0.x < 0.95f && uv0.y < 0.95f);
		int x1 = (int)(uv1.x + 0.5f), y1 = (int)(uv1.y + 0.5f);
		int x2 = (int)(uv2.x + 0.5f), y2 = (int)(uv2.y + 0.5f);
		ufbxt_assert_close_real(err, uv1.x - (ufbx_real)x1, 0.0);
		ufbxt_assert_close_real(err, uv1.y - (ufbx_real)y1, 0.0);
		ufbxt_assert_close_real(err, uv2.x - (ufbx_real)x2, 0.0);
		ufbxt_assert_close_real(err, uv2.y - (ufbx_real)y2, 0.0);
		ufbxt_assert(x1 >= 0 && x1 <= 1 && y1 >= 0 && y1 <= 1);
		ufbxt_assert(x2 >= 0 && x2 <= 6 && y2 >= 0 && y2 <= 1);
		counts1[x1][y1]++;
		counts2[x2][y2]++;
	}

	ufbxt_assert(counts1[0][0] == 6);
	ufbxt_assert(counts1[0][1] == 6);
	ufbxt_assert(counts1[1][0] == 6);
	ufbxt_assert(counts1[1][1] == 6);

	for (size_t i = 0; i < 7; i++) {
		size_t n = (i == 0 || i == 6) ? 1 : 2;
		ufbxt_assert(counts2[i][0] == n);
		ufbxt_assert(counts2[i][1] == n);
	}
}
#endif

UFBXT_FILE_TEST(synthetic_sets_reorder)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->color_sets.count == 4);
	ufbxt_assert(!strcmp(mesh->color_sets.data[0].name.data, "RGBCube"));
	ufbxt_assert(!strcmp(mesh->color_sets.data[1].name.data, "White"));
	ufbxt_assert(!strcmp(mesh->color_sets.data[2].name.data, "Black"));
	ufbxt_assert(!strcmp(mesh->color_sets.data[3].name.data, "Alpha"));
	ufbxt_assert(!strcmp(mesh->uv_sets.data[0].name.data, "Default"));
	ufbxt_assert(!strcmp(mesh->uv_sets.data[1].name.data, "PerFace"));
	ufbxt_assert(!strcmp(mesh->uv_sets.data[2].name.data, "Row"));
}
#endif

UFBXT_FILE_TEST(maya_cone)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCone1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->vertex_crease.exists);
	ufbxt_assert(mesh->edges.count);
	ufbxt_assert(mesh->edge_crease.count);
	ufbxt_assert(mesh->edge_smoothing.count);

	ufbxt_assert(mesh->faces.data[0].num_indices == 16);

	for (size_t i = 0; i < mesh->num_indices; i++) {
		ufbx_vec3 pos = ufbx_get_vertex_vec3(&mesh->vertex_position, i);
		ufbx_real crease = ufbx_get_vertex_real(&mesh->vertex_crease, i);

		ufbxt_assert_close_real(err, crease, pos.y > 0.0f ? 0.998f : 0.0f);
	}

	for (size_t i = 0; i < mesh->num_edges; i++) {
		ufbx_edge edge = mesh->edges.data[i];
		ufbx_real crease = mesh->edge_crease.data[i];
		bool smoothing = mesh->edge_smoothing.data[i];
		ufbx_vec3 a = ufbx_get_vertex_vec3(&mesh->vertex_position, edge.a);
		ufbx_vec3 b = ufbx_get_vertex_vec3(&mesh->vertex_position, edge.b);

		if (a.y < 0.0 && b.y < 0.0) {
			ufbxt_assert_close_real(err, crease, 0.583f);
			ufbxt_assert(!smoothing);
		} else {
			ufbxt_assert(a.y > 0.0 || b.y > 0.0);
			ufbxt_assert_close_real(err, crease, 0.0f);
			ufbxt_assert(smoothing);
		}
	}
}
#endif

#if UFBXT_IMPL
static void ufbxt_check_tangent_space(ufbxt_diff_error *err, ufbx_mesh *mesh)
{
	for (size_t set_i = 0; set_i < mesh->uv_sets.count; set_i++) {
		ufbx_uv_set set = mesh->uv_sets.data[set_i];
		ufbxt_assert(set.vertex_uv.exists);
		ufbxt_assert(set.vertex_bitangent.exists);
		ufbxt_assert(set.vertex_tangent.exists);

		for (size_t face_i = 0; face_i < mesh->num_faces; face_i++) {
			ufbx_face face = mesh->faces.data[face_i];

			for (size_t i = 0; i < face.num_indices; i++) {
				size_t a = face.index_begin + i;
				size_t b = face.index_begin + (i + 1) % face.num_indices;

				ufbx_vec3 pa = ufbx_get_vertex_vec3(&mesh->vertex_position, a);
				ufbx_vec3 pb = ufbx_get_vertex_vec3(&mesh->vertex_position, b);
				ufbx_vec3 ba = ufbx_get_vertex_vec3(&set.vertex_bitangent, a);
				ufbx_vec3 bb = ufbx_get_vertex_vec3(&set.vertex_bitangent, b);
				ufbx_vec3 ta = ufbx_get_vertex_vec3(&set.vertex_tangent, a);
				ufbx_vec3 tb = ufbx_get_vertex_vec3(&set.vertex_tangent, b);
				ufbx_vec2 ua = ufbx_get_vertex_vec2(&set.vertex_uv, a);
				ufbx_vec2 ub = ufbx_get_vertex_vec2(&set.vertex_uv, b);

				ufbx_vec3 dp = ufbxt_sub3(pb, pa);
				ufbx_vec2 du = ufbxt_sub2(ua, ub);

				ufbx_real dp_len = (ufbx_real)sqrt(ufbxt_dot3(dp, dp));
				dp.x /= dp_len;
				dp.y /= dp_len;
				dp.z /= dp_len;

				ufbx_real du_len = (ufbx_real)sqrt(ufbxt_dot2(du, du));
				du.x /= du_len;
				du.y /= du_len;

				ufbx_real dba = ufbxt_dot3(dp, ba);
				ufbx_real dbb = ufbxt_dot3(dp, bb);
				ufbx_real dta = ufbxt_dot3(dp, ta);
				ufbx_real dtb = ufbxt_dot3(dp, tb);
				ufbxt_assert_close_real(err, dba, dbb);
				ufbxt_assert_close_real(err, dta, dtb);

				ufbxt_assert_close_real(err, ub.x - ua.x, dta);
				ufbxt_assert_close_real(err, ub.y - ua.y, dba);
			}
		}
	}
}
#endif

UFBXT_FILE_TEST(maya_uv_set_tangents)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pPlane1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_check_tangent_space(err, mesh);
}
#endif

UFBXT_FILE_TEST(blender_279_uv_set_tangents)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Plane");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_check_tangent_space(err, mesh);
}
#endif

UFBXT_FILE_TEST(synthetic_tangents_reorder)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pPlane1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_check_tangent_space(err, mesh);
}
#endif

UFBXT_FILE_TEST(blender_279_ball)
#if UFBXT_IMPL
{
	ufbx_material *red = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Red");
	ufbx_material *white = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "White");
	ufbxt_assert(red && !strcmp(red->name.data, "Red"));
	ufbxt_assert(white && !strcmp(white->name.data, "White"));

	ufbx_vec3 red_ref = { 0.8f, 0.0f, 0.0f };
	ufbx_vec3 white_ref = { 0.8f, 0.8f, 0.8f };
	ufbxt_assert_close_vec3(err, red->fbx.diffuse_color.value_vec3, red_ref);
	ufbxt_assert_close_vec3(err, white->fbx.diffuse_color.value_vec3, white_ref);

	ufbx_node *node = ufbx_find_node(scene, "Icosphere");
	ufbxt_assert(node);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh);
	ufbxt_assert(mesh->face_material.count);
	ufbxt_assert(mesh->face_smoothing.count);

	ufbxt_assert(mesh->materials.count == 2);
	ufbxt_assert(mesh->materials.data[0] == red);
	ufbxt_assert(mesh->materials.data[1] == white);

	for (size_t face_i = 0; face_i < mesh->num_faces; face_i++) {
		ufbx_face face = mesh->faces.data[face_i];
		ufbx_vec3 mid = { 0 };
		for (size_t i = 0; i < face.num_indices; i++) {
			mid = ufbxt_add3(mid, ufbx_get_vertex_vec3(&mesh->vertex_position, face.index_begin + i));
		}
		mid.x /= (ufbx_real)face.num_indices;
		mid.y /= (ufbx_real)face.num_indices;
		mid.z /= (ufbx_real)face.num_indices;

		bool smoothing = mesh->face_smoothing.data[face_i];
		int32_t material = mesh->face_material.data[face_i];
		ufbxt_assert(smoothing == (mid.x > 0.0));
		ufbxt_assert(material == (mid.z < 0.0 ? 1 : 0));
	}
}
#endif

UFBXT_FILE_TEST(synthetic_broken_material)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->materials.count == 0);
	ufbxt_assert(mesh->face_material.data == NULL);
}
#endif

UFBXT_FILE_TEST(maya_uv_and_color_sets)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->uv_sets.count == 2);
	ufbxt_assert(mesh->color_sets.count == 2);
	ufbxt_assert(!strcmp(mesh->uv_sets.data[0].name.data, "UVA"));
	ufbxt_assert(!strcmp(mesh->uv_sets.data[1].name.data, "UVB"));
	ufbxt_assert(!strcmp(mesh->color_sets.data[0].name.data, "ColorA"));
	ufbxt_assert(!strcmp(mesh->color_sets.data[1].name.data, "ColorB"));
}
#endif

UFBXT_FILE_TEST(maya_bad_face)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_empty_faces == 0);
	ufbxt_assert(mesh->num_point_faces == 1);
	ufbxt_assert(mesh->num_line_faces == 1);
	ufbxt_assert(mesh->num_triangles == 7);
	ufbxt_assert(mesh->num_faces == 6);

	ufbxt_assert(mesh->faces.data[0].num_indices == 1);
	ufbxt_assert(mesh->faces.data[1].num_indices == 2);
	ufbxt_assert(mesh->faces.data[2].num_indices == 3);
	ufbxt_assert(mesh->faces.data[3].num_indices == 4);

	ufbxt_assert(mesh->faces.data[0].index_begin == 0);
	ufbxt_assert(mesh->faces.data[1].index_begin == 1);
	ufbxt_assert(mesh->faces.data[2].index_begin == 3);
	ufbxt_assert(mesh->faces.data[3].index_begin == 6);

	// ??? Maya exports an edge for the single point
	ufbxt_assert(mesh->num_edges == 12);
	ufbxt_assert(mesh->edges.data[0].a == 0);
	ufbxt_assert(mesh->edges.data[0].b == 0);
	ufbxt_assert(mesh->edges.data[1].a == 2);
	ufbxt_assert(mesh->edges.data[1].b == 1);
}
#endif

UFBXT_FILE_TEST(blender_279_edge_vertex)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Plane");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_empty_faces == 0);
	ufbxt_assert(mesh->num_point_faces == 0);
	ufbxt_assert(mesh->num_line_faces == 1);
	ufbxt_assert(mesh->num_triangles == 0);
	ufbxt_assert(mesh->num_faces == 1);

	ufbxt_assert(mesh->faces.data[0].index_begin == 0);
	ufbxt_assert(mesh->faces.data[0].num_indices == 2);

	// ??? Maya exports an edge for the single point
	if (scene->metadata.version == 7400) {
		ufbxt_assert(mesh->num_edges == 1);
		ufbxt_assert(mesh->edges.data[0].a == 0);
		ufbxt_assert(mesh->edges.data[0].b == 1);
	} else {
		// 6100 has an edge for both directions
		ufbxt_assert(mesh->num_edges == 2);
		ufbxt_assert(mesh->edges.data[0].a == 0);
		ufbxt_assert(mesh->edges.data[0].b == 1);
		ufbxt_assert(mesh->edges.data[1].a == 1);
		ufbxt_assert(mesh->edges.data[1].b == 0);
	}
}
#endif

UFBXT_FILE_TEST(blender_279_edge_circle)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Circle");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_indices == 512);

	// ??? The 7400_binary export starts off with individual edges but has a
	// massive N-gon for the rest of it.
	if (scene->metadata.version == 7400) {
		ufbxt_assert(mesh->num_line_faces == 127);
		ufbxt_assert(mesh->num_faces == 128);

		for (size_t i = 0; i < 127; i++) {
			ufbxt_assert(mesh->faces.data[i].num_indices == 2);
		}
		ufbxt_assert(mesh->faces.data[127].num_indices == 258);

		ufbxt_assert(mesh->num_edges == 256);
	} else {
		ufbxt_assert(mesh->num_line_faces == 256);
		ufbxt_assert(mesh->num_faces == 256);

		for (size_t i = 0; i < 256; i++) {
			ufbxt_assert(mesh->faces.data[i].num_indices == 2);
		}

		// 6100 has an edge for both directions
		ufbxt_assert(mesh->num_edges == 512);
	}
}
#endif

UFBXT_FILE_TEST(blender_293_instancing)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->meshes.count == 1);
	ufbx_mesh *mesh = scene->meshes.data[0];
	ufbxt_assert(mesh->instances.count == 8);
}
#endif

UFBXT_FILE_TEST(synthetic_indexed_by_vertex)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->meshes.count == 1);
	ufbx_mesh *mesh = scene->meshes.data[0];

	for (size_t vi = 0; vi < mesh->num_vertices; vi++) {
		int32_t ii = mesh->vertex_first_index.data[vi];
		ufbx_vec3 pos = mesh->vertex_position.values.data[mesh->vertex_position.indices.data[ii]];
		ufbx_vec3 normal = mesh->vertex_normal.values.data[mesh->vertex_normal.indices.data[ii]];
		ufbx_vec3 ref_normal = { 0.0f, pos.y > 0.0f ? 1.0f : -1.0f, 0.0f };
		ufbxt_assert_close_vec3(err, normal, ref_normal);
	}

	for (size_t ii = 0; ii < mesh->num_indices; ii++) {
		ufbx_vec3 pos = mesh->vertex_position.values.data[mesh->vertex_position.indices.data[ii]];
		ufbx_vec3 normal = mesh->vertex_normal.values.data[mesh->vertex_normal.indices.data[ii]];
		ufbx_vec3 ref_normal = { 0.0f, pos.y > 0.0f ? 1.0f : -1.0f, 0.0f };
		ufbxt_assert_close_vec3(err, normal, ref_normal);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_by_vertex_bad_index, UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbxt_check_warning(scene, UFBX_WARNING_INDEX_CLAMPED, UFBX_ELEMENT_MESH, "#pCube1", 9, NULL);

	ufbxt_assert(scene->meshes.count == 1);
	ufbx_mesh *mesh = scene->meshes.data[0];

	for (size_t vi = 0; vi < mesh->num_vertices; vi++) {
		int32_t ii = mesh->vertex_first_index.data[vi];
		ufbx_vec3 pos = mesh->vertex_position.values.data[mesh->vertex_position.indices.data[ii]];
		ufbx_vec3 normal = mesh->vertex_normal.values.data[mesh->vertex_normal.indices.data[ii]];
		ufbx_vec3 ref_normal = { 0.0f, pos.y > 0.0f ? 1.0f : -1.0f, 0.0f };
		ufbxt_assert_close_vec3(err, normal, ref_normal);
	}

	for (size_t ii = 0; ii < mesh->num_indices; ii++) {
		ufbx_vec3 pos = mesh->vertex_position.values.data[mesh->vertex_position.indices.data[ii]];
		ufbx_vec3 normal = mesh->vertex_normal.values.data[mesh->vertex_normal.indices.data[ii]];
		ufbx_vec3 ref_normal = { 0.0f, pos.y > 0.0f ? 1.0f : -1.0f, 0.0f };
		ufbxt_assert_close_vec3(err, normal, ref_normal);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_by_vertex_overflow, UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbxt_check_warning(scene, UFBX_WARNING_INDEX_CLAMPED, UFBX_ELEMENT_MESH, "#pCube1", 12, NULL);

	ufbxt_assert(scene->meshes.count == 1);
	ufbx_mesh *mesh = scene->meshes.data[0];

	ufbxt_assert(mesh->vertex_normal.values.count == 4);
	for (size_t ii = 0; ii < mesh->num_indices; ii++) {
		uint32_t vertex_ix = mesh->vertex_position.indices.data[ii];
		uint32_t normal_ix = mesh->vertex_normal.indices.data[ii];
		if (vertex_ix < mesh->vertex_normal.values.count) {
			ufbxt_assert(normal_ix == vertex_ix);
		} else {
			ufbxt_assert(normal_ix == (uint32_t)mesh->vertex_normal.values.count - 1);
		}
	}
}
#endif

UFBXT_FILE_TEST(maya_lod_group)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "LOD_Group_1");
		ufbxt_assert(node);
		ufbxt_assert(node->attrib_type == UFBX_ELEMENT_LOD_GROUP);
		ufbx_lod_group *lod_group = (ufbx_lod_group*)node->attrib;
		ufbxt_assert(lod_group->element.type == UFBX_ELEMENT_LOD_GROUP);

		ufbxt_assert(node->children.count == 3);
		ufbxt_assert(lod_group->lod_levels.count == 3);

		ufbxt_assert(lod_group->relative_distances);
		ufbxt_assert(!lod_group->use_distance_limit);
		ufbxt_assert(!lod_group->ignore_parent_transform);

		ufbxt_assert(lod_group->lod_levels.data[0].display == UFBX_LOD_DISPLAY_USE_LOD);
		ufbxt_assert(lod_group->lod_levels.data[1].display == UFBX_LOD_DISPLAY_USE_LOD);
		ufbxt_assert(lod_group->lod_levels.data[2].display == UFBX_LOD_DISPLAY_USE_LOD);

		if (scene->metadata.version >= 7000) {
			ufbxt_assert_close_real(err, lod_group->lod_levels.data[0].distance, 100.0f);
			ufbxt_assert_close_real(err, lod_group->lod_levels.data[1].distance, 64.0f);
			ufbxt_assert_close_real(err, lod_group->lod_levels.data[2].distance, 32.0f);
		}
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "LOD_Group_2");
		ufbxt_assert(node);
		ufbxt_assert(node->attrib_type == UFBX_ELEMENT_LOD_GROUP);
		ufbx_lod_group *lod_group = (ufbx_lod_group*)node->attrib;
		ufbxt_assert(lod_group->element.type == UFBX_ELEMENT_LOD_GROUP);

		ufbxt_assert(node->children.count == 3);
		ufbxt_assert(lod_group->lod_levels.count == 3);

		ufbxt_assert(!lod_group->relative_distances);
		ufbxt_assert(!lod_group->use_distance_limit);
		ufbxt_assert(lod_group->ignore_parent_transform);

		ufbxt_assert(lod_group->lod_levels.data[0].display == UFBX_LOD_DISPLAY_USE_LOD);
		ufbxt_assert(lod_group->lod_levels.data[1].display == UFBX_LOD_DISPLAY_SHOW);
		ufbxt_assert(lod_group->lod_levels.data[2].display == UFBX_LOD_DISPLAY_HIDE);

		if (scene->metadata.version >= 7000) {
			ufbxt_assert_close_real(err, lod_group->lod_levels.data[0].distance, 0.0f);
			ufbxt_assert_close_real(err, lod_group->lod_levels.data[1].distance, 4.520276f);
			ufbxt_assert_close_real(err, lod_group->lod_levels.data[2].distance, 18.081102f);
		}
	}
}
#endif

UFBXT_FILE_TEST(synthetic_missing_normals)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(!mesh->vertex_normal.exists);
	ufbxt_assert(!mesh->skinned_normal.exists);
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_generate_normals_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.generate_missing_normals = true;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(synthetic_missing_normals_generated, synthetic_missing_normals, ufbxt_generate_normals_opts, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->vertex_normal.exists);
	ufbxt_assert(mesh->skinned_normal.exists);
	ufbxt_assert(mesh->generated_normals);

	for (size_t face_ix = 0; face_ix < mesh->faces.count; face_ix++) {
		ufbx_face face = mesh->faces.data[face_ix];
		ufbx_vec3 normal = ufbx_get_weighted_face_normal(&mesh->vertex_position, face);
		normal = ufbxt_normalize(normal);
		for (size_t i = 0; i < face.num_indices; i++) {
			ufbx_vec3 mesh_normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, face.index_begin + i);
			ufbx_vec3 skinned_normal = ufbx_get_vertex_vec3(&mesh->skinned_normal, face.index_begin + i);
			ufbxt_assert_close_vec3(err, normal, mesh_normal);
			ufbxt_assert_close_vec3(err, normal, skinned_normal);
		}
	}
}
#endif

UFBXT_FILE_TEST(blender_279_nested_meshes)
#if UFBXT_IMPL
{
	// Diff to .obj file with nested objects and FBXASC escaped names
}
#endif

UFBXT_FILE_TEST(max_edge_visibility)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "Box001");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh->edge_visibility.count > 0);

		{
			size_t num_visible = 0;

			// Diagonal edges should be hidden
			for (size_t i = 0; i < mesh->num_edges; i++) {
				ufbx_edge edge = mesh->edges.data[i];
				ufbx_vec3 a = ufbx_get_vertex_vec3(&mesh->vertex_position, edge.a);
				ufbx_vec3 b = ufbx_get_vertex_vec3(&mesh->vertex_position, edge.b);
				ufbx_real len = ufbxt_length3(ufbxt_sub3(a, b));
				bool expected = len < 21.0f;
				bool visible = mesh->edge_visibility.data[i];
				ufbxt_assert(visible == expected);
				num_visible += visible ? 1u : 0u;
			}

			ufbxt_assert(mesh->num_edges == 18);
			ufbxt_assert(num_visible == 12);
		}

		ufbx_mesh *sub_mesh = ufbxt_subdivide_mesh(mesh, 2, NULL, NULL);
		ufbxt_assert(sub_mesh);
		ufbxt_check_mesh(scene, sub_mesh);

		{
			size_t num_visible = 0;
			for (size_t i = 0; i < sub_mesh->num_edges; i++) {
				if (sub_mesh->edge_visibility.data[i]) {
					num_visible++;
				}
			}
			ufbxt_assert(num_visible == 12 * 4);
		}

		ufbx_free_mesh(sub_mesh);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Cylinder001");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh->edge_visibility.count > 0);
		size_t num_visible = 0;

		// Diagonal and edges to the center should be hidden
		for (size_t i = 0; i < mesh->num_edges; i++) {
			ufbx_edge edge = mesh->edges.data[i];
			ufbx_vec3 a = ufbx_get_vertex_vec3(&mesh->vertex_position, edge.a);
			ufbx_vec3 b = ufbx_get_vertex_vec3(&mesh->vertex_position, edge.b);
			ufbx_vec2 a2 = { a.x, a.y };
			ufbx_vec2 b2 = { b.x, b.y };
			ufbx_real len = ufbxt_length3(ufbxt_sub3(a, b));
			ufbx_real len_a2 = ufbxt_length2(a2);
			ufbx_real len_b2 = ufbxt_length2(b2);
			bool expected = len < 20.7f && len_a2 > 0.1f && len_b2 > 0.1f;
			bool visible = mesh->edge_visibility.data[i];
			ufbxt_assert(visible == expected);
			num_visible += visible ? 1u : 0u;
		}

		ufbxt_assert(mesh->num_edges == 54);
		ufbxt_assert(num_visible == 27);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(zbrush_d20, UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->nodes.count == 3);

	{
		ufbx_node *node = ufbx_find_node(scene, "20 Sided");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh->num_faces == 20);
		ufbxt_assert(mesh->face_group.count == 20);
		for (int32_t i = 0; i < 20; i++) {
			uint32_t group_ix = mesh->face_group.data[i];
			ufbx_face_group *group = &mesh->face_groups.data[group_ix];
			ufbxt_assert(group->id == 10 + i * 5);
		}
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "PolyMesh3D1");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh->num_faces == 24);
		ufbxt_assert(mesh->face_group.count == 24);

		{
			uint32_t num_front = 0;
			for (size_t i = 0; i < 24; i++) {
				ufbx_face face = mesh->faces.data[i];
				ufbx_vec3 normal = ufbx_get_weighted_face_normal(&mesh->vertex_position, face);
				int32_t group = normal.z < 0.0f ? 9598 : 15349;
				num_front += normal.z < 0.0f ? 1 : 0;
				uint32_t group_ix = mesh->face_group.data[i];
				ufbxt_assert(mesh->face_groups.data[group_ix].id == group);
			}
			ufbxt_assert(num_front == mesh->num_faces / 2);
		}

		ufbxt_assert(mesh->blend_deformers.count > 0);
		ufbx_blend_deformer *blend = mesh->blend_deformers.data[0];

		// WHAT? The 6100 version has duplicated blend shapes
		// and 7500 has duplicated blend deformers...
		// After e8cab0f ufbx ignores duplicated connections so we see only
		// a single blend deformer here..
		if (scene->metadata.version == 6100) {
			ufbxt_assert(mesh->blend_deformers.count == 1);
			ufbxt_assert(blend->channels.count == 4);
		} else {
			ufbxt_assert(mesh->blend_deformers.count == 1);
			ufbxt_assert(blend->channels.count == 2);

			ufbx_blend_deformer *blend_deformer = mesh->blend_deformers.data[0];
			char warning_substring[128];
			snprintf(warning_substring, sizeof(warning_substring), "to %u", mesh->element_id);
			ufbxt_check_warning_imp(scene, UFBX_WARNING_DUPLICATE_CONNECTION, blend_deformer->element_id, 1, warning_substring);
		}

		// Check that poly groups work in subdivision
		ufbx_mesh *sub_mesh = ufbxt_subdivide_mesh(mesh, 2, NULL, NULL);
		ufbxt_assert(sub_mesh);
		ufbxt_check_mesh(scene, sub_mesh);

		// Check that we didn't break the original mesh
		ufbxt_check_mesh(scene, mesh);

		{
			uint32_t num_front = 0;
			ufbxt_assert(sub_mesh->face_group.count == sub_mesh->num_faces);
			for (size_t i = 0; i < sub_mesh->num_faces; i++) {
				ufbx_face face = sub_mesh->faces.data[i];
				ufbx_vec3 normal = ufbx_get_weighted_face_normal(&sub_mesh->vertex_position, face);
				int32_t group = normal.z < 0.0f ? 9598 : 15349;
				num_front += normal.z < 0.0f ? 1 : 0;
				uint32_t group_ix = sub_mesh->face_group.data[i];
				ufbxt_assert(sub_mesh->face_groups.data[group_ix].id == group);
			}
			ufbxt_assert(num_front == sub_mesh->num_faces / 2);
		}

		ufbx_free_mesh(sub_mesh);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(zbrush_d20_selection_set, UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "PolyMesh3D1");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;

		for (size_t i = 0; i < 2; i++)
		{
			bool front = i == 0;
			const char *name = front ? "PolyMesh3D1_9598" : "PolyMesh3D1_15349";
			ufbx_selection_set *set = (ufbx_selection_set*)ufbx_find_element(scene, UFBX_ELEMENT_SELECTION_SET, name);
			ufbxt_assert(set);
			ufbxt_assert(set->nodes.count == 1);
			ufbx_selection_node *sel_node = set->nodes.data[0];

			ufbxt_assert(sel_node->target_node == node);
			ufbxt_assert(sel_node->target_mesh == mesh);
			ufbxt_assert(sel_node->faces.count == 12);

			for (size_t i = 0; i < sel_node->faces.count; i++) {
				uint32_t index = sel_node->faces.data[i];
				ufbxt_assert(index < mesh->faces.count);
				ufbx_face face = mesh->faces.data[index];
				ufbx_vec3 normal = ufbx_get_weighted_face_normal(&mesh->vertex_position, face);
				ufbxt_assert((normal.z < 0.0f) == front);
			}
		}
	}
}
#endif

UFBXT_FILE_TEST(maya_polygon_hole)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 6);

	{
		size_t num_holes = 0;
		ufbxt_assert(mesh->face_hole.count == mesh->num_faces);
		for (size_t i = 0; i < mesh->num_faces; i++) {
			ufbx_face face = mesh->faces.data[i];
			ufbx_vec3 avg_pos = ufbx_zero_vec3;
			for (size_t j = 0; j < face.num_indices; j++) {
				ufbx_vec3 p = ufbx_get_vertex_vec3(&mesh->vertex_position, face.index_begin + j);
				avg_pos = ufbxt_add3(avg_pos, p);
			}
			avg_pos = ufbxt_mul3(avg_pos, 1.0f / (ufbx_real)face.num_indices);

			bool hole = fabs(avg_pos.y) > 0.49f;
			num_holes += hole ? 1 : 0;
			ufbxt_assert(mesh->face_hole.data[i] == hole);
		}
		ufbxt_assert(num_holes == 2);
	}

	ufbx_mesh *sub_mesh = ufbxt_subdivide_mesh(mesh, 2, NULL, NULL);
	ufbxt_assert(sub_mesh);
	ufbxt_check_mesh(scene, sub_mesh);

	{
		size_t num_holes = 0;
		ufbxt_assert(sub_mesh->face_hole.count == sub_mesh->num_faces);
		for (size_t i = 0; i < sub_mesh->num_faces; i++) {
			ufbx_face face = sub_mesh->faces.data[i];
			ufbx_vec3 avg_pos = ufbx_zero_vec3;
			for (size_t j = 0; j < face.num_indices; j++) {
				ufbx_vec3 p = ufbx_get_vertex_vec3(&sub_mesh->vertex_position, face.index_begin + j);
				avg_pos = ufbxt_add3(avg_pos, p);
			}
			avg_pos = ufbxt_mul3(avg_pos, 1.0f / (ufbx_real)face.num_indices);

			bool hole = fabs(avg_pos.y) > 0.49f;
			num_holes += hole ? 1 : 0;
			ufbxt_assert(sub_mesh->face_hole.data[i] == hole);
		}
		ufbxt_assert(num_holes == 32);
	}

	ufbx_free_mesh(sub_mesh);
}
#endif

UFBXT_FILE_TEST_OPTS(synthetic_cursed_geometry, ufbxt_generate_normals_opts)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pDisc1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	uint32_t indices[64];

	for (size_t i = 0; i < mesh->num_faces; i++) {
		ufbx_face face = mesh->faces.data[i];
		size_t num_tris = ufbx_triangulate_face(indices, ufbxt_arraycount(indices), mesh, face);
		ufbxt_assert(num_tris == 0 || num_tris == face.num_indices - 2);
	}

	ufbx_mesh *sub_mesh = ufbxt_subdivide_mesh(mesh, 2, NULL, NULL);
	ufbx_free_mesh(sub_mesh);
}
#endif

UFBXT_FILE_TEST(synthetic_vertex_gaps)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pPlane1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->vertices.count == 7);

	ufbx_vec3 gap_values[] = {
		{ -1.0f, -1.1f, -1.2f },
		{ -2.0f, -2.1f, -2.2f },
		{ -3.0f, -3.1f, -3.2f },
		{ -4.0f, -4.1f, -4.2f },
	};

	ufbxt_assert_close_vec3(err, mesh->vertices.data[0], gap_values[0]);
	ufbxt_assert_close_vec3(err, mesh->vertices.data[3], gap_values[1]);
	ufbxt_assert_close_vec3(err, mesh->vertices.data[6], gap_values[2]);

	ufbxt_assert(mesh->vertex_normal.values.count == 8);
	ufbxt_assert_close_vec3(err, mesh->vertex_normal.values.data[0], gap_values[0]);
	ufbxt_assert_close_vec3(err, mesh->vertex_normal.values.data[3], gap_values[1]);
	ufbxt_assert_close_vec3(err, mesh->vertex_normal.values.data[6], gap_values[2]);
	ufbxt_assert_close_vec3(err, mesh->vertex_normal.values.data[7], gap_values[3]);

	ufbx_vec2 gap_uvs[] = {
		{ -1.3f, -1.4f },
		{ -2.3f, -2.4f },
		{ -3.3f, -3.4f },
		{ -4.3f, -4.4f },
	};

	ufbxt_assert(mesh->vertex_uv.values.count == 8);
	ufbxt_assert_close_vec2(err, mesh->vertex_uv.values.data[0], gap_uvs[0]);
	ufbxt_assert_close_vec2(err, mesh->vertex_uv.values.data[2], gap_uvs[1]);
	ufbxt_assert_close_vec2(err, mesh->vertex_uv.values.data[5], gap_uvs[2]);
	ufbxt_assert_close_vec2(err, mesh->vertex_uv.values.data[7], gap_uvs[3]);
}
#endif

#if UFBXT_IMPL

typedef struct {
	size_t num_faces;
	size_t num_groups;
} ufbxt_groups_per_face_count;

#endif

UFBXT_FILE_TEST(zbrush_polygroup_mess)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->meshes.count == 1);
	ufbx_mesh *mesh = scene->meshes.data[0];

	ufbxt_groups_per_face_count groups_per_face_count_ref[] = {
		{ 1, 2 }, { 2, 1164 }, { 3, 1 }, { 4, 482 }, { 6, 181 }, { 8, 90 }, { 10, 26 }, { 12, 21 },
		{ 14, 8 }, { 16, 10 }, { 18, 5 }, { 20, 6 }, { 22, 4 }, { 24, 7 }, { 26, 1 }, { 28, 1 },
		{ 30, 1 }, { 32, 5 }, { 34, 2 }, { 40, 1 }, { 48, 3 }, { 50, 1 }, { 60, 1 }, { 64, 2 },
		{ 72, 1 }, { 104, 1 }, { 128, 1 }, { 328, 1 },
	};

	uint32_t groups_per_face_count[512] = { 0 };

	ufbxt_assert(mesh->face_groups.count == 2029);
	for (size_t i = 0; i < mesh->face_groups.count; i++) {
		ufbx_mesh_part *part = &mesh->face_group_parts.data[i];
		ufbxt_assert(part->num_faces < ufbxt_arraycount(groups_per_face_count));
		groups_per_face_count[part->num_faces]++;
	}

	for (size_t i = 0; i < ufbxt_arraycount(groups_per_face_count_ref); i++) {
		ufbxt_groups_per_face_count ref = groups_per_face_count_ref[i];
		ufbxt_assert(groups_per_face_count[ref.num_faces] == ref.num_groups);
	}
}
#endif

UFBXT_FILE_TEST(zbrush_cut_sphere)
#if UFBXT_IMPL
{
}
#endif

#if UFBXT_IMPL
typedef struct {
	int32_t id;
	uint32_t face_count;
} ufbxt_face_group_ref;
#endif

UFBXT_FILE_TEST(synthetic_face_group_id)
#if UFBXT_IMPL
{
	const ufbxt_face_group_ref ref_groups[] = {
		{ -2147483647 - 1, 1 },
		{ -2000, 2 },
		{ -2, 1 },
		{ -1, 1 },
		{ 0, 3 },
		{ 1, 2 },
		{ 2, 2 },
		{ 10, 1 },
		{ 20, 1 },
		{ 30, 1 },
		{ 40, 1 },
		{ 50, 1 },
		{ 3000, 2 },
		{ 2147483647, 1 },
	};

	ufbx_node *node = ufbx_find_node(scene, "20 Sided");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	for (size_t i = 0; i < ufbxt_arraycount(ref_groups); i++) {
		ufbxt_hintf("i = %zu", i);
		ufbxt_assert(i < mesh->face_groups.count);
		ufbx_face_group *group = &mesh->face_groups.data[i];
		ufbx_mesh_part *part = &mesh->face_group_parts.data[i];
		ufbxt_assert(group->id == ref_groups[i].id);
		ufbxt_assert(part->num_faces == ref_groups[i].face_count);
	}
}
#endif

UFBXT_FILE_TEST(blender_279_empty_cube)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Cube");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 0);
	ufbxt_assert(mesh->num_indices == 0);
	ufbxt_assert(mesh->num_vertices == 0);

	ufbxt_assert(mesh->materials.count == 1);
	ufbxt_assert(mesh->materials.data[0]);
	ufbxt_assert(mesh->material_parts.data[0].num_faces == 0);
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_truncated_crease_partial, UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbxt_check_warning(scene, UFBX_WARNING_TRUNCATED_ARRAY, UFBX_ELEMENT_MESH, "#pPlane1", 1, "Truncated array: EdgeCrease");

	ufbx_node *node = ufbx_find_node(scene, "pPlane1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->edge_crease.count == 4);
	ufbxt_assert_close_real(err, mesh->edge_crease.data[0], 0.25f);
	ufbxt_assert_close_real(err, mesh->edge_crease.data[1], 0.5f);
	ufbxt_assert_close_real(err, mesh->edge_crease.data[2], 0.5f);
	ufbxt_assert_close_real(err, mesh->edge_crease.data[3], 0.5f);
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_truncated_crease_full, UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbxt_check_warning(scene, UFBX_WARNING_TRUNCATED_ARRAY, UFBX_ELEMENT_MESH, "#pPlane1", 1, "Truncated array: EdgeCrease");

	ufbx_node *node = ufbx_find_node(scene, "pPlane1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->edge_crease.count == 4);
	ufbxt_assert_close_real(err, mesh->edge_crease.data[0], 0.0f);
	ufbxt_assert_close_real(err, mesh->edge_crease.data[1], 0.0f);
	ufbxt_assert_close_real(err, mesh->edge_crease.data[2], 0.0f);
	ufbxt_assert_close_real(err, mesh->edge_crease.data[3], 0.0f);
}
#endif

UFBXT_FILE_TEST_FLAGS(motionbuilder_smoothing, UFBXT_FILE_TEST_FLAG_ALLOW_INVALID_UNICODE|UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbxt_check_warning(scene, UFBX_WARNING_MISSING_GEOMETRY_DATA, UFBX_ELEMENT_MESH, "#pCube1", 1, "Missing geometry data: Smoothing");

	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->edge_smoothing.count == 0);
}
#endif

#if UFBXT_IMPL 
static ufbx_load_opts ufbxt_skip_mesh_parts_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.skip_mesh_parts = true;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_cube_skip_mesh_parts, maya_cube, ufbxt_skip_mesh_parts_opts)
#if UFBXT_IMPL 
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->materials.count == 1);
	ufbxt_assert(mesh->material_parts.count == 0);
	ufbxt_assert(mesh->face_groups.count == 0);
	ufbxt_assert(mesh->face_group_parts.count == 0);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(synthetic_face_group_id_skip_mesh_parts, synthetic_face_group_id, ufbxt_skip_mesh_parts_opts)
#if UFBXT_IMPL 
{
	ufbx_node *node = ufbx_find_node(scene, "20 Sided");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->materials.count == 1);
	ufbxt_assert(mesh->material_parts.count == 0);
	ufbxt_assert(mesh->face_groups.count == 14);
	ufbxt_assert(mesh->face_group_parts.count == 0);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(synthetic_face_groups_skip_mesh_parts, synthetic_face_groups, ufbxt_skip_mesh_parts_opts)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Cube");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->materials.count == 2);
	ufbxt_assert(mesh->material_parts.count == 0);
	ufbxt_assert(mesh->face_groups.count == 4);
	ufbxt_assert(mesh->face_group_parts.count == 0);
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_cm_y_up_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_unit_meters = (ufbx_real)0.01;
	opts.target_axes = ufbx_axes_right_handed_y_up;
	return opts;
}
static ufbx_load_opts ufbxt_cm_y_up_abort_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.target_unit_meters = (ufbx_real)0.01;
	opts.target_axes = ufbx_axes_right_handed_y_up;
	opts.index_error_handling = UFBX_INDEX_ERROR_HANDLING_ABORT_LOADING;
	return opts;
}
#endif

#if UFBXT_IMPL
static ufbx_metadata_object *ufbxt_find_metadata_object(ufbx_element *element)
{
	for (size_t i = 0; i < element->connections_dst.count; i++) {
		ufbx_connection *conn = &element->connections_dst.data[i];
		if (conn->src_prop.length != 0 || conn->dst_prop.length != 0) continue;
		if (conn->src->type == UFBX_ELEMENT_METADATA_OBJECT) {
			return ufbx_as_metadata_object(conn->src);
		}
	}
	return NULL;
}
#endif

UFBXT_FILE_TEST_OPTS(revit_wall_square, ufbxt_cm_y_up_opts)
#if UFBXT_IMPL
{
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(revit_wall_square_abort, revit_wall_square, ufbxt_cm_y_up_abort_opts)
#if UFBXT_IMPL
{
}
#endif

UFBXT_FILE_TEST(synthetic_direct_by_polygon)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pPlane1");
	ufbxt_assert(node);

	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh);

	ufbxt_assert(mesh->faces.count == 2);
	ufbxt_assert(!mesh->vertex_normal.unique_per_vertex);
	ufbxt_assert(mesh->vertex_normal.values.count == 2);

	ufbx_vec3 ref_normals[] = {
		{ -0.707106769f, 0.707106769f, 0.0f },
		{ 0.707106769f, 0.707106769f, 0.0f },
	};
	for (size_t face_ix = 0; face_ix < mesh->faces.count; face_ix++) {
		ufbx_face face = mesh->faces.data[face_ix];
		for (size_t i = 0; i < face.num_indices; i++) {
			ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, face.index_begin + i);
			ufbxt_assert_close_vec3(err, normal, ref_normals[face_ix]);
		}
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(blender_292_circle, UFBXT_FILE_TEST_FLAG_ALLOW_STRICT_ERROR)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Circle");
	ufbxt_assert(node);
	ufbxt_assert(node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->num_vertices == 8);

	for (size_t i = 0; i < mesh->num_vertices; i++) {
		ufbxt_assert(mesh->vertex_first_index.data[i] == UFBX_NO_INDEX);
	}
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_retain_vertex_w_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.retain_vertex_attrib_w = true;
	return opts;
}
#endif

#if UFBXT_IMPL
static void ufbxt_check_tangent_sign(ufbx_mesh *mesh, ufbx_real ref_sign)
{
	if (mesh->element.scene->metadata.version < 7000) {
		ufbxt_assert(mesh->vertex_normal.values_w.count == 0);
		ufbxt_assert(mesh->vertex_tangent.values_w.count == 0);
		ufbxt_assert(mesh->vertex_bitangent.values_w.count == 0);
		return;
	}

	for (size_t ix = 0; ix < mesh->num_indices; ix++) {
		ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, ix);
		ufbx_vec3 tangent = ufbx_get_vertex_vec3(&mesh->vertex_tangent, ix);
		ufbx_vec3 bitangent = ufbx_get_vertex_vec3(&mesh->vertex_bitangent, ix);

		ufbx_vec3 b = ufbxt_cross3(normal, tangent);
		ufbx_real dot = ufbxt_dot3(bitangent, b);
		ufbx_real sign = dot > 0.0f ? 1.0f : -1.0f;

		ufbx_real tangent_w = ufbx_get_vertex_w_vec3(&mesh->vertex_tangent, ix);
		ufbxt_assert(tangent_w == sign);
		if (ref_sign != 0.0f) {
			ufbxt_assert(sign == ref_sign);
		}
	}
}
static void ufbxt_check_tangent_sign_set(ufbx_mesh *mesh, ufbx_uv_set *set, ufbx_real ref_sign)
{
	if (mesh->element.scene->metadata.version < 7000) {
		ufbxt_assert(mesh->vertex_normal.values_w.count == 0);
		ufbxt_assert(set->vertex_tangent.values_w.count == 0);
		ufbxt_assert(set->vertex_bitangent.values_w.count == 0);
		return;
	}

	for (size_t ix = 0; ix < mesh->num_indices; ix++) {
		ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, ix);
		ufbx_vec3 tangent = ufbx_get_vertex_vec3(&set->vertex_tangent, ix);
		ufbx_vec3 bitangent = ufbx_get_vertex_vec3(&set->vertex_bitangent, ix);

		ufbx_vec3 b = ufbxt_cross3(normal, tangent);
		ufbx_real dot = ufbxt_dot3(bitangent, b);
		ufbx_real sign = dot > 0.0f ? 1.0f : -1.0f;

		ufbx_real tangent_w = ufbx_get_vertex_w_vec3(&set->vertex_tangent, ix);
		ufbxt_assert(tangent_w == sign);
		if (ref_sign != 0.0f) {
			ufbxt_assert(sign == ref_sign);
		}
	}
}
static void ufbxt_check_vertex_w(ufbx_mesh *mesh, ufbx_vertex_vec3 *attrib, ufbx_real ref)
{
	if (mesh->element.scene->metadata.version < 7000) {
		ufbxt_assert(attrib->values_w.count == 0);
		return;
	}

	for (size_t ix = 0; ix < mesh->num_indices; ix++) {
		ufbx_real value_w = ufbx_get_vertex_w_vec3(attrib, ix);
		ufbxt_assert(value_w == ref);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_FLAGS(maya_tangent_sign, ufbxt_retain_vertex_w_opts, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "Positive");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_check_tangent_sign(mesh, 1.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_normal, 1.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_bitangent, 1.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "FlipX");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_check_tangent_sign(mesh, -1.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_normal, 1.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_bitangent, 1.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "FlipY");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_check_tangent_sign(mesh, -1.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_normal, 1.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_bitangent, 1.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "FlipBoth");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_check_tangent_sign(mesh, 1.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_normal, 1.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_bitangent, 1.0f);
	}
}
#endif

UFBXT_FILE_TEST_OPTS(max_tangent_sign, ufbxt_retain_vertex_w_opts)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "Positive");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_check_tangent_sign(mesh, 1.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_normal, 0.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_bitangent, 1.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "FlipX");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_check_tangent_sign(mesh, -1.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_normal, 0.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_bitangent, 1.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "FlipY");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_check_tangent_sign(mesh, -1.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_normal, 0.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_bitangent, 1.0f);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "FlipBoth");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_check_tangent_sign(mesh, 1.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_normal, 0.0f);
		ufbxt_check_vertex_w(mesh, &mesh->vertex_bitangent, 1.0f);
	}
}
#endif

UFBXT_FILE_TEST_OPTS(blender340_tangent_sign, ufbxt_retain_vertex_w_opts)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "Positive");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh->vertex_normal.values_w.count == 0);
		ufbxt_assert(mesh->vertex_tangent.values_w.count == 0);
		ufbxt_assert(mesh->vertex_bitangent.values_w.count == 0);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "FlipX");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh->vertex_normal.values_w.count == 0);
		ufbxt_assert(mesh->vertex_tangent.values_w.count == 0);
		ufbxt_assert(mesh->vertex_bitangent.values_w.count == 0);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "FlipY");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh->vertex_normal.values_w.count == 0);
		ufbxt_assert(mesh->vertex_tangent.values_w.count == 0);
		ufbxt_assert(mesh->vertex_bitangent.values_w.count == 0);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "FlipXY");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh->vertex_normal.values_w.count == 0);
		ufbxt_assert(mesh->vertex_tangent.values_w.count == 0);
		ufbxt_assert(mesh->vertex_bitangent.values_w.count == 0);
	}
}
#endif

UFBXT_FILE_TEST_ALT(maya_tangent_sign_default, maya_tangent_sign)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "Positive");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh->vertex_normal.values_w.count == 0);
		ufbxt_assert(mesh->vertex_tangent.values_w.count == 0);
		ufbxt_assert(mesh->vertex_bitangent.values_w.count == 0);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "FlipX");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh->vertex_normal.values_w.count == 0);
		ufbxt_assert(mesh->vertex_tangent.values_w.count == 0);
		ufbxt_assert(mesh->vertex_bitangent.values_w.count == 0);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "FlipY");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh->vertex_normal.values_w.count == 0);
		ufbxt_assert(mesh->vertex_tangent.values_w.count == 0);
		ufbxt_assert(mesh->vertex_bitangent.values_w.count == 0);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "FlipBoth");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh->vertex_normal.values_w.count == 0);
		ufbxt_assert(mesh->vertex_tangent.values_w.count == 0);
		ufbxt_assert(mesh->vertex_bitangent.values_w.count == 0);
	}
}
#endif

UFBXT_FILE_TEST_OPTS(maya_tangent_sign_mixed_split, ufbxt_retain_vertex_w_opts)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "polySurface2");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_check_vertex_w(mesh, &mesh->vertex_normal, 1.0f);
	ufbxt_check_vertex_w(mesh, &mesh->vertex_bitangent, 1.0f);
	ufbxt_check_tangent_sign(mesh, 0.0f);
}
#endif

UFBXT_FILE_TEST_OPTS(maya_tangent_sign_mixed, ufbxt_retain_vertex_w_opts)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pPlane1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_check_vertex_w(mesh, &mesh->vertex_normal, 1.0f);
	ufbxt_check_vertex_w(mesh, &mesh->vertex_bitangent, 1.0f);

	// ???: The tangent sign seems absolutely arbitrary here
	static const ufbx_real tangent_w_ref[] = {
		-1.0f,-1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f,
	};
	for (size_t i = 0; i < mesh->num_indices; i++) {
		ufbx_real w = ufbx_get_vertex_w_vec3(&mesh->vertex_tangent, i);
		ufbxt_assert(w == tangent_w_ref[i]);
	}
}
#endif

UFBXT_FILE_TEST_OPTS(maya_uv_set_tangent_w, ufbxt_retain_vertex_w_opts)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pPlane1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(mesh->uv_sets.count == 3);
	ufbxt_check_vertex_w(mesh, &mesh->vertex_normal, 1.0f);

	ufbx_uv_set *uv_map1 = &mesh->uv_sets.data[0];
	ufbxt_assert(!strcmp(uv_map1->name.data, "map1"));
	ufbxt_check_tangent_sign_set(mesh, uv_map1, 1.0f);
	ufbxt_check_vertex_w(mesh, &uv_map1->vertex_bitangent, 1.0f);

	ufbx_uv_set *uv_left = &mesh->uv_sets.data[1];
	ufbxt_assert(!strcmp(uv_left->name.data, "FlipLeft"));
	ufbxt_check_vertex_w(mesh, &uv_left->vertex_bitangent, 1.0f);

	// ???: The tangent sign seems absolutely arbitrary here
	static const ufbx_real tangent_w_left[] = {
		-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f,
	};
	for (size_t i = 0; i < mesh->num_indices; i++) {
		ufbx_real w = ufbx_get_vertex_w_vec3(&uv_left->vertex_tangent, i);
		if (scene->metadata.version >= 7000) {
			ufbxt_assert(w == tangent_w_left[i]);
		} else {
			ufbxt_assert(w == 0.0f);
		}
	}

	ufbx_uv_set *uv_right = &mesh->uv_sets.data[2];
	ufbxt_assert(!strcmp(uv_right->name.data, "FlipRight"));
	ufbxt_check_vertex_w(mesh, &uv_right->vertex_bitangent, 1.0f);

	// ???: The tangent sign seems absolutely arbitrary here,
	// they are however inverted compared to `uv_left` whivch makes sense,
	// so there is a chance that they do encode something useful (?)
	static const ufbx_real tangent_w_right[] = {
		1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f,
	};
	for (size_t i = 0; i < mesh->num_indices; i++) {
		ufbx_real w = ufbx_get_vertex_w_vec3(&uv_right->vertex_tangent, i);
		if (scene->metadata.version >= 7000) {
			ufbxt_assert(w == tangent_w_right[i]);
		} else {
			ufbxt_assert(w == 0.0f);
		}
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_missing_mapping, UFBXT_FILE_TEST_FLAG_ALLOW_STRICT_ERROR|UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	static const char *const ref_warnings[] = {
		"Ignoring geometry 'Normals' with bad mapping mode ''",
		"Ignoring geometry 'Binormals' with bad mapping mode ''",
		"Ignoring geometry 'Tangents' with bad mapping mode 'Missing'",
		"Ignoring geometry 'UV' with bad mapping mode ''",
		"Ignoring geometry 'Smoothing' with bad mapping mode ''",
		"Ignoring geometry 'Materials' with bad mapping mode ''",
	};
	bool found[ufbxt_arraycount(ref_warnings)] = { 0 };

	ufbxt_assert(scene->metadata.warnings.count == 6);
	for (size_t i = 0; i < scene->metadata.warnings.count; i++) {
		ufbx_warning *warning = &scene->metadata.warnings.data[i];
		ufbxt_assert(warning->type == UFBX_WARNING_MISSING_POLYGON_MAPPING);
		ufbxt_assert(warning->element_id == mesh->element_id);
		for (size_t j = 0; j < ufbxt_arraycount(ref_warnings); j++) {
			if (!strcmp(warning->description.data, ref_warnings[j])) {
				found[j] = true;
				break;
			}
		}
	}

	for (size_t i = 0; i < ufbxt_arraycount(ref_warnings); i++) {
		ufbxt_hintf("i=%zu", i);
		ufbxt_assert(found[i]);
	}
}
#endif
