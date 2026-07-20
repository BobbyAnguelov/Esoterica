#define _CRT_SECURE_NO_WARNINGS

#if defined(_WIN32)
#define ufbx_assert(cond) do { \
		if (!(cond)) __debugbreak(); \
	} while (0)
#else
#define ufbx_assert(cond) do { \
		if (!(cond)) __builtin_trap(); \
	} while (0)
#endif

#define ufbxt_assert_fail(file, line, msg) ufbx_assert(false)
#define ufbxt_assert(m_cond) ufbx_assert(m_cond)

#define IM_ARG_IMPLEMENTATION

#include "decode_file.h"
#include "../../ufbx.h"
#include "../../test/check_scene.h"
#include "../../test/hash_scene.h"
#include "../../test/testing_fuzz.h"
#include "../im_arg.h"

#if defined(AFL)
	#include <unistd.h>
#else
	#include <filesystem>
	namespace fs = std::filesystem;
#endif

char g_src[1024*1024];
char g_dst[4*1024*1024];
semfuzz::Field g_fields[128*1024];
semfuzz::Value g_values[128*1024];
semfuzz::NumberCache g_number_cache;

static void init_opts(ufbx_load_opts &opts, const semfuzz::File &file)
{
#if defined(DO_LIMIT)
	if (file.temp_limit > 0) {
		opts.temp_allocator.allocation_limit = file.temp_limit;
		opts.temp_allocator.huge_threshold = 1;
	}
	if (file.result_limit > 0) {
		opts.result_allocator.allocation_limit = file.result_limit;
		opts.result_allocator.huge_threshold = 1;
	}
#endif

#if defined(ASAN)
	opts.temp_allocator.huge_threshold = 1;
	opts.result_allocator.huge_threshold = 1;
#endif

	opts.file_format = UFBX_FILE_FORMAT_FBX;
	opts.result_allocator.memory_limit = UINT64_C(4000000000);
	opts.temp_allocator.memory_limit = UINT64_C(4000000000);

	ufbxt_fuzz_apply_flags(&opts, file.flags);
}

uint64_t load_file(const semfuzz::File &file, int index)
{
#if defined(BINARY)
	size_t dst_size = semfuzz::write_binary(g_dst, sizeof(g_dst), file);
#else
	size_t dst_size = semfuzz::write_ascii(g_dst, sizeof(g_dst), file, &g_number_cache);
#endif

	if (dst_size == 0) return 0;
	uint64_t hash = 0;

	ufbx_load_opts opts = { };
	init_opts(opts, file);

	ufbx_error error;
	ufbx_scene *scene = ufbx_load_memory(g_dst, dst_size, &opts, &error);

	if (scene) {
		#if defined(DO_CHECK)
		ufbxt_check_scene(scene);

		{
			ufbx_evaluate_opts eval_opts = { };
			eval_opts.evaluate_skinning = true;
			ufbx_scene *state = ufbx_evaluate_scene(scene, scene->anim, 0.1, &eval_opts, &error);
			ufbxt_assert(state);
			ufbxt_check_scene(state);
			ufbx_free_scene(state);
		}

		{
			ufbx_baked_anim *bake = ufbx_bake_anim(scene, scene->anim, NULL, NULL);
			ufbxt_assert(bake);
			ufbx_free_baked_anim(bake);
		}
		#endif

		#if defined(DO_HASH)
		{
			FILE *hash_a = NULL, *hash_b = NULL;
			if (index == -1) {
				hash_a = fopen("hash_a.txt", "w");
				hash_b = fopen("hash_b.txt", "w");
			}

			uint64_t scene_hash = ufbxt_hash_scene(scene, hash_a);
			ufbx_scene *scene_alt = ufbx_load_memory(g_dst, dst_size, &opts, &error);
			ufbxt_assert(scene_alt);

			uint64_t alt_hash = ufbxt_hash_scene(scene_alt, hash_b);

			hash = scene_hash ? scene_hash : 1;

			if (hash_a) fclose(hash_a);
			if (hash_b) fclose(hash_b);

			ufbxt_assert(scene_hash == alt_hash);

			ufbx_free_scene(scene_alt);
		}
		#else
		{
			hash = 1;
		}
		#endif
	}

	ufbx_free_scene(scene);
	return hash;
}

