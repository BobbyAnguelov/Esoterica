#ifndef UFBXT_TESTING_FUZZ_INCLUDED
#define UFBXT_TESTING_FUZZ_INCLUDED

#include <stdint.h>

typedef struct ufbxt_fuzz_header {
	char magic[8];
	uint32_t version;
	uint32_t flags;
	uint32_t fbx_version;
	uint32_t fbx_format;
	char unused[32];
} ufbxt_fuzz_header;

static void ufbxt_fuzz_apply_flags(ufbx_load_opts *opts, uint32_t flags)
{
	opts->file_format = UFBX_FILE_FORMAT_FBX;
	opts->result_allocator.memory_limit = UINT64_C(4000000000);
	opts->temp_allocator.memory_limit = UINT64_C(4000000000);

	bool lefthanded = false;
	bool z_up = false;

	if ((flags & 0x00000003u) == 0x00000001u) opts->space_conversion = UFBX_SPACE_CONVERSION_TRANSFORM_ROOT;
	if ((flags & 0x00000003u) == 0x00000002u) opts->space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
	if ((flags & 0x00000003u) == 0x00000003u) opts->space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
	if ((flags & 0x0000000cu) == 0x00000004u) opts->geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_HELPER_NODES;
	if ((flags & 0x0000000cu) == 0x00000008u) opts->geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY;
	if ((flags & 0x0000000cu) == 0x0000000cu) opts->geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY_NO_FALLBACK;
	if ((flags & 0x00000030u) == 0x00000010u) opts->inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_HELPER_NODES;
	if ((flags & 0x00000030u) == 0x00000020u) opts->inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_COMPENSATE;
	if ((flags & 0x00000030u) == 0x00000030u) opts->inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_IGNORE;
	if ((flags & 0x000000c0u) == 0x00000040u) opts->handedness_conversion_axis = UFBX_MIRROR_AXIS_X;
	if ((flags & 0x000000c0u) == 0x00000080u) opts->handedness_conversion_axis = UFBX_MIRROR_AXIS_Y;
	if ((flags & 0x000000c0u) == 0x000000c0u) opts->handedness_conversion_axis = UFBX_MIRROR_AXIS_Z;
	if ((flags & 0x00000100u) == 0x00000100u) opts->ignore_geometry = true;
	if ((flags & 0x00000200u) == 0x00000200u) opts->ignore_animation = true;
	if ((flags & 0x00000400u) == 0x00000400u) opts->ignore_embedded = true;
	if ((flags & 0x00000800u) == 0x00000800u) opts->disable_quirks = true;
	if ((flags & 0x00001000u) == 0x00001000u) opts->strict = true;
	if ((flags & 0x00002000u) == 0x00002000u) opts->connect_broken_elements = true;
	if ((flags & 0x00004000u) == 0x00004000u) opts->allow_nodes_out_of_root = true;
	if ((flags & 0x00008000u) == 0x00008000u) opts->allow_missing_vertex_position = true;
	if ((flags & 0x00010000u) == 0x00010000u) opts->allow_empty_faces = true;
	if ((flags & 0x00020000u) == 0x00020000u) opts->generate_missing_normals = true;
	if ((flags & 0x00040000u) == 0x00040000u) opts->reverse_winding = true;
	if ((flags & 0x00080000u) == 0x00080000u) opts->normalize_normals = true;
	if ((flags & 0x00100000u) == 0x00100000u) opts->normalize_tangents = true;
	if ((flags & 0x00200000u) == 0x00200000u) opts->retain_dom = true;
	if ((flags & 0x00c00000u) == 0x00400000u) opts->index_error_handling = UFBX_INDEX_ERROR_HANDLING_NO_INDEX;
	if ((flags & 0x00c00000u) == 0x00800000u) opts->index_error_handling = UFBX_INDEX_ERROR_HANDLING_ABORT_LOADING;
	if ((flags & 0x10000000u) == 0x10000000u) lefthanded = true;
	if ((flags & 0x20000000u) == 0x20000000u) z_up = true;

	opts->target_unit_meters = 1.0f;
	if (lefthanded) {
		opts->target_axes = z_up ? ufbx_axes_left_handed_z_up : ufbx_axes_left_handed_y_up;
	} else {
		opts->target_axes = z_up ? ufbx_axes_right_handed_z_up : ufbx_axes_right_handed_y_up;
	}

}

#endif
