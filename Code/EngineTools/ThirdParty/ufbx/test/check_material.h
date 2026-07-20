#ifndef UFBXT_CHECK_MATERIAL_H_INCLUDED
#define UFBXT_CHECK_MATERIAL_H_INCLUDED

#include "../ufbx.h"
#include <stdlib.h>

static const char *ufbxt_fbx_map_name(ufbx_material_fbx_map map)
{
	switch (map) {
	case UFBX_MATERIAL_FBX_DIFFUSE_FACTOR: return "diffuse_factor";
	case UFBX_MATERIAL_FBX_DIFFUSE_COLOR: return "diffuse_color";
	case UFBX_MATERIAL_FBX_SPECULAR_FACTOR: return "specular_factor";
	case UFBX_MATERIAL_FBX_SPECULAR_COLOR: return "specular_color";
	case UFBX_MATERIAL_FBX_SPECULAR_EXPONENT: return "specular_exponent";
	case UFBX_MATERIAL_FBX_REFLECTION_FACTOR: return "reflection_factor";
	case UFBX_MATERIAL_FBX_REFLECTION_COLOR: return "reflection_color";
	case UFBX_MATERIAL_FBX_TRANSPARENCY_FACTOR: return "transparency_factor";
	case UFBX_MATERIAL_FBX_TRANSPARENCY_COLOR: return "transparency_color";
	case UFBX_MATERIAL_FBX_EMISSION_FACTOR: return "emission_factor";
	case UFBX_MATERIAL_FBX_EMISSION_COLOR: return "emission_color";
	case UFBX_MATERIAL_FBX_AMBIENT_FACTOR: return "ambient_factor";
	case UFBX_MATERIAL_FBX_AMBIENT_COLOR: return "ambient_color";
	case UFBX_MATERIAL_FBX_NORMAL_MAP: return "normal_map";
	case UFBX_MATERIAL_FBX_BUMP: return "bump";
	case UFBX_MATERIAL_FBX_BUMP_FACTOR: return "bump_factor";
	case UFBX_MATERIAL_FBX_DISPLACEMENT_FACTOR: return "displacement_factor";
	case UFBX_MATERIAL_FBX_DISPLACEMENT: return "displacement";
	case UFBX_MATERIAL_FBX_VECTOR_DISPLACEMENT_FACTOR: return "vector_displacement_factor";
	case UFBX_MATERIAL_FBX_VECTOR_DISPLACEMENT: return "vector_displacement";
#if UFBX_HAS_FORCE_32BIT
	case UFBX_MATERIAL_FBX_MAP_FORCE_32BIT: ufbx_assert(0); return NULL;
#endif
	}

	ufbxt_assert(0 && "Unhandled PBR map name");
	return NULL;
}

