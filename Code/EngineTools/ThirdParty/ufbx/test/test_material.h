#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "material"

#if UFBXT_IMPL

void ufbxt_check_blob_content(ufbx_blob blob, const char *filename)
{
	char buf[512];

	ufbxt_assert(blob.size > 0);
	ufbxt_assert(blob.data);

	snprintf(buf, sizeof(buf), "%s%s", data_root, filename);
	void *ref = malloc(blob.size + 1);
	ufbxt_assert(ref);

	FILE *f = fopen(buf, "rb");
	ufbxt_assert(f);
	size_t num_read = fread(ref, 1, blob.size + 1, f);
	fclose(f);

	ufbxt_assert(num_read == blob.size);
	ufbxt_assert(!memcmp(ref, blob.data, blob.size));

	free(ref);
}

void ufbxt_check_texture_content(ufbx_scene *scene, ufbx_texture *texture, const char *filename)
{
	char buf[512];
	snprintf(buf, sizeof(buf), "textures/%s", filename);
	ufbxt_assert(texture);
	ufbxt_check_blob_content(texture->content, buf);
}

void ufbxt_check_material_texture_ex(ufbx_scene *scene, ufbx_texture *texture, const char *directory, const char *filename, bool require_content)
{
	char buf[512];

	snprintf(buf, sizeof(buf), "%s%s", directory, filename);
	ufbxt_assert(!strcmp(texture->relative_filename.data, buf));

	if (require_content && (scene->metadata.version >= 7000 || !scene->metadata.ascii)) {
		ufbxt_assert(texture->content.size);
	}

	if (texture->content.size) {
		ufbxt_check_texture_content(scene, texture, filename);
	}
}

void ufbxt_check_material_texture(ufbx_scene *scene, ufbx_texture *texture, const char *filename, bool require_content)
{
	ufbxt_check_material_texture_ex(scene, texture, "textures\\", filename, require_content);
}

#endif

UFBXT_FILE_TEST(maya_textured_cube)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "phong1");
	ufbxt_assert(material);
	ufbxt_assert(material->textures.count == 6);

	ufbxt_check_material_texture(scene, material->fbx.diffuse_color.texture, "checkerboard_diffuse.png", true);
	ufbxt_check_material_texture(scene, material->fbx.specular_color.texture, "checkerboard_specular.png", true);
	ufbxt_check_material_texture(scene, material->fbx.reflection_color.texture, "checkerboard_reflection.png", true);
	ufbxt_check_material_texture(scene, material->fbx.transparency_color.texture, "checkerboard_transparency.png", true);
	ufbxt_check_material_texture(scene, material->fbx.emission_color.texture, "checkerboard_emissive.png", true);
	ufbxt_check_material_texture(scene, material->fbx.ambient_color.texture, "checkerboard_ambient.png", true);
}
#endif

UFBXT_FILE_TEST_FLAGS(synthetic_texture_split, UFBXT_FILE_TEST_FLAG_ALLOW_WARNINGS)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "phong1");
	ufbxt_assert(material);
	ufbxt_assert(material->textures.count == 6);

	ufbxt_check_material_texture(scene, material->fbx.diffuse_color.texture, "checkerboard_diffuse.png", true);
	ufbxt_check_material_texture(scene, material->fbx.specular_color.texture, "checkerboard_specular.png", true);
	ufbxt_check_material_texture(scene, material->fbx.reflection_color.texture, "checkerboard_reflection.png", false);
	ufbxt_check_material_texture(scene, material->fbx.transparency_color.texture, "checkerboard_transparency.png", false);
	ufbxt_check_material_texture(scene, material->fbx.emission_color.texture, "checkerboard_emissive.png", true);
	ufbxt_check_material_texture(scene, material->fbx.ambient_color.texture, "checkerboard_ambient.png", true);

	if (scene->metadata.ascii) {
		ufbxt_check_warning(scene, UFBX_WARNING_BAD_BASE64_CONTENT, UFBX_ELEMENT_VIDEO, "file2", 1, NULL);
		ufbxt_check_warning(scene, UFBX_WARNING_BAD_BASE64_CONTENT, UFBX_ELEMENT_VIDEO, "file6", 1, NULL);
	}
}
#endif

UFBXT_TEST(ignore_embedded)
#if UFBXT_IMPL
{
	char path[512];

	ufbxt_file_iterator iter = { "maya_textured_cube" };
	while (ufbxt_next_file(&iter, path, sizeof(path))) {
		ufbx_load_opts opts = { 0 };
		opts.ignore_embedded = true;

		ufbx_scene *scene = ufbx_load_file(path, &opts, NULL);
		ufbxt_assert(scene);
		ufbxt_check_scene(scene);
		
		for (size_t i = 0; i < scene->videos.count; i++) {
			ufbxt_assert(scene->videos.data[i]->content.data == NULL);
			ufbxt_assert(scene->videos.data[i]->content.size == 0);
		}
		
		for (size_t i = 0; i < scene->textures.count; i++) {
			ufbxt_assert(scene->textures.data[i]->content.data == NULL);
			ufbxt_assert(scene->textures.data[i]->content.size == 0);
		}

		ufbx_free_scene(scene);
	}
}
#endif

UFBXT_FILE_TEST(maya_shared_textures)
#if UFBXT_IMPL
{
	ufbx_material *material;

	material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Shared");
	ufbxt_assert(material);
	ufbxt_assert(material->textures.count == 6);

	ufbxt_assert(material->shader_type == UFBX_SHADER_FBX_LAMBERT);
	ufbxt_check_material_texture(scene, material->fbx.diffuse_color.texture, "checkerboard_ambient.png", true); // sic: test has wrong texture
	ufbxt_check_material_texture(scene, material->fbx.diffuse_factor.texture, "checkerboard_diffuse.png", true);
	ufbxt_check_material_texture(scene, material->fbx.emission_color.texture, "checkerboard_emissive.png", true);
	ufbxt_check_material_texture(scene, material->fbx.ambient_color.texture, "checkerboard_ambient.png", true);
	ufbxt_check_material_texture(scene, material->fbx.transparency_color.texture, "checkerboard_transparency.png", true);
	ufbxt_check_material_texture(scene, material->fbx.bump.texture, "checkerboard_bump.png", true);

	material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Special");
	ufbxt_assert(material->shader_type == UFBX_SHADER_FBX_PHONG);
	ufbxt_assert(material);
	ufbxt_assert(material->textures.count == 1);

	ufbxt_assert(!strcmp(material->fbx.diffuse_color.texture->relative_filename.data, "textures\\tiny_clouds.png"));
}
#endif

UFBXT_FILE_TEST(maya_arnold_textures)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "aiStandardSurface1");
	ufbxt_assert(material);
	ufbxt_assert(material->textures.count == 5);

	ufbxt_assert(material->shader_type == UFBX_SHADER_ARNOLD_STANDARD_SURFACE);
	ufbxt_check_material_texture(scene, material->pbr.base_color.texture, "checkerboard_diffuse.png", true);
	ufbxt_check_material_texture(scene, material->pbr.specular_color.texture, "checkerboard_specular.png", true);
	ufbxt_check_material_texture(scene, material->pbr.roughness.texture, "checkerboard_roughness.png", true);
	ufbxt_check_material_texture(scene, material->pbr.metalness.texture, "checkerboard_metallic.png", true);
	ufbxt_check_material_texture(scene, material->pbr.diffuse_roughness.texture, "checkerboard_roughness.png", true);
}
#endif

UFBXT_FILE_TEST(max_physical_material_properties)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "PhysicalMaterial");
	ufbxt_assert(material);

	ufbxt_assert(material->shader_type == UFBX_SHADER_3DS_MAX_PHYSICAL_MATERIAL);

	ufbxt_assert( 1 == (int)round(100.0f * material->pbr.base_factor.value_real));
	ufbxt_assert( 2 == (int)round(100.0f * material->pbr.base_color.value_vec4.x));
	ufbxt_assert( 3 == (int)round(100.0f * material->pbr.base_color.value_vec4.y));
	ufbxt_assert( 4 == (int)round(100.0f * material->pbr.base_color.value_vec4.z));
	ufbxt_assert( 5 == (int)round(100.0f * material->pbr.base_color.value_vec4.w));
	ufbxt_assert( 6 == (int)round(100.0f * material->pbr.roughness.value_real));
	ufbxt_assert( 7 == (int)round(100.0f * material->pbr.metalness.value_real));
	ufbxt_assert(80 == (int)round(100.0f * material->pbr.specular_ior.value_real)); // 3ds Max doesn't allow lower than 0.1 IOR
	ufbxt_assert( 9 == (int)round(100.0f * material->pbr.transmission_factor.value_real));
	ufbxt_assert(10 == (int)round(100.0f * material->pbr.transmission_color.value_vec4.x));
	ufbxt_assert(11 == (int)round(100.0f * material->pbr.transmission_color.value_vec4.y));
	ufbxt_assert(12 == (int)round(100.0f * material->pbr.transmission_color.value_vec4.z));
	ufbxt_assert(13 == (int)round(100.0f * material->pbr.transmission_color.value_vec4.w));
	ufbxt_assert(14 == (int)round(100.0f * material->pbr.transmission_roughness.value_real));
	ufbxt_assert( 6 == (int)round(100.0f * material->pbr.transmission_depth.value_real)); // ??? Unit conversion probably
	ufbxt_assert(16 == (int)round(100.0f * material->pbr.subsurface_factor.value_real));
	ufbxt_assert(17 == (int)round(100.0f * material->pbr.subsurface_tint_color.value_vec4.x));
	ufbxt_assert(18 == (int)round(100.0f * material->pbr.subsurface_tint_color.value_vec4.y));
	ufbxt_assert(19 == (int)round(100.0f * material->pbr.subsurface_tint_color.value_vec4.z));
	ufbxt_assert(20 == (int)round(100.0f * material->pbr.subsurface_tint_color.value_vec4.w));
	ufbxt_assert(21 == (int)round(100.0f * material->pbr.subsurface_color.value_vec4.x));
	ufbxt_assert(22 == (int)round(100.0f * material->pbr.subsurface_color.value_vec4.y));
	ufbxt_assert(23 == (int)round(100.0f * material->pbr.subsurface_color.value_vec4.z));
	ufbxt_assert(24 == (int)round(100.0f * material->pbr.subsurface_color.value_vec4.w));
	ufbxt_assert(10 == (int)round(100.0f * material->pbr.subsurface_radius.value_real)); // ??? Unit conversion probably
	ufbxt_assert(26 == (int)round(100.0f * material->pbr.subsurface_scale.value_real));
	ufbxt_assert(27 == (int)round(100.0f * material->pbr.emission_factor.value_real));
	ufbxt_assert(28 == (int)round(100.0f * material->pbr.emission_color.value_vec4.x));
	ufbxt_assert(29 == (int)round(100.0f * material->pbr.emission_color.value_vec4.y));
	ufbxt_assert(30 == (int)round(100.0f * material->pbr.emission_color.value_vec4.z));
	ufbxt_assert(31 == (int)round(100.0f * material->pbr.emission_color.value_vec4.w));
	ufbxt_assert(32 == (int)round(100.0f * material->pbr.specular_anisotropy.value_real));
	ufbxt_assert(33 == (int)round(100.0f * material->pbr.specular_rotation.value_real));
	ufbxt_assert(34 == (int)round(100.0f * material->pbr.normal_map.value_real));
	ufbxt_assert(35 == (int)round(100.0f * material->pbr.coat_normal.value_real));
	ufbxt_assert(36 == (int)round(100.0f * material->pbr.displacement_map.value_real));
	ufbxt_assert(37 == (int)round(100.0f * material->pbr.diffuse_roughness.value_real));
	ufbxt_assert(38 == (int)round(100.0f * material->pbr.coat_factor.value_real));
	ufbxt_assert(39 == (int)round(100.0f * material->pbr.coat_color.value_vec4.x));
	ufbxt_assert(40 == (int)round(100.0f * material->pbr.coat_color.value_vec4.y));
	ufbxt_assert(41 == (int)round(100.0f * material->pbr.coat_color.value_vec4.z));
	ufbxt_assert(42 == (int)round(100.0f * material->pbr.coat_color.value_vec4.w));
	ufbxt_assert(43 == (int)round(100.0f * material->pbr.coat_roughness.value_real));
	ufbxt_assert(44 == (int)round(100.0f * material->pbr.coat_ior.value_real));
	ufbxt_assert(45 == (int)round(100.0f * material->pbr.coat_affect_base_color.value_real));
	ufbxt_assert(46 == (int)round(100.0f * material->pbr.coat_affect_base_roughness.value_real));

	ufbxt_assert(material->pbr.base_factor.texture_enabled);
	ufbxt_assert(material->pbr.base_color.texture_enabled);
	ufbxt_assert(material->pbr.specular_factor.texture_enabled);
	ufbxt_assert(material->pbr.specular_color.texture_enabled);
	ufbxt_assert(material->pbr.roughness.texture_enabled);
	ufbxt_assert(material->pbr.metalness.texture_enabled);
	ufbxt_assert(material->pbr.diffuse_roughness.texture_enabled);
	ufbxt_assert(material->pbr.specular_anisotropy.texture_enabled);
	ufbxt_assert(material->pbr.specular_rotation.texture_enabled);
	ufbxt_assert(material->pbr.transmission_factor.texture_enabled);
	ufbxt_assert(material->pbr.transmission_color.texture_enabled);
	ufbxt_assert(material->pbr.transmission_roughness.texture_enabled);
	ufbxt_assert(material->pbr.specular_ior.texture_enabled);
	ufbxt_assert(material->pbr.subsurface_factor.texture_enabled);
	ufbxt_assert(material->pbr.subsurface_tint_color.texture_enabled);
	ufbxt_assert(material->pbr.subsurface_scale.texture_enabled);
	ufbxt_assert(material->pbr.emission_factor.texture_enabled);
	ufbxt_assert(material->pbr.emission_color.texture_enabled);
	ufbxt_assert(material->pbr.coat_factor.texture_enabled);
	ufbxt_assert(material->pbr.coat_color.texture_enabled);
	ufbxt_assert(material->pbr.coat_roughness.texture_enabled);
	ufbxt_assert(material->pbr.normal_map.texture_enabled);
	ufbxt_assert(material->pbr.coat_normal.texture_enabled);
	ufbxt_assert(material->pbr.displacement_map.texture_enabled);
	ufbxt_assert(material->pbr.opacity.texture_enabled);

	ufbxt_assert(material->pbr.base_factor.value_components == 1);
	ufbxt_assert(material->pbr.base_color.value_components == 4);
	ufbxt_assert(material->pbr.specular_factor.value_components == 1);
	ufbxt_assert(material->pbr.specular_color.value_components == 4);
	ufbxt_assert(material->pbr.roughness.value_components == 1);
	ufbxt_assert(material->pbr.metalness.value_components == 1);
	ufbxt_assert(material->pbr.diffuse_roughness.value_components == 1);
	ufbxt_assert(material->pbr.specular_anisotropy.value_components == 1);
	ufbxt_assert(material->pbr.specular_rotation.value_components == 1);
	ufbxt_assert(material->pbr.transmission_factor.value_components == 1);
	ufbxt_assert(material->pbr.transmission_color.value_components == 4);
	ufbxt_assert(material->pbr.transmission_roughness.value_components == 1);
	ufbxt_assert(material->pbr.transmission_extra_roughness.value_components == 0);
	ufbxt_assert(material->pbr.specular_ior.value_components == 1);
	ufbxt_assert(material->pbr.subsurface_factor.value_components == 1);
	ufbxt_assert(material->pbr.subsurface_tint_color.value_components == 4);
	ufbxt_assert(material->pbr.subsurface_color.value_components == 4);
	ufbxt_assert(material->pbr.subsurface_radius.value_components == 1);
	ufbxt_assert(material->pbr.subsurface_scale.value_components == 1);
	ufbxt_assert(material->pbr.emission_factor.value_components == 1);
	ufbxt_assert(material->pbr.emission_color.value_components == 4);
	ufbxt_assert(material->pbr.coat_factor.value_components == 1);
	ufbxt_assert(material->pbr.coat_color.value_components == 4);
	ufbxt_assert(material->pbr.coat_roughness.value_components == 1);
	ufbxt_assert(material->pbr.normal_map.value_components == 1);
	ufbxt_assert(material->pbr.coat_normal.value_components == 1);
	ufbxt_assert(material->pbr.displacement_map.value_components == 1);
	ufbxt_assert(material->pbr.opacity.value_components == 0);
}
#endif

