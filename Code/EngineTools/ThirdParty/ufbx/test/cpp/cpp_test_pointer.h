
UFBXT_CPP_TEST(test_unique_ptr)
{
    ufbx_unique_ptr<ufbx_scene> scene{ufbx_load_file(data_path("maya_cube_7500_ascii.fbx"), nullptr, nullptr)};
	ufbxt_assert(scene);
    ufbx_unique_ptr<ufbx_scene> scene2 = std::move(scene);
    ufbx_unique_ptr<ufbx_scene> scene3;
	ufbxt_assert(!scene3);
    scene3 = std::move(scene2);
}

UFBXT_CPP_TEST(test_shared_ptr)
{
    ufbx_shared_ptr<ufbx_scene> scene{ufbx_load_file(data_path("maya_cube_7500_ascii.fbx"), nullptr, nullptr)};
	ufbxt_assert(scene);
    ufbx_shared_ptr<ufbx_scene> scene2 = scene;
    ufbx_shared_ptr<ufbx_scene> scene3 = std::move(scene2);
    ufbx_shared_ptr<ufbx_scene> scene4;
	ufbxt_assert(!scene4);
    scene4 = scene3;
    scene4 = scene4;
}

UFBXT_CPP_TEST(test_unique_deleter)
{
    std::unique_ptr<ufbx_scene, ufbx_deleter> scene{ufbx_load_file(data_path("maya_cube_7500_ascii.fbx"), nullptr, nullptr)};
	ufbxt_assert(scene);
    std::unique_ptr<ufbx_scene, ufbx_deleter> scene2 = std::move(scene);
}

UFBXT_CPP_TEST(test_shared_deleter)
{
    std::shared_ptr<ufbx_scene> scene{ufbx_load_file(data_path("maya_cube_7500_ascii.fbx"), nullptr, nullptr), ufbx_deleter{}};
	ufbxt_assert(scene);
    std::shared_ptr<ufbx_scene> scene2 = scene;
    std::shared_ptr<ufbx_scene> scene3 = std::move(scene2);
}

UFBXT_CPP_TEST(test_mesh_ptr)
{
    ufbx_unique_ptr<ufbx_scene> scene{ufbx_load_file(data_path("maya_cube_7500_ascii.fbx"), nullptr, nullptr)};
	ufbxt_assert(scene);

    ufbx_node *node = ufbx_find_node(scene.get(), "pCube1");
    ufbxt_assert(node && node->mesh);
    ufbx_mesh *mesh = node->mesh;

    ufbx_unique_ptr<ufbx_mesh> mesh1{ufbx_subdivide_mesh(mesh, 1, nullptr, nullptr)};
    ufbx_shared_ptr<ufbx_mesh> mesh2{ufbx_subdivide_mesh(mesh, 1, nullptr, nullptr)};
    ufbx_shared_ptr<ufbx_mesh> mesh3{mesh2};
    std::unique_ptr<ufbx_mesh, ufbx_deleter> mesh4{ufbx_subdivide_mesh(mesh, 1, nullptr, nullptr)};
    ufbxt_assert(mesh1);
    ufbxt_assert(mesh2);
    ufbxt_assert(mesh3);
    ufbxt_assert(mesh4);
}

UFBXT_CPP_TEST(test_line_curve_ptr)
{
    ufbx_unique_ptr<ufbx_scene> scene{ufbx_load_file(data_path("maya_nurbs_curve_form_7700_ascii.fbx"), nullptr, nullptr)};
	ufbxt_assert(scene);

    ufbx_node *node = ufbx_find_node(scene.get(), "circleOpen");
    ufbx_nurbs_curve *curve = ufbx_as_nurbs_curve(node->attrib);

    ufbx_unique_ptr<ufbx_line_curve> line1{ufbx_tessellate_nurbs_curve(curve, nullptr, nullptr)};
    ufbx_shared_ptr<ufbx_line_curve> line2{ufbx_tessellate_nurbs_curve(curve, nullptr, nullptr)};
    ufbx_shared_ptr<ufbx_line_curve> line3{line2};
    std::unique_ptr<ufbx_line_curve, ufbx_deleter> line4{ufbx_tessellate_nurbs_curve(curve, nullptr, nullptr)};
    ufbxt_assert(line1);
    ufbxt_assert(line2);
    ufbxt_assert(line3);
    ufbxt_assert(line4);
}

UFBXT_CPP_TEST(test_geometry_cache_ptr)
{
    ufbx_unique_ptr<ufbx_geometry_cache> cache1{ufbx_load_geometry_cache(data_path("marvelous_quad.xml"), nullptr, nullptr)};
    ufbx_shared_ptr<ufbx_geometry_cache> cache2{ufbx_load_geometry_cache(data_path("marvelous_quad.xml"), nullptr, nullptr)};
    ufbx_shared_ptr<ufbx_geometry_cache> cache3{cache2};
    std::unique_ptr<ufbx_geometry_cache, ufbx_deleter> cache4{ufbx_load_geometry_cache(data_path("marvelous_quad.xml"), nullptr, nullptr)};
	ufbxt_assert(cache1);
	ufbxt_assert(cache2);
	ufbxt_assert(cache3);
	ufbxt_assert(cache4);
}

UFBXT_CPP_TEST(test_anim_ptr)
{
    ufbx_unique_ptr<ufbx_scene> scene{ufbx_load_file(data_path("maya_cube_7500_ascii.fbx"), nullptr, nullptr)};
	ufbxt_assert(scene);

    std::vector<uint32_t> layers = { scene->anim_layers[0]->typed_id };

    ufbx_anim_opts opts = { };
    opts.layer_ids = { layers.data(), layers.size() };

    ufbx_unique_ptr<ufbx_anim> anim1{ufbx_create_anim(scene.get(), &opts, nullptr)};
    ufbx_shared_ptr<ufbx_anim> anim2{ufbx_create_anim(scene.get(), &opts, nullptr)};
    ufbx_shared_ptr<ufbx_anim> anim3{anim2};
    std::unique_ptr<ufbx_anim, ufbx_deleter> anim4{ufbx_create_anim(scene.get(), &opts, nullptr)};
    ufbxt_assert(anim1);
    ufbxt_assert(anim2);
    ufbxt_assert(anim3);
    ufbxt_assert(anim4);
}
