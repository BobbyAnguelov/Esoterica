#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "obj"

UFBXT_FILE_TEST(zbrush_vertex_color)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->nodes.count == 2);
	ufbx_node *node = scene->nodes.data[1];
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->vertex_color.exists);
	ufbxt_assert(mesh->vertex_color.unique_per_vertex);

	ufbxt_assert(mesh->num_vertices == 6);
	for (size_t i = 0; i < mesh->num_vertices; i++) {
		ufbx_vec3 pos = mesh->vertex_position.values.data[i];
		ufbx_vec4 color = ufbx_get_vertex_vec4(&mesh->vertex_color, mesh->vertex_first_index.data[i]);
		ufbx_vec4 ref = { 0.0f, 0.0f, 0.0f, 1.0f };

		pos.y -= 1.0f;

		if (pos.x < -0.5f) {
			ref.x = 1.0f;
		} else if (pos.x > 0.5f) {
			ref.y = ref.z = 1.0f;
		} else if (pos.y > 0.5f) {
			ref.y = 1.0f;
		} else if (pos.y < -0.5f) {
			ref.x = ref.z = 1.0f;
		} else if (pos.z > 0.5f) {
			ref.z = 1.0f;
		} else if (pos.z < -0.5f) {
			ref.x = ref.y = 1.0f;
		}

		ufbxt_assert_close_vec4(err, color, ref);
		ufbxt_assert_close_vec4(err, color, ref);
	}

}
#endif

UFBXT_FILE_TEST(synthetic_color_suzanne)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->nodes.count == 2);
	ufbx_node *node = scene->nodes.data[1];
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->vertex_color.exists);
	ufbxt_assert(mesh->vertex_color.unique_per_vertex);

	ufbxt_assert(mesh->num_faces == 500);
	ufbxt_assert(mesh->num_triangles == 968);

	for (size_t i = 0; i < mesh->num_indices; i++) {
		ufbx_vec3 position = ufbx_get_vertex_vec3(&mesh->vertex_position, i);
		ufbx_vec4 color = ufbx_get_vertex_vec4(&mesh->vertex_color, i);

		ufbx_vec3 col = { color.x, color.y, color.z };

		ufbx_vec3 ref;
		ref.x = ufbxt_clamp(position.x * 0.5f + 0.5f, 0.0f, 1.0f);
		ref.y = ufbxt_clamp(position.y * 0.5f + 0.5f, 0.0f, 1.0f);
		ref.z = ufbxt_clamp(position.z * 0.5f + 0.5f, 0.0f, 1.0f);

		ufbxt_assert_close_vec3_threshold(err, col, ref, (ufbx_real)(1.0/256.0));
	}
}
#endif

#if UFBXT_IMPL
static void ufbxt_check_obj_elements(ufbxt_diff_error *err, ufbx_scene *scene, int32_t v, int32_t vt, int32_t vn, int32_t vc, const char *name)
{
	ufbxt_hintf("name = \"%s\"", name);

	ufbx_node *node = ufbx_find_node(scene, name);
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;
	ufbxt_assert(!strcmp(mesh->name.data, name));

	ufbxt_assert(mesh->num_faces == 1);
	ufbxt_assert(mesh->num_triangles == 1);

	ufbx_face face = mesh->faces.data[0];
	ufbxt_assert(face.index_begin == 0);
	ufbxt_assert(face.num_indices == 3);

	if (v > 0) {
		ufbxt_assert(mesh->vertex_position.exists);
		ufbxt_assert(mesh->vertex_position.indices.count == 3);
		const ufbx_vec3 refs[] = {
			{ (ufbx_real)-v, 0.0f, (ufbx_real)(v - 1) },
			{ (ufbx_real)+v, 0.0f, (ufbx_real)(v - 1) },
			{ 0.0f, (ufbx_real)+v, (ufbx_real)(v - 1) },
		};
		for (size_t ix = 0; ix < 3; ix++) {
			ufbx_vec3 val = ufbx_get_vertex_vec3(&mesh->vertex_position, ix);
			ufbxt_assert_close_vec3(err, val, refs[ix]);
		}
	} else {
		ufbxt_assert(!mesh->vertex_position.exists);
	}

	if (vt > 0) {
		ufbxt_assert(mesh->vertex_uv.exists);
		ufbxt_assert(mesh->vertex_uv.indices.count == 3);
		const ufbx_vec2 refs[] = {
			{ 0.0f, 0.0f },
			{ (ufbx_real)vt, 0.0f },
			{ 0.0f, (ufbx_real)vt },
		};
		for (size_t ix = 0; ix < 3; ix++) {
			ufbx_vec2 val = ufbx_get_vertex_vec2(&mesh->vertex_uv, ix);
			ufbxt_assert_close_vec2(err, val, refs[ix]);
		}
	} else {
		ufbxt_assert(!mesh->vertex_uv.exists);
	}

	if (vn > 0) {
		ufbxt_assert(mesh->vertex_normal.exists);
		ufbxt_assert(mesh->vertex_normal.indices.count == 3);
		const ufbx_vec3 refs[] = {
			{ 0.0f, (ufbx_real)-vn, 0.0f },
			{ 0.0f, (ufbx_real)-vn, 0.0f },
			{ 0.0f, (ufbx_real)+vn, 0.0f },
		};
		for (size_t ix = 0; ix < 3; ix++) {
			ufbx_vec3 val = ufbx_get_vertex_vec3(&mesh->vertex_normal, ix);
			ufbxt_assert_close_vec3(err, val, refs[ix]);
		}
	} else {
		ufbxt_assert(!mesh->vertex_normal.exists);
	}

	if (vc > 0) {
		ufbxt_assert(mesh->vertex_color.exists);
		ufbxt_assert(mesh->vertex_color.indices.count == 3);
		const ufbx_vec4 refs[] = {
			{ (ufbx_real)vc, 0.0f, 0.0f, 1.0f },
			{ 0.0f, (ufbx_real)vc, 0.0f, 1.0f },
			{ 0.0f, 0.0f, (ufbx_real)vc, 1.0f },
		};
		for (size_t ix = 0; ix < 3; ix++) {
			ufbx_vec4 val = ufbx_get_vertex_vec4(&mesh->vertex_color, ix);
			ufbxt_assert_close_vec4(err, val, refs[ix]);
		}
	} else {
		ufbxt_assert(!mesh->vertex_color.exists);
	}
}
#endif

UFBXT_FILE_TEST(synthetic_mixed_attribs)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->nodes.count == 9);
	ufbxt_assert(scene->meshes.count == 8);
	ufbxt_check_obj_elements(err, scene, 1, 0, 0, 0, "V");
	ufbxt_check_obj_elements(err, scene, 2, 1, 0, 0, "VT");
	ufbxt_check_obj_elements(err, scene, 3, 0, 1, 0, "VN");
	ufbxt_check_obj_elements(err, scene, 4, 2, 2, 0, "VTN");
	ufbxt_check_obj_elements(err, scene, 5, 0, 0, 1, "VC");
	ufbxt_check_obj_elements(err, scene, 6, 3, 0, 2, "VTC");
	ufbxt_check_obj_elements(err, scene, 7, 0, 3, 3, "VNC");
	ufbxt_check_obj_elements(err, scene, 8, 4, 4, 4, "VTNC");
}
#endif

UFBXT_FILE_TEST(synthetic_mixed_attribs_reverse)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->nodes.count == 9);
	ufbxt_assert(scene->meshes.count == 8);
	ufbxt_check_obj_elements(err, scene, 1, 0, 0, 0, "V");
	ufbxt_check_obj_elements(err, scene, 2, 1, 0, 0, "VT");
	ufbxt_check_obj_elements(err, scene, 3, 0, 1, 0, "VN");
	ufbxt_check_obj_elements(err, scene, 4, 2, 2, 0, "VTN");
	ufbxt_check_obj_elements(err, scene, 5, 0, 0, 1, "VC");
	ufbxt_check_obj_elements(err, scene, 6, 3, 0, 2, "VTC");
	ufbxt_check_obj_elements(err, scene, 7, 0, 3, 3, "VNC");
	ufbxt_check_obj_elements(err, scene, 8, 4, 4, 4, "VTNC");
}
#endif

UFBXT_FILE_TEST(synthetic_mixed_attribs_reuse)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->nodes.count == 9);
	ufbxt_assert(scene->meshes.count == 8);
	ufbxt_check_obj_elements(err, scene, 1, 0, 0, 0, "V");
	ufbxt_check_obj_elements(err, scene, 1, 1, 0, 0, "VT");
	ufbxt_check_obj_elements(err, scene, 1, 0, 1, 0, "VN");
	ufbxt_check_obj_elements(err, scene, 1, 1, 1, 0, "VTN");
	ufbxt_check_obj_elements(err, scene, 2, 0, 0, 1, "VC");
	ufbxt_check_obj_elements(err, scene, 2, 1, 0, 1, "VTC");
	ufbxt_check_obj_elements(err, scene, 2, 0, 1, 1, "VNC");
	ufbxt_check_obj_elements(err, scene, 2, 1, 1, 1, "VTNC");
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_no_index_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.index_error_handling = UFBX_INDEX_ERROR_HANDLING_NO_INDEX;
	return opts;
}