UFBXT_FILE_TEST(max_physical_material_inverted)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "InvertedMaterial");
	ufbxt_assert(material);

	ufbxt_assert(material->shader_type == UFBX_SHADER_3DS_MAX_PHYSICAL_MATERIAL);

	ufbxt_assert_close_real(err, material->pbr.roughness.value_real, 0.9f);
	ufbxt_assert_close_real(err, material->pbr.transmission_roughness.value_real, 0.8f);
	ufbxt_assert_close_real(err, material->pbr.coat_roughness.value_real, 0.7f);
}
#endif

UFBXT_FILE_TEST(max_physical_material_textures)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "PhysicalMaterial");
	ufbxt_assert(material);

	ufbxt_assert(material->shader_type == UFBX_SHADER_3DS_MAX_PHYSICAL_MATERIAL);
	ufbxt_check_material_texture(scene, material->pbr.base_factor.texture, "checkerboard_weight.png", true);
	ufbxt_check_material_texture(scene, material->pbr.base_color.texture, "checkerboard_diffuse.png", true);
	ufbxt_check_material_texture(scene, material->pbr.specular_factor.texture, "checkerboard_reflection.png", true);
	ufbxt_check_material_texture(scene, material->pbr.specular_color.texture, "checkerboard_reflection.png", true);
	ufbxt_check_material_texture(scene, material->pbr.roughness.texture, "checkerboard_roughness.png", true);
	ufbxt_check_material_texture(scene, material->pbr.metalness.texture, "checkerboard_metallic.png", true);
	ufbxt_check_material_texture(scene, material->pbr.diffuse_roughness.texture, "checkerboard_roughness.png", true);
	ufbxt_check_material_texture(scene, material->pbr.specular_anisotropy.texture, "checkerboard_weight.png", true);
	ufbxt_check_material_texture(scene, material->pbr.specular_rotation.texture, "checkerboard_weight.png", true);
	ufbxt_check_material_texture(scene, material->pbr.transmission_factor.texture, "checkerboard_transparency.png", true);
	ufbxt_check_material_texture(scene, material->pbr.transmission_color.texture, "checkerboard_transparency.png", true);
	ufbxt_check_material_texture(scene, material->pbr.transmission_roughness.texture, "checkerboard_roughness.png", true);
	ufbxt_check_material_texture(scene, material->pbr.specular_ior.texture, "checkerboard_weight.png", true);
	ufbxt_check_material_texture(scene, material->pbr.subsurface_factor.texture, "checkerboard_weight.png", true);
	ufbxt_check_material_texture(scene, material->pbr.subsurface_tint_color.texture, "checkerboard_weight.png", true);
	ufbxt_check_material_texture(scene, material->pbr.subsurface_scale.texture, "checkerboard_weight.png", true);
	ufbxt_check_material_texture(scene, material->pbr.emission_factor.texture, "checkerboard_emissive.png", true);
	ufbxt_check_material_texture(scene, material->pbr.emission_color.texture, "checkerboard_emissive.png", true);
	ufbxt_check_material_texture(scene, material->pbr.coat_factor.texture, "checkerboard_specular.png", true);
	ufbxt_check_material_texture(scene, material->pbr.coat_color.texture, "checkerboard_specular.png", true);
	ufbxt_check_material_texture(scene, material->pbr.coat_roughness.texture, "checkerboard_specular.png", true);
	ufbxt_check_material_texture(scene, material->pbr.normal_map.texture, "checkerboard_bump.png", true);
	ufbxt_check_material_texture(scene, material->pbr.coat_normal.texture, "checkerboard_bump.png", true);
	ufbxt_check_material_texture(scene, material->pbr.displacement_map.texture, "checkerboard_displacement.png", true);
	ufbxt_check_material_texture(scene, material->pbr.opacity.texture, "checkerboard_transparency.png", true);

}
#endif

UFBXT_FILE_TEST(max_pbr_metal_rough_material)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "01 - Default");
	ufbxt_assert(material);

	ufbxt_assert(material->shader_type == UFBX_SHADER_3DS_MAX_PBR_METAL_ROUGH);

	ufbxt_assert( 1 == (int)round(100.0f * material->pbr.base_color.value_vec4.x));
	ufbxt_assert( 2 == (int)round(100.0f * material->pbr.base_color.value_vec4.y));
	ufbxt_assert( 3 == (int)round(100.0f * material->pbr.base_color.value_vec4.z));
	ufbxt_assert( 4 == (int)round(100.0f * material->pbr.base_color.value_vec4.w));
	ufbxt_assert( 5 == (int)round(100.0f * material->pbr.metalness.value_real));
	ufbxt_assert( 6 == (int)round(100.0f * material->pbr.roughness.value_real));
	// ufbxt_assert( 7 == (int)round(100.0f * material->pbr.normal_map.value_real)); (not in file?!)
	ufbxt_assert( 8 == (int)round(100.0f * material->pbr.emission_color.value_vec4.x));
	ufbxt_assert( 9 == (int)round(100.0f * material->pbr.emission_color.value_vec4.y));
	ufbxt_assert(10 == (int)round(100.0f * material->pbr.emission_color.value_vec4.z));
	ufbxt_assert(11 == (int)round(100.0f * material->pbr.emission_color.value_vec4.w));
	ufbxt_assert(12 == (int)round(100.0f * material->pbr.displacement_map.value_real));

	ufbxt_assert_close_real(err, material->pbr.base_factor.value_real, 1.0f);
	ufbxt_assert_close_real(err, material->pbr.emission_factor.value_real, 1.0f);

	ufbxt_check_material_texture(scene, material->pbr.base_color.texture, "checkerboard_diffuse.png", false);
	ufbxt_check_material_texture(scene, material->pbr.metalness.texture, "checkerboard_metallic.png", false);
	ufbxt_check_material_texture(scene, material->pbr.roughness.texture, "checkerboard_roughness.png", false);
	ufbxt_check_material_texture(scene, material->pbr.ambient_occlusion.texture, "checkerboard_ambient.png", false);
	ufbxt_check_material_texture(scene, material->pbr.normal_map.texture, "checkerboard_normal.png", false);
	ufbxt_check_material_texture(scene, material->pbr.emission_color.texture, "checkerboard_emissive.png", false);
	ufbxt_check_material_texture(scene, material->pbr.displacement_map.texture, "checkerboard_displacement.png", false);
	ufbxt_check_material_texture(scene, material->pbr.opacity.texture, "checkerboard_transparency.png", false);

	ufbxt_assert(material->features.metalness.enabled);
	ufbxt_assert(!material->features.specular.enabled);
}
#endif

UFBXT_FILE_TEST(max_pbr_metal_gloss_material)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "01 - Default");
	ufbxt_assert(material);

	ufbxt_assert(material->shader_type == UFBX_SHADER_3DS_MAX_PBR_METAL_ROUGH);

	ufbxt_assert( 1 == (int)round(100.0f * material->pbr.base_color.value_vec4.x));
	ufbxt_assert( 2 == (int)round(100.0f * material->pbr.base_color.value_vec4.y));
	ufbxt_assert( 3 == (int)round(100.0f * material->pbr.base_color.value_vec4.z));
	ufbxt_assert( 4 == (int)round(100.0f * material->pbr.base_color.value_vec4.w));
	ufbxt_assert( 5 == (int)round(100.0f * material->pbr.metalness.value_real));
	ufbxt_assert( 6 == (int)round(100.0f * material->pbr.glossiness.value_real));
	// ufbxt_assert( 7 == (int)round(100.0f * material->pbr.normal_map.value_real)); (not in file?!)
	ufbxt_assert( 8 == (int)round(100.0f * material->pbr.emission_color.value_vec4.x));
	ufbxt_assert( 9 == (int)round(100.0f * material->pbr.emission_color.value_vec4.y));
	ufbxt_assert(10 == (int)round(100.0f * material->pbr.emission_color.value_vec4.z));
	ufbxt_assert(11 == (int)round(100.0f * material->pbr.emission_color.value_vec4.w));
	ufbxt_assert(12 == (int)round(100.0f * material->pbr.displacement_map.value_real));

	ufbxt_assert(100 - 6 == (int)round(100.0f * material->pbr.roughness.value_real));

	ufbxt_assert_close_real(err, material->pbr.base_factor.value_real, 1.0f);
	ufbxt_assert_close_real(err, material->pbr.emission_factor.value_real, 1.0f);

	ufbxt_check_material_texture(scene, material->pbr.base_color.texture, "checkerboard_diffuse.png", false);
	ufbxt_check_material_texture(scene, material->pbr.metalness.texture, "checkerboard_metallic.png", false);
	ufbxt_check_material_texture(scene, material->pbr.glossiness.texture, "checkerboard_roughness.png", false);
	ufbxt_check_material_texture(scene, material->pbr.ambient_occlusion.texture, "checkerboard_ambient.png", false);
	ufbxt_check_material_texture(scene, material->pbr.normal_map.texture, "checkerboard_normal.png", false);
	ufbxt_check_material_texture(scene, material->pbr.emission_color.texture, "checkerboard_emissive.png", false);
	ufbxt_check_material_texture(scene, material->pbr.displacement_map.texture, "checkerboard_displacement.png", false);
	ufbxt_check_material_texture(scene, material->pbr.opacity.texture, "checkerboard_transparency.png", false);

	ufbxt_assert(material->pbr.roughness.texture == NULL);

	ufbxt_assert(material->features.metalness.enabled);
	ufbxt_assert(!material->features.specular.enabled);
}
#endif

UFBXT_FILE_TEST(max_pbr_spec_rough_material)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "02 - Default");
	ufbxt_assert(material);

	ufbxt_assert(material->shader_type == UFBX_SHADER_3DS_MAX_PBR_SPEC_GLOSS);

	ufbxt_assert( 1 == (int)round(100.0f * material->pbr.base_color.value_vec4.x));
	ufbxt_assert( 2 == (int)round(100.0f * material->pbr.base_color.value_vec4.y));
	ufbxt_assert( 3 == (int)round(100.0f * material->pbr.base_color.value_vec4.z));
	ufbxt_assert( 4 == (int)round(100.0f * material->pbr.base_color.value_vec4.w));
	ufbxt_assert( 5 == (int)round(100.0f * material->pbr.specular_color.value_vec4.x));
	ufbxt_assert( 6 == (int)round(100.0f * material->pbr.specular_color.value_vec4.y));
	ufbxt_assert( 7 == (int)round(100.0f * material->pbr.specular_color.value_vec4.z));
	ufbxt_assert( 8 == (int)round(100.0f * material->pbr.specular_color.value_vec4.w));
	ufbxt_assert( 9 == (int)round(100.0f * material->pbr.roughness.value_real));
	// ufbxt_assert(10 == (int)round(100.0f * material->pbr.normal_map.value_real)); (not in file?!)
	ufbxt_assert(12 == (int)round(100.0f * material->pbr.emission_color.value_vec4.x));
	ufbxt_assert(13 == (int)round(100.0f * material->pbr.emission_color.value_vec4.y));
	ufbxt_assert(14 == (int)round(100.0f * material->pbr.emission_color.value_vec4.z));
	ufbxt_assert(15 == (int)round(100.0f * material->pbr.emission_color.value_vec4.w));
	ufbxt_assert(16 == (int)round(100.0f * material->pbr.displacement_map.value_real));

	ufbxt_assert_close_real(err, material->pbr.base_factor.value_real, 1.0f);
	ufbxt_assert_close_real(err, material->pbr.emission_factor.value_real, 1.0f);

	ufbxt_check_material_texture(scene, material->pbr.base_color.texture, "checkerboard_diffuse.png", false);
	ufbxt_check_material_texture(scene, material->pbr.specular_color.texture, "checkerboard_specular.png", false);
	ufbxt_check_material_texture(scene, material->pbr.roughness.texture, "checkerboard_weight.png", false);
	ufbxt_check_material_texture(scene, material->pbr.ambient_occlusion.texture, "checkerboard_ambient.png", false);
	ufbxt_check_material_texture(scene, material->pbr.normal_map.texture, "checkerboard_normal.png", false);
	ufbxt_check_material_texture(scene, material->pbr.emission_color.texture, "checkerboard_emissive.png", false);
	ufbxt_check_material_texture(scene, material->pbr.displacement_map.texture, "checkerboard_displacement.png", false);
	ufbxt_check_material_texture(scene, material->pbr.opacity.texture, "checkerboard_transparency.png", false);

	ufbxt_assert(!material->features.metalness.enabled);
	ufbxt_assert(material->features.specular.enabled);
}
#endif

