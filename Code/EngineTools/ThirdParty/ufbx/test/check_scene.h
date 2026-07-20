#ifndef UFBXT_CHECK_SCENE_H_INCLUDED
#define UFBXT_CHECK_SCENE_H_INCLUDED

#ifndef ufbxt_assert
#include <assert.h>
#define ufbxt_assert(cond) assert(cond)
#endif

#ifndef ufbxt_arraycount
#define ufbxt_arraycount(arr) (sizeof(arr) / sizeof(*(arr)))
#endif

static uint32_t ufbxt_sink = 0;

#if !defined(UFBX_NO_LIBC)
	#include <math.h>
	#include <string.h>
#endif

#include "../ufbx.h"

static int ufbxt_is_utf8(const char *str, size_t length)
{
	// Table of valid UTF-8 byte sequences: {min,max} (inclusive)
	// Adapted from https://www.w3.org/International/questions/qa-forms-utf-8
	static const unsigned char utf8_forms[8][4][2] = { 
		{ {0xc2,0xdf}, {0x80,0xbf},                           }, // Non-overlong 2-byte
		{ {0xe0,0xe0}, {0xa0,0xbf}, {0x80,0xbf},              }, // Excluding overlongs
		{ {0xe1,0xec}, {0x80,0xbf}, {0x80,0xbf},              }, // Straight 3-byte
		{ {0xee,0xef}, {0x80,0xbf}, {0x80,0xbf},              }, // Straight 3-byte
		{ {0xed,0xed}, {0x80,0x9f}, {0x80,0xbf},              }, // Excluding surrogates
		{ {0xf0,0xf0}, {0x90,0xbf}, {0x80,0xbf}, {0x80,0xbf}, }, // Planes 1-3
		{ {0xf1,0xf3}, {0x80,0xbf}, {0x80,0xbf}, {0x80,0xbf}, }, // Planes 4-15
		{ {0xf4,0xf4}, {0x80,0x8f}, {0x80,0xbf}, {0x80,0xbf}, }, // Plane 16
	};

	size_t pos = 0, form, len;
	while (pos < length) {
		unsigned char c = (unsigned char)str[pos];

		// Fast path for ASCII (U+0000 excluded in ufbx)
		if (c >= 0x01 && c <= 0x7f) {
			pos += 1;
			continue;
		}

		// All forms can be distinguished from the leading byte
		for (form = 0; form < 8; form++) {
			const unsigned char *range = utf8_forms[form][0];
			// Check that the first byte is within range
			if (c >= range[0] && c <= range[1]) break;
		}

		// Invalid starting byte
		if (form == 8) return 0;

		// Validate the following 1-3 bytes
		for (len = 1; len < 4; len++) {
			const unsigned char *range = utf8_forms[form][len];
			// Forms end with max=0 (zero initialized)
			if (range[1] == 0) break;
			// Out of bounds
			if (pos + len >= length) return 0;
			// Check that the next byte is within range
			c = (unsigned char)str[pos + len];
			if (c < utf8_forms[form][len][0] || c > utf8_forms[form][len][1]) return 0;
		}

		// Advance to the next codepoint
		pos += len;
	}

	return 1;
}

static bool ufbxt_float_equal(double a, double b)
{
	if (isnan(a)) {
		return isnan(b);
	} else {
		return a == b;
	}
}

static void ufbxt_check_string(ufbx_string str)
{
	// Data may never be NULL, empty strings should have data = ""
	ufbxt_assert(str.data != NULL);
	ufbxt_assert(strlen(str.data) == str.length);

	// `ufbx_string` is always UTF-8
	ufbxt_assert(ufbxt_is_utf8(str.data, str.length));
}

static void ufbxt_check_memory(const void *data, size_t size)
{
	const uint8_t *d = (const uint8_t*)data;
	for (size_t i = 0; i < size; i++) {
		ufbxt_sink += (uint32_t)d[i];
	}
}

static void ufbxt_check_blob(ufbx_blob blob)
{
	ufbxt_check_memory(blob.data, blob.size);
}

static void ufbxt_check_bool(const bool *p_value)
{
	ufbxt_assert(*(const char*)p_value == 0 || *(const char*)p_value == 1);
}

static void ufbxt_check_bool_list(ufbx_bool_list list)
{
	for (size_t i = 0; i < list.count; i++) {
		ufbxt_check_bool(&list.data[i]);
	}
}

#define ufbxt_check_list(list) ufbxt_check_memory((list).data, (list).count * sizeof(*(list).data))

static void ufbxt_check_element_ptr_any(ufbx_scene *scene, ufbx_element *element)
{
	if (!element) return;
	ufbxt_assert(scene->elements.data[element->element_id] == element);
	ufbxt_assert(scene->elements_by_type[element->type].data[element->typed_id] == element);
}

static void ufbxt_check_element_ptr(ufbx_scene *scene, void *v_element, ufbx_element_type type)
{
	if (!v_element) return;
	ufbx_element *element = (ufbx_element*)v_element;
	ufbxt_assert(element->type == type);
	ufbxt_assert(scene->elements.data[element->element_id] == element);
	ufbxt_assert(scene->elements_by_type[element->type].data[element->typed_id] == element);
}

static void ufbxt_check_vertex_element(ufbx_scene *scene, ufbx_mesh *mesh, void *void_elem, size_t elem_size)
{
	ufbx_vertex_attrib *elem = (ufbx_vertex_attrib*)void_elem;
	if (!elem->exists) {
		ufbxt_assert(elem->values.data == NULL);
		ufbxt_assert(elem->values.count == 0);
		ufbxt_assert(elem->indices.data == NULL);
		ufbxt_assert(elem->indices.count == 0);
		return;
	}

	ufbxt_assert(elem->values.count >= 0);
	ufbxt_assert(elem->values.count <= INT32_MAX);
	ufbxt_assert(elem->indices.count == mesh->num_indices);
	if (mesh->num_indices > 0) {
		ufbxt_assert(elem->indices.data != NULL);
	}

	// Check that the indices are in range
	for (size_t i = 0; i < mesh->num_indices; i++) {
		uint32_t ix = elem->indices.data[i];
		ufbxt_assert(ix < elem->values.count || (ix == UFBX_NO_INDEX && scene->metadata.may_contain_no_index));
	}

	// Check that the data at invalid index is valid and zero
	if (elem->indices.count > 0) {
		char zero[32] = { 0 };
		ufbxt_assert(elem_size <= 32);
		ufbxt_assert(!memcmp((char*)elem->values.data - elem_size, zero, elem_size));
	}

	// Check attrib W if defined
	if (elem->values_w.count > 0) {
		if (elem->indices.count > 0) {
			ufbxt_assert(elem->values_w.data[-1] == 0.0);
		}
		ufbxt_assert(elem->values_w.count == elem->values.count);
	}
}

static void ufbxt_check_mesh_list_count(ufbx_scene *scene, ufbx_mesh *mesh, size_t count, size_t required_count, bool optional)
{
	if (count > 0 || !optional) {
		ufbxt_assert(count == required_count);
	}
}

#define ufbxt_check_mesh_list(scene, mesh, list, required_count, optional) do { \
		ufbxt_check_list(list); \
		ufbxt_check_mesh_list_count((scene), (mesh), (list).count, (required_count), (optional)); \
	} while (0)

static void ufbxt_check_props(ufbx_scene *scene, const ufbx_props *props, bool top)
{
	ufbx_prop *prev = NULL;
	for (size_t i = 0; i < props->props.count; i++) {
		ufbx_prop *prop = &props->props.data[i];

		ufbxt_assert(prop->type < UFBX_PROP_TYPE_COUNT);
		ufbxt_check_string(prop->name);
		ufbxt_check_string(prop->value_str);
		ufbxt_check_blob(prop->value_blob);

		// Properties should be sorted by name
		if (prev) {
			ufbxt_assert(prop->_internal_key >= prev->_internal_key);
			ufbxt_assert(strcmp(prop->name.data, prev->name.data) >= 0);
		}

		ufbx_prop *ref = ufbx_find_prop(props, prop->name.data);
		if (top) {
			ufbxt_assert(ref == prop);
		} else {
			ufbxt_assert(ref != NULL);
		}

		// `REAL/VEC2/VEC3/VEC4` are mutually exclusive
		uint32_t vec_flag = (uint32_t)prop->flags & (UFBX_PROP_FLAG_VALUE_REAL|UFBX_PROP_FLAG_VALUE_VEC2|UFBX_PROP_FLAG_VALUE_VEC3|UFBX_PROP_FLAG_VALUE_VEC4);
		ufbxt_assert((vec_flag & (vec_flag - 1)) == 0);

		prev = prop;
	}

	if (props->defaults) {
		ufbxt_check_props(scene, props->defaults, false);
	}
}