static ufbx_load_opts ufbxt_abort_index_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.index_error_handling = UFBX_INDEX_ERROR_HANDLING_ABORT_LOADING;
	return opts;
}

static void ufbxt_check_obj_face(ufbx_mesh *mesh, size_t face_ix, int32_t v, int32_t vt, int32_t vn, int32_t vc, bool no_index)
{
	ufbxt_hintf("face_ix = %zu", face_ix);

	ufbxt_assert(face_ix < mesh->faces.count);
	ufbx_face face = mesh->faces.data[face_ix];
	uint32_t a = face.index_begin + 0;
	uint32_t b = face.index_begin + 1;
	uint32_t c = face.index_begin + 2;

	if (v) {
		ufbxt_assert(mesh->vertex_position.indices.data[a] == v - 1 + 0);
		ufbxt_assert(mesh->vertex_position.indices.data[b] == v - 1 + 1);
		ufbxt_assert(mesh->vertex_position.indices.data[c] == v - 1 + 2);
	} else {
		uint32_t sentinel = no_index ? UFBX_NO_INDEX : (uint32_t)mesh->vertex_position.values.count - 1;
		ufbxt_assert(mesh->vertex_position.indices.data[a] == sentinel);
		ufbxt_assert(mesh->vertex_position.indices.data[b] == sentinel);
		ufbxt_assert(mesh->vertex_position.indices.data[c] == sentinel);
	}

	if (vt) {
		ufbxt_assert(mesh->vertex_uv.indices.data[a] == vt - 1 + 0);
		ufbxt_assert(mesh->vertex_uv.indices.data[b] == vt - 1 + 1);
		ufbxt_assert(mesh->vertex_uv.indices.data[c] == vt - 1 + 2);
	} else {
		uint32_t sentinel = no_index ? UFBX_NO_INDEX : (uint32_t)mesh->vertex_uv.values.count - 1;
		ufbxt_assert(mesh->vertex_uv.indices.data[a] == sentinel);
		ufbxt_assert(mesh->vertex_uv.indices.data[b] == sentinel);
		ufbxt_assert(mesh->vertex_uv.indices.data[c] == sentinel);
	}

	if (vn) {
		ufbxt_assert(mesh->vertex_normal.indices.data[a] == vn - 1 + 0);
		ufbxt_assert(mesh->vertex_normal.indices.data[b] == vn - 1 + 1);
		ufbxt_assert(mesh->vertex_normal.indices.data[c] == vn - 1 + 2);
	} else {
		uint32_t sentinel = no_index ? UFBX_NO_INDEX : (uint32_t)mesh->vertex_normal.values.count - 1;
		ufbxt_assert(mesh->vertex_normal.indices.data[a] == sentinel);
		ufbxt_assert(mesh->vertex_normal.indices.data[b] == sentinel);
		ufbxt_assert(mesh->vertex_normal.indices.data[c] == sentinel);
	}

	if (vc) {
		ufbxt_assert(mesh->vertex_color.indices.data[a] == vc - 1 + 0);
		ufbxt_assert(mesh->vertex_color.indices.data[b] == vc - 1 + 1);
		ufbxt_assert(mesh->vertex_color.indices.data[c] == vc - 1 + 2);
	} else {
		uint32_t sentinel = no_index ? UFBX_NO_INDEX : (uint32_t)mesh->vertex_color.values.count - 1;
		ufbxt_assert(mesh->vertex_color.indices.data[a] == sentinel);
		ufbxt_assert(mesh->vertex_color.indices.data[b] == sentinel);
		ufbxt_assert(mesh->vertex_color.indices.data[c] == sentinel);
	}
}

static void ufbxt_check_obj_index(ufbx_mesh *mesh, size_t index, int32_t v, int32_t vt, int32_t vn, int32_t vc, bool no_index)
{
	ufbxt_hintf("index = %zu", index);
	ufbxt_assert(index < mesh->num_indices);

	if (v) {
		ufbxt_assert(mesh->vertex_position.indices.data[index] == v - 1);
	} else {
		uint32_t sentinel = no_index ? UFBX_NO_INDEX : (uint32_t)mesh->vertex_position.values.count - 1;
		ufbxt_assert(mesh->vertex_position.indices.data[index] == sentinel);
	}

	if (vt) {
		ufbxt_assert(mesh->vertex_uv.indices.data[index] == vt - 1);
	} else {
		uint32_t sentinel = no_index ? UFBX_NO_INDEX : (uint32_t)mesh->vertex_uv.values.count - 1;
		ufbxt_assert(mesh->vertex_uv.indices.data[index] == sentinel);
	}

	if (vn) {
		ufbxt_assert(mesh->vertex_normal.indices.data[index] == vn - 1);
	} else {
		uint32_t sentinel = no_index ? UFBX_NO_INDEX : (uint32_t)mesh->vertex_normal.values.count - 1;
		ufbxt_assert(mesh->vertex_normal.indices.data[index] == sentinel);
	}

	if (vc) {
		ufbxt_assert(mesh->vertex_color.indices.data[index] == vc - 1);
	} else {
		uint32_t sentinel = no_index ? UFBX_NO_INDEX : (uint32_t)mesh->vertex_color.values.count - 1;
		ufbxt_assert(mesh->vertex_color.indices.data[index] == sentinel);
	}
}

#endif

UFBXT_FILE_TEST_FLAGS(synthetic_partial_attrib, UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbxt_check_warning(scene, UFBX_WARNING_INDEX_CLAMPED, UFBX_ELEMENT_UNKNOWN, NULL, 60, NULL);

	ufbx_node *node = ufbx_find_node(scene, "Mesh");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 12);
	ufbxt_assert(mesh->num_triangles == 12);
	ufbxt_assert(mesh->vertex_position.exists);
	ufbxt_assert(mesh->vertex_uv.exists);
	ufbxt_assert(mesh->vertex_normal.exists);
	ufbxt_assert(mesh->vertex_color.exists);

	ufbxt_check_obj_face(mesh,  0, 1, 0, 0, 0, false);
	ufbxt_check_obj_face(mesh,  1, 1, 1, 0, 0, false);
	ufbxt_check_obj_face(mesh,  2, 1, 0, 1, 0, false);
	ufbxt_check_obj_face(mesh,  3, 1, 1, 1, 0, false);
	ufbxt_check_obj_face(mesh,  4, 4, 0, 0, 4, false);
	ufbxt_check_obj_face(mesh,  5, 4, 1, 0, 4, false);
	ufbxt_check_obj_face(mesh,  6, 4, 0, 1, 4, false);
	ufbxt_check_obj_face(mesh,  7, 4, 1, 1, 4, false);
	ufbxt_check_obj_face(mesh,  8, 0, 0, 0, 0, false);
	ufbxt_check_obj_face(mesh,  9, 0, 1, 0, 0, false);
	ufbxt_check_obj_face(mesh, 10, 0, 0, 1, 0, false);
	ufbxt_check_obj_face(mesh, 11, 0, 1, 1, 0, false);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(synthetic_partial_attrib_no_index, synthetic_partial_attrib, ufbxt_no_index_opts)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Mesh");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 12);
	ufbxt_assert(mesh->num_triangles == 12);
	ufbxt_assert(mesh->vertex_position.exists);
	ufbxt_assert(mesh->vertex_uv.exists);
	ufbxt_assert(mesh->vertex_normal.exists);
	ufbxt_assert(mesh->vertex_color.exists);

	ufbxt_check_obj_face(mesh,  0, 1, 0, 0, 0, true);
	ufbxt_check_obj_face(mesh,  1, 1, 1, 0, 0, true);
	ufbxt_check_obj_face(mesh,  2, 1, 0, 1, 0, true);
	ufbxt_check_obj_face(mesh,  3, 1, 1, 1, 0, true);
	ufbxt_check_obj_face(mesh,  4, 4, 0, 0, 4, true);
	ufbxt_check_obj_face(mesh,  5, 4, 1, 0, 4, true);
	ufbxt_check_obj_face(mesh,  6, 4, 0, 1, 4, true);
	ufbxt_check_obj_face(mesh,  7, 4, 1, 1, 4, true);
	ufbxt_check_obj_face(mesh,  8, 0, 0, 0, 0, true);
	ufbxt_check_obj_face(mesh,  9, 0, 1, 0, 0, true);
	ufbxt_check_obj_face(mesh, 10, 0, 0, 1, 0, true);
	ufbxt_check_obj_face(mesh, 11, 0, 1, 1, 0, true);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(synthetic_partial_attrib_strict, synthetic_partial_attrib, ufbxt_abort_index_opts, UFBXT_FILE_TEST_FLAG_ALLOW_ERROR)
