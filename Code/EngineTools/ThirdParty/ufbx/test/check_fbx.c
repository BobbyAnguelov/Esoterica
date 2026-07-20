#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#else
	#include <sys/types.h>
	#include <dirent.h>
#endif

static void ufbxt_assert_fail_imp(const char *func, const char *file, size_t line, const char *msg)
{
	fprintf(stderr, "%s:%zu: %s(%s) failed\n", file, line, func, msg);
	exit(2);
}

#define ufbxt_assert_fail(file, line, msg) ufbxt_assert_fail_imp("ufbxt_assert_fail", file, line, msg)
#define ufbxt_assert(m_cond) do { if (!(m_cond)) ufbxt_assert_fail_imp("ufbxt_assert", __FILE__, __LINE__, #m_cond); } while (0)
#define ufbx_assert(m_cond) do { if (!(m_cond)) ufbxt_assert_fail_imp("ufbx_assert", __FILE__, __LINE__, #m_cond); } while (0)

bool g_verbose = false;

#include "../ufbx.h"

#if defined(THREADS)
	#include "../extra/ufbx_os.h"
#endif

#include "check_scene.h"
#include "check_material.h"
#include "testing_utils.h"
#include "testing_fuzz.h"
#include "cputime.h"

typedef struct {
	const char *name;
	int value;
} ufbxt_enum_name;

static const ufbxt_enum_name ufbxt_names_ufbx_geometry_transform_handling[] = {
	{ "preserve", UFBX_GEOMETRY_TRANSFORM_HANDLING_PRESERVE },
	{ "helper-nodes", UFBX_GEOMETRY_TRANSFORM_HANDLING_HELPER_NODES },
	{ "modify-geometry", UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY },
	{ "modify-geometry-no-fallback", UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY_NO_FALLBACK },
};

static const ufbxt_enum_name ufbxt_names_ufbx_inherit_mode_handling[] = {
	{ "preserve", UFBX_INHERIT_MODE_HANDLING_PRESERVE },
	{ "helper-nodes", UFBX_INHERIT_MODE_HANDLING_HELPER_NODES },
	{ "compensate", UFBX_INHERIT_MODE_HANDLING_COMPENSATE },
	{ "compensate-no-fallback", UFBX_INHERIT_MODE_HANDLING_COMPENSATE_NO_FALLBACK },
};

static const ufbxt_enum_name ufbxt_names_ufbx_space_conversion[] = {
	{ "transform-root", UFBX_SPACE_CONVERSION_TRANSFORM_ROOT },
	{ "adjust-transforms", UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS },
	{ "modify-geometry", UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY },
};

static const ufbxt_enum_name ufbxt_names_ufbx_index_error_handling[] = {
	{ "clamp", UFBX_INDEX_ERROR_HANDLING_CLAMP },
	{ "no-index", UFBX_INDEX_ERROR_HANDLING_NO_INDEX },
	{ "abort", UFBX_INDEX_ERROR_HANDLING_ABORT_LOADING },
	{ "unsafe-ignore", UFBX_INDEX_ERROR_HANDLING_UNSAFE_IGNORE },
};

static const ufbxt_enum_name ufbxt_names_ufbx_pivot_handling[] = {
	{ "retain", UFBX_PIVOT_HANDLING_RETAIN },
	{ "adjust-to-pivot", UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT },
	{ "adjust-to-rotation-pivot", UFBX_PIVOT_HANDLING_ADJUST_TO_ROTATION_PIVOT },
};

static int ufbxt_str_to_enum_imp(const ufbxt_enum_name *names, size_t count, const char *type_name, const char *name)
{
	for (size_t i = 0; i < count; i++) {
		if (!strcmp(names[i].name, name)) {
			return names[i].value;
		}
	}
	fprintf(stderr, "Unsupported enum name in %s: %s\n", type_name, name);
	ufbxt_assert(false);
	return -1;
}