static void ufbxt_check_element_cast(ufbx_element *element)
{
	if (ufbx_as_unknown(element)) ufbxt_assert(element->type == UFBX_ELEMENT_UNKNOWN);
	if (ufbx_as_node(element)) ufbxt_assert(element->type == UFBX_ELEMENT_NODE);
	if (ufbx_as_mesh(element)) ufbxt_assert(element->type == UFBX_ELEMENT_MESH);
	if (ufbx_as_light(element)) ufbxt_assert(element->type == UFBX_ELEMENT_LIGHT);
	if (ufbx_as_camera(element)) ufbxt_assert(element->type == UFBX_ELEMENT_CAMERA);
	if (ufbx_as_bone(element)) ufbxt_assert(element->type == UFBX_ELEMENT_BONE);
	if (ufbx_as_empty(element)) ufbxt_assert(element->type == UFBX_ELEMENT_EMPTY);
	if (ufbx_as_line_curve(element)) ufbxt_assert(element->type == UFBX_ELEMENT_LINE_CURVE);
	if (ufbx_as_nurbs_curve(element)) ufbxt_assert(element->type == UFBX_ELEMENT_NURBS_CURVE);
	if (ufbx_as_nurbs_surface(element)) ufbxt_assert(element->type == UFBX_ELEMENT_NURBS_SURFACE);
	if (ufbx_as_nurbs_trim_surface(element)) ufbxt_assert(element->type == UFBX_ELEMENT_NURBS_TRIM_SURFACE);
	if (ufbx_as_nurbs_trim_boundary(element)) ufbxt_assert(element->type == UFBX_ELEMENT_NURBS_TRIM_BOUNDARY);
	if (ufbx_as_procedural_geometry(element)) ufbxt_assert(element->type == UFBX_ELEMENT_PROCEDURAL_GEOMETRY);
	if (ufbx_as_stereo_camera(element)) ufbxt_assert(element->type == UFBX_ELEMENT_STEREO_CAMERA);
	if (ufbx_as_camera_switcher(element)) ufbxt_assert(element->type == UFBX_ELEMENT_CAMERA_SWITCHER);
	if (ufbx_as_marker(element)) ufbxt_assert(element->type == UFBX_ELEMENT_MARKER);
	if (ufbx_as_lod_group(element)) ufbxt_assert(element->type == UFBX_ELEMENT_LOD_GROUP);
	if (ufbx_as_skin_deformer(element)) ufbxt_assert(element->type == UFBX_ELEMENT_SKIN_DEFORMER);
	if (ufbx_as_skin_cluster(element)) ufbxt_assert(element->type == UFBX_ELEMENT_SKIN_CLUSTER);
	if (ufbx_as_blend_deformer(element)) ufbxt_assert(element->type == UFBX_ELEMENT_BLEND_DEFORMER);
	if (ufbx_as_blend_channel(element)) ufbxt_assert(element->type == UFBX_ELEMENT_BLEND_CHANNEL);
	if (ufbx_as_blend_shape(element)) ufbxt_assert(element->type == UFBX_ELEMENT_BLEND_SHAPE);
	if (ufbx_as_cache_deformer(element)) ufbxt_assert(element->type == UFBX_ELEMENT_CACHE_DEFORMER);
	if (ufbx_as_cache_file(element)) ufbxt_assert(element->type == UFBX_ELEMENT_CACHE_FILE);
	if (ufbx_as_material(element)) ufbxt_assert(element->type == UFBX_ELEMENT_MATERIAL);
	if (ufbx_as_texture(element)) ufbxt_assert(element->type == UFBX_ELEMENT_TEXTURE);
	if (ufbx_as_video(element)) ufbxt_assert(element->type == UFBX_ELEMENT_VIDEO);
	if (ufbx_as_shader(element)) ufbxt_assert(element->type == UFBX_ELEMENT_SHADER);
	if (ufbx_as_shader_binding(element)) ufbxt_assert(element->type == UFBX_ELEMENT_SHADER_BINDING);
	if (ufbx_as_anim_stack(element)) ufbxt_assert(element->type == UFBX_ELEMENT_ANIM_STACK);
	if (ufbx_as_anim_layer(element)) ufbxt_assert(element->type == UFBX_ELEMENT_ANIM_LAYER);
	if (ufbx_as_anim_value(element)) ufbxt_assert(element->type == UFBX_ELEMENT_ANIM_VALUE);
	if (ufbx_as_anim_curve(element)) ufbxt_assert(element->type == UFBX_ELEMENT_ANIM_CURVE);
	if (ufbx_as_display_layer(element)) ufbxt_assert(element->type == UFBX_ELEMENT_DISPLAY_LAYER);
	if (ufbx_as_selection_set(element)) ufbxt_assert(element->type == UFBX_ELEMENT_SELECTION_SET);
	if (ufbx_as_selection_node(element)) ufbxt_assert(element->type == UFBX_ELEMENT_SELECTION_NODE);
	if (ufbx_as_character(element)) ufbxt_assert(element->type == UFBX_ELEMENT_CHARACTER);
	if (ufbx_as_constraint(element)) ufbxt_assert(element->type == UFBX_ELEMENT_CONSTRAINT);
	if (ufbx_as_audio_layer(element)) ufbxt_assert(element->type == UFBX_ELEMENT_AUDIO_LAYER);
	if (ufbx_as_audio_clip(element)) ufbxt_assert(element->type == UFBX_ELEMENT_AUDIO_CLIP);
	if (ufbx_as_pose(element)) ufbxt_assert(element->type == UFBX_ELEMENT_POSE);
	if (ufbx_as_metadata_object(element)) ufbxt_assert(element->type == UFBX_ELEMENT_METADATA_OBJECT);
}

static void ufbxt_check_element(ufbx_scene *scene, ufbx_element *element)
{
	if (scene->dom_root && scene->metadata.file_format == UFBX_FILE_FORMAT_FBX) {
		bool requires_dom = true;
		uint32_t version = scene->metadata.version;

		if (element->type == UFBX_ELEMENT_NODE && ((ufbx_node*)element)->is_root) {
			requires_dom = false;
		} else if (version < 6000 && element->type == UFBX_ELEMENT_ANIM_STACK) {
			requires_dom = false;
		} else if (version < 6000 && element->type == UFBX_ELEMENT_ANIM_LAYER) {
			requires_dom = false;
		} else if (version < 6000 && element->type == UFBX_ELEMENT_SKIN_DEFORMER) {
			requires_dom = false;
		}

		if (requires_dom) {
			ufbxt_assert(element->dom_node);
		}
	}

	ufbxt_check_element_cast(element);
	ufbxt_check_props(scene, &element->props, true);
	ufbxt_check_string(element->name);
	ufbxt_assert(scene->elements.data[element->element_id] == element);

	ufbxt_assert(scene->elements.data[element->element_id] == element);
	ufbxt_assert(scene->elements_by_type[element->type].data[element->typed_id] == element);

	for (size_t i = 0; i < element->connections_src.count; i++) {
		ufbx_connection *c = &element->connections_src.data[i];
		ufbxt_check_string(c->src_prop);
		ufbxt_check_string(c->dst_prop);
		ufbxt_assert(c->src == element);
		ufbxt_check_element_ptr_any(scene, c->dst);
		if (i > 0) {
			int cmp = strcmp(c[-1].src_prop.data, c[0].src_prop.data);
			ufbxt_assert(cmp <= 0);
			if (cmp == 0) {
				ufbxt_assert(strcmp(c[-1].dst_prop.data, c[0].dst_prop.data) <= 0);
			}
		}
	}

	for (size_t i = 0; i < element->connections_dst.count; i++) {
		ufbx_connection *c = &element->connections_dst.data[i];
		ufbxt_check_string(c->src_prop);
		ufbxt_check_string(c->dst_prop);
		ufbxt_assert(c->dst == element);
		ufbxt_check_element_ptr_any(scene, c->src);
		if (i > 0) {
			int cmp = strcmp(c[-1].dst_prop.data, c[0].dst_prop.data);
			ufbxt_assert(cmp <= 0);
			if (cmp == 0) {
				ufbxt_assert(strcmp(c[-1].src_prop.data, c[0].src_prop.data) <= 0);
			}
		}
	}

	ufbxt_assert(element->type >= 0);
	ufbxt_assert(element->type < UFBX_ELEMENT_TYPE_COUNT);
	if (element->type >= UFBX_ELEMENT_TYPE_FIRST_ATTRIB && element->type <= UFBX_ELEMENT_TYPE_LAST_ATTRIB) {
		for (size_t i = 0; i < element->instances.count; i++) {
			ufbx_node *node = element->instances.data[i];
			ufbxt_check_element_ptr(scene, node, UFBX_ELEMENT_NODE);
			bool found = false;
			for (size_t j = 0; j < node->all_attribs.count; j++) {
				if (node->all_attribs.data[j] == element) {
					found = true;
					break;
				}
			}
			ufbxt_assert(found);
		}
	} else {
		ufbxt_assert(element->instances.count == 0);
	}
}

static void ufbxt_check_unknown(ufbx_scene *scene, ufbx_unknown *unknown)
{
	ufbxt_check_string(unknown->type);
	ufbxt_check_string(unknown->sub_type);
	ufbxt_check_string(unknown->super_type);
}

