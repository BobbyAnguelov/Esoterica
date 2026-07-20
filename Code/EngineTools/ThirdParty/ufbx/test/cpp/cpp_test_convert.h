
UFBXT_CPP_TEST(find_node_string)
{
    ufbx_unique_ptr<ufbx_scene> scene{ufbx_load_file(data_path("maya_cube_7500_ascii.fbx"), nullptr, nullptr)};

    std::string name = "pCube1";
    std::string non_name = "pCube2";

    ufbxt_assert(ufbx_find_node(scene.get(), name) != nullptr);
    ufbxt_assert(ufbx_find_node(scene.get(), non_name) == nullptr);

    ufbx_node *node = ufbx_find_node(scene.get(), "pCube1");
    ufbxt_assert(node);
    ufbxt_assert(std::string(node->name) == "pCube1");
}

#if defined(__cpp_lib_string_view)
UFBXT_CPP_TEST(find_node_string_view)
{
    ufbx_unique_ptr<ufbx_scene> scene{ufbx_load_file(data_path("maya_cube_7500_ascii.fbx"), nullptr, nullptr)};

    std::string_view name = "pCube1";
    std::string_view non_name = "pCube2";

    ufbxt_assert(ufbx_find_node(scene.get(), name) != nullptr);
    ufbxt_assert(ufbx_find_node(scene.get(), non_name) == nullptr);
}
#endif

struct vec3f { float x, y, z; };
struct quatf { float x, y, z, w; };
struct transformf {
    vec3f translation;
    quatf rotation;
    vec3f scale;
};

template <> struct ufbx_converter<vec3f> {
    static inline vec3f from(const ufbx_vec3 &v) { return { (float)v.x, (float)v.y, (float)v.z }; }
};

template <> struct ufbx_converter<quatf> {
    static inline quatf from(const ufbx_quat &v) { return { (float)v.x, (float)v.y, (float)v.z, (float)v.w }; }
};

template <> struct ufbx_converter<transformf> {
    static inline transformf from(const ufbx_transform &v) { return transformf{ v.translation, v.rotation, v.scale }; }
};

UFBXT_CPP_TEST(convert_transform)
{
    ufbx_unique_ptr<ufbx_scene> scene{ufbx_load_file(data_path("maya_cube_7500_ascii.fbx"), nullptr, nullptr)};
    ufbx_node *cube = ufbx_find_node(scene.get(), "pCube1");
    transformf transform = cube->local_transform;
}

template <typename T>
struct array_view { T *data; size_t size; };

template <typename T> struct ufbx_converter<array_view<T>> {
    static inline array_view<T> from_list(T *ts, size_t count) { return { ts, count }; }
};

UFBXT_CPP_TEST(convert_list)
{
    ufbx_unique_ptr<ufbx_scene> scene{ufbx_load_file(data_path("maya_cube_7500_ascii.fbx"), nullptr, nullptr)};
    array_view<ufbx_element*> elements = scene->elements;
}

UFBXT_CPP_TEST(find_prop_element_string)
{
    ufbx_unique_ptr<ufbx_scene> scene{ufbx_load_file(data_path("motionbuilder_lights_7700_ascii.fbx"), nullptr, nullptr)};

    ufbx_node *node = ufbx_find_node(scene.get(), "SpotLook");
    ufbxt_assert(node);

    std::string name = "LookAtProperty";
    std::string non_name = "pCube2";

    ufbx_node *look_at = (ufbx_node*)ufbx_find_prop_element(&node->element, name, UFBX_ELEMENT_NODE);
    ufbxt_assert(std::string(look_at->name) == "Cube");
}
