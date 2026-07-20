#define _CRT_SECURE_NO_WARNINGS

#include "picort.h"
#include <stdio.h>
#include <algorithm>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

// -- Material

struct Texture
{
	Vec3 value;
	Image image;
};

struct Material
{
	bool has_alpha = false;
	bool has_metallic = false;
	bool has_specular = false;
	bool cast_shadows = true;
	Texture base_factor;
	Texture base_color;
	Texture specular_factor;
	Texture specular_color;
	Texture roughness;
	Texture metallic;
	Texture emission_factor;
	Texture emission_color;
};

struct Light
{
	Vec3 position;
	Vec3 color;
	Real radius = 0.0f;
	bool directional = false;
};

struct TriangleInfo
{
	Vec3 v[3];
	Vec2 uv[3];
	Vec3 normal[3];
	uint32_t material = 0;
};

struct Camera
{
	bool orthographic = false;
	Vec3 origin;
	Vec3 forward;
	Vec3 plane_x;
	Vec3 plane_y;
	float dof = 0.0f;
	float dof_focus = 0.0f;
	uint32_t resolution_x = 0;
	uint32_t resolution_y = 0;
};

struct Scene
{
	Camera camera;
	BVH root;
	std::vector<TriangleInfo> triangles;
	std::vector<Material> materials;
	std::vector<Light> lights;

	Image sky;
	Real sky_factor = 0.0f;
	Real sky_rotation = 0.0f;

	Real exposure = 1.0f;
	Vec3 indirect_clamp;
	int indirect_depth = -1;
	bool bvh_heatmap = false;
};

Vec3 from_ufbx(const ufbx_vec3 &v) { return { (Real)v.x, (Real)v.y, (Real)v.z }; }

void setup_texture(Texture &texture, const ufbx_material_map &map)
{
	if (map.has_value) {
		texture.value = from_ufbx(map.value_vec3);
	}
	if (map.texture && map.texture->content.size > 0) {
		verbosef("Loading texture: %s\n", map.texture->relative_filename.data);

		ImageResult result = read_png(map.texture->content.data, map.texture->content.size);
		if (result.error) {
			fprintf(stderr, "Failed to load embedded %s: %s\n", map.texture->relative_filename.data, result.error);
			return;
		}
		texture.image = std::move(result.image);
	}
	
	if (map.texture) {
		std::string path { map.texture->filename.data, map.texture->filename.length };
		if (!ends_with(path, ".png")) {
			size_t dot = path.rfind('.');
			if (dot != std::string::npos) {
				path = path.substr(0, dot) + ".png";
			}
			ImageResult result = load_png(path.c_str());
			if (result.error) {
				fprintf(stderr, "Failed to load %s: %s\n", map.texture->relative_filename.data, result.error);
				return;
			}
			texture.image = result.image;
		}
	}
}