#define ufbxt_cat2(a, b) a##b
#define ufbxt_cat(a, b) ufbxt_cat2(a,b)
#define ufbxt_str_to_enum_names(m_type, m_names, m_str) \
	(m_type)ufbxt_str_to_enum_imp(m_names, ufbxt_arraycount(m_names), #m_type, (m_str))
#define ufbxt_str_to_enum(m_type, m_str) \
	ufbxt_str_to_enum_names(m_type, ufbxt_cat2(ufbxt_names_, m_type), m_str)

typedef struct {
	size_t count;
	const char *names[64];
} ufbxt_fbx_features;

static void ufbxt_add_feature(ufbxt_fbx_features *features, const char *name)
{
	for (size_t i = 0; i < features->count; i++) {
		if (!strcmp(features->names[i], name)) return;
	}

	size_t index = features->count++;
	ufbxt_assert(index < ufbxt_arraycount(features->names));
	features->names[index] = name;
}

typedef struct {
	ufbxt_obj_file *obj_file;
	const char *path;
} ufbxt_obj_file_task;

static ufbxt_obj_file *ufbxt_load_obj_file(const char *path)
{
	size_t obj_size;
	void *obj_data = ufbxt_read_file_ex(path, &obj_size);
	if (!obj_data) {
		fprintf(stderr, "Failed to read .obj file: %s\n", path);
		exit(1);
	}

	ufbxt_load_obj_opts obj_opts = { 0 };

	ufbxt_obj_file *obj_file = ufbxt_load_obj(obj_data, obj_size, &obj_opts);
	free(obj_data);

	obj_file->normalize_units = true;

	return obj_file;
}

static void ufbxt_obj_file_task_fn(void *user, uint32_t index)
{
	ufbxt_obj_file_task *t = (ufbxt_obj_file_task*)user;
	t->obj_file = ufbxt_load_obj_file(t->path);
}

int check_fbx_main(int argc, char **argv, const char *path)
{
	if (path) {
		printf("-- %s\n", path);
	}

	cputime_begin_init();

	const char *obj_path = NULL;
	const char *mat_path = NULL;
	const char *dump_obj_path = NULL;
	int profile_runs = 0;
	int frame = INT_MIN;
	bool allow_bad_unicode = false;
	bool allow_unknown = false;
	bool sink = false;
	bool dedicated_allocs = false;
	bool bake = false;
	bool ignore_warnings = false;
	double bake_fps = -1.0;
	double override_fps = -1.0;

	ufbx_load_opts opts = { 0 };

	opts.evaluate_skinning = true;
	opts.evaluate_caches = true;
	opts.load_external_files = true;
	opts.generate_missing_normals = true;
	opts.target_axes = ufbx_axes_right_handed_y_up;
	opts.target_unit_meters = (ufbx_real)0.01;
	opts.obj_search_mtl_by_filename = true;
	opts.index_error_handling = UFBX_INDEX_ERROR_HANDLING_ABORT_LOADING;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) {
			// Handled in main()
		} else if (!strcmp(argv[i], "--obj")) {
			if (++i < argc) obj_path = argv[i];
		} else if (!strcmp(argv[i], "--mat")) {
			if (++i < argc) mat_path = argv[i];
		} else if (!strcmp(argv[i], "--dump-obj")) {
			if (++i < argc) dump_obj_path = argv[i];
		} else if (!strcmp(argv[i], "--profile-runs")) {
			if (++i < argc) profile_runs = atoi(argv[i]);
		} else if (!strcmp(argv[i], "--frame")) {
			if (++i < argc) frame = atoi(argv[i]);
		} else if (!strcmp(argv[i], "--allow-bad-unicode")) {
			allow_bad_unicode = true;
		} else if (!strcmp(argv[i], "--allow-unknown")) {
			allow_unknown = true;
		} else if (!strcmp(argv[i], "--dedicated-allocs")) {
			dedicated_allocs = true;
		} else if (!strcmp(argv[i], "--sink")) {
			sink = true;
		} else if (!strcmp(argv[i], "--ignore-missing-external")) {
			opts.ignore_missing_external_files = true;
		} else if (!strcmp(argv[i], "--geometry-transform-handling")) {
			if (++i < argc) opts.geometry_transform_handling = ufbxt_str_to_enum(ufbx_geometry_transform_handling, argv[i]);
		} else if (!strcmp(argv[i], "--inherit-mode-handling")) {
			if (++i < argc) opts.inherit_mode_handling = ufbxt_str_to_enum(ufbx_inherit_mode_handling, argv[i]);
		} else if (!strcmp(argv[i], "--pivot-handling")) {
			if (++i < argc) opts.pivot_handling = ufbxt_str_to_enum(ufbx_pivot_handling, argv[i]);
		} else if (!strcmp(argv[i], "--space-conversion")) {
			if (++i < argc) opts.space_conversion = ufbxt_str_to_enum(ufbx_space_conversion, argv[i]);
		} else if (!strcmp(argv[i], "--index-error-handling")) {
			if (++i < argc) opts.index_error_handling = ufbxt_str_to_enum(ufbx_index_error_handling, argv[i]);
		} else if (!strcmp(argv[i], "--fps")) {
			if (++i < argc) override_fps = strtod(argv[i], NULL);
		} else if (!strcmp(argv[i], "-d")) {
			++i; // Handled in main()
		} else if (!strcmp(argv[i], "--bake")) {
			bake = true;
		} else if (!strcmp(argv[i], "--ignore-warnings")) {
			ignore_warnings = true;
		} else if (!strcmp(argv[i], "--bake-fps")) {
			if (++i < argc) bake_fps = strtod(argv[i], NULL);
		} else if (argv[i][0] == '-') {
			fprintf(stderr, "Unrecognized flag: %s\n", argv[i]);
			exit(1);
		} else {
			path = argv[i];
		}
	}

	if (!path) {
		fprintf(stderr, "Usage: check_fbx <file.fbx>\n");
		return 1;
	}

	if (strstr(path, "ufbx-bad-unicode")) {
		allow_bad_unicode = true;
	}

	if (dedicated_allocs) {
		opts.temp_allocator.huge_threshold = 1;
		opts.result_allocator.huge_threshold = 1;
	}

	if (!allow_bad_unicode) {
		opts.unicode_error_handling = UFBX_UNICODE_ERROR_HANDLING_ABORT_LOADING;
	}

	// Check if the file is a ufbxfuzz case
	FILE *stdio_file = NULL;
	bool is_fuzz = false;
	{
		FILE *f = NULL;

		#if defined(_WIN32)
		{
			wchar_t wpath[512];
			int res = MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, ufbxt_arraycount(wpath));
			ufbxt_assert(res > 0 && res < ufbxt_arraycount(wpath) - 3);
			if (_wfopen_s(&f, wpath, L"rb") != 0) {
				f = NULL;
			}
		}
		#else
			f = fopen(path, "rb");
		#endif

		if (f) {
			ufbxt_fuzz_header header;
			size_t num_read = fread(&header, 1, sizeof(ufbxt_fuzz_header), f);
			if (num_read == sizeof(ufbxt_fuzz_header) && !memcmp(header.magic, "ufbxfuzz", 8) && header.version == 1) {
				stdio_file = f;
				is_fuzz = true;

				memset(&opts, 0, sizeof(opts));
				ufbxt_fuzz_apply_flags(&opts, header.flags);
			}
			if (!stdio_file) {
				fclose(f);
			}
		}
	}

	#if defined(THREADS)
		ufbx_os_thread_pool *thread_pool = ufbx_os_create_thread_pool(NULL);
		ufbxt_assert(thread_pool);

		ufbxt_obj_file_task obj_task = { NULL };
		uint64_t obj_task_id = ~(uint64_t)0;

		if (obj_path) {
			obj_task.path = obj_path;
			obj_task_id = ufbx_os_thread_pool_run(thread_pool, &ufbxt_obj_file_task_fn, &obj_task, 1);
		}

		ufbx_os_init_ufbx_thread_pool(&opts.thread_opts.pool, thread_pool);
	#endif

	ufbx_error error;
	ufbx_scene *scene;

	uint64_t load_delta = 0;
	{
		uint64_t load_begin = cputime_cpu_tick();
		if (stdio_file) {
			scene = ufbx_load_stdio(stdio_file, &opts, &error);
			fclose(stdio_file);
		} else {
			scene = ufbx_load_file(path, &opts, &error);
		}
		uint64_t load_end = cputime_cpu_tick();
		load_delta = load_end - load_begin;
	}

	if (!scene) {
		char buf[1024];
		ufbx_format_error(buf, sizeof(buf), &error);
		fprintf(stderr, "%s\n", buf);
		return 1;
	}

	cputime_end_init();

	{
		size_t fbx_size = ufbxt_file_size(path);
		printf("Loaded in %.2fms: File %.1fkB, temp %.1fkB (%zu allocs), result %.1fkB (%zu allocs)\n",
			cputime_cpu_delta_to_sec(NULL, load_delta) * 1e3,
			(double)fbx_size * 1e-3,
			(double)scene->metadata.temp_memory_used * 1e-3,
			scene->metadata.temp_allocs,
			(double)scene->metadata.result_memory_used * 1e-3,
			scene->metadata.result_allocs
		);
	}

	const char *exporters[] = {
		"Unknown",
		"FBX SDK",
		"Blender Binary",
		"Blender ASCII",
		"MotionBuilder",
	};

	const char *formats[2][2] = {
		{ "binary", "binary (big-endian)" },
		{ "ascii", "!?!?ascii (big-endian)!?!?" },
	};

	const char *application = scene->metadata.latest_application.name.data;
	if (!application[0]) application = "unknown";

	printf("FBX %u %s via %s %u.%u.%u (%s)\n",
		scene->metadata.version,
		formats[scene->metadata.ascii][scene->metadata.big_endian],
		exporters[scene->metadata.exporter],
		ufbx_version_major(scene->metadata.exporter_version),
		ufbx_version_minor(scene->metadata.exporter_version),
		ufbx_version_patch(scene->metadata.exporter_version),
		application);

	{
		size_t fbx_size = 0;
		void *fbx_data = ufbxt_read_file(path, &fbx_size);
		if (fbx_data) {

			for (int i = 0; i < profile_runs; i++) {
				ufbx_load_opts memory_opts = { 0 };
				#if defined(THREADS)
					ufbx_os_init_ufbx_thread_pool(&memory_opts.thread_opts.pool, thread_pool);
				#endif

				uint64_t load_begin = cputime_cpu_tick();
				ufbx_scene *memory_scene = ufbx_load_memory(fbx_data, fbx_size, &memory_opts, NULL);
				uint64_t load_end = cputime_cpu_tick();

				printf("Loaded in %.2fms: File %.1fkB, temp %.1fkB (%zu allocs), result %.1fkB (%zu allocs)\n",
					cputime_cpu_delta_to_sec(NULL, load_end - load_begin) * 1e3,
					(double)fbx_size * 1e-3,
					(double)scene->metadata.temp_memory_used * 1e-3,
					scene->metadata.temp_allocs,
					(double)scene->metadata.result_memory_used * 1e-3,
					scene->metadata.result_allocs
				);

				ufbxt_assert(memory_scene);
				ufbx_free_scene(memory_scene);
			}

			free(fbx_data);
		}
	}

	int result = 0;

	if (!strstr(path, "ufbx-unknown") && !is_fuzz && !allow_unknown) {
		bool ignore_unknowns = false;
		bool has_unknowns = false;

		for (size_t i = 0; i < scene->unknowns.count; i++) {
			ufbx_unknown *unknown = scene->unknowns.data[i];
			if (strstr(unknown->super_type.data, "MotionBuilder")) continue;
			if (strstr(unknown->type.data, "Container")) continue;
			if (!strcmp(unknown->super_type.data, "Object") && unknown->type.length == 0 && unknown->sub_type.length == 0) continue;
			if (!strcmp(unknown->super_type.data, "PluginParameters")) continue;
			if (!strcmp(unknown->super_type.data, "TimelineXTrack")) continue;
			if (!strcmp(unknown->super_type.data, "GlobalShading")) continue;
			if (!strcmp(unknown->super_type.data, "ControlSetPlug")) continue;
			if (!strcmp(unknown->sub_type.data, "NodeAttribute")) continue;
			if (!strcmp(unknown->type.data, "GroupSelection")) continue;
			if (!strcmp(unknown->name.data, "ADSKAssetReferencesVersion3.0")) {
				ignore_unknowns = true;
			}

			has_unknowns = true;
			fprintf(stderr, "Unknown element: %s/%s/%s : %s\n", unknown->super_type.data, unknown->type.data, unknown->sub_type.data, unknown->name.data);
		}

		if (has_unknowns && !ignore_unknowns) {
			result = 3;
		}
	}

	bool known_unknown = false;
	if (strstr(scene->metadata.creator.data, "kenney")) known_unknown = true;
	if (strstr(scene->metadata.creator.data, "assetforge")) known_unknown = true;
	if (strstr(scene->metadata.creator.data, "SmallFBX")) known_unknown = true;
	if (strstr(scene->metadata.creator.data, "FBX Unity Export")) known_unknown = true;
	if (strstr(scene->metadata.creator.data, "Open Asset Import Library")) known_unknown = true;
	if (scene->metadata.version < 5800) known_unknown = true;
	ufbxt_assert(scene->metadata.exporter != UFBX_EXPORTER_UNKNOWN || known_unknown || is_fuzz || allow_unknown);

	ufbxt_check_scene(scene);

	ufbxt_fbx_features features = { 0 };
	for (size_t i = 0; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];
		if (node->has_geometry_transform) {
			ufbxt_add_feature(&features, "geometry-transform");
		}
		if (node->inherit_mode != UFBX_INHERIT_MODE_NORMAL) {
			ufbxt_add_feature(&features, "inherit-mode");
		}

		ufbx_vec3 rotation_pivot = ufbx_find_vec3(&node->props, "RotationPivot", ufbx_zero_vec3);
		ufbx_vec3 scale_pivot = ufbx_find_vec3(&node->props, "ScalingPivot", ufbx_zero_vec3);
		ufbx_vec3 scale_offset = ufbx_find_vec3(&node->props, "ScalingOffset", ufbx_zero_vec3);
		if (!ufbxt_eq3(rotation_pivot, ufbx_zero_vec3)) {
			if (ufbxt_eq3(rotation_pivot, scale_pivot)) {
				ufbxt_add_feature(&features, "simple-pivot");
			} else {
				ufbxt_add_feature(&features, "complex-pivot");
			}
		}

		if (fabs(scale_offset.x) >= 0.01f || fabs(scale_offset.y) >= 0.01f || fabs(scale_offset.z) >= 0.01f) {
			ufbxt_add_feature(&features, "scaling-offset");
		}

		ufbx_vec3 pivot_diff = ufbxt_sub3(rotation_pivot, ufbxt_add3(scale_pivot, scale_offset));
		if (fabs(pivot_diff.x) >= 0.1f || fabs(pivot_diff.y) >= 0.1f || fabs(pivot_diff.z) >= 0.1f) {
			ufbxt_add_feature(&features, "separate-pivot");
		}
	}

	for (size_t i = 0; i < scene->meshes.count; i++) {
		ufbx_mesh *mesh = scene->meshes.data[i];
		if (mesh->instances.count > 1) {
			ufbxt_add_feature(&features, "instanced-mesh");
		}
	}

	for (size_t i = 0; i < scene->anim_curves.count; i++) {
		ufbx_anim_curve *curve = scene->anim_curves.data[i];
		if (curve->pre_extrapolation.mode != UFBX_EXTRAPOLATION_CONSTANT || curve->post_extrapolation.mode != UFBX_EXTRAPOLATION_CONSTANT) {
			ufbxt_add_feature(&features, "anim-extrapolation");
		}
	}

	ufbx_vec3 unit_axes[] = {
		{ 1.0f, 0.0f, 0.0f },
		{ -1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f, 0.0f },
	};

	ufbx_vec3 axis_x = unit_axes[scene->settings.axes.right];
	ufbx_vec3 axis_y = unit_axes[scene->settings.axes.up];
	ufbx_vec3 axis_z = unit_axes[scene->settings.axes.front];
	ufbx_vec3 cross = ufbxt_cross3(axis_x, axis_y);
	ufbx_real d = ufbxt_dot3(cross, axis_z);

	if (d < 0.0f) ufbxt_add_feature(&features, "left-handed");
	if (scene->nurbs_curves.count > 0) ufbxt_add_feature(&features, "nurbs-curve");
	if (scene->nurbs_surfaces.count > 0) ufbxt_add_feature(&features, "nurbs-surface");
	if (scene->lod_groups.count > 0) ufbxt_add_feature(&features, "lod-group");

	if (features.count > 0) {
		printf("Features:");
		for (size_t i = 0; i < features.count; i++) {
			printf(" %s", features.names[i]);
		}
		printf("\n");
	}

	if (!ignore_warnings) {
		bool warning_seen[UFBX_WARNING_TYPE_COUNT] = { false };
		size_t warning_count[UFBX_WARNING_TYPE_COUNT] = { 0 };
		for (size_t i = 0; i < scene->metadata.warnings.count; i++) {
			ufbx_warning *warning = &scene->metadata.warnings.data[i];
			size_t count = warning->count > 0 ? warning->count : 1;
			warning_count[warning->type] += count;
		}
		for (size_t i = 0; i < scene->metadata.warnings.count; i++) {
			ufbx_warning *warning = &scene->metadata.warnings.data[i];
			if (warning_seen[warning->type]) continue;
			warning_seen[warning->type] = true;
			size_t extra = warning_count[warning->type] - warning->count;
			if (extra > 0) {
				if (warning->count > 1) {
					printf("Warning: %s (x%zu) + %zu\n", warning->description.data, warning->count, extra);
				} else {
					printf("Warning: %s + %zu\n", warning->description.data, extra);
				}
			} else {
				if (warning->count > 1) {
					printf("Warning: %s (x%zu)\n", warning->description.data, warning->count);
				} else {
					printf("Warning: %s\n", warning->description.data);
				}
			}
		}
	}

	if (obj_path) {
		#if defined(THREADS)
			ufbx_os_thread_pool_wait(thread_pool, obj_task_id);
			ufbxt_obj_file *obj_file = obj_task.obj_file;
		#else
			ufbxt_obj_file *obj_file = ufbxt_load_obj_file(obj_path);
		#endif

		ufbx_scene *state;
		if (obj_file->animation_frame >= 0 || frame != INT_MIN || obj_file->bind_pose) {
			ufbx_anim *anim = scene->anim;

			if (obj_file->animation_name[0]) {
				ufbx_anim_stack *stack = ufbx_find_anim_stack(scene, obj_file->animation_name);
				ufbxt_assert(stack);
				anim = stack->anim;
			}

			double fps = scene->settings.frames_per_second;
			if (override_fps > 0.0)
				fps = override_fps;

			double time = anim->time_begin + (double)obj_file->animation_frame / fps;

			if (frame != INT_MIN) {
				time = (double)frame / fps;
			}

			if (obj_file->bind_pose) {
				ufbx_transform_override *transform_overrides = (ufbx_transform_override*)calloc(scene->nodes.count, sizeof(ufbx_transform_override));

				size_t num_transform_overrides = 0;
				for (size_t i = 0; i < scene->nodes.count; i++) {
					ufbx_node *node = scene->nodes.data[i];
					if (!node->bind_pose) continue;

					ufbx_bone_pose *pose = ufbx_get_bone_pose(node->bind_pose, node);
					ufbxt_assert(pose);
					ufbx_transform local_transform = ufbx_matrix_to_transform(&pose->bone_to_parent);

					ufbx_transform_override *over = &transform_overrides[num_transform_overrides++];
					over->node_id = node->typed_id;
					over->transform = local_transform;
				}

				ufbx_anim_opts anim_opts = { 0 };
				anim_opts.transform_overrides.data = transform_overrides;
				anim_opts.transform_overrides.count = num_transform_overrides;

				anim = ufbx_create_anim(scene, &anim_opts, NULL);
				ufbxt_assert(anim);

				free(transform_overrides);

			} else if (bake && !obj_file->no_bake) {
				ufbx_bake_opts opts = { 0 };
				opts.max_keyframe_segments = 4096;
				if (bake_fps > 0) {
					opts.resample_rate = bake_fps;
				}

				ufbx_baked_anim *bake = ufbx_bake_anim(scene, anim, &opts, NULL);
				ufbxt_assert(bake);

				size_t num_prop_overrides = 0;
				for (size_t i = 0; i < bake->elements.count; i++) {
					num_prop_overrides += bake->elements.data[i].props.count;
				}

				ufbx_prop_override_desc *prop_overrides = (ufbx_prop_override_desc*)calloc(num_prop_overrides, sizeof(ufbx_prop_override_desc));
				ufbx_transform_override *transform_overrides = (ufbx_transform_override*)calloc(bake->nodes.count, sizeof(ufbx_transform_override));
				ufbxt_assert(prop_overrides);
				ufbxt_assert(transform_overrides);

				ufbx_prop_override_desc *prop_over = prop_overrides;
				for (size_t elem_ix = 0; elem_ix < bake->elements.count; elem_ix++) {
					ufbx_baked_element *elem = &bake->elements.data[elem_ix];
					for (size_t prop_ix = 0; prop_ix < elem->props.count; prop_ix++) {
						ufbx_baked_prop *prop = &elem->props.data[prop_ix];
						prop_over->element_id = elem->element_id;
						prop_over->prop_name = prop->name;
						ufbx_vec3 val = ufbx_evaluate_baked_vec3(prop->keys, time);
						prop_over->value.x = val.x;
						prop_over->value.y = val.y;
						prop_over->value.z = val.z;
						prop_over++;
					}
				}

				for (size_t i = 0; i < bake->nodes.count; i++) {
					ufbx_baked_node *node = &bake->nodes.data[i];
					transform_overrides[i].node_id = node->typed_id;
					transform_overrides[i].transform.translation = ufbx_evaluate_baked_vec3(node->translation_keys, time);
					transform_overrides[i].transform.rotation = ufbx_evaluate_baked_quat(node->rotation_keys, time);
					transform_overrides[i].transform.scale = ufbx_evaluate_baked_vec3(node->scale_keys, time);
				}

				ufbxt_assert(prop_over == prop_overrides + num_prop_overrides);

				ufbx_anim_opts anim_opts = { 0 };
				anim_opts.prop_overrides.data = prop_overrides;
				anim_opts.prop_overrides.count = num_prop_overrides;
				anim_opts.transform_overrides.data = transform_overrides;
				anim_opts.transform_overrides.count = bake->nodes.count;

				anim = ufbx_create_anim(scene, &anim_opts, NULL);
				ufbxt_assert(anim);

				free(prop_overrides);
				free(transform_overrides);
			}

			ufbx_evaluate_opts eval_opts = { 0 };
			eval_opts.evaluate_skinning = true;
			eval_opts.evaluate_caches = true;
			eval_opts.load_external_files = true;
			state = ufbx_evaluate_scene(scene, anim, time, &eval_opts, NULL);
			ufbxt_assert(state);

			ufbx_free_anim(anim);
		} else {
			state = scene;
			ufbx_retain_scene(state);
		}

		ufbx_scene *model;
		if (obj_file->model_name[0]) {
			char model_path[512];
			size_t base_length = strlen(path);
			while (base_length > 0) {
				char c = path[base_length - 1];
				if (c == '/' || c == '\\') break;
				base_length -= 1;
			}

			int model_path_len = snprintf(model_path, sizeof(model_path), "%.*s%s",
				(int)base_length, path, obj_file->model_name);
			ufbxt_assert(model_path_len > 0 && model_path_len < sizeof(model_path));

			ufbx_scene *model_scene = ufbx_load_file(model_path, &opts, &error);
			if (!model_scene) {
				char buf[1024];
				ufbx_format_error(buf, sizeof(buf), &error);
				fprintf(stderr, "%s\n", buf);
				return 1;
			}

			ufbxt_check_scene(model_scene);

			ufbx_transform_override *transform_overrides = (ufbx_transform_override*)calloc(state->nodes.count, sizeof(ufbx_transform_override));
			ufbxt_assert(transform_overrides);
			size_t num_transform_overrides = 0;

			for (size_t i = 0; i < state->nodes.count; i++) {
				ufbx_node *state_node = state->nodes.data[i];
				if (state_node->is_root) continue;

				ufbx_node *model_node = ufbx_find_node(model_scene, state_node->name.data);
				if (!model_node) continue;

				ufbx_transform_override *over = &transform_overrides[num_transform_overrides++];
				over->node_id = model_node->typed_id;
				over->transform = state_node->local_transform;
			}

			ufbx_anim_opts anim_opts = { 0 };
			anim_opts.transform_overrides.data = transform_overrides;
			anim_opts.transform_overrides.count = num_transform_overrides;

			ufbx_anim *model_anim = ufbx_create_anim(model_scene, &anim_opts, NULL);
			ufbxt_assert(model_anim);

			ufbx_evaluate_opts eval_opts = { 0 };
			eval_opts.evaluate_skinning = true;
			eval_opts.evaluate_caches = true;
			eval_opts.load_external_files = true;
			model = ufbx_evaluate_scene(model_scene, model_anim, 0.0, &eval_opts, NULL);
			ufbxt_assert(model);

			ufbx_free_scene(model_scene);
			free(transform_overrides);
		} else {
			model = state;
			ufbx_retain_scene(model);
		}

		if (dump_obj_path) {
			ufbxt_debug_dump_obj_scene(dump_obj_path, model);
			printf("Dumped .obj to %s\n", dump_obj_path);
		}

		ufbxt_diff_error err = { 0 };

		uint32_t diff_flags = 0;

		if (bake) {
			diff_flags |= UFBXT_OBJ_DIFF_FLAG_BAKED_ANIM;
		}

		if (opts.inherit_mode_handling == UFBX_INHERIT_MODE_HANDLING_COMPENSATE || opts.inherit_mode_handling == UFBX_INHERIT_MODE_HANDLING_COMPENSATE_NO_FALLBACK) {
			diff_flags |= UFBXT_OBJ_DIFF_FLAG_BAKED_ANIM;
		}

		ufbxt_diff_to_obj(model, obj_file, &err, diff_flags);

		if (err.num > 0) {
			ufbx_real avg = err.sum / (ufbx_real)err.num;
			printf("Absolute diff: avg %.3g, max %.3g (%zu tests)\n", avg, err.max, err.num);
		}

		ufbx_free_scene(model);
		ufbx_free_scene(state);
		free(obj_file);
	} else {
		if (dump_obj_path) {
			ufbxt_debug_dump_obj_scene(dump_obj_path, scene);
			printf("Dumped .obj to %s\n", dump_obj_path);
		}
	}

	if (mat_path) {
		size_t mat_size;
		void *mat_data = ufbxt_read_file_ex(mat_path, &mat_size);
		if (!mat_data) {
			fprintf(stderr, "Failed to read .mat file: %s\n", mat_path);
			return 1;
		}

		const char *mat_filename = mat_path + strlen(mat_path);
		while (mat_filename > mat_path && (mat_filename[-1] != '\\' && mat_filename[-1] != '/')) {
			mat_filename--;
		}
		bool ok = ufbxt_check_materials(scene, (const char*)mat_data, mat_filename);
		if (!ok && !result) {
			result = 4;
		}

		free(mat_data);
	}

	ufbx_free_scene(scene);

	if (sink) {
		printf("%u\n", ufbxt_sink);
	}

	#if defined(THREADS)
		ufbx_os_free_thread_pool(thread_pool);
	#endif

	return result;
}

