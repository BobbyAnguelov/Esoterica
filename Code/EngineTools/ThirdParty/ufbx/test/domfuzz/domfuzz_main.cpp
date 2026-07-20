#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <cctype>

static void ufbxt_assert_fail_imp(const char *func, const char *file, size_t line, const char *msg)
{
	fprintf(stderr, "%s:%zu: %s(%s) failed\n", file, line, func, msg);
	exit(2);
}

#define ufbxt_assert_fail(file, line, msg) ufbxt_assert_fail_imp("ufbxt_assert_fail", file, line, msg)
#define ufbxt_assert(m_cond) do { if (!(m_cond)) ufbxt_assert_fail_imp("ufbxt_assert", __FILE__, __LINE__, #m_cond); } while (0)

#include "fbxdom.h"
#include <vector>
#include <string>
#include <algorithm>
#include "../../ufbx.h"
#include "../check_scene.h"

bool g_verbose = false;

std::vector<char> read_file(const char *path)
{
	FILE *f = fopen(path, "rb");
	fseek(f, 0, SEEK_END);
	std::vector<char> data;
	data.resize(ftell(f));
	fseek(f, 0, SEEK_SET);
	fread(data.data(), 1, data.size(), f);
	fclose(f);
	return data;
}

using mutate_simple_fn = bool(fbxdom::node_ptr node, uint32_t index);

bool mutate_remove_child(fbxdom::node_ptr node, uint32_t index)
{
	if (index >= node->children.size()) return false;
	node->children.erase(node->children.begin() + index);
	return true;
}

bool mutate_duplicate_child(fbxdom::node_ptr node, uint32_t index)
{
	if (index >= node->children.size()) return false;
	fbxdom::node_ptr child = node->children[index];
	node->children.insert(node->children.begin() + index, std::move(child));
	return true;
}

bool mutate_reverse_children(fbxdom::node_ptr node, uint32_t index)
{
	if (index > 0) return false;
	std::reverse(node->children.begin(), node->children.end());
	return true;
}

bool mutate_remove_value(fbxdom::node_ptr node, uint32_t index)
{
	if (node->values.size() > 10) return false;
	if (index >= node->values.size()) return false;
	node->values.erase(node->values.begin() + index);
	return true;
}

bool mutate_duplicate_value(fbxdom::node_ptr node, uint32_t index)
{
	if (node->values.size() > 10) return false;
	if (index >= node->values.size()) return false;
	fbxdom::value value = node->values[index];
	node->values.insert(node->values.begin() + index, std::move(value));
	return true;
}

bool mutate_clear_value(fbxdom::node_ptr node, uint32_t index)
{
	if (node->values.size() > 10) return false;
	if (index >= node->values.size()) return false;
	fbxdom::value &value = node->values[index];
	value.data_int = 0;
	value.data_float = 0.0f;
	value.data_array.length = 0;
	value.data_array.compressed_length = 0;
	value.data_array.encoding = 0;
	return true;
}

bool mutate_shrink_value(fbxdom::node_ptr node, uint32_t index)
{
	if (node->values.size() > 10) return false;
	if (index >= node->values.size()) return false;
	fbxdom::value &value = node->values[index];
	if (value.type == 'L') {
		value.type = 'I';
	} else if (value.type == 'D') {
		value.type = 'F';
	} else if (value.type == 'I') {
		value.type = 'Y';
	} else if (value.type == 'Y') {
		value.type = 'C';
	} else if (value.type == 'S') {
		value.type = 'L';
	} else {
		value.type = 'I';
	}
	return true;
}

bool mutate_reverse_values(fbxdom::node_ptr node, uint32_t index)
{
	if (index > 0) return false;
	std::reverse(node->values.begin(), node->values.end());
	return true;
}

struct mutator
{
	const char *name;
	mutate_simple_fn *simple_fn;
};

static const mutator mutators[] = {
	{ "remove child", &mutate_remove_child },
	{ "duplicate child", &mutate_duplicate_child },
	{ "reverse children", &mutate_reverse_children },
	{ "remove value", &mutate_remove_value },
	{ "duplicate value", &mutate_duplicate_value },
	{ "clear value", &mutate_clear_value },
	{ "shrink value", &mutate_shrink_value },
	{ "reverse values", &mutate_reverse_values },
};

