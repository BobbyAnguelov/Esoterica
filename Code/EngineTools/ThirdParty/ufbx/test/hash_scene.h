#ifndef UFBXT_HASH_SCENE_H_INCLUDED
#define UFBXT_HASH_SCENE_H_INCLUDED

#include "testing_basics.h"

#if !defined(UFBX_NO_LIBC)
	#include <stdio.h>
	#include <string.h>
	#include <assert.h>
	#define ufbxh_assert(cond) assert(cond)
	#define ufbxh_FILE FILE
	#define ufbxh_fprintf fprintf
	#define ufbxh_snprintf snprintf
	#define ufbxh_fflush fflush
#endif

typedef struct ufbxt_hash {
	uint64_t state;
	bool dumping;
	ufbxh_FILE *dump_file;
	size_t indent;
	bool at_tag;
	bool at_value;
	bool big_endian;
} ufbxt_hash;

ufbxt_noinline static void ufbxt_hash_init(ufbxt_hash *h, ufbxh_FILE *dump_file)
{
	memset(h, 0, sizeof(ufbxt_hash));
	h->state = UINT64_C(0xcbf29ce484222325);
	{
		uint8_t buf[2];
		uint16_t val = 0xbbaa;
		memcpy(buf, &val, 2);
		h->big_endian = buf[0] == 0xbb;
	}

	if (dump_file) {
		h->dumping = true;
		h->dump_file = dump_file;
	}
}

ufbxt_noinline static uint64_t ufbxt_hash_finish(ufbxt_hash *h)
{
	if (h->dump_file) {
		ufbxh_fflush(h->dump_file);
	}
	return h->state;
}

ufbxt_noinline static void ufbxt_push_tag(ufbxt_hash *h, const char *tag)
{
	if (!h->dumping) return;
	if (tag[0] == '&') {
		tag++;
	}
	if (!strncmp(tag, "v->", 3)) {
		tag += 3;
	} else if (!strncmp(tag, "v.", 2)) {
		tag += 2;
	}

	if (h->at_tag) {
		ufbxh_fprintf(h->dump_file, "{\n");
		h->indent += 1;
	}
	for (size_t i = 0; i < h->indent; i++) {
		ufbxh_fprintf(h->dump_file, "  ");
	}
	ufbxh_fprintf(h->dump_file, "%s: ", tag);
	h->at_tag = true;
}

ufbxt_noinline static void ufbxt_push_tag_index(ufbxt_hash *h, size_t index)
{
	if (!h->dumping) return;
	char buf[128];
	ufbxh_snprintf(buf, sizeof(buf), "[%zu]", index);
	ufbxt_push_tag(h, buf);
}

ufbxt_noinline static void ufbxt_pop_tag(ufbxt_hash *h)
{
	if (!h->dumping) return;
	if (h->at_value || h->at_tag) {
		ufbxh_fprintf(h->dump_file, "\n");
		h->at_value = false;
		h->at_tag = false;
	} else {
		h->indent -= 1;
		for (size_t i = 0; i < h->indent; i++) {
			ufbxh_fprintf(h->dump_file, "  ");
		}
		ufbxh_fprintf(h->dump_file, "}\n");
	}
}

ufbxt_noinline static void ufbxt_dump_data(ufbxt_hash *h, const void *data, size_t size, bool reverse)
{
	ufbxh_assert(h->at_tag && !h->at_value);
	h->at_tag = false;
	h->at_value = true;

	const char *d = (const char*)data;
	if (reverse) {
		for (size_t i = size; i > 0; i--) {
			uint32_t b = (uint32_t)(uint8_t)d[i - 1];
			ufbxh_fprintf(h->dump_file, "%02x", b);
		}
	} else {
		for (size_t i = 0; i < size; i++) {
			uint32_t b = (uint32_t)(uint8_t)d[i];
			ufbxh_fprintf(h->dump_file, "%02x", b);
		}
	}
}

ufbxt_noinline static void ufbxt_hash_data(ufbxt_hash *h, const void *data, size_t size)
{
	const char *d = (const char*)data;
	uint64_t state = h->state;
	for (size_t i = 0; i < size; i++) {
		state = (state ^ (uint8_t)d[i]) * UINT64_C(0x00000100000001B3);
	}
	h->state = state;

	if (h->dumping) {
		ufbxt_dump_data(h, data, size, false);
	}
}

ufbxt_noinline static void ufbxt_hash_endian_data(ufbxt_hash *h, const void *data, size_t size)
{
	bool reverse = !h->big_endian;
	ufbxh_assert(size == 1 || size == 2 || size == 4 || size == 8);

	const char *d = (const char*)data;
	uint64_t state = h->state;
	if (reverse) {
		for (size_t i = size; i > 0; i--) {
			state = (state ^ (uint8_t)d[i - 1]) * UINT64_C(0x00000100000001B3);
		}
	} else {
		for (size_t i = 0; i < size; i++) {
			state = (state ^ (uint8_t)d[i]) * UINT64_C(0x00000100000001B3);
		}
	}
	h->state = state;

	if (h->dumping) {
		ufbxt_dump_data(h, data, size, true);
	}
}

#define ufbxt_hash_pod_imp(h, v) ufbxt_hash_endian_data((h), &(v), sizeof((v)))
#define ufbxt_hash_pod(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_pod_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_float_imp(ufbxt_hash *h, float v)
{
	if (v == v) {
		ufbxt_hash_pod_imp(h, v);
	} else {
		uint32_t u = UINT32_MAX;
		ufbxt_hash_pod_imp(h, u);
	}
}

ufbxt_noinline static void ufbxt_hash_double_imp(ufbxt_hash *h, double v)
{
	if (v == v) {
		ufbxt_hash_pod_imp(h, v);
	} else {
		uint64_t u = UINT64_MAX;
		ufbxt_hash_pod_imp(h, u);
	}
}

ufbxt_noinline static void ufbxt_hash_real_imp(ufbxt_hash *h, ufbx_real v)
{
	if (v == v) {
		ufbxt_hash_pod_imp(h, v);
	} else {
		uint64_t u = UINT64_MAX;
		ufbxt_hash_pod_imp(h, u);
	}
}

ufbxt_noinline static void ufbxt_hash_size_t_imp(ufbxt_hash *h, size_t v)
{
	uint64_t u = v;
	ufbxt_hash_pod_imp(h, u);
}

#define ufbxt_hash_float(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_float_imp(h, v), ufbxt_pop_tag(h))
#define ufbxt_hash_double(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_double_imp(h, v), ufbxt_pop_tag(h))
#define ufbxt_hash_real(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_real_imp(h, v), ufbxt_pop_tag(h))
#define ufbxt_hash_size_t(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_size_t_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_vec2_imp(ufbxt_hash *h, ufbx_vec2 v)
{
	ufbxt_hash_real(h, v.x);
	ufbxt_hash_real(h, v.y);
}

ufbxt_noinline static void ufbxt_hash_vec3_imp(ufbxt_hash *h, ufbx_vec3 v)
{
	ufbxt_hash_real(h, v.x);
	ufbxt_hash_real(h, v.y);
	ufbxt_hash_real(h, v.z);
}

ufbxt_noinline static void ufbxt_hash_vec4_imp(ufbxt_hash *h, ufbx_vec4 v)
{
	ufbxt_hash_real(h, v.x);
	ufbxt_hash_real(h, v.y);
	ufbxt_hash_real(h, v.z);
	ufbxt_hash_real(h, v.w);
}

ufbxt_noinline static void ufbxt_hash_quat_imp(ufbxt_hash *h, ufbx_quat v)
{
	ufbxt_hash_real(h, v.x);
	ufbxt_hash_real(h, v.y);
	ufbxt_hash_real(h, v.z);
	ufbxt_hash_real(h, v.w);
}

#define ufbxt_hash_vec2(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_vec2_imp(h, v), ufbxt_pop_tag(h))
#define ufbxt_hash_vec3(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_vec3_imp(h, v), ufbxt_pop_tag(h))
#define ufbxt_hash_vec4(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_vec4_imp(h, v), ufbxt_pop_tag(h))
#define ufbxt_hash_quat(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_quat_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_transform_imp(ufbxt_hash *h, ufbx_transform v)
{
	ufbxt_hash_vec3(h, v.translation);
	ufbxt_hash_quat(h, v.rotation);
	ufbxt_hash_vec3(h, v.scale);
}