Scene create_scene(ufbx_scene *original_scene, const Opts &opts, int frame_offset=0)
{
	std::vector<Triangle> triangles;
	Scene result;

	bool has_time = opts.time.defined;
	double time = opts.time.value;
	if (opts.frame.defined) {
		time = opts.frame.value / original_scene->settings.frames_per_second;
		has_time = true;
	}

	if (frame_offset > 0) {
		double fps = opts.fps.defined ? opts.fps.value : original_scene->settings.frames_per_second;
		time += frame_offset / fps;
	}

	ufbx_scene *scene;
	if (has_time) {
		ufbx_evaluate_opts eval_opts = { };
		eval_opts.evaluate_skinning = true;
		eval_opts.evaluate_caches = true;
		eval_opts.load_external_files = true;

		ufbx_anim *anim = original_scene->anim;
		if (opts.animation.defined) {
			ufbx_anim_stack *stack = ufbx_find_anim_stack(original_scene, opts.animation.value.c_str());
			if (stack) {
				anim = stack->anim;
			}
		}

		ufbx_error error;
		scene = ufbx_evaluate_scene(original_scene, anim, time, &eval_opts, &error);
		if (!scene) {
			char buf[4096];
			ufbx_format_error(buf, sizeof(buf), &error);
			fprintf(stderr, "%s\n", buf);
			exit(1);
		}
	} else {
		scene = original_scene;
	}

	result.materials.resize(scene->materials.count + 1);

	// Reserve one undefined material
	result.materials[0].base_factor.value.x = 1.0f;
	result.materials[0].base_color.value = Vec3{ 0.8f, 0.8f, 0.8f };
	result.materials[0].roughness.value.x = 0.5f;
	result.materials[0].metallic.value.x = 0.0f;

	verbosef("Processing materials: %zu\n", scene->materials.count);

	for (size_t i = 0; i < scene->materials.count; i++) {
		ufbx_material *mat = scene->materials.data[i];
		Material &dst = result.materials[i + 1];
		dst.base_factor.value.x = 1.0f;
		setup_texture(dst.base_factor, mat->pbr.base_factor);
		setup_texture(dst.base_color, mat->pbr.base_color);
		setup_texture(dst.specular_factor, mat->pbr.specular_factor);
		setup_texture(dst.specular_color, mat->pbr.specular_color);
		setup_texture(dst.roughness, mat->pbr.roughness);
		setup_texture(dst.metallic, mat->pbr.metalness);
		setup_texture(dst.emission_factor, mat->pbr.emission_factor);
		setup_texture(dst.emission_color, mat->pbr.emission_color);
		dst.has_metallic = mat->features.metalness.enabled;
		dst.has_specular = mat->features.specular.enabled;
		dst.base_color.image.srgb = true;
		dst.emission_color.image.srgb = true;
		if (ufbx_find_int(&mat->props, "NoShadow", 0) != 0) {
			dst.cast_shadows = false;
		}

		if (dst.base_color.image.width > 0) {
			uint32_t num_pixels = dst.base_color.image.width * dst.base_color.image.height;
			const Pixel16 *pixels = dst.base_color.image.pixels.data();
			for (uint32_t i = 0; i < num_pixels; i++) {
				if (pixels[i].a < 0xffff) {
					dst.has_alpha = true;
					break;
				}
			}
		}
	}

	verbosef("Processing meshes: %zu\n", scene->meshes.count);

	std::vector<uint32_t> indices;
	for (ufbx_mesh *original_mesh : scene->meshes) {
		if (original_mesh->instances.count == 0) continue;
		ufbx_mesh *mesh = ufbx_subdivide_mesh(original_mesh, 0, NULL, NULL);

		if (indices.size() < mesh->max_face_triangles * 3) {
			indices.resize(mesh->max_face_triangles * 3);
		}

		// Iterate over all instances of the mesh
		for (ufbx_node *node : mesh->instances) {
			if (!node->visible) continue;

			verbosef("%s: %zu triangles\n", node->name.data, mesh->num_triangles);

			ufbx_matrix normal_to_world = ufbx_matrix_for_normals(&node->geometry_to_world);

			// Iterate over all the N-gon faces of the mesh
			for (size_t face_ix = 0; face_ix < mesh->num_faces; face_ix++) {

				// Split each face into triangles
				size_t num_tris = ufbx_triangulate_face(indices.data(), indices.size(), mesh, mesh->faces[face_ix]);

				// Iterate over reach split triangle
				for (size_t tri_ix = 0; tri_ix < num_tris; tri_ix++) {
					Triangle tri;
					TriangleInfo info;
					tri.index = result.triangles.size();

					if (mesh->face_material.count > 0) {
						ufbx_material *mat = mesh->materials.data[mesh->face_material[face_ix]];
						info.material = mat->element.typed_id + 1;
					}

					for (size_t corner_ix = 0; corner_ix < 3; corner_ix++) {
						uint32_t index = indices[tri_ix*3 + corner_ix];

						// Load the skinned vertex position at `index`
						ufbx_vec3 v = ufbx_get_vertex_vec3(&mesh->skinned_position, index);

						ufbx_vec2 uv = mesh->vertex_uv.exists ? ufbx_get_vertex_vec2(&mesh->vertex_uv, index) : ufbx_vec2{};
						ufbx_vec3 n = ufbx_get_vertex_vec3(&mesh->skinned_normal, index);

						// If the skinned positions are local we must apply `to_root` to get
						// to world coordinates
						if (mesh->skinned_is_local) {
							v = ufbx_transform_position(&node->geometry_to_world, v);
							n = ufbx_transform_direction(&normal_to_world, n);
						}

						info.v[corner_ix] = tri.v[corner_ix] = { (Real)v.x, (Real)v.y, (Real)v.z };
						info.uv[corner_ix] = { (Real)uv.x, (Real)uv.y };
						info.normal[corner_ix] = normalize(Vec3{ (Real)n.x, (Real)n.y, (Real)n.z });
					}

					triangles.push_back(tri);
					result.triangles.push_back(info);
				}
			}
		}

		// Free the potentially subdivided mesh
		ufbx_free_mesh(mesh);
	}

	for (ufbx_light *light : scene->lights) {

		// Iterate over all instances of the light
		for (ufbx_node *node : light->instances) {
			Light l;

			ufbx_prop *radius = ufbx_find_prop(&light->props, "Radius");
			if (!radius) radius = ufbx_find_prop(&node->props, "Radius");

			if (light->type == UFBX_LIGHT_DIRECTIONAL) {
				ufbx_vec3 dir = ufbx_transform_direction(&node->node_to_world, light->local_direction);
				l.position = normalize(-from_ufbx(dir));
				l.directional = true;
			} else {
				ufbx_transform world_transform = ufbx_matrix_to_transform(&node->node_to_world);
				l.position = from_ufbx(world_transform.translation);
			}

			l.color = from_ufbx(light->color) * (Real)light->intensity;
			if (radius) l.radius = (Real)radius->value_real;

			result.lights.push_back(l);
		}
	}

	result.camera.resolution_x = (uint32_t)opts.resolution.value.x;
	result.camera.resolution_y = (uint32_t)opts.resolution.value.y;

	{
		ufbx_node *camera_node = ufbx_find_node(scene, opts.camera.value.c_str());
		if (!camera_node && scene->cameras.count > 0) {
			camera_node = scene->cameras[0]->instances[0];
		}

		if (camera_node && camera_node->camera && !opts.camera_position.defined) {
			ufbx_camera *cam = camera_node->camera;

			Vec3 m0 = normalize(from_ufbx(camera_node->node_to_world.cols[0]));
			Vec3 m1 = normalize(from_ufbx(camera_node->node_to_world.cols[1]));
			Vec3 m2 = normalize(from_ufbx(camera_node->node_to_world.cols[2]));
			Vec3 m3 = from_ufbx(camera_node->node_to_world.cols[3]);

			if (!opts.resolution.defined) {
				if (cam->resolution_is_pixels) {
					result.camera.resolution_x = (uint32_t)round(cam->resolution.x);
					result.camera.resolution_y = (uint32_t)round(cam->resolution.y);
				} else if (cam->aspect_mode != UFBX_ASPECT_MODE_WINDOW_SIZE) {
					result.camera.resolution_x = (uint32_t)((double)result.camera.resolution_y * cam->resolution.x / cam->resolution.y);
				}
			}

			if (cam->projection_mode == UFBX_PROJECTION_MODE_PERSPECTIVE) {
				result.camera.plane_x = m2 * (Real)cam->field_of_view_tan.x;
				result.camera.plane_y = m1 * (Real)cam->field_of_view_tan.y;
			} else {
				result.camera.plane_x = m2 * (Real)cam->orthographic_size.x * 0.5f;
				result.camera.plane_y = m1 * (Real)cam->orthographic_size.y * 0.5f;
				result.camera.orthographic = true;
			}
			result.camera.forward = m0;
			result.camera.origin = m3;
		} else {
			Vec3 forward = normalize(opts.camera_direction.value);
			if (opts.camera_target.defined) {
				forward = normalize(opts.camera_target.value - opts.camera_position.value);
			}
			Vec3 right = normalize(cross(forward, opts.camera_up.value));
			Vec3 up = normalize(cross(right, forward));
			Real aspect = (Real)result.camera.resolution_x / (Real)result.camera.resolution_y;
			Real tan_fov = tan(opts.camera_fov.value * 0.5f * (Pi / 180.0f));

			result.camera.plane_x = right * tan_fov * aspect;
			result.camera.plane_y = up * tan_fov;
			result.camera.forward = forward;
			result.camera.origin = opts.camera_position.value;
		}
	}

	double res_scale = opts.resolution_scale.value;
	if (res_scale != 1.0) {
		result.camera.resolution_x = (uint32_t)(result.camera.resolution_x * res_scale);
		result.camera.resolution_y = (uint32_t)(result.camera.resolution_y * res_scale);
	}

	verbosef("Building BVH: %zu triangles\n", triangles.size());
	result.root = build_bvh(triangles.data(), triangles.size());

	if (scene != original_scene) {
		ufbx_free_scene(scene);
	}

	return result;
}