static void ufbxt_check_node(ufbx_scene *scene, ufbx_node *node)
{
	ufbxt_check_element_ptr(scene, node->parent, UFBX_ELEMENT_NODE);
	if (node->parent) {
		bool found = false;
		for (size_t i = 0; i < node->parent->children.count; i++) {
			if (node->parent->children.data[i] == node) {
				found = true;
				break;
			}
		}
		ufbxt_assert(found);
	}

	for (size_t i = 0; i < node->children.count; i++) {
		ufbxt_assert(node->children.data[i]->parent == node);
	}

	for (size_t i = 0; i < node->all_attribs.count; i++) {
		ufbx_element *attrib = node->all_attribs.data[i];
		ufbxt_check_element_ptr_any(scene, attrib);
		bool found = false;
		for (size_t j = 0; j < attrib->instances.count; j++) {
			if (attrib->instances.data[j] == node) {
				found = true;
				break;
			}
		}
		ufbxt_assert(found);
	}

	if (node->all_attribs.count > 0) {
		ufbxt_assert(node->attrib == node->all_attribs.data[0]);
		if (node->all_attribs.count == 1) {
			ufbxt_assert(node->attrib_type == node->attrib->type);
		}
	}

	ufbxt_check_element_ptr(scene, node->mesh, UFBX_ELEMENT_MESH);
	ufbxt_check_element_ptr(scene, node->light, UFBX_ELEMENT_LIGHT);
	ufbxt_check_element_ptr(scene, node->camera, UFBX_ELEMENT_CAMERA);
	ufbxt_check_element_ptr(scene, node->bone, UFBX_ELEMENT_BONE);
	ufbxt_check_element_ptr(scene, node->inherit_scale_node, UFBX_ELEMENT_NODE);
	ufbxt_check_element_ptr(scene, node->scale_helper, UFBX_ELEMENT_NODE);
	ufbxt_check_element_ptr(scene, node->geometry_transform_helper, UFBX_ELEMENT_NODE);

	switch (node->attrib_type) {
	case UFBX_ELEMENT_MESH: ufbxt_assert(node->mesh); break;
	case UFBX_ELEMENT_LIGHT: ufbxt_assert(node->light); break;
	case UFBX_ELEMENT_CAMERA: ufbxt_assert(node->camera); break;
	case UFBX_ELEMENT_BONE: ufbxt_assert(node->bone); break;
	default: /* No shorthand */ break;
	}

	for (size_t i = 0; i < node->materials.count; i++) {
		ufbxt_assert(node->materials.data[i]);
		ufbxt_check_element_ptr(scene, node->materials.data[i], UFBX_ELEMENT_MATERIAL);
	}

	ufbxt_assert((uint32_t)node->attrib_type < (uint32_t)UFBX_ELEMENT_TYPE_COUNT);
	ufbxt_assert((uint32_t)node->inherit_mode < (uint32_t)UFBX_INHERIT_MODE_COUNT);

	if (node->is_root) {
		ufbxt_assert(!node->has_geometry_transform);
	}

	if (node->geometry_transform_helper) {
		ufbxt_assert(node->geometry_transform_helper->is_geometry_transform_helper);

		if (scene->metadata.has_warning[UFBX_WARNING_DUPLICATE_OBJECT_ID]) {
			// In broken cases we may have multiple geometry transform helpers
			ufbxt_assert(node->children.count > 0);
			ufbxt_assert(node->children.data[0]->is_geometry_transform_helper);
		} else {
			// Geometry transform helper must always be the first child if the scene
			ufbxt_assert(node->geometry_transform_helper == node->children.data[0]);
		}
	}

	if (node->is_geometry_transform_helper) {
		if (!scene->metadata.has_warning[UFBX_WARNING_DUPLICATE_OBJECT_ID]) {
			ufbxt_assert(node->parent);
			ufbxt_assert(node->parent->geometry_transform_helper == node);
		}
	}

	ufbxt_check_element_ptr(scene, node->bind_pose, UFBX_ELEMENT_POSE);
	if (node->bind_pose) {
		ufbx_bone_pose *pose = ufbx_get_bone_pose(node->bind_pose, node);
		ufbxt_assert(pose);
		ufbxt_assert(pose->bone_node == node);
	}
}

static void ufbxt_check_mesh_part(ufbx_scene *scene, ufbx_mesh *mesh, ufbx_mesh_part *part, uint32_t index, uint32_t *source_list)
{
	ufbxt_assert(part->index == index);
	ufbxt_assert(part->num_faces <= mesh->num_faces);
	ufbxt_assert(part->face_indices.count == part->num_faces);

	size_t mat_bad_faces[3] = { 0 };
	size_t mat_tris = 0;
	for (size_t j = 0; j < part->num_faces; j++) {
		uint32_t ix = part->face_indices.data[j];
		ufbx_face face = mesh->faces.data[ix];
		if (face.num_indices >= 3) {
			mat_tris += face.num_indices - 2;
		} else {
			mat_bad_faces[face.num_indices]++;
		}
		if (source_list) {
			ufbxt_assert(source_list[ix] == index);
		}
	}

	ufbxt_assert(part->num_triangles == mat_tris);
	ufbxt_assert(part->num_empty_faces == mat_bad_faces[0]);
	ufbxt_assert(part->num_point_faces == mat_bad_faces[1]);
	ufbxt_assert(part->num_line_faces == mat_bad_faces[2]);
}