ufbxt_noinline static void ufbxt_hash_matrix_imp(ufbxt_hash *h, ufbx_matrix v)
{
	ufbxt_hash_vec3(h, v.cols[0]);
	ufbxt_hash_vec3(h, v.cols[1]);
	ufbxt_hash_vec3(h, v.cols[2]);
	ufbxt_hash_vec3(h, v.cols[3]);
}

ufbxt_noinline static void ufbxt_hash_string_imp(ufbxt_hash *h, ufbx_string v)
{
	ufbxt_hash_size_t(h, v.length);
	ufbxt_push_tag(h, "v.data");
	ufbxt_hash_data(h, v.data, v.length + 1);
	ufbxt_pop_tag(h);
}

ufbxt_noinline static void ufbxt_hash_blob_imp(ufbxt_hash *h, ufbx_blob v)
{
	ufbxt_hash_size_t(h, v.size);
	ufbxt_push_tag(h, "v.data");
	ufbxt_hash_data(h, v.data, v.size);
	ufbxt_pop_tag(h);
}

#define ufbxt_hash_transform(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_transform_imp(h, v), ufbxt_pop_tag(h))
#define ufbxt_hash_matrix(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_matrix_imp(h, v), ufbxt_pop_tag(h))
#define ufbxt_hash_string(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_string_imp(h, v), ufbxt_pop_tag(h))
#define ufbxt_hash_blob(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_blob_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_element_ref_imp(ufbxt_hash *h, const void *v)
{
	const ufbx_element *elem = (const ufbx_element*)v;
	uint32_t id = elem ? elem->element_id : UINT32_MAX;
	ufbxt_hash_pod(h, id);
}

ufbxt_noinline static void ufbxt_hash_prop_ref_imp(ufbxt_hash *h, const ufbx_prop *prop)
{
	if (!prop) {
		uint32_t zero = UINT32_MAX;
		ufbxt_hash_pod(h, zero);
	} else {
		ufbxt_hash_string(h, prop->name);
	}
}

#define ufbxt_hash_prop_ref(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_prop_ref_imp(h, v), ufbxt_pop_tag(h))
#define ufbxt_hash_element_ref(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_element_ref_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_connection_imp(ufbxt_hash *h, ufbx_connection v)
{
	ufbxt_hash_element_ref(h, v.src);
	ufbxt_hash_element_ref(h, v.dst);
	ufbxt_hash_string(h, v.src_prop);
	ufbxt_hash_string(h, v.dst_prop);
}

#define ufbxt_hash_connection(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_connection_imp(h, v), ufbxt_pop_tag(h))

#define ufbxt_hash_array(h, v, func) do { \
		ufbxt_push_tag(h, #v); \
		for (size_t i = 0; i < sizeof(v) / sizeof(*(v)); i++) { \
			ufbxt_push_tag_index(h, i); \
			func((h), (v)[i]); \
			ufbxt_pop_tag(h); \
		} \
		ufbxt_pop_tag(h); \
	} while (0)

#define ufbxt_hash_list(h, v, func) do { \
		ufbxt_push_tag(h, #v); \
		size_t count = (v).count; \
		ufbxt_hash_size_t((h), count); \
		ufbxt_push_tag(h, "data"); \
		for (size_t i = 0; i < count; i++) { \
			ufbxt_push_tag_index(h, i); \
			func((h), (v).data[i]); \
			ufbxt_pop_tag(h); \
		} \
		ufbxt_pop_tag(h); \
		ufbxt_pop_tag(h); \
	} while (0)

#define ufbxt_hash_list_ptr(h, v, func) do { \
		ufbxt_push_tag(h, #v); \
		size_t count = (v).count; \
		ufbxt_hash_size_t((h), count); \
		ufbxt_push_tag(h, "data"); \
		for (size_t i = 0; i < count; i++) { \
			ufbxt_push_tag_index(h, i); \
			func((h), &(v).data[i]); \
			ufbxt_pop_tag(h); \
		} \
		ufbxt_pop_tag(h); \
		ufbxt_pop_tag(h); \
	} while (0)

ufbxt_noinline static void ufbxt_hash_prop_imp(ufbxt_hash *h, const ufbx_prop *v)
{
	ufbxt_hash_string(h, v->name);
	ufbxt_hash_pod(h, v->_internal_key);
	ufbxt_hash_pod(h, v->type);
	ufbxt_hash_pod(h, v->flags);
	ufbxt_hash_string(h, v->value_str);
	ufbxt_hash_blob(h, v->value_blob);
	ufbxt_hash_pod(h, v->value_int);
	ufbxt_hash_vec4(h, v->value_vec4);
}

#define ufbxt_hash_prop(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_prop_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_dom_value_imp(ufbxt_hash *h, const ufbx_dom_value *v)
{
	ufbxt_hash_pod(h, v->type);
	ufbxt_hash_string(h, v->value_str);
	size_t count = (size_t)v->value_int;
	if (v->type == UFBX_DOM_VALUE_ARRAY_BLOB) {
		const ufbx_blob *vs = (const ufbx_blob*)v->value_blob.data;
		for (size_t i = 0; i < count; i++) {
			ufbxt_hash_blob(h, vs[i]);
		}
	} else if (v->type == UFBX_DOM_VALUE_ARRAY_F32) {
		const float *vs = (const float*)v->value_blob.data;
		for (size_t i = 0; i < count; i++) {
			ufbxt_hash_float(h, vs[i]);
		}
	} else if (v->type == UFBX_DOM_VALUE_ARRAY_F64) {
		const double *vs = (const double*)v->value_blob.data;
		for (size_t i = 0; i < count; i++) {
			ufbxt_hash_double(h, vs[i]);
		}
	} else {
		ufbxt_hash_blob(h, v->value_blob);
	}
	ufbxt_hash_pod(h, v->value_int);
	ufbxt_hash_double(h, v->value_float);
}

#define ufbxt_hash_dom_value(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_dom_value_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_dom_node_imp(ufbxt_hash *h, const ufbx_dom_node *v)
{
	ufbxt_hash_string(h, v->name);
	ufbxt_hash_list(h, v->children, ufbxt_hash_dom_node_imp);
	ufbxt_hash_list_ptr(h, v->values, ufbxt_hash_dom_value_imp);
}

#define ufbxt_hash_dom_node(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_dom_node_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_props_imp(ufbxt_hash *h, const ufbx_props *v)
{
	ufbxt_hash_list_ptr(h, v->props, ufbxt_hash_prop_imp);
	ufbxt_hash_size_t(h, v->num_animated);
	if (v->defaults) {
		ufbxt_push_tag(h, "defaults");
		ufbxt_hash_props_imp(h, v->defaults);
		ufbxt_pop_tag(h);
	}
}

#define ufbxt_hash_props(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_props_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_element_imp(ufbxt_hash *h, const ufbx_element *v)
{
	ufbxt_hash_string(h, v->name);
	ufbxt_hash_props(h, &v->props);
	ufbxt_hash_pod(h, v->element_id);
	ufbxt_hash_pod(h, v->typed_id);
	ufbxt_hash_list(h, v->instances, ufbxt_hash_element_ref_imp);
	ufbxt_hash_pod(h, v->type);
	ufbxt_hash_list(h, v->connections_src, ufbxt_hash_connection_imp);
	ufbxt_hash_list(h, v->connections_dst, ufbxt_hash_connection_imp);
}

ufbxt_noinline static void ufbxt_hash_unknown_imp(ufbxt_hash *h, const ufbx_unknown *v)
{
	ufbxt_hash_string(h, v->type);
	ufbxt_hash_string(h, v->super_type);
	ufbxt_hash_string(h, v->sub_type);
}