#if UFBXT_IMPL
{
	ufbxt_assert(!scene);
	ufbxt_assert(load_error);
	ufbxt_assert(load_error->type == UFBX_ERROR_BAD_INDEX);
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_partial_attrib_face, UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbxt_check_warning(scene, UFBX_WARNING_INDEX_CLAMPED, UFBX_ELEMENT_UNKNOWN, NULL, 20, NULL);

	ufbx_node *node = ufbx_find_node(scene, "Mesh");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 1);
	ufbxt_assert(mesh->num_triangles == 10);
	ufbxt_assert(mesh->num_indices == 12);
	ufbxt_assert(mesh->vertex_position.exists);
	ufbxt_assert(mesh->vertex_uv.exists);
	ufbxt_assert(mesh->vertex_normal.exists);
	ufbxt_assert(mesh->vertex_color.exists);

	ufbx_face face = mesh->faces.data[0];
	ufbxt_assert(face.index_begin == 0);
	ufbxt_assert(face.num_indices == 12);

	ufbxt_check_obj_index(mesh,  0, 1, 0, 0, 0, false);
	ufbxt_check_obj_index(mesh,  1, 2, 1, 0, 0, false);
	ufbxt_check_obj_index(mesh,  2, 3, 0, 1, 0, false);
	ufbxt_check_obj_index(mesh,  3, 4, 2, 2, 0, false);
	ufbxt_check_obj_index(mesh,  4, 5, 0, 0, 5, false);
	ufbxt_check_obj_index(mesh,  5, 6, 3, 0, 6, false);
	ufbxt_check_obj_index(mesh,  6, 7, 0, 3, 7, false);
	ufbxt_check_obj_index(mesh,  7, 8, 4, 4, 8, false);
	ufbxt_check_obj_index(mesh,  8, 0, 0, 0, 0, false);
	ufbxt_check_obj_index(mesh,  9, 0, 5, 0, 0, false);
	ufbxt_check_obj_index(mesh, 10, 0, 0, 5, 0, false);
	ufbxt_check_obj_index(mesh, 11, 0, 6, 6, 0, false);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(synthetic_partial_attrib_face_no_index, synthetic_partial_attrib_face, ufbxt_no_index_opts)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Mesh");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 1);
	ufbxt_assert(mesh->num_triangles == 10);
	ufbxt_assert(mesh->num_indices == 12);
	ufbxt_assert(mesh->vertex_position.exists);
	ufbxt_assert(mesh->vertex_uv.exists);
	ufbxt_assert(mesh->vertex_normal.exists);
	ufbxt_assert(mesh->vertex_color.exists);

	ufbx_face face = mesh->faces.data[0];
	ufbxt_assert(face.index_begin == 0);
	ufbxt_assert(face.num_indices == 12);

	ufbxt_check_obj_index(mesh,  0, 1, 0, 0, 0, true);
	ufbxt_check_obj_index(mesh,  1, 2, 1, 0, 0, true);
	ufbxt_check_obj_index(mesh,  2, 3, 0, 1, 0, true);
	ufbxt_check_obj_index(mesh,  3, 4, 2, 2, 0, true);
	ufbxt_check_obj_index(mesh,  4, 5, 0, 0, 5, true);
	ufbxt_check_obj_index(mesh,  5, 6, 3, 0, 6, true);
	ufbxt_check_obj_index(mesh,  6, 7, 0, 3, 7, true);
	ufbxt_check_obj_index(mesh,  7, 8, 4, 4, 8, true);
	ufbxt_check_obj_index(mesh,  8, 0, 0, 0, 0, true);
	ufbxt_check_obj_index(mesh,  9, 0, 5, 0, 0, true);
	ufbxt_check_obj_index(mesh, 10, 0, 0, 5, 0, true);
	ufbxt_check_obj_index(mesh, 11, 0, 6, 6, 0, true);
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(synthetic_partial_attrib_face_strict, synthetic_partial_attrib_face, ufbxt_abort_index_opts, UFBXT_FILE_TEST_FLAG_ALLOW_ERROR)
#if UFBXT_IMPL
{
	ufbxt_assert(!scene);
	ufbxt_assert(load_error);
	ufbxt_assert(load_error->type == UFBX_ERROR_BAD_INDEX);
}
#endif

UFBXT_FILE_TEST(synthetic_simple_materials)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->materials.count == 3);

	{
		ufbx_material *mat = ufbx_find_material(scene, "RGB");
		ufbxt_assert(mat);
		ufbxt_assert(mat->shader_type == UFBX_SHADER_WAVEFRONT_MTL);

		ufbx_vec3 ka = { 1.0f, 0.0f, 0.0f };
		ufbx_vec3 kd = { 0.0f, 1.0f, 0.0f };
		ufbx_vec3 ks = { 0.0f, 0.0f, 1.0f };
		ufbx_vec3 ke = { 1.0f, 0.0f, 1.0f };
		ufbx_real ns = 99.0f;
		ufbx_real d = 0.25f;

		ufbxt_assert_close_vec3(err, mat->fbx.ambient_color.value_vec3, ka);
		ufbxt_assert_close_vec3(err, mat->fbx.diffuse_color.value_vec3, kd);
		ufbxt_assert_close_vec3(err, mat->fbx.specular_color.value_vec3, ks);
		ufbxt_assert_close_vec3(err, mat->fbx.emission_color.value_vec3, ke);
		ufbxt_assert_close_real(err, mat->fbx.specular_exponent.value_real, ns);
		ufbxt_assert_close_real(err, mat->fbx.transparency_factor.value_real, 1.0f - d);
		ufbxt_assert(mat->fbx.ambient_factor.value_real == 1.0f);
		ufbxt_assert(mat->fbx.diffuse_factor.value_real == 1.0f);
		ufbxt_assert(mat->fbx.specular_factor.value_real == 1.0f);
		ufbxt_assert(mat->fbx.emission_factor.value_real == 1.0f);

		ufbxt_assert_close_vec3(err, mat->pbr.base_color.value_vec3, kd);
		ufbxt_assert_close_vec3(err, mat->pbr.specular_color.value_vec3, ks);
		ufbxt_assert_close_vec3(err, mat->pbr.emission_color.value_vec3, ke);
		ufbxt_assert_close_real(err, mat->pbr.roughness.value_real, 0.00501256289f);
		ufbxt_assert_close_real(err, mat->pbr.opacity.value_real, d);
		ufbxt_assert(mat->pbr.base_factor.value_real == 1.0f);
		ufbxt_assert(mat->pbr.specular_factor.value_real == 1.0f);
		ufbxt_assert(mat->pbr.emission_factor.value_real == 1.0f);

		ufbxt_assert(mat->features.diffuse.enabled);
		ufbxt_assert(mat->features.specular.enabled);
		ufbxt_assert(mat->features.opacity.enabled);
		ufbxt_assert(!mat->features.pbr.enabled);
		ufbxt_assert(!mat->features.metalness.enabled);
		ufbxt_assert(!mat->features.sheen.enabled);
		ufbxt_assert(!mat->features.coat.enabled);
		ufbxt_assert(!mat->features.transmission.enabled);
	}

	{
		ufbx_material *mat = ufbx_find_material(scene, "PBR");
		ufbxt_assert(mat);
		ufbxt_assert(mat->shader_type == UFBX_SHADER_WAVEFRONT_MTL);

		ufbx_real pr = 0.1f;
		ufbx_real pm = 0.2f;
		ufbx_vec3 ps = { 0.3f, 0.4f, 0.5f };
		ufbx_real pc = 0.6f;
		ufbx_real pcr = 0.7f;
		ufbx_real ni = 1.33f;
		ufbx_vec3 tf = { 0.8f, 0.9f, 1.0f };
		ufbx_real d = 0.75f;

		ufbxt_assert_close_real(err, mat->pbr.roughness.value_real, pr);
		ufbxt_assert_close_real(err, mat->pbr.metalness.value_real, pm);
		ufbxt_assert_close_vec3(err, mat->pbr.sheen_color.value_vec3, ps);
		ufbxt_assert_close_real(err, mat->pbr.coat_factor.value_real, pc);
		ufbxt_assert_close_real(err, mat->pbr.coat_roughness.value_real, pcr);
		ufbxt_assert_close_real(err, mat->pbr.specular_ior.value_real, ni);
		ufbxt_assert_close_vec3(err, mat->pbr.transmission_color.value_vec3, tf);
		ufbxt_assert_close_real(err, mat->pbr.opacity.value_real, d);

		ufbxt_assert(mat->pbr.sheen_factor.value_real == 1.0f);
		ufbxt_assert(mat->pbr.transmission_factor.value_real == 1.0f);

		ufbxt_assert(mat->features.pbr.enabled);
		ufbxt_assert(mat->features.metalness.enabled);
		ufbxt_assert(mat->features.diffuse.enabled);
		ufbxt_assert(mat->features.specular.enabled);
		ufbxt_assert(mat->features.sheen.enabled);
		ufbxt_assert(mat->features.coat.enabled);
		ufbxt_assert(mat->features.transmission.enabled);
		ufbxt_assert(mat->features.opacity.enabled);
	}

	{
		ufbx_material *mat = ufbx_find_material(scene, "Wide");
		ufbxt_assert(mat);
		ufbxt_assert(mat->shader_type == UFBX_SHADER_WAVEFRONT_MTL);

		ufbx_vec3 ka = { 0.1f, 0.1f, 0.1f };
		ufbx_vec3 kd = { 0.2f, 0.2f, 0.2f };
		ufbx_vec3 ks = { 0.3f, 0.3f, 0.3f };
		ufbx_vec3 ke = { 0.4f, 0.4f, 0.4f };
		ufbx_vec3 ps = { 0.5f, 0.5f, 0.5f };
		ufbx_vec3 tf = { 0.6f, 0.6f, 0.6f };

		ufbxt_assert_close_vec3(err, mat->fbx.ambient_color.value_vec3, ka);
		ufbxt_assert_close_vec3(err, mat->fbx.diffuse_color.value_vec3, kd);
		ufbxt_assert_close_vec3(err, mat->fbx.specular_color.value_vec3, ks);
		ufbxt_assert_close_vec3(err, mat->fbx.emission_color.value_vec3, ke);

		ufbxt_assert_close_vec3(err, mat->pbr.base_color.value_vec3, kd);
		ufbxt_assert_close_vec3(err, mat->pbr.specular_color.value_vec3, ks);
		ufbxt_assert_close_vec3(err, mat->pbr.emission_color.value_vec3, ke);
		ufbxt_assert_close_vec3(err, mat->pbr.sheen_color.value_vec3, ps);
		ufbxt_assert_close_vec3(err, mat->pbr.transmission_color.value_vec3, tf);

	}
}
#endif

