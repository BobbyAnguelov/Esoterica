#ifndef UFBXT_TESTING_UTILS_INCLUDED
#define UFBXT_TESTING_UTILS_INCLUDED

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#include "testing_basics.h"

#ifndef ufbxt_soft_assert
#define ufbxt_soft_assert(cond) ufbxt_assert(cond)
#endif

// -- Vector helpers

static ufbx_real ufbxt_dot2(ufbx_vec2 a, ufbx_vec2 b)
{
	return a.x*b.x + a.y*b.y;
}

static ufbx_real ufbxt_dot3(ufbx_vec3 a, ufbx_vec3 b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

static ufbx_real ufbxt_length2(ufbx_vec2 a)
{
	return (ufbx_real)sqrt(ufbxt_dot2(a, a));
}

static ufbx_real ufbxt_length3(ufbx_vec3 a)
{
	return (ufbx_real)sqrt(ufbxt_dot3(a, a));
}

static ufbx_vec2 ufbxt_add2(ufbx_vec2 a, ufbx_vec2 b)
{
	ufbx_vec2 v;
	v.x = a.x + b.x;
	v.y = a.y + b.y;
	return v;
}

static ufbx_vec3 ufbxt_add3(ufbx_vec3 a, ufbx_vec3 b)
{
	ufbx_vec3 v;
	v.x = a.x + b.x;
	v.y = a.y + b.y;
	v.z = a.z + b.z;
	return v;
}

static ufbx_vec2 ufbxt_sub2(ufbx_vec2 a, ufbx_vec2 b)
{
	ufbx_vec2 v;
	v.x = a.x - b.x;
	v.y = a.y - b.y;
	return v;
}

static ufbx_vec3 ufbxt_sub3(ufbx_vec3 a, ufbx_vec3 b)
{
	ufbx_vec3 v;
	v.x = a.x - b.x;
	v.y = a.y - b.y;
	v.z = a.z - b.z;
	return v;
}

static ufbx_vec2 ufbxt_mul2(ufbx_vec2 a, ufbx_real b)
{
	ufbx_vec2 v;
	v.x = a.x * b;
	v.y = a.y * b;
	return v;
}

static ufbx_vec3 ufbxt_mul3(ufbx_vec3 a, ufbx_real b)
{
	ufbx_vec3 v;
	v.x = a.x * b;
	v.y = a.y * b;
	v.z = a.z * b;
	return v;
}

static ufbx_vec3 ufbxt_cross3(ufbx_vec3 a, ufbx_vec3 b)
{
	ufbx_vec3 v = { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
	return v;
}

static bool ufbxt_eq3(ufbx_vec3 a, ufbx_vec3 b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

static ufbx_vec3 ufbxt_normalize(ufbx_vec3 a) {
	ufbx_real len = (ufbx_real)sqrt(ufbxt_dot3(a, a));
	if (len != 0.0) {
		return ufbxt_mul3(a, (ufbx_real)1.0 / len);
	} else {
		ufbx_vec3 zero = { (ufbx_real)0 };
		return zero;
	}
}

static ufbx_real ufbxt_min(ufbx_real a, ufbx_real b) { return a < b ? a : b; }
static ufbx_real ufbxt_max(ufbx_real a, ufbx_real b) { return a < b ? b : a; }
static ufbx_real ufbxt_clamp(ufbx_real v, ufbx_real min_v, ufbx_real max_v) { return ufbxt_min(ufbxt_max(v, min_v), max_v); }


// -- obj load and diff

typedef struct {
	const char **groups;
	const char *original_group;
	size_t num_groups;

	size_t num_faces;
	size_t num_indices;

	ufbx_face *faces;

	ufbx_vertex_vec3 vertex_position;
	ufbx_vertex_vec3 vertex_normal;
	ufbx_vertex_vec2 vertex_uv;

} ufbxt_obj_mesh;

typedef enum {
	UFBXT_OBJ_EXPORTER_UNKNOWN,
	UFBXT_OBJ_EXPORTER_BLENDER,
} ufbxt_obj_exporter;

typedef struct {

	ufbxt_obj_mesh *meshes;
	size_t num_meshes;

	bool bad_normals;
	bool bad_order;
	bool bad_uvs;
	bool bad_faces;
	bool has_empty;
	bool missing_uvs;
	bool line_faces;
	bool point_faces;
	bool no_subdivision;
	bool root_groups_at_bone;
	bool remove_namespaces;
	bool match_by_order;
	bool negate_xz;
	bool bind_pose;
	bool ignore_unskinned;
	bool transform_skin;
	bool no_bake;
	ufbx_real tolerance;
	ufbx_real uv_tolerance;
	uint32_t allow_missing;
	int32_t animation_frame;

	bool normalize_units;
	bool ignore_duplicates;

	ufbxt_obj_exporter exporter;

	double position_scale;
	double position_scale_float;
	double position_scale_bake;
	double position_scale_compensate;

	double fbx_position_scale;
	ufbx_quat fbx_rotation;

	char animation_name[128];
	char model_name[128];

} ufbxt_obj_file;

typedef struct {

	const char *name_end_chars;

} ufbxt_load_obj_opts;

static int ufbxt_cmp_obj_mesh(const void *va, const void *vb)
{
	const ufbxt_obj_mesh *a = (const ufbxt_obj_mesh*)va, *b = (const ufbxt_obj_mesh*)vb;
	if (a->num_groups < b->num_groups) return -1;
	if (a->num_groups > b->num_groups) return +1;
	return 0;
}

static uint16_t ufbxt_read_u16(const void *ptr) {
	const char *p = (const char*)ptr;
	return (uint16_t)(
		(unsigned)(uint8_t)p[0] << 0u |
		(unsigned)(uint8_t)p[1] << 8u );
}

static uint32_t ufbxt_read_u32(const void *ptr) {
	const char *p = (const char*)ptr;
	return (uint32_t)(
		(unsigned)(uint8_t)p[0] <<  0u |
		(unsigned)(uint8_t)p[1] <<  8u |
		(unsigned)(uint8_t)p[2] << 16u |
		(unsigned)(uint8_t)p[3] << 24u );
}

static ufbxt_noinline void *ufbxt_decompress_gzip(const void *gz_data, size_t gz_size, size_t *p_size)
{
	const uint8_t *gz = (const uint8_t*)gz_data;
	if (gz_size < 10) return NULL;
	if (gz[0] != 0x1f || gz[1] != 0x8b || gz[2] != 0x08) return NULL;
	uint8_t flags = gz[3];

	size_t offset = 10;

	if (flags & 0x4) {
		// FEXTRA
		if (gz_size - offset < 2) return NULL;
		uint16_t xlen = ufbxt_read_u16(gz + offset);
		offset += 2;
		if (gz_size - offset < xlen) return NULL;
		offset += xlen;
	}

	if (flags & 0x8) {
		// FNAME
		const uint8_t *end = (const uint8_t*)memchr(gz + offset, 0, gz_size - offset);
		if (!end) return NULL;
		offset = (size_t)(end - gz) + 1;
	}

	if (flags & 0x10) {
		// FCOMMENT
		const uint8_t *end = (const uint8_t*)memchr(gz + offset, 0, gz_size - offset);
		if (!end) return NULL;
		offset = (size_t)(end - gz) + 1;
	}

	if (flags & 0x2) {
		// FHCRC
		if (gz_size - offset < 2) return NULL;
		offset += 2;
	}

	uint32_t isize = ufbxt_read_u32(gz + gz_size - 4);

	void *dst = malloc(isize + 1);
	if (!dst) return NULL;

	ufbx_inflate_retain retain;
	retain.initialized = false;

	ufbx_inflate_input input = { 0 };

	input.data = gz + offset;
	input.data_size = gz_size - offset;

	input.total_size = input.data_size;

	input.no_header = true;
	input.no_checksum = true;

	ptrdiff_t result = ufbx_inflate(dst, isize, &input, &retain);
	if (result != isize) {
		free(dst);
		return NULL;
	}

	((char*)dst)[isize] = '\0';

	*p_size = isize;
	return dst;
}

#define UFBXT_MAX_PARSED_INT 
#define UFBXT_MAX_EXPONENT 325

static double ufbxt_pow10_table[UFBXT_MAX_EXPONENT*2 + 1];
static volatile bool ufbxt_parsing_init_done = false;

static void ufbxt_init_parsing()
{
	if (ufbxt_parsing_init_done) return;

	for (int32_t i = 0; i <= UFBXT_MAX_EXPONENT*2; i++) {
		ufbxt_pow10_table[i] = pow(10.0, (double)(i - UFBXT_MAX_EXPONENT));
	}

	ufbxt_parsing_init_done = true;
}

static size_t ufbxt_parse_ints(int32_t *p_result, size_t max_count, const char **str)
{
	const char *c = *str;

	size_t num;
	for (num = 0; num < max_count; num++) {
		if (*c == '/') {
			c++;
			p_result[num] = 0;
			continue;
		}

		int32_t sign = 1;
		if (*c == '+') {
			c++;
		} else if (*c == '-') {
			sign = -1;
			c++;
		}

		if (!(*c >= '0' && *c <= '9')) return num;

		const uint32_t max_value = UINT64_C(100000000);
		uint32_t value = 0;

		do {
			if (value < max_value) {
				value = value * 10 + (uint64_t)(*c++ - '0');
			} else {
				return num;
			}
		} while (*c >= '0' && *c <= '9');

		p_result[num] = (int32_t)value * sign;

		if (*c != '/') return num;
		c++;
	}

	*str = c;
	return num;
}

static bool ufbxt_parse_reals(ufbx_real *p_result, size_t count, const char *str)
{
	const char *c = str;

	for (size_t i = 0; i < count; i++) {
		// Skip separator
		while (*c == ' ') c++;

		const uint64_t max_value = UINT64_C(1000000000000000000);
		uint64_t value = 0;

		double sign = 1.0;
		if (*c == '-') {
			sign = -1.0;
			c++;
		} else if (*c == '+') {
			c++;
		}

		int32_t exponent = 0;

		if (!(*c >= '0' && *c <= '9')) {
			if (!strncmp(c, "nan(ind)", 8)) {
				*p_result = NAN;
				c += 8;
				continue;
			}

			return false;
		}

		do {
			if (value < max_value) {
				value = value * 10 + (uint64_t)(*c - '0');
			} else {
				exponent--;
			}
			c++;
		} while (*c >= '0' && *c <= '9');

		if (*c == '.') {
			c++;

			while (*c >= '0' && *c <= '9') {
				if (value < max_value) {
					value = value * 10 + (uint64_t)(*c - '0');
					exponent--;
				}
				c++;
			}

			if ((*c | 0x20) == 'e') {
				c++;
				int32_t expsign = 1;
				if (*c == '+') {
					c++;
				} else if (*c == '-') {
					expsign = -1;
					c++;
				}

				int32_t exp = 0;
				while (*c >= '0' && *c <= '9') {
					if (exp < 4096) {
						exp = exp * 10 + (int32_t)(*c - '0');
					}
					c++;
				}

				exponent += expsign * exp;
			}
		}

		if (exponent >= -UFBXT_MAX_EXPONENT && exponent <= UFBXT_MAX_EXPONENT) {
			p_result[i] = (ufbx_real)(sign * ufbxt_pow10_table[exponent + UFBXT_MAX_EXPONENT] * (double)value);
		} else if (exponent < 0) {
			p_result[i] = 0.0f;
		} else {
			p_result[i] = (ufbx_real)INFINITY;
		}
	}

	return true;
}

static char *ufbxt_find_newline(char *src)
{
	for (;;) {
		char c = *src;
		if (c == '\0') return NULL;
		if (c == '\r' || c == '\n') return src;
		src++;
	}
}

static bool ufbxt_is_space(char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static ufbxt_noinline ufbxt_obj_file *ufbxt_load_obj(void *obj_data, size_t obj_size, const ufbxt_load_obj_opts *opts)
{
	ufbxt_load_obj_opts zero_opts;
	if (!opts) {
		memset(&zero_opts, 0, sizeof(zero_opts));
		opts = &zero_opts;
	}

	ufbxt_init_parsing();

	size_t num_positions = 0;
	size_t num_normals = 0;
	size_t num_uvs = 0;
	size_t num_faces = 0;
	size_t num_meshes = 0;
	size_t num_indices = 0;
	size_t num_groups = 0;
	size_t total_name_length = 0;

	bool merge_groups = false;
	bool allow_default = false;

	char *line = (char*)obj_data;
	for (;;) {
		char *end = ufbxt_find_newline(line);
		char prev = '\0';
		if (end) {
			prev = *end;
			*end = '\0';
		}

		if (!strncmp(line, "v ", 2)) num_positions++;
		else if (!strncmp(line, "vt ", 3)) num_uvs++;
		else if (!strncmp(line, "vn ", 3)) num_normals++;
		else if (!strncmp(line, "f ", 2)) {
			num_faces++;
			bool prev_space = false;
			for (char *c = line; *c; c++) {
				bool space = *c == ' ' || *c == '\t';
				if (space && !prev_space) num_indices++;
				prev_space = space;
			}
		}
		else if (!strncmp(line, "g default", 7) && !allow_default) { /* ignore default group */ }
		else if ((!strncmp(line, "g ", 2) && !merge_groups) || !strncmp(line, "o ", 2)) {
			bool prev_space = false;
			num_groups++;
			for (char *c = line; *c; c++) {
				bool space = *c == ' ' || *c == '\t';
				if (space && !prev_space) num_groups++;
				if (!space) total_name_length++;
				total_name_length++;
				prev_space = space;
			}
			total_name_length++;
			num_meshes++;
		} else if (strstr(line, "ufbx:merge_groups")) {
			merge_groups = true;
		} else if (strstr(line, "ufbx:allow_default")) {
			allow_default = true;
		}

		if (end) {
			*end = prev;
			line = end + 1;
		} else {
			break;
		}
	}

	bool implicit_mesh = false;
	if (num_meshes == 0) {
		num_meshes = 1;
		implicit_mesh = true;
	}

	total_name_length += num_groups;

	size_t alloc_size = 0;
	alloc_size += sizeof(ufbxt_obj_file);
	alloc_size += num_meshes * sizeof(ufbxt_obj_mesh);
	alloc_size += num_groups * sizeof(const char*);
	alloc_size += num_positions * sizeof(ufbx_vec3);
	alloc_size += num_normals * sizeof(ufbx_vec3);
	alloc_size += num_uvs * sizeof(ufbx_vec2);
	alloc_size += num_faces * sizeof(ufbx_face);
	alloc_size += num_indices * 3 * sizeof(int32_t);
	alloc_size += total_name_length * sizeof(char);

	void *data = malloc(alloc_size);
	ufbxt_assert(data);

	ufbxt_obj_file *obj = (ufbxt_obj_file*)data;
	const char **group_ptrs = (const char**)(obj + 1);
	ufbxt_obj_mesh *meshes = (ufbxt_obj_mesh*)(group_ptrs + num_groups);
	ufbx_vec3 *positions = (ufbx_vec3*)(meshes + num_meshes);
	ufbx_vec3 *normals = (ufbx_vec3*)(positions + num_positions);
	ufbx_vec2 *uvs = (ufbx_vec2*)(normals + num_normals);
	ufbx_face *faces = (ufbx_face*)(uvs + num_uvs);
	int32_t *position_indices = (int32_t*)(faces + num_faces);
	int32_t *normal_indices = (int32_t*)(position_indices + num_indices);
	int32_t *uv_indices = (int32_t*)(normal_indices + num_indices);
	char *name_data = (char*)(uv_indices + num_indices);
	void *data_end = name_data + total_name_length;
	ufbxt_assert((char*)data_end - (char*)data == alloc_size);

	memset(obj, 0, sizeof(ufbxt_obj_file));

	ufbx_vec3 *dp = positions;
	ufbx_vec3 *dn = normals;
	ufbx_vec2 *du = uvs;
	ufbxt_obj_mesh *mesh = NULL;

	int32_t *dpi = position_indices;
	int32_t *dni = normal_indices;
	int32_t *dui = uv_indices;

	ufbx_face *df = faces;

	obj->meshes = meshes;
	obj->num_meshes = num_meshes;
	obj->tolerance = (ufbx_real)0.001;
	obj->uv_tolerance = (ufbx_real)0.001;
	obj->allow_missing = 0;
	obj->normalize_units = false;
	obj->animation_frame = -1;
	obj->exporter = UFBXT_OBJ_EXPORTER_UNKNOWN;
	obj->position_scale = 1.0;
	obj->position_scale_float = 1.0;
	obj->position_scale_bake = 1.0;
	obj->position_scale_compensate = 1.0;
	obj->fbx_position_scale = 1.0;
	obj->fbx_rotation = ufbx_identity_quat;

	if (implicit_mesh) {
		mesh = meshes;
		memset(mesh, 0, sizeof(ufbxt_obj_mesh));

		mesh->faces = df;
		mesh->vertex_position.values.data = positions;
		mesh->vertex_normal.values.data = normals;
		mesh->vertex_uv.values.data = uvs;
		mesh->vertex_position.indices.data = (uint32_t*)dpi;
		mesh->vertex_normal.indices.data = (uint32_t*)dni;
		mesh->vertex_uv.indices.data = (uint32_t*)dui;
	}

	line = (char*)obj_data;
	for (;;) {
		char *line_end = ufbxt_find_newline(line);
		char prev = '\0';
		if (line_end) {
			prev = *line_end;
			*line_end = '\0';
		}

		if (!strncmp(line, "v ", 2)) {
			line += 2;
			ufbxt_assert(ufbxt_parse_reals(dp->v, 3, line));
			dp++;
		} else if (!strncmp(line, "vt ", 3)) {
			line += 3;
			ufbxt_assert(ufbxt_parse_reals(du->v, 2, line));
			du++;
		} else if (!strncmp(line, "vn ", 3)) {
			line += 3;
			ufbxt_assert(ufbxt_parse_reals(dn->v, 3, line));
			dn++;
		} else if (!strncmp(line, "f ", 2)) {
			ufbxt_assert(mesh);

			df->index_begin = (uint32_t)mesh->num_indices;
			df->num_indices = 0;

			char *begin = line + 2;
			do {
				char *end = strchr(begin, ' ');
				if (end) *end++ = '\0';

				while (ufbxt_is_space(*begin)) {
					begin++;
				}
				if (*begin == '\0') {
					begin = end;
					continue;
				}

				int32_t indices[3] = { 0, 0, 0 };
				ufbxt_parse_ints(indices, 3, (const char**)&begin);

				mesh->vertex_position.indices.count++;
				mesh->vertex_normal.indices.count++;
				mesh->vertex_uv.indices.count++;

				mesh->vertex_position.values.count = (size_t)(dp - positions);
				mesh->vertex_normal.values.count = (size_t)(dn - normals);
				mesh->vertex_uv.values.count = (size_t)(du - uvs);
				if (indices[0] > 0) mesh->vertex_position.exists = true;
				if (indices[1] > 0) mesh->vertex_uv.exists = true;
				if (indices[2] > 0) mesh->vertex_normal.exists = true;

				*dpi++ = indices[0] - 1;
				*dni++ = indices[2] - 1;
				*dui++ = indices[1] - 1;
				mesh->num_indices++;
				df->num_indices++;

				begin = end;
			} while (begin);

			mesh->num_faces++;
			df++;
		} else if (!strncmp(line, "g default", 7) && !allow_default) {
			/* ignore default group */
		} else if ((!strncmp(line, "g ", 2) && !merge_groups) || !strncmp(line, "o ", 2)) {
			mesh = mesh ? mesh + 1 : meshes;
			memset(mesh, 0, sizeof(ufbxt_obj_mesh));

			size_t groups_len = strlen(line + 2);
			memcpy(name_data, line + 2, groups_len + 1);
			mesh->original_group = name_data;
			name_data += groups_len + 1;

			mesh->groups = group_ptrs;

			const char *c = line + 2;
			for (;;) {
				while (*c == ' ' || *c == '\t' || *c == '\r') {
					c++;
				}

				if (*c == '\n' || *c == '\0') break;

				const char *group_begin = c;
				char *group_copy = name_data;
				char *dst = group_copy;

				while (*c != ' ' && *c != '\t' && *c != '\r' && *c != '\n' && *c != '\0') {
					if (!strncmp(c, "FBXASC", 6)) {
						c += 6;
						char num[4] = { 0 };
						if (*c != '\0') num[0] = *c++;
						if (*c != '\0') num[1] = *c++;
						if (*c != '\0') num[2] = *c++;
						*dst++ = (char)atoi(num);
					} else {
						*dst++ = *c++;
					}
				}
				*dst++ = '\0';
				name_data = dst;

				*group_ptrs++ = group_copy;
			}

			mesh->num_groups = group_ptrs - mesh->groups;

			mesh->faces = df;
			mesh->vertex_position.values.data = positions;
			mesh->vertex_normal.values.data = normals;
			mesh->vertex_uv.values.data = uvs;
			mesh->vertex_position.indices.data = (uint32_t*)dpi;
			mesh->vertex_normal.indices.data = (uint32_t*)dni;
			mesh->vertex_uv.indices.data = (uint32_t*)dui;
		}

		if (line[0] == '#') {
			line += 1;
			char *end = line_end;
			while (line < end && (line[0] == ' ' || line[0] == '\t')) {
				line++;
			}
			while (end > line && (end[-1] == ' ' || end[-1] == '\t')) {
				*--end = '\0';
			}
			if (!strcmp(line, "ufbx:bad_normals")) {
				obj->bad_normals = true;
			}
			if (!strcmp(line, "ufbx:bad_order")) {
				obj->bad_order = true;
			}
			if (!strcmp(line, "ufbx:bad_uvs")) {
				obj->bad_uvs = true;
			}
			if (!strcmp(line, "ufbx:bad_faces")) {
				obj->bad_faces = true;
			}
			if (!strcmp(line, "ufbx:has_empty")) {
				obj->has_empty = true;
			}
			if (!strcmp(line, "ufbx:missing_uvs")) {
				obj->missing_uvs = true;
			}
			if (!strcmp(line, "ufbx:line_faces")) {
				obj->line_faces = true;
			}
			if (!strcmp(line, "ufbx:point_faces")) {
				obj->point_faces = true;
			}
			if (!strcmp(line, "ufbx:no_subdivision")) {
				obj->no_subdivision = true;
			}
			if (!strcmp(line, "ufbx:ignore_duplicates")) {
				obj->ignore_duplicates = true;
			}
			if (!strcmp(line, "ufbx:root_groups_at_bone")) {
				obj->root_groups_at_bone = true;
			}
			if (!strcmp(line, "ufbx:remove_namespaces")) {
				obj->remove_namespaces = true;
			}
			if (!strcmp(line, "ufbx:match_by_order")) {
				obj->match_by_order = true;
			}
			if (!strcmp(line, "ufbx:negate_xz")) {
				obj->negate_xz = true;
			}
			if (!strcmp(line, "ufbx:bind_pose")) {
				obj->bind_pose = true;
			}
			if (!strcmp(line, "ufbx:ignore_unskinned")) {
				obj->ignore_unskinned = true;
			}
			if (!strcmp(line, "ufbx:transform_skin")) {
				obj->transform_skin = true;
			}
			if (!strcmp(line, "ufbx:no_bake")) {
				obj->no_bake = true;
			}
			if (!strcmp(line, "www.blender.org")) {
				obj->exporter = UFBXT_OBJ_EXPORTER_BLENDER;
			}
			if (!strncmp(line, "ufbx:animation=", 15)) {
				line += 15;
				size_t len = strcspn(line, "\r\n");
				ufbxt_assert(len + 1 < sizeof(obj->animation_name));
				memcpy(obj->animation_name, line, len);
			}
			if (!strncmp(line, "ufbx:model=", 11)) {
				line += 11;
				size_t len = strcspn(line, "\r\n");
				ufbxt_assert(len + 1 < sizeof(obj->model_name));
				memcpy(obj->model_name, line, len);
			}
			double tolerance = 0.0;
			if (sscanf(line, "ufbx:tolerance=%lf", &tolerance) == 1) {
				obj->tolerance = (ufbx_real)tolerance;
			}
			double uv_tolerance = 0.0;
			if (sscanf(line, "ufbx:uv_tolerance=%lf", &uv_tolerance) == 1) {
				obj->uv_tolerance = (ufbx_real)uv_tolerance;
			}
			int allow_missing = 0;
			if (sscanf(line, "ufbx:allow_missing=%d", &allow_missing) == 1) {
				obj->allow_missing = (uint32_t)allow_missing;
			}
			double position_scale = 0.0;
			if (sscanf(line, "ufbx:position_scale=%lf", &position_scale) == 1) {
				obj->position_scale = position_scale;
			}
			double position_scale_float = 0.0;
			if (sscanf(line, "ufbx:position_scale_float=%lf", &position_scale_float) == 1) {
				obj->position_scale_float = position_scale_float;
			}
			double position_scale_bake = 0.0;
			if (sscanf(line, "ufbx:position_scale_bake=%lf", &position_scale_bake) == 1) {
				obj->position_scale_bake = position_scale_bake;
			}
			double position_scale_compensate = 0.0;
			if (sscanf(line, "ufbx:position_scale_compensate=%lf", &position_scale_compensate) == 1) {
				obj->position_scale_compensate = position_scale_compensate;
			}
			int frame = 0;
			if (sscanf(line, "ufbx:frame=%d", &frame) == 1) {
				obj->animation_frame = (int32_t)frame;
			}
		}

		if (line_end) {
			*line_end = prev;
			line = line_end + 1;
		} else {
			break;
		}
	}

	qsort(obj->meshes, obj->num_meshes, sizeof(ufbxt_obj_mesh), ufbxt_cmp_obj_mesh);

	return obj;
}

static ufbxt_noinline void ufbxt_debug_dump_obj_mesh(const char *file, ufbx_node *node, ufbx_mesh *mesh)
{
	FILE *f = fopen(file, "wb");
	ufbxt_assert(f);

	fprintf(f, "s 1\n");

	for (size_t i = 0; i < mesh->vertex_position.values.count; i++) {
		ufbx_vec3 v = mesh->vertex_position.values.data[i];
		v = ufbx_transform_position(&node->geometry_to_world, v);
		fprintf(f, "v %f %f %f\n", v.x, v.y, v.z);
	}
	for (size_t i = 0; i < mesh->vertex_uv.values.count; i++) {
		ufbx_vec2 v = mesh->vertex_uv.values.data[i];
		fprintf(f, "vt %f %f\n", v.x, v.y);
	}

	ufbx_matrix mat = ufbx_matrix_for_normals(&node->geometry_to_world);
	for (size_t i = 0; i < mesh->vertex_normal.values.count; i++) {
		ufbx_vec3 v = mesh->vertex_normal.values.data[i];
		v = ufbx_transform_direction(&mat, v);
		fprintf(f, "vn %f %f %f\n", v.x, v.y, v.z);
	}


	for (size_t fi = 0; fi < mesh->num_faces; fi++) {
		ufbx_face face = mesh->faces.data[fi];
		fprintf(f, "f");
		for (size_t ci = 0; ci < face.num_indices; ci++) {
			int32_t vi = mesh->vertex_position.indices.data[face.index_begin + ci];
			int32_t ti = mesh->vertex_uv.indices.data[face.index_begin + ci];
			int32_t ni = mesh->vertex_normal.indices.data[face.index_begin + ci];
			fprintf(f, " %d/%d/%d", vi + 1, ti + 1, ni + 1);
		}
		fprintf(f, "\n");
	}

	fclose(f);
}

static ufbxt_noinline void ufbxt_debug_dump_obj_scene(const char *file, ufbx_scene *scene)
{
	FILE *f = fopen(file, "wb");
	ufbxt_assert(f);

	for (size_t mi = 0; mi < scene->meshes.count; mi++) {
		ufbx_mesh *mesh = scene->meshes.data[mi];
		for (size_t ni = 0; ni < mesh->instances.count; ni++) {
			ufbx_node *node = mesh->instances.data[ni];

			for (size_t i = 0; i < mesh->vertex_position.values.count; i++) {
				ufbx_vec3 v = mesh->skinned_position.values.data[i];
				if (mesh->skinned_is_local) {
					v = ufbx_transform_position(&node->geometry_to_world, v);
				}
				fprintf(f, "v %f %f %f\n", v.x, v.y, v.z);
			}

			for (size_t i = 0; i < mesh->vertex_uv.values.count; i++) {
				ufbx_vec2 v = mesh->vertex_uv.values.data[i];
				fprintf(f, "vt %f %f\n", v.x, v.y);
			}

			ufbx_matrix mat = ufbx_matrix_for_normals(&node->geometry_to_world);
			for (size_t i = 0; i < mesh->skinned_normal.values.count; i++) {
				ufbx_vec3 v = mesh->skinned_normal.values.data[i];
				if (mesh->skinned_is_local) {
					v = ufbx_transform_direction(&mat, v);
				}
				fprintf(f, "vn %f %f %f\n", v.x, v.y, v.z);
			}

			fprintf(f, "\n");
		}
	}

	int32_t v_off = 0, t_off = 0, n_off = 0;
	for (size_t mi = 0; mi < scene->meshes.count; mi++) {
		ufbx_mesh *mesh = scene->meshes.data[mi];
		for (size_t ni = 0; ni < mesh->instances.count; ni++) {
			ufbx_node *node = mesh->instances.data[ni];
			fprintf(f, "g %s\n", node->name.data);

			for (size_t fi = 0; fi < mesh->num_faces; fi++) {
				ufbx_face face = mesh->faces.data[fi];
				fprintf(f, "f");
				for (size_t ci = 0; ci < face.num_indices; ci++) {
					int32_t vi = v_off + mesh->skinned_position.indices.data[face.index_begin + ci];
					int32_t ni = n_off + mesh->skinned_normal.indices.data[face.index_begin + ci];
					if (mesh->vertex_uv.exists) {
						int32_t ti = t_off + mesh->vertex_uv.indices.data[face.index_begin + ci];
						fprintf(f, " %d/%d/%d", vi + 1, ti + 1, ni + 1);
					} else {
						fprintf(f, " %d//%d", vi + 1, ni + 1);
					}
				}
				fprintf(f, "\n");
			}

			fprintf(f, "\n");

			v_off += (int32_t)mesh->skinned_position.values.count;
			t_off += (int32_t)mesh->vertex_uv.values.count;
			n_off += (int32_t)mesh->skinned_normal.values.count;
		}
	}

	fclose(f);
}

typedef struct {
	size_t num;
	double sum;
	double max;
} ufbxt_diff_error;

static void ufbxt_assert_close_real(ufbxt_diff_error *p_err, ufbx_real a, ufbx_real b)
{
	ufbx_real err = (ufbx_real)fabs(a - b);
	ufbxt_soft_assert(err < 0.001);
	p_err->num++;
	p_err->sum += err;
	if (err > p_err->max) p_err->max = err;
}

static void ufbxt_assert_close_double(ufbxt_diff_error *p_err, double a, double b)
{
	double err = (double)fabs(a - b);
	ufbxt_soft_assert(err < 0.001);
	p_err->num++;
	p_err->sum += err;
	if (err > p_err->max) p_err->max = err;
}

static void ufbxt_assert_close_vec2(ufbxt_diff_error *p_err, ufbx_vec2 a, ufbx_vec2 b)
{
	ufbxt_assert_close_real(p_err, a.x, b.x);
	ufbxt_assert_close_real(p_err, a.y, b.y);
}

static void ufbxt_assert_close_vec3(ufbxt_diff_error *p_err, ufbx_vec3 a, ufbx_vec3 b)
{
	ufbxt_assert_close_real(p_err, a.x, b.x);
	ufbxt_assert_close_real(p_err, a.y, b.y);
	ufbxt_assert_close_real(p_err, a.z, b.z);
}

static void ufbxt_assert_close_vec3_xyz(ufbxt_diff_error *p_err, ufbx_vec3 a, ufbx_real x, ufbx_real y, ufbx_real z)
{
	ufbxt_assert_close_real(p_err, a.x, x);
	ufbxt_assert_close_real(p_err, a.y, y);
	ufbxt_assert_close_real(p_err, a.z, z);
}

static void ufbxt_assert_close_vec4(ufbxt_diff_error *p_err, ufbx_vec4 a, ufbx_vec4 b)
{
	ufbxt_assert_close_real(p_err, a.x, b.x);
	ufbxt_assert_close_real(p_err, a.y, b.y);
	ufbxt_assert_close_real(p_err, a.z, b.z);
	ufbxt_assert_close_real(p_err, a.w, b.w);
}

static void ufbxt_assert_close_quat(ufbxt_diff_error *p_err, ufbx_quat a, ufbx_quat b)
{
	ufbxt_assert_close_real(p_err, a.x, b.x);
	ufbxt_assert_close_real(p_err, a.y, b.y);
	ufbxt_assert_close_real(p_err, a.z, b.z);
	ufbxt_assert_close_real(p_err, a.w, b.w);
}

static void ufbxt_assert_close_matrix(ufbxt_diff_error *p_err, ufbx_matrix a, ufbx_matrix b)
{
	ufbxt_assert_close_vec3(p_err, a.cols[0], b.cols[0]);
	ufbxt_assert_close_vec3(p_err, a.cols[1], b.cols[1]);
	ufbxt_assert_close_vec3(p_err, a.cols[2], b.cols[2]);
	ufbxt_assert_close_vec3(p_err, a.cols[3], b.cols[3]);
}

static void ufbxt_assert_close_real_threshold(ufbxt_diff_error *p_err, ufbx_real a, ufbx_real b, ufbx_real threshold)
{
	ufbx_real err = (ufbx_real)fabs(a - b);
	ufbxt_assert(err < threshold);
	p_err->num++;
	p_err->sum += err;
	if (err > p_err->max) p_err->max = err;
}

static void ufbxt_assert_close_vec2_threshold(ufbxt_diff_error *p_err, ufbx_vec2 a, ufbx_vec2 b, ufbx_real threshold)
{
	ufbxt_assert_close_real_threshold(p_err, a.x, b.x, threshold);
	ufbxt_assert_close_real_threshold(p_err, a.y, b.y, threshold);
}

static void ufbxt_assert_close_vec3_threshold(ufbxt_diff_error *p_err, ufbx_vec3 a, ufbx_vec3 b, ufbx_real threshold)
{
	ufbxt_assert_close_real_threshold(p_err, a.x, b.x, threshold);
	ufbxt_assert_close_real_threshold(p_err, a.y, b.y, threshold);
	ufbxt_assert_close_real_threshold(p_err, a.z, b.z, threshold);
}

static void ufbxt_assert_close_vec4_threshold(ufbxt_diff_error *p_err, ufbx_vec4 a, ufbx_vec4 b, ufbx_real threshold)
{
	ufbxt_assert_close_real_threshold(p_err, a.x, b.x, threshold);
	ufbxt_assert_close_real_threshold(p_err, a.y, b.y, threshold);
	ufbxt_assert_close_real_threshold(p_err, a.z, b.z, threshold);
	ufbxt_assert_close_real_threshold(p_err, a.w, b.w, threshold);
}

static void ufbxt_assert_close_quat_threshold(ufbxt_diff_error *p_err, ufbx_quat a, ufbx_quat b, ufbx_real threshold)
{
	ufbxt_assert_close_real_threshold(p_err, a.x, b.x, threshold);
	ufbxt_assert_close_real_threshold(p_err, a.y, b.y, threshold);
	ufbxt_assert_close_real_threshold(p_err, a.z, b.z, threshold);
	ufbxt_assert_close_real_threshold(p_err, a.w, b.w, threshold);
}

static ufbxt_noinline void ufbxt_check_source_vertices(ufbx_mesh *mesh, ufbx_mesh *src_mesh, ufbxt_diff_error *p_err, ufbx_real threshold)
{
	if (threshold == 0.0) threshold = 0.001;

	ufbx_subdivision_result *sub = mesh->subdivision_result;
	ufbxt_assert(sub);

	size_t num_vertices = mesh->num_vertices;
	ufbxt_assert(sub->source_vertex_ranges.count == num_vertices);
	for (size_t vi = 0; vi < num_vertices; vi++) {
		ufbx_subdivision_weight_range range = sub->source_vertex_ranges.data[vi];

		ufbx_vec3 sum = ufbx_zero_vec3;
		for (size_t i = 0; i < range.num_weights; i++) {
			ufbx_subdivision_weight weight = sub->source_vertex_weights.data[range.weight_begin + i];
			ufbx_vec3 v = src_mesh->vertices.data[weight.index];
			sum.x += v.x * weight.weight;
			sum.y += v.y * weight.weight;
			sum.z += v.z * weight.weight;
		}

		ufbx_vec3 ref = mesh->vertices.data[vi];
		ufbxt_assert_close_vec3_threshold(p_err, ref, sum, threshold);
		ref = ref;
	}
}

typedef struct {
	ufbx_vec3 pos;
	ufbx_vec3 normal;
	ufbx_vec2 uv;
} ufbxt_match_vertex;

static int ufbxt_cmp_sub_vertex(const void *va, const void *vb)
{
	const ufbxt_match_vertex *a = (const ufbxt_match_vertex*)va, *b = (const ufbxt_match_vertex*)vb;
	if (a->pos.x != b->pos.x) return a->pos.x < b->pos.x ? -1 : +1;
	if (a->pos.y != b->pos.y) return a->pos.y < b->pos.y ? -1 : +1;
	if (a->pos.z != b->pos.z) return a->pos.z < b->pos.z ? -1 : +1;
	if (a->normal.x != b->normal.x) return a->normal.x < b->normal.x ? -1 : +1;
	if (a->normal.y != b->normal.y) return a->normal.y < b->normal.y ? -1 : +1;
	if (a->normal.z != b->normal.z) return a->normal.z < b->normal.z ? -1 : +1;
	if (a->uv.x != b->uv.x) return a->uv.x < b->uv.x ? -1 : +1;
	if (a->uv.y != b->uv.y) return a->uv.y < b->uv.y ? -1 : +1;
	return 0;
}

static ufbxt_noinline bool ufbxt_face_has_duplicate_vertex(ufbx_mesh *mesh, ufbx_face face)
{
	for (size_t i = 0; i < face.num_indices; i++) {
		uint32_t ix = mesh->vertex_indices.data[face.index_begin + i];
		for (size_t j = 0; j < i; j++) {
			uint32_t jx = mesh->vertex_indices.data[face.index_begin + j];
			if (ix == jx) {
				return true;
			}
		}
	}
	return false;
}

static ufbxt_noinline void ufbxt_match_obj_mesh(ufbxt_obj_file *obj, ufbx_node *fbx_node, ufbx_mesh *fbx_mesh, ufbxt_obj_mesh *obj_mesh, ufbxt_diff_error *p_err, ufbx_real scale)
{
	ufbx_real tolerance = obj->tolerance;

	if (!obj->bad_faces) {
		ufbxt_assert(fbx_mesh->num_faces == obj_mesh->num_faces);
		ufbxt_assert(fbx_mesh->num_indices == obj_mesh->num_indices);
	}

	// Check that all vertices exist, anything more doesn't really make sense
	ufbxt_match_vertex *obj_verts = (ufbxt_match_vertex*)calloc(obj_mesh->num_indices, sizeof(ufbxt_match_vertex));
	ufbxt_match_vertex *fbx_verts = (ufbxt_match_vertex*)calloc(fbx_mesh->num_indices, sizeof(ufbxt_match_vertex));
	ufbxt_assert(obj_verts && fbx_verts);

	ufbx_matrix norm_mat = ufbx_get_compatible_matrix_for_normals(fbx_node);

	for (size_t i = 0; i < obj_mesh->num_indices; i++) {
		obj_verts[i].pos = ufbx_get_vertex_vec3(&obj_mesh->vertex_position, i);
		obj_verts[i].normal = ufbx_get_vertex_vec3(&obj_mesh->vertex_normal, i);
		if (obj_mesh->vertex_uv.exists) {
			obj_verts[i].uv = ufbx_get_vertex_vec2(&obj_mesh->vertex_uv, i);
		}

		if (obj->fbx_position_scale != 1.0) {
			obj_verts[i].pos.x *= (ufbx_real)obj->fbx_position_scale;
			obj_verts[i].pos.y *= (ufbx_real)obj->fbx_position_scale;
			obj_verts[i].pos.z *= (ufbx_real)obj->fbx_position_scale;
		}
		if (obj->fbx_rotation.w != 1.0f) {
			obj_verts[i].pos = ufbx_quat_rotate_vec3(obj->fbx_rotation, obj_verts[i].pos);
		}
		if (obj->negate_xz) {
			obj_verts[i].pos.x *= -1.0f;
			obj_verts[i].pos.z *= -1.0f;
			obj_verts[i].normal.x *= -1.0f;
			obj_verts[i].normal.z *= -1.0f;
		}

		if (scale != 1.0) {
			obj_verts[i].pos.x *= scale;
			obj_verts[i].pos.y *= scale;
			obj_verts[i].pos.z *= scale;
		}
	}
	for (size_t i = 0; i < fbx_mesh->num_indices; i++) {
		ufbx_vec3 fp = ufbx_get_vertex_vec3(&fbx_mesh->skinned_position, i);
		ufbx_vec3 fn = fbx_mesh->skinned_normal.exists ? ufbx_get_vertex_vec3(&fbx_mesh->skinned_normal, i) : ufbx_zero_vec3;
		if (fbx_mesh->skinned_is_local) {
			fp = ufbx_transform_position(&fbx_node->geometry_to_world, fp);
			fn = ufbx_transform_direction(&norm_mat, fn);
			fn = ufbxt_normalize(fn);
		}
		fbx_verts[i].pos = fp;
		fbx_verts[i].normal = fn;
		if (fbx_mesh->vertex_uv.exists) {
			fbx_verts[i].uv = ufbx_get_vertex_vec2(&fbx_mesh->vertex_uv, i);
		}

		if (scale != 1.0) {
			fbx_verts[i].pos.x *= scale;
			fbx_verts[i].pos.y *= scale;
			fbx_verts[i].pos.z *= scale;
		}
	}

	qsort(obj_verts, obj_mesh->num_indices, sizeof(ufbxt_match_vertex), &ufbxt_cmp_sub_vertex);
	qsort(fbx_verts, fbx_mesh->num_indices, sizeof(ufbxt_match_vertex), &ufbxt_cmp_sub_vertex);

	int32_t obj_verts_left = (int32_t)obj_mesh->num_indices;
	for (int32_t i = (int32_t)fbx_mesh->num_indices - 1; i >= 0; i--) {
		ufbxt_match_vertex v = fbx_verts[i];

		uint32_t face_ix = ufbx_find_face_index(fbx_mesh, (size_t)i);
		ufbx_face face = fbx_mesh->faces.data[face_ix];

		if (obj->bad_faces) {
			if (obj->bad_faces && ufbxt_face_has_duplicate_vertex(fbx_mesh, face)) {
				continue;
			} else if (obj->line_faces && face.num_indices == 2) {
				continue;
			} else if (obj->point_faces && face.num_indices == 1) {
				continue;
			}
		}

		bool found = false;
		uint32_t missing_count = 0;
		for (int32_t j = obj_verts_left - 1; j >= 0 && obj_verts[j].pos.x >= v.pos.x - tolerance; j--) {
			ufbx_real dx = obj_verts[j].pos.x - v.pos.x;
			ufbx_real dy = obj_verts[j].pos.y - v.pos.y;
			ufbx_real dz = obj_verts[j].pos.z - v.pos.z;
			ufbx_real dnx = obj_verts[j].normal.x - v.normal.x;
			ufbx_real dny = obj_verts[j].normal.y - v.normal.y;
			ufbx_real dnz = obj_verts[j].normal.z - v.normal.z;
			ufbx_real du = obj_verts[j].uv.x - v.uv.x;
			ufbx_real dv = obj_verts[j].uv.y - v.uv.y;

			if (obj->bad_normals) {
				dnx = 0.0f;
				dny = 0.0f;
				dnz = 0.0f;
			}

			if (obj->bad_uvs || !obj_mesh->vertex_uv.exists) {
				du = 0.0f;
				dv = 0.0f;
			}

			// ufbxt_assert(dx <= tolerance);
			ufbx_real err = (ufbx_real)sqrt(dx*dx + dy*dy + dz*dz + dnx*dnx + dny*dny + dnz*dnz + du*du + dv*dv);
			if (err < tolerance) {
				if (err > p_err->max) p_err->max = err;
				p_err->sum += err;
				p_err->num++;

				obj_verts[j] = obj_verts[obj_verts_left - 1];
				obj_verts_left--;
				found = true;
				break;
			}
		}

		if (!found) {
			ufbxt_assert(missing_count < obj->allow_missing);
			missing_count++;
		}
	}

	free(obj_verts);
	free(fbx_verts);
}

typedef struct {
	const char *cat_name;
	ufbx_node *node;
} ufbxt_obj_node;

static int ufbxt_cmp_string(const void *va, const void *vb)
{
	const char **a = (const char**)va, **b = (const char**)vb;
	return strcmp(*a, *b);
}

static int ufbxt_cmp_obj_node(const void *va, const void *vb)
{
	const ufbxt_obj_node *a = (const ufbxt_obj_node*)va, *b = (const ufbxt_obj_node*)vb;
	return strcmp(a->cat_name, b->cat_name);
}

static ufbxt_noinline size_t ufbxt_obj_group_key(char *cat_buf, size_t cat_cap, const char **groups, size_t num_groups, bool remove_namespaces)
{
	ufbxt_assert(num_groups > 0);
	qsort((void*)groups, num_groups, sizeof(const char*), &ufbxt_cmp_string);

	char *cat_ptr = cat_buf, *cat_end = cat_buf + cat_cap;
	for (size_t i = 0; i < num_groups; i++) {
		const char *group = groups[i];

		if (remove_namespaces) {
			for (;;) {
				const char *next = strchr(group, ':');
				if (!next) break;
				group = next + 1;
			}
		}

		size_t space = (size_t)(cat_end - cat_ptr);
		int res = snprintf(cat_ptr, space, "%s\n", group);
		ufbxt_assert(res >= 0 && (size_t)res < space);
		cat_ptr += res;
	}

	*--cat_ptr = '\0';
	return cat_ptr - cat_buf;
}

enum {
	UFBXT_OBJ_DIFF_FLAG_CHECK_DEFORMED_NORMALS = 0x1,
	UFBXT_OBJ_DIFF_FLAG_IGNORE_NORMALS = 0x2,
	UFBXT_OBJ_DIFF_FLAG_BAKED_ANIM = 0x4,
	UFBXT_OBJ_DIFF_FLAG_INHERIT_COMPENSATE = 0x4,
};

static bool ufbxt_has_mesh(ufbx_node *node)
{
	if (!node) return false;
	if (node->mesh) return true;
	if (ufbxt_has_mesh(node->scale_helper)) return true;
	if (ufbxt_has_mesh(node->geometry_transform_helper)) return true;
	return false;
}

static ufbxt_noinline void ufbxt_diff_to_obj(ufbx_scene *scene, ufbxt_obj_file *obj, ufbxt_diff_error *p_err, uint32_t flags)
{
	ufbx_node **used_nodes = (ufbx_node**)malloc(obj->num_meshes * sizeof(ufbx_node*));
	ufbxt_assert(used_nodes);

	size_t num_used_nodes = 0;

	char cat_name[4096];
	const char *groups[64];

	char *cat_name_data = NULL;
	size_t cat_name_offset = 0;

	ufbxt_obj_node *obj_nodes = (ufbxt_obj_node*)calloc(scene->nodes.count, sizeof(ufbxt_obj_node));
	ufbxt_assert(obj_nodes);

	size_t num_obj_nodes = 0;

	for (int step = 0; step < 2; step++) {
		for (size_t node_i = 0; node_i < scene->nodes.count; node_i++) {
			ufbx_node *node = scene->nodes.data[node_i];
			if (!node->name.length) continue;

			size_t num_groups = 0;
			ufbx_node *parent = node;
			while (parent && !parent->is_root) {
				if (obj->root_groups_at_bone && parent->attrib_type == UFBX_ELEMENT_BONE) break;

				ufbxt_assert(num_groups < ufbxt_arraycount(groups));
				groups[num_groups++] = parent->name.data;
				parent = parent->parent;
			}

			if (num_groups == 0) continue;

			size_t cat_len = ufbxt_obj_group_key(cat_name, sizeof(cat_name), groups, num_groups, obj->remove_namespaces);
			if (cat_name_data) {
				char *dst = cat_name_data + cat_name_offset;
				memcpy(dst, cat_name, cat_len + 1);

				ufbxt_obj_node *obj_node = &obj_nodes[num_obj_nodes++];
				obj_node->cat_name = dst;
				obj_node->node = node;
			}
			cat_name_offset += cat_len + 1;
		}

		if (cat_name_data) break;

		cat_name_data = (char*)malloc(cat_name_offset);
		ufbxt_assert(cat_name_data);
		cat_name_offset = 0;
	}

	qsort(obj_nodes, num_obj_nodes, sizeof(ufbxt_obj_node), &ufbxt_cmp_obj_node);

	for (size_t mesh_i = 0; mesh_i < obj->num_meshes; mesh_i++) {
		ufbxt_obj_mesh *obj_mesh = &obj->meshes[mesh_i];
		if (obj_mesh->num_indices == 0) continue;

		ufbx_node *node = NULL;

		if (obj->match_by_order) {
			ufbxt_assert(mesh_i + 1 < scene->nodes.count);
			node = scene->nodes.data[mesh_i + 1];
		}

		// Search for a node containing all the groups
		if (!node && obj_mesh->num_groups > 0) {
			ufbxt_obj_group_key(cat_name, sizeof(cat_name), obj_mesh->groups, obj_mesh->num_groups, obj->remove_namespaces);
			ufbxt_obj_node key = { cat_name };
			ufbxt_obj_node *found = (ufbxt_obj_node*)bsearch(&key, obj_nodes, num_obj_nodes,
				sizeof(ufbxt_obj_node), &ufbxt_cmp_obj_node);

			if (found) {
				if (found->node && ufbxt_has_mesh(found->node)) {
					bool seen = false;
					for (size_t i = 0; i < num_used_nodes; i++) {
						if (used_nodes[i] == found->node) {
							seen = true;
							break;
						}
					}
					if (!seen) {
						node = found->node;
					}
				}
			}
		}

		if (!node && obj_mesh->original_group && *obj_mesh->original_group) {
			if (obj->has_empty && !strcmp(obj_mesh->original_group, "ufbx:empty")) {
				for (size_t i = 1; i < scene->nodes.count; i++) {
					ufbx_node *empty_node = scene->nodes.data[i];
					if (empty_node->name.length == 0 && empty_node->mesh) {
						node = empty_node;
						break;
					}
				}
			} else {
				node = ufbx_find_node(scene, obj_mesh->original_group);
			}
		}

		if (!node) {
			for (size_t group_i = 0; group_i < obj_mesh->num_groups; group_i++) {
				const char *name = obj_mesh->groups[group_i];

				node = ufbx_find_node(scene, name);
				if (!node && obj->exporter == UFBXT_OBJ_EXPORTER_BLENDER) {
					// Blender concatenates _Material to names
					size_t name_len = strcspn(name, "_");
					node = ufbx_find_node_len(scene, name, name_len);
				}

				if (node && ufbxt_has_mesh(node)) {
					bool seen = false;
					for (size_t i = 0; i < num_used_nodes; i++) {
						if (used_nodes[i] == node) {
							seen = true;
							break;
						}
					}
					if (!seen) break;
				}
			}
		}

		if (obj->ignore_duplicates) {
			if (!node || !ufbxt_has_mesh(node)) {
				bool is_ncl = false;
				for (size_t i = 0; i < obj_mesh->num_groups; i++) {
					if (strstr(obj_mesh->groups[i], "_ncl1_")) {
						is_ncl = true;
						break;
					}
				}
				if (is_ncl) continue;
			}

			if (node && node->parent) {
				bool duplicate_name = false;
				ufbx_node *parent = node->parent;
				for (size_t i = 0; i < parent->children.count; i++) {
					ufbx_node *child = parent->children.data[i];
					if (child == node) continue;
					if (!strcmp(child->name.data, node->name.data)) {
						duplicate_name = true;
						break;
					}
				}
				if (duplicate_name) continue;
			}
		}

		if (!node) {
			ufbxt_assert(scene->meshes.count == 1);
			ufbxt_assert(scene->meshes.data[0]->instances.count == 1);
			node = scene->meshes.data[0]->instances.data[0];
		}

		ufbxt_assert(node);
		if (node->scale_helper) {
			node = node->scale_helper;
		}
		if (node->geometry_transform_helper) {
			node = node->geometry_transform_helper;
		}

		ufbx_mesh *mesh = node->mesh;

		double scale = 1.0;

		if (obj->normalize_units && scene->settings.unit_meters != 1.0) {
			scale *= scene->settings.unit_meters;
		}

		if (obj->position_scale != 1.0) {
			scale *= obj->position_scale;
		}
		#if defined(UFBX_REAL_IS_FLOAT)
			scale *= obj->position_scale_float;
		#endif
		if (flags & UFBXT_OBJ_DIFF_FLAG_BAKED_ANIM) {
			scale *= obj->position_scale_bake;
		}
		if (flags & UFBXT_OBJ_DIFF_FLAG_INHERIT_COMPENSATE) {
			scale *= obj->position_scale_compensate;
		}

		used_nodes[num_used_nodes++] = node;

		if (!mesh && node->attrib_type == UFBX_ELEMENT_NURBS_SURFACE) {
			ufbx_nurbs_surface *surface = (ufbx_nurbs_surface*)node->attrib;
			ufbx_tessellate_surface_opts opts = { 0 };
			opts.span_subdivision_u = surface->span_subdivision_u;
			opts.span_subdivision_v = surface->span_subdivision_v;
			ufbx_mesh *tess_mesh = ufbx_tessellate_nurbs_surface(surface, &opts, NULL);
			ufbxt_assert(tess_mesh);

			// ufbxt_debug_dump_obj_mesh("test.obj", node, tess_mesh);

			ufbxt_check_mesh(scene, tess_mesh);
			ufbxt_match_obj_mesh(obj, node, tess_mesh, obj_mesh, p_err, (ufbx_real)scale);
			ufbx_free_mesh(tess_mesh);

			continue;
		}

		ufbxt_assert(mesh);

		// Multiple skin deformers are not supported
		if (mesh->skin_deformers.count > 1) continue;

		ufbx_matrix *mat = &node->geometry_to_world;
		ufbx_matrix norm_mat = ufbx_get_compatible_matrix_for_normals(node);
		ufbx_matrix skin_mat;

		if (obj->transform_skin && mesh->skin_deformers.count > 0) {
			ufbx_pose *pose = node->bind_pose;
			ufbx_bone_pose *node_bone = ufbx_get_bone_pose(pose, node);

			// Try to find a shared bind pose
			if (!node_bone) {
				for (size_t i = 0; i < mesh->instances.count; i++) {
					ufbx_node *inst = mesh->instances.data[i];
					pose = inst->bind_pose;
					node_bone = ufbx_get_bone_pose(pose, inst);
					if (node_bone) break;
				}
			}

			ufbxt_assert(node_bone);
			ufbx_matrix inv_bind = ufbx_matrix_invert(&node_bone->bone_to_world);
			skin_mat = ufbx_matrix_mul(mat, &inv_bind);
			mat = &skin_mat;
			norm_mat = ufbx_matrix_for_normals(mat);
		}

		if (!obj->no_subdivision && (mesh->subdivision_display_mode == UFBX_SUBDIVISION_DISPLAY_SMOOTH || mesh->subdivision_display_mode == UFBX_SUBDIVISION_DISPLAY_HULL_AND_SMOOTH)) {
			ufbx_subdivide_opts opts = { 0 };
			opts.evaluate_source_vertices = true;
			opts.evaluate_skin_weights = true;

			int64_t uv_boundary = ufbx_find_int(&node->props, "ufbx:UVBoundary", -1);

			if (uv_boundary >= 0 && uv_boundary < UFBX_SUBDIVISION_BOUNDARY_COUNT) {
				opts.uv_boundary = (ufbx_subdivision_boundary)uv_boundary;
			} else {
				opts.uv_boundary = UFBX_SUBDIVISION_BOUNDARY_SHARP_BOUNDARY;
			}

			ufbx_mesh *sub_mesh = ufbx_subdivide_mesh(mesh, mesh->subdivision_preview_levels, &opts, NULL);
			ufbxt_assert(sub_mesh);

			// Check that we didn't break the original mesh
			ufbxt_check_mesh(scene, mesh);

			ufbxt_check_source_vertices(sub_mesh, mesh, p_err, 0.0);

			ufbxt_check_mesh(scene, sub_mesh);
			ufbxt_match_obj_mesh(obj, node, sub_mesh, obj_mesh, p_err, (ufbx_real)scale);
			ufbx_free_mesh(sub_mesh);

			continue;
		}

		if (!(obj->bad_faces || obj->line_faces || obj->point_faces)) {
			ufbxt_assert(obj_mesh->num_faces == mesh->num_faces);
			ufbxt_assert(obj_mesh->num_indices == mesh->num_indices);
		}

		bool check_normals = true;
		if (obj->bad_normals) check_normals = false;
		if ((flags & UFBXT_OBJ_DIFF_FLAG_CHECK_DEFORMED_NORMALS) == 0 && mesh->all_deformers.count > 0) check_normals = false;
		if ((flags & UFBXT_OBJ_DIFF_FLAG_IGNORE_NORMALS) != 0) check_normals = false;

		ufbx_string uv_set_name = ufbx_find_string(&node->props, "currentUVSet", ufbx_empty_string);
		ufbx_vertex_vec2 vertex_uv = mesh->vertex_uv;
		if (uv_set_name.length > 0) {
			for (size_t i = 0; i < mesh->uv_sets.count; i++) {
				const ufbx_uv_set *uv_set = &mesh->uv_sets.data[i];
				if (!strcmp(uv_set->name.data, uv_set_name.data)) {
					vertex_uv = uv_set->vertex_uv;
				}
			}
		}

		if (obj->bad_order) {
			ufbxt_match_obj_mesh(obj, node, mesh, obj_mesh, p_err, (ufbx_real)scale);
		} else {
			size_t obj_face_ix = 0;

			// Assume that the indices are in the same order!
			for (size_t face_ix = 0; face_ix < mesh->num_faces; face_ix++) {
				ufbx_face obj_face = obj_mesh->faces[obj_face_ix];
				ufbx_face face = mesh->faces.data[face_ix];

				if (obj->bad_faces && ufbxt_face_has_duplicate_vertex(mesh, face)) {
					continue;
				} else if (obj->line_faces && face.num_indices == 2) {
					continue;
				} else if (obj->point_faces && face.num_indices == 1) {
					continue;
				} else if (!obj->bad_faces) {
					ufbxt_assert(obj_face.index_begin == face.index_begin);
				}

				ufbxt_assert(obj_face.num_indices == face.num_indices);
				obj_face_ix++;

				for (size_t i = 0; i < face.num_indices; i++) {
					size_t oix = obj_face.index_begin + i;
					size_t fix = face.index_begin + i;

					ufbx_vec3 op = ufbx_get_vertex_vec3(&obj_mesh->vertex_position, oix);
					ufbx_vec3 fp = ufbx_get_vertex_vec3(&mesh->skinned_position, fix);
					ufbx_vec3 on = check_normals ? ufbx_get_vertex_vec3(&obj_mesh->vertex_normal, oix) : ufbx_zero_vec3;
					ufbx_vec3 fn = check_normals ? ufbx_get_vertex_vec3(&mesh->skinned_normal, fix) : ufbx_zero_vec3;

					if (mesh->skinned_is_local || obj->transform_skin) {
						fp = ufbx_transform_position(mat, fp);
						fn = ufbx_transform_direction(&norm_mat, fn);

						ufbx_real fn_len = (ufbx_real)sqrt(fn.x*fn.x + fn.y*fn.y + fn.z*fn.z);
						if (fn_len > 0.00001f) {
							fn.x /= fn_len;
							fn.y /= fn_len;
							fn.z /= fn_len;
						}

						ufbx_real on_len = (ufbx_real)sqrt(on.x*on.x + on.y*on.y + on.z*on.z);
						if (on_len > 0.00001f) {
							on.x /= on_len;
							on.y /= on_len;
							on.z /= on_len;
						}
					}

					if (obj->fbx_position_scale != 1.0) {
						fp.x *= (ufbx_real)obj->fbx_position_scale;
						fp.y *= (ufbx_real)obj->fbx_position_scale;
						fp.z *= (ufbx_real)obj->fbx_position_scale;
					}
					if (obj->fbx_rotation.w != 1.0f) {
						fp = ufbx_quat_rotate_vec3(obj->fbx_rotation, fp);
					}

					if (scale != 1.0) {
						fp.x *= (ufbx_real)scale;
						fp.y *= (ufbx_real)scale;
						fp.z *= (ufbx_real)scale;
						op.x *= (ufbx_real)scale;
						op.y *= (ufbx_real)scale;
						op.z *= (ufbx_real)scale;
					}

					if (obj->negate_xz) {
						op.x *= -1.0f;
						op.z *= -1.0f;
						on.x *= -1.0f;
						on.z *= -1.0f;
					}

					if (obj->ignore_unskinned && mesh->skin_deformers.count > 0) {
						ufbx_skin_deformer *skin = mesh->skin_deformers.data[0];
						ufbx_skin_vertex vert = skin->vertices.data[mesh->vertex_indices.data[fix]];
						if (vert.num_weights == 0) {
							continue;
						}
					}

					ufbxt_assert_close_vec3(p_err, op, fp);

					if (check_normals && !mesh->generated_normals) {
						ufbx_real on2 = on.x*on.x + on.y*on.y + on.z*on.z;
						ufbx_real fn2 = fn.x*fn.x + fn.y*fn.y + fn.z*fn.z;
						if (on2 > 0.01f && fn2 > 0.01f) {
							ufbxt_assert_close_vec3(p_err, on, fn);
						}
					}

					if (obj_mesh->vertex_uv.exists && !obj->bad_uvs) {
						ufbxt_assert(mesh->vertex_uv.exists);
						uint32_t ovx = obj_mesh->vertex_uv.indices.data[oix];

						if (ovx == UFBX_NO_INDEX) {
							if (!obj->missing_uvs) {
								ufbxt_assert(vertex_uv.indices.data[fix] == UFBX_NO_INDEX);
							}
						} else {
							ufbx_vec2 ou = ufbx_get_vertex_vec2(&obj_mesh->vertex_uv, oix);
							ufbx_vec2 fu = ufbx_get_vertex_vec2(&vertex_uv, fix);
							ufbxt_assert_close_vec2_threshold(p_err, ou, fu, obj->uv_tolerance);
							ufbxt_assert_close_vec2_threshold(p_err, ou, fu, obj->uv_tolerance);
						}
					}
				}
			}
		}
	}

	free(obj_nodes);
	free(cat_name_data);
	free(used_nodes);
}

static ufbx_mesh *ufbxt_subdivide_mesh(const ufbx_mesh *mesh, size_t level, const ufbx_subdivide_opts *opts, ufbx_error *error)
{
	ufbx_mesh *result = ufbx_subdivide_mesh(mesh, level, opts, error);
	if (!result) return result;

	if (result) {
		ufbx_subdivide_opts memory_opts = { 0 };
		if (opts) memory_opts = *opts;
		memory_opts.temp_allocator.huge_threshold = 1;
		memory_opts.result_allocator.huge_threshold = 1;

		ufbx_mesh *memory_result = ufbx_subdivide_mesh(mesh, level, &memory_opts, NULL);
		ufbxt_assert(memory_result);

		ufbx_subdivision_result *res = memory_result->subdivision_result;
		ufbxt_assert(res);

		for (size_t i = 1; i < res->temp_allocs; i++) {
			ufbx_error fail_error;
			ufbx_subdivide_opts fail_opts = memory_opts;
			fail_opts.temp_allocator.allocation_limit = i;
			ufbx_mesh *fail_result = ufbx_subdivide_mesh(mesh, level, &fail_opts, &fail_error);
			ufbxt_assert(!fail_result);
			ufbxt_assert(fail_error.type == UFBX_ERROR_ALLOCATION_LIMIT);
		}

		for (size_t i = 1; i < res->result_allocs; i++) {
			ufbx_error fail_error;
			ufbx_subdivide_opts fail_opts = memory_opts;
			fail_opts.result_allocator.allocation_limit = i;
			ufbx_mesh *fail_result = ufbx_subdivide_mesh(mesh, level, &fail_opts, &fail_error);
			ufbxt_assert(!fail_result);
			ufbxt_assert(fail_error.type == UFBX_ERROR_ALLOCATION_LIMIT);
		}

		ufbx_free_mesh(memory_result);
	}

	return result;
}

static void ufbxt_check_baked_anim(const ufbx_scene *scene, ufbx_baked_anim *bake)
{
	if (!bake) return;

	for (size_t i = 0; i < bake->nodes.count; i++) {
		if (i > 0) {
			ufbxt_assert(bake->nodes.data[i - 1].typed_id < bake->nodes.data[i].typed_id);
		}

		ufbx_baked_node *bake_node = &bake->nodes.data[i];
		ufbx_node *scene_node = scene->nodes.data[bake_node->typed_id];
		ufbxt_assert(scene_node->element_id == bake_node->element_id);

		ufbx_baked_node *ref = ufbx_find_baked_node(bake, scene_node);
		ufbxt_assert(ref == bake_node);
	}
	for (size_t i = 0; i < bake->elements.count; i++) {
		if (i > 0) {
			ufbxt_assert(bake->elements.data[i - 1].element_id < bake->elements.data[i].element_id);
		}

		ufbx_baked_element *bake_element = &bake->elements.data[i];
		ufbx_element *scene_element = scene->elements.data[bake_element->element_id];

		ufbx_baked_element *ref = ufbx_find_baked_element(bake, scene_element);
		ufbxt_assert(ref == bake_element);
	}
}

static ufbx_baked_anim *ufbxt_bake_anim(const ufbx_scene *scene, const ufbx_anim *anim, const ufbx_bake_opts *opts, ufbx_error *error)
{
	ufbx_baked_anim *result = ufbx_bake_anim(scene, anim, opts, error);
	if (!result) return result;
	ufbxt_check_baked_anim(scene, result);

	if (result) {
		ufbx_bake_opts memory_opts = { 0 };
		if (opts) memory_opts = *opts;
		memory_opts.temp_allocator.huge_threshold = 1;
		memory_opts.result_allocator.huge_threshold = 1;

		ufbx_baked_anim *memory_result = ufbx_bake_anim(scene, anim, &memory_opts, NULL);
		ufbxt_assert(memory_result);

		for (size_t i = 1; i < memory_result->metadata.temp_allocs; i++) {
			ufbx_error fail_error;
			ufbx_bake_opts fail_opts = memory_opts;
			fail_opts.temp_allocator.allocation_limit = i;
			ufbx_baked_anim *fail_result = ufbx_bake_anim(scene, anim, &fail_opts, &fail_error);
			ufbxt_assert(!fail_result);
			ufbxt_assert(fail_error.type == UFBX_ERROR_ALLOCATION_LIMIT);
		}

		for (size_t i = 1; i < memory_result->metadata.result_allocs; i++) {
			ufbx_error fail_error;
			ufbx_bake_opts fail_opts = memory_opts;
			fail_opts.result_allocator.allocation_limit = i;
			ufbx_baked_anim *fail_result = ufbx_bake_anim(scene, anim, &fail_opts, &fail_error);
			ufbxt_assert(!fail_result);
			ufbxt_assert(fail_error.type == UFBX_ERROR_ALLOCATION_LIMIT);
		}

		ufbx_free_baked_anim(memory_result);
	}

	return result;
}

typedef struct {
	uint16_t r, g, b, a;
} ufbxt_pixel16;

typedef struct {
	ufbxt_pixel16 *pixels;
	uint32_t width;
	uint32_t height;
} ufbxt_image16;

static ufbxt_pixel16 ufbxt_px16(uint16_t r, uint16_t g, uint16_t b, uint16_t a) {
	ufbxt_pixel16 px = { r, g, b, a };
	return px;
}

// Load a PNG image file
// http://www.libpng.org/pub/png/spec/1.2/PNG-Contents.html
static ufbxt_image16 ufbxt_read_png(const void *data, size_t size, const char **p_error)
{
	ufbxt_image16 image = { 0 };
	const ufbxt_pixel16 empty_pixel = { 0 };

	const uint8_t *ptr = (const uint8_t*)data, *end = ptr + size;
	uint8_t bit_depth = 0, pixel_values = 0, pixel_format;
	uint8_t *deflate_data = NULL, *src = NULL, *dst = NULL;
	ufbxt_pixel16 *palette = NULL;
	size_t deflate_size = 0, palette_size = 0;
	static const uint8_t lace_none[] = { 0,0,1,1, 0,0,0,0 };
	static const uint8_t lace_adam7[] = {
		0,0,8,8, 4,0,8,8, 0,4,4,8, 2,0,4,4, 0,2,2,4, 1,0,2,2, 0,1,1,2, 0,0,0,0, };
	uint32_t scale = 1;
	const uint8_t *lace = lace_none; // Interlacing pattern (x0,y0,dx,dy)
	ufbxt_pixel16 trns = { 0, 0, 0, 0 }; // Transparent pixel value for RGB
	const char *error = NULL;

	if (end - ptr < 8) error = "file header truncated";
	if (!error && memcmp(ptr, "\x89PNG\r\n\x1a\n", 8)) error = "bad_file_header";
	ptr += 8;

	// Iterate chunks: gather IDAT into a single buffer
	while (!error) {
		if (end - ptr < 8) {
			error = "chunk header truncated";
			break;
		}
		uint32_t chunk_len = ptr[0]<<24 | ptr[1]<<16 | ptr[2]<<8 | ptr[3];
		const uint8_t *tag = ptr + 4;
		ptr += 8;
		if ((uint32_t)(end - ptr) < chunk_len + 4) {
			error = "chunk data truncated";
			break;
		}
		if (!memcmp(tag, "IHDR", 4) && chunk_len >= 13) {
			image.width  = ptr[0]<<24 | ptr[1]<<16 | ptr[2]<<8 | ptr[3];
			image.height = ptr[4]<<24 | ptr[5]<<16 | ptr[6]<<8 | ptr[7];
			bit_depth = ptr[8];
			pixel_format = ptr[9];
			switch (pixel_format) {
			case 0: pixel_values = 1; break;
			case 2: pixel_values = 3; break;
			case 3: pixel_values = 1; break;
			case 4: pixel_values = 2; break;
			case 6: pixel_values = 4; break;
			default:
				error = "unknown pixel format";
				continue;
			}
			for (uint32_t i = 0; i < 16 && pixel_format != 3; i += bit_depth) scale |= 1u << i;
			if (ptr[12] != 0) lace = lace_adam7;
			if (ptr[10] != 0 || ptr[11] != 0) {
				error = "unknown settings";
				break;
			}
		} else if (!memcmp(tag, "IDAT", 4)) {
			deflate_data = (uint8_t*)realloc(deflate_data, deflate_size + chunk_len);
			if (!deflate_data) {
				error = "allocation failure";
				continue;
			}
			memcpy(deflate_data + deflate_size, ptr, chunk_len);
			deflate_size += chunk_len;
		} else if (!memcmp(tag, "PLTE", 4)) {
			palette_size = chunk_len / 3;
			palette = (ufbxt_pixel16*)malloc(palette_size);
			if (!palette) {
				error = "allocation failure";
				continue;
			}
			for (size_t i = 0; i < palette_size; i++) {
				palette[i].r = (uint16_t)(ptr[i*3 + 0]*0x101);
				palette[i].g = (uint16_t)(ptr[i*3 + 1]*0x101);
				palette[i].b = (uint16_t)(ptr[i*3 + 2]*0x101);
				palette[i].a = 0;
			}
		} else if (!memcmp(tag, "IEND", 4)) {
			break;
		} else if (!memcmp(tag, "tRNS", 4)) {
			if (pixel_format == 2 && chunk_len >= 6) {
				trns.r = (uint16_t)((ptr[0]<<8 | ptr[1]) * scale);
				trns.g = (uint16_t)((ptr[2]<<8 | ptr[3]) * scale);
				trns.b = (uint16_t)((ptr[4]<<8 | ptr[5]) * scale);
			} else if (pixel_format == 0 && chunk_len >= 2) {
				uint16_t v = (uint16_t)(ptr[0]<<8 | ptr[1]) * scale;
				trns.r = trns.g = trns.b = v;
			} else if (pixel_format == 3) {
				for (size_t i = 0; i < chunk_len; i++) {
					if (palette_size) palette[i].a = ptr[i] * 0x101;
				}
			}
		}
		ptr += chunk_len + 4; // Skip data and CRC
	}

	size_t bpp = (pixel_values * bit_depth + 7) / 8;
	size_t stride = (image.width * pixel_values * bit_depth + 7) / 8;
	if (image.width == 0 || image.height == 0) {
		error = "bad image size";
	}
	size_t src_size = image.height * stride + image.height * (lace == lace_adam7 ? 14 : 1);
	if (!error) {
		src = (uint8_t*)calloc(src_size, sizeof(uint8_t));
		dst = (uint8_t*)calloc((image.height + 1) * (stride + bpp), sizeof(uint8_t));
		image.pixels = (ufbxt_pixel16*)calloc(image.width * image.height, sizeof(ufbxt_pixel16));
	}
	if (!src || !dst || !image.pixels) {
		error = "allocation failure";
	}

	// Decompress the combined IDAT chunks
	ufbx_inflate_retain retain;
	retain.initialized = false;

	ufbx_inflate_input input = { 0 };
	input.total_size = deflate_size;
	input.data_size = deflate_size;
	input.data = deflate_data;

	ptrdiff_t res = ufbx_inflate(src, src_size, &input, &retain);
	if (res != (ptrdiff_t)src_size) {
		error = "deflate error";
	}
	uint8_t *sp = src, *sp_end = sp + src_size;

	for (; !error && lace[2]; lace += 4) {
		int32_t width = ((int32_t)image.width - lace[0] + lace[2] - 1) / lace[2];
		int32_t height = ((int32_t)image.height - lace[1] + lace[3] - 1) / lace[3];
		if (width <= 0 || height <= 0) continue;
		size_t lace_stride = (width * pixel_values * bit_depth + 7) / 8;
		if ((size_t)(sp_end - sp) < height * (1 + lace_stride)) {
			error = "data truncated";
			break;
		}

		// Unfilter the scanlines
		ptrdiff_t dx = bpp, dy = stride + bpp;
		for (int32_t y = 0; y < height; y++) {
			uint8_t *dp = dst + (stride + bpp) * (1 + y) + bpp;
			uint8_t filter = *sp++;
			for (int32_t x = 0; x < lace_stride; x++) {
				uint8_t s = *sp++, *d = dp++;
				switch (filter) {
				case 0: d[0] = s; break; // 6.2: No filter
				case 1: d[0] = d[-dx] + s; break; // 6.3: Sub (predict left)
				case 2: d[0] = d[-dy] + s; break; // 6.4: Up (predict top)
				case 3: d[0] = (d[-dx] + d[-dy]) / 2 + s; break; // 6.5: Average (top+left)
				case 4: { // 6.6: Paeth (choose closest of 3 to estimate)
					int32_t a = d[-dx], b = d[-dy], c = d[-dx - dy], p = a+b-c;
					int32_t pa = abs(p-a), pb = abs(p-b), pc = abs(p-c);
					if (pa <= pb && pa <= pc) d[0] = (uint8_t)(a + s);
					else if (pb <= pc) d[0] = (uint8_t)(b + s);
					else d[0] = (uint8_t)(c + s);
				} break;
				default:
					error = "unknown filter";
					break;
				}
			}
		}

		// Expand to RGBA pixels
		for (int32_t y = 0; y < height; y++) {
			uint8_t *dr = dst + (stride + bpp) * (y + 1) + bpp;
			for (int32_t x = 0; x < width; x++) {
				uint16_t v[4];
				for (uint32_t c = 0; c < pixel_values; c++) {
					ptrdiff_t bit = (x * pixel_values + c + 1) * bit_depth;
					uint32_t raw = (dr[(bit - 9) >> 3] << 8 | dr[(bit - 1) >> 3]) >> ((8 - bit) & 7);
					v[c] = (raw & ((1 << bit_depth) - 1)) * scale;
				}

				ufbxt_pixel16 px;
				switch (pixel_format) {
				case 0: px = ufbxt_px16(v[0], v[0], v[0], 0xffff); break;
				case 2: px = ufbxt_px16(v[0], v[1], v[2], 0xffff); break;
				case 3: px = v[0] < palette_size ? palette[v[0]] : empty_pixel; break;
				case 4: px = ufbxt_px16(v[0], v[0], v[0], v[1]); break;
				case 6: px = ufbxt_px16(v[0], v[1], v[2], v[3]); break;
				}
				if (px.r == trns.r && px.g == trns.g && px.b == trns.b && px.a == trns.a) px = empty_pixel;
				image.pixels[(lace[1]+y*lace[3]) * image.width + (lace[0]+x*lace[2])] = px;
			}
		}
	}

	free(deflate_data);
	free(palette);
	free(src);
	free(dst);
	if (error) {
		if (p_error) *p_error = error;
		free(image.pixels);
		image.pixels = NULL;
		image.width = 0;
		image.height = 0;
	}

	return image;
}

static ufbx_real ufbxt_srgb_to_linear(ufbx_real a)
{
	a = ufbxt_clamp(a, 0.0f, 1.0f);
	return a <= (ufbx_real)0.04045
		? (ufbx_real)0.0773993808 * a
		: (ufbx_real)pow((double)a*0.947867298578f+0.0521327014218, 2.4);
}

// -- IO

static ufbxt_noinline size_t ufbxt_file_size(const char *name)
{
	FILE *file = fopen(name, "rb");
	if (!file) return 0;
	fseek(file, 0, SEEK_END);
#if defined(_WIN32)
	size_t size = (size_t)_ftelli64(file);
#else
	size_t size = (size_t)ftell(file);
#endif
	fclose(file);
	return size;
}

static ufbxt_noinline void *ufbxt_read_file(const char *name, size_t *p_size)
{
	FILE *file = fopen(name, "rb");
	if (!file) return NULL;

	fseek(file, 0, SEEK_END);
#if defined(_WIN32)
	size_t size = (size_t)_ftelli64(file);
#else
	size_t size = (size_t)ftell(file);
#endif
	fseek(file, 0, SEEK_SET);

	char *data = (char*)malloc(size + 1);
	ufbxt_assert(data != NULL);
	size_t num_read = fread(data, 1, size, file);
	fclose(file);

	data[size] = '\0';

	if (num_read != size) {
		ufbxt_assert_fail(__FILE__, __LINE__, "Failed to load file");
	}

	*p_size = size;
	return data;
}

static ufbxt_noinline void *ufbxt_read_file_ex(const char *name, size_t *p_size)
{
	size_t name_len = strlen(name);
	if (name_len >= 3 && !memcmp(name + name_len - 3, ".gz", 3)) {
		size_t gz_size = 0;
		void *gz_data = ufbxt_read_file(name, &gz_size);
		if (!gz_data) return NULL;

		void *result = ufbxt_decompress_gzip(gz_data, gz_size, p_size);
		free(gz_data);

		return result;
	} else {
		return ufbxt_read_file(name, p_size);
	}
}

#endif