ufbxt_noinline static void ufbxt_hash_node_imp(ufbxt_hash *h, const ufbx_node *v)
{
	ufbxt_hash_element_ref(h, v->parent);
	ufbxt_hash_list(h, v->children, ufbxt_hash_element_ref_imp);
	ufbxt_hash_element_ref(h, v->mesh);
	ufbxt_hash_element_ref(h, v->light);
	ufbxt_hash_element_ref(h, v->camera);
	ufbxt_hash_element_ref(h, v->bone);
	ufbxt_hash_element_ref(h, v->attrib);
	ufbxt_hash_element_ref(h, v->geometry_transform_helper);
	ufbxt_hash_element_ref(h, v->scale_helper);
	ufbxt_hash_pod(h, v->attrib_type);
	ufbxt_hash_list(h, v->all_attribs, ufbxt_hash_element_ref_imp);
	ufbxt_hash_pod(h, v->inherit_mode);
	ufbxt_hash_pod(h, v->original_inherit_mode);
	ufbxt_hash_transform(h, v->local_transform);
	ufbxt_hash_transform(h, v->geometry_transform);
	ufbxt_hash_vec3(h, v->inherit_scale);
	ufbxt_hash_pod(h, v->rotation_order);
	ufbxt_hash_vec3(h, v->euler_rotation);
	ufbxt_hash_matrix(h, v->node_to_parent);
	ufbxt_hash_matrix(h, v->node_to_world);
	ufbxt_hash_matrix(h, v->geometry_to_node);
	ufbxt_hash_matrix(h, v->geometry_to_world);
	ufbxt_hash_matrix(h, v->unscaled_node_to_world);
	ufbxt_hash_quat(h, v->adjust_pre_rotation);
	ufbxt_hash_real(h, v->adjust_pre_scale);
	ufbxt_hash_quat(h, v->adjust_post_rotation);
	ufbxt_hash_real(h, v->adjust_post_scale);
	ufbxt_hash_real(h, v->adjust_mirror_axis);
	ufbxt_hash_pod(h, v->visible);
	ufbxt_hash_pod(h, v->is_root);
	ufbxt_hash_pod(h, v->has_geometry_transform);
	ufbxt_hash_pod(h, v->has_root_adjust_transform);
	ufbxt_hash_pod(h, v->has_adjust_transform);
	ufbxt_hash_pod(h, v->is_geometry_transform_helper);
	ufbxt_hash_pod(h, v->is_scale_helper);
	ufbxt_hash_pod(h, v->is_scale_compensate_parent);
	ufbxt_hash_pod(h, v->node_depth);
	ufbxt_hash_list(h, v->materials, ufbxt_hash_element_ref_imp);
}

ufbxt_noinline static void ufbxt_hash_vertex_real_imp(ufbxt_hash *h, const ufbx_vertex_real *v)
{
	ufbxt_hash_pod(h, v->exists);
	ufbxt_hash_list(h, v->values, ufbxt_hash_real_imp);
	ufbxt_hash_list(h, v->indices, ufbxt_hash_pod_imp);
	ufbxt_hash_size_t(h, v->value_reals);
	ufbxt_hash_pod(h, v->unique_per_vertex);
	ufbxt_hash_list(h, v->values_w, ufbxt_hash_real_imp);
}

ufbxt_noinline static void ufbxt_hash_vertex_vec2_imp(ufbxt_hash *h, const ufbx_vertex_vec2 *v)
{
	ufbxt_hash_pod(h, v->exists);
	ufbxt_hash_list(h, v->values, ufbxt_hash_vec2_imp);
	ufbxt_hash_list(h, v->indices, ufbxt_hash_pod_imp);
	ufbxt_hash_size_t(h, v->value_reals);
	ufbxt_hash_pod(h, v->unique_per_vertex);
	ufbxt_hash_list(h, v->values_w, ufbxt_hash_real_imp);
}

ufbxt_noinline static void ufbxt_hash_vertex_vec3_imp(ufbxt_hash *h, const ufbx_vertex_vec3 *v)
{
	ufbxt_hash_pod(h, v->exists);
	ufbxt_hash_list(h, v->values, ufbxt_hash_vec3_imp);
	ufbxt_hash_list(h, v->indices, ufbxt_hash_pod_imp);
	ufbxt_hash_size_t(h, v->value_reals);
	ufbxt_hash_pod(h, v->unique_per_vertex);
	ufbxt_hash_list(h, v->values_w, ufbxt_hash_real_imp);
}

ufbxt_noinline static void ufbxt_hash_vertex_vec4_imp(ufbxt_hash *h, const ufbx_vertex_vec4 *v)
{
	ufbxt_hash_pod(h, v->exists);
	ufbxt_hash_list(h, v->values, ufbxt_hash_vec4);
	ufbxt_hash_list(h, v->indices, ufbxt_hash_pod);
	ufbxt_hash_size_t(h, v->value_reals);
	ufbxt_hash_pod(h, v->unique_per_vertex);
	ufbxt_hash_list(h, v->values_w, ufbxt_hash_real_imp);
}

#define ufbxt_hash_vertex_real(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_vertex_real_imp(h, v), ufbxt_pop_tag(h))
#define ufbxt_hash_vertex_vec2(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_vertex_vec2_imp(h, v), ufbxt_pop_tag(h))
#define ufbxt_hash_vertex_vec3(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_vertex_vec3_imp(h, v), ufbxt_pop_tag(h))
#define ufbxt_hash_vertex_vec4(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_vertex_vec4_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_uv_set_imp(ufbxt_hash *h, const ufbx_uv_set *v)
{
	ufbxt_hash_string(h, v->name);
	ufbxt_hash_pod(h, v->index);
	ufbxt_hash_vertex_vec2(h, &v->vertex_uv);
	ufbxt_hash_vertex_vec3(h, &v->vertex_tangent);
	ufbxt_hash_vertex_vec3(h, &v->vertex_bitangent);
}

ufbxt_noinline static void ufbxt_hash_color_set_imp(ufbxt_hash *h, const ufbx_color_set *v)
{
	ufbxt_hash_string(h, v->name);
	ufbxt_hash_pod(h, v->index);
	ufbxt_hash_vertex_vec4(h, &v->vertex_color);
}

ufbxt_noinline static void ufbxt_hash_mesh_part_imp(ufbxt_hash *h, const ufbx_mesh_part *v)
{
	ufbxt_hash_pod(h, v->index);
	ufbxt_hash_size_t(h, v->num_faces);
	ufbxt_hash_size_t(h, v->num_triangles);
	ufbxt_hash_size_t(h, v->num_empty_faces);
	ufbxt_hash_size_t(h, v->num_point_faces);
	ufbxt_hash_size_t(h, v->num_line_faces);
	ufbxt_hash_list(h, v->face_indices, ufbxt_hash_pod_imp);
}

ufbxt_noinline static void ufbxt_hash_face_group_imp(ufbxt_hash *h, const ufbx_face_group *v)
{
	ufbxt_hash_pod(h, v->id);
	ufbxt_hash_string(h, v->name);
}

ufbxt_noinline static void ufbxt_hash_subdivision_weight_range_imp(ufbxt_hash *h, ufbx_subdivision_weight_range v)
{
	ufbxt_hash_pod(h, v.weight_begin);
	ufbxt_hash_pod(h, v.num_weights);
}

ufbxt_noinline static void ufbxt_hash_subdivision_weight_imp(ufbxt_hash *h, ufbx_subdivision_weight v)
{
	ufbxt_hash_real(h, v.weight);
	ufbxt_hash_pod(h, v.index);
}

ufbxt_noinline static void ufbxt_hash_subdivision_result_imp(ufbxt_hash *h, const ufbx_subdivision_result *v)
{
	ufbxt_hash_list(h, v->source_vertex_ranges, ufbxt_hash_subdivision_weight_range_imp);
	ufbxt_hash_list(h, v->source_vertex_weights, ufbxt_hash_subdivision_weight_imp);
	ufbxt_hash_list(h, v->skin_cluster_ranges, ufbxt_hash_subdivision_weight_range_imp);
	ufbxt_hash_list(h, v->skin_cluster_weights, ufbxt_hash_subdivision_weight_imp);
}

ufbxt_noinline static void ufbxt_hash_face_imp(ufbxt_hash *h, ufbx_face v)
{
	ufbxt_hash_pod(h, v.index_begin);
	ufbxt_hash_pod(h, v.num_indices);
}

ufbxt_noinline static void ufbxt_hash_edge_imp(ufbxt_hash *h, ufbx_edge v)
{
	ufbxt_hash_pod(h, v.a);
	ufbxt_hash_pod(h, v.b);
}