UFBXT_FILE_TEST(max_pbr_spec_gloss_material)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "02 - Default");
	ufbxt_assert(material);

	ufbxt_assert(material->shader_type == UFBX_SHADER_3DS_MAX_PBR_SPEC_GLOSS);

	ufbxt_assert( 1 == (int)round(100.0f * material->pbr.base_color.value_vec4.x));
	ufbxt_assert( 2 == (int)round(100.0f * material->pbr.base_color.value_vec4.y));
	ufbxt_assert( 3 == (int)round(100.0f * material->pbr.base_color.value_vec4.z));
	ufbxt_assert( 4 == (int)round(100.0f * material->pbr.base_color.value_vec4.w));
	ufbxt_assert( 5 == (int)round(100.0f * material->pbr.specular_color.value_vec4.x));
	ufbxt_assert( 6 == (int)round(100.0f * material->pbr.specular_color.value_vec4.y));
	ufbxt_assert( 7 == (int)round(100.0f * material->pbr.specular_color.value_vec4.z));
	ufbxt_assert( 8 == (int)round(100.0f * material->pbr.specular_color.value_vec4.w));
	ufbxt_assert( 9 == (int)round(100.0f * material->pbr.glossiness.value_real));
	// ufbxt_assert(10 == (int)round(100.0f * material->pbr.normal_map.value_real)); (not in file?!)
	ufbxt_assert(12 == (int)round(100.0f * material->pbr.emission_color.value_vec4.x));
	ufbxt_assert(13 == (int)round(100.0f * material->pbr.emission_color.value_vec4.y));
	ufbxt_assert(14 == (int)round(100.0f * material->pbr.emission_color.value_vec4.z));
	ufbxt_assert(15 == (int)round(100.0f * material->pbr.emission_color.value_vec4.w));
	ufbxt_assert(16 == (int)round(100.0f * material->pbr.displacement_map.value_real));

	ufbxt_assert(100 - 9 == (int)round(100.0f * material->pbr.roughness.value_real));

	ufbxt_assert_close_real(err, material->pbr.base_factor.value_real, 1.0f);
	ufbxt_assert_close_real(err, material->pbr.emission_factor.value_real, 1.0f);

	ufbxt_check_material_texture(scene, material->pbr.base_color.texture, "checkerboard_diffuse.png", false);
	ufbxt_check_material_texture(scene, material->pbr.specular_color.texture, "checkerboard_specular.png", false);
	ufbxt_check_material_texture(scene, material->pbr.glossiness.texture, "checkerboard_weight.png", false);
	ufbxt_check_material_texture(scene, material->pbr.ambient_occlusion.texture, "checkerboard_ambient.png", false);
	ufbxt_check_material_texture(scene, material->pbr.normal_map.texture, "checkerboard_normal.png", false);
	ufbxt_check_material_texture(scene, material->pbr.emission_color.texture, "checkerboard_emissive.png", false);
	ufbxt_check_material_texture(scene, material->pbr.displacement_map.texture, "checkerboard_displacement.png", false);
	ufbxt_check_material_texture(scene, material->pbr.opacity.texture, "checkerboard_transparency.png", false);

	ufbxt_assert(material->pbr.roughness.texture == NULL);

	ufbxt_assert(!material->features.metalness.enabled);
	ufbxt_assert(material->features.specular.enabled);
}
#endif

UFBXT_FILE_TEST(max_gltf_material)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "01 - Default");
	ufbxt_assert(material);

	ufbxt_assert(material->shader_type == UFBX_SHADER_GLTF_MATERIAL);

	ufbxt_assert( 1 == (int)round(100.0f * material->pbr.base_color.value_vec4.x));
	ufbxt_assert( 2 == (int)round(100.0f * material->pbr.base_color.value_vec4.y));
	ufbxt_assert( 3 == (int)round(100.0f * material->pbr.base_color.value_vec4.z));
	ufbxt_assert( 4 == (int)round(100.0f * material->pbr.base_color.value_vec4.w));
	ufbxt_assert( 5 == (int)round(100.0f * material->pbr.metalness.value_real));
	ufbxt_assert( 6 == (int)round(100.0f * material->pbr.roughness.value_real));
	ufbxt_assert( 7 == (int)round(100.0f * material->pbr.normal_map.value_real));
	ufbxt_assert( 8 == (int)round(100.0f * material->pbr.ambient_occlusion.value_real));
	ufbxt_assert( 9 == (int)round(100.0f * material->pbr.emission_color.value_vec4.x));
	ufbxt_assert(10 == (int)round(100.0f * material->pbr.emission_color.value_vec4.y));
	ufbxt_assert(11 == (int)round(100.0f * material->pbr.emission_color.value_vec4.z));
	ufbxt_assert(12 == (int)round(100.0f * material->pbr.emission_color.value_vec4.w));
	ufbxt_assert(13 == (int)round(100.0f * material->pbr.coat_factor.value_real));
	ufbxt_assert(14 == (int)round(100.0f * material->pbr.coat_roughness.value_real));
	ufbxt_assert(15 == (int)round(100.0f * material->pbr.coat_normal.value_real));
	ufbxt_assert(16 == (int)round(100.0f * material->pbr.sheen_color.value_vec4.x));
	ufbxt_assert(17 == (int)round(100.0f * material->pbr.sheen_color.value_vec4.y));
	ufbxt_assert(18 == (int)round(100.0f * material->pbr.sheen_color.value_vec4.z));
	ufbxt_assert(19 == (int)round(100.0f * material->pbr.sheen_color.value_vec4.w));
	ufbxt_assert(20 == (int)round(100.0f * material->pbr.sheen_roughness.value_real));
	ufbxt_assert(21 == (int)round(100.0f * material->pbr.specular_factor.value_real));
	ufbxt_assert(22 == (int)round(100.0f * material->pbr.specular_color.value_vec4.x));
	ufbxt_assert(23 == (int)round(100.0f * material->pbr.specular_color.value_vec4.y));
	ufbxt_assert(24 == (int)round(100.0f * material->pbr.specular_color.value_vec4.z));
	ufbxt_assert(25 == (int)round(100.0f * material->pbr.specular_color.value_vec4.w));
	ufbxt_assert(26 == (int)round(100.0f * material->pbr.transmission_factor.value_real));
	// TODO? Volume
	ufbxt_assert(133 == (int)round(100.0f * material->pbr.specular_ior.value_real));

	ufbxt_assert_close_real(err, material->pbr.base_factor.value_real, 1.0f);
	ufbxt_assert_close_real(err, material->pbr.emission_factor.value_real, 1.0f);

	ufbxt_check_material_texture(scene, material->pbr.base_color.texture, "checkerboard_diffuse.png", true);
	ufbxt_check_material_texture(scene, material->pbr.opacity.texture, "checkerboard_transparency.png", true);
	ufbxt_check_material_texture(scene, material->pbr.metalness.texture, "checkerboard_metallic.png", true);
	ufbxt_check_material_texture(scene, material->pbr.roughness.texture, "checkerboard_roughness.png", true);
	ufbxt_check_material_texture(scene, material->pbr.normal_map.texture, "checkerboard_normal.png", true);
	ufbxt_check_material_texture(scene, material->pbr.ambient_occlusion.texture, "checkerboard_weight.png", true);
	ufbxt_check_material_texture(scene, material->pbr.emission_color.texture, "checkerboard_emissive.png", true);
	ufbxt_check_material_texture(scene, material->pbr.coat_factor.texture, "checkerboard_weight.png", true);
	ufbxt_check_material_texture(scene, material->pbr.coat_roughness.texture, "checkerboard_roughness.png", true);
	ufbxt_check_material_texture(scene, material->pbr.coat_normal.texture, "checkerboard_normal.png", true);
	ufbxt_check_material_texture(scene, material->pbr.sheen_color.texture, "checkerboard_ambient.png", true);
	ufbxt_check_material_texture(scene, material->pbr.sheen_roughness.texture, "checkerboard_roughness.png", true);
	ufbxt_check_material_texture(scene, material->pbr.specular_factor.texture, "checkerboard_weight.png", true);
	ufbxt_check_material_texture(scene, material->pbr.specular_color.texture, "checkerboard_specular.png", true);
	ufbxt_check_material_texture(scene, material->pbr.transmission_factor.texture, "checkerboard_transparency.png", true);

	ufbxt_assert(material->features.metalness.enabled);
	ufbxt_assert(!material->features.metalness.is_explicit);
	ufbxt_assert(material->features.double_sided.enabled);
	ufbxt_assert(material->features.double_sided.is_explicit);
	ufbxt_assert(material->features.coat.enabled);
	ufbxt_assert(material->features.coat.is_explicit);
	ufbxt_assert(material->features.sheen.enabled);
	ufbxt_assert(material->features.sheen.is_explicit);
	ufbxt_assert(material->features.specular.enabled);
	ufbxt_assert(material->features.specular.is_explicit);
	ufbxt_assert(material->features.transmission.enabled);
	ufbxt_assert(material->features.transmission.is_explicit);
	ufbxt_assert(material->features.ior.enabled);
	ufbxt_assert(material->features.ior.is_explicit);
}
#endif

UFBXT_FILE_TEST(maya_shaderfx_pbs_material)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "StingrayPBS1");
	ufbxt_assert(material);

	ufbxt_assert(material->shader_type == UFBX_SHADER_SHADERFX_GRAPH);

	ufbxt_assert( 1 == (int)round(100.0f * material->pbr.base_color.value_vec3.x));
	ufbxt_assert( 2 == (int)round(100.0f * material->pbr.base_color.value_vec3.y));
	ufbxt_assert( 3 == (int)round(100.0f * material->pbr.base_color.value_vec3.z));
	ufbxt_assert( 4 == (int)round(100.0f * material->pbr.metalness.value_vec3.x));
	ufbxt_assert( 5 == (int)round(100.0f * material->pbr.roughness.value_vec3.x));
	ufbxt_assert( 6 == (int)round(100.0f * material->pbr.emission_color.value_vec3.x));
	ufbxt_assert( 7 == (int)round(100.0f * material->pbr.emission_color.value_vec3.y));
	ufbxt_assert( 8 == (int)round(100.0f * material->pbr.emission_color.value_vec3.z));
	ufbxt_assert( 9 == (int)round(100.0f * material->pbr.emission_factor.value_real));
	ufbxt_assert_close_real(err, material->pbr.base_factor.value_real, 1.0f);

	ufbxt_check_material_texture(scene, material->pbr.base_color.texture, "checkerboard_diffuse.png", true);
	ufbxt_check_material_texture(scene, material->pbr.normal_map.texture, "checkerboard_normal.png", true);
	ufbxt_check_material_texture(scene, material->pbr.metalness.texture, "checkerboard_metallic.png", true);
	ufbxt_check_material_texture(scene, material->pbr.roughness.texture, "checkerboard_roughness.png", true);
	ufbxt_check_material_texture(scene, material->pbr.emission_color.texture, "checkerboard_emissive.png", true);
	ufbxt_check_material_texture(scene, material->pbr.ambient_occlusion.texture, "checkerboard_weight.png", true);

	ufbxt_assert(material->pbr.base_color.texture_enabled == true);
	ufbxt_assert(material->pbr.normal_map.texture_enabled == false);
	ufbxt_assert(material->pbr.metalness.texture_enabled == true);
	ufbxt_assert(material->pbr.roughness.texture_enabled == false);
	ufbxt_assert(material->pbr.emission_color.texture_enabled == true);
	ufbxt_assert(material->pbr.ambient_occlusion.texture_enabled == false);

	ufbx_shader *shader = material->shader;
	ufbxt_assert(shader);

	if (scene->metadata.version >= 7000) {
		ufbx_blob blob = ufbx_find_blob(&shader->props, "ShaderGraph", ufbx_empty_blob);
		ufbxt_assert(blob.size == 32918);
		uint32_t hash_ref = ufbxt_fnv1a(blob.data, blob.size);
		ufbxt_assert(hash_ref == 0x7be121c5u);
	}
}
#endif

