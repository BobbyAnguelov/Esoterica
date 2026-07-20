#include "../ufbx.h"

int main(int argc, char **argv)
{
    (void)argc;
    ufbx_scene *scene = ufbx_load_file(argv[1], NULL, NULL);
    ufbx_geometry_cache *cache = ufbx_load_geometry_cache(argv[1], NULL, NULL);

    ufbx_mesh *mesh = scene->meshes.data[0];
    ufbx_nurbs_curve *curve = scene->nurbs_curves.data[0];
    ufbx_nurbs_surface *surface = scene->nurbs_surfaces.data[0];

    uint32_t indices[32];
    ufbx_triangulate_face(indices, 32, mesh, mesh->faces.data[0]);

    char vertices[32] = { 0 };
    ufbx_vertex_stream stream = { vertices, 32, 1 };
    ufbx_generate_indices(&stream, 1, indices, 32, NULL, NULL);

    ufbx_free_scene(ufbx_evaluate_scene(scene, NULL, 0.0, NULL, NULL));
    ufbx_free_mesh(ufbx_subdivide_mesh(mesh, 1, NULL, NULL));
    ufbx_free_mesh(ufbx_tessellate_nurbs_surface(surface, NULL, NULL));
    ufbx_free_line_curve(ufbx_tessellate_nurbs_curve(curve, NULL, NULL));

    ufbx_free_scene(scene);
    ufbx_free_geometry_cache(cache);
}
