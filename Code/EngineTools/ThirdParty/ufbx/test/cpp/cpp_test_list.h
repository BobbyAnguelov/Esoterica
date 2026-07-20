
UFBXT_CPP_TEST(list)
{
    ufbx_unique_ptr<ufbx_scene> scene{ufbx_load_file(data_path("blender_293_instancing_7400_binary.fbx"), nullptr, nullptr)};

    size_t count = 0;
    for (ufbx_node *node : scene->nodes) {
        ufbxt_assert(node == scene->nodes[count]);
        ++count;
    }
    ufbxt_assert(count == 9);
}
