#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "triangulate"

#if UFBXT_IMPL
size_t do_triangulate_test(ufbx_scene *scene)
{
	size_t num_fail = 0;

	for (size_t mesh_ix = 0; mesh_ix < scene->meshes.count; mesh_ix++) {
		ufbx_mesh *mesh = scene->meshes.data[mesh_ix];
		ufbxt_assert(mesh->instances.count == 1);
		bool should_be_top_left = mesh->instances.data[0]->name.data[0] == 'A';
		ufbxt_assert(mesh->num_faces == 1);
		ufbx_face face = mesh->faces.data[0];
		ufbxt_assert(face.index_begin == 0);
		ufbxt_assert(face.num_indices == 4);
		uint32_t tris[6];
		bool ok = ufbx_triangulate_face(tris, 6, mesh, face);
		ufbxt_assert(ok);

		size_t top_left_ix = 0;
		ufbx_real best_dot = HUGE_VALF;
		for (size_t ix = 0; ix < 4; ix++) {
			ufbx_vec3 v = ufbx_get_vertex_vec3(&mesh->vertex_position, ix);
			ufbx_real dot = v.x + v.z;
			if (dot < best_dot) {
				top_left_ix = ix;
				best_dot = dot;
			}
		}

		uint32_t top_left_count = 0;
		for (size_t i = 0; i < 6; i++) {
			if (tris[i] == top_left_ix) top_left_count++;
		}

		if (should_be_top_left != (top_left_count == 2)) {
			ufbxt_logf("Fail: %s", mesh->instances.data[0]->name.data);
			num_fail++;
		}
	}

	ufbxt_logf("Triangulations OK: %zu/%zu", scene->meshes.count - num_fail, scene->meshes.count);
	return num_fail;
}
#endif

UFBXT_FILE_TEST(maya_triangulate)
#if UFBXT_IMPL
{
	size_t num_fail = do_triangulate_test(scene);
	ufbxt_assert(num_fail <= 4);
}
#endif

UFBXT_FILE_TEST(maya_triangulate_down)
#if UFBXT_IMPL
{
	size_t num_fail = do_triangulate_test(scene);
	ufbxt_assert(num_fail <= 1);
}
#endif

UFBXT_FILE_TEST(maya_tri_cone)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCone1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	for (size_t i = 0; i < mesh->num_faces; i++) {
		ufbx_face face = mesh->faces.data[i];
		ufbxt_assert(face.num_indices >= 3 && face.num_indices <= 32);

		uint32_t tris[32];

		size_t num_tris = face.num_indices - 2;
		for (size_t i = 0; i < 32; i++) {
			ufbx_panic panic;
			panic.did_panic = false;
			size_t num_tris_returned = ufbx_catch_triangulate_face(&panic, tris, i, mesh, face);
			if (i >= num_tris * 3) {
				ufbxt_assert(!panic.did_panic);
				ufbxt_assert(num_tris_returned == num_tris);
			} else {
				ufbxt_assert(panic.did_panic);
				ufbxt_assert(num_tris_returned == 0);
			}
		}

		ufbxt_assert(ufbx_triangulate_face(tris, ufbxt_arraycount(tris), mesh, face));
	}
}
#endif

#if UFBXT_IMPL

static void ufbxt_ngon_write_obj(const char *path, ufbx_mesh *mesh, const uint32_t *indices, size_t num_triangles)
{
	FILE *f = fopen(path, "w");

	for (size_t i = 0; i < mesh->num_vertices; i++) {
		ufbx_vec3 v = mesh->vertices.data[i];
		fprintf(f, "v %f %f %f\n", v.x, v.y, v.z);
	}

	fprintf(f, "\n");
	for (size_t i = 0; i < num_triangles; i++) {
		const uint32_t *tri = indices + i * 3;
		fprintf(f, "f %u %u %u\n",
			mesh->vertex_indices.data[tri[0]] + 1,
			mesh->vertex_indices.data[tri[1]] + 1,
			mesh->vertex_indices.data[tri[2]] + 1);
	}

	fclose(f);
}

static ufbx_vec2 ufbxt_ngon_3d_to_2d(const ufbx_vec3 basis[2], ufbx_vec3 pos)
{
	ufbx_vec2 uv;
	uv.x = ufbxt_dot3(basis[0], pos);
	uv.y = ufbxt_dot3(basis[1], pos);
	return uv;
}