#if UFBXT_IMPL
void ufbxt_check_obj_texture(ufbx_scene *scene, ufbx_texture *texture, const char *filename)
{
	char rel_path[256];
	snprintf(rel_path, sizeof(rel_path), "textures/%s", filename);

	ufbxt_assert(texture);
	ufbxt_assert(!strcmp(texture->relative_filename.data, rel_path));
}
#endif

UFBXT_FILE_TEST(synthetic_simple_textures)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->materials.count == 3);

	{
		ufbx_material *mat = ufbx_find_material(scene, "RGB");
		ufbxt_assert(mat);
		ufbxt_assert(mat->shader_type == UFBX_SHADER_WAVEFRONT_MTL);

		ufbxt_check_obj_texture(scene, mat->fbx.ambient_color.texture, "checkerboard_ambient.png");
		ufbxt_check_obj_texture(scene, mat->fbx.diffuse_color.texture, "checkerboard_diffuse.png");
		ufbxt_check_obj_texture(scene, mat->fbx.specular_color.texture, "checkerboard_specular.png");
		ufbxt_check_obj_texture(scene, mat->fbx.emission_color.texture, "checkerboard_emissive.png");
		ufbxt_check_obj_texture(scene, mat->fbx.specular_exponent.texture, "checkerboard_roughness.png");
		ufbxt_check_obj_texture(scene, mat->fbx.transparency_factor.texture, "checkerboard_transparency.png");
		ufbxt_check_obj_texture(scene, mat->fbx.bump.texture, "checkerboard_bump.png");

		ufbxt_check_obj_texture(scene, mat->pbr.base_color.texture, "checkerboard_diffuse.png");
		ufbxt_check_obj_texture(scene, mat->pbr.specular_color.texture, "checkerboard_specular.png");
		ufbxt_check_obj_texture(scene, mat->pbr.emission_color.texture, "checkerboard_emissive.png");
		ufbxt_check_obj_texture(scene, mat->pbr.roughness.texture, "checkerboard_roughness.png");
		ufbxt_check_obj_texture(scene, mat->pbr.normal_map.texture, "checkerboard_bump.png");
	}

	{
		ufbx_material *mat = ufbx_find_material(scene, "PBR");
		ufbxt_assert(mat);
		ufbxt_assert(mat->shader_type == UFBX_SHADER_WAVEFRONT_MTL);

		ufbxt_check_obj_texture(scene, mat->pbr.roughness.texture, "checkerboard_roughness.png");
		ufbxt_check_obj_texture(scene, mat->pbr.metalness.texture, "checkerboard_metallic.png");
		ufbxt_check_obj_texture(scene, mat->pbr.sheen_color.texture, "checkerboard_reflection.png");
		ufbxt_check_obj_texture(scene, mat->pbr.coat_factor.texture, "checkerboard_specular.png");
		ufbxt_check_obj_texture(scene, mat->pbr.coat_roughness.texture, "checkerboard_weight.png");
		ufbxt_check_obj_texture(scene, mat->pbr.transmission_color.texture, "checkerboard_transparency.png");
		ufbxt_check_obj_texture(scene, mat->pbr.opacity.texture, "checkerboard_weight.png");
		ufbxt_check_obj_texture(scene, mat->pbr.specular_ior.texture, "checkerboard_specular.png");
		ufbxt_check_obj_texture(scene, mat->pbr.normal_map.texture, "checkerboard_normal.png");
		ufbxt_check_obj_texture(scene, mat->pbr.displacement_map.texture, "checkerboard_displacement.png");

		ufbxt_check_obj_texture(scene, mat->fbx.transparency_factor.texture, "checkerboard_weight.png");
		ufbxt_check_obj_texture(scene, mat->fbx.normal_map.texture, "checkerboard_normal.png");
		ufbxt_check_obj_texture(scene, mat->fbx.displacement.texture, "checkerboard_displacement.png");
	}

	{
		ufbx_material *mat = ufbx_find_material(scene, "NonMap");
		ufbxt_assert(mat);
		ufbxt_assert(mat->shader_type == UFBX_SHADER_WAVEFRONT_MTL);

		ufbxt_assert(mat->textures.count == 3);

		ufbxt_check_obj_texture(scene, mat->pbr.normal_map.texture, "checkerboard_normal.png");
		ufbxt_check_obj_texture(scene, mat->pbr.displacement_map.texture, "checkerboard_displacement.png");

		ufbxt_check_obj_texture(scene, mat->fbx.normal_map.texture, "checkerboard_normal.png");
		ufbxt_check_obj_texture(scene, mat->fbx.displacement.texture, "checkerboard_displacement.png");

		ufbxt_check_obj_texture(scene, ufbx_find_prop_texture(mat, "norm"), "checkerboard_normal.png");
		ufbxt_check_obj_texture(scene, ufbx_find_prop_texture(mat, "disp"), "checkerboard_displacement.png");
		ufbxt_check_obj_texture(scene, ufbx_find_prop_texture(mat, "bump"), "checkerboard_bump.png");
	}
}
#endif

#if UFBXT_IMPL
void ufbxt_check_obj_prop(ufbxt_diff_error *err, ufbx_props *props, const char *name, const char *str, int64_t i, ufbx_real x, ufbx_real y, ufbx_real z)
{
	ufbxt_hintf("name = \"%s\"", name);

	ufbx_prop *prop = ufbx_find_prop(props, name);
	ufbxt_assert(prop);
	if (str) {
		ufbxt_assert(!strcmp(prop->value_str.data, str));
	}

	ufbxt_assert(prop->value_int == i);

	ufbxt_assert_close_real(err, prop->value_vec3.x, x);
	ufbxt_assert_close_real(err, prop->value_vec3.y, y);
	ufbxt_assert_close_real(err, prop->value_vec3.z, z);
}
#endif

UFBXT_FILE_TEST(synthetic_texture_opts)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->materials.count == 1);

	ufbx_material *mat = ufbx_find_material(scene, "Opts");
	ufbxt_assert(mat);
	ufbxt_assert(mat->shader_type == UFBX_SHADER_WAVEFRONT_MTL);

	{
		ufbx_texture *tex = mat->fbx.diffuse_color.texture;
		ufbxt_assert(tex);
		ufbxt_assert(!strcmp(tex->relative_filename.data, "textures/checkerboard_diffuse.png"));

		ufbxt_check_obj_prop(err, &tex->props, "blendu", "off", 0, 0.0f, 0.0f, 0.0f);
		ufbxt_check_obj_prop(err, &tex->props, "blendv", "on", 1, 1.0f, 0.0f, 0.0f);
		ufbxt_check_obj_prop(err, &tex->props, "clamp", "off", 0, 0.0f, 0.0f, 0.0f);
		ufbxt_check_obj_prop(err, &tex->props, "imfchan", "r", 0, 0.0f, 0.0f, 0.0f);
		ufbxt_check_obj_prop(err, &tex->props, "mm", "1 2", 1, 1.0f, 2.0f, 0.0f);
		ufbxt_check_obj_prop(err, &tex->props, "o", "0.1 0.2 0.3", 0, 0.1f, 0.2f, 0.3f);
		ufbxt_check_obj_prop(err, &tex->props, "s", "0.4 0.5 0.6", 0, 0.4f, 0.5f, 0.6f);
		ufbxt_check_obj_prop(err, &tex->props, "t", "0.7 0.8 0.9", 0, 0.7f, 0.8f, 0.9f);
		ufbxt_check_obj_prop(err, &tex->props, "texres", "512", 512, 512.0f, 0.0f, 0.0f);
	}

	{
		ufbx_texture *tex = mat->fbx.specular_color.texture;
		ufbxt_assert(tex);
		ufbxt_assert(!strcmp(tex->relative_filename.data, "textures/checkerboard_specular.png"));

		ufbxt_check_obj_prop(err, &tex->props, "blendu", "on", 1, 1.0f, 0.0f, 0.0f);
		ufbxt_check_obj_prop(err, &tex->props, "blendv", "off", 0, 0.0f, 0.0f, 0.0f);
		ufbxt_check_obj_prop(err, &tex->props, "clamp", "on", 1, 1.0f, 0.0f, 0.0f);
		ufbxt_check_obj_prop(err, &tex->props, "imfchan", "g", 0, 0.0f, 0.0f, 0.0f);
		ufbxt_check_obj_prop(err, &tex->props, "mm", "3 4", 3, 3.0f, 4.0f, 0.0f);
		ufbxt_check_obj_prop(err, &tex->props, "o", "-0.1 -.2 1.3", 0, -0.1f, -0.2f, 1.3f);
		ufbxt_check_obj_prop(err, &tex->props, "s", NULL, 1, 1.4f, 1.5f, 1.6f);
		ufbxt_check_obj_prop(err, &tex->props, "t", "1.7 1.8 1.9", 1, 1.7f, 1.8f, 1.9f);
		ufbxt_check_obj_prop(err, &tex->props, "texres", "1024", 1024, 1024.0f, 0.0f, 0.0f);
		ufbxt_check_obj_prop(err, &tex->props, "unknown", "hello world", 0, 0.0f, 0.0f, 0.0f);
		ufbxt_check_obj_prop(err, &tex->props, "single-unknown", "", 0, 0.0f, 0.0f, 0.0f);
	}

}
#endif