static void ufbxt_check_mesh(ufbx_scene *scene, ufbx_mesh *mesh)
{
	// ufbx_mesh *found = ufbx_find_mesh(scene, mesh->node.name.data);
	// ufbxt_assert(found && !strcmp(found->node.name.data, mesh->node.name.data));

	ufbxt_assert(mesh->vertices.data == mesh->vertex_position.values.data);
	ufbxt_assert(mesh->vertices.count == mesh->num_vertices);
	ufbxt_assert(mesh->vertex_indices.data == mesh->vertex_position.indices.data);
	ufbxt_assert(mesh->vertex_indices.count == mesh->num_indices);
	ufbxt_assert(mesh->vertex_first_index.count == mesh->num_vertices);

	for (size_t ii = 0; ii < mesh->num_indices; ii++) {
		uint32_t vi = mesh->vertex_indices.data[ii];
		if (vi != UFBX_NO_INDEX) {
			ufbxt_assert(vi < mesh->num_vertices);
			if (!mesh->reversed_winding) {
				ufbxt_assert(mesh->vertex_first_index.data[vi] <= ii);
			}
		}
	}

	for (size_t vi = 0; vi < mesh->num_vertices; vi++) {
		int32_t ii = (int32_t)mesh->vertex_first_index.data[vi];
		if (ii >= 0) {
			ufbxt_assert(mesh->vertex_indices.data[ii] == vi);
		} else {
			ufbxt_assert((uint32_t)ii == UFBX_NO_INDEX);
		}
	}

	ufbxt_check_vertex_element(scene, mesh, &mesh->vertex_position, sizeof(ufbx_vec3));
	ufbxt_check_vertex_element(scene, mesh, &mesh->vertex_normal, sizeof(ufbx_vec3));
	ufbxt_check_vertex_element(scene, mesh, &mesh->vertex_tangent, sizeof(ufbx_vec3));
	ufbxt_check_vertex_element(scene, mesh, &mesh->vertex_bitangent, sizeof(ufbx_vec3));
	ufbxt_check_vertex_element(scene, mesh, &mesh->vertex_uv, sizeof(ufbx_vec2));
	ufbxt_check_vertex_element(scene, mesh, &mesh->vertex_color, sizeof(ufbx_vec4));
	ufbxt_check_vertex_element(scene, mesh, &mesh->skinned_position, sizeof(ufbx_vec3));
	ufbxt_check_vertex_element(scene, mesh, &mesh->skinned_normal, sizeof(ufbx_vec3));

	ufbxt_assert(mesh->num_vertices == mesh->vertex_position.values.count);
	ufbxt_assert(mesh->num_triangles <= mesh->num_indices);

	ufbxt_assert(mesh->vertex_position.value_reals == 3);
	ufbxt_assert(mesh->vertex_normal.value_reals == 3);
	ufbxt_assert(mesh->vertex_tangent.value_reals == 3);
	ufbxt_assert(mesh->vertex_bitangent.value_reals == 3);
	ufbxt_assert(mesh->vertex_uv.value_reals == 2);
	ufbxt_assert(mesh->vertex_color.value_reals == 4);
	ufbxt_assert(mesh->vertex_crease.value_reals == 1);

	if (!scene->metadata.may_contain_missing_vertex_position) {
		ufbxt_assert(mesh->vertex_position.exists);
	}

	if (mesh->vertex_position.exists) {
		ufbxt_assert(mesh->vertex_position.unique_per_vertex);
	}

	ufbxt_check_mesh_list(scene, mesh, mesh->edges, mesh->num_edges, false);
	ufbxt_check_mesh_list(scene, mesh, mesh->edge_crease, mesh->num_edges, true);
	ufbxt_check_mesh_list(scene, mesh, mesh->edge_smoothing, mesh->num_edges, true);
	ufbxt_check_mesh_list(scene, mesh, mesh->edge_visibility, mesh->num_edges, true);
	ufbxt_check_bool_list(mesh->edge_smoothing);
	ufbxt_check_bool_list(mesh->edge_visibility);

	ufbxt_check_mesh_list(scene, mesh, mesh->face_material, mesh->num_faces, true);
	ufbxt_check_mesh_list(scene, mesh, mesh->face_smoothing, mesh->num_faces, true);
	ufbxt_check_mesh_list(scene, mesh, mesh->face_group, mesh->num_faces, true);
	ufbxt_check_mesh_list(scene, mesh, mesh->face_hole, mesh->num_faces, true);
	ufbxt_check_bool_list(mesh->face_smoothing);
	ufbxt_check_bool_list(mesh->face_hole);

	size_t num_triangles = 0;
	size_t max_face_triangles = 0;
	size_t num_bad_faces[3] = { 0 };

	uint32_t prev_end = 0;
	ufbxt_assert(mesh->faces.count == mesh->num_faces);
	for (size_t i = 0; i < mesh->num_faces; i++) {
		ufbx_face face = mesh->faces.data[i];
		ufbxt_assert(face.index_begin == prev_end);
		prev_end = face.index_begin + face.num_indices;
		ufbxt_assert(prev_end <= mesh->num_indices);

		if (face.num_indices >= 3) {
			size_t tris = face.num_indices - 2;
			num_triangles += tris;
			if (tris > max_face_triangles) {
				max_face_triangles = tris;
			}
		} else {
			num_bad_faces[face.num_indices]++;
		}
	}

	ufbxt_assert(mesh->num_triangles == num_triangles);
	ufbxt_assert(mesh->max_face_triangles == max_face_triangles);
	ufbxt_assert(mesh->num_empty_faces == num_bad_faces[0]);
	ufbxt_assert(mesh->num_point_faces == num_bad_faces[1]);
	ufbxt_assert(mesh->num_line_faces == num_bad_faces[2]);

	if (!mesh->from_tessellated_nurbs && !mesh->subdivision_evaluated) {
		ufbxt_assert(scene->metadata.max_face_triangles >= max_face_triangles);
	}

	ufbxt_assert(mesh->edges.count == mesh->num_edges);
	for (size_t i = 0; i < mesh->num_edges; i++) {
		ufbx_edge edge = mesh->edges.data[i];
		ufbxt_assert(edge.a < mesh->num_indices);
		ufbxt_assert(edge.b < mesh->num_indices);
	}

	for (size_t i = 0; i < mesh->uv_sets.count; i++) {
		ufbx_uv_set *set = &mesh->uv_sets.data[i];
		ufbxt_assert(set->vertex_uv.value_reals == 2);
		ufbxt_assert(set->vertex_tangent.value_reals == 3);
		ufbxt_assert(set->vertex_bitangent.value_reals == 3);

		if (i == 0) {
			ufbxt_assert(mesh->vertex_uv.values.data == set->vertex_uv.values.data);
			ufbxt_assert(mesh->vertex_uv.values.count == set->vertex_uv.values.count);
			ufbxt_assert(mesh->vertex_uv.indices.data == set->vertex_uv.indices.data);
			ufbxt_assert(mesh->vertex_uv.indices.count == set->vertex_uv.indices.count);
		}
		ufbxt_check_string(set->name);
		ufbxt_check_vertex_element(scene, mesh, &set->vertex_uv, sizeof(ufbx_vec2));
		ufbxt_check_vertex_element(scene, mesh, &set->vertex_tangent, sizeof(ufbx_vec3));
		ufbxt_check_vertex_element(scene, mesh, &set->vertex_bitangent, sizeof(ufbx_vec3));
	}

	for (size_t i = 0; i < mesh->color_sets.count; i++) {
		ufbx_color_set *set = &mesh->color_sets.data[i];
		ufbxt_assert(set->vertex_color.value_reals == 4);

		if (i == 0) {
			ufbxt_assert(mesh->vertex_color.values.data == set->vertex_color.values.data);
			ufbxt_assert(mesh->vertex_color.values.count == set->vertex_color.values.count);
			ufbxt_assert(mesh->vertex_color.indices.data == set->vertex_color.indices.data);
			ufbxt_assert(mesh->vertex_color.indices.count == set->vertex_color.indices.count);
		}
		ufbxt_check_string(set->name);
		ufbxt_check_vertex_element(scene, mesh, &set->vertex_color, sizeof(ufbx_vec4));
	}

	for (size_t i = 0; i < mesh->num_edges; i++) {
		ufbx_edge edge = mesh->edges.data[i];
		ufbxt_assert(edge.a < mesh->num_indices);
		ufbxt_assert(edge.b < mesh->num_indices);
	}

	if (mesh->face_material.count) {
		ufbxt_assert(mesh->face_material.count == mesh->num_faces);
		for (size_t i = 0; i < mesh->num_faces; i++) {
			int32_t material = mesh->face_material.data[i];
			ufbxt_assert(material >= 0 && (size_t)material < mesh->materials.count);
		}
	} else {
		ufbxt_assert(mesh->face_material.count == 0);
	}

	size_t total_material_faces = 0;
	size_t total_material_tris = 0;
	if (mesh->materials.count > 0) {
		ufbxt_assert(mesh->material_parts.count == 0 || mesh->material_parts.count == mesh->materials.count);

		for (size_t i = 0; i < mesh->materials.count; i++) {
			ufbx_material *mat = mesh->materials.data[i];
			ufbxt_check_element_ptr(scene, mat, UFBX_ELEMENT_MATERIAL);

			if (mesh->material_parts.count > 0) {
				ufbxt_check_mesh_part(scene, mesh, &mesh->material_parts.data[i], (uint32_t)i, mesh->face_material.data);
				total_material_faces += mesh->material_parts.data[i].num_faces;
				total_material_tris += mesh->material_parts.data[i].num_triangles;
			}
		}
	} else {
		if (mesh->material_parts.count > 0) {
			ufbxt_assert(mesh->material_parts.count == 1);
			ufbxt_check_mesh_part(scene, mesh, &mesh->material_parts.data[0], 0, NULL);
			total_material_faces += mesh->material_parts.data[0].num_faces;
			total_material_tris += mesh->material_parts.data[0].num_triangles;
		}
	}
	if (mesh->material_parts.count > 0) {
		ufbxt_assert(total_material_faces == mesh->num_faces);
		ufbxt_assert(total_material_tris == mesh->num_triangles);
	}

	for (size_t i = 0; i < mesh->material_part_usage_order.count; i++) {
		ufbxt_assert(mesh->material_part_usage_order.data[i] < mesh->material_parts.count);
		if (i > 0) {
			ufbx_mesh_part *pa = &mesh->material_parts.data[mesh->material_part_usage_order.data[i - 1]];
			ufbx_mesh_part *pb = &mesh->material_parts.data[mesh->material_part_usage_order.data[i]];
			if (pa->face_indices.count > 0 && pb->face_indices.count > 0) {
				ufbxt_assert(pa->face_indices.data[0] < pb->face_indices.data[0]);
			} else if (pa->face_indices.count == pb->face_indices.count) {
				ufbxt_assert(pa < pb);
			} else {
				ufbxt_assert(pa->face_indices.count > pb->face_indices.count);
			}
		}
	}
	ufbxt_assert(mesh->material_part_usage_order.count == mesh->material_parts.count);

	if (mesh->face_group.count) {
		ufbxt_assert(mesh->face_group.count == mesh->num_faces);
		for (size_t i = 0; i < mesh->num_faces; i++) {
			uint32_t group = mesh->face_group.data[i];
			ufbxt_assert((size_t)group < mesh->face_groups.count);
		}
	} else {
		ufbxt_assert(mesh->face_groups.count == 0);
	}

	ufbxt_assert(mesh->face_group_parts.count == 0 || mesh->face_group_parts.count == mesh->face_groups.count);

	if (mesh->face_groups.count > 0) {
		size_t total_group_faces = 0;
		size_t total_group_tris = 0;
		for (size_t i = 0; i < mesh->face_groups.count; i++) {
			ufbx_face_group *group = &mesh->face_groups.data[i];

			if (mesh->face_group_parts.count > 0) {
				ufbxt_check_mesh_part(scene, mesh, &mesh->face_group_parts.data[i], (uint32_t)i, mesh->face_group.data);
				total_group_faces += mesh->face_group_parts.data[i].num_faces;
				total_group_tris += mesh->face_group_parts.data[i].num_triangles;
			}

			if (i > 0) {
				ufbxt_assert(group->id >= mesh->face_groups.data[i - 1].id);
			}

			ufbxt_check_string(group->name);
		}

		if (mesh->face_group_parts.count > 0) {
			ufbxt_assert(total_group_faces == mesh->num_faces);
			ufbxt_assert(total_group_tris == mesh->num_triangles);
		}
	}

	for (size_t i = 0; i < mesh->skin_deformers.count; i++) {
		ufbxt_assert(mesh->skin_deformers.data[i]->vertices.count >= mesh->num_vertices);
		ufbxt_check_element_ptr(scene, mesh->skin_deformers.data[i], UFBX_ELEMENT_SKIN_DEFORMER);
	}
	for (size_t i = 0; i < mesh->blend_deformers.count; i++) {
		ufbxt_check_element_ptr(scene, mesh->blend_deformers.data[i], UFBX_ELEMENT_BLEND_DEFORMER);
	}
	for (size_t i = 0; i < mesh->cache_deformers.count; i++) {
		ufbxt_check_element_ptr(scene, mesh->cache_deformers.data[i], UFBX_ELEMENT_CACHE_DEFORMER);
	}
	for (size_t i = 0; i < mesh->all_deformers.count; i++) {
		ufbxt_check_element_ptr_any(scene, mesh->all_deformers.data[i]);
	}

	for (size_t i = 0; i < mesh->instances.count; i++) {
		ufbxt_assert(mesh->instances.data[i]->materials.count >= mesh->materials.count);
	}

	// No loose UV or color
	if (mesh->vertex_uv.exists) {
		ufbxt_assert(mesh->uv_sets.count > 0);
	}
	if (mesh->vertex_color.exists) {
		ufbxt_assert(mesh->color_sets.count > 0);
	}
}