UFBXT_FILE_TEST(max_openpbr_material)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "OpenPBR");
	ufbxt_assert(material);

	ufbxt_assert(material->shader_type == UFBX_SHADER_OPENPBR_MATERIAL);

	ufbxt_assert( 1 == (int)round(100.0f * material->pbr.base_factor.value_real));
	ufbxt_assert( 2 == (int)round(100.0f * material->pbr.base_color.value_vec4.x));
	ufbxt_assert( 3 == (int)round(100.0f * material->pbr.base_color.value_vec4.y));
	ufbxt_assert( 4 == (int)round(100.0f * material->pbr.base_color.value_vec4.z));
	ufbxt_assert( 5 == (int)round(100.0f * material->pbr.base_color.value_vec4.w));
	ufbxt_assert( 6 == (int)round(100.0f * material->pbr.metalness.value_real));
	ufbxt_assert( 7 == (int)round(100.0f * material->pbr.diffuse_roughness.value_real));
	ufbxt_assert( 8 == (int)round(100.0f * material->pbr.specular_factor.value_real));
	ufbxt_assert( 9 == (int)round(100.0f * material->pbr.specular_color.value_vec4.x));
	ufbxt_assert(10 == (int)round(100.0f * material->pbr.specular_color.value_vec4.y));
	ufbxt_assert(11 == (int)round(100.0f * material->pbr.specular_color.value_vec4.z));
	ufbxt_assert(12 == (int)round(100.0f * material->pbr.specular_color.value_vec4.w));
	// 13 specular_ior
	ufbxt_assert(14 == (int)round(100.0f * material->pbr.roughness.value_real));
	ufbxt_assert(15 == (int)round(100.0f * material->pbr.specular_anisotropy.value_real));
	ufbxt_assert(16 == (int)round(100.0f * material->pbr.transmission_factor.value_real));
	ufbxt_assert(17 == (int)round(100.0f * material->pbr.transmission_color.value_vec4.x));
	ufbxt_assert(18 == (int)round(100.0f * material->pbr.transmission_color.value_vec4.y));
	ufbxt_assert(19 == (int)round(100.0f * material->pbr.transmission_color.value_vec4.z));
	ufbxt_assert(20 == (int)round(100.0f * material->pbr.transmission_color.value_vec4.w));
	ufbxt_assert(21 == (int)round(100.0f * material->pbr.transmission_depth.value_real));
	ufbxt_assert(22 == (int)round(100.0f * material->pbr.transmission_scatter.value_vec4.x));
	ufbxt_assert(23 == (int)round(100.0f * material->pbr.transmission_scatter.value_vec4.y));
	ufbxt_assert(24 == (int)round(100.0f * material->pbr.transmission_scatter.value_vec4.z));
	ufbxt_assert(25 == (int)round(100.0f * material->pbr.transmission_scatter.value_vec4.w));
	ufbxt_assert(26 == (int)round(100.0f * material->pbr.transmission_scatter_anisotropy.value_real));
	ufbxt_assert(27 == (int)round(100.0f * material->pbr.transmission_dispersion.value_real));
	// 28 Abbe number unsupported
	ufbxt_assert(29 == (int)round(100.0f * material->pbr.subsurface_factor.value_real));
	ufbxt_assert(30 == (int)round(100.0f * material->pbr.subsurface_color.value_vec4.x));
	ufbxt_assert(31 == (int)round(100.0f * material->pbr.subsurface_color.value_vec4.y));
	ufbxt_assert(32 == (int)round(100.0f * material->pbr.subsurface_color.value_vec4.z));
	ufbxt_assert(33 == (int)round(100.0f * material->pbr.subsurface_color.value_vec4.w));
	ufbxt_assert(34 == (int)round(100.0f * material->pbr.subsurface_anisotropy.value_real));
	ufbxt_assert(35 == (int)round(100.0f * material->pbr.subsurface_radius.value_vec4.x));
	ufbxt_assert(36 == (int)round(100.0f * material->pbr.subsurface_radius.value_vec4.y));
	ufbxt_assert(37 == (int)round(100.0f * material->pbr.subsurface_radius.value_vec4.z));
	ufbxt_assert(38 == (int)round(100.0f * material->pbr.subsurface_radius.value_vec4.w));
	ufbxt_assert(39 == (int)round(100.0f * material->pbr.subsurface_scale.value_real));
	ufbxt_assert(40 == (int)round(100.0f * material->pbr.coat_factor.value_real));
	ufbxt_assert(41 == (int)round(100.0f * material->pbr.coat_color.value_vec4.x));
	ufbxt_assert(42 == (int)round(100.0f * material->pbr.coat_color.value_vec4.y));
	ufbxt_assert(43 == (int)round(100.0f * material->pbr.coat_color.value_vec4.z));
	ufbxt_assert(44 == (int)round(100.0f * material->pbr.coat_color.value_vec4.w));
	// 45 coat_ior
	ufbxt_assert(46 == (int)round(100.0f * material->pbr.coat_roughness.value_real));
	ufbxt_assert(47 == (int)round(100.0f * material->pbr.coat_anisotropy.value_real));
	// 48 Coat roughness unsupported
	ufbxt_assert(49 == (int)round(100.0f * material->pbr.sheen_factor.value_real));
	ufbxt_assert(50 == (int)round(100.0f * material->pbr.sheen_color.value_vec4.x));
	ufbxt_assert(51 == (int)round(100.0f * material->pbr.sheen_color.value_vec4.y));
	ufbxt_assert(52 == (int)round(100.0f * material->pbr.sheen_color.value_vec4.z));
	ufbxt_assert(53 == (int)round(100.0f * material->pbr.sheen_color.value_vec4.w));
	ufbxt_assert(54 == (int)round(100.0f * material->pbr.sheen_roughness.value_real));
	// 55 emission combined check below
	ufbxt_assert(56 == (int)round(100.0f * material->pbr.emission_color.value_vec4.x));
	ufbxt_assert(57 == (int)round(100.0f * material->pbr.emission_color.value_vec4.y));
	ufbxt_assert(58 == (int)round(100.0f * material->pbr.emission_color.value_vec4.z));
	ufbxt_assert(59 == (int)round(100.0f * material->pbr.emission_color.value_vec4.w));
	// 60 luminance combined with emission_factor
	ufbxt_assert(61 == (int)round(100.0f * material->pbr.thin_film_factor.value_real));
	ufbxt_assert(62 == (int)round(100.0f * material->pbr.thin_film_thickness.value_real));
	// 63 thin_film_ior
	ufbxt_assert(64 == (int)round(100.0f * material->pbr.normal_map.value_real));
	ufbxt_assert(65 == (int)round(100.0f * material->pbr.coat_normal.value_real));
	ufbxt_assert(66 == (int)round(100.0f * material->pbr.displacement_map.value_real));
	// 67 indirect_diffuse currently unparsed due to not having a prefix
	// 68 indirecct_specular currently unparsed due to not having a prefix

	ufbxt_assert(130 == (int)round(100.0f * material->pbr.specular_ior.value_real));
	ufbxt_assert(450 == (int)round(100.0f * material->pbr.coat_ior.value_real));
	ufbxt_assert(630 == (int)round(100.0f * material->pbr.thin_film_ior.value_real));

	ufbxt_assert_close_real(err, material->pbr.emission_factor.value_real, 0.55f * 6000.0f);

	ufbxt_check_material_texture(scene, material->pbr.base_color.texture, "checkerboard_diffuse.png", false);
	ufbxt_check_material_texture(scene, material->pbr.metalness.texture, "checkerboard_metallic.png", false);
	ufbxt_check_material_texture(scene, material->pbr.specular_color.texture, "checkerboard_specular.png", false);
	ufbxt_check_material_texture(scene, material->pbr.roughness.texture, "checkerboard_roughness.png", false);
	ufbxt_check_material_texture(scene, material->pbr.transmission_color.texture, "checkerboard_transparency.png", false);
	ufbxt_check_material_texture(scene, material->pbr.transmission_scatter.texture, "checkerboard_reflection.png", false);
	ufbxt_check_material_texture(scene, material->pbr.subsurface_color.texture, "checkerboard_ambient.png", false);
	ufbxt_check_material_texture(scene, material->pbr.coat_color.texture, "checkerboard_weight.png", false);
	ufbxt_check_material_texture(scene, material->pbr.emission_color.texture, "checkerboard_emissive.png", false);
	ufbxt_check_material_texture(scene, material->pbr.emission_factor.texture, "tiny_clouds.png", false);
	ufbxt_check_material_texture(scene, material->pbr.normal_map.texture, "checkerboard_normal.png", false);
	ufbxt_check_material_texture(scene, material->pbr.displacement_map.texture, "checkerboard_vector_displacement.png", false);

	ufbxt_assert(material->pbr.base_factor.texture_enabled);
	ufbxt_assert(material->pbr.base_color.texture_enabled);
	ufbxt_assert(material->pbr.specular_factor.texture_enabled);
	ufbxt_assert(material->pbr.specular_color.texture_enabled);
	ufbxt_assert(material->pbr.roughness.texture_enabled);
	ufbxt_assert(material->pbr.metalness.texture_enabled);
	ufbxt_assert(material->pbr.diffuse_roughness.texture_enabled);
	ufbxt_assert(material->pbr.specular_anisotropy.texture_enabled);
	ufbxt_assert(material->pbr.transmission_factor.texture_enabled);
	ufbxt_assert(material->pbr.transmission_color.texture_enabled);
	ufbxt_assert(material->pbr.specular_ior.texture_enabled);
	ufbxt_assert(material->pbr.subsurface_factor.texture_enabled);
	ufbxt_assert(material->pbr.subsurface_scale.texture_enabled);
	ufbxt_assert(material->pbr.emission_factor.texture_enabled);
	ufbxt_assert(material->pbr.emission_color.texture_enabled);
	ufbxt_assert(material->pbr.coat_factor.texture_enabled);
	ufbxt_assert(material->pbr.coat_color.texture_enabled);
	ufbxt_assert(material->pbr.coat_roughness.texture_enabled);
	ufbxt_assert(material->pbr.normal_map.texture_enabled);
	ufbxt_assert(material->pbr.coat_normal.texture_enabled);
	ufbxt_assert(material->pbr.displacement_map.texture_enabled);
	ufbxt_assert(material->pbr.opacity.texture_enabled);

	ufbxt_assert(material->pbr.base_factor.value_components == 1);
	ufbxt_assert(material->pbr.base_color.value_components == 4);
	ufbxt_assert(material->pbr.specular_factor.value_components == 1);
	ufbxt_assert(material->pbr.specular_color.value_components == 4);
	ufbxt_assert(material->pbr.roughness.value_components == 1);
	ufbxt_assert(material->pbr.metalness.value_components == 1);
	ufbxt_assert(material->pbr.diffuse_roughness.value_components == 1);
	ufbxt_assert(material->pbr.specular_anisotropy.value_components == 1);
	ufbxt_assert(material->pbr.transmission_factor.value_components == 1);
	ufbxt_assert(material->pbr.transmission_color.value_components == 4);
	ufbxt_assert(material->pbr.transmission_extra_roughness.value_components == 0);
	ufbxt_assert(material->pbr.specular_ior.value_components == 1);
	ufbxt_assert(material->pbr.subsurface_factor.value_components == 1);
	ufbxt_assert(material->pbr.subsurface_color.value_components == 4);
	ufbxt_assert(material->pbr.subsurface_radius.value_components == 4);
	ufbxt_assert(material->pbr.subsurface_scale.value_components == 1);
	ufbxt_assert(material->pbr.emission_factor.value_components == 1);
	ufbxt_assert(material->pbr.emission_color.value_components == 4);
	ufbxt_assert(material->pbr.coat_factor.value_components == 1);
	ufbxt_assert(material->pbr.coat_color.value_components == 4);
	ufbxt_assert(material->pbr.coat_roughness.value_components == 1);
	ufbxt_assert(material->pbr.normal_map.value_components == 1);
	ufbxt_assert(material->pbr.coat_normal.value_components == 1);
	ufbxt_assert(material->pbr.displacement_map.value_components == 1);
	ufbxt_assert(material->pbr.opacity.value_components == 0);

	ufbxt_assert(material->features.diffuse.enabled);
	ufbxt_assert(material->features.metalness.enabled);
	ufbxt_assert(material->features.specular.enabled);
	ufbxt_assert(material->features.coat.enabled);
	ufbxt_assert(material->features.sheen.enabled);
	ufbxt_assert(material->features.transmission.enabled);
	ufbxt_assert(material->features.opacity.enabled);
	ufbxt_assert(material->features.ior.enabled);
	ufbxt_assert(material->features.diffuse_roughness.enabled);
	ufbxt_assert(material->features.thin_walled.enabled);
	ufbxt_assert(material->features.thin_walled.is_explicit);
}
#endif

UFBXT_FILE_TEST(blender_279_internal_textures)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Material.001");
	ufbxt_assert(material);
	ufbxt_assert(material->textures.count == 5);

	ufbxt_assert(material->shader_type == UFBX_SHADER_FBX_PHONG);
	ufbxt_assert(!strcmp(material->fbx.diffuse_color.texture->relative_filename.data, "textures\\checkerboard_diffuse.png"));
	ufbxt_assert(!strcmp(material->fbx.specular_color.texture->relative_filename.data, "textures\\checkerboard_specular.png"));
	ufbxt_assert(!strcmp(material->fbx.specular_factor.texture->relative_filename.data, "textures\\checkerboard_weight.png"));
	ufbxt_assert(!strcmp(material->fbx.ambient_factor.texture->relative_filename.data, "textures\\checkerboard_ambient.png"));
	ufbxt_assert(!strcmp(material->fbx.emission_factor.texture->relative_filename.data, "textures\\checkerboard_emissive.png"));
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_blender_pbr_load_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.use_blender_pbr_material = true;
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS_FLAGS(blender_293_textures, ufbxt_blender_pbr_load_opts, UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Material.001");
	ufbxt_assert(material);
	ufbxt_assert(material->textures.count == 5);

	ufbxt_assert(material->shader_type == UFBX_SHADER_BLENDER_PHONG);
	ufbxt_assert(!strcmp(material->pbr.base_color.texture->relative_filename.data, "textures\\checkerboard_diffuse.png"));
	ufbxt_assert(!strcmp(material->pbr.roughness.texture->relative_filename.data, "textures\\checkerboard_roughness.png"));
	ufbxt_assert(!strcmp(material->pbr.metalness.texture->relative_filename.data, "textures\\checkerboard_metallic.png"));
	ufbxt_assert(!strcmp(material->pbr.emission_color.texture->relative_filename.data, "textures\\checkerboard_emissive.png"));
	ufbxt_assert(!strcmp(material->pbr.opacity.texture->relative_filename.data, "textures\\checkerboard_weight.png"));
}
#endif

UFBXT_FILE_TEST_ALT_FLAGS(blender_293_textures_default, blender_293_textures, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Material.001");
	ufbxt_assert(material);
	ufbxt_assert(material->textures.count == 5);

	ufbxt_assert(material->shader_type == UFBX_SHADER_FBX_PHONG);
	ufbxt_assert(!strcmp(material->pbr.base_color.texture->relative_filename.data, "textures\\checkerboard_diffuse.png"));
	ufbxt_assert(!strcmp(material->pbr.roughness.texture->relative_filename.data, "textures\\checkerboard_roughness.png"));
	ufbxt_assert(!material->pbr.metalness.texture);
	ufbxt_assert(!strcmp(material->pbr.emission_color.texture->relative_filename.data, "textures\\checkerboard_emissive.png"));
	ufbxt_assert(!material->pbr.opacity.texture);
}
#endif

UFBXT_FILE_TEST_OPTS_FLAGS(blender_293_embedded_textures, ufbxt_blender_pbr_load_opts, UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Material.001");
	ufbxt_assert(material);
	ufbxt_assert(material->textures.count == 5);

	ufbxt_assert(material->shader_type == UFBX_SHADER_BLENDER_PHONG);
	ufbxt_check_texture_content(scene, material->pbr.base_color.texture, "checkerboard_diffuse.png");
	ufbxt_check_texture_content(scene, material->pbr.roughness.texture, "checkerboard_roughness.png");
	ufbxt_check_texture_content(scene, material->pbr.metalness.texture, "checkerboard_metallic.png");
	ufbxt_check_texture_content(scene, material->pbr.emission_color.texture, "checkerboard_emissive.png");
	ufbxt_check_texture_content(scene, material->pbr.opacity.texture, "checkerboard_weight.png");
}
#endif

UFBXT_FILE_TEST_ALT_FLAGS(blender_293_embedded_textures_default, blender_293_embedded_textures, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Material.001");
	ufbxt_assert(material);
	ufbxt_assert(material->textures.count == 5);

	ufbxt_assert(material->shader_type == UFBX_SHADER_FBX_PHONG);
	ufbxt_check_texture_content(scene, material->pbr.base_color.texture, "checkerboard_diffuse.png");
	ufbxt_check_texture_content(scene, material->pbr.roughness.texture, "checkerboard_roughness.png");
	ufbxt_assert(!material->pbr.metalness.texture);
	ufbxt_check_texture_content(scene, ufbx_find_prop_texture(material, "ReflectionFactor"), "checkerboard_metallic.png");
	ufbxt_check_texture_content(scene, material->pbr.emission_color.texture, "checkerboard_emissive.png");
	ufbxt_assert(!material->pbr.opacity.texture);
	ufbxt_check_texture_content(scene, ufbx_find_prop_texture(material, "TransparencyFactor"), "checkerboard_weight.png");
}
#endif

