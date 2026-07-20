#include "../ufbx.c"
#include <inttypes.h>

typedef struct {
	uint32_t begin;
	uint32_t increment;
	uint64_t attempts;
	uint32_t *slots;
	size_t num_slots;
	size_t target_slot;
} hash_info;

typedef void hash_fn(hash_info info);

typedef struct {
	char str[256];
	uint32_t *slots;
	size_t mask;
	size_t max_length;
	int64_t attempts_left;
	size_t target_slot;
} str_state;

ufbxi_noinline void print_string(str_state *state, size_t length)
{
#if _OPENMP
	#pragma omp critical
#endif
	{
		state->str[length] = '\0';
		puts(state->str);
	}
}

ufbxi_noinline void hash_string_imp(str_state *state, size_t length)
{
	if (state->attempts_left < 0) return;
	state->attempts_left -= ('Z' - 'A' + 1) * 2;

	uint32_t *slots = state->slots;
	size_t mask = state->mask;
	size_t target_slot = state->target_slot;

	char *p = state->str + length - 1;
	for (uint32_t c = 'A'; c <= 'Z'; c++) {
		*p = c;
		uint32_t hash = ufbxi_hash_string(state->str, length);
		if ((hash & mask) == target_slot) print_string(state, length);
		slots[hash & mask]++;
		*p = c | 0x20;
		hash = ufbxi_hash_string(state->str, length);
		if ((hash & mask) == target_slot) print_string(state, length);
		slots[hash & mask]++;
	}

	if (length < state->max_length) {
		for (uint32_t c = 'A'; c <= 'Z'; c++) {
			*p = c;
			hash_string_imp(state, length + 1);
			*p = c | 0x20;
			hash_string_imp(state, length + 1);
		}
	}
}

ufbxi_noinline void hash_string(hash_info info)
{
	size_t mask = info.num_slots - 1;
	str_state state;
	state.attempts_left = (int64_t)info.attempts;
	state.mask = mask;
	state.slots = info.slots;
	state.target_slot = info.target_slot;

	size_t max_len = 0;
	uint64_t len_attempts = 1;
	while (len_attempts < info.attempts) {
		len_attempts *= ('Z' - 'A' + 1) * 2;
		max_len += 1;
	}
	state.max_length = max_len;

	for (uint32_t c = 'A' + info.begin; c <= 'Z'; c += info.increment) {
		state.str[0] = c;
		uint32_t hash = ufbxi_hash_string(state.str, 1);
		if ((hash & mask) == info.target_slot) print_string(&state, 1);
		info.slots[hash & mask]++;
		if (max_len > 1) {
			hash_string_imp(&state, 2);
		}

		state.str[0] = c | 0x20;
		hash = ufbxi_hash_string(state.str, 1);
		if ((hash & mask) == info.target_slot) print_string(&state, 1);
		info.slots[hash & mask]++;
		if (max_len > 1) {
			hash_string_imp(&state, 2);
		}
	}
}

ufbxi_noinline void print_uint64(uint64_t v)
{
#if _OPENMP
	#pragma omp critical
#endif
	{
		printf("%" PRIu64 "\n", v);
	}
}


ufbxi_noinline void hash_uint64(hash_info info)
{
	size_t mask = info.num_slots - 1;
	uint64_t increment = info.increment;
	uint64_t end = info.attempts;
	for (uint64_t i = info.begin; i < end; i += increment) {
		uint32_t hash = ufbxi_hash64(i);
		if ((hash & mask) == info.target_slot) print_uint64(i);
		info.slots[hash & mask]++;
	}
}

ufbxi_noinline void run_test(hash_info info, hash_fn *fn, uint32_t *accumulator)
{
	info.slots = malloc(sizeof(uint32_t) * info.num_slots);
	memset(info.slots, 0, sizeof(uint32_t) * info.num_slots);
	fn(info);

#if _OPENMP
	#pragma omp critical
#endif
	{
		for (size_t i = 0; i < info.num_slots; i++) {
			accumulator[i] += info.slots[i];
		}
	}
}

typedef struct {
	const char *name;
	hash_fn *fn;
} hash_test;

hash_test tests[] = {
	"string", &hash_string,
	"uint64", &hash_uint64,
};

int main(int argc, char **argv)
{
	const char *func = "";
	size_t num_slots = 0;
	size_t target_slot = SIZE_MAX;
	uint64_t attempts = 0;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-f")) {
			if (++i < argc) func = argv[i];
		} else if (!strcmp(argv[i], "--slots")) {
			if (++i < argc) num_slots = (size_t)strtoull(argv[i], NULL, 10);
		} else if (!strcmp(argv[i], "--attempts")) {
			if (++i < argc) attempts = (uint64_t)strtoull(argv[i], NULL, 10);
		} else if (!strcmp(argv[i], "--target")) {
			if (++i < argc) target_slot = (size_t)strtoull(argv[i], NULL, 10);
		} else if (!strcmp(argv[i], "--threads")) {
			#if _OPENMP
			if (++i < argc) omp_set_num_threads(atoi(argv[i]));
			#endif
		}
	}

	hash_fn *hash_fn = NULL;
	for (size_t i = 0; i < ufbxi_arraycount(tests); i++) {
		if (!strcmp(tests[i].name, func)) {
			hash_fn = tests[i].fn;
			break;
		}
	}

	if (!hash_fn) {
		fprintf(stderr, "Unkonwn hash function '%s'\n", func);
		return 1;
	}

	if ((num_slots & (num_slots - 1)) != 0) {
		fprintf(stderr, "Slot amount must be a power of two, got %zu\n", num_slots);
		return 1;
	}

	uint32_t *slots = malloc(num_slots * sizeof(uint32_t));
	memset(slots, 0, sizeof(uint32_t) * num_slots);

	hash_info info;
	info.attempts = (int64_t)attempts;
	info.increment = 1;
	info.num_slots = num_slots;
	info.target_slot = target_slot;

#if _OPENMP
	#pragma omp parallel
	{
		info.begin = (uint64_t)omp_get_thread_num();
		info.increment = (uint64_t)omp_get_num_threads();
		run_test(info, hash_fn, slots);
	}
#else
	{
		info.begin = 0;
		info.increment = 1;
		run_test(info, hash_fn, slots);
	}
#endif

	uint32_t max_collisions = 0;
	size_t worst_slot = 0;
	for (size_t i = 0; i < num_slots; i++) {
		if (slots[i] > max_collisions) {
			max_collisions = slots[i];
			worst_slot = i;
		}
	}

	if (target_slot == SIZE_MAX) {
		printf("Worst slot: %zu (%u collisions)\n", worst_slot, max_collisions);
	}
	return 0;
}