#define ufbxt_hash_subdivision_result(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_subdivision_result_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_mesh_imp(ufbxt_hash *h, const ufbx_mesh *v)
{
	ufbxt_hash_size_t(h, v->num_vertices);
	ufbxt_hash_size_t(h, v->num_indices);
	ufbxt_hash_size_t(h, v->num_faces);
	ufbxt_hash_size_t(h, v->num_triangles);
	ufbxt_hash_size_t(h, v->num_edges);

	ufbxt_hash_list(h, v->faces, ufbxt_hash_face_imp);
	ufbxt_hash_list(h, v->face_smoothing, ufbxt_hash_pod_imp);
	ufbxt_hash_list(h, v->face_material, ufbxt_hash_pod_imp);
	ufbxt_hash_list(h, v->face_group, ufbxt_hash_pod_imp);
	ufbxt_hash_list(h, v->face_hole, ufbxt_hash_pod_imp);
	ufbxt_hash_size_t(h, v->max_face_triangles);
	ufbxt_hash_size_t(h, v->num_empty_faces);
	ufbxt_hash_size_t(h, v->num_point_faces);
	ufbxt_hash_size_t(h, v->num_line_faces);

	ufbxt_hash_list(h, v->edges, ufbxt_hash_edge_imp);
	ufbxt_hash_list(h, v->edge_smoothing, ufbxt_hash_pod_imp);
	ufbxt_hash_list(h, v->edge_crease, ufbxt_hash_real_imp);
	ufbxt_hash_list(h, v->edge_visibility, ufbxt_hash_pod_imp);

	ufbxt_hash_list(h, v->vertex_indices, ufbxt_hash_pod_imp);
	ufbxt_hash_list(h, v->vertices, ufbxt_hash_vec3_imp);
	ufbxt_hash_list(h, v->vertex_first_index, ufbxt_hash_pod_imp);

	ufbxt_hash_vertex_vec3(h, &v->vertex_position);
	ufbxt_hash_vertex_vec3(h, &v->vertex_normal);
	ufbxt_hash_vertex_vec2(h, &v->vertex_uv);
	ufbxt_hash_vertex_vec3(h, &v->vertex_tangent);
	ufbxt_hash_vertex_vec3(h, &v->vertex_bitangent);
	ufbxt_hash_vertex_vec4(h, &v->vertex_color);
	ufbxt_hash_vertex_real(h, &v->vertex_crease);

	ufbxt_hash_list_ptr(h, v->uv_sets, ufbxt_hash_uv_set_imp);
	ufbxt_hash_list_ptr(h, v->color_sets, ufbxt_hash_color_set_imp);

	ufbxt_hash_list(h, v->materials, ufbxt_hash_element_ref_imp);
	ufbxt_hash_list_ptr(h, v->face_groups, ufbxt_hash_face_group_imp);

	ufbxt_hash_list_ptr(h, v->material_parts, ufbxt_hash_mesh_part_imp);
	ufbxt_hash_list_ptr(h, v->face_group_parts, ufbxt_hash_mesh_part_imp);

	ufbxt_hash_pod(h, v->skinned_is_local);
	ufbxt_hash_vertex_vec3(h, &v->skinned_position);
	ufbxt_hash_vertex_vec3(h, &v->skinned_normal);

	ufbxt_hash_list(h, v->skin_deformers, ufbxt_hash_element_ref_imp);
	ufbxt_hash_list(h, v->blend_deformers, ufbxt_hash_element_ref_imp);
	ufbxt_hash_list(h, v->cache_deformers, ufbxt_hash_element_ref_imp);
	ufbxt_hash_list(h, v->all_deformers, ufbxt_hash_element_ref_imp);

	ufbxt_hash_pod(h, v->subdivision_preview_levels);
	ufbxt_hash_pod(h, v->subdivision_render_levels);
	ufbxt_hash_pod(h, v->subdivision_display_mode);
	ufbxt_hash_pod(h, v->subdivision_boundary);
	ufbxt_hash_pod(h, v->subdivision_uv_boundary);

	ufbxt_hash_pod(h, v->subdivision_evaluated);
	if (v->subdivision_result) ufbxt_hash_subdivision_result(h, v->subdivision_result);
	ufbxt_hash_pod(h, v->from_tessellated_nurbs);
}

ufbxt_noinline static void ufbxt_hash_light_imp(ufbxt_hash *h, const ufbx_light *v)
{
	ufbxt_hash_vec3(h, v->color);
	ufbxt_hash_real(h, v->intensity);
	ufbxt_hash_vec3(h, v->local_direction);
	ufbxt_hash_pod(h, v->type);
	ufbxt_hash_pod(h, v->decay);
	ufbxt_hash_pod(h, v->area_shape);
	ufbxt_hash_real(h, v->inner_angle);
	ufbxt_hash_real(h, v->outer_angle);
	ufbxt_hash_pod(h, v->cast_light);
	ufbxt_hash_pod(h, v->cast_shadows);
}

ufbxt_noinline static void ufbxt_hash_camera_imp(ufbxt_hash *h, const ufbx_camera *v)
{
	ufbxt_hash_pod(h, v->resolution_is_pixels);
	ufbxt_hash_vec2(h, v->resolution);
	ufbxt_hash_vec2(h, v->field_of_view_deg);
	ufbxt_hash_vec2(h, v->field_of_view_tan);
	ufbxt_hash_pod(h, v->aspect_mode);
	ufbxt_hash_pod(h, v->aperture_mode);
	ufbxt_hash_pod(h, v->gate_fit);
	ufbxt_hash_pod(h, v->aperture_format);
	ufbxt_hash_real(h, v->focal_length_mm);
	ufbxt_hash_vec2(h, v->film_size_inch);
	ufbxt_hash_vec2(h, v->aperture_size_inch);
	ufbxt_hash_real(h, v->squeeze_ratio);
}

ufbxt_noinline static void ufbxt_hash_bone_imp(ufbxt_hash *h, const ufbx_bone *v)
{
	ufbxt_hash_real(h, v->radius);
	ufbxt_hash_real(h, v->relative_length);
	ufbxt_hash_pod(h, v->is_root);
}

ufbxt_noinline static void ufbxt_hash_empty_imp(ufbxt_hash *h, const ufbx_empty *v)
{
}

ufbxt_noinline static void ufbxt_hash_nurbs_basis_imp(ufbxt_hash *h, const ufbx_nurbs_basis *v)
{
	ufbxt_hash_pod(h, v->order);
	ufbxt_hash_pod(h, v->topology);
	ufbxt_hash_list(h, v->knot_vector, ufbxt_hash_real_imp);
	ufbxt_hash_real(h, v->t_min);
	ufbxt_hash_real(h, v->t_max);
	ufbxt_hash_list(h, v->spans, ufbxt_hash_real_imp);
	ufbxt_hash_pod(h, v->is_2d);
	ufbxt_hash_size_t(h, v->num_wrap_control_points);
	ufbxt_hash_pod(h, v->valid);
}

#define ufbxt_hash_nurbs_basis(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_nurbs_basis_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_line_segment_imp(ufbxt_hash *h, ufbx_line_segment v)
{
	ufbxt_hash_pod(h, v.index_begin);
	ufbxt_hash_pod(h, v.num_indices);
}

ufbxt_noinline static void ufbxt_hash_line_curve_imp(ufbxt_hash *h, const ufbx_line_curve *v)
{
	ufbxt_hash_vec3(h, v->color);
	ufbxt_hash_list(h, v->control_points, ufbxt_hash_vec3_imp);
	ufbxt_hash_list(h, v->point_indices, ufbxt_hash_pod_imp);
	ufbxt_hash_list(h, v->segments, ufbxt_hash_line_segment_imp);
	ufbxt_hash_pod(h, v->from_tessellated_nurbs);
}

ufbxt_noinline static void ufbxt_hash_nurbs_curve_imp(ufbxt_hash *h, const ufbx_nurbs_curve *v)
{
	ufbxt_hash_nurbs_basis(h, &v->basis);
	ufbxt_hash_list(h, v->control_points, ufbxt_hash_vec4_imp);
}