static const char *ufbxt_pbr_map_name(ufbx_material_pbr_map map)
{
	switch (map) {
	case UFBX_MATERIAL_PBR_BASE_FACTOR: return "base_factor";
	case UFBX_MATERIAL_PBR_BASE_COLOR: return "base_color";
	case UFBX_MATERIAL_PBR_ROUGHNESS: return "roughness";
	case UFBX_MATERIAL_PBR_METALNESS: return "metalness";
	case UFBX_MATERIAL_PBR_DIFFUSE_ROUGHNESS: return "diffuse_roughness";
	case UFBX_MATERIAL_PBR_SPECULAR_FACTOR: return "specular_factor";
	case UFBX_MATERIAL_PBR_SPECULAR_COLOR: return "specular_color";
	case UFBX_MATERIAL_PBR_SPECULAR_IOR: return "specular_ior";
	case UFBX_MATERIAL_PBR_SPECULAR_ANISOTROPY: return "specular_anisotropy";
	case UFBX_MATERIAL_PBR_SPECULAR_ROTATION: return "specular_rotation";
	case UFBX_MATERIAL_PBR_TRANSMISSION_FACTOR: return "transmission_factor";
	case UFBX_MATERIAL_PBR_TRANSMISSION_COLOR: return "transmission_color";
	case UFBX_MATERIAL_PBR_TRANSMISSION_DEPTH: return "transmission_depth";
	case UFBX_MATERIAL_PBR_TRANSMISSION_SCATTER: return "transmission_scatter";
	case UFBX_MATERIAL_PBR_TRANSMISSION_SCATTER_ANISOTROPY: return "transmission_scatter_anisotropy";
	case UFBX_MATERIAL_PBR_TRANSMISSION_DISPERSION: return "transmission_dispersion";
	case UFBX_MATERIAL_PBR_TRANSMISSION_ROUGHNESS: return "transmission_roughness";
	case UFBX_MATERIAL_PBR_TRANSMISSION_EXTRA_ROUGHNESS: return "transmission_extra_roughness";
	case UFBX_MATERIAL_PBR_TRANSMISSION_PRIORITY: return "transmission_priority";
	case UFBX_MATERIAL_PBR_TRANSMISSION_ENABLE_IN_AOV: return "transmission_enable_in_aov";
	case UFBX_MATERIAL_PBR_SUBSURFACE_FACTOR: return "subsurface_factor";
	case UFBX_MATERIAL_PBR_SUBSURFACE_COLOR: return "subsurface_color";
	case UFBX_MATERIAL_PBR_SUBSURFACE_RADIUS: return "subsurface_radius";
	case UFBX_MATERIAL_PBR_SUBSURFACE_SCALE: return "subsurface_scale";
	case UFBX_MATERIAL_PBR_SUBSURFACE_ANISOTROPY: return "subsurface_anisotropy";
	case UFBX_MATERIAL_PBR_SUBSURFACE_TINT_COLOR: return "subsurface_tint_color";
	case UFBX_MATERIAL_PBR_SUBSURFACE_TYPE: return "subsurface_type";
	case UFBX_MATERIAL_PBR_SHEEN_FACTOR: return "sheen_factor";
	case UFBX_MATERIAL_PBR_SHEEN_COLOR: return "sheen_color";
	case UFBX_MATERIAL_PBR_SHEEN_ROUGHNESS: return "sheen_roughness";
	case UFBX_MATERIAL_PBR_COAT_FACTOR: return "coat_factor";
	case UFBX_MATERIAL_PBR_COAT_COLOR: return "coat_color";
	case UFBX_MATERIAL_PBR_COAT_ROUGHNESS: return "coat_roughness";
	case UFBX_MATERIAL_PBR_COAT_IOR: return "coat_ior";
	case UFBX_MATERIAL_PBR_COAT_ANISOTROPY: return "coat_anisotropy";
	case UFBX_MATERIAL_PBR_COAT_ROTATION: return "coat_rotation";
	case UFBX_MATERIAL_PBR_COAT_NORMAL: return "coat_normal";
	case UFBX_MATERIAL_PBR_COAT_AFFECT_BASE_COLOR: return "coat_affect_base_color";
	case UFBX_MATERIAL_PBR_COAT_AFFECT_BASE_ROUGHNESS: return "coat_affect_base_roughness";
	case UFBX_MATERIAL_PBR_THIN_FILM_FACTOR: return "thin_film_factor";
	case UFBX_MATERIAL_PBR_THIN_FILM_THICKNESS: return "thin_film_thickness";
	case UFBX_MATERIAL_PBR_THIN_FILM_IOR: return "thin_film_ior";
	case UFBX_MATERIAL_PBR_EMISSION_FACTOR: return "emission_factor";
	case UFBX_MATERIAL_PBR_EMISSION_COLOR: return "emission_color";
	case UFBX_MATERIAL_PBR_OPACITY: return "opacity";
	case UFBX_MATERIAL_PBR_INDIRECT_DIFFUSE: return "indirect_diffuse";
	case UFBX_MATERIAL_PBR_INDIRECT_SPECULAR: return "indirect_specular";
	case UFBX_MATERIAL_PBR_NORMAL_MAP: return "normal_map";
	case UFBX_MATERIAL_PBR_TANGENT_MAP: return "tangent_map";
	case UFBX_MATERIAL_PBR_DISPLACEMENT_MAP: return "displacement_map";
	case UFBX_MATERIAL_PBR_MATTE_FACTOR: return "matte_factor";
	case UFBX_MATERIAL_PBR_MATTE_COLOR: return "matte_color";
	case UFBX_MATERIAL_PBR_AMBIENT_OCCLUSION: return "ambient_occlusion";
	case UFBX_MATERIAL_PBR_GLOSSINESS: return "glossiness";
	case UFBX_MATERIAL_PBR_COAT_GLOSSINESS: return "coat_glossiness";
	case UFBX_MATERIAL_PBR_TRANSMISSION_GLOSSINESS: return "transmission_glossiness";
#if UFBX_HAS_FORCE_32BIT
	case UFBX_MATERIAL_PBR_MAP_FORCE_32BIT: ufbx_assert(0); return NULL;
#endif
	}

	ufbxt_assert(0 && "Unhandled PBR map name");
	return 0;
}

static const char *ufbxt_material_feature_name(ufbx_material_feature feature)
{
	switch (feature) {
	case UFBX_MATERIAL_FEATURE_PBR: return "pbr";
	case UFBX_MATERIAL_FEATURE_METALNESS: return "metalness";
	case UFBX_MATERIAL_FEATURE_DIFFUSE: return "diffuse";
	case UFBX_MATERIAL_FEATURE_SPECULAR: return "specular";
	case UFBX_MATERIAL_FEATURE_EMISSION: return "emission";
	case UFBX_MATERIAL_FEATURE_TRANSMISSION: return "transmission";
	case UFBX_MATERIAL_FEATURE_COAT: return "coat";
	case UFBX_MATERIAL_FEATURE_SHEEN: return "sheen";
	case UFBX_MATERIAL_FEATURE_OPACITY: return "opacity";
	case UFBX_MATERIAL_FEATURE_AMBIENT_OCCLUSION: return "ambient_occlusion";
	case UFBX_MATERIAL_FEATURE_MATTE: return "matte";
	case UFBX_MATERIAL_FEATURE_UNLIT: return "unlit";
	case UFBX_MATERIAL_FEATURE_IOR: return "ior";
	case UFBX_MATERIAL_FEATURE_DIFFUSE_ROUGHNESS: return "diffuse_roughness";
	case UFBX_MATERIAL_FEATURE_TRANSMISSION_ROUGHNESS: return "transmission_roughness";
	case UFBX_MATERIAL_FEATURE_THIN_WALLED: return "thin_walled";
	case UFBX_MATERIAL_FEATURE_CAUSTICS: return "caustics";
	case UFBX_MATERIAL_FEATURE_EXIT_TO_BACKGROUND: return "exit_to_background";
	case UFBX_MATERIAL_FEATURE_INTERNAL_REFLECTIONS: return "internal_reflections";
	case UFBX_MATERIAL_FEATURE_DOUBLE_SIDED: return "double_sided";
	case UFBX_MATERIAL_FEATURE_ROUGHNESS_AS_GLOSSINESS: return "roughness_as_glossiness";
	case UFBX_MATERIAL_FEATURE_COAT_ROUGHNESS_AS_GLOSSINESS: return "coat_roughness_as_glossiness";
	case UFBX_MATERIAL_FEATURE_TRANSMISSION_ROUGHNESS_AS_GLOSSINESS: return "transmission_roughness_as_glossiness";
#if UFBX_HAS_FORCE_32BIT
	case UFBX_MATERIAL_FEATURE_FORCE_32BIT: ufbx_assert(0); return NULL;
#endif
	}

	ufbxt_assert(0 && "Unhandled material feature name");
	return 0;
}