static void ufbxt_check_ngon_triangulation(ufbxt_diff_error *err, ufbx_mesh *mesh, ufbx_face face, uint32_t *indices, size_t num_triangles, const ufbx_vec3 basis[2])
{
	// Check that the area matches for now
	// TODO: More rigorous tests

	ufbx_real poly_area = 0.0f;
	for (size_t i = 0; i < face.num_indices; i++) {
		ufbx_vec2 a = ufbxt_ngon_3d_to_2d(basis, ufbx_get_vertex_vec3(&mesh->vertex_position, face.index_begin + i + 0));
		ufbx_vec2 b = ufbxt_ngon_3d_to_2d(basis, ufbx_get_vertex_vec3(&mesh->vertex_position, face.index_begin + (i + 1) % face.num_indices));
		poly_area += 0.5f * (a.x*b.y - a.y*b.x);
	}

	ufbx_real tri_area = 0.0f;
	for (size_t i = 0; i < num_triangles; i++) {
		ufbx_vec2 a = ufbxt_ngon_3d_to_2d(basis, ufbx_get_vertex_vec3(&mesh->vertex_position, indices[i*3 + 0]));
		ufbx_vec2 b = ufbxt_ngon_3d_to_2d(basis, ufbx_get_vertex_vec3(&mesh->vertex_position, indices[i*3 + 1]));
		ufbx_vec2 c = ufbxt_ngon_3d_to_2d(basis, ufbx_get_vertex_vec3(&mesh->vertex_position, indices[i*3 + 2]));
		ufbx_real area = 0.5f * (a.x*(b.y-c.y) + b.x*(c.y-a.y) + c.x*(a.y-b.y));
		ufbxt_assert(area >= -0.05f);
		tri_area += area;
	}

	#if defined(UFBX_REAL_IS_FLOAT)
		ufbxt_assert_close_real_threshold(err, poly_area, tri_area, (ufbx_real)fabs(poly_area) * 0.001f);
	#else
		ufbxt_assert_close_real(err, poly_area, tri_area);
	#endif
}

#endif

UFBXT_FILE_TEST(blender_300_ngon_intersection)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Plane");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 1);
	ufbx_face face = mesh->faces.data[0];

	uint32_t indices[3*3];
	size_t num_tris = ufbx_triangulate_face(indices, ufbxt_arraycount(indices), mesh, face);
	ufbxt_assert(num_tris == 3);

	const ufbx_vec3 basis[2] = {
		{ { 1.0f, 0.0f, 0.0f } },
		{ { 0.0f, 1.0f, 0.0f } },
	};
	ufbxt_check_ngon_triangulation(err, mesh, face, indices, num_tris, basis);
}
#endif

UFBXT_FILE_TEST(blender_300_ngon_e)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Plane");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 1);
	ufbx_face face = mesh->faces.data[0];

	uint32_t indices[10*3];
	size_t num_tris = ufbx_triangulate_face(indices, ufbxt_arraycount(indices), mesh, face);
	ufbxt_assert(num_tris == 10);

	const ufbx_vec3 basis[2] = {
		{ { 1.0f, 0.0f, 0.0f } },
		{ { 0.0f, 1.0f, 0.0f } },
	};
	ufbxt_check_ngon_triangulation(err, mesh, face, indices, num_tris, basis);
}
#endif

UFBXT_FILE_TEST(blender_300_ngon_abstract)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Plane");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 1);
	ufbx_face face = mesh->faces.data[0];

	uint32_t indices[144*3];
	size_t num_tris = ufbx_triangulate_face(indices, ufbxt_arraycount(indices), mesh, face);
	ufbxt_assert(num_tris == 144);

	const ufbx_vec3 basis[2] = {
		{ { 1.0f, 0.0f, 0.0f } },
		{ { 0.0f, 1.0f, 0.0f } },
	};
	ufbxt_check_ngon_triangulation(err, mesh, face, indices, num_tris, basis);
}
#endif