UFBXT_FILE_TEST_PATH(blender_331_space_texture, "blender_331_space texture")
#if UFBXT_IMPL
{
	ufbxt_assert(scene->materials.count == 1);
	ufbx_material *material = ufbx_find_material(scene, "Material");
	ufbxt_assert(material);

	ufbx_texture *texture = material->fbx.diffuse_color.texture;
	ufbxt_assert(texture);
	ufbxt_assert(!strcmp(texture->relative_filename.data, "space dir/space tex.png"));

	if (scene->metadata.file_format == UFBX_FILE_FORMAT_OBJ) {
		ufbx_node *node = ufbx_find_node(scene, "Cube");
		ufbxt_assert(node && node->mesh);
		ufbxt_assert(node->materials.count == 1);
		ufbxt_assert(node->materials.data[0] == material);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_obj_zoo, UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbxt_check_warning(scene, UFBX_WARNING_UNKNOWN_OBJ_DIRECTIVE, UFBX_ELEMENT_UNKNOWN, NULL, 24, "Unknown .obj directive, skipped line");

	ufbx_node *node = ufbx_find_node(scene, "Object");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 7);
	ufbxt_assert(mesh->num_line_faces == 2);
	ufbxt_assert(mesh->num_point_faces == 4);
}
#endif

#if UFBXT_IMPL

typedef struct ufbxt_mesh_material {
	ufbx_material *material;
	ufbx_mesh_part *part;
} ufbxt_mesh_material;

typedef struct ufbxt_mesh_group {
	ufbx_face_group *group;
	ufbx_mesh_part *part;
} ufbxt_mesh_group;

static ufbxt_mesh_material ufbxt_find_mesh_material(ufbx_mesh *mesh, const char *name)
{
	ufbxt_mesh_material mat = { NULL, NULL };
	for (size_t i = 0; i < mesh->materials.count; i++) {
		if (!strcmp(mesh->materials.data[i]->name.data, name)) {
			mat.material = mesh->materials.data[i];
			mat.part = mesh->material_parts.count > 0 ? &mesh->material_parts.data[i] : NULL;
			return mat;
		}
	}
	return mat;
}

static ufbxt_mesh_group ufbxt_find_face_group(ufbx_mesh *mesh, const char *name)
{
	ufbxt_mesh_group group = { NULL, NULL };
	for (size_t i = 0; i < mesh->face_groups.count; i++) {
		if (!strcmp(mesh->face_groups.data[i].name.data, name)) {
			group.group = &mesh->face_groups.data[i];
			group.part = mesh->face_group_parts.count > 0 ? &mesh->face_group_parts.data[i] : NULL;
		}
	}
	return group;
}

static ufbx_mesh_part *ufbxt_find_mesh_material_part(ufbx_mesh *mesh, const char *name)
{
	ufbxt_mesh_material mat = ufbxt_find_mesh_material(mesh, name);
	return mat.part;
}

static ufbx_mesh_part *ufbxt_find_face_group_part(ufbx_mesh *mesh, const char *name)
{
	ufbxt_mesh_group group = ufbxt_find_face_group(mesh, name);
	return group.part;
}

static void ufbxt_check_planar_face(ufbxt_diff_error *err, ufbx_mesh *mesh, ufbx_face face, ufbx_vec3 normal, ufbx_real w)
{
	for (size_t i = 0; i < face.num_indices; i++) {
		ufbx_vec3 pos = ufbx_get_vertex_vec3(&mesh->vertex_position, face.index_begin + i);
		ufbx_real ref = ufbxt_dot3(normal, pos);
		ufbxt_assert_close_real(err, ref, w);
	}
}

static void ufbxt_check_planar_face_ix(ufbxt_diff_error *err, ufbx_mesh *mesh, size_t face_ix, ufbx_vec3 normal, ufbx_real w)
{
	ufbxt_assert(face_ix < mesh->faces.count);
	ufbxt_check_planar_face(err, mesh, mesh->faces.data[face_ix], normal, w);
}

#endif

UFBXT_FILE_TEST(synthetic_face_groups)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Cube");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbx_vec3 pos_x = { +1.0f, 0.0f, 0.0f };
	ufbx_vec3 neg_x = { -1.0f, 0.0f, 0.0f };
	ufbx_vec3 pos_y = { 0.0f, +1.0f, 0.0f };
	ufbx_vec3 neg_y = { 0.0f, -1.0f, 0.0f };
	ufbx_vec3 pos_z = { 0.0f, 0.0f, +1.0f };
	ufbx_vec3 neg_z = { 0.0f, 0.0f, -1.0f };

	{
		ufbx_mesh_part *group = ufbxt_find_face_group_part(mesh, "");
		ufbxt_assert(group);
		ufbxt_assert(group->num_faces == 2);
		ufbxt_assert(group->face_indices.data[0] == 0);
		ufbxt_assert(group->face_indices.data[1] == 5);
		ufbxt_check_planar_face_ix(err, mesh, group->face_indices.data[0], pos_y, 1.0f);
		ufbxt_check_planar_face_ix(err, mesh, group->face_indices.data[1], neg_y, 1.0f);
	}

	{
		ufbx_mesh_part *group = ufbxt_find_face_group_part(mesh, "Front");
		ufbxt_assert(group);
		ufbxt_assert(group->num_faces == 1);
		ufbxt_assert(group->face_indices.data[0] == 1);
		ufbxt_check_planar_face_ix(err, mesh, group->face_indices.data[0], neg_z, 1.0f);
	}

	{
		ufbx_mesh_part *group = ufbxt_find_face_group_part(mesh, "Sides");
		ufbxt_assert(group);
		ufbxt_assert(group->num_faces == 2);
		ufbxt_assert(group->face_indices.data[0] == 2);
		ufbxt_assert(group->face_indices.data[1] == 4);
		ufbxt_check_planar_face_ix(err, mesh, group->face_indices.data[0], neg_x, 1.0f);
		ufbxt_check_planar_face_ix(err, mesh, group->face_indices.data[1], pos_x, 1.0f);
	}

	{
		ufbx_mesh_part *group = ufbxt_find_face_group_part(mesh, "Back");
		ufbxt_assert(group);
		ufbxt_assert(group->num_faces == 1);
		ufbxt_assert(group->face_indices.data[0] == 3);
		ufbxt_check_planar_face_ix(err, mesh, group->face_indices.data[0], pos_z, 1.0f);
	}

	{
		ufbx_mesh_part *mat = ufbxt_find_mesh_material_part(mesh, "A");
		ufbxt_assert(mat);
		ufbxt_assert(mat->num_faces == 3);
		ufbxt_assert(mat->face_indices.data[0] == 0);
		ufbxt_assert(mat->face_indices.data[1] == 1);
		ufbxt_assert(mat->face_indices.data[2] == 2);
	}

	{
		ufbx_mesh_part *mat = ufbxt_find_mesh_material_part(mesh, "B");
		ufbxt_assert(mat);
		ufbxt_assert(mat->num_faces == 3);
		ufbxt_assert(mat->face_indices.data[0] == 3);
		ufbxt_assert(mat->face_indices.data[1] == 4);
		ufbxt_assert(mat->face_indices.data[2] == 5);
	}
}
#endif

#if UFBXT_IMPL

static ufbx_load_opts ufbxt_split_groups_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.obj_split_groups = true;
	return opts;
}

static ufbx_load_opts ufbxt_merge_objects_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.obj_merge_objects = true;
	return opts;
}

#endif