static void ufbxt_check_light(ufbx_scene *scene, ufbx_light *light)
{
	ufbxt_assert((uint32_t)light->type < (uint32_t)UFBX_LIGHT_TYPE_COUNT);
	ufbxt_assert((uint32_t)light->decay < (uint32_t)UFBX_LIGHT_DECAY_COUNT);
	ufbxt_assert((uint32_t)light->area_shape < (uint32_t)UFBX_LIGHT_AREA_SHAPE_COUNT);
}

static void ufbxt_check_camera(ufbx_scene *scene, ufbx_camera *camera)
{
	ufbxt_assert((uint32_t)camera->projection_mode < (uint32_t)UFBX_PROJECTION_MODE_COUNT);
	ufbxt_assert((uint32_t)camera->aspect_mode < (uint32_t)UFBX_ASPECT_MODE_COUNT);
	ufbxt_assert((uint32_t)camera->aperture_mode < (uint32_t)UFBX_APERTURE_MODE_COUNT);
	ufbxt_assert((uint32_t)camera->gate_fit < (uint32_t)UFBX_GATE_FIT_COUNT);
	ufbxt_assert((uint32_t)camera->aperture_format < (uint32_t)UFBX_APERTURE_FORMAT_COUNT);

	if (camera->projection_mode == UFBX_PROJECTION_MODE_PERSPECTIVE) {
		ufbxt_assert(ufbxt_float_equal(camera->projection_plane.x, camera->field_of_view_tan.x));
		ufbxt_assert(ufbxt_float_equal(camera->projection_plane.y, camera->field_of_view_tan.y));
	} else if (camera->projection_mode == UFBX_PROJECTION_MODE_ORTHOGRAPHIC) {
		ufbxt_assert(ufbxt_float_equal(camera->projection_plane.x, camera->orthographic_size.x));
		ufbxt_assert(ufbxt_float_equal(camera->projection_plane.y, camera->orthographic_size.y));
	} else {
		ufbxt_assert(false && "Unhandled projection_mode");
	}
}

static void ufbxt_check_bone(ufbx_scene *scene, ufbx_bone *bone)
{
}

static void ufbxt_check_empty(ufbx_scene *scene, ufbx_empty *empty)
{
}

static void ufbxt_check_line_curve(ufbx_scene *scene, ufbx_line_curve *line)
{
	for (size_t i = 0; i < line->point_indices.count; i++) {
		uint32_t ix = line->point_indices.data[i];
		if (scene->metadata.may_contain_no_index && ix == UFBX_NO_INDEX) continue;
		ufbxt_assert(ix < line->control_points.count);
	}

	for (size_t i = 0; i < line->segments.count; i++) {
		ufbx_line_segment seg = line->segments.data[i];
		ufbxt_assert(seg.num_indices <= line->point_indices.count);
		ufbxt_assert(seg.index_begin <= line->point_indices.count - seg.num_indices);
	}

	ufbxt_check_list(line->control_points);
}

static void ufbxt_check_nurbs_basis(ufbx_scene *scene, ufbx_nurbs_basis *basis)
{
	ufbxt_assert((uint32_t)basis->topology < (uint32_t)UFBX_NURBS_TOPOLOGY_COUNT);
	ufbxt_check_list(basis->knot_vector);
	ufbxt_check_list(basis->spans);
	if (basis->valid) {
		ufbxt_assert(basis->order > 1);
		ufbxt_assert(basis->knot_vector.count >= 2*(basis->order-1) + 1);
	}
}

static void ufbxt_check_nurbs_curve(ufbx_scene *scene, ufbx_nurbs_curve *curve)
{
	ufbxt_check_nurbs_basis(scene, &curve->basis);
	ufbxt_check_list(curve->control_points);
}

static void ufbxt_check_nurbs_surface(ufbx_scene *scene, ufbx_nurbs_surface *surface)
{
	ufbxt_check_nurbs_basis(scene, &surface->basis_u);
	ufbxt_check_nurbs_basis(scene, &surface->basis_v);
	ufbxt_check_list(surface->control_points);
}

static void ufbxt_check_nurbs_trim_surface(ufbx_scene *scene, ufbx_nurbs_trim_surface *surface)
{
}

static void ufbxt_check_nurbs_trim_boundary(ufbx_scene *scene, ufbx_nurbs_trim_boundary *boundary)
{
}

static void ufbxt_check_procedural_geometry(ufbx_scene *scene, ufbx_procedural_geometry *geometry)
{
}

static void ufbxt_check_stereo_camera(ufbx_scene *scene, ufbx_stereo_camera *camera)
{
	ufbxt_check_element_ptr(scene, camera->left, UFBX_ELEMENT_CAMERA);
	ufbxt_check_element_ptr(scene, camera->right, UFBX_ELEMENT_CAMERA);
}

static void ufbxt_check_camera_switcher(ufbx_scene *scene, ufbx_camera_switcher *switcher)
{
}

static void ufbxt_check_marker(ufbx_scene *scene, ufbx_marker *marker)
{
	ufbxt_assert((uint32_t)marker->type < (uint32_t)UFBX_MARKER_TYPE_COUNT);
}

static void ufbxt_check_lod_level(ufbx_scene *scene, ufbx_lod_group *lod_group, ufbx_lod_level *level)
{
	ufbxt_assert((uint32_t)level->display < (uint32_t)UFBX_LOD_DISPLAY_COUNT);
}

static void ufbxt_check_lod_group(ufbx_scene *scene, ufbx_lod_group *lod_group)
{
	for (size_t i = 0; i < lod_group->lod_levels.count; i++) {
		ufbxt_check_lod_level(scene, lod_group, &lod_group->lod_levels.data[i]);
	}
}

static void ufbxt_check_skin_deformer(ufbx_scene *scene, ufbx_skin_deformer *deformer)
{
	for (size_t i = 0; i < deformer->clusters.count; i++) {
		ufbxt_check_element_ptr(scene, deformer->clusters.data[i], UFBX_ELEMENT_SKIN_CLUSTER);
		if (!scene->metadata.may_contain_broken_elements) {
			ufbxt_assert(deformer->clusters.data[i]->bone_node);
		}
	}
	for (size_t i = 0; i < deformer->vertices.count; i++) {
		ufbx_skin_vertex vertex = deformer->vertices.data[i];
		ufbxt_assert(vertex.weight_begin <= deformer->weights.count);
		ufbxt_assert(deformer->weights.count - vertex.weight_begin >= vertex.num_weights);

		for (size_t i = 1; i < vertex.num_weights; i++) {
			size_t ix = vertex.weight_begin + i;
			if (!isnan(deformer->weights.data[ix - 1].weight) && !isnan(deformer->weights.data[ix].weight)) {
				ufbxt_assert(deformer->weights.data[ix - 1].weight >= deformer->weights.data[ix].weight);
			}
		}
	}
	for (size_t i = 0; i < deformer->weights.count; i++) {
		ufbxt_assert(deformer->weights.data[i].cluster_index < deformer->clusters.count);
	}
}

static void ufbxt_check_skin_cluster(ufbx_scene *scene, ufbx_skin_cluster *cluster)
{
	ufbxt_check_element_ptr(scene, cluster->bone_node, UFBX_ELEMENT_NODE);
	ufbxt_check_list(cluster->vertices);
	ufbxt_check_list(cluster->weights);
}

static void ufbxt_check_blend_deformer(ufbx_scene *scene, ufbx_blend_deformer *deformer)
{
	for (size_t i = 0; i < deformer->channels.count; i++) {
		ufbxt_check_element_ptr(scene, deformer->channels.data[i], UFBX_ELEMENT_BLEND_CHANNEL);
	}
}

static void ufbxt_check_blend_channel(ufbx_scene *scene, ufbx_blend_channel *channel)
{
	for (size_t i = 0; i < channel->keyframes.count; i++) {
		if (i > 0 && !isnan(channel->keyframes.data[i - 1].target_weight) && !isnan(channel->keyframes.data[i].target_weight)) {
			ufbxt_assert(channel->keyframes.data[i - 1].target_weight <= channel->keyframes.data[i].target_weight);
		}
		ufbxt_assert(channel->keyframes.data[i].shape);
		ufbxt_check_element_ptr(scene, channel->keyframes.data[i].shape, UFBX_ELEMENT_BLEND_SHAPE);
	}
	ufbxt_check_element_ptr(scene, channel->target_shape, UFBX_ELEMENT_BLEND_SHAPE);

	if (channel->keyframes.count > 0) {
		ufbxt_assert(channel->target_shape == channel->keyframes.data[channel->keyframes.count - 1].shape);
	} else {
		ufbxt_assert(channel->target_shape == NULL);
	}
}

