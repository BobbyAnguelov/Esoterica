#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define FLAG_MASK 0x30e4035f

typedef struct afl_state_t afl_state_t;

static size_t min_sz(size_t a, size_t b) { return a < b ? a : b; }
static size_t max_sz(size_t a, size_t b) { return a > b ? a : b; }

typedef struct {
    afl_state_t *afl;
    unsigned char *mutated_out;
    size_t mutated_out_cap;
    uint32_t counter;
} flag_mutator;

void *afl_custom_init(afl_state_t *afl, unsigned int seed)
{
    flag_mutator *mut = calloc(1, sizeof(flag_mutator));
    mut->afl = afl;
    return mut;
}

unsigned int afl_custom_fuzz_count(flag_mutator *mut, const unsigned char *buf, size_t buf_size)
{
    return 1 << 14;
}

size_t afl_custom_fuzz(flag_mutator *mut, unsigned char *buf, size_t buf_size, unsigned char **out_buf, unsigned char *add_buf, size_t add_buf_size, size_t max_size)
{
    size_t result_size = min_sz(buf_size, max_size);
    if (result_size < 14) return 0;

    if (result_size > mut->mutated_out_cap) {
        mut->mutated_out_cap = max_sz(mut->mutated_out_cap * 2, result_size);
        mut->mutated_out = realloc(mut->mutated_out, mut->mutated_out_cap);
    }
    memcpy(mut->mutated_out, buf, result_size);

    uint32_t index = mut->counter++;
    uint32_t flags = *(const uint32_t*)mut->mutated_out + 2;

    for (uint32_t ix = 0; ix < 32; ix++) {
        uint32_t bit = 1u << ix;
        if (!(FLAG_MASK & bit)) continue;
    
        if (index & 1) {
            flags ^= bit;
        }
        index >>= 1;
    }

    *(uint32_t*)mut->mutated_out = flags;

    *out_buf = mut->mutated_out;
    return result_size;
}

void afl_custom_deinit(flag_mutator *mut)
{
    free(mut->mutated_out);
    free(mut);
}