// -- Materials

struct Surface
{
	Vec3 diffuse;
	Vec3 specular;
	Vec3 emission;
	Real alpha = 1.0f;
	Real roughness = 0.0f;
};

Vec3 eval_texture(const Texture &texture, const Vec2 &uv)
{
	if (texture.image.width > 0) {
		Vec4 v = sample_image(texture.image, uv);
		return { v.x, v.y, v.z };
	}
	return texture.value;
}

Surface eval_surface(const Material &material, const Vec2 &uv)
{
	Vec3 base_color = eval_texture(material.base_color, uv) * eval_texture(material.base_factor, uv).x;
	Vec3 specular_color = eval_texture(material.specular_color, uv) * eval_texture(material.specular_factor, uv).x;
	Vec3 emission = eval_texture(material.emission_color, uv) * eval_texture(material.emission_factor, uv).x;
	Surface s;
	s.diffuse = base_color;
	if (material.has_metallic) {
		Real metallic = eval_texture(material.metallic, uv).x;
		s.diffuse = lerp(base_color, Vec3{}, metallic);
		s.specular = lerp(Vec3{0.04f,0.04f,0.04f}, base_color, metallic);
	}
	if (material.has_specular) {
		s.specular = s.specular * specular_color;
	}
	s.emission = emission;
	s.roughness = max(0.05f, eval_texture(material.roughness, uv).x); // TODO??
	return s;
}