UFBXT_FILE_TEST_OPTS_FLAGS(blender_293_material_mapping, ufbxt_blender_pbr_load_opts, UFBXT_FILE_TEST_FLAG_FUZZ_OPTS)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Material.001");
	ufbxt_assert(material);

	ufbxt_assert(material->shader_type == UFBX_SHADER_BLENDER_PHONG);
	ufbxt_assert_close_real(err, material->fbx.specular_exponent.value_vec3.x, 76.913f);
	ufbxt_assert_close_real(err, material->fbx.transparency_factor.value_vec3.x, 0.544f);
	ufbxt_assert_close_real(err, material->pbr.opacity.value_vec3.x, 0.456f);
	ufbxt_assert_close_real(err, material->pbr.roughness.value_vec3.x, 0.123f);
}
#endif

UFBXT_FILE_TEST_ALT_FLAGS(blender_293_material_mapping_default, blender_293_material_mapping, UFBXT_FILE_TEST_FLAG_FUZZ_ALWAYS)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Material.001");
	ufbxt_assert(material);

	ufbxt_assert(material->shader_type == UFBX_SHADER_FBX_PHONG);
	ufbxt_assert_close_real(err, material->fbx.specular_exponent.value_vec3.x, 76.913f);
	ufbxt_assert_close_real(err, material->fbx.transparency_factor.value_vec3.x, 0.544f);
	ufbxt_assert(!material->pbr.opacity.has_value);
	ufbxt_assert_close_real(err, material->pbr.roughness.value_vec3.x, 0.123f);
}
#endif

UFBXT_FILE_TEST(maya_different_shaders)
#if UFBXT_IMPL
{
	ufbx_material *lambert1 = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "lambert1");
	ufbx_material *phong1 = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "phong1");
	ufbx_material *arnold = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "aiStandardSurface1");
	ufbxt_assert(lambert1 && phong1 && arnold);

	ufbx_vec3 r = { 1.0f, 0.0f, 0.0f };
	ufbx_vec3 g = { 0.0f, 1.0f, 0.0f };
	ufbx_vec3 b = { 0.0f, 0.0f, 1.0f };

	ufbxt_assert(lambert1->shader_type == UFBX_SHADER_FBX_LAMBERT);
	ufbxt_assert(!strcmp(lambert1->shading_model_name.data, "lambert"));
	ufbxt_assert_close_vec3(err, lambert1->fbx.diffuse_color.value_vec3, g);
	ufbxt_assert_close_vec3(err, lambert1->pbr.base_color.value_vec3, g);
	ufbxt_assert_close_real(err, lambert1->pbr.specular_factor.value_vec3.x, 0.0f);

	ufbxt_assert(phong1->shader_type == UFBX_SHADER_FBX_PHONG);
	ufbxt_assert(!strcmp(phong1->shading_model_name.data, "phong"));
	ufbxt_assert_close_vec3(err, phong1->fbx.diffuse_color.value_vec3, b);
	ufbxt_assert_close_vec3(err, phong1->pbr.base_color.value_vec3, b);
	ufbxt_assert_close_real(err, phong1->pbr.specular_factor.value_vec3.x, 1.0f);

	ufbxt_assert(arnold->shader_type == UFBX_SHADER_ARNOLD_STANDARD_SURFACE);
	ufbxt_assert(!strcmp(arnold->shading_model_name.data, "unknown"));
	ufbxt_assert_close_vec3(err, arnold->pbr.base_color.value_vec3, r);
}
#endif

UFBXT_TEST(blender_phong_quirks)
#if UFBXT_IMPL
{
	for (int quirks = 0; quirks <= 1; quirks++) {
		ufbx_load_opts opts = { 0 };
		opts.disable_quirks = (quirks == 0);
		opts.use_blender_pbr_material = true;

		char buf[512];
		snprintf(buf, sizeof(buf), "%s%s", data_root, "blender_293_textures_7400_binary.fbx");

		ufbx_scene *scene = ufbx_load_file(buf, &opts, NULL);
		ufbxt_assert(scene);

		// Exporter should be detected even with quirks off
		ufbxt_assert(scene->metadata.exporter == UFBX_EXPORTER_BLENDER_BINARY);
		ufbxt_assert(scene->metadata.exporter_version == ufbx_pack_version(4, 22, 0));

		ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Material.001");
		ufbxt_assert(material);
		if (quirks) {
			ufbxt_assert(material->shader_type == UFBX_SHADER_BLENDER_PHONG);
		} else {
			ufbxt_assert(material->shader_type == UFBX_SHADER_FBX_PHONG);
		}

		ufbx_free_scene(scene);
	}
}
#endif

UFBXT_FILE_TEST(maya_phong_properties)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "phong1");
	ufbxt_assert(material);
	ufbxt_assert(material->shader_type == UFBX_SHADER_FBX_PHONG);

	ufbxt_assert( 1 == (int)round(100.0f * material->fbx.diffuse_color.value_vec3.x));
	ufbxt_assert( 2 == (int)round(100.0f * material->fbx.diffuse_color.value_vec3.y));
	ufbxt_assert( 3 == (int)round(100.0f * material->fbx.diffuse_color.value_vec3.z));
	ufbxt_assert( 4 == (int)round(100.0f * material->fbx.transparency_color.value_vec3.x));
	ufbxt_assert( 5 == (int)round(100.0f * material->fbx.transparency_color.value_vec3.y));
	ufbxt_assert( 6 == (int)round(100.0f * material->fbx.transparency_color.value_vec3.z));
	ufbxt_assert( 7 == (int)round(100.0f * material->fbx.ambient_color.value_vec3.x));
	ufbxt_assert( 8 == (int)round(100.0f * material->fbx.ambient_color.value_vec3.y));
	ufbxt_assert( 9 == (int)round(100.0f * material->fbx.ambient_color.value_vec3.z));
	ufbxt_assert(10 == (int)round(100.0f * material->fbx.emission_color.value_vec3.x));
	ufbxt_assert(11 == (int)round(100.0f * material->fbx.emission_color.value_vec3.y));
	ufbxt_assert(12 == (int)round(100.0f * material->fbx.emission_color.value_vec3.z));
	ufbxt_assert(13 == (int)round(100.0f * material->fbx.diffuse_factor.value_vec3.x));
	ufbxt_assert(17 == (int)round(  1.0f * material->fbx.specular_exponent.value_vec3.x));
	ufbxt_assert(18 == (int)round(100.0f * material->fbx.specular_color.value_vec3.x));
	ufbxt_assert(19 == (int)round(100.0f * material->fbx.specular_color.value_vec3.y));
	ufbxt_assert(20 == (int)round(100.0f * material->fbx.specular_color.value_vec3.z));
	ufbxt_assert(21 == (int)round(100.0f * material->fbx.reflection_factor.value_vec3.x));
	ufbxt_assert(22 == (int)round(100.0f * material->fbx.reflection_color.value_vec3.x));
	ufbxt_assert(23 == (int)round(100.0f * material->fbx.reflection_color.value_vec3.y));
	ufbxt_assert(24 == (int)round(100.0f * material->fbx.reflection_color.value_vec3.z));

	ufbxt_assert( 1 == (int)round(100.0f * material->pbr.base_color.value_vec3.x));
	ufbxt_assert( 2 == (int)round(100.0f * material->pbr.base_color.value_vec3.y));
	ufbxt_assert( 3 == (int)round(100.0f * material->pbr.base_color.value_vec3.z));
	ufbxt_assert(13 == (int)round(100.0f * material->pbr.base_factor.value_vec3.x));
	ufbxt_assert_close_real(err, material->pbr.metalness.value_real, 0.0f);
	ufbxt_assert_close_real(err, material->pbr.roughness.value_real, 0.587689f);
}
#endif

UFBXT_FILE_TEST(maya_arnold_properties)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "aiStandardSurface1");
	ufbxt_assert(material);
	ufbxt_assert(material->shader_type == UFBX_SHADER_ARNOLD_STANDARD_SURFACE);

	ufbxt_assert( 1 == (int)round(100.0f * material->pbr.base_factor.value_vec3.x));
	ufbxt_assert( 2 == (int)round(100.0f * material->pbr.base_color.value_vec3.x));
	ufbxt_assert( 3 == (int)round(100.0f * material->pbr.base_color.value_vec3.y));
	ufbxt_assert( 4 == (int)round(100.0f * material->pbr.base_color.value_vec3.z));
	ufbxt_assert( 5 == (int)round(100.0f * material->pbr.diffuse_roughness.value_vec3.x));
	ufbxt_assert( 6 == (int)round(100.0f * material->pbr.metalness.value_vec3.x));
	ufbxt_assert( 7 == (int)round(100.0f * material->pbr.specular_factor.value_vec3.x));
	ufbxt_assert( 8 == (int)round(100.0f * material->pbr.specular_color.value_vec3.x));
	ufbxt_assert( 9 == (int)round(100.0f * material->pbr.specular_color.value_vec3.y));
	ufbxt_assert(10 == (int)round(100.0f * material->pbr.specular_color.value_vec3.z));
	ufbxt_assert(11 == (int)round(100.0f * material->pbr.roughness.value_vec3.x));
	ufbxt_assert(12 == (int)round(100.0f * material->pbr.specular_ior.value_vec3.x));
	ufbxt_assert(13 == (int)round(100.0f * material->pbr.specular_anisotropy.value_vec3.x));
	ufbxt_assert(14 == (int)round(100.0f * material->pbr.specular_rotation.value_vec3.x));
	ufbxt_assert(15 == (int)round(100.0f * material->pbr.transmission_factor.value_vec3.x));
	ufbxt_assert(16 == (int)round(100.0f * material->pbr.transmission_color.value_vec3.x));
	ufbxt_assert(17 == (int)round(100.0f * material->pbr.transmission_color.value_vec3.y));
	ufbxt_assert(18 == (int)round(100.0f * material->pbr.transmission_color.value_vec3.z));
	ufbxt_assert(19 == (int)round(100.0f * material->pbr.transmission_depth.value_vec3.x));
	ufbxt_assert(20 == (int)round(100.0f * material->pbr.transmission_scatter.value_vec3.x));
	ufbxt_assert(21 == (int)round(100.0f * material->pbr.transmission_scatter.value_vec3.y));
	ufbxt_assert(22 == (int)round(100.0f * material->pbr.transmission_scatter.value_vec3.z));
	ufbxt_assert(23 == (int)round(100.0f * material->pbr.transmission_scatter_anisotropy.value_vec3.x));
	ufbxt_assert(24 == (int)round(100.0f * material->pbr.transmission_dispersion.value_vec3.x));
	ufbxt_assert(25 == (int)round(100.0f * material->pbr.transmission_extra_roughness.value_vec3.x));
	ufbxt_assert(26 == (int)round(100.0f * material->pbr.subsurface_factor.value_vec3.x));
	ufbxt_assert(27 == (int)round(100.0f * material->pbr.subsurface_color.value_vec3.x));
	ufbxt_assert(28 == (int)round(100.0f * material->pbr.subsurface_color.value_vec3.y));
	ufbxt_assert(29 == (int)round(100.0f * material->pbr.subsurface_color.value_vec3.z));
	ufbxt_assert(30 == (int)round(100.0f * material->pbr.subsurface_radius.value_vec3.x));
	ufbxt_assert(31 == (int)round(100.0f * material->pbr.subsurface_radius.value_vec3.y));
	ufbxt_assert(32 == (int)round(100.0f * material->pbr.subsurface_radius.value_vec3.z));
	ufbxt_assert(33 == (int)round(100.0f * material->pbr.subsurface_scale.value_vec3.x));
	ufbxt_assert(34 == (int)round(100.0f * material->pbr.subsurface_anisotropy.value_vec3.x));
	ufbxt_assert(35 == (int)round(100.0f * material->pbr.coat_factor.value_vec3.x));
	ufbxt_assert(36 == (int)round(100.0f * material->pbr.coat_color.value_vec3.x));
	ufbxt_assert(37 == (int)round(100.0f * material->pbr.coat_color.value_vec3.y));
	ufbxt_assert(38 == (int)round(100.0f * material->pbr.coat_color.value_vec3.z));
	ufbxt_assert(39 == (int)round(100.0f * material->pbr.coat_roughness.value_vec3.x));
	ufbxt_assert(40 == (int)round(100.0f * material->pbr.coat_ior.value_vec3.x));
	ufbxt_assert(41 == (int)round(100.0f * material->pbr.coat_anisotropy.value_vec3.x));
	ufbxt_assert(42 == (int)round(100.0f * material->pbr.coat_rotation.value_vec3.x));
	ufbxt_assert(43 == (int)round(100.0f * material->pbr.coat_normal.value_vec3.x));
	ufbxt_assert(44 == (int)round(100.0f * material->pbr.coat_normal.value_vec3.y));
	ufbxt_assert(45 == (int)round(100.0f * material->pbr.coat_normal.value_vec3.z));
	ufbxt_assert(46 == (int)round(100.0f * material->pbr.sheen_factor.value_vec3.x));
	ufbxt_assert(47 == (int)round(100.0f * material->pbr.sheen_color.value_vec3.x));
	ufbxt_assert(48 == (int)round(100.0f * material->pbr.sheen_color.value_vec3.y));
	ufbxt_assert(49 == (int)round(100.0f * material->pbr.sheen_color.value_vec3.z));
	ufbxt_assert(50 == (int)round(100.0f * material->pbr.sheen_roughness.value_vec3.x));
	ufbxt_assert(51 == (int)round(100.0f * material->pbr.emission_factor.value_vec3.x));
	ufbxt_assert(52 == (int)round(100.0f * material->pbr.emission_color.value_vec3.x));
	ufbxt_assert(53 == (int)round(100.0f * material->pbr.emission_color.value_vec3.y));
	ufbxt_assert(54 == (int)round(100.0f * material->pbr.emission_color.value_vec3.z));
	ufbxt_assert(55 == (int)round(100.0f * material->pbr.thin_film_thickness.value_vec3.x));
	ufbxt_assert(56 == (int)round(100.0f * material->pbr.thin_film_ior.value_vec3.x));
	ufbxt_assert(57 == (int)round(100.0f * material->pbr.opacity.value_vec3.x));
	ufbxt_assert(58 == (int)round(100.0f * material->pbr.opacity.value_vec3.y));
	ufbxt_assert(59 == (int)round(100.0f * material->pbr.opacity.value_vec3.z));
	ufbxt_assert(60 == (int)round(100.0f * material->pbr.tangent_map.value_vec3.x));
	ufbxt_assert(61 == (int)round(100.0f * material->pbr.tangent_map.value_vec3.y));
	ufbxt_assert(62 == (int)round(100.0f * material->pbr.tangent_map.value_vec3.z));
	ufbxt_assert(63 == (int)round(100.0f * material->pbr.indirect_diffuse.value_vec3.x));
	ufbxt_assert(64 == (int)round(100.0f * material->pbr.indirect_specular.value_vec3.x));
	ufbxt_assert(65 == (int)round(100.0f * material->pbr.matte_color.value_vec3.x));
	ufbxt_assert(66 == (int)round(100.0f * material->pbr.matte_color.value_vec3.y));
	ufbxt_assert(67 == (int)round(100.0f * material->pbr.matte_color.value_vec3.z));
	ufbxt_assert(68 == (int)round(100.0f * material->pbr.matte_factor.value_vec3.x));
	ufbxt_assert(69 == material->pbr.transmission_priority.value_int);

	// Computed
	ufbxt_assert_close_real(err, material->pbr.transmission_roughness.value_real, 0.36f); // 11+25
	ufbxt_assert(material->pbr.thin_film_factor.value_real == 1.0f);

	ufbxt_assert(material->pbr.base_factor.value_components == 1);
	ufbxt_assert(material->pbr.base_color.value_components == 3);
	ufbxt_assert(material->pbr.specular_factor.value_components == 1);
	ufbxt_assert(material->pbr.specular_color.value_components == 3);
	ufbxt_assert(material->pbr.roughness.value_components == 1);
	ufbxt_assert(material->pbr.metalness.value_components == 1);
	ufbxt_assert(material->pbr.diffuse_roughness.value_components == 1);
	ufbxt_assert(material->pbr.specular_anisotropy.value_components == 1);
	ufbxt_assert(material->pbr.specular_rotation.value_components == 1);
	ufbxt_assert(material->pbr.transmission_factor.value_components == 1);
	ufbxt_assert(material->pbr.transmission_color.value_components == 3);
	ufbxt_assert(material->pbr.transmission_roughness.value_components == 0);
	ufbxt_assert(material->pbr.specular_ior.value_components == 1);
	ufbxt_assert(material->pbr.subsurface_factor.value_components == 1);
	ufbxt_assert(material->pbr.subsurface_tint_color.value_components == 0);
	ufbxt_assert(material->pbr.subsurface_color.value_components == 3);
	ufbxt_assert(material->pbr.subsurface_radius.value_components == 3);
	ufbxt_assert(material->pbr.subsurface_scale.value_components == 1);
	ufbxt_assert(material->pbr.emission_factor.value_components == 1);
	ufbxt_assert(material->pbr.emission_color.value_components == 3);
	ufbxt_assert(material->pbr.coat_factor.value_components == 1);
	ufbxt_assert(material->pbr.coat_color.value_components == 3);
	ufbxt_assert(material->pbr.coat_roughness.value_components == 1);
	ufbxt_assert(material->pbr.normal_map.value_components == 3);
	ufbxt_assert(material->pbr.coat_normal.value_components == 3);
	ufbxt_assert(material->pbr.displacement_map.value_components == 0);
	ufbxt_assert(material->pbr.opacity.value_components == 3);

	ufbxt_assert(material->pbr.subsurface_type.value_int == 1);
	ufbxt_assert(material->pbr.transmission_enable_in_aov.value_int != 0);
	ufbxt_assert(material->features.thin_walled.enabled);
	ufbxt_assert(material->features.thin_walled.is_explicit);
	ufbxt_assert(material->features.matte.enabled);
	ufbxt_assert(material->features.matte.is_explicit);
	ufbxt_assert(material->features.caustics.enabled);
	ufbxt_assert(material->features.caustics.is_explicit);
	ufbxt_assert(material->features.internal_reflections.enabled);
	ufbxt_assert(material->features.internal_reflections.is_explicit);
	ufbxt_assert(material->features.exit_to_background.enabled);
	ufbxt_assert(material->features.exit_to_background.is_explicit);
}
#endif