static void ufbxt_check_blend_shape(ufbx_scene *scene, ufbx_blend_shape *shape)
{
	ufbxt_check_list(shape->offset_vertices);
	ufbxt_check_list(shape->position_offsets);
	ufbxt_check_list(shape->normal_offsets);
	ufbxt_assert(shape->offset_vertices.count == shape->position_offsets.count);
	if (shape->normal_offsets.count > 0) {
		ufbxt_assert(shape->normal_offsets.count == shape->offset_vertices.count);
	}
}

static void ufbxt_check_cache_deformer(ufbx_scene *scene, ufbx_cache_deformer *deformer)
{
	ufbxt_check_string(deformer->channel);
	ufbxt_check_element_ptr(scene, deformer->file, UFBX_ELEMENT_CACHE_FILE);
}

static void ufbxt_check_cache_file(ufbx_scene *scene, ufbx_cache_file *file)
{
	ufbxt_check_string(file->filename);
	ufbxt_check_string(file->absolute_filename);
	ufbxt_check_string(file->relative_filename);
}

static void ufbxt_check_material_map(ufbx_scene *scene, ufbx_material *material, ufbx_material_map *map)
{
	ufbxt_check_element_ptr(scene, map->texture, UFBX_ELEMENT_TEXTURE);
	ufbxt_assert(map->value_components <= 4);
}

static void ufbxt_check_material(ufbx_scene *scene, ufbx_material *material)
{
	for (size_t i = 0; i < material->textures.count; i++) {
		ufbxt_check_string(material->textures.data[i].material_prop);
		ufbxt_check_string(material->textures.data[i].shader_prop);
		ufbxt_check_element_ptr(scene, material->textures.data[i].texture, UFBX_ELEMENT_TEXTURE);
	}

	for (size_t i = 0; i < UFBX_MATERIAL_FBX_MAP_COUNT; i++) {
		ufbxt_check_material_map(scene, material, &material->fbx.maps[i]);
	}

	for (size_t i = 0; i < UFBX_MATERIAL_PBR_MAP_COUNT; i++) {
		ufbxt_check_material_map(scene, material, &material->pbr.maps[i]);
	}

	ufbxt_check_string(material->shader_prop_prefix);
	ufbxt_check_element_ptr(scene, material->shader, UFBX_ELEMENT_SHADER);
}

static void ufbxt_check_shader_texture_input(ufbx_scene *scene, ufbx_texture *texture, ufbx_shader_texture *shader, ufbx_shader_texture_input *input)
{
	ufbxt_check_string(input->name);
	ufbxt_assert(input->prop);

	ufbxt_check_string(input->value_str);

	if (input->prop) {
		ufbx_prop *prop = input->prop;
		ufbxt_assert(prop == ufbx_find_prop(&texture->props, prop->name.data));
	}

	if (input->texture_prop) {
		ufbx_prop *prop = input->texture_prop;
		ufbxt_assert(prop == ufbx_find_prop(&texture->props, prop->name.data));
	}

	if (input->texture_enabled_prop) {
		ufbx_prop *prop = input->texture_enabled_prop;
		ufbxt_assert(prop == ufbx_find_prop(&texture->props, prop->name.data));
	}

	ufbxt_check_element_ptr(scene, input->texture, UFBX_ELEMENT_TEXTURE);
}

static void ufbxt_check_shader_texture(ufbx_scene *scene, ufbx_texture *texture, ufbx_shader_texture *shader)
{
	ufbxt_check_string(shader->shader_name);
	ufbxt_check_string(shader->shader_source);

	ufbxt_check_element_ptr(scene, shader->main_texture, UFBX_ELEMENT_TEXTURE);

	for (size_t i = 0; i < shader->inputs.count; i++) {
		ufbxt_check_shader_texture_input(scene, texture, shader, &shader->inputs.data[i]);
	}
}

static void ufbxt_check_texture(ufbx_scene *scene, ufbx_texture *texture)
{
	ufbxt_check_string(texture->filename);
	ufbxt_check_string(texture->relative_filename);
	ufbxt_check_string(texture->absolute_filename);

	ufbxt_check_blob(texture->raw_filename);
	ufbxt_check_blob(texture->raw_relative_filename);
	ufbxt_check_blob(texture->raw_absolute_filename);

	ufbxt_check_element_ptr(scene, texture->video, UFBX_ELEMENT_VIDEO);
	for (size_t i = 0; i < texture->layers.count; i++) {
		ufbxt_assert(texture->layers.data[i].texture);
		ufbxt_check_element_ptr(scene, texture->layers.data[i].texture, UFBX_ELEMENT_TEXTURE);
	}

	if (texture->shader) {
		ufbxt_check_shader_texture(scene, texture, texture->shader);
	}

	for (size_t i = 0; i < texture->file_textures.count; i++) {
		ufbx_texture *file = texture->file_textures.data[i];
		ufbxt_check_element_ptr(scene, file, UFBX_ELEMENT_TEXTURE);
		ufbxt_assert(file->type == UFBX_TEXTURE_FILE);
	}

	if (texture->type == UFBX_TEXTURE_FILE) {
		ufbxt_assert(texture->file_textures.count >= 1);
		ufbxt_assert(texture->file_textures.data[0] == texture);
	} else if (texture->type == UFBX_TEXTURE_SHADER) {
		ufbxt_assert(texture->shader);
	}

	if (texture->raw_absolute_filename.size > 0 || texture->raw_relative_filename.size > 0) {
		ufbxt_assert(texture->has_file);
	}

	ufbxt_assert(texture->file_index < scene->texture_files.count || texture->file_index == UFBX_NO_INDEX);
	if (texture->has_file) {
		ufbxt_assert(texture->file_index < scene->texture_files.count);
		ufbx_texture_file *file = &scene->texture_files.data[texture->file_index];
		ufbxt_assert(!strcmp(file->absolute_filename.data, texture->absolute_filename.data)
			|| !strcmp(file->relative_filename.data, texture->relative_filename.data));
	} else {
		ufbxt_assert(texture->file_index == UFBX_NO_INDEX);
	}
}

static void ufbxt_check_video(ufbx_scene *scene, ufbx_video *video)
{
	ufbxt_check_string(video->filename);
	ufbxt_check_string(video->relative_filename);
	ufbxt_check_string(video->absolute_filename);

	ufbxt_check_blob(video->raw_filename);
	ufbxt_check_blob(video->raw_relative_filename);
	ufbxt_check_blob(video->raw_absolute_filename);
}

static void ufbxt_check_shader(ufbx_scene *scene, ufbx_shader *shader)
{
	ufbxt_assert((uint32_t)shader->type < (uint32_t)UFBX_SHADER_TYPE_COUNT);
	for (size_t i = 0; i < shader->bindings.count; i++) {
		ufbxt_check_element_ptr(scene, shader->bindings.data[i], UFBX_ELEMENT_SHADER_BINDING);
	}
}

static void ufbxt_check_shader_binding(ufbx_scene *scene, ufbx_shader_binding *binding)
{
	for (size_t i = 0; i < binding->prop_bindings.count; i++) {
		ufbx_shader_prop_binding prop = binding->prop_bindings.data[i];
		ufbxt_check_string(prop.material_prop);
		ufbxt_check_string(prop.shader_prop);
	}
}

static void ufbxt_check_anim(ufbx_scene *scene, ufbx_anim *anim)
{
	ufbxt_assert(anim);
	for (size_t i = 0; i < anim->layers.count; i++) {
		ufbxt_check_element_ptr(scene, anim->layers.data[i], UFBX_ELEMENT_ANIM_LAYER);
	}
	for (size_t i = 0; i < anim->prop_overrides.count; i++) {
		ufbx_prop_override *over = &anim->prop_overrides.data[i];
		ufbxt_check_string(over->prop_name);
		ufbxt_check_string(over->value_str);
	}
	if (!anim->custom) {
		ufbxt_assert(anim->prop_overrides.count == 0);
		ufbxt_assert(anim->transform_overrides.count == 0);
		ufbxt_assert(anim->override_layer_weights.count == 0);
	}
}

static void ufbxt_check_anim_stack(ufbx_scene *scene, ufbx_anim_stack *anim_stack)
{
	for (size_t i = 0; i < anim_stack->layers.count; i++) {
		ufbxt_check_element_ptr(scene, anim_stack->layers.data[i], UFBX_ELEMENT_ANIM_LAYER);
	}
}