Vec3 schlick_f(const Vec3 &f0, const Vec3 &wo)
{
	Real x = 1.0f - wo.z;
	return f0 + (Vec3{1.0f,1.0f,1.0f} - f0) * (x*x*x*x*x);
}

Real ggx_d(const Vec3 &wm, Real a2)
{
	Real x = wm.z * wm.z * (a2 - 1.0f) + 1.0f;
	return a2 / (Pi * x * x);
}

Real ggx_g1(const Vec3 &wo, Real a2)
{
	Real x = sqrt_safe(a2 + (1.0f - a2) * wo.z * wo.z) + wo.z;
	return (2.0f * wo.z) / x;
}

Real ggx_g2(const Vec3 &wo, const Vec3 &wi, Real a2)
{
	Real x = wo.z * sqrt_safe(a2 + (1.0f - a2) * wi.z * wi.z);
	Real y = wi.z * sqrt_safe(a2 + (1.0f - a2) * wo.z * wo.z);
	return (2.0f * wo.z * wi.z) / (x + y);
}

// Sample a visible normal vector for `uv`
// http://www.jcgt.org/published/0007/04/01/paper.pdf
Vec3 ggx_vndf_wm_sample(const Vec3 &wo, Real a, const Vec2 &uv)
{
	Basis basis = basis_normal(Vec3{a*wo.x, a*wo.y, wo.z});
	Real x = 1.0f / (1.0f + basis.z.z);
	Real r = sqrt_safe(uv.x);
	Real t = uv.y < x ? uv.y/x * Pi : Pi + (uv.y-x) / (1.0f-x) * Pi;
	Vec2 p { r*cos(t), r*sin(t)*(uv.y < x ? 1.0f : basis.z.z) };
	Vec3 n = basis.to_world(Vec3{ p.x, p.y, sqrt_safe(1.0f - p.x*p.x - p.y*p.y) });
	return normalize(Vec3{a*n.x, a*n.y, max(n.z, 0.0f)});
}

// Probability distribution for `ggx_vndf_wm_sample()`
// http://www.jcgt.org/published/0007/04/01/paper.pdf eq. (17) and (3)
Real ggx_vndf_wm_pdf(const Vec3 &wo, const Vec3 &wm, Real a2)
{
	// TODO: dot(wo, wm) / wo.z^2?
	return (ggx_g1(wo, a2) * ggx_d(wm, a2)) / (wo.z * 4.0f);
}

Vec3 surface_wi_sample(const Vec3 &uvw, const Surface &surface, const Vec3 &wo, Real *out_pdf)
{
	Real a = surface.roughness * surface.roughness, a2 = a * a;
	Vec3 f = schlick_f(surface.specular, wo);

	Real kd = length(surface.diffuse), ks = length(f);
	if (kd + ks <= FLT_MIN) return { };
	Real pd = kd / (kd + ks), ps = 1.0f - pd;

	Vec3 wm, wi;
	if (uvw.z <= pd) {
		wi = cosine_hemisphere_sample(Vec2{ uvw.x, uvw.y });
		wm = normalize(wi + wo);
	} else {
		wm = ggx_vndf_wm_sample(wo, a, Vec2{ uvw.x, uvw.y });
		wi = reflect(wm, wo);
	}
	if (wi.z <= 0.0f) return Vec3{};

	*out_pdf = pd * cosine_hemisphere_pdf(wi) + ps * ggx_vndf_wm_pdf(wo, wm, a2);
	return wi;
}