UFBXT_FILE_TEST_OPTS_ALT(synthetic_face_groups_split, synthetic_face_groups, ufbxt_split_groups_opts)
#if UFBXT_IMPL
{
	ufbx_vec3 pos_x = { +1.0f, 0.0f, 0.0f };
	ufbx_vec3 neg_x = { -1.0f, 0.0f, 0.0f };
	ufbx_vec3 pos_y = { 0.0f, +1.0f, 0.0f };
	ufbx_vec3 neg_y = { 0.0f, -1.0f, 0.0f };
	ufbx_vec3 pos_z = { 0.0f, 0.0f, +1.0f };
	ufbx_vec3 neg_z = { 0.0f, 0.0f, -1.0f };

	ufbx_material *mat_a = ufbx_find_material(scene, "A");
	ufbx_material *mat_b = ufbx_find_material(scene, "B");

	size_t num_a_found = 0;
	size_t num_b_found = 0;
	for (size_t i = 0; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];
		if (strcmp(node->name.data, "Cube") != 0) continue;
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh);

		ufbxt_assert(mesh->materials.count == 1);
		ufbxt_assert(mesh->num_faces == 1);

		ufbx_material *material = mesh->materials.data[0];
		if (material == mat_a) {
			ufbxt_check_planar_face_ix(err, mesh, 0, pos_y, 1.0f);
			num_a_found++;
		} else {
			ufbxt_assert(material == mat_b);
			ufbxt_check_planar_face_ix(err, mesh, 0, neg_y, 1.0f);
			num_b_found++;
		}
	}

	ufbxt_assert(num_a_found == 1);
	ufbxt_assert(num_b_found == 1);

	{
		ufbx_node *node = ufbx_find_node(scene, "Front");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;

		ufbxt_assert(mesh->num_faces == 1);
		ufbxt_check_planar_face_ix(err, mesh, 0, neg_z, 1.0f);

		ufbxt_assert(mesh->materials.count == 1);
		ufbxt_assert(mesh->materials.data[0] == mat_a);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Back");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;

		ufbxt_assert(mesh->num_faces == 1);
		ufbxt_check_planar_face_ix(err, mesh, 0, pos_z, 1.0f);

		ufbxt_assert(mesh->materials.count == 1);
		ufbxt_assert(mesh->materials.data[0] == mat_b);
	}

	num_a_found = 0;
	num_b_found = 0;
	for (size_t i = 0; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];
		if (strcmp(node->name.data, "Sides") != 0) continue;
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh);

		ufbxt_assert(mesh->materials.count == 1);
		ufbxt_assert(mesh->num_faces == 1);

		ufbx_material *material = mesh->materials.data[0];
		if (material == mat_a) {
			ufbxt_check_planar_face_ix(err, mesh, 0, neg_x, 1.0f);
			num_a_found++;
		} else {
			ufbxt_assert(material == mat_b);
			ufbxt_check_planar_face_ix(err, mesh, 0, pos_x, 1.0f);
			num_b_found++;
		}
	}

	ufbxt_assert(num_a_found == 1);
	ufbxt_assert(num_b_found == 1);

}
#endif

UFBXT_FILE_TEST(synthetic_partial_material)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->materials.count == 8);
	ufbxt_assert(ufbx_find_material(scene, "A"));
	ufbxt_assert(ufbx_find_material(scene, "B"));
	ufbxt_assert(ufbx_find_material(scene, "C"));
	ufbxt_assert(ufbx_find_material(scene, "D"));
	ufbxt_assert(ufbx_find_material(scene, "E"));
	ufbxt_assert(ufbx_find_material(scene, "F"));
	ufbxt_assert(ufbx_find_material(scene, "G"));
	ufbxt_assert(ufbx_find_material(scene, "H"));

	{
		ufbx_node *node = ufbx_find_node(scene, "First");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;
		ufbxt_assert(mesh->face_material.count == 0);
		ufbxt_assert(mesh->materials.count == 0);
		ufbxt_assert(mesh->material_parts.count == 1);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Second");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;

		ufbxt_assert(mesh->materials.count == 3);
		ufbxt_assert(!strcmp(mesh->materials.data[0]->name.data, "A"));
		ufbxt_assert(!strcmp(mesh->materials.data[1]->name.data, "B"));
		ufbxt_assert(!strcmp(mesh->materials.data[2]->name.data, "D"));

		ufbxt_assert(mesh->face_material.count == 5);
		ufbxt_assert(mesh->face_material.data[0] == 0);
		ufbxt_assert(mesh->face_material.data[1] == 0);
		ufbxt_assert(mesh->face_material.data[2] == 1);
		ufbxt_assert(mesh->face_material.data[3] == 2);
		ufbxt_assert(mesh->face_material.data[4] == 0);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "Third");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;

		ufbxt_assert(mesh->materials.count == 2);
		ufbxt_assert(!strcmp(mesh->materials.data[0]->name.data, "F"));
		ufbxt_assert(!strcmp(mesh->materials.data[1]->name.data, "A"));

		ufbxt_assert(mesh->face_material.count == 2);
		ufbxt_assert(mesh->face_material.data[0] == 0);
		ufbxt_assert(mesh->face_material.data[1] == 1);
	}
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(synthetic_partial_material_merged, synthetic_partial_material, ufbxt_merge_objects_opts)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->materials.count == 8);
	ufbxt_assert(ufbx_find_material(scene, "A"));
	ufbxt_assert(ufbx_find_material(scene, "B"));
	ufbxt_assert(ufbx_find_material(scene, "C"));
	ufbxt_assert(ufbx_find_material(scene, "D"));
	ufbxt_assert(ufbx_find_material(scene, "E"));
	ufbxt_assert(ufbx_find_material(scene, "F"));
	ufbxt_assert(ufbx_find_material(scene, "G"));
	ufbxt_assert(ufbx_find_material(scene, "H"));

	ufbxt_assert(scene->nodes.count == 2);
	ufbx_node *node = scene->nodes.data[1];
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->materials.count == 4);
	ufbxt_assert(!strcmp(mesh->materials.data[0]->name.data, "A"));
	ufbxt_assert(!strcmp(mesh->materials.data[1]->name.data, "B"));
	ufbxt_assert(!strcmp(mesh->materials.data[2]->name.data, "D"));
	ufbxt_assert(!strcmp(mesh->materials.data[3]->name.data, "F"));

	ufbxt_assert(mesh->face_material.count == 8);
	ufbxt_assert(mesh->face_material.data[0] == 0);
	ufbxt_assert(mesh->face_material.data[1] == 0);
	ufbxt_assert(mesh->face_material.data[2] == 0);
	ufbxt_assert(mesh->face_material.data[3] == 1);
	ufbxt_assert(mesh->face_material.data[4] == 2);
	ufbxt_assert(mesh->face_material.data[5] == 0);
	ufbxt_assert(mesh->face_material.data[6] == 3);
	ufbxt_assert(mesh->face_material.data[7] == 0);
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_search_mtl_by_filename_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.obj_search_mtl_by_filename = true;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS_FLAGS(synthetic_filename_mtl, ufbxt_search_mtl_by_filename_opts, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS|UFBXT_FILE_TEST_FLAG_FUZZ_OPTS|UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbxt_check_warning(scene, UFBX_WARNING_MISSING_EXTERNAL_FILE, UFBX_ELEMENT_UNKNOWN, NULL, 1, "materials.mtl");
	ufbxt_check_warning(scene, UFBX_WARNING_IMPLICIT_MTL, UFBX_ELEMENT_UNKNOWN, NULL, 1, "synthetic_filename_mtl_0_obj.mtl");

	{
		ufbx_vec3 ka = { 1.0f, 0.0f, 0.0f };
		ufbx_vec3 kd = { 0.0f, 1.0f, 0.0f };
		ufbx_vec3 ks = { 0.0f, 0.0f, 1.0f };

		ufbx_material *material = ufbx_find_material(scene, "Material");
		ufbxt_assert(material);
		ufbxt_assert_close_vec3(err, material->fbx.ambient_color.value_vec3, ka);
		ufbxt_assert_close_vec3(err, material->fbx.diffuse_color.value_vec3, kd);
		ufbxt_assert_close_vec3(err, material->fbx.specular_color.value_vec3, ks);
	}

	{
		ufbx_material *material = ufbx_find_material(scene, "Other");
		ufbxt_assert(material);
	}
}
#endif

UFBXT_FILE_TEST_ALT_FLAGS(synthetic_filename_mtl_not_found, synthetic_filename_mtl, UFBXT_FILE_TEST_FLAG_ALLOW_ERROR)
#if UFBXT_IMPL
{
	ufbxt_assert(load_error->type == UFBX_ERROR_EXTERNAL_FILE_NOT_FOUND);
	ufbxt_assert(!strcmp(load_error->info, "materials.mtl"));
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_bad_unicode_mtl, UFBXT_FILE_TEST_FLAG_ALLOW_ERROR)
#if UFBXT_IMPL
{
	ufbxt_assert(load_error->type == UFBX_ERROR_EXTERNAL_FILE_NOT_FOUND);
	ufbxt_assert(!strcmp(load_error->info, "material?.mtl"));
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_missing_position_fail, UFBXT_FILE_TEST_FLAG_ALLOW_ERROR)
#if UFBXT_IMPL
{
}
#endif