ufbxt_noinline static void ufbxt_hash_nurbs_surface_imp(ufbxt_hash *h, const ufbx_nurbs_surface *v)
{
	ufbxt_hash_nurbs_basis(h, &v->basis_u);
	ufbxt_hash_nurbs_basis(h, &v->basis_v);
	ufbxt_hash_size_t(h, v->num_control_points_u);
	ufbxt_hash_size_t(h, v->num_control_points_v);
	ufbxt_hash_list(h, v->control_points, ufbxt_hash_vec4_imp);
	ufbxt_hash_pod(h, v->span_subdivision_u);
	ufbxt_hash_pod(h, v->span_subdivision_v);
	ufbxt_hash_pod(h, v->flip_normals);
	ufbxt_hash_element_ref(h, v->material);
}

ufbxt_noinline static void ufbxt_hash_nurbs_trim_surface_imp(ufbxt_hash *h, const ufbx_nurbs_trim_surface *v)
{
}

ufbxt_noinline static void ufbxt_hash_nurbs_trim_boundary_imp(ufbxt_hash *h, const ufbx_nurbs_trim_boundary *v)
{
}

ufbxt_noinline static void ufbxt_hash_procedural_geometry_imp(ufbxt_hash *h, const ufbx_procedural_geometry *v)
{
}

ufbxt_noinline static void ufbxt_hash_stereo_camera_imp(ufbxt_hash *h, const ufbx_stereo_camera *v)
{
	ufbxt_hash_element_ref(h, v->left);
	ufbxt_hash_element_ref(h, v->right);
}

ufbxt_noinline static void ufbxt_hash_camera_switcher_imp(ufbxt_hash *h, const ufbx_camera_switcher *v)
{
}

ufbxt_noinline static void ufbxt_hash_marker_imp(ufbxt_hash *h, const ufbx_marker *v)
{
	ufbxt_hash_pod(h, v->type);
}

ufbxt_noinline static void ufbxt_hash_lod_level_imp(ufbxt_hash *h, ufbx_lod_level v)
{
	ufbxt_hash_real(h, v.distance);
	ufbxt_hash_pod(h, v.display);
}

ufbxt_noinline static void ufbxt_hash_lod_group_imp(ufbxt_hash *h, const ufbx_lod_group *v)
{
	ufbxt_hash_pod(h, v->relative_distances);
	ufbxt_hash_list(h, v->lod_levels, ufbxt_hash_lod_level_imp);
	ufbxt_hash_pod(h, v->ignore_parent_transform);
	ufbxt_hash_pod(h, v->use_distance_limit);
	ufbxt_hash_real(h, v->distance_limit_min);
	ufbxt_hash_real(h, v->distance_limit_max);
}

ufbxt_noinline static void ufbxt_hash_skin_vertex_imp(ufbxt_hash *h, ufbx_skin_vertex v)
{
	ufbxt_hash_pod(h, v.weight_begin);
	ufbxt_hash_pod(h, v.num_weights);
	ufbxt_hash_real(h, v.dq_weight);
}

ufbxt_noinline static void ufbxt_hash_skin_weight_imp(ufbxt_hash *h, ufbx_skin_weight v)
{
	ufbxt_hash_pod(h, v.cluster_index);
	ufbxt_hash_real(h, v.weight);
}

ufbxt_noinline static void ufbxt_hash_skin_deformer_imp(ufbxt_hash *h, const ufbx_skin_deformer *v)
{
	ufbxt_hash_pod(h, v->skinning_method);
	ufbxt_hash_list(h, v->clusters, ufbxt_hash_element_ref_imp);
	ufbxt_hash_list(h, v->vertices, ufbxt_hash_skin_vertex_imp);
	ufbxt_hash_list(h, v->weights, ufbxt_hash_skin_weight_imp);
	ufbxt_hash_size_t(h, v->max_weights_per_vertex);
	ufbxt_hash_size_t(h, v->num_dq_weights);
	ufbxt_hash_list(h, v->dq_vertices, ufbxt_hash_pod_imp);
	ufbxt_hash_list(h, v->dq_weights, ufbxt_hash_real_imp);
}

ufbxt_noinline static void ufbxt_hash_skin_cluster_imp(ufbxt_hash *h, const ufbx_skin_cluster *v)
{
	ufbxt_hash_element_ref(h, v->bone_node);
	ufbxt_hash_matrix(h, v->geometry_to_bone);
	ufbxt_hash_matrix(h, v->mesh_node_to_bone);
	ufbxt_hash_matrix(h, v->bind_to_world);
	ufbxt_hash_matrix(h, v->geometry_to_world);
	ufbxt_hash_transform(h, v->geometry_to_world_transform);
	ufbxt_hash_size_t(h, v->num_weights);
	ufbxt_hash_list(h, v->vertices, ufbxt_hash_pod_imp);
	ufbxt_hash_list(h, v->weights, ufbxt_hash_real_imp);
}

ufbxt_noinline static void ufbxt_hash_blend_deformer_imp(ufbxt_hash *h, const ufbx_blend_deformer *v)
{
	ufbxt_hash_list(h, v->channels, ufbxt_hash_element_ref_imp);
}

ufbxt_noinline static void ufbxt_hash_blend_keyframe_imp(ufbxt_hash *h, const ufbx_blend_keyframe *v)
{
	ufbxt_hash_element_ref(h, v->shape);
	ufbxt_hash_real(h, v->target_weight);
	ufbxt_hash_real(h, v->effective_weight);
}

ufbxt_noinline static void ufbxt_hash_blend_channel_imp(ufbxt_hash *h, const ufbx_blend_channel *v)
{
	ufbxt_hash_real(h, v->weight);
	ufbxt_hash_list_ptr(h, v->keyframes, ufbxt_hash_blend_keyframe_imp);
}

ufbxt_noinline static void ufbxt_hash_blend_shape_imp(ufbxt_hash *h, const ufbx_blend_shape *v)
{
	ufbxt_hash_size_t(h, v->num_offsets);
	ufbxt_hash_list(h, v->offset_vertices, ufbxt_hash_pod_imp);
	ufbxt_hash_list(h, v->position_offsets, ufbxt_hash_vec3_imp);
	ufbxt_hash_list(h, v->normal_offsets, ufbxt_hash_vec3_imp);
}

ufbxt_noinline static void ufbxt_hash_cache_frame_imp(ufbxt_hash *h, const ufbx_cache_frame *v)
{
	ufbxt_hash_string(h, v->channel);
	ufbxt_hash_double(h, v->time);
	ufbxt_hash_pod(h, v->file_format);
	ufbxt_hash_pod(h, v->data_format);
	ufbxt_hash_pod(h, v->data_encoding);
	ufbxt_hash_pod(h, v->data_offset);
	ufbxt_hash_pod(h, v->data_count);
	ufbxt_hash_pod(h, v->data_element_bytes);
	ufbxt_hash_pod(h, v->data_total_bytes);
}

ufbxt_noinline static void ufbxt_hash_cache_channel_imp(ufbxt_hash *h, const ufbx_cache_channel *v)
{
	ufbxt_hash_string(h, v->name);
	ufbxt_hash_pod(h, v->interpretation);
	ufbxt_hash_string(h, v->interpretation_name);
	ufbxt_hash_list_ptr(h, v->frames, ufbxt_hash_cache_frame_imp);
}

ufbxt_noinline static void ufbxt_hash_geometry_cache_imp(ufbxt_hash *h, const ufbx_geometry_cache *v)
{
	ufbxt_hash_list_ptr(h, v->channels, ufbxt_hash_cache_channel_imp);
	ufbxt_hash_list_ptr(h, v->frames, ufbxt_hash_cache_frame_imp);
	ufbxt_hash_list(h, v->extra_info, ufbxt_hash_string_imp);
}

#define ufbxt_hash_geometry_cache(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_geometry_cache_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_cache_deformer_imp(ufbxt_hash *h, const ufbx_cache_deformer *v)
{
	ufbxt_hash_string(h, v->channel);
	ufbxt_hash_element_ref(h, v->file);
}

ufbxt_noinline static void ufbxt_hash_cache_file_imp(ufbxt_hash *h, const ufbx_cache_file *v)
{
	ufbxt_hash_string(h, v->absolute_filename);
	ufbxt_hash_string(h, v->relative_filename);
	ufbxt_hash_blob(h, v->raw_absolute_filename);
	ufbxt_hash_blob(h, v->raw_relative_filename);

	if (v->external_cache) ufbxt_hash_geometry_cache(h, v->external_cache);
}