Real surface_wi_pdf(const Surface &surface, const Vec3 &wo, const Vec3 &wi)
{
	// TODO: Cache these
	if (wi.z <= 0.0f) return 0.0f;
	Real a = surface.roughness * surface.roughness, a2 = a * a;
	Vec3 f = schlick_f(surface.specular, wo);
	Vec3 wm = normalize(wi + wo);
	Real kd = length(surface.diffuse), ks = length(f);
	Real pd = kd / (kd + ks), ps = 1.0f - pd;
	return pd * cosine_hemisphere_pdf(wi) + ps * ggx_vndf_wm_pdf(wo, wm, a2);
}

Vec3 surface_brdf(const Surface &surface, const Vec3 &wo, const Vec3 &wi)
{
	Real a = surface.roughness * surface.roughness, a2 = a * a;
	Vec3 wm = normalize(wi + wo);
	Vec3 f = schlick_f(surface.specular, wo);

	Vec3 dif = surface.diffuse * (wi.z / Pi);

	Real g = ggx_g2(wo, wi, a2);
	Real d = ggx_d(wm, a2);
	Vec3 spec = f * (d * g) / (4.0f * wo.z);

	return dif*(Vec3{1.0f,1.0f,1.0f} - f) + spec;
}

Vec3 shade_sky(Scene &scene, const Ray &ray)
{
	Vec3 sky;
	if (scene.sky.width > 0) {
		Vec3 dir = normalize(ray.direction);
		Vec2 uv = {
			(atan2(dir.z, dir.x) + scene.sky_rotation) * (0.5f/Pi),
			0.99f - acos(clamp(dir.y, -1.0f, 1.0f)) * (0.98f/Pi),
		};
		Vec4 v = sample_image(scene.sky, uv);
		sky = Vec3{ v.x, v.y, v.z };
	} else {
		sky = (Vec3{ 0.05f, 0.05f, 0.05f }
			+ Vec3{ 1.0f, 1.0f, 1.3f } * max(ray.direction.y, 0.0f)
			+ Vec3{ 0.12f, 0.15f, 0.0f } * max(-ray.direction.y, 0.0f)) * 0.03f;
	}
	return sky * scene.sky_factor;
}

// -- Path tracing

RayHit trace_ray(Random &rng, Scene &scene, const Ray &ray, Real max_t=Inf, bool shadow=false)
{
	RayHit hit = { };
	Real t_offset = 0.0f;
	for (uint32_t depth = 0; depth < 256; depth++) {
		Ray test_ray = { ray.origin + ray.direction * t_offset, ray.direction };
		RayTrace trace = setup_trace(test_ray, max_t - t_offset);
		intersect_bvh(trace, scene.root);
		hit = trace.hit;
		if (hit.index == SIZE_MAX) break;

		const TriangleInfo &tri = scene.triangles[hit.index];
		const Material &material = scene.materials[tri.material];
		if (shadow && !material.cast_shadows) {
			// Ignore always
		} else if (material.has_alpha) {
			Real u = hit.u, v = hit.v, w = 1.0f - u - v;
			Vec2 uv = tri.uv[0]*u + tri.uv[1]*v + tri.uv[2]*w;
			Real alpha = sample_image(material.base_color.image, uv).w;
			if (uniform_real(rng) < alpha) break;
		} else {
			break;
		}

		t_offset += hit.t * 1.001f + 0.00001f;
	}

	hit.t += t_offset;
	return hit;
}