int main(int argc, char **argv)
{
	semfuzz::File file;
	file.fields = g_fields;
	file.max_fields = sizeof(g_fields) / sizeof(*g_fields);
	file.values = g_values;
	file.max_values = sizeof(g_values) / sizeof(*g_values);

	g_number_cache.init();

#if defined(AFL)
	while (__AFL_LOOP(100000)) {
		size_t src_size = (size_t)read(0, g_src, sizeof(g_src));
		if (!semfuzz::read_fbb(file, g_src, src_size)) continue;
		load_file(file, 0);
	}
#else
	const char *source_path = NULL;
	const char *expand_path = NULL;
#if defined(BINARY)
	const char *format = "binary";
#else
	const char *format = "ascii";
#endif
	int target_index = -1;

	im_arg_begin_c(argc, argv);
	while (im_arg_next()) {
		im_arg_help("--help", "Show this help");

		if (im_arg("--expand dst", "Expand to output directory")) {
			expand_path = im_arg_str(0);
		}

		if (im_arg("-f format", "Format for exporting files")) {
			format = im_arg_str(0);
		}

		if (im_arg("-i index", "Only process a specific index")) {
			target_index = im_arg_int(0);
		}

		if (im_arg("path", "Path for input files")) {
			source_path = im_arg_str(0);
		}
	}

	if (source_path) {
		fs::path path{source_path};
		std::vector<fs::path> paths;

		if (fs::is_directory(path)) {
			for (const fs::directory_entry &entry : fs::directory_iterator(path)) {
				if (!entry.is_regular_file()) continue;
				paths.push_back(entry.path());
			}
		} else {
			paths.push_back(path);
		}

		int index = 0;
		for (const fs::path &path : paths) {

			if (target_index >= 0 && index != target_index) {
				index++;
				continue;
			}

			FILE *f = nullptr;
			#if defined(_WIN32)
			{
				std::u16string str = path.u16string();
				wprintf(L"[%d] %s", index, (wchar_t*)str.c_str());
				_wfopen_s(&f, (wchar_t*)str.c_str(), L"rb");
			}
			#else
			{
				std::string str = path.u8string();
				printf("[%d] %s", index, str.c_str());
				f = fopen(path.c_str(), "rb");
			}
			#endif
			ufbxt_assert(f);

			fflush(stdout);

			size_t src_size = fread(g_src, 1, sizeof(g_src), f);
			fclose(f);
			if (!semfuzz::read_fbb(file, g_src, src_size)) {
				return 1;
			}

			if (expand_path) {
				char name[256];
				snprintf(name, sizeof(name), "fuzz_%08d.fbx.fuzz", index);
				fs::path dst_path = fs::path(expand_path) / fs::path(name);

				printf(" -> %s\n", name);

				FILE *of = nullptr;
				#if defined(_WIN32)
				{
					std::u16string str = dst_path.u16string();
					_wfopen_s(&of, (wchar_t*)str.c_str(), L"wb");
				}
				#else
				{
					std::string str = dst_path.u8string();
					of = fopen(path.c_str(), "wb");
				}
				#endif
				ufbxt_assert(of);

				uint32_t fbx_format = 0;
				size_t dst_size = 0;
				if (!strcmp(format, "binary")) {
					dst_size = semfuzz::write_binary(g_dst, sizeof(g_dst), file);
					fbx_format = 1;
				} else if (!strcmp(format, "ascii")) {
					dst_size = semfuzz::write_ascii(g_dst, sizeof(g_dst), file, &g_number_cache);
					fbx_format = 2;
				}

				ufbxt_fuzz_header header;
				memcpy(header.magic, "ufbxfuzz", 8);
				header.version = 1;
				header.flags = file.flags;
				header.fbx_version = file.version;
				header.fbx_format = fbx_format;
				memset(header.unused, 0, sizeof(header.unused));

				fwrite(&header, 1, sizeof(ufbxt_fuzz_header), of);
				fwrite(g_dst, 1, dst_size, of);
				fclose(of);

			} else {
				uint64_t hash = load_file(file, index);
				printf(" %llu\n", (unsigned long long)hash);
			}

			index++;
		}
	}
#endif

	return 0;
}