ufbxt_noinline static void ufbxt_hash_material_map_imp(ufbxt_hash *h, const ufbx_material_map *v)
{
	ufbxt_hash_vec4(h, v->value_vec4);
	ufbxt_hash_pod(h, v->value_int);
	ufbxt_hash_element_ref(h, v->texture);
	ufbxt_hash_pod(h, v->has_value);
	ufbxt_hash_pod(h, v->texture_enabled);
	ufbxt_hash_pod(h, v->value_components);
}

ufbxt_noinline static void ufbxt_hash_material_feature_imp(ufbxt_hash *h, const ufbx_material_feature_info *v)
{
	ufbxt_hash_pod(h, v->enabled);
	ufbxt_hash_pod(h, v->is_explicit);
}

ufbxt_noinline static void ufbxt_hash_material_texture_imp(ufbxt_hash *h, const ufbx_material_texture *v)
{
	ufbxt_hash_string(h, v->material_prop);
	ufbxt_hash_string(h, v->shader_prop);
	ufbxt_hash_element_ref(h, v->texture);
}

ufbxt_noinline static void ufbxt_hash_material_imp(ufbxt_hash *h, const ufbx_material *v)
{
	ufbxt_push_tag(h, "fbx");
	for (size_t i = 0; i < UFBX_MATERIAL_FBX_MAP_COUNT; i++) {
		ufbxt_push_tag_index(h, i);
		ufbxt_hash_material_map_imp(h, &v->fbx.maps[i]);
		ufbxt_pop_tag(h);
	}
	ufbxt_pop_tag(h);

	ufbxt_push_tag(h, "pbr");
	for (size_t i = 0; i < UFBX_MATERIAL_PBR_MAP_COUNT; i++) {
		ufbxt_push_tag_index(h, i);
		ufbxt_hash_material_map_imp(h, &v->pbr.maps[i]);
		ufbxt_pop_tag(h);
	}
	ufbxt_pop_tag(h);

	ufbxt_push_tag(h, "features");
	for (size_t i = 0; i < UFBX_MATERIAL_FEATURE_COUNT; i++) {
		ufbxt_push_tag_index(h, i);
		ufbxt_hash_material_feature_imp(h, &v->features.features[i]);
		ufbxt_pop_tag(h);
	}
	ufbxt_pop_tag(h);

	ufbxt_hash_pod(h, v->shader_type);
	ufbxt_hash_element_ref(h, v->shader);
	ufbxt_hash_string(h, v->shading_model_name);

	ufbxt_hash_string(h, v->shader_prop_prefix);
	ufbxt_hash_list_ptr(h, v->textures, ufbxt_hash_material_texture_imp);
}

ufbxt_noinline static void ufbxt_hash_texture_layer_imp(ufbxt_hash *h, const ufbx_texture_layer v)
{
	ufbxt_hash_element_ref(h, v.texture);
	ufbxt_hash_pod(h, v.blend_mode);
	ufbxt_hash_real(h, v.alpha);
}

ufbxt_noinline static void ufbxt_hash_shader_texture_input_imp(ufbxt_hash *h, const ufbx_shader_texture_input *v)
{
	ufbxt_hash_string(h, v->name);
	ufbxt_hash_vec4(h, v->value_vec4);
	ufbxt_hash_pod(h, v->value_int);
	ufbxt_hash_string(h, v->value_str);
	ufbxt_hash_blob(h, v->value_blob);
	ufbxt_hash_element_ref(h, v->texture);
	ufbxt_hash_pod(h, v->texture_enabled);
	ufbxt_hash_prop_ref(h, v->prop);
	ufbxt_hash_prop_ref(h, v->texture_prop);
	ufbxt_hash_prop_ref(h, v->texture_enabled_prop);
}

ufbxt_noinline static void ufbxt_hash_shader_texture_imp(ufbxt_hash *h, const ufbx_shader_texture *v)
{
	ufbxt_hash_pod(h, v->type);
	ufbxt_hash_string(h, v->shader_name);
	ufbxt_hash_pod(h, v->shader_type_id);
	ufbxt_hash_list_ptr(h, v->inputs, ufbxt_hash_shader_texture_input_imp);
	ufbxt_hash_string(h, v->shader_source);
	ufbxt_hash_blob(h, v->raw_shader_source);
	ufbxt_hash_element_ref(h, v->main_texture);
	ufbxt_hash_pod(h, v->main_texture_output_index);
	ufbxt_hash_string(h, v->prop_prefix);
}

#define ufbxt_hash_shader_texture(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_shader_texture_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_texture_imp(ufbxt_hash *h, const ufbx_texture *v)
{
	ufbxt_hash_pod(h, v->type);

	ufbxt_hash_string(h, v->absolute_filename);
	ufbxt_hash_string(h, v->relative_filename);
	ufbxt_hash_blob(h, v->raw_absolute_filename);
	ufbxt_hash_blob(h, v->raw_relative_filename);

	ufbxt_hash_blob(h, v->content);
	ufbxt_hash_element_ref(h, v->video);

	ufbxt_hash_pod(h, v->file_index);
	ufbxt_hash_pod(h, v->has_file);

	ufbxt_hash_list(h, v->layers, ufbxt_hash_texture_layer_imp);

	ufbxt_hash_string(h, v->uv_set);
	ufbxt_hash_pod(h, v->wrap_u);
	ufbxt_hash_pod(h, v->wrap_v);
	ufbxt_hash_pod(h, v->has_uv_transform);
	ufbxt_hash_transform(h, v->uv_transform);
	ufbxt_hash_matrix(h, v->texture_to_uv);
	ufbxt_hash_matrix(h, v->uv_to_texture);

	ufbxt_hash_list(h, v->file_textures, ufbxt_hash_element_ref_imp);

	if (v->shader) {
		ufbxt_hash_shader_texture(h, v->shader);
	}
}

ufbxt_noinline static void ufbxt_hash_video_imp(ufbxt_hash *h, const ufbx_video *v)
{
	ufbxt_hash_string(h, v->absolute_filename);
	ufbxt_hash_string(h, v->relative_filename);
	ufbxt_hash_blob(h, v->raw_absolute_filename);
	ufbxt_hash_blob(h, v->raw_relative_filename);

	ufbxt_hash_blob(h, v->content);
}

ufbxt_noinline static void ufbxt_hash_shader_imp(ufbxt_hash *h, const ufbx_shader *v)
{
	ufbxt_hash_pod(h, v->type);
	ufbxt_hash_list(h, v->bindings, ufbxt_hash_element_ref_imp);
}

ufbxt_noinline static void ufbxt_hash_shader_prop_binding_imp(ufbxt_hash *h, ufbx_shader_prop_binding v)
{
	ufbxt_hash_string(h, v.shader_prop);
	ufbxt_hash_string(h, v.material_prop);
}

ufbxt_noinline static void ufbxt_hash_shader_binding_imp(ufbxt_hash *h, const ufbx_shader_binding *v)
{
	ufbxt_hash_list(h, v->prop_bindings, ufbxt_hash_shader_prop_binding_imp);
}

ufbxt_noinline static void ufbxt_hash_prop_override_imp(ufbxt_hash *h, const ufbx_prop_override *v)
{
	ufbxt_hash_pod(h, v->element_id);
	ufbxt_hash_pod(h, v->_internal_key);
	ufbxt_hash_string(h, v->prop_name);
	ufbxt_hash_vec4(h, v->value);
	ufbxt_hash_string(h, v->value_str);
	ufbxt_hash_pod(h, v->value_int);
}

ufbxt_noinline static void ufbxt_hash_transform_override_imp(ufbxt_hash *h, const ufbx_transform_override *v)
{
	ufbxt_hash_pod(h, v->node_id);
	ufbxt_hash_transform(h, v->transform);
}

ufbxt_noinline static void ufbxt_hash_anim_imp(ufbxt_hash *h, const ufbx_anim *v)
{
	ufbxt_hash_double(h, v->time_begin);
	ufbxt_hash_double(h, v->time_end);
	ufbxt_hash_list(h, v->layers, ufbxt_hash_element_ref_imp);
	ufbxt_hash_list(h, v->override_layer_weights, ufbxt_hash_pod_imp);
	ufbxt_hash_list_ptr(h, v->prop_overrides, ufbxt_hash_prop_override_imp);
	ufbxt_hash_list_ptr(h, v->transform_overrides, ufbxt_hash_transform_override_imp);
	ufbxt_hash_pod(h, v->ignore_connections);
	ufbxt_hash_pod(h, v->custom);
}

