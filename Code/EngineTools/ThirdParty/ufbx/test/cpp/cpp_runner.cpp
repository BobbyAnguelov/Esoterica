#include "../../ufbx.h"
#include "../../extra/ufbx_stl.h"

struct ufbxt_assert_exception {
	const char *desc;
};

struct ufbxt_error_exception {
	ufbx_error error;
};

#define ufbxt_assert(p_cond) do { if (!(p_cond)) { \
	throw ufbxt_assert_exception{#p_cond}; } } while (0)

struct ufbxt_cpp_test {
	static ufbxt_cpp_test *last;

	void (*test_fn)();
	const char *name;
	const ufbxt_cpp_test *prev;

	ufbxt_cpp_test(void (*test_fn)(), const char *name)
		: test_fn(test_fn), name(name), prev(last)
	{
		last = this;
	}
};

ufbxt_cpp_test *ufbxt_cpp_test::last = nullptr;

#define UFBXT_CPP_TEST(p_name) \
	void p_name(); \
	ufbxt_cpp_test ufbxt_test_##p_name { &p_name, #p_name }; \
	void p_name()

#include <stdio.h>
#include <string.h>
#include <string>
#include <memory>
#include <vector>
#include <algorithm>

std::string g_data_root;

std::string data_path(const char *filename)
{
	return g_data_root + filename;
}

ufbx_unique_ptr<ufbx_scene> load_scene(const char *filename, const ufbx_load_opts &opts={})
{
	ufbx_error error;
	ufbx_scene *scene = ufbx_load_file(data_path(filename), &opts, &error);
	if (!scene) throw ufbxt_error_exception{error};
	return ufbx_unique_ptr<ufbx_scene>{scene};
}

#include "cpp_test_basic.h"
#include "cpp_test_pointer.h"
#include "cpp_test_callback.h"
#include "cpp_test_list.h"
#include "cpp_test_convert.h"

int main(int argc, char **argv)
{
	std::vector<ufbxt_cpp_test> tests;
	const char *test_filter = nullptr;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--test")) {
			if (++i < argc) {
				test_filter = argv[i];
			}
		}
		if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--data")) {
			if (++i < argc) {
				g_data_root = argv[i];
				if (!g_data_root.empty()) {
					char last = g_data_root[g_data_root.size() - 1];
					if (last != '/' && last != '\\') {
						g_data_root.push_back('/');
					}
				}
			}
		}
	}


	{
		const ufbxt_cpp_test *test = ufbxt_cpp_test::last;
		while (test) {
			tests.push_back(*test);
			test = test->prev;
		}
		std::reverse(tests.begin(), tests.end());
	}

	uint32_t num_ran = 0, num_ok = 0;
	for (const ufbxt_cpp_test &test : tests) {
		if (test_filter && strcmp(test.name, test_filter) != 0) {
			continue;
		}

		printf("%s: ", test.name);
		fflush(stdout);

		try {
			num_ran++;
			test.test_fn();
			printf("OK\n");
			num_ok++;
		} catch (const ufbxt_assert_exception &fail) {
			printf("FAIL\n");
			printf("  Assertion failure: %s\n", fail.desc);
		} catch (const ufbxt_error_exception &fail) {
			printf("FAIL\n");
			printf("  ufbx error: %s\n", ufbx_format_error_string(fail.error).c_str());
		}
	}

	printf("\nTests passed: %u/%u\n", num_ok, num_ran);
	return num_ok == num_ran ? 0 : 1;
}