UFBXT_TEST(obj_opts_mtl_data)
#if UFBXT_IMPL
{
	const char obj[] =
		"v 0 0 0\n"
		"v 1 0 0\n"
		"v 0 1 0\n"
		"usemtl Material\n"
		"f 0 1 2\n";

	const char mtl[] =
		"newmtl Material\n"
		"Ka 1 0 0\n"
		"Kd 0 1 0\n"
		"Ks 0 0 1\n";

	ufbx_load_opts opts = { 0 };
	opts.obj_mtl_data.data = mtl;
	opts.obj_mtl_data.size = sizeof(mtl) - 1;
	ufbx_scene *scene = ufbx_load_memory(obj, sizeof(obj) - 1, &opts, NULL);
	ufbxt_assert(scene);
	ufbxt_assert(scene->meshes.count == 1);
	ufbx_mesh *mesh = scene->meshes.data[0];
	ufbxt_assert(mesh->faces.count == 1);
	ufbxt_assert(mesh->materials.count == 1);
	ufbx_material *material = mesh->materials.data[0];
	ufbxt_assert(!strcmp(material->name.data, "Material"));

	ufbx_vec3 ka = { 1.0f, 0.0f, 0.0f };
	ufbx_vec3 kd = { 0.0f, 1.0f, 0.0f };
	ufbx_vec3 ks = { 0.0f, 0.0f, 1.0f };

	ufbxt_diff_error err = { 0 };
	ufbxt_assert_close_vec3(&err, material->fbx.ambient_color.value_vec3, ka);
	ufbxt_assert_close_vec3(&err, material->fbx.diffuse_color.value_vec3, kd);
	ufbxt_assert_close_vec3(&err, material->fbx.specular_color.value_vec3, ks);
	if (err.num > 0) {
		double avg = err.sum / (double)err.num;
		ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", avg, err.max, err.num);
	}

	ufbx_free_scene(scene);
}
#endif

UFBXT_TEST(obj_opts_mtl_path)
#if UFBXT_IMPL
{
	const char obj[] =
		"v 0 0 0\n"
		"v 1 0 0\n"
		"v 0 1 0\n"
		"usemtl RGB\n"
		"f 0 1 2\n";

	char buf[512];
	snprintf(buf, sizeof(buf), "%ssynthetic_simple_materials_0_mtl.mtl", data_root);

	ufbx_load_opts opts = { 0 };
	opts.obj_mtl_path.data = buf;
	opts.obj_mtl_path.length = SIZE_MAX;
	ufbx_scene *scene = ufbx_load_memory(obj, sizeof(obj) - 1, &opts, NULL);
	ufbxt_assert(scene);
	ufbxt_assert(scene->meshes.count == 1);
	ufbx_mesh *mesh = scene->meshes.data[0];
	ufbxt_assert(mesh->faces.count == 1);
	ufbxt_assert(mesh->materials.count == 1);
	ufbx_material *material = mesh->materials.data[0];
	ufbxt_assert(!strcmp(material->name.data, "RGB"));

	ufbx_vec3 ka = { 1.0f, 0.0f, 0.0f };
	ufbx_vec3 kd = { 0.0f, 1.0f, 0.0f };
	ufbx_vec3 ks = { 0.0f, 0.0f, 1.0f };

	ufbxt_diff_error err = { 0 };
	ufbxt_assert_close_vec3(&err, material->fbx.ambient_color.value_vec3, ka);
	ufbxt_assert_close_vec3(&err, material->fbx.diffuse_color.value_vec3, kd);
	ufbxt_assert_close_vec3(&err, material->fbx.specular_color.value_vec3, ks);
	if (err.num > 0) {
		double avg = err.sum / (double)err.num;
		ufbxt_logf(".. Absolute diff: avg %.3g, max %.3g (%zu tests)", avg, err.max, err.num);
	}

	ufbx_free_scene(scene);
}
#endif