static const char *ufbxt_shader_type_name(ufbx_shader_type map)
{
	switch (map) {
	case UFBX_SHADER_UNKNOWN: return "unknown";
	case UFBX_SHADER_FBX_LAMBERT: return "fbx_lambert";
	case UFBX_SHADER_FBX_PHONG: return "fbx_phong";
	case UFBX_SHADER_OSL_STANDARD_SURFACE: return "osl_standard_surface";
	case UFBX_SHADER_ARNOLD_STANDARD_SURFACE: return "arnold_standard_surface";
	case UFBX_SHADER_3DS_MAX_PHYSICAL_MATERIAL: return "3ds_max_physical_material";
	case UFBX_SHADER_3DS_MAX_PBR_METAL_ROUGH: return "3ds_max_pbr_metal_rough";
	case UFBX_SHADER_3DS_MAX_PBR_SPEC_GLOSS: return "3ds_max_pbr_spec_gloss";
	case UFBX_SHADER_GLTF_MATERIAL: return "gltf_material";
	case UFBX_SHADER_OPENPBR_MATERIAL: return "openpbr_material";
	case UFBX_SHADER_SHADERFX_GRAPH: return "shaderfx_graph";
	case UFBX_SHADER_BLENDER_PHONG: return "blender_phong";
	case UFBX_SHADER_WAVEFRONT_MTL: return "wavefront_mtl";
#if UFBX_HAS_FORCE_32BIT
	case UFBX_SHADER_TYPE_FORCE_32BIT: ufbx_assert(0); return NULL;
#endif
	}

	ufbxt_assert(0 && "Unhandled shader type");
	return 0;
}

static size_t ufbxt_tokenize_line(char *buf, size_t buf_len, const char **tokens, size_t max_tokens, const char **p_line, const char *sep_chars)
{
	bool prev_sep = true;

	size_t num_tokens = 0;
	size_t buf_pos = 0;

	const char *p = *p_line;
	for (;;) {
		char c = *p;
		if (c == '\0' || c == '\n') break;

		bool sep = false;
		for (const char *s = sep_chars; *s; s++) {
			if (*s == c) {
				sep = true;
				break;
			}
		}

		if (!sep) {
			if (prev_sep) {
				ufbxt_assert(buf_pos < buf_len);
				buf[buf_pos++] = '\0';

				ufbxt_assert(num_tokens < max_tokens);
				tokens[num_tokens++] = buf + buf_pos;
			}

			if (c == '"') {
				p++;
				while (*p != '"') {
					c = *p;
					if (c == '\\') {
						p++;
						switch (*p) {
						case 'n': c = '\n'; break;
						case 'r': c = '\r'; break;
						case 't': c = '\t'; break;
						case '\\': c = '\\'; break;
						case '"': c = '"'; break;
						default:
							fprintf(stderr, "Bad escape '\\%c'\n", *p);
							ufbxt_assert(0 && "Bad escape");
							return 0;
						}
					}
					ufbxt_assert(buf_pos < buf_len);
					buf[buf_pos++] = c;
					p++;
				}
			} else {
				ufbxt_assert(buf_pos < buf_len);
				buf[buf_pos++] = c;
			}
		}
		prev_sep = sep;
		p++;
	}

	ufbxt_assert(buf_pos < buf_len);
	buf[buf_pos++] = '\0';

	for (size_t i = num_tokens; i < max_tokens; i++) {
		tokens[i] = "";
	}

	if (*p == '\n') p += 1;
	*p_line = p;

	return num_tokens;
}

typedef struct {
	uint8_t chunk[64];
	uint64_t len;
	uint32_t a,b,c,d;
} ufbxt_md5_ctx;

static void ufbxt_md5_init(ufbxt_md5_ctx*v) {
	v->len = 0;
	v->a = 0x67452301u;
	v->b = 0xEFCDAB89u;
	v->c = 0x98BADCFEu;
	v->d = 0x10325476u;
}