UFBXT_FILE_TEST(blender_300_ngon_big)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Plane");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbxt_assert(mesh->num_faces == 1);
	ufbx_face face = mesh->faces.data[0];

	uint32_t expected_tris = 8028;
	uint32_t *indices = malloc(expected_tris * 3 * sizeof(uint32_t));
	ufbxt_assert(indices);

	size_t num_tris = ufbx_triangulate_face(indices, expected_tris * 3, mesh, face);
	ufbxt_assert(num_tris == expected_tris);

	const ufbx_vec3 basis[2] = {
		{ { 1.0f, 0.0f, 0.0f } },
		{ { 0.0f, 1.0f, 0.0f } },
	};
	ufbxt_check_ngon_triangulation(err, mesh, face, indices, num_tris, basis);

	free(indices);
}
#endif

UFBXT_FILE_TEST(blender_300_ngon_irregular)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Plane");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	uint32_t indices[256];

	for (size_t i = 0; i < mesh->num_faces; i++) {
		ufbx_face face = mesh->faces.data[i];

		size_t num_tris = ufbx_triangulate_face(indices, ufbxt_arraycount(indices), mesh, face);
		ufbxt_assert(num_tris == face.num_indices - 2);
	}
}
#endif

UFBXT_FILE_TEST_ALT(triangulate_empty, maya_cube_7500_binary)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh);
	ufbx_mesh *mesh = node->mesh;

	ufbx_face empty_face = { 0, 0 };
	uint32_t indices[16];
	uint32_t result = ufbx_triangulate_face(indices, 16, mesh, empty_face);
	ufbxt_assert(result == 0);
}
#endif

UFBXT_FILE_TEST(maya_ngon_gs)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "G");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;

		ufbxt_assert(mesh->num_faces == 1);
		ufbx_face face = mesh->faces.data[0];

		uint32_t indices[128*3];
		size_t num_tris = ufbx_triangulate_face(indices, ufbxt_arraycount(indices), mesh, face);
		ufbxt_assert(num_tris == 126);

		const ufbx_vec3 basis[2] = {
			{ { 1.0f, 0.0f, 0.0f } },
			{ { 0.0f, 1.0f, 0.0f } },
		};
		ufbxt_check_ngon_triangulation(err, mesh, face, indices, num_tris, basis);
	}

	{
		ufbx_node *node = ufbx_find_node(scene, "S");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;

		ufbxt_assert(mesh->num_faces == 1);
		ufbx_face face = mesh->faces.data[0];

		uint32_t indices[256*3];
		size_t num_tris = ufbx_triangulate_face(indices, ufbxt_arraycount(indices), mesh, face);
		ufbxt_assert(num_tris == 198);

		const ufbx_vec3 basis[2] = {
			{ { 1.0f, 0.0f, 0.0f } },
			{ { 0.0f, 1.0f, 0.0f } },
		};
		ufbxt_check_ngon_triangulation(err, mesh, face, indices, num_tris, basis);
	}
}
#endif

UFBXT_FILE_TEST(maya_ngon_maze_segment)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "Grid");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;

		ufbxt_assert(mesh->num_faces == 1);
		ufbx_face face = mesh->faces.data[0];

		uint32_t indices[64*3];
		size_t num_tris = ufbx_triangulate_face(indices, ufbxt_arraycount(indices), mesh, face);
		ufbxt_assert(num_tris == 60);

		const ufbx_vec3 basis[2] = {
			{ { 1.0f, 0.0f, 0.0f } },
			{ { 0.0f, 1.0f, 0.0f } },
		};
		ufbxt_check_ngon_triangulation(err, mesh, face, indices, num_tris, basis);
	}
}
#endif

UFBXT_FILE_TEST(maya_ngon_l)
#if UFBXT_IMPL
{
	{
		ufbx_node *node = ufbx_find_node(scene, "Grid");
		ufbxt_assert(node && node->mesh);
		ufbx_mesh *mesh = node->mesh;

		ufbxt_assert(mesh->num_faces == 1);
		ufbx_face face = mesh->faces.data[0];

		uint32_t indices[8*3];
		size_t num_tris = ufbx_triangulate_face(indices, ufbxt_arraycount(indices), mesh, face);
		ufbxt_assert(num_tris == 8);

		const ufbx_vec3 basis[2] = {
			{ { 1.0f, 0.0f, 0.0f } },
			{ { 0.0f, 1.0f, 0.0f } },
		};
		ufbxt_check_ngon_triangulation(err, mesh, face, indices, num_tris, basis);
	}
}
#endif