#define ufbxt_hash_anim(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_anim_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_anim_stack_imp(ufbxt_hash *h, const ufbx_anim_stack *v)
{
	ufbxt_hash_double(h, v->time_begin);
	ufbxt_hash_double(h, v->time_end);

	ufbxt_hash_list(h, v->layers, ufbxt_hash_element_ref_imp);
	ufbxt_hash_anim(h, v->anim);
}

ufbxt_noinline static void ufbxt_hash_anim_prop_imp(ufbxt_hash *h, const ufbx_anim_prop *v)
{
	ufbxt_hash_element_ref(h, v->element);
	ufbxt_hash_pod(h, v->_internal_key);
	ufbxt_hash_string(h, v->prop_name);
	ufbxt_hash_element_ref(h, v->anim_value);
}

ufbxt_noinline static void ufbxt_hash_anim_layer_imp(ufbxt_hash *h, const ufbx_anim_layer *v)
{
	ufbxt_hash_real(h, v->weight);
	ufbxt_hash_pod(h, v->weight_is_animated);
	ufbxt_hash_pod(h, v->blended);
	ufbxt_hash_pod(h, v->additive);
	ufbxt_hash_pod(h, v->compose_rotation);
	ufbxt_hash_pod(h, v->compose_scale);

	ufbxt_hash_list(h, v->anim_values, ufbxt_hash_element_ref_imp);
	ufbxt_hash_list_ptr(h, v->anim_props, ufbxt_hash_anim_prop_imp);

	ufbxt_hash_anim(h, v->anim);

	ufbxt_hash_pod(h, v->_min_element_id);
	ufbxt_hash_pod(h, v->_max_element_id);
	ufbxt_hash_pod(h, v->_element_id_bitmask[0]);
	ufbxt_hash_pod(h, v->_element_id_bitmask[1]);
	ufbxt_hash_pod(h, v->_element_id_bitmask[2]);
	ufbxt_hash_pod(h, v->_element_id_bitmask[3]);
}

ufbxt_noinline static void ufbxt_hash_anim_value_imp(ufbxt_hash *h, const ufbx_anim_value *v)
{
	ufbxt_hash_vec3(h, v->default_value);
	ufbxt_hash_array(h, v->curves, ufbxt_hash_element_ref_imp);
}

ufbxt_noinline static void ufbxt_hash_tangent_imp(ufbxt_hash *h, ufbx_tangent v)
{
	ufbxt_hash_float(h, v.dx);
	ufbxt_hash_float(h, v.dy);
}

#define ufbxt_hash_tangent(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_tangent_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_keyframe_imp(ufbxt_hash *h, const ufbx_keyframe *v)
{
	ufbxt_hash_double(h, v->time);
	ufbxt_hash_real(h, v->value);
	ufbxt_hash_pod(h, v->interpolation);
	ufbxt_hash_tangent(h, v->left);
	ufbxt_hash_tangent(h, v->right);
}

ufbxt_noinline static void ufbxt_hash_anim_curve_imp(ufbxt_hash *h, const ufbx_anim_curve *v)
{
	ufbxt_hash_list_ptr(h, v->keyframes, ufbxt_hash_keyframe_imp);
	ufbxt_hash_real(h, v->min_value);
	ufbxt_hash_real(h, v->max_value);
}

ufbxt_noinline static void ufbxt_hash_display_layer_imp(ufbxt_hash *h, const ufbx_display_layer *v)
{
	ufbxt_hash_list(h, v->nodes, ufbxt_hash_element_ref_imp);
	ufbxt_hash_pod(h, v->visible);
	ufbxt_hash_pod(h, v->frozen);
	ufbxt_hash_vec3(h, v->ui_color);
}

ufbxt_noinline static void ufbxt_hash_selection_set_imp(ufbxt_hash *h, const ufbx_selection_set *v)
{
	ufbxt_hash_list(h, v->nodes, ufbxt_hash_element_ref_imp);
}

ufbxt_noinline static void ufbxt_hash_selection_node_imp(ufbxt_hash *h, const ufbx_selection_node *v)
{
	ufbxt_hash_element_ref(h, v->target_node);
	ufbxt_hash_element_ref(h, v->target_mesh);
	ufbxt_hash_pod(h, v->include_node);

	ufbxt_hash_list(h, v->vertices, ufbxt_hash_pod_imp);
	ufbxt_hash_list(h, v->edges, ufbxt_hash_pod_imp);
	ufbxt_hash_list(h, v->faces, ufbxt_hash_pod_imp);
}

ufbxt_noinline static void ufbxt_hash_character_imp(ufbxt_hash *h, const ufbx_character *v)
{
}

ufbxt_noinline static void ufbxt_hash_constraint_target_imp(ufbxt_hash *h, const ufbx_constraint_target *v)
{
	ufbxt_hash_element_ref(h, v->node);
	ufbxt_hash_real(h, v->weight);
	ufbxt_hash_transform(h, v->transform);
}

ufbxt_noinline static void ufbxt_hash_constraint_imp(ufbxt_hash *h, const ufbx_constraint *v)
{
	ufbxt_hash_pod(h, v->type);
	ufbxt_hash_string(h, v->type_name);
	ufbxt_hash_element_ref(h, v->node);
	ufbxt_hash_list_ptr(h, v->targets, ufbxt_hash_constraint_target_imp);
	ufbxt_hash_real(h, v->weight);
	ufbxt_hash_pod(h, v->active);

	ufbxt_hash_array(h, v->constrain_translation, ufbxt_hash_pod_imp);
	ufbxt_hash_array(h, v->constrain_rotation, ufbxt_hash_pod_imp);
	ufbxt_hash_array(h, v->constrain_scale, ufbxt_hash_pod_imp);

	ufbxt_hash_transform(h, v->transform_offset);

	ufbxt_hash_vec3(h, v->aim_vector);
	ufbxt_hash_pod(h, v->aim_up_type);
	ufbxt_hash_element_ref(h, v->aim_up_node);
	ufbxt_hash_vec3(h, v->aim_up_vector);

	ufbxt_hash_element_ref(h, v->ik_effector);
	ufbxt_hash_element_ref(h, v->ik_end_node);
	ufbxt_hash_vec3(h, v->ik_pole_vector);
}

ufbxt_noinline static void ufbxt_hash_audio_layer_imp(ufbxt_hash *h, const ufbx_audio_layer *v)
{
	ufbxt_hash_list(h, v->clips, ufbxt_hash_element_ref_imp);
}

ufbxt_noinline static void ufbxt_hash_audio_clip_imp(ufbxt_hash *h, const ufbx_audio_clip *v)
{
	ufbxt_hash_string(h, v->absolute_filename);
	ufbxt_hash_string(h, v->relative_filename);
	ufbxt_hash_blob(h, v->raw_absolute_filename);
	ufbxt_hash_blob(h, v->raw_relative_filename);
	ufbxt_hash_blob(h, v->content);
}

ufbxt_noinline static void ufbxt_hash_bone_pose_imp(ufbxt_hash *h, const ufbx_bone_pose *v)
{
	ufbxt_hash_element_ref(h, v->bone_node);
	ufbxt_hash_matrix(h, v->bone_to_world);
}

ufbxt_noinline static void ufbxt_hash_pose_imp(ufbxt_hash *h, const ufbx_pose *v)
{
	ufbxt_hash_pod(h, v->is_bind_pose);
	ufbxt_hash_list_ptr(h, v->bone_poses, ufbxt_hash_bone_pose_imp);
}

ufbxt_noinline static void ufbxt_hash_metadata_object_imp(ufbxt_hash *h, const ufbx_metadata_object *v)
{
}

ufbxt_noinline static void ufbxt_hash_texture_file_imp(ufbxt_hash *h, const ufbx_texture_file *v)
{
	ufbxt_hash_pod(h, v->index);
	ufbxt_hash_string(h, v->absolute_filename);
	ufbxt_hash_string(h, v->relative_filename);
	ufbxt_hash_blob(h, v->raw_absolute_filename);
	ufbxt_hash_blob(h, v->raw_relative_filename);
	ufbxt_hash_blob(h, v->content);
}