static void ufbxt_check_anim_layer(ufbx_scene *scene, ufbx_anim_layer *anim_layer)
{
	ufbxt_check_string(anim_layer->name);

	for (size_t i = 0; i < anim_layer->anim_values.count; i++) {
		ufbxt_check_element_ptr(scene, anim_layer->anim_values.data[i], UFBX_ELEMENT_ANIM_VALUE);
	}

	size_t props_left = 0;
	ufbx_element *prev_element = NULL;

	for (size_t i = 0; i < anim_layer->anim_props.count; i++) {
		ufbx_anim_prop *anim_prop = &anim_layer->anim_props.data[i];

		ufbxt_check_string(anim_prop->prop_name);
		ufbxt_check_element_ptr_any(scene, anim_prop->element);
		ufbxt_check_element_ptr(scene, anim_prop->anim_value, UFBX_ELEMENT_ANIM_VALUE);
		ufbxt_assert(anim_prop->anim_value);

		ufbx_anim_prop *ref = ufbx_find_anim_prop(anim_layer, anim_prop->element, anim_prop->prop_name.data);
		ufbxt_assert(ref);
		ufbxt_assert(!strcmp(ref->prop_name.data, anim_prop->prop_name.data));

		if (props_left > 0) {
			ufbxt_assert(anim_prop->element == prev_element);
			props_left--;
		} else {
			ufbx_anim_prop_list list = ufbx_find_anim_props(anim_layer, anim_prop->element);
			ufbxt_assert(list.count > 0);
			prev_element = anim_prop->element;
			props_left = list.count - 1;
		}
	}

	ufbxt_assert(props_left == 0);
}

static void ufbxt_check_anim_value(ufbx_scene *scene, ufbx_anim_value *anim_value)
{
	ufbxt_check_element_ptr(scene, anim_value->curves[0], UFBX_ELEMENT_ANIM_CURVE);
	ufbxt_check_element_ptr(scene, anim_value->curves[1], UFBX_ELEMENT_ANIM_CURVE);
	ufbxt_check_element_ptr(scene, anim_value->curves[2], UFBX_ELEMENT_ANIM_CURVE);
}

static void ufbxt_check_anim_curve(ufbx_scene *scene, ufbx_anim_curve *anim_curve)
{
	for (size_t i = 0; i < anim_curve->keyframes.count; i++) {
		ufbx_keyframe key = anim_curve->keyframes.data[i];
		ufbxt_assert((uint32_t)key.interpolation < (uint32_t)UFBX_INTERPOLATION_COUNT);
	}
}

static void ufbxt_check_display_layer(ufbx_scene *scene, ufbx_display_layer *layer)
{
	for (size_t i = 0; i < layer->nodes.count; i++) {
		ufbxt_assert(layer->nodes.data[i]);
		ufbxt_check_element_ptr(scene, layer->nodes.data[i], UFBX_ELEMENT_NODE);
	}
}

static void ufbxt_check_selection_set(ufbx_scene *scene, ufbx_selection_set *set)
{
	for (size_t i = 0; i < set->nodes.count; i++) {
		ufbxt_assert(set->nodes.data[i]);
		ufbxt_check_element_ptr(scene, set->nodes.data[i], UFBX_ELEMENT_SELECTION_NODE);
	}
}

static void ufbxt_check_selection_node(ufbx_scene *scene, ufbx_selection_node *node)
{
	ufbxt_check_element_ptr(scene, node->target_node, UFBX_ELEMENT_NODE);
	ufbxt_check_element_ptr(scene, node->target_mesh, UFBX_ELEMENT_MESH);
}

static void ufbxt_check_character(ufbx_scene *scene, ufbx_character *character)
{
}

static void ufbxt_check_constraint(ufbx_scene *scene, ufbx_constraint *constraint)
{
	ufbxt_check_element_ptr(scene, constraint->node, UFBX_ELEMENT_NODE);
	ufbxt_check_element_ptr(scene, constraint->aim_up_node, UFBX_ELEMENT_NODE);
	ufbxt_check_element_ptr(scene, constraint->ik_effector, UFBX_ELEMENT_NODE);
	ufbxt_check_element_ptr(scene, constraint->ik_end_node, UFBX_ELEMENT_NODE);
	for (size_t i = 0; i < constraint->targets.count; i++) {
		ufbxt_assert(constraint->targets.data[i].node);
		ufbxt_check_element_ptr(scene, constraint->targets.data[i].node, UFBX_ELEMENT_NODE);
	}
}

static void ufbxt_check_audio_layer(ufbx_scene *scene, ufbx_audio_layer *layer)
{
	for (size_t i = 0; i < layer->clips.count; i++) {
		ufbxt_check_element_ptr(scene, layer->clips.data[i], UFBX_ELEMENT_AUDIO_CLIP);
	}
}

static void ufbxt_check_audio_clip(ufbx_scene *scene, ufbx_audio_clip *clip)
{
	ufbxt_check_string(clip->filename);
	ufbxt_check_string(clip->absolute_filename);
	ufbxt_check_string(clip->relative_filename);
	ufbxt_check_blob(clip->raw_filename);
	ufbxt_check_blob(clip->raw_absolute_filename);
	ufbxt_check_blob(clip->raw_relative_filename);
	ufbxt_check_blob(clip->content);
}

static void ufbxt_check_pose(ufbx_scene *scene, ufbx_pose *pose)
{
	for (size_t i = 0; i < pose->bone_poses.count; i++) {
		ufbx_bone_pose bone_pose = pose->bone_poses.data[i];
		ufbxt_assert(bone_pose.bone_node);
		ufbxt_check_element_ptr(scene, bone_pose.bone_node, UFBX_ELEMENT_NODE);

		if (i > 0) {
			ufbxt_assert(pose->bone_poses.data[i - 1].bone_node->typed_id <= bone_pose.bone_node->typed_id);
		}

		ufbx_bone_pose *pose_ref = ufbx_get_bone_pose(pose, bone_pose.bone_node);
		ufbxt_assert(pose_ref);
		ufbxt_assert(pose_ref->bone_node == bone_pose.bone_node);
	}
}

static void ufbxt_check_metadata_object(ufbx_scene *scene, ufbx_metadata_object *metadata_object)
{
}

static void ufbxt_check_texture_file(ufbx_scene *scene, ufbx_texture_file *texture_file)
{
	ufbxt_check_string(texture_file->filename);
	ufbxt_check_string(texture_file->absolute_filename);
	ufbxt_check_string(texture_file->relative_filename);
	ufbxt_check_blob(texture_file->raw_filename);
	ufbxt_check_blob(texture_file->raw_absolute_filename);
	ufbxt_check_blob(texture_file->raw_relative_filename);
	ufbxt_check_blob(texture_file->content);
}

static void ufbxt_check_application(ufbx_scene *scene, ufbx_application *application)
{
	ufbxt_check_string(application->name);
	ufbxt_check_string(application->vendor);
	ufbxt_check_string(application->version);
}

static void ufbxt_check_thumbnail(ufbx_scene *scene, ufbx_thumbnail *thumbnail)
{
	ufbxt_assert((uint32_t)thumbnail->format < (uint32_t)UFBX_THUMBNAIL_FORMAT_COUNT);
	ufbxt_check_blob(thumbnail->data);
}

static void ufbxt_check_metadata(ufbx_scene *scene, ufbx_metadata *metadata)
{
	ufbxt_check_string(metadata->creator);
	ufbxt_check_string(metadata->filename);
	ufbxt_check_string(metadata->relative_root);
	ufbxt_check_blob(metadata->raw_filename);
	ufbxt_check_blob(metadata->raw_relative_root);
	ufbxt_check_application(scene, &metadata->latest_application);
	ufbxt_check_application(scene, &metadata->original_application);

	if (metadata->file_format == UFBX_FILE_FORMAT_FBX) {
	} else if (metadata->file_format == UFBX_FILE_FORMAT_OBJ) {
		ufbxt_assert(metadata->ascii);
	} else if (metadata->file_format == UFBX_FILE_FORMAT_MTL) {
		ufbxt_assert(metadata->ascii);
	} else {
		ufbxt_assert(0 && "Invalid file format");
	}

	for (size_t i = 0; i < metadata->warnings.count; i++) {
		ufbx_warning *warning = &metadata->warnings.data[i];
		ufbxt_check_string(warning->description);
		ufbxt_assert(metadata->has_warning[warning->type]);
		if (warning->element_id != UFBX_NO_INDEX) {
			ufbxt_assert(warning->element_id < scene->elements.count);
		}
	}

	ufbxt_check_thumbnail(scene, &metadata->thumbnail);
}

static void ufbxt_check_dom_value(ufbx_scene *scene, ufbx_dom_value *value)
{
	ufbxt_check_string(value->value_str);
	ufbxt_check_blob(value->value_blob);
	size_t array_stride = 0;
	switch (value->type) {
	case UFBX_DOM_VALUE_NUMBER:
		break;
	case UFBX_DOM_VALUE_STRING:
		break;
	case UFBX_DOM_VALUE_BLOB:
		array_stride = 1;
		break;
	case UFBX_DOM_VALUE_ARRAY_I32:
		array_stride = 4;
		break;
	case UFBX_DOM_VALUE_ARRAY_I64:
		array_stride = 8;
		break;
	case UFBX_DOM_VALUE_ARRAY_F32:
		array_stride = 4;
		break;
	case UFBX_DOM_VALUE_ARRAY_F64:
		array_stride = 8;
		break;
	case UFBX_DOM_VALUE_ARRAY_BLOB:
		array_stride = sizeof(ufbx_blob);
		break;
	case UFBX_DOM_VALUE_ARRAY_IGNORED:
		array_stride = 0;
		break;
	default:
		ufbxt_assert(0 && "Unhandled type");
		break;
	}

	if (array_stride) {
		ufbxt_assert(value->value_blob.size == (size_t)value->value_int * array_stride);
	}

	if (value->type == UFBX_DOM_VALUE_ARRAY_BLOB) {
		const ufbx_blob *blobs = (const ufbx_blob*)value->value_blob.data;
		for (size_t i = 0; i < (size_t)value->value_int; i++) {
			ufbxt_check_blob(blobs[i]);
		}
	}
}