UFBXT_FILE_TEST_ALT(maya_arnold_properties_shader, maya_arnold_properties)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "aiStandardSurface1");
	ufbxt_assert(material);
	ufbxt_assert(material->shader_type == UFBX_SHADER_ARNOLD_STANDARD_SURFACE);

	ufbx_shader *shader = material->shader;
	ufbxt_assert(shader);
	ufbxt_assert(shader->type == UFBX_SHADER_ARNOLD_STANDARD_SURFACE);

	ufbx_string prop = ufbx_find_shader_prop(shader, "baseColor");
	ufbxt_assert(!strcmp(prop.data, "Maya|baseColor"));

	ufbx_vec3 base_color = ufbx_find_vec3(&material->props, prop.data, ufbx_zero_vec3);
	ufbxt_assert(2 == (int)round(100.0f * base_color.x));
	ufbxt_assert(3 == (int)round(100.0f * base_color.y));
	ufbxt_assert(4 == (int)round(100.0f * base_color.z));

	ufbx_string prop_none = ufbx_find_shader_prop(shader, "nonexistent");
	ufbxt_assert(prop_none.length == 0);
}
#endif

UFBXT_FILE_TEST(maya_osl_properties)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "standardSurface2");
	ufbxt_assert(material);
	ufbxt_assert(material->shader_type == UFBX_SHADER_OSL_STANDARD_SURFACE);

	ufbxt_assert( 1 == (int)round(100.0f * material->pbr.base_factor.value_vec3.x));
	ufbxt_assert( 2 == (int)round(100.0f * material->pbr.base_color.value_vec3.x));
	ufbxt_assert( 3 == (int)round(100.0f * material->pbr.base_color.value_vec3.y));
	ufbxt_assert( 4 == (int)round(100.0f * material->pbr.base_color.value_vec3.z));
	ufbxt_assert( 5 == (int)round(100.0f * material->pbr.diffuse_roughness.value_vec3.x));
	ufbxt_assert( 6 == (int)round(100.0f * material->pbr.metalness.value_vec3.x));
	ufbxt_assert( 7 == (int)round(100.0f * material->pbr.specular_factor.value_vec3.x));
	ufbxt_assert( 8 == (int)round(100.0f * material->pbr.specular_color.value_vec3.x));
	ufbxt_assert( 9 == (int)round(100.0f * material->pbr.specular_color.value_vec3.y));
	ufbxt_assert(10 == (int)round(100.0f * material->pbr.specular_color.value_vec3.z));
	ufbxt_assert(11 == (int)round(100.0f * material->pbr.roughness.value_vec3.x));
	ufbxt_assert(12 == (int)round(100.0f * material->pbr.specular_ior.value_vec3.x));
	ufbxt_assert(13 == (int)round(100.0f * material->pbr.specular_anisotropy.value_vec3.x));
	ufbxt_assert(14 == (int)round(100.0f * material->pbr.specular_rotation.value_vec3.x));
	ufbxt_assert(15 == (int)round(100.0f * material->pbr.transmission_factor.value_vec3.x));
	ufbxt_assert(16 == (int)round(100.0f * material->pbr.transmission_color.value_vec3.x));
	ufbxt_assert(17 == (int)round(100.0f * material->pbr.transmission_color.value_vec3.y));
	ufbxt_assert(18 == (int)round(100.0f * material->pbr.transmission_color.value_vec3.z));
	ufbxt_assert(19 == (int)round(100.0f * material->pbr.transmission_depth.value_vec3.x));
	ufbxt_assert(20 == (int)round(100.0f * material->pbr.transmission_scatter.value_vec3.x));
	ufbxt_assert(21 == (int)round(100.0f * material->pbr.transmission_scatter.value_vec3.y));
	ufbxt_assert(22 == (int)round(100.0f * material->pbr.transmission_scatter.value_vec3.z));
	ufbxt_assert(23 == (int)round(100.0f * material->pbr.transmission_scatter_anisotropy.value_vec3.x));
	ufbxt_assert(24 == (int)round(100.0f * material->pbr.transmission_dispersion.value_vec3.x));
	ufbxt_assert(25 == (int)round(100.0f * material->pbr.transmission_extra_roughness.value_vec3.x));
	ufbxt_assert(26 == (int)round(100.0f * material->pbr.subsurface_factor.value_vec3.x));
	ufbxt_assert(27 == (int)round(100.0f * material->pbr.subsurface_color.value_vec3.x));
	ufbxt_assert(28 == (int)round(100.0f * material->pbr.subsurface_color.value_vec3.y));
	ufbxt_assert(29 == (int)round(100.0f * material->pbr.subsurface_color.value_vec3.z));
	ufbxt_assert(30 == (int)round(100.0f * material->pbr.subsurface_radius.value_vec3.x));
	ufbxt_assert(31 == (int)round(100.0f * material->pbr.subsurface_radius.value_vec3.y));
	ufbxt_assert(32 == (int)round(100.0f * material->pbr.subsurface_radius.value_vec3.z));
	ufbxt_assert(33 == (int)round(100.0f * material->pbr.subsurface_scale.value_vec3.x));
	ufbxt_assert(34 == (int)round(100.0f * material->pbr.subsurface_anisotropy.value_vec3.x));
	ufbxt_assert(35 == (int)round(100.0f * material->pbr.coat_factor.value_vec3.x));
	ufbxt_assert(36 == (int)round(100.0f * material->pbr.coat_color.value_vec3.x));
	ufbxt_assert(37 == (int)round(100.0f * material->pbr.coat_color.value_vec3.y));
	ufbxt_assert(38 == (int)round(100.0f * material->pbr.coat_color.value_vec3.z));
	ufbxt_assert(39 == (int)round(100.0f * material->pbr.coat_roughness.value_vec3.x));
	ufbxt_assert(40 == (int)round(100.0f * material->pbr.coat_ior.value_vec3.x));
	ufbxt_assert(41 == (int)round(100.0f * material->pbr.coat_anisotropy.value_vec3.x));
	ufbxt_assert(42 == (int)round(100.0f * material->pbr.coat_rotation.value_vec3.x));
	// Not used: ufbxt_assert(43 == (int)round(100.0f * material->pbr.coat_normal.value_vec3.x));
	// Not used: ufbxt_assert(44 == (int)round(100.0f * material->pbr.coat_normal.value_vec3.y));
	// Not used: ufbxt_assert(45 == (int)round(100.0f * material->pbr.coat_normal.value_vec3.z));
	ufbxt_assert(46 == (int)round(100.0f * material->pbr.sheen_factor.value_vec3.x));
	ufbxt_assert(47 == (int)round(100.0f * material->pbr.sheen_color.value_vec3.x));
	ufbxt_assert(48 == (int)round(100.0f * material->pbr.sheen_color.value_vec3.y));
	ufbxt_assert(49 == (int)round(100.0f * material->pbr.sheen_color.value_vec3.z));
	ufbxt_assert(50 == (int)round(100.0f * material->pbr.sheen_roughness.value_vec3.x));
	ufbxt_assert(51 == (int)round(100.0f * material->pbr.emission_factor.value_vec3.x));
	ufbxt_assert(52 == (int)round(100.0f * material->pbr.emission_color.value_vec3.x));
	ufbxt_assert(53 == (int)round(100.0f * material->pbr.emission_color.value_vec3.y));
	ufbxt_assert(54 == (int)round(100.0f * material->pbr.emission_color.value_vec3.z));
	ufbxt_assert(55 == (int)round(100.0f * material->pbr.thin_film_thickness.value_vec3.x));
	ufbxt_assert(56 == (int)round(100.0f * material->pbr.thin_film_ior.value_vec3.x));
	ufbxt_assert(57 == (int)round(100.0f * material->pbr.opacity.value_vec3.x));
	ufbxt_assert(58 == (int)round(100.0f * material->pbr.opacity.value_vec3.y));
	ufbxt_assert(59 == (int)round(100.0f * material->pbr.opacity.value_vec3.z));

	// Computed
	ufbxt_assert_close_real(err, material->pbr.transmission_roughness.value_real, 0.36f); // 11+25
	ufbxt_assert(material->pbr.thin_film_factor.value_real == 1.0f);

	ufbxt_assert(material->features.thin_walled.enabled);
	ufbxt_assert(material->features.thin_walled.is_explicit);
}
#endif

UFBXT_FILE_TEST(maya_texture_layers)
#if UFBXT_IMPL
{
	// TODO: Recover layered textures from <7000.....
	if (scene->metadata.version < 7000) return;

	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh && node->mesh->materials.count == 1);
	ufbx_material *material = node->mesh->materials.data[0];

	ufbx_texture *layered = material->fbx.diffuse_color.texture;
	ufbxt_assert(layered);
	ufbxt_assert(layered->type == UFBX_TEXTURE_LAYERED);
	ufbxt_assert(layered->layers.count == 3);
	ufbxt_assert(layered->layers.data[0].blend_mode == UFBX_BLEND_MULTIPLY);
	ufbxt_assert_close_real(err, layered->layers.data[0].alpha, 0.75f);
	ufbxt_assert(!strcmp(layered->layers.data[0].texture->relative_filename.data, "textures\\checkerboard_weight.png"));
	ufbxt_assert(layered->layers.data[1].blend_mode == UFBX_BLEND_OVER);
	ufbxt_assert_close_real(err, layered->layers.data[1].alpha, 0.5f);
	ufbxt_assert(!strcmp(layered->layers.data[1].texture->relative_filename.data, "textures\\checkerboard_diffuse.png"));
	ufbxt_assert(layered->layers.data[2].blend_mode == UFBX_BLEND_ADDITIVE);
	ufbxt_assert_close_real(err, layered->layers.data[2].alpha, 1.0f);
	ufbxt_assert(!strcmp(layered->layers.data[2].texture->relative_filename.data, "textures\\checkerboard_ambient.png"));

	ufbxt_assert(layered->file_textures.count == 3);
	for (size_t i = 0; i < 3; i++) {
		ufbxt_assert(layered->file_textures.data[i] == layered->layers.data[i].texture);
	}

	{
		ufbx_texture *texture = material->fbx.emission_color.texture;
		ufbxt_assert(texture);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_emissive.png"));
		ufbxt_assert_close_real(err, texture->uv_transform.translation.x, 1.0f);
		ufbxt_assert_close_real(err, texture->uv_transform.translation.y, 2.0f);
		ufbxt_assert_close_real(err, texture->uv_transform.translation.z, 0.0f);

		ufbx_vec3 uv = { 0.5f, 0.5f, 0.0f };
		uv = ufbx_transform_position(&texture->uv_to_texture, uv);
		ufbxt_assert_close_real(err, uv.x, -0.5f);
		ufbxt_assert_close_real(err, uv.y, -1.5f);
		ufbxt_assert_close_real(err, uv.z, 0.0f);
	}

	{
		ufbx_texture *texture = material->fbx.transparency_color.texture;
		ufbxt_assert(texture);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_transparency.png"));
		ufbxt_assert_close_real(err, texture->uv_transform.translation.x, 0.5f);
		ufbxt_assert_close_real(err, texture->uv_transform.translation.y, -0.20710678f);
		ufbxt_assert_close_real(err, texture->uv_transform.translation.z, 0.0f);
		ufbx_vec3 euler = ufbx_quat_to_euler(texture->uv_transform.rotation, UFBX_ROTATION_ORDER_XYZ);
		ufbxt_assert_close_real(err, euler.x, 0.0f);
		ufbxt_assert_close_real(err, euler.y, 0.0f);
		ufbxt_assert_close_real(err, euler.z, 45.0f);

		{
			ufbx_vec3 uv = { 0.5f, 0.5f, 0.0f };
			uv = ufbx_transform_position(&texture->uv_to_texture, uv);
			ufbxt_assert_close_real(err, uv.x, 0.5f);
			ufbxt_assert_close_real(err, uv.y, 0.5f);
			ufbxt_assert_close_real(err, uv.z, 0.0f);
		}

		{
			ufbx_vec3 uv = { 1.0f, 0.5f, 0.0f };
			uv = ufbx_transform_position(&texture->uv_to_texture, uv);
			ufbxt_assert_close_real(err, uv.x, 0.853553f);
			ufbxt_assert_close_real(err, uv.y, 0.146447f);
			ufbxt_assert_close_real(err, uv.z, 0.0f);
		}
	}
}
#endif