#define CPUTIME_IMPLEMENTATION
#ifndef UFBX_DEV
	#define UFBX_DEV
#endif

#ifdef _WIN32
int wmain(int argc, wchar_t **wide_argv)
#else
int main(int argc, char **argv)
#endif
{
#ifdef _WIN32
	char **argv = (char**)malloc(sizeof(char*) * argc);
	ufbxt_assert(argv);
	for (int i = 0; i < argc; i++) {
		int res = WideCharToMultiByte(CP_UTF8, 0, wide_argv[i], -1, NULL, 0, NULL, NULL);
		ufbxt_assert(res > 0);
		size_t dst_size = (size_t)res + 1;
		char *dst = (char*)malloc(dst_size);
		ufbxt_assert(dst);
		res = WideCharToMultiByte(CP_UTF8, 0, wide_argv[i], -1, dst, (int)dst_size, NULL, NULL);
		ufbxt_assert(res > 0 && (size_t)res < dst_size);
		argv[i] = dst;
	}
#endif

	const char *directory = NULL;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) {
			g_verbose = true;
		} else if (!strcmp(argv[i], "-d")) {
			if (++i < argc) directory = argv[i];
		}
	}

	if (directory) {
		int ok_count = 0;
		int total_count = 0;

		#if defined(_WIN32)
		{
			wchar_t wide_directory[512];
			char narrow_path[1024];
			char full_path[2048];
			{
				int res;

				res = MultiByteToWideChar(CP_UTF8, 0, directory, -1, wide_directory, ufbxt_arraycount(wide_directory));
				ufbxt_assert(res > 0 && res < ufbxt_arraycount(wide_directory) - 3);
				wide_directory[res - 1 + 0] = '\\';
				wide_directory[res - 1 + 1] = '*';
				wide_directory[res - 1 + 2] = '\0';

				WIN32_FIND_DATAW find_data = { 0 };
				HANDLE find_handle = FindFirstFileW(wide_directory, &find_data);
				ufbxt_assert(find_handle != NULL && INVALID_HANDLE_VALUE != NULL);
				do {
					res = WideCharToMultiByte(CP_UTF8, 0, find_data.cFileName, -1, narrow_path, ufbxt_arraycount(narrow_path), NULL, NULL);
					ufbxt_assert(res > 0 && res < ufbxt_arraycount(narrow_path) - 1);

					if (!strcmp(narrow_path, ".") || !strcmp(narrow_path, "..")) continue;

					res = snprintf(full_path, sizeof(full_path), "%s\\%s", directory, narrow_path);
					ufbxt_assert(res > 0 && res < ufbxt_arraycount(full_path) - 1);

					total_count += 1;
					if (check_fbx_main(argc, argv, full_path) == 0) {
						ok_count += 1;
					}

				} while (FindNextFileW(find_handle, &find_data));
				FindClose(find_handle);
			}
		}
		#else
		{
			DIR *dir = opendir(directory);
			ufbxt_assert(dir);
			char full_path[2048];

			struct dirent *entry;
			while ((entry = readdir(dir)) != NULL) {
				int res = snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);
				ufbxt_assert(res > 0 && res < ufbxt_arraycount(full_path) - 1);

				total_count += 1;
				if (check_fbx_main(argc, argv, full_path) == 0) {
					ok_count += 1;
				}
			}

			closedir(dir);
		}
		#endif

		printf("\n%d/%d OK\n", ok_count, total_count);
	} else {
		return check_fbx_main(argc, argv, NULL);
	}
}

#include "cputime.h"

#if defined(THREADS)
	#define UFBX_OS_IMPLEMENTATION
	#include "../extra/ufbx_os.h"
#endif

#ifndef EXTERNAL_UFBX
	#include "../ufbx.c"
#endif