static void ufbxt_check_dom_node(ufbx_scene *scene, ufbx_dom_node *node)
{
	ufbxt_assert(node);
	ufbxt_check_string(node->name);
	for (size_t i = 0; i < node->children.count; i++) {
		ufbxt_check_dom_node(scene, node->children.data[i]);
	}
	for (size_t i = 0; i < node->values.count; i++) {
		ufbxt_check_dom_value(scene, &node->values.data[i]);
	}
}

static void ufbxt_check_scene(ufbx_scene *scene)
{
	// TODO: Partial safety validation?
	if (scene->metadata.is_unsafe) return;

	ufbxt_check_element_cast(NULL);

	ufbxt_check_metadata(scene, &scene->metadata);

	for (size_t i = 0; i < scene->elements.count; i++) {
		ufbxt_assert(scene->elements.data[i]->element_id == (uint32_t)i);
		ufbxt_check_element(scene, scene->elements.data[i]);
	}

	for (size_t i = 0; i < scene->unknowns.count; i++) {
		ufbxt_check_unknown(scene, scene->unknowns.data[i]);
	}

	for (size_t i = 0; i < scene->nodes.count; i++) {
		ufbxt_check_node(scene, scene->nodes.data[i]);
	}

	for (size_t i = 0; i < scene->meshes.count; i++) {
		ufbxt_check_mesh(scene, scene->meshes.data[i]);
	}

	for (size_t i = 0; i < scene->lights.count; i++) {
		ufbxt_check_light(scene, scene->lights.data[i]);
	}

	for (size_t i = 0; i < scene->cameras.count; i++) {
		ufbxt_check_camera(scene, scene->cameras.data[i]);
	}

	for (size_t i = 0; i < scene->bones.count; i++) {
		ufbxt_check_bone(scene, scene->bones.data[i]);
	}

	for (size_t i = 0; i < scene->empties.count; i++) {
		ufbxt_check_empty(scene, scene->empties.data[i]);
	}

	for (size_t i = 0; i < scene->line_curves.count; i++) {
		ufbxt_check_line_curve(scene, scene->line_curves.data[i]);
	}

	for (size_t i = 0; i < scene->nurbs_curves.count; i++) {
		ufbxt_check_nurbs_curve(scene, scene->nurbs_curves.data[i]);
	}

	for (size_t i = 0; i < scene->nurbs_surfaces.count; i++) {
		ufbxt_check_nurbs_surface(scene, scene->nurbs_surfaces.data[i]);
	}

	for (size_t i = 0; i < scene->nurbs_trim_surfaces.count; i++) {
		ufbxt_check_nurbs_trim_surface(scene, scene->nurbs_trim_surfaces.data[i]);
	}

	for (size_t i = 0; i < scene->nurbs_trim_boundaries.count; i++) {
		ufbxt_check_nurbs_trim_boundary(scene, scene->nurbs_trim_boundaries.data[i]);
	}

	for (size_t i = 0; i < scene->procedural_geometries.count; i++) {
		ufbxt_check_procedural_geometry(scene, scene->procedural_geometries.data[i]);
	}

	for (size_t i = 0; i < scene->stereo_cameras.count; i++) {
		ufbxt_check_stereo_camera(scene, scene->stereo_cameras.data[i]);
	}

	for (size_t i = 0; i < scene->camera_switchers.count; i++) {
		ufbxt_check_camera_switcher(scene, scene->camera_switchers.data[i]);
	}

	for (size_t i = 0; i < scene->markers.count; i++) {
		ufbxt_check_marker(scene, scene->markers.data[i]);
	}

	for (size_t i = 0; i < scene->lod_groups.count; i++) {
		ufbxt_check_lod_group(scene, scene->lod_groups.data[i]);
	}

	for (size_t i = 0; i < scene->skin_deformers.count; i++) {
		ufbxt_check_skin_deformer(scene, scene->skin_deformers.data[i]);
	}

	for (size_t i = 0; i < scene->skin_clusters.count; i++) {
		ufbxt_check_skin_cluster(scene, scene->skin_clusters.data[i]);
	}

	for (size_t i = 0; i < scene->blend_deformers.count; i++) {
		ufbxt_check_blend_deformer(scene, scene->blend_deformers.data[i]);
	}

	for (size_t i = 0; i < scene->blend_channels.count; i++) {
		ufbxt_check_blend_channel(scene, scene->blend_channels.data[i]);
	}

	for (size_t i = 0; i < scene->blend_shapes.count; i++) {
		ufbxt_check_blend_shape(scene, scene->blend_shapes.data[i]);
	}

	for (size_t i = 0; i < scene->cache_deformers.count; i++) {
		ufbxt_check_cache_deformer(scene, scene->cache_deformers.data[i]);
	}

	for (size_t i = 0; i < scene->cache_files.count; i++) {
		ufbxt_check_cache_file(scene, scene->cache_files.data[i]);
	}

	for (size_t i = 0; i < scene->materials.count; i++) {
		ufbxt_check_material(scene, scene->materials.data[i]);
	}

	for (size_t i = 0; i < scene->textures.count; i++) {
		ufbxt_check_texture(scene, scene->textures.data[i]);
	}

	for (size_t i = 0; i < scene->videos.count; i++) {
		ufbxt_check_video(scene, scene->videos.data[i]);
	}

	for (size_t i = 0; i < scene->shaders.count; i++) {
		ufbxt_check_shader(scene, scene->shaders.data[i]);
	}

	for (size_t i = 0; i < scene->shader_bindings.count; i++) {
		ufbxt_check_shader_binding(scene, scene->shader_bindings.data[i]);
	}

	for (size_t i = 0; i < scene->anim_stacks.count; i++) {
		ufbxt_check_anim_stack(scene, scene->anim_stacks.data[i]);
	}

	for (size_t i = 0; i < scene->anim_layers.count; i++) {
		ufbxt_check_anim_layer(scene, scene->anim_layers.data[i]);
	}

	for (size_t i = 0; i < scene->anim_values.count; i++) {
		ufbxt_check_anim_value(scene, scene->anim_values.data[i]);
	}

	for (size_t i = 0; i < scene->anim_curves.count; i++) {
		ufbxt_check_anim_curve(scene, scene->anim_curves.data[i]);
	}

	for (size_t i = 0; i < scene->display_layers.count; i++) {
		ufbxt_check_display_layer(scene, scene->display_layers.data[i]);
	}

	for (size_t i = 0; i < scene->selection_sets.count; i++) {
		ufbxt_check_selection_set(scene, scene->selection_sets.data[i]);
	}

	for (size_t i = 0; i < scene->selection_nodes.count; i++) {
		ufbxt_check_selection_node(scene, scene->selection_nodes.data[i]);
	}

	for (size_t i = 0; i < scene->characters.count; i++) {
		ufbxt_check_character(scene, scene->characters.data[i]);
	}

	for (size_t i = 0; i < scene->constraints.count; i++) {
		ufbxt_check_constraint(scene, scene->constraints.data[i]);
	}

	for (size_t i = 0; i < scene->audio_layers.count; i++) {
		ufbxt_check_audio_layer(scene, scene->audio_layers.data[i]);
	}

	for (size_t i = 0; i < scene->audio_clips.count; i++) {
		ufbxt_check_audio_clip(scene, scene->audio_clips.data[i]);
	}

	for (size_t i = 0; i < scene->poses.count; i++) {
		ufbxt_check_pose(scene, scene->poses.data[i]);
	}

	for (size_t i = 0; i < scene->metadata_objects.count; i++) {
		ufbxt_check_metadata_object(scene, scene->metadata_objects.data[i]);
	}

	for (size_t i = 0; i < scene->texture_files.count; i++) {
		ufbxt_assert(scene->texture_files.data[i].index == i);
		ufbxt_check_texture_file(scene, &scene->texture_files.data[i]);
	}

	if (scene->dom_root) {
		ufbxt_check_dom_node(scene, scene->dom_root);
	}

	for (size_t type_ix = 0; type_ix < UFBX_ELEMENT_TYPE_COUNT; type_ix++) {
		size_t num_elements = scene->elements_by_type[type_ix].count;
		for (size_t i = 0; i < num_elements; i++) {
			ufbx_element *element = scene->elements_by_type[type_ix].data[i];
			ufbxt_assert(element->type == (ufbx_element_type)type_ix);
			ufbxt_assert(element->typed_id == (uint32_t)i);
		}
	}


	ufbxt_assert(scene->root_node);
	ufbxt_check_element_ptr(scene, scene->root_node, UFBX_ELEMENT_NODE);
	ufbxt_assert(scene->root_node->is_root);
}

#endif