UFBXT_FILE_TEST(maya_texture_blend_modes)
#if UFBXT_IMPL
{
	// TODO: Recover layered textures from <7000.....
	if (scene->metadata.version < 7000) return;

	ufbx_node *node = ufbx_find_node(scene, "pCube1");
	ufbxt_assert(node && node->mesh && node->mesh->materials.count == 1);
	ufbx_material *material = node->mesh->materials.data[0];

	ufbx_texture *layered = material->fbx.diffuse_color.texture;
	ufbxt_assert(layered);
	ufbxt_assert(layered->type == UFBX_TEXTURE_LAYERED);
	ufbxt_assert(layered->layers.count == 14);

	for (size_t i = 0; i < layered->layers.count; i++) {
		ufbx_real alpha = i < 10 ? (ufbx_real)(i + 1) * 0.1f : 1.0f;
		size_t ix = layered->layers.count - i - 1;
		ufbxt_assert_close_real(err, layered->layers.data[ix].alpha, alpha);
	}

	// All point to the same texture and should be deduplicated
	ufbxt_assert(layered->file_textures.count == 1);
	for (size_t i = 0; i < 14; i++) {
		ufbxt_assert(layered->layers.data[i].texture == layered->file_textures.data[0]);
	}

	ufbxt_assert(layered->layers.data[ 0].blend_mode == UFBX_BLEND_REPLACE);    // "CPV Modulate" (unsupported)
	ufbxt_assert(layered->layers.data[ 1].blend_mode == UFBX_BLEND_LUMINOSITY); // "Illuminate"
	ufbxt_assert(layered->layers.data[ 2].blend_mode == UFBX_BLEND_REPLACE);    // "Desaturate" (unsupported)
	ufbxt_assert(layered->layers.data[ 3].blend_mode == UFBX_BLEND_SATURATION); // "Saturate"
	ufbxt_assert(layered->layers.data[ 4].blend_mode == UFBX_BLEND_DARKEN);     // "Darken"
	ufbxt_assert(layered->layers.data[ 5].blend_mode == UFBX_BLEND_LIGHTEN);    // "Lighten"
	ufbxt_assert(layered->layers.data[ 6].blend_mode == UFBX_BLEND_DIFFERENCE); // "Difference"
	ufbxt_assert(layered->layers.data[ 7].blend_mode == UFBX_BLEND_MULTIPLY);   // "Multiply"
	ufbxt_assert(layered->layers.data[ 8].blend_mode == UFBX_BLEND_SUBTRACT);   // "Subtract"
	ufbxt_assert(layered->layers.data[ 9].blend_mode == UFBX_BLEND_ADDITIVE);   // "Add"
	ufbxt_assert(layered->layers.data[10].blend_mode == UFBX_BLEND_REPLACE);    // "Out" (unsupported)
	ufbxt_assert(layered->layers.data[11].blend_mode == UFBX_BLEND_REPLACE);    // "In" (unsupported)
	ufbxt_assert(layered->layers.data[12].blend_mode == UFBX_BLEND_OVER);       // "Over"
	ufbxt_assert(layered->layers.data[13].blend_mode == UFBX_BLEND_REPLACE);    // "None"
}
#endif

UFBXT_FILE_TEST(max_texture_mapping)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "01 - Default");
	ufbxt_assert(material);

	{
		ufbx_texture *texture = material->pbr.base_factor.texture;
		ufbxt_assert(texture && texture->shader);
		ufbx_shader_texture *shader = texture->shader;
		ufbxt_assert(texture->type == UFBX_TEXTURE_FILE);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_weight.png"));
		ufbxt_assert(shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(shader->shader_name.data, "OSLBitmap2"));

		uint32_t source_hash = ufbxt_fnv1a(shader->shader_source.data, shader->shader_source.length);
		ufbxt_assert(source_hash == 0x599994a9u);

		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(texture->file_textures.data[0] == texture);
	}

	{
		ufbx_texture *texture = material->pbr.base_color.texture;
		ufbxt_assert(texture && texture->shader);
		ufbx_shader_texture *shader = texture->shader;
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(shader->shader_name.data, "RandomTilingBitmap"));
		uint32_t source_hash = ufbxt_fnv1a(shader->shader_source.data, shader->shader_source.length);
		ufbxt_assert(source_hash == 0x60186ba7u);

		{
			ufbx_shader_texture_input *input = ufbx_find_shader_texture_input(shader, "Filename");
			ufbxt_assert(input);
			ufbxt_assert(!strcmp(input->value_str.data, "D:\\Dev\\clean\\ufbx\\data\\textures\\checkerboard_diffuse.png"));
		}

		{
			ufbx_shader_texture_input *input = ufbx_find_shader_texture_input(shader, "Scale");
			ufbxt_assert(input);
			ufbxt_assert_close_real(err, input->value_real, 0.25f);
		}

		ufbxt_assert(texture->file_textures.count == 0);
	}

	{
		ufbx_texture *texture = material->pbr.roughness.texture;
		ufbxt_assert(texture && texture->shader);
		ufbx_shader_texture *shader = texture->shader;
		ufbxt_assert(texture->type == UFBX_TEXTURE_FILE);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_roughness.png"));
		ufbxt_assert(shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(shader->shader_name.data, "UberBitmap2"));

		uint32_t source_hash = ufbxt_fnv1a(shader->shader_source.data, shader->shader_source.length);
		ufbxt_assert(source_hash == 0x19833d47);

		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(texture->file_textures.data[0] == texture);
	}

	{
		ufbx_texture *texture = material->pbr.metalness.texture;
		ufbxt_assert(texture && !texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_FILE);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_metallic.png"));

		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(texture->file_textures.data[0] == texture);
	}

	{
		ufbx_texture *texture = material->pbr.transmission_color.texture;
		ufbxt_assert(texture && texture->shader);
		ufbx_shader_texture *shader = texture->shader;
		ufbxt_assert(texture->type == UFBX_TEXTURE_FILE);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_transparency.png"));
		ufbxt_assert(shader->type == UFBX_SHADER_TEXTURE_UNKNOWN);
		ufbxt_assert(!strcmp(shader->shader_name.data, "ai_image"));
		ufbxt_assert(shader->shader_source.length == 0);

		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(texture->file_textures.data[0] == texture);
	}

	{
		ufbx_texture *texture = material->pbr.transmission_color.texture;
		ufbxt_assert(texture && texture->shader);
		ufbx_shader_texture *shader = texture->shader;
		ufbxt_assert(texture->type == UFBX_TEXTURE_FILE);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_transparency.png"));
		ufbxt_assert(shader->type == UFBX_SHADER_TEXTURE_UNKNOWN);
		ufbxt_assert(shader->shader_source.length == 0);

		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(texture->file_textures.data[0] == texture);
	}

	{
		ufbx_texture *texture = material->pbr.emission_color.texture;
		ufbxt_assert(texture && texture->shader);
		ufbx_shader_texture *shader = texture->shader;
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(shader->shader_name.data, "ColorTweak"));

		uint32_t source_hash = ufbxt_fnv1a(shader->shader_source.data, shader->shader_source.length);
		ufbxt_assert(source_hash == 0xd2f4f86f);

		ufbx_shader_texture_input *input = ufbx_find_shader_texture_input(shader, "Input");
		ufbxt_assert(input);
		ufbx_texture *input_texture = input->texture;
		ufbxt_assert(input_texture);
		ufbxt_assert(input_texture->type == UFBX_TEXTURE_FILE);

		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(texture->file_textures.data[0] == input_texture);
	}

	{
		ufbx_texture *texture = material->pbr.normal_map.texture;
		ufbxt_assert(texture && texture->shader);
		ufbx_shader_texture *shader = texture->shader;
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(shader->type == UFBX_SHADER_TEXTURE_UNKNOWN);
		ufbxt_assert(!strcmp(shader->shader_name.data, "ai_bump2d"));
		ufbxt_assert(shader->shader_source.length == 0);

		ufbx_shader_texture_input *bump_map = ufbx_find_shader_texture_input(shader, "bump_map");
		ufbxt_assert(bump_map);
		ufbx_texture *bump_texture = bump_map->texture;
		ufbxt_assert(bump_texture);
		ufbxt_assert(bump_texture->type == UFBX_TEXTURE_FILE);

		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(texture->file_textures.data[0] == bump_texture);
	}

}
#endif

UFBXT_FILE_TEST(synthetic_missing_material_factor)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "phong1");
	ufbxt_assert(material);

	ufbxt_assert(material->shader_type == UFBX_SHADER_FBX_PHONG);

	ufbxt_assert_close_real(err, material->fbx.diffuse_factor.value_vec3.x, 1.0f);
	ufbxt_assert_close_real(err, material->fbx.specular_factor.value_vec3.x, 1.0f);
	ufbxt_assert_close_real(err, material->fbx.reflection_factor.value_vec3.x, 1.0f);
	ufbxt_assert_close_real(err, material->fbx.transparency_factor.value_vec3.x, 1.0f);
	ufbxt_assert_close_real(err, material->fbx.emission_factor.value_vec3.x, 1.0f);
	ufbxt_assert_close_real(err, material->fbx.ambient_factor.value_vec3.x, 1.0f);

	// TODO: Figure out how to map transmission/opacity
	ufbxt_assert_close_real(err, material->pbr.base_factor.value_vec3.x, 1.0f);
	ufbxt_assert_close_real(err, material->pbr.specular_factor.value_vec3.x, 1.0f);
	ufbxt_assert_close_real(err, material->pbr.emission_factor.value_vec3.x, 1.0f);
}
#endif

UFBXT_FILE_TEST(max_instanced_material)
#if UFBXT_IMPL
{
	ufbx_node *red_node = ufbx_find_node(scene, "Red");
	ufbx_node *green_node = ufbx_find_node(scene, "Green");
	ufbx_node *blue_node = ufbx_find_node(scene, "Blue");
	ufbxt_assert(red_node && red_node->mesh);
	ufbxt_assert(green_node && green_node->mesh);
	ufbxt_assert(blue_node && blue_node->mesh);
	ufbx_mesh *red_mesh = red_node->mesh;
	ufbx_mesh *green_mesh = green_node->mesh;
	ufbx_mesh *blue_mesh = blue_node->mesh;

	ufbx_material *red_mat = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "RedMat");
	ufbx_material *green_mat = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "GreenMat");
	ufbx_material *blue_mat = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "BlueMat");

	if (scene->metadata.version < 7000) {
		ufbxt_assert(red_mesh != green_mesh);
		ufbxt_assert(red_mesh != blue_mesh);
		ufbxt_assert(green_mesh != blue_mesh);

		// 6100 is a bit broken with materials?

		ufbxt_assert(green_node->materials.count == 3);
		ufbxt_assert(red_node->materials.count == 2);
		ufbxt_assert(blue_node->materials.count == 1);

		ufbxt_assert(green_mesh->materials.count == 3);
		ufbxt_assert(red_mesh->materials.count == 2);
		ufbxt_assert(blue_mesh->materials.count == 1);

	} else {
		ufbxt_assert(red_mesh == green_mesh);
		ufbxt_assert(red_mesh == blue_mesh);
		ufbxt_assert(green_mesh == blue_mesh);

		ufbxt_assert(green_node->materials.count == 1);
		ufbxt_assert(red_node->materials.count == 1);
		ufbxt_assert(blue_node->materials.count == 1);

		ufbxt_assert(green_node->materials.data[0] == green_mat);
		ufbxt_assert(red_node->materials.data[0] == red_mat);
		ufbxt_assert(blue_node->materials.data[0] == blue_mat);

		ufbxt_assert(red_mesh->materials.data[0] == green_mat);
	}
}
#endif

#if UFBXT_IMPL

static bool ufbxt_has_texture(ufbx_texture_list list, const char *name)
{
	for (size_t i = 0; i < list.count; i++) {
		if (!strcmp(list.data[i]->relative_filename.data, name)) return true;
	}
	return false;
}

static void ufbxt_check_shader_input_map(ufbx_texture *texture, const char *name, const char *map, int64_t output_index)
{
	ufbxt_assert(texture->shader);
	ufbx_shader_texture_input *input = ufbx_find_shader_texture_input(texture->shader, name);
	ufbxt_assert(input);
	ufbxt_assert(input->texture);
	ufbxt_assert(!strcmp(input->texture->name.data, map));
	ufbxt_assert(input->texture_output_index == output_index);
}

#endif