static const size_t num_mutators = sizeof(mutators) / sizeof(*mutators);

struct mutable_result
{
	fbxdom::node_ptr self;
	fbxdom::node_ptr target;
};

fbxdom::node_ptr find_path(fbxdom::node_ptr root, const std::vector<uint32_t> &path)
{
	fbxdom::node_ptr node = root;
	for (uint32_t ix : path) {
		node = node->children[ix];
	}
	return node;
}

void format_path(char *buf, size_t buf_size, fbxdom::node_ptr root, const std::vector<uint32_t> &path)
{
	int res = snprintf(buf, buf_size, "Root");
	if (res < 0 || res >= buf_size) return;
	size_t pos = (size_t)res;

	fbxdom::node_ptr node = root;
	for (uint32_t ix : path) {
		node = node->children[ix];
		int res = snprintf(buf + pos, buf_size - pos, "/%u/%.*s", ix, (int)node->name.size(), node->name.data());
		pos += res;
		if (res < 0 || pos >= buf_size) break;
	}
}

fbxdom::node_ptr mutate_path(fbxdom::node_ptr &node, const std::vector<uint32_t> &path, uint32_t depth=0)
{
	fbxdom::node_ptr copy = node->copy();
	if (depth < path.size()) {
		fbxdom::node_ptr &child_ref = copy->children[path[depth]];
		fbxdom::node_ptr child = mutate_path(child_ref, path, depth + 1);
		node = copy;
		return child;
	} else {
		node = copy;
		return copy;
	}
}

bool increment_path(fbxdom::node_ptr root, std::vector<uint32_t> &path)
{
	{
		fbxdom::node_ptr node = find_path(root, path);
		if (!node->children.empty()) {
			path.push_back(0);
			return true;
		}
	}

	while (!path.empty()) {
		uint32_t index = path.back();
		path.pop_back();
		fbxdom::node_ptr current = find_path(root, path);
		if (index + 1 < current->children.size()) {
			path.push_back(index + 1);
			return true;
		}
	}

	return false;
}

struct mutation_iterator
{
	std::vector<uint32_t> path = { };
	uint32_t mutator_index = 0;
	uint32_t mutator_internal_index = 0;
};

fbxdom::node_ptr next_mutation(fbxdom::node_ptr root, mutation_iterator &iter)
{
	for (;;) {
		fbxdom::node_ptr new_root = root;
		fbxdom::node_ptr new_node = mutate_path(new_root, iter.path);

		if (mutators[iter.mutator_index].simple_fn(new_node, iter.mutator_internal_index)) {
			iter.mutator_internal_index++;
			return new_root;
		}

		iter.mutator_internal_index = 0;
		if (iter.mutator_index + 1 < num_mutators) {
			iter.mutator_index++;
			continue;
		}

		iter.mutator_index = 0;
		if (increment_path(root, iter.path)) {
			continue;
		}

		return { };
	}
}

static char dump_buffer[16*1024*1024];

size_t g_coverage_counter = 0;

#if defined(DOMFUZZ_COVERAGE)

extern "C" void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop)
{
	std::fill(start, stop, 1u);
}

extern "C" void __sanitizer_cov_trace_pc_guard(uint32_t *guard)
{
	if (!*guard) return;
	g_coverage_counter++;
	*guard = 0;
}

#endif

int selected_step = -1;
int start_step = -1;
size_t coverage_output_count = 0;