static void ufbxt_md5_step(ufbxt_md5_ctx *v)
{
	static const uint8_t s[64] = {
		7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
		5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
		4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
		6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
	};
	static const uint32_t k[64] = {
		0xD76AA478u, 0xE8C7B756u, 0x242070DBu, 0xC1BDCEEEu,
		0xF57C0FAFu, 0x4787C62Au, 0xA8304613u, 0xFD469501u,
		0x698098D8u, 0x8B44F7AFu, 0xFFFF5BB1u, 0x895CD7BEu,
		0x6B901122u, 0xFD987193u, 0xA679438Eu, 0x49B40821u,
		0xF61E2562u, 0xC040B340u, 0x265E5A51u, 0xE9B6C7AAu,
		0xD62F105Du, 0x02441453u, 0xD8A1E681u, 0xE7D3FBC8u,
		0x21E1CDE6u, 0xC33707D6u, 0xF4D50D87u, 0x455A14EDu,
		0xA9E3E905u, 0xFCEFA3F8u, 0x676F02D9u, 0x8D2A4C8Au,
		0xFFFA3942u, 0x8771F681u, 0x6D9D6122u, 0xFDE5380Cu,
		0xA4BEEA44u, 0x4BDECFA9u, 0xF6BB4B60u, 0xBEBFBC70u,
		0x289B7EC6u, 0xEAA127FAu, 0xD4EF3085u, 0x04881D05u,
		0xD9D4D039u, 0xE6DB99E5u, 0x1FA27CF8u, 0xC4AC5665u,
		0xF4292244u, 0x432AFF97u, 0xAB9423A7u, 0xFC93A039u,
		0x655B59C3u, 0x8F0CCC92u, 0xFFEFF47Du, 0x85845DD1u,
		0x6FA87E4Fu, 0xFE2CE6E0u, 0xA3014314u, 0x4E0811A1u,
		0xF7537E82u, 0xBD3AF235u, 0x2AD7D2BBu, 0xEB86D391u,
	};
	static const uint8_t g[64] = {
		0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
		4, 24, 44, 0, 20, 40, 60, 16, 36, 56, 12, 32, 52, 8, 28, 48,
		20, 32, 44, 56, 4, 16, 28, 40, 52, 0, 12, 24, 36, 48, 60, 8,
		0, 28, 56, 20, 48, 12, 40, 4, 32, 60, 24, 52, 16, 44, 8, 36,
	};
	uint32_t a = v->a, b = v->b, c = v->c, d = v->d;
	for(uint32_t i = 0; i < 64; i++) {
		uint32_t f = 0;
		switch(i&0x30) {
			case 0x00: f = (b&c)|(d&~b); break;
			case 0x10: f = (b&d)|(c&~d); break;
			case 0x20: f = b^c^d; break;
			case 0x30: f = c^(b|~d); break;
		}

		f += a + k[i] + v -> chunk[g[i]] + (v -> chunk[g[i] + 1] << 8) + (v -> chunk[g[i] + 2] << 16) + (v -> chunk[g[i] + 3] << 24);
		a = d; d = c; c = b;
		b += (f << s[i]) | (f >> (32 - s[i]));
	}
	v->a += a; v->b += b; v->c += c; v->d += d;
}

static void ufbxt_md5_write(ufbxt_md5_ctx *v, const char *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		v->chunk[v->len++ & 63] = buf[i];
		if ((v->len & 63) == 0) {
			ufbxt_md5_step(v);
		}
	}
}

static void ufbxt_md5_finish(ufbxt_md5_ctx *v, uint8_t *result)
{
	uint64_t n = v->len * 8;

	uint8_t buf[8];
	buf[0] = (uint8_t)(n >> 0x00); buf[1] = (uint8_t)(n >> 0x08); buf[2] = (uint8_t)(n >> 0x10); buf[3] = (uint8_t)(n >> 0x18);
	buf[4] = (uint8_t)(n >> 0x20); buf[5] = (uint8_t)(n >> 0x28); buf[6] = (uint8_t)(n >> 0x30); buf[7] = (uint8_t)(n >> 0x38);
	ufbxt_md5_write(v, "\x80", 1);

	size_t offset = (size_t)v->len & 63;
	memset(v->chunk + offset, 0, 64 - offset);

	if(offset > 56) {
		ufbxt_md5_step(v);
		memset(v->chunk, 0, 56);
	}
	memcpy(v->chunk + 56, buf, 8);
	ufbxt_md5_step(v);

	result[0x0] = (uint8_t)(v->a); result[0x1] = (uint8_t)(v->a>>8); result[0x2] = (uint8_t)(v->a>>16); result[0x3] = (uint8_t)(v->a>>24);
	result[0x4] = (uint8_t)(v->b); result[0x5] = (uint8_t)(v->b>>8); result[0x6] = (uint8_t)(v->b>>16); result[0x7] = (uint8_t)(v->b>>24);
	result[0x8] = (uint8_t)(v->c); result[0x9] = (uint8_t)(v->c>>8); result[0xa] = (uint8_t)(v->c>>16); result[0xb] = (uint8_t)(v->c>>24);
	result[0xc] = (uint8_t)(v->d); result[0xd] = (uint8_t)(v->d>>8); result[0xe] = (uint8_t)(v->d>>16); result[0xf] = (uint8_t)(v->d>>24);
}

static void ufbxt_md5(uint8_t result[16], const void *data, size_t length)
{
	ufbxt_md5_ctx ctx;
	ufbxt_md5_init(&ctx);
	ufbxt_md5_write(&ctx, (const char*)data, length);
	ufbxt_md5_finish(&ctx, result);
}

static void ufbxt_to_hex(char *dst, size_t dst_len, uint8_t *src, size_t src_len)
{
	ufbxt_assert(dst_len >= src_len * 2 + 1);
	static char hex_digits[] = "0123456789abcdef";
	size_t di = 0;
	for (size_t i = 0; i < src_len; i++) {
		unsigned s = (unsigned)src[i];
		dst[di++] = hex_digits[s >> 4];
		dst[di++] = hex_digits[s & 0xf];
	}
	dst[di] = '\0';
}

