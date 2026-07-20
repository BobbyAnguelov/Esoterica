#include "../ufbx.h"
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <stdio.h>

std::atomic_int32_t atomic_count { 0 };
int32_t race_count = 0;
bool allocator_freed = false;

void free_allocator(void*)
{
	allocator_freed = true;
}

void worker(ufbx_scene *scene)
{
	std::this_thread::sleep_for(std::chrono::milliseconds{100});
	for (uint32_t i = 0; i < 0x1000000; i++) {
		ufbx_retain_scene(scene);
		ufbx_free_scene(scene);
		atomic_count.fetch_add(1, std::memory_order_relaxed);
		race_count++;
	}
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "Usage: check_threads <file.fbx> <num-threads>\n");
		return 1;
	}

	if (!ufbx_is_thread_safe()) {
		fprintf(stderr, "WARNING: Not thread safe, expect failure!\n");
	}

	size_t num_threads = (size_t)atoi(argv[2]);

	ufbx_load_opts opts = { };
	opts.result_allocator.allocator.free_allocator_fn = &free_allocator;

	ufbx_error error;
	ufbx_scene *scene = ufbx_load_file(argv[1], &opts, &error);
	if (!scene) {
		fprintf(stderr, "Failed to load scene: %s\n", error.description.data);
		return 1;
	}

	if (allocator_freed) {
		fprintf(stderr, "Allocator prematurely freed\n");
		return 1;
	}

	for (uint32_t run = 0; run < 8; run++) {
		std::vector<std::thread> threads;
		for (size_t i = 0; i < num_threads; i++) {
			threads.push_back(std::thread(worker, scene));
		}

		for (std::thread &thread : threads) {
			thread.join();
		}

		int32_t delta = std::abs(atomic_count.load(std::memory_order_relaxed) - race_count);
		printf("Race delta: %d\n", delta);
		if (delta >= 1000) break;
	}

	ufbx_free_scene(scene);

	if (!allocator_freed) {
		fprintf(stderr, "Scene not freed\n");
		return 1;
	}

	return 0;
}