ufbxt_noinline static void ufbxt_hash_name_element_imp(ufbxt_hash *h, const ufbx_name_element *v)
{
	ufbxt_hash_string(h, v->name);
	ufbxt_hash_pod(h, v->type);
	ufbxt_hash_pod(h, v->_internal_key);
	ufbxt_hash_element_ref(h, v->element);
}

ufbxt_noinline static void ufbxt_hash_application_imp(ufbxt_hash *h, const ufbx_application *v)
{
	ufbxt_hash_string(h, v->vendor);
	ufbxt_hash_string(h, v->name);
	ufbxt_hash_string(h, v->version);
}

#define ufbxt_hash_application(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_application_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_metadata_imp(ufbxt_hash *h, const ufbx_metadata *v)
{
	ufbxt_hash_pod(h, v->ascii);
	ufbxt_hash_pod(h, v->version);
	ufbxt_hash_string(h, v->creator);
	ufbxt_hash_pod(h, v->is_unsafe);
	ufbxt_hash_pod(h, v->big_endian);
	ufbxt_hash_pod(h, v->exporter);
	ufbxt_hash_pod(h, v->exporter_version);
	ufbxt_hash_props(h, &v->scene_props);
	ufbxt_hash_application(h, &v->original_application);
	ufbxt_hash_application(h, &v->latest_application);
	ufbxt_hash_array(h, v->has_warning, ufbxt_hash_pod_imp);
}

#define ufbxt_hash_metadata(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_metadata_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_coordinate_axes_imp(ufbxt_hash *h, ufbx_coordinate_axes v)
{
	ufbxt_hash_pod(h, v.right);
	ufbxt_hash_pod(h, v.up);
	ufbxt_hash_pod(h, v.front);
}

#define ufbxt_hash_coordinate_axes(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_coordinate_axes_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_scene_settings_imp(ufbxt_hash *h, const ufbx_scene_settings *v)
{
	ufbxt_hash_props(h, &v->props);
	ufbxt_hash_coordinate_axes(h, v->axes);
	ufbxt_hash_real(h, v->unit_meters);
	ufbxt_hash_double(h, v->frames_per_second);
	ufbxt_hash_vec3(h, v->ambient_color);
	ufbxt_hash_pod(h, v->time_mode);
	ufbxt_hash_pod(h, v->time_protocol);
	ufbxt_hash_pod(h, v->snap_mode);
	ufbxt_hash_pod(h, v->original_axis_up);
	ufbxt_hash_pod(h, v->original_unit_meters);
}

#define ufbxt_hash_scene_settings(h, v) (ufbxt_push_tag(h, #v), ufbxt_hash_scene_settings_imp(h, v), ufbxt_pop_tag(h))

ufbxt_noinline static void ufbxt_hash_scene_imp(ufbxt_hash *h, const ufbx_scene *v)
{
	ufbxt_hash_metadata(h, &v->metadata);
	ufbxt_hash_scene_settings(h, &v->settings);
	ufbxt_hash_element_ref(h, v->root_node);
	ufbxt_hash_anim(h, v->anim);

	ufbxt_hash_list(h, v->elements, ufbxt_hash_element_imp);

	ufbxt_hash_list(h, v->unknowns, ufbxt_hash_unknown_imp);
	ufbxt_hash_list(h, v->nodes, ufbxt_hash_node_imp);
	ufbxt_hash_list(h, v->meshes, ufbxt_hash_mesh_imp);
	ufbxt_hash_list(h, v->lights, ufbxt_hash_light_imp);
	ufbxt_hash_list(h, v->cameras, ufbxt_hash_camera_imp);
	ufbxt_hash_list(h, v->bones, ufbxt_hash_bone_imp);
	ufbxt_hash_list(h, v->empties, ufbxt_hash_empty_imp);
	ufbxt_hash_list(h, v->line_curves, ufbxt_hash_line_curve_imp);
	ufbxt_hash_list(h, v->nurbs_curves, ufbxt_hash_nurbs_curve_imp);
	ufbxt_hash_list(h, v->nurbs_surfaces, ufbxt_hash_nurbs_surface_imp);
	ufbxt_hash_list(h, v->nurbs_trim_surfaces, ufbxt_hash_nurbs_trim_surface_imp);
	ufbxt_hash_list(h, v->nurbs_trim_boundaries, ufbxt_hash_nurbs_trim_boundary_imp);
	ufbxt_hash_list(h, v->procedural_geometries, ufbxt_hash_procedural_geometry_imp);
	ufbxt_hash_list(h, v->stereo_cameras, ufbxt_hash_stereo_camera_imp);
	ufbxt_hash_list(h, v->camera_switchers, ufbxt_hash_camera_switcher_imp);
	ufbxt_hash_list(h, v->markers, ufbxt_hash_marker_imp);
	ufbxt_hash_list(h, v->lod_groups, ufbxt_hash_lod_group_imp);
	ufbxt_hash_list(h, v->skin_deformers, ufbxt_hash_skin_deformer_imp);
	ufbxt_hash_list(h, v->skin_clusters, ufbxt_hash_skin_cluster_imp);
	ufbxt_hash_list(h, v->blend_deformers, ufbxt_hash_blend_deformer_imp);
	ufbxt_hash_list(h, v->blend_channels, ufbxt_hash_blend_channel_imp);
	ufbxt_hash_list(h, v->blend_shapes, ufbxt_hash_blend_shape_imp);
	ufbxt_hash_list(h, v->cache_deformers, ufbxt_hash_cache_deformer_imp);
	ufbxt_hash_list(h, v->cache_files, ufbxt_hash_cache_file_imp);
	ufbxt_hash_list(h, v->materials, ufbxt_hash_material_imp);
	ufbxt_hash_list(h, v->textures, ufbxt_hash_texture_imp);
	ufbxt_hash_list(h, v->videos, ufbxt_hash_video_imp);
	ufbxt_hash_list(h, v->shaders, ufbxt_hash_shader_imp);
	ufbxt_hash_list(h, v->shader_bindings, ufbxt_hash_shader_binding_imp);
	ufbxt_hash_list(h, v->anim_stacks, ufbxt_hash_anim_stack_imp);
	ufbxt_hash_list(h, v->anim_layers, ufbxt_hash_anim_layer_imp);
	ufbxt_hash_list(h, v->anim_values, ufbxt_hash_anim_value_imp);
	ufbxt_hash_list(h, v->anim_curves, ufbxt_hash_anim_curve_imp);
	ufbxt_hash_list(h, v->display_layers, ufbxt_hash_display_layer_imp);
	ufbxt_hash_list(h, v->selection_sets, ufbxt_hash_selection_set_imp);
	ufbxt_hash_list(h, v->selection_nodes, ufbxt_hash_selection_node_imp);
	ufbxt_hash_list(h, v->characters, ufbxt_hash_character_imp);
	ufbxt_hash_list(h, v->constraints, ufbxt_hash_constraint_imp);
	ufbxt_hash_list(h, v->audio_layers, ufbxt_hash_audio_layer_imp);
	ufbxt_hash_list(h, v->audio_clips, ufbxt_hash_audio_clip_imp);
	ufbxt_hash_list(h, v->poses, ufbxt_hash_pose_imp);
	ufbxt_hash_list(h, v->metadata_objects, ufbxt_hash_metadata_object_imp);

	ufbxt_hash_list_ptr(h, v->texture_files, ufbxt_hash_texture_file_imp);

	ufbxt_hash_list(h, v->connections_src, ufbxt_hash_connection_imp);
	ufbxt_hash_list(h, v->connections_dst, ufbxt_hash_connection_imp);
	ufbxt_hash_list_ptr(h, v->elements_by_name, ufbxt_hash_name_element_imp);

	if (v->dom_root) ufbxt_hash_dom_node(h, v->dom_root);
}

ufbxt_noinline static uint64_t ufbxt_hash_scene(const ufbx_scene *v, ufbxh_FILE *dump_file)
{
	ufbxt_hash h;
	ufbxt_hash_init(&h, dump_file);
	ufbxt_hash_scene_imp(&h, v);
	return ufbxt_hash_finish(&h);
}

#endif