UFBXT_TEST(obj_opts_no_extrnal_files)
#if UFBXT_IMPL
{
	char path[512];

	ufbxt_diff_error err = { 0 };

	ufbxt_file_iterator iter = { "blender_279_ball" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (int load_external = 0; load_external <= 1; load_external++) {
			ufbx_load_opts opts = { 0 };
			opts.load_external_files = load_external != 0;
			ufbx_scene *scene = ufbx_load_file(path, &opts, NULL);
			ufbxt_assert(scene);
			ufbxt_check_scene(scene);

			if (scene->metadata.file_format == UFBX_FILE_FORMAT_OBJ) {
				ufbx_material *red = ufbx_find_material(scene, "Red");
				ufbx_material *white = ufbx_find_material(scene, "White");
				ufbxt_assert(red);
				ufbxt_assert(white);

				if (load_external) {
					ufbx_vec3 ref_red = { 0.8f, 0.0f, 0.0f };
					ufbx_vec3 ref_white = { 0.8f, 0.8f, 0.8f };
					ufbxt_assert_close_vec3(&err, red->fbx.diffuse_color.value_vec3, ref_red);
					ufbxt_assert_close_vec3(&err, white->fbx.diffuse_color.value_vec3, ref_white);
				} else {
					ufbxt_assert(red->props.props.count == 0);
					ufbxt_assert(white->props.props.count == 0);
				}
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

UFBXT_TEST(obj_opts_no_extrnal_files_by_filename)
#if UFBXT_IMPL
{
	char path[512];
	ufbxt_diff_error err = { 0 };

	ufbxt_file_iterator iter = { "synthetic_filename_mtl" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		for (int load_by_filename = 0; load_by_filename <= 1; load_by_filename++)
		for (int load_external = 0; load_external <= 1; load_external++) {
			ufbx_load_opts opts = { 0 };
			opts.load_external_files = load_external != 0;
			opts.obj_search_mtl_by_filename = load_by_filename != 0;
			opts.ignore_missing_external_files = true;
			ufbx_scene *scene = ufbx_load_file(path, &opts, NULL);
			ufbxt_assert(scene);
			ufbxt_check_scene(scene);

			if (scene->metadata.file_format == UFBX_FILE_FORMAT_OBJ) {
				ufbx_material *material = ufbx_find_material(scene, "Material");
				ufbxt_assert(material);

				if (load_external && load_by_filename) {
					ufbxt_check_warning(scene, UFBX_WARNING_IMPLICIT_MTL, UFBX_ELEMENT_UNKNOWN, NULL, 1, "synthetic_filename_mtl_0_obj.mtl");
					ufbx_vec3 ka = { 1.0f, 0.0f, 0.0f };
					ufbx_vec3 kd = { 0.0f, 1.0f, 0.0f };
					ufbx_vec3 ks = { 0.0f, 0.0f, 1.0f };
					ufbxt_assert_close_vec3(&err, material->fbx.ambient_color.value_vec3, ka);
					ufbxt_assert_close_vec3(&err, material->fbx.diffuse_color.value_vec3, kd);
					ufbxt_assert_close_vec3(&err, material->fbx.specular_color.value_vec3, ks);
				} else {
					if (load_external && !load_by_filename) {
						ufbxt_check_warning(scene, UFBX_WARNING_MISSING_EXTERNAL_FILE, UFBX_ELEMENT_UNKNOWN, NULL, 1, "materials.mtl");

					}
					ufbxt_assert(material->props.props.count == 0);
				}
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

#if UFBXT_IMPL
static void ufbxt_check_only_texture(ufbx_material_map *map, const char *filename)
{
	ufbxt_assert(!map->has_value);
	ufbxt_assert(map->texture);
	ufbxt_assert(!strcmp(map->texture->relative_filename.data, filename));
}
#endif

UFBXT_FILE_TEST(synthetic_map_feature)
#if UFBXT_IMPL
{
	{
		ufbx_material *material = ufbx_find_material(scene, "Phong");
		ufbxt_assert(material->shader_type == UFBX_SHADER_WAVEFRONT_MTL);

		ufbxt_check_only_texture(&material->fbx.diffuse_color, "diffuse.png");
		ufbxt_check_only_texture(&material->fbx.specular_color, "specular.png");

		ufbxt_assert(!material->features.pbr.enabled);
		ufbxt_assert(!material->features.sheen.enabled);
		ufbxt_assert(!material->features.coat.enabled);
		ufbxt_assert(!material->features.metalness.enabled);
		ufbxt_assert(!material->features.ior.enabled);
		ufbxt_assert(!material->features.opacity.enabled);
		ufbxt_assert(!material->features.transmission.enabled);
		ufbxt_assert(!material->features.emission.enabled);
	}

	{
		ufbx_material *material = ufbx_find_material(scene, "PBR");
		ufbxt_assert(material->shader_type == UFBX_SHADER_WAVEFRONT_MTL);

		ufbxt_check_only_texture(&material->pbr.roughness, "roughness.png");
		ufbxt_check_only_texture(&material->pbr.metalness, "metalness.png");

		ufbxt_assert(material->features.pbr.enabled);
		ufbxt_assert(!material->features.sheen.enabled);
		ufbxt_assert(!material->features.coat.enabled);
		ufbxt_assert(material->features.metalness.enabled);
		ufbxt_assert(!material->features.ior.enabled);
		ufbxt_assert(!material->features.opacity.enabled);
		ufbxt_assert(!material->features.transmission.enabled);
		ufbxt_assert(!material->features.emission.enabled);
	}

	{
		ufbx_material *material = ufbx_find_material(scene, "Extended");
		ufbxt_assert(material->shader_type == UFBX_SHADER_WAVEFRONT_MTL);

		ufbxt_check_only_texture(&material->pbr.sheen_color, "sheen.png");
		ufbxt_check_only_texture(&material->pbr.coat_factor, "coat.png");
		ufbxt_check_only_texture(&material->pbr.metalness, "metalness.png");
		ufbxt_check_only_texture(&material->pbr.specular_ior, "ior.png");
		ufbxt_check_only_texture(&material->pbr.opacity, "opacity.png");
		ufbxt_check_only_texture(&material->pbr.transmission_color, "transmission.png");
		ufbxt_check_only_texture(&material->pbr.emission_color, "emission.png");

		ufbxt_assert(material->features.pbr.enabled);
		ufbxt_assert(material->features.sheen.enabled);
		ufbxt_assert(material->features.coat.enabled);
		ufbxt_assert(material->features.metalness.enabled);
		ufbxt_assert(material->features.ior.enabled);
		ufbxt_assert(material->features.opacity.enabled);
		ufbxt_assert(material->features.transmission.enabled);
		ufbxt_assert(material->features.emission.enabled);
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(blender_340_line_point, UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->metadata.has_warning[UFBX_WARNING_INDEX_CLAMPED]);

	ufbx_node *node = ufbx_find_node(scene, "Cube");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 6);
	ufbxt_assert(mesh->num_empty_faces == 0);
	ufbxt_assert(mesh->num_point_faces == 0);
	ufbxt_assert(mesh->num_line_faces == 5);
}
#endif

UFBXT_FILE_TEST(synthetic_extended_line)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Line");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 4);
	ufbxt_assert(mesh->num_empty_faces == 0);
	ufbxt_assert(mesh->num_point_faces == 0);
	ufbxt_assert(mesh->num_line_faces == 4);
	ufbxt_assert(mesh->num_indices == 8);

	ufbx_vec3 pos_ref[] = {
		{ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f },
		{ 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },
	};

	ufbx_vec2 uv_ref[] = {
		{ 0.0f, 0.0f }, { 2.0f, 0.0f },
		{ 2.0f, 0.0f }, { 0.0f, 2.0f },
		{ 0.0f, 2.0f }, { 2.0f, 2.0f },
		{ 2.0f, 2.0f }, { 0.0f, 0.0f },
	};

	ufbx_vec4 col_ref[] = {
		{ 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f },
	};

	for (size_t i = 0; i < mesh->num_indices; i++) {
		ufbxt_hintf("i=%zu", i);
		ufbxt_assert_close_vec3(err, pos_ref[i], ufbx_get_vertex_vec3(&mesh->vertex_position, i));
		ufbxt_assert_close_vec2(err, uv_ref[i], ufbx_get_vertex_vec2(&mesh->vertex_uv, i));
		ufbxt_assert_close_vec4(err, col_ref[i], ufbx_get_vertex_vec4(&mesh->vertex_color, i));
	}
}
#endif

UFBXT_FILE_TEST(synthetic_extended_points)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Points");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 3);
	ufbxt_assert(mesh->num_empty_faces == 0);
	ufbxt_assert(mesh->num_point_faces == 3);
	ufbxt_assert(mesh->num_line_faces == 0);
	ufbxt_assert(mesh->num_indices == 3);

	ufbx_vec3 pos_ref[] = {
		{ 0.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
	};

	ufbx_vec4 col_ref[] = {
		{ 1.0f, 0.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f, 1.0f },
	};

	for (size_t i = 0; i < mesh->num_indices; i++) {
		ufbxt_hintf("i=%zu", i);
		ufbxt_assert_close_vec3(err, pos_ref[i], ufbx_get_vertex_vec3(&mesh->vertex_position, i));
		ufbxt_assert_close_vec4(err, col_ref[i], ufbx_get_vertex_vec4(&mesh->vertex_color, i));
	}
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_empty_face, UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbxt_check_warning(scene, UFBX_WARNING_EMPTY_FACE_REMOVED, UFBX_ELEMENT_UNKNOWN, NULL, 1, NULL);

	ufbx_node *node = ufbx_find_node(scene, "Object");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 0);
	ufbxt_assert(mesh->num_empty_faces == 0);
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_allow_empty_faces_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.allow_empty_faces = true;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(synthetic_empty_face_allow, synthetic_empty_face, ufbxt_allow_empty_faces_opts)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Object");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 1);
	ufbxt_assert(mesh->num_empty_faces == 1);

	ufbxt_assert(mesh->faces.data[0].index_begin == 0);
	ufbxt_assert(mesh->faces.data[0].num_indices == 0);
}
#endif

UFBXT_FILE_TEST_ALT_FLAGS(obj_space_default, blender_279_ball, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS)
#if UFBXT_IMPL
{
	if (scene->metadata.file_format == UFBX_FILE_FORMAT_OBJ) {
		ufbxt_assert(scene->settings.axes.right == UFBX_COORDINATE_AXIS_UNKNOWN);
		ufbxt_assert(scene->settings.axes.up == UFBX_COORDINATE_AXIS_UNKNOWN);
		ufbxt_assert(scene->settings.axes.front == UFBX_COORDINATE_AXIS_UNKNOWN);
		ufbxt_assert(scene->settings.unit_meters == 0.0f);
		ufbxt_assert(scene->settings.original_unit_meters == 0.0f);
	} else {
		ufbxt_assert(scene->settings.axes.right == UFBX_COORDINATE_AXIS_POSITIVE_X);
		ufbxt_assert(scene->settings.axes.up == UFBX_COORDINATE_AXIS_POSITIVE_Y);
		ufbxt_assert(scene->settings.axes.front == UFBX_COORDINATE_AXIS_POSITIVE_Z);
		ufbxt_assert(scene->settings.unit_meters == (ufbx_real)0.01);
		ufbxt_assert(scene->settings.original_unit_meters == (ufbx_real)0.01);
	}
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts obj_axes_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.obj_unit_meters = (ufbx_real)0.1;
	opts.obj_axes = ufbx_axes_right_handed_z_up;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(obj_space_manual_axes, blender_279_ball, obj_axes_opts, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS)
#if UFBXT_IMPL
{
	if (scene->metadata.file_format == UFBX_FILE_FORMAT_OBJ) {
		ufbxt_assert(scene->settings.axes.right == UFBX_COORDINATE_AXIS_POSITIVE_X);
		ufbxt_assert(scene->settings.axes.up == UFBX_COORDINATE_AXIS_POSITIVE_Z);
		ufbxt_assert(scene->settings.axes.front == UFBX_COORDINATE_AXIS_NEGATIVE_Y);
		ufbxt_assert(scene->settings.unit_meters == (ufbx_real)0.1);
		ufbxt_assert(scene->settings.original_unit_meters == (ufbx_real)0.1);
	} else {
		ufbxt_assert(scene->settings.axes.right == UFBX_COORDINATE_AXIS_POSITIVE_X);
		ufbxt_assert(scene->settings.axes.up == UFBX_COORDINATE_AXIS_POSITIVE_Y);
		ufbxt_assert(scene->settings.axes.front == UFBX_COORDINATE_AXIS_POSITIVE_Z);
		ufbxt_assert(scene->settings.unit_meters == (ufbx_real)0.01);
		ufbxt_assert(scene->settings.original_unit_meters == (ufbx_real)0.01);
	}
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts obj_axes_convert_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.obj_unit_meters = (ufbx_real)0.1;
	opts.obj_axes = ufbx_axes_right_handed_z_up;
	opts.target_unit_meters = 1.0f;
	opts.target_axes = ufbx_axes_right_handed_y_up;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS_ALT_FLAGS(obj_space_manual_convert, blender_279_ball, obj_axes_convert_opts, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS)
#if UFBXT_IMPL
{
	if (scene->metadata.file_format == UFBX_FILE_FORMAT_OBJ) {
		ufbxt_assert(scene->settings.axes.right == UFBX_COORDINATE_AXIS_POSITIVE_X);
		ufbxt_assert(scene->settings.axes.up == UFBX_COORDINATE_AXIS_POSITIVE_Z);
		ufbxt_assert(scene->settings.axes.front == UFBX_COORDINATE_AXIS_NEGATIVE_Y);
		ufbxt_assert(scene->settings.unit_meters == (ufbx_real)0.1);
		ufbxt_assert(scene->settings.original_unit_meters == (ufbx_real)0.1);

		ufbx_node *node = scene->root_node;
		ufbx_vec3 scale = { 0.1f, 0.1f, 0.1f };
		ufbx_vec3 euler = { -90.0f, 0.0f, 0.0f };
		ufbx_quat rotation = ufbx_euler_to_quat(euler, UFBX_ROTATION_ORDER_XYZ);

		ufbxt_assert_close_vec3(err, node->local_transform.scale, scale);
		ufbxt_assert_close_quat(err, node->local_transform.rotation, rotation);
	} else {
		ufbxt_assert(scene->settings.axes.right == UFBX_COORDINATE_AXIS_POSITIVE_X);
		ufbxt_assert(scene->settings.axes.up == UFBX_COORDINATE_AXIS_POSITIVE_Y);
		ufbxt_assert(scene->settings.axes.front == UFBX_COORDINATE_AXIS_POSITIVE_Z);
		ufbxt_assert(scene->settings.unit_meters == (ufbx_real)0.01);
		ufbxt_assert(scene->settings.original_unit_meters == (ufbx_real)0.01);
	}
}
#endif

UFBXT_FILE_TEST(blender_441_empty_material)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Cube_Cube");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->materials.count == 1);
	for (size_t i = 0; i < mesh->face_material.count; i++) {
		uint32_t index = mesh->face_material.data[i];
		ufbxt_assert(index == 0);
	}
}
#endif

UFBXT_FILE_TEST(synthetic_empty_material)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Cube");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->materials.count == 0);
	ufbxt_assert(mesh->face_material.count == 0);
}
#endif