bool process_fbx_file(const char *input_file, const char *coverage_path, bool dry_run)
{
	std::vector<char> data = read_file(input_file);

	{
		ufbx_scene *scene = ufbx_load_memory(data.data(), data.size(), NULL, NULL);
		if (!scene) return 0;
		if (scene->metadata.ascii || scene->metadata.big_endian) {
			ufbx_free_scene(scene);
			return false;
		}
		ufbx_free_scene(scene);
	}

	fbxdom::node_ptr root = fbxdom::parse(data.data(), data.size());
	ufbx_error error = { };

	{
		size_t dump_size = fbxdom::dump(dump_buffer, sizeof(dump_buffer), root);
		ufbx_scene *scene = ufbx_load_memory(data.data(), data.size(), NULL, &error);

		ufbxt_assert(scene);
		ufbxt_check_scene(scene);

		ufbx_free_scene(scene);
	}

	char path_str[512];

	mutation_iterator iter;
	fbxdom::node_ptr mut_root;
	int current_step = 0;
	while ((mut_root = next_mutation(root, iter)) != nullptr) {
		if (selected_step >= 0) {
			if (current_step != selected_step) {
				current_step++;
				continue;
			}
		}
		if (start_step >= 0) {
			if (current_step < start_step) {
				current_step++;
				continue;
			}
		}

		format_path(path_str, sizeof(path_str), root, iter.path);
		if (g_verbose) {
			printf("%d: %s/ %s %u: ", current_step, path_str, mutators[iter.mutator_index].name, iter.mutator_internal_index);
		}

		size_t dump_size = fbxdom::dump(dump_buffer, sizeof(dump_buffer), mut_root);
		if (dry_run) {
			if (g_verbose) {
				printf("SKIP (dry run)\n");
			}
			continue;
		}

		size_t pre_cov = g_coverage_counter;
		ufbx_scene *scene = ufbx_load_memory(dump_buffer, dump_size, NULL, &error);
		size_t post_cov = g_coverage_counter;

		if (scene) {
			ufbxt_check_scene(scene);
			if (g_verbose) {
				printf("OK!\n");
			}
		} else {
			if (g_verbose) {
				printf("%s\n", error.description.data);
			}
		}

		if (coverage_path && pre_cov != post_cov) {
			size_t index = ++coverage_output_count;
			char dst_path[1024];
			if (!g_verbose) {
				printf("%d: %s/ %s %u: ", current_step, path_str, mutators[iter.mutator_index].name, iter.mutator_internal_index);
			}
			snprintf(dst_path, sizeof(dst_path), "%s%06zu.fbx", coverage_path, coverage_output_count);
			printf("%zu new edges! Writing to: %s\n", post_cov - pre_cov, dst_path);
			FILE *f = fopen(dst_path, "wb");
			ufbxt_assert(f);
			ufbxt_assert(fwrite(dump_buffer, 1, dump_size, f) == dump_size);
			ufbxt_assert(fclose(f) == 0);
		}

		ufbx_free_scene(scene);
		current_step++;
	}

	return true;
}

void process_coverage(const char *input_file, const char *output)
{
	FILE *f = fopen(input_file, "r");
	ufbxt_assert(f);

	std::vector<std::string> files;

	printf("== DRY RUN ==\n");

	char line_buf[1024];
	while (fgets(line_buf, sizeof(line_buf), f)) {
		std::string line { line_buf };
		while (!line.empty() && std::isspace(line.back())) {
			line.pop_back();
		}
		if (line.empty()) continue;

		printf("-- %s --\n", line.c_str());
		if (!process_fbx_file(line.c_str(), NULL, true)) {
			fprintf(stderr, "Failed to load test case\n");
			exit(3);
		}

		files.push_back(std::move(line));
	}

	fclose(f);

	if (g_coverage_counter == 0) {
		fprintf(stderr, "No coverage detected.. make sure to compile with DOMFUZZ_COVERAGE");
		exit(3);
	}

	printf("Found %zu initial edges\n", g_coverage_counter);
	printf("== REAL RUN ==\n");

	for (const std::string &file : files) {
		printf("-- %s --\n", file.c_str());
		if (!process_fbx_file(file.c_str(), output, false)) {
			fprintf(stderr, "Failed to load test case\n");
			exit(3);
		}
	}
}

int main(int argc, char **argv)
{
	const char *input_file = NULL;
	const char *coverage_output = NULL;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) {
			g_verbose = 1;
		} else if (!strcmp(argv[i], "--step")) {
			if (++i < argc) selected_step = atoi(argv[i]);
		} else if (!strcmp(argv[i], "--start")) {
			if (++i < argc) start_step = atoi(argv[i]);
		} else if (!strcmp(argv[i], "--coverage")) {
			if (++i < argc) coverage_output = argv[i];
		} else {
			input_file = argv[i];
		}
	}

	if (coverage_output) {
		process_coverage(input_file, coverage_output);
	} else if (input_file) {
		process_fbx_file(input_file, NULL, false);
	} else {
		fprintf(stderr, "Usage: domfuzz <file.fbx>\n");
	}

	return 0;
}