UFBXT_FILE_TEST(max_shadergraph)
#if UFBXT_IMPL
{
	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #1");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_UNKNOWN);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "ai_checkerboard"));
		ufbxt_assert(texture->file_textures.count == 0);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #2");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_UNKNOWN);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "ai_add"));
		ufbxt_assert(texture->file_textures.count == 0);
		ufbxt_check_shader_input_map(texture, "input1", "Map #1", 0);
		ufbxt_check_shader_input_map(texture, "input2", "Map #3", 0);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #3");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "Checker"));
		ufbxt_assert(texture->file_textures.count == 0);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #6");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "ColorMul"));
		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_diffuse.png"));
		ufbxt_check_shader_input_map(texture, "A", "Map #2", 0);
		ufbxt_check_shader_input_map(texture, "B", "Map #7", 0);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #7");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_FILE);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_diffuse.png"));
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "OSLBitmap2"));
		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_diffuse.png"));
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #11");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_FILE);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_metallic.png"));
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_UNKNOWN);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "ai_image"));
		ufbxt_assert(texture->file_textures.count == 2);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_diffuse.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_metallic.png"));
		ufbxt_check_shader_input_map(texture, "offset", "Map #7", 2);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #13");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "ColorMax"));
		ufbxt_assert(texture->file_textures.count == 2);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_diffuse.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_weight.png"));
		ufbxt_check_shader_input_map(texture, "A", "Map #15", 0);
		ufbxt_check_shader_input_map(texture, "B", "Map #6", 0);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #14");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_FILE);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_weight.png"));
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "UberBitmap2"));
		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_weight.png"));
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #15");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "TriTone"));
		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_weight.png"));
		ufbxt_check_shader_input_map(texture, "Input_map", "Map #14", 0);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #16");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_UNKNOWN);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "ai_mix_rgba"));
		ufbxt_assert(texture->file_textures.count == 3);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_diffuse.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_metallic.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_weight.png"));
		ufbxt_check_shader_input_map(texture, "input1", "Map #13", 0);
		ufbxt_check_shader_input_map(texture, "input2", "Map #11", 0);
		ufbxt_check_shader_input_map(texture, "mix", "Map #7", 1);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #17");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_UNKNOWN);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "ai_dot"));
		ufbxt_assert(texture->file_textures.count == 3);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_diffuse.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_metallic.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_weight.png"));
		ufbxt_check_shader_input_map(texture, "input1", "Map #16", 0);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #19");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "ColorAdd"));
		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_emissive.png"));
		ufbxt_check_shader_input_map(texture, "A", "Map #20", 0);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #20");
		ufbxt_assert(texture && !texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_FILE);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_emissive.png"));
		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_emissive.png"));
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #21");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_UNKNOWN);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "ai_add"));
		ufbxt_assert(texture->file_textures.count == 2);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_emissive.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_specular.png"));
		ufbxt_check_shader_input_map(texture, "input1", "Map #22", 0);
		ufbxt_check_shader_input_map(texture, "input2", "Map #19", 0);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #22");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_FILE);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_specular.png"));
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_UNKNOWN);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "ai_image"));
		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_specular.png"));
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #26");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "VectorAdd"));
		ufbxt_assert(texture->file_textures.count == 3);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_emissive.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_specular.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_transparency.png"));
		ufbxt_check_shader_input_map(texture, "A", "Map #28", 0);
		ufbxt_check_shader_input_map(texture, "B", "Map #21", 0);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #28");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_FILE);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_transparency.png"));
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "OSLBitmap2"));
		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_transparency.png"));
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #29");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "ColorAdd"));
		ufbxt_assert(texture->file_textures.count == 4);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_emissive.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_specular.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_transparency.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_ambient.png"));
		ufbxt_check_shader_input_map(texture, "A", "Map #30", 0);
		ufbxt_check_shader_input_map(texture, "B", "Map #26", 0);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #30");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_FILE);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_ambient.png"));
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_OSL);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, "UberBitmap2"));
		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_ambient.png"));
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #31");
		ufbxt_assert(texture && texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_SHADER);
		ufbxt_assert(texture->shader->type == UFBX_SHADER_TEXTURE_UNKNOWN);
		ufbxt_assert(!strcmp(texture->shader->shader_name.data, ""));
		ufbxt_assert(texture->shader->shader_type_id == UINT64_C(0x0000023000000000));
		ufbxt_assert(texture->file_textures.count == 5);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_emissive.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_specular.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_transparency.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_ambient.png"));
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_displacement.png"));
		ufbxt_check_shader_input_map(texture, "map1", "Map #29", 0);
		ufbxt_check_shader_input_map(texture, "map2", "Map #32", 0);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "Map #32");
		ufbxt_assert(texture && !texture->shader);
		ufbxt_assert(texture->type == UFBX_TEXTURE_FILE);
		ufbxt_assert(!strcmp(texture->relative_filename.data, "textures\\checkerboard_displacement.png"));
		ufbxt_assert(texture->file_textures.count == 1);
		ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_displacement.png"));
	}

	{
		ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "Material #25");
		ufbxt_assert(material);
		ufbxt_assert(material->shader_type == UFBX_SHADER_OSL_STANDARD_SURFACE);

		ufbxt_assert(material->pbr.base_factor.texture);
		ufbxt_assert(!strcmp(material->pbr.base_factor.texture->name.data, "Map #17"));

		ufbxt_assert(material->pbr.base_color.texture);
		ufbxt_assert(!strcmp(material->pbr.base_color.texture->name.data, "Map #13"));

		ufbxt_assert(material->pbr.emission_factor.texture);
		ufbxt_assert(!strcmp(material->pbr.emission_factor.texture->name.data, "Map #31"));

		{
			ufbx_texture *texture = material->pbr.roughness.texture;
			ufbxt_assert(texture);
			ufbxt_assert(texture->shader);
			ufbxt_assert(texture->file_textures.count == 1);
			ufbxt_assert(ufbxt_has_texture(texture->file_textures, "textures\\checkerboard_ambient.png"));
			ufbxt_assert(texture->shader->main_texture);
			ufbxt_assert(texture->shader->main_texture_output_index == 6);
			ufbxt_assert(!strcmp(texture->shader->main_texture->name.data, "Map #30"));
			ufbxt_check_shader_input_map(texture, "sourceMap", "Map #30", 6);
		}
	}
}
#endif

UFBXT_FILE_TEST(maya_duplicated_texture)
#if UFBXT_IMPL
{
	ufbxt_assert(scene->texture_files.count == 1);
	ufbxt_assert(scene->textures.count == 4);

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "file1");
		ufbxt_assert(texture->has_file);
		ufbxt_assert(texture->file_index == 0);
		ufbxt_assert(!strcmp(texture->uv_set.data, "map1"));
		ufbxt_assert(!texture->has_uv_transform);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "file2");
		ufbxt_assert(texture->has_file);
		ufbxt_assert(texture->file_index == 0);
		ufbxt_assert(!strcmp(texture->uv_set.data, "map1"));
		ufbxt_assert(!texture->has_uv_transform);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "file3");
		ufbxt_assert(texture->has_file);
		ufbxt_assert(texture->file_index == 0);
		ufbxt_assert(!strcmp(texture->uv_set.data, "map1"));
		ufbxt_assert(texture->has_uv_transform);
		ufbxt_assert_close_real(err, texture->uv_transform.scale.x, 2.0f);
		ufbxt_assert_close_real(err, texture->uv_transform.scale.y, 2.0f);
	}

	{
		ufbx_texture *texture = (ufbx_texture*)ufbx_find_element(scene, UFBX_ELEMENT_TEXTURE, "file4");
		ufbxt_assert(texture->has_file);
		ufbxt_assert(texture->file_index == 0);
		ufbxt_assert(!strcmp(texture->uv_set.data, "second"));
		ufbxt_assert(!texture->has_uv_transform);
	}
}
#endif

#if UFBXT_IMPL
static ufbx_load_opts ufbxt_slash_separator_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.path_separator = '/';
	return opts;
}
static ufbx_load_opts ufbxt_backslash_separator_opts()
{
	ufbx_load_opts opts = { 0 };
	opts.path_separator = '\\';
	return opts;
}
#endif

UFBXT_FILE_TEST_OPTS(maya_absolute_texture, ufbxt_slash_separator_opts)
#if UFBXT_IMPL
{
	ufbx_material *material = ufbx_find_material(scene, "lambert1");
	ufbxt_assert(material);

	ufbx_texture *texture = material->fbx.diffuse_color.texture;
	ufbxt_assert(texture);
	ufbxt_assert(!strcmp(texture->relative_filename.data, "W:\\checkerboard_diffuse.png"));
	ufbxt_assert(!strcmp(texture->absolute_filename.data, "W:/checkerboard_diffuse.png"));
	ufbxt_assert(!strcmp(texture->filename.data, "W:/checkerboard_diffuse.png"));
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(maya_absolute_texture_backslash, maya_absolute_texture, ufbxt_backslash_separator_opts)
#if UFBXT_IMPL
{
	ufbx_material *material = ufbx_find_material(scene, "lambert1");
	ufbxt_assert(material);

	ufbx_texture *texture = material->fbx.diffuse_color.texture;
	ufbxt_assert(texture);
	ufbxt_assert(!strcmp(texture->relative_filename.data, "W:\\checkerboard_diffuse.png"));
	ufbxt_assert(!strcmp(texture->absolute_filename.data, "W:/checkerboard_diffuse.png"));
	ufbxt_assert(!strcmp(texture->filename.data, "W:\\checkerboard_diffuse.png"));
}
#endif

#if UFBXT_IMPL
static ufbx_vec3 ufbxt_load_pixel(ufbx_texture *texture)
{
	const char *error = NULL;
	ufbxt_image16 image = ufbxt_read_png(texture->content.data, texture->content.size, &error);
	if (error) ufbxt_logf("failed to read embedded png %s: %s", texture->relative_filename.data, error);
	ufbxt_assert(image.width >= 1 && image.height >= 1);
	ufbxt_pixel16 pixel = image.pixels[0];
	ufbx_vec3 color;
	color.x = ufbxt_srgb_to_linear((ufbx_real)pixel.r / 65535.0f);
	color.y = ufbxt_srgb_to_linear((ufbx_real)pixel.g / 65535.0f);
	color.z = ufbxt_srgb_to_linear((ufbx_real)pixel.b / 65535.0f);
	free(image.pixels);
	return color;
}
static void ufbxt_check_chart_material(ufbxt_diff_error *err, ufbx_material *mat, ufbx_vec3 ref)
{
	ufbxt_hintf("material = %s", mat->name.data);

	ufbx_texture *base_texture = mat->pbr.base_color.texture;
	ufbx_vec3 base_color = mat->pbr.base_color.value_vec3;
	ufbx_real base_factor = mat->pbr.base_factor.value_real;
	if (base_texture) {
		base_color = ufbxt_load_pixel(base_texture);
	}

	ufbx_texture *emit_texture = mat->pbr.emission_color.texture;
	ufbx_vec3 emit_color = mat->pbr.emission_color.value_vec3;
	ufbx_real emit_factor = mat->pbr.emission_factor.value_real;
	if (emit_texture) {
		emit_color = ufbxt_load_pixel(emit_texture);
	}

	ufbx_vec3 sum = ufbxt_add3(ufbxt_mul3(base_color, base_factor), ufbxt_mul3(emit_color, emit_factor));
	if (base_texture || emit_texture) {
		ufbxt_assert_close_vec3_threshold(err, sum, ref, 0.01f);
	} else {
		ufbxt_assert_close_vec3(err, sum, ref);
	}
}
#endif

UFBXT_FILE_TEST(maya_material_chart)
#if UFBXT_IMPL
{
	ufbx_display_layer *layer = ufbx_as_display_layer(ufbx_find_element(scene, UFBX_ELEMENT_DISPLAY_LAYER, "Sheets"));
	ufbxt_assert(layer);
	ufbxt_assert(layer->nodes.count == 12);

	ufbx_vec3 ref = { 0.125f, 0.25f, 0.5f };
	for (size_t i = 0; i < layer->nodes.count; i++) {
		ufbx_node *node = layer->nodes.data[i];
		ufbxt_assert(node->mesh);
		ufbxt_assert(node->materials.count == 1);
		ufbx_material *material = node->materials.data[0];
		ufbxt_check_chart_material(err, material, ref);
	}
}
#endif

UFBXT_FILE_TEST(blender_402_material_chart)
#if UFBXT_IMPL
{
	ufbx_vec3 ref = { 0.25f, 0.125f, 0.5f };
	ufbxt_assert(scene->materials.count == 6);
	for (size_t i = 0; i < scene->materials.count; i++) {
		ufbx_material *material = scene->materials.data[i];
		ufbxt_check_chart_material(err, material, ref);
	}
}
#endif

UFBXT_FILE_TEST(blender_suzanne_multimaterial)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Suzanne");
	ufbxt_assert(node);
	ufbxt_assert(node->mesh);
	ufbx_mesh *mesh = node->mesh;

	static const uint32_t order_ref[] = { 0, 4, 6, 1, 5, 2, 3 };
	ufbxt_assert(mesh->material_part_usage_order.count == ufbxt_arraycount(order_ref));
	for (size_t i = 0; i < mesh->material_part_usage_order.count; i++) {
		ufbxt_assert(mesh->material_part_usage_order.data[i] == order_ref[i]);
	}
}
#endif

UFBXT_FILE_TEST(blender_suzanne_multimaterial_reorder)
#if UFBXT_IMPL
{
	ufbx_node *node = ufbx_find_node(scene, "Suzanne");
	ufbxt_assert(node);
	ufbxt_assert(node->mesh);
	ufbx_mesh *mesh = node->mesh;

	static const uint32_t order_ref[] = { 4, 0, 6, 1, 5, 2, 3 };
	ufbxt_assert(mesh->material_part_usage_order.count == ufbxt_arraycount(order_ref));
	for (size_t i = 0; i < mesh->material_part_usage_order.count; i++) {
		ufbxt_assert(mesh->material_part_usage_order.data[i] == order_ref[i]);
	}
}
#endif

UFBXT_FILE_TEST(synthetic_embedded_base64)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "lambert1");
	ufbxt_assert(material);

	ufbxt_check_texture_content(scene, material->fbx.diffuse_color.texture, "tiny_clouds.png");
}
#endif

UFBXT_FILE_TEST_OPTS_ALT(synthetic_embedded_base64_dom, synthetic_embedded_base64, ufbxt_retain_dom_opts)
#if UFBXT_IMPL
{
	ufbx_material *material = (ufbx_material*)ufbx_find_element(scene, UFBX_ELEMENT_MATERIAL, "lambert1");
	ufbxt_assert(material);

	ufbx_texture *texture = material->fbx.diffuse_color.texture;
	ufbxt_assert(texture);

	ufbx_video *video = texture->video;
	ufbxt_assert(video);

	ufbx_dom_node *dom_video = video->element.dom_node;
	ufbxt_assert(dom_video);

	ufbx_dom_node *dom_content = ufbx_dom_find(dom_video, "Content");
	ufbxt_assert(dom_content);

	ufbx_blob_list blobs = ufbx_dom_as_blob_list(dom_content);
	ufbxt_assert(blobs.count == 71);

	size_t total_size = 0;
	for (size_t i = 0; i < blobs.count; i++) {
		total_size += blobs.data[i].size;
	}

	size_t offset = 0;
	char *data = malloc(total_size);
	for (size_t i = 0; i < blobs.count; i++) {
		ufbx_blob blob = blobs.data[i];
		memcpy(data + offset, blob.data, blob.size);
		offset += blob.size;
	}
	ufbxt_assert(offset == total_size);

	ufbx_blob total_blob = { data, total_size };
	ufbxt_check_blob_content(total_blob, "textures/tiny_clouds.png");

	free(data);
}
#endif