Vec3 trace_path(Random &rng, Scene &scene, const Ray &ray, const Vec3 &uvw, int depth=0)
{
	if (depth > scene.indirect_depth) return { };

	RayHit hit = trace_ray(rng, scene, ray);
	if (scene.bvh_heatmap) {
		return Vec3{ 0.005f, 0.003f, 0.1f } * (Real)hit.steps / scene.exposure;
	}

	if (hit.t >= Inf || hit.index == SIZE_MAX) {
		return depth == 0 ? shade_sky(scene, ray) : Vec3{ };
	}

	Real u = hit.u, v = hit.v, w = 1.0f - u - v;
	const TriangleInfo &tri = scene.triangles[hit.index];

	Vec3 pos = ray.origin + ray.direction * (hit.t * 0.9999f);
	Vec2 uv = tri.uv[0]*u + tri.uv[1]*v + tri.uv[2]*w;
	Vec3 normal = tri.normal[0]*u + tri.normal[1]*v + tri.normal[2]*w;
	Vec3 tri_normal = cross(tri.v[1] - tri.v[0], tri.v[2] - tri.v[0]);
	if (dot(tri_normal, ray.direction) > 0.0f) {
		normal = -normal;
	}

	Basis basis = basis_normal(normal);

	Vec3 wo = basis.to_basis(-ray.direction);
	if (wo.z <= 0.001f) {
		wo.z = 0.001f;
		wo = normalize(wo);
	}

	const Material &material = scene.materials[tri.material];
	Surface surface = eval_surface(material, uv);

	Vec3 li = surface.emission;

	for (const Light &light : scene.lights) {
		Real max_t = 1.0f;
		Vec3 delta;
		if (light.directional) {
			max_t = Inf;
			delta = light.position;
		} else {
			Vec3 p = light.position + uniform_sphere_sample(uniform_vec2(rng)) * light.radius;
			delta = p - pos;
			max_t = length(delta);
		}

		Vec3 dir = normalize(delta);
		Vec3 wi = basis.to_basis(dir);
		if (wi.z <= 0.0f) continue;

		Ray shadow_ray = { pos, dir };
		RayHit shadow_hit = trace_ray(rng, scene, shadow_ray, max_t, true);
		if (shadow_hit.t < max_t) continue;

		Real att = light.directional ? 1.0f : max(dot(delta, delta), 0.01f);
		li = li + light.color * surface_brdf(surface, wo, wi) / att;
	}

	{
		Real brdf_pdf = 0.0f;
		Vec3 brdf_wi = surface_wi_sample(uvw, surface, wo, &brdf_pdf);
		Vec3 brdf_dir = basis.to_world(brdf_wi);
		Vec3 brdf_li;

		if (brdf_wi.z >= 0.0f && brdf_pdf > 0.0f) {
			Ray shadow_ray = { pos, brdf_dir };
			RayHit shadow_hit = trace_ray(rng, scene, shadow_ray, Inf, true);
			if (shadow_hit.t >= Inf) {
				Vec3 sky = shade_sky(scene, shadow_ray);
				brdf_li = surface_brdf(surface, wo, brdf_wi) * sky;
			}

			li = li + brdf_li / brdf_pdf;
		}
	}

	{
		Real pdf = 0.0f;
		Vec3 wi = surface_wi_sample(uniform_vec3(rng), surface, wo, &pdf);
		if (pdf == 0.0f) return li;

		Vec3 next_dir = basis.to_world(wi);
		Ray next_ray = { pos - ray.direction * 0.0001f, next_dir };
		Vec3 uvw = uniform_vec3(rng);
		Vec3 l = trace_path(rng, scene, next_ray, uvw, depth + 1);
		l = surface_brdf(surface, wo, wi) * l / pdf;
		l = min(l, scene.indirect_clamp);
		return li + l;
	}
}

Ray camera_ray(Camera &camera, Vec2 uv, Random rng)
{
	Vec2 dof_offset;
	if (camera.dof > 0.0f) {
		float aspect_ratio = length(camera.plane_x) / length(camera.plane_y);
		dof_offset = uniform_disk_sample(uniform_vec2(rng)) * camera.dof * Vec2 { 1.0f, aspect_ratio };
	}
	Vec3 plane = camera.plane_x*(uv.x*2.0f - 1.0f + dof_offset.x) + camera.plane_y*(uv.y*-2.0f + 1.0f + dof_offset.y);
	Ray ray;
	if (camera.orthographic) {
		ray.origin = camera.origin + plane;
		ray.direction = camera.forward;
	} else {
		ray.origin = camera.origin + (camera.plane_x*dof_offset.x + camera.plane_y*dof_offset.y) * camera.dof_focus;
		ray.direction = normalize(camera.forward + plane);
	}
	return ray;
}

