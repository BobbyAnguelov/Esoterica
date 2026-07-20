#include "../ufbx.h"
#include <stddef.h>
#include <string.h>

int main() {
    ufbx_load_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.target_axes = ufbx_axes_right_handed_y_up;
    ufbx_scene *scene = ufbx_load_file("test.fbx", &opts, NULL);
    if (!scene) return 1;
    ufbx_free_scene(scene);
    return 0;
}