static void ufbxt_md5_hex(char *result, size_t result_len, const void *data, size_t length)
{
	uint8_t hash[16];
	ufbxt_md5(hash, data, length);
	ufbxt_to_hex(result, result_len, hash, 16);
}

static bool ufbxt_check_materials(ufbx_scene *scene, const char *spec, const char *filename)
{
	bool ok = true;

	char line_buf[512];
	const char *tokens[8];

	char dot_buf[512];
	const char *dots[8];

	bool seen_materials[256];

	ufbx_material *material = NULL;
	ufbx_display_layer *display_layer = NULL;

	size_t num_materials = 0;
	size_t num_props = 0;
	size_t num_display_layers = 0;
	size_t num_selection_sets = 0;
	size_t num_display_layer_nodes = 0;
	size_t num_pivots = 0;

	double err_sum = 0.0;
	double err_max = 0.0;
	size_t err_num = 0;
	double pivot_err_max = 0.0;

	bool material_error = false;
	bool display_layer_error = false;
	bool require_all = false;

	long version = 0;

	const long current_version = 8;

	int line = 0;
	while (*spec != '\0') {
		size_t num_tokens = ufbxt_tokenize_line(line_buf, sizeof(line_buf), tokens, ufbxt_arraycount(tokens), &spec, " \t\r");
		line++;

		if (num_tokens == 0) continue;

		const char *first_token = tokens[0];
		size_t num_dots = ufbxt_tokenize_line(dot_buf, sizeof(dot_buf), dots, ufbxt_arraycount(dots), &first_token, ".");

		if (!strcmp(dots[0], "version")) {
			char *end = NULL;
			version = strtol(tokens[1], &end, 10);
			if (!end || *end != '\0') {
				fprintf(stderr, "%s:%d: Bad value in '%s': '%s'\n", filename, line, tokens[0], tokens[1]);
				ok = false;
				continue;
			}

			if (version > current_version)
			{
				fprintf(stderr, "%s:%d: \"version %ld\" is too high for current check_material.h, maximum supported is %ld\n", filename, line,
					version, current_version);
				ok = false;
				break;
			}

			continue;
		} else if (!strcmp(dots[0], "require")) {
			if (!strcmp(tokens[1], "all")) {
				if (scene->materials.count > ufbxt_arraycount(seen_materials)) {
					fprintf(stderr, "%s:%d: Too many materials in file for 'reqiure all': %zu (max %zu)\n", filename, line,
						scene->materials.count, (size_t)ufbxt_arraycount(seen_materials));
					ok = false;
					continue;
				}
				require_all = true;
				memset(seen_materials, 0, scene->materials.count * sizeof(bool));
			} else {
				fprintf(stderr, "%s:%d: Bad require directive: '%s'\n", filename, line, tokens[1]);
				ok = false;
				continue;
			}
			continue;
		} else if (!strcmp(dots[0], "material")) {
			if (*tokens[1] == '\0') {
				fprintf(stderr, "%s:%d: Expected material name for 'material'\n", filename, line);
				ok = false;
			}

			material = ufbx_find_material(scene, tokens[1]);
			if (!material) {
				fprintf(stderr, "%s:%d: Material not found: '%s'\n", filename, line, tokens[1]);
				ok = false;
			}
			material_error = !material;

			if (material) {
				seen_materials[material->typed_id] = true;
			}

			num_materials++;
			continue;
		} else if (!strcmp(dots[0], "display_layer")) {
			if (dots[1] && !strcmp(dots[1], "node")) {
				if (*tokens[1] == '\0') {
					fprintf(stderr, "%s:%d: Expected ndoe name for 'display_layer.node'\n", filename, line);
					ok = false;
				}

				if (!display_layer) {
					if (!display_layer_error) {
						fprintf(stderr, "%s:%d: Statement '%s' needs to have a display_layer defined\n", filename, line, tokens[0]);
					}
					ok = false;
					continue;
				}

				ufbx_node *node = ufbx_find_node(scene, tokens[1]);
				if (!node) {
					fprintf(stderr, "%s:%d: display_layer.node: Could not find node '%s'\n", filename, line, tokens[1]);
					ok = false;
					continue;
				}

				bool found = false;
				for (size_t i = 0; i < display_layer->nodes.count; i++) {
					if (display_layer->nodes.data[i] == node) {
						found = true;
						break;
					}
				}
				if (!found) {
					fprintf(stderr, "%s:%d: display_layer.node: Node '%s' not found in display layer '%s'\n", filename, line, tokens[1],
						display_layer->name.data);
					ok = false;
					continue;
				}

				num_display_layer_nodes++;

			} else {
				if (*tokens[1] == '\0') {
					fprintf(stderr, "%s:%d: Expected display layer name for 'display_layer'\n", filename, line);
					ok = false;
				}

				display_layer = ufbx_as_display_layer(ufbx_find_element(scene, UFBX_ELEMENT_DISPLAY_LAYER, tokens[1]));
				if (!display_layer) {
					fprintf(stderr, "%s:%d: Display layer not found: '%s'\n", filename, line, tokens[1]);
					ok = false;
				}
				display_layer_error = !material;
				num_display_layers++;
			}

			continue;
		} else if (!strcmp(dots[0], "selection_set")) {
			if (dots[1] && !strcmp(dots[1], "faces")) {
				if (*tokens[1] == '\0') {
					fprintf(stderr, "%s:%d: Expected node name for 'selection_set.faces'\n", filename, line);
					ok = false;
					continue;
				}
				int face_amount = atoi(tokens[2]);
				if (face_amount <= 0) {
					fprintf(stderr, "%s:%d: Expected a face amount for 'selection_set.faces'\n", filename, line);
					ok = false;
					continue;
				}
				int triangle_amount = atoi(tokens[3]);
				if (triangle_amount <= 0) {
					fprintf(stderr, "%s:%d: Expected a triangle amount for 'selection_set.faces'\n", filename, line);
					ok = false;
					continue;
				}

				ufbx_node *node = ufbx_find_node(scene, tokens[1]);
				if (!node || !node->mesh) {
					fprintf(stderr, "%s:%d: Could not find node '%s' with a mesh for 'selection_set.faces'\n", filename, line, tokens[1]);
					ok = false;
					continue;
				}
				ufbx_mesh *mesh = node->mesh;

				ufbx_selection_node *sel_node = NULL;
				for (size_t sel_i = 0; sel_i < scene->selection_nodes.count; sel_i++) {
					ufbx_selection_node *sn = scene->selection_nodes.data[sel_i];
					if (sn->target_node == node && sn->target_mesh == node->mesh) {
						sel_node = sn;
						break;
					}
				}
				if (sel_node == NULL) {
					fprintf(stderr, "%s:%d: Could not find selection node for '%s'\n", filename, line, node->name.data);
					ok = false;
					continue;
				}

				size_t triangle_count = 0;
				for (size_t i = 0; i < sel_node->faces.count; i++) {
					uint32_t face_ix = sel_node->faces.data[i];
					ufbxt_assert(face_ix <= mesh->faces.count);
					triangle_count += mesh->faces.data[face_ix].num_indices - 2;
				}

				if (sel_node->faces.count != (size_t)face_amount) {
					fprintf(stderr, "%s:%d: Selection set for '%s' has the wrong amount of faces: %zu (expected %zu)\n", filename, line, node->name.data,
						sel_node->faces.count, (size_t)face_amount);
					ok = false;
				}

				if (triangle_count != (size_t)triangle_amount) {
					fprintf(stderr, "%s:%d: Selection set for '%s' has the wrong amount of triangles: %zu (expected %zu)\n", filename, line, node->name.data,
						triangle_count, (size_t)triangle_amount);
					ok = false;
				}

			} else {
				fprintf(stderr, "%s:%d: Unknown 'selection_set' specifier\n", filename, line);
				ok = false;
			}

			num_selection_sets++;
			continue;
		} else if (!strcmp(dots[0], "pivot")) {
			bool check_pivot = false;
			if (dots[1] && !strcmp(dots[1], "adjusted")) {
				check_pivot = scene->metadata.pivot_handling == UFBX_PIVOT_HANDLING_ADJUST_TO_ROTATION_PIVOT;
			} else if (dots[1] && !strcmp(dots[1], "original")) {
				check_pivot = scene->metadata.pivot_handling == UFBX_PIVOT_HANDLING_RETAIN;
			} else if (dots[1] && !strcmp(dots[1], "legacy")) {
				check_pivot = scene->metadata.pivot_handling == UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT;
			} else {
				fprintf(stderr, "%s:%d: Unknown 'pivot' specifier '%s'\n", filename, line, dots[1] ? dots[1] : "");
				ok = false;
			}

			if (check_pivot) {
				ufbx_node *node = ufbx_find_node(scene, tokens[1]);
				if (!node) {
					fprintf(stderr, "%s:%d: Pivot node not found %s\n", filename, line, tokens[1]);
					ok = false;
					continue;
				}

				ufbx_vec3 translation = node->node_to_world.cols[3];
				ufbx_vec3 ref;
				ref.x = (ufbx_real)strtod(tokens[2], NULL);
				ref.y = (ufbx_real)strtod(tokens[3], NULL);
				ref.z = (ufbx_real)strtod(tokens[4], NULL);
				double error = 0.0;
				error = fmax(error, fabs(translation.x - ref.x));
				error = fmax(error, fabs(translation.y - ref.y));
				error = fmax(error, fabs(translation.z - ref.z));
				if (error >= 0.01) {
					fprintf(stderr, "%s:%d: Pivot is incorrect (%.3f, %.3f, %.3f), expected (%.3f, %.3f, %.3f)\n", filename, line,
						translation.x, translation.y, translation.z, ref.x, ref.y, ref.z);
					ok = false;
				}

				pivot_err_max = fmax(pivot_err_max, error);
				num_pivots++;
			}

			continue;
		}

		if (!material) {
			if (!material_error) {
				fprintf(stderr, "%s:%d: Statement '%s' needs to have a material defined\n", filename, line, tokens[0]);
			}
			ok = false;
			continue;
		}

		num_props++;

		if (!strcmp(dots[0], "fbx") || !strcmp(dots[0], "pbr")) {
			ufbx_material_map *map = NULL;
			if (!strcmp(dots[0], "fbx")) {
				ufbx_material_fbx_map name = (ufbx_material_fbx_map)UFBX_MATERIAL_FBX_MAP_COUNT;
				for (size_t i = 0; i < UFBX_MATERIAL_FBX_MAP_COUNT; i++) {
					const char *str = ufbxt_fbx_map_name((ufbx_material_fbx_map)i);
					if (!strcmp(dots[1], str)) {
						name = (ufbx_material_fbx_map)i;
						break;
					}
				}
				if (name == (ufbx_material_fbx_map)UFBX_MATERIAL_FBX_MAP_COUNT) {
					fprintf(stderr, "%s:%d: Unknown FBX material map '%s' in '%s'\n", filename, line, dots[1], tokens[0]);
					ok = false;
				} else {
					map = &material->fbx.maps[name];
				}
			} else if (!strcmp(dots[0], "pbr")) {
				ufbx_material_pbr_map name = (ufbx_material_pbr_map)UFBX_MATERIAL_PBR_MAP_COUNT;
				for (size_t i = 0; i < UFBX_MATERIAL_PBR_MAP_COUNT; i++) {
					const char *str = ufbxt_pbr_map_name((ufbx_material_pbr_map)i);
					if (!strcmp(dots[1], str)) {
						name = (ufbx_material_pbr_map)i;
						break;
					}
				}
				if (name == (ufbx_material_pbr_map)UFBX_MATERIAL_PBR_MAP_COUNT) {
					fprintf(stderr, "%s:%d: Unknown PBR material map '%s' in '%s'\n", filename, line, dots[1], tokens[0]);
					ok = false;
				} else {
					map = &material->pbr.maps[name];
				}
			} else {
				ufbxt_assert(0 && "Unhandled branch");
			}

			// Errors reported above, but set ok to false just to be sure..
			if (!map) {
				ok = false;
				continue;
			}

			if (!strcmp(dots[2], "texture")) {
				size_t tok_start = 1;
				bool content = false;
				bool absolute = false;
				for (;;) {
					const char *tok = tokens[tok_start];
					if (!strcmp(tok, "content")) {
						content = true;
					} else if (!strcmp(tok, "absolute")) {
						absolute = true;
					} else {
						break;
					}
					tok_start++;
				}

				const char *tex_name = tokens[tok_start];
				if (map->texture) {
					ufbx_string tex_filename = map->texture->relative_filename;
					if (absolute) {
						tex_filename = map->texture->absolute_filename;
					}
					if (strcmp(tex_filename.data, tex_name) != 0) {
						fprintf(stderr, "%s:%d: Material '%s' %s.%s is different: got '%s', expected '%s'\n", filename, line,
							material->name.data, dots[0], dots[1], tex_filename.data, tex_name);
						ok = false;
					}
					if (content && map->texture->content.size == 0) {
						fprintf(stderr, "%s:%d: Material '%s' %s.%s expected content for the texture %s\n", filename, line,
							material->name.data, dots[0], dots[1], tex_filename.data);
						ok = false;
					}

					const char *hash = tokens[tok_start + 1];
					if (*hash) {
						if (map->texture->content.size == 0) {
							fprintf(stderr, "%s:%d: Material '%s' %s.%s expected content for the texture %s, due to checksum\n", filename, line,
								material->name.data, dots[0], dots[1], tex_filename.data);
							ok = false;
						} else {
							char md5_hex[33];
							ufbxt_md5_hex(md5_hex, sizeof(md5_hex), map->texture->content.data, map->texture->content.size);

							if (strcmp(hash, md5_hex) != 0) {
								fprintf(stderr, "%s:%d: Material '%s' %s.%s texture %s has incorrect MD5 checksum: %s (expected %s)\n", filename, line,
									material->name.data, dots[0], dots[1], tex_filename.data, md5_hex, hash);
								ok = false;
							}
						}
					}
				} else {
					fprintf(stderr, "%s:%d: Material '%s' %s.%s missing texture, expected '%s'\n", filename, line,
						material->name.data, dots[0], dots[1], tex_name);
					ok = false;
				}
			} else {
				size_t tok_start = 1;
				bool implicit = false;
				bool widen = false;
				for (;;) {
					const char *tok = tokens[tok_start];
					if (!strcmp(tok, "implicit")) {
						implicit = true;
					} else if (!strcmp(tok, "widen")) {
						widen = true;
					} else {
						break;
					}
					tok_start++;
				}

				double ref[4];
				size_t num_ref = 0;
				for (; num_ref < 4; num_ref++) {
					const char *tok = tokens[tok_start + num_ref];
					if (!*tok) break;

					char *end = NULL;
					ref[num_ref] = strtod(tok, &end);
					if (!end || *end != '\0') {
						fprintf(stderr, "%s:%d: Bad number in '%s': '%s'\n", filename, line, tokens[0], tok);
						ok = false;
						num_ref = 0;
						break;
					}
				}

				if (num_ref == 0) continue;

				if (!implicit && !map->has_value) {
					fprintf(stderr, "%s:%d: Material '%s' %s.%s not defined in material\n", filename, line,
						material->name.data, dots[0], dots[1]);
					ok = false;
					continue;
				} else if (implicit && map->has_value) {
					fprintf(stderr, "%s:%d: Material '%s' %s.%s defined in material, expected implicit\n", filename, line,
						material->name.data, dots[0], dots[1]);
					ok = false;
					continue;
				}

				if (!implicit && !widen && (size_t)map->value_components != num_ref) {
					fprintf(stderr, "%s:%d: Material '%s' %s.%s has wrong number of components: got %zu, expected %zu\n", filename, line,
						material->name.data, dots[0], dots[1], (size_t)map->value_components, num_ref);
					ok = false;
					continue;
				}

				if (widen && map->value_components >= num_ref) {
					fprintf(stderr, "%s:%d: Material '%s' %s.%s exected to be widened got %zu components, expected %zu\n", filename, line,
						material->name.data, dots[0], dots[1], (size_t)map->value_components, num_ref);
					ok = false;
					continue;
				}

				char mat_str[128];
				char ref_str[128];
				size_t mat_pos = 0;
				size_t ref_pos = 0;

				bool equal = true;
				for (size_t i = 0; i < num_ref; i++) {
					double mat_value = map->value_vec4.v[i];
					double ref_value = ref[i];
					double err = fabs(mat_value - ref_value);
					if (err > 0.002) {
						equal = false;
					}

					err_sum += err;
					if (err > err_max) err_max = err;
					err_num += 1;

					if (i > 0) {
						mat_pos += (size_t)snprintf(mat_str + mat_pos, sizeof(mat_str) - mat_pos, ", ");
						ref_pos += (size_t)snprintf(ref_str + ref_pos, sizeof(ref_str) - ref_pos, ", ");
					}

					mat_pos += (size_t)snprintf(mat_str + mat_pos, sizeof(mat_str) - mat_pos, "%.3f", mat_value);
					ref_pos += (size_t)snprintf(ref_str + ref_pos, sizeof(ref_str) - ref_pos, "%.3f", ref_value);
				}

				if (!equal) {
					fprintf(stderr, "%s:%d: Material '%s' %s.%s has wrong value: got (%s), expected (%s)\n", filename, line,
						material->name.data, dots[0], dots[1], mat_str, ref_str);
					ok = false;
				}
			}

		} else if (!strcmp(dots[0], "features")) {
			ufbx_material_feature name = (ufbx_material_feature)UFBX_MATERIAL_FEATURE_COUNT;
			for (size_t i = 0; i < UFBX_MATERIAL_FEATURE_COUNT; i++) {
				const char *str = ufbxt_material_feature_name((ufbx_material_feature)i);
				if (!strcmp(dots[1], str)) {
					name = (ufbx_material_feature)i;
					break;
				}
			}
			if (name == (ufbx_material_feature)UFBX_MATERIAL_FEATURE_COUNT) {
				fprintf(stderr, "%s:%d: Unknown material feature '%s' in '%s'\n", filename, line, dots[1], tokens[0]);
				ok = false;
			}

			ufbx_material_feature_info *feature = &material->features.features[name];

			char *end = NULL;
			long enabled = strtol(tokens[1], &end, 10);
			if (!end || *end != '\0') {
				fprintf(stderr, "%s:%d: Bad value in '%s': '%s'\n", filename, line, tokens[0], tokens[1]);
				ok = false;
				continue;
			}

			if (enabled != (long)feature->enabled) {
				fprintf(stderr, "%s:%d: Material '%s' features.%s mismatch: got %ld, expected %ld\n", filename, line,
					material->name.data, dots[1], (long)feature->enabled, enabled);
				ok = false;
				continue;
			}
		} else if (!strcmp(dots[0], "shader_type")) {
			ufbx_shader_type name = (ufbx_shader_type)UFBX_SHADER_TYPE_COUNT;
			for (size_t i = 0; i < UFBX_SHADER_TYPE_COUNT; i++) {
				const char *str = ufbxt_shader_type_name((ufbx_shader_type)i);
				if (!strcmp(tokens[1], str)) {
					name = (ufbx_shader_type)i;
					break;
				}
			}
			if (name == (ufbx_shader_type)UFBX_SHADER_TYPE_COUNT) {
				fprintf(stderr, "%s:%d: Unknown shader type '%s' in '%s'\n", filename, line, tokens[1], tokens[0]);
				ok = false;
			}

			if (material->shader_type != name) {
				const char *mat_name = ufbxt_shader_type_name(material->shader_type);
				fprintf(stderr, "%s:%d: Material '%s' shader type mismatch: got '%s', expected '%s'\n", filename, line,
					material->name.data, mat_name, tokens[1]);
			}
		} else {
			fprintf(stderr, "%s:%d: Invalid token '%s'\n", filename, line, tokens[0]);
			ok = false;
		}
	}

	if (require_all) {
		for (size_t i = 0; i < scene->materials.count; i++) {
			if (!seen_materials[i]) {
				fprintf(stderr, "%s: 'require all': Material in FBX not described: '%s'\n", filename,
					scene->materials.data[i]->name.data);
				ok = false;
			}
		}
	}

	if (ok) {
		double avg = err_num > 0 ? err_sum / (double)err_num : 0.0;
		printf("Checked %zu materials, %zu properties (error avg %.3g, max %.3g, %zu tests)\n", num_materials, num_props, avg, err_max, err_num);
		if (num_display_layers > 0) {
			printf(".. also %zu display layers with %zu nodes\n", num_display_layers, num_display_layer_nodes);
		}
		if (num_selection_sets > 0) {
			printf(".. also %zu selection sets\n", num_selection_sets);
		}
		if (num_pivots > 0) {
			printf(".. also %zu pivots (max error %.3g)\n", num_pivots, pivot_err_max);
		}
	}

	return ok;
}

#endif