struct ImageTracer
{
	Scene &scene;
	Framebuffer &framebuffer;
	size_t num_samples = 0;
	uint64_t seed = 0;
	std::atomic_uint32_t a_counter { 0 };
	std::atomic_uint32_t a_workers { 0 };
};

struct DebugTracer : DebugTracerBase
{
	Scene scene;
	DebugTracer(Scene &&scene) : scene(std::move(scene)) { }
	virtual Vec3 trace(Vec2 uv) override {
		Random rng { 1 };
		Ray ray = camera_ray(scene.camera, uv, rng);
		return trace_path(rng, scene, ray, Vec3{}) * scene.exposure;
	}
};

// https://graphics-programming.org/resources/tonemapping/index.html
Vec3 uncharted_tonemap(Vec3 v)
{
	auto tonemap_partial = [](Vec3 x) {
		const float A = 0.15f, B = 0.50f, C = 0.10f, D = 0.20f, E = 0.02f, F = 0.30f;
		return ((x*(x*A+vec3(C*B))+vec3(D*E))/(x*(x*A+vec3(B))+vec3(D*F)))-vec3(E/F);
	};
	float exposure_bias = 2.0f;
	Vec3 curr = tonemap_partial(v * exposure_bias);
	Vec3 W = vec3(11.2f);
	Vec3 white_scale = vec3(1.0f) / tonemap_partial(W);
	return curr * white_scale;
}

void trace_image(ImageTracer &tracer, bool print_status)
{
	static const size_t tile_size = 16;
	uint32_t width = tracer.framebuffer.width;
	uint32_t height = tracer.framebuffer.height;
	uint32_t num_tiles_x = (width + tile_size - 1) / tile_size;
	uint32_t num_tiles_y = (height + tile_size - 1) / tile_size;
	Vec2 resolution = { (Real)width, (Real)height };

	tracer.a_workers.fetch_add(1u, std::memory_order_relaxed);

	for (;;) {
		uint32_t tile_ix = tracer.a_counter.fetch_add(1u, std::memory_order_relaxed);
		if (tile_ix >= num_tiles_x * num_tiles_y) break;

		uint32_t tile_x = tile_ix % num_tiles_x;
		uint32_t tile_y = tile_ix / num_tiles_x;

		Random rng { (tracer.seed << 32u) + tile_ix };
		rng.next();

		for (uint32_t dy = 0; dy < tile_size; dy++)
		for (uint32_t dx = 0; dx < tile_size; dx++) {
			uint32_t x = tile_x * tile_size + dx, y = tile_y * tile_size + dy;
			if (x >= width || y >= height) continue;

			Real exposure = tracer.scene.exposure;
			Vec3 offset = uniform_vec3(rng);

			Vec3 color;
			Real weight = 0.0f;
			for (size_t i = 0; i < tracer.num_samples; i++) {
				Vec2 aa = uniform_vec2(rng) - Vec2{ 0.5f, 0.5f };
				Vec2 uv = Vec2 { (Real)x + aa.x, (Real)y + aa.y } / resolution;

				Ray ray = camera_ray(tracer.scene.camera, uv, rng);

				Vec3 uvw = halton_vec3(i, offset);
				Vec3 l = trace_path(rng, tracer.scene, ray, uvw) * exposure;
				l = min(l, Vec3{ 100.0f, 100.0f, 100.0f });

				color = color + l;
				weight += 1.0f;
			}

			color = color / weight;
			color = uncharted_tonemap(color);
			color = linear_to_srgb(color);
			color = min(max(color, Vec3{0.0f,0.0f,0.0f}), Vec3{1.0f,1.0f,1.0f}) * 255.9f;

			size_t ix = y * width + x;
			tracer.framebuffer.pixels[ix] = { (uint8_t)color.x, (uint8_t)color.y, (uint8_t)color.z };
		}

		if (print_status) {
			verbosef("%u/%u\n", tile_ix+1, num_tiles_x*num_tiles_y);
		}
	}

	tracer.a_workers.fetch_sub(1u, std::memory_order_relaxed);
}

void render_frame(ufbx_scene *original_scene, const Opts &opts, int frame_offset=0)
{
	Scene scene = create_scene(original_scene, opts, frame_offset);
	if (opts.sky.defined) {
		std::string path = get_path(opts, opts.sky);
		ImageResult result = load_png(path.c_str());
		if (result.error) {
			fprintf(stderr, "Failed to load sky %s: %s", path.c_str(), result.error);
		}
		scene.sky = result.image;
		scene.sky.srgb = true;
	}

	scene.sky_factor = pow((Real)2.0, opts.sky_exposure.value);
	scene.sky_rotation = opts.sky_rotation.value * (Pi/180.0f);
	scene.exposure = pow((Real)2.0, opts.exposure.value);
	scene.indirect_clamp = Vec3{ 1.0f, 1.0f, 1.0f } * opts.indirect_clamp.value / scene.exposure;
	scene.indirect_depth = opts.bounces.value;
	scene.bvh_heatmap = opts.bvh_heatmap.value;

	Framebuffer framebuffer { scene.camera.resolution_x, scene.camera.resolution_y };

	ImageTracer tracer = { scene, framebuffer };
	tracer.num_samples = (size_t)opts.samples.value;
	tracer.seed = (uint64_t)frame_offset;

	size_t num_threads = std::thread::hardware_concurrency() - 1;
	if (opts.threads.value > 0) num_threads = (size_t)opts.threads.value - 1;
	std::unique_ptr<std::thread[]> threads = std::make_unique<std::thread[]>(num_threads);

	verbosef("Using %zu threads\n", num_threads + 1);

	auto time_begin = std::chrono::high_resolution_clock::now();

	for (size_t i = 0; i < num_threads; i++) {
		threads[i] = std::thread { trace_image, std::ref(tracer), false };
	}

	if (opts.gui.value) {
		enable_gui(&framebuffer);
	}

	trace_image(tracer, true);

	auto time_end = std::chrono::high_resolution_clock::now();
	printf("Done in %.2fs\n", (double)std::chrono::duration_cast<std::chrono::nanoseconds>(time_end - time_begin).count() * 1e-9);

	for (size_t i = 0; i < num_threads; i++) {
		threads[i].join();
	}

	std::string output_path = get_path(opts, opts.output);
	save_png(output_path.c_str(), framebuffer.pixels.data(), framebuffer.width, framebuffer.height, frame_offset);

	if (opts.gui.value) {
		disable_gui(std::move(framebuffer), std::make_unique<DebugTracer>(std::move(scene)));
	}
}

void render_file(const Opts &opts)
{
	verbosef("Loading scene: %s\n", opts.input.value.c_str());

	ufbx_load_opts load_opts = { };

	load_opts.evaluate_skinning = true;
	load_opts.load_external_files = true;

	int progress_counter = 0;
	auto progress_cb = [&](const ufbx_progress *progress) {
		if ((progress_counter++ & 0xfff) == 0) {
			verbosef("%.1f/%.1f MB\n", (double)progress->bytes_read/1e6, (double)progress->bytes_total/1e6);
		}
		return UFBX_PROGRESS_CONTINUE;
	};

	load_opts.progress_cb = &progress_cb;
	ufbx_real scale = (ufbx_real)opts.scene_scale.value;

	load_opts.use_root_transform = true;
	load_opts.root_transform.rotation = ufbx_identity_quat;
	load_opts.root_transform.scale = ufbx_vec3{ scale, scale, scale };

	std::string path = get_path(opts, opts.input);

	ufbx_error error;
	ufbx_scene *scene = ufbx_load_file(path.c_str(), &load_opts, &error);
	if (!scene) {
		char buf[4096];
		ufbx_format_error(buf, sizeof(buf), &error);
		fprintf(stderr, "%s\n", buf);
		exit(1);
	}

	for (int i = opts.frame_offset.value; i < opts.num_frames.value; i++) {
		render_frame(scene, opts, i);
	}

	ufbx_free_scene(scene);

	if (opts.gui.value) {
		if (!opts.keep_open.value) close_gui();
		wait_gui();
	}
}

int main(int argc, char **argv)
{
	Opts opts = setup_opts(argc, argv);

	render_file(opts);

	if (opts.help.value) {
		return 0;
	}

	if (opts.compare.defined) {
		compare_images(opts, opts.compare.value.v[0].c_str(), opts.compare.value.v[1].c_str());
		return 0;
	}

	if (!opts.input.defined) {
		fprintf(stderr, "Usage: picort input.fbx/.picort.txt <opts> (--help)\n");
		return 0;
	}

	if (opts.reference.defined) {
		std::string output = get_path(opts, opts.output);
		std::string reference = get_path(opts, opts.reference);
		compare_images(opts, output.c_str(), reference.c_str());
	}

	return 0;
}
