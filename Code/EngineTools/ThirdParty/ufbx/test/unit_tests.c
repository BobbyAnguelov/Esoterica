#include "../ufbx.c"
#include "../ufbx.h"

#include <stdio.h>
#include <string.h>

void ufbxt_assert_imp(bool cond, const char *message, int line)
{
	if (!cond) {
		fprintf(stderr, "unit_tests.c:%d: Assertion failed: %s\n", line, message);
		exit(1);
	}
}

#define ufbxt_assert(cond) ufbxt_assert_imp((cond), #cond, __LINE__)

typedef struct  {
	uint32_t a, b;
} uint_pair;

typedef enum {
	SORT_STABLE_MACRO,	
	SORT_STABLE_FUNCTION,	
	SORT_UNSTABLE_FUNCTION,	

	SORT_MODE_COUNT,
} sort_mode;

static size_t g_linear_size = 2;
static sort_mode g_sort_mode = SORT_STABLE_MACRO;

static bool uint_less(void *user, const void *va, const void *vb)
{
	uint32_t a = *(const uint32_t*)va, b = *(const uint32_t*)vb;
	return a < b;
}

static bool pair_less_a(void *user, const void *va, const void *vb)
{
	uint_pair a = *(const uint_pair*)va, b = *(const uint_pair*)vb;
	return a.a < b.a;
}

static bool pair_less_b(void *user, const void *va, const void *vb)
{
	uint_pair a = *(const uint_pair*)va, b = *(const uint_pair*)vb;
	return a.b < b.b;
}

static bool str_less(void *user, const void *va, const void *vb)
{
	const char *a = *(const char**)va, *b = *(const char**)vb;
	return strcmp(a, b) < 0;
}

void sort_uints(uint32_t *test_data, uint32_t *test_tmp, size_t test_size)
{
	switch (g_sort_mode) {
		case SORT_STABLE_MACRO:
			ufbxi_macro_stable_sort(uint32_t, g_linear_size, test_data, test_tmp, test_size, (*a < *b));
			break;
		case SORT_STABLE_FUNCTION:
			ufbxi_stable_sort(sizeof(uint32_t), g_linear_size, test_data, test_tmp, test_size, &uint_less, NULL);
			break;
		case SORT_UNSTABLE_FUNCTION:
			ufbxi_unstable_sort(test_data, test_size, sizeof(uint32_t), &uint_less, NULL);
			break;
		default:
			ufbxt_assert(0);
			break;
	}
}

void sort_pairs_by_a(uint_pair *data, uint_pair *tmp, size_t size)
{
	switch (g_sort_mode) {
		case SORT_STABLE_MACRO:
			ufbxi_macro_stable_sort(uint_pair, g_linear_size, data, tmp, size, (a->a < b->a));
			break;
		case SORT_STABLE_FUNCTION:
			ufbxi_stable_sort(sizeof(uint_pair), g_linear_size, data, tmp, size, &pair_less_a, NULL);
			break;
		case SORT_UNSTABLE_FUNCTION:
			ufbxi_unstable_sort(data, size, sizeof(uint_pair), &pair_less_a, NULL);
			break;
		default:
			ufbxt_assert(0);
			break;
	}
}

void sort_pairs_by_b(uint_pair *data, uint_pair *tmp, size_t size)
{
	switch (g_sort_mode) {
		case SORT_STABLE_MACRO:
			ufbxi_macro_stable_sort(uint_pair, g_linear_size, data, tmp, size, (a->b < b->b));
			break;
		case SORT_STABLE_FUNCTION:
			ufbxi_stable_sort(sizeof(uint_pair), g_linear_size, data, tmp, size, &pair_less_b, NULL);
			break;
		case SORT_UNSTABLE_FUNCTION:
			ufbxi_unstable_sort(data, size, sizeof(uint_pair), &pair_less_b, NULL);
			break;
		default:
			ufbxt_assert(0);
			break;
	}
}

size_t find_uint(uint32_t *test_data, size_t test_size, uint32_t value)
{
	size_t index = SIZE_MAX;

	ufbxi_macro_lower_bound_eq(uint32_t, g_linear_size, &index, test_data, 0, test_size,
		(*a < value),
		(*a == value));

	return index;
}

size_t find_uint_end(uint32_t *test_data, size_t test_size, size_t test_begin, uint32_t value)
{
	size_t index = SIZE_MAX;
	ufbxi_macro_upper_bound_eq(uint32_t, g_linear_size, &index, test_data, test_begin, test_size, (*a == value));
	return index;
}

size_t find_pair_by_a(uint_pair *data, size_t size, uint32_t value)
{
	size_t pair_ix = SIZE_MAX;
	ufbxi_macro_lower_bound_eq(uint_pair, g_linear_size, &pair_ix, data, 0, size,
		(a->a < value),
		(a->a == value));
	return pair_ix;
}

void sort_strings(const char **data, void *tmp, size_t size)
{
	switch (g_sort_mode) {
		case SORT_STABLE_MACRO:
			ufbxi_macro_stable_sort(const char*, g_linear_size, data, tmp, size, (strcmp(*a, *b) < 0));
			break;
		case SORT_STABLE_FUNCTION:
			ufbxi_stable_sort(sizeof(const char*), g_linear_size, data, tmp, size, &str_less, NULL);
			break;
		case SORT_UNSTABLE_FUNCTION:
			ufbxi_unstable_sort(data, size, sizeof(const char*), &str_less, NULL);
			break;
	}
}

size_t find_first_string(const char **data, size_t size, const char *str)
{
	size_t str_index;

	ufbxi_macro_lower_bound_eq(const char*, g_linear_size, &str_index, data, 0, size,
		(strcmp(*a, str) < 0),
		(strcmp(*a, str) == 0));

	return str_index;
}

static uint32_t xorshift32(uint32_t *state)
{
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint32_t x = *state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return *state = x;
}

static ufbx_real xorshift32_real(uint32_t *state)
{
	uint32_t u = xorshift32(state);
	return (ufbx_real)u * (ufbx_real)2.3283064365386962890625e-10;
}

void generate_linear(uint32_t *dst, size_t size, uint32_t start, uint32_t delta)
{
	for (size_t i = 0; i < size; i++) {
		dst[i] = start;
		start += delta;
	}
}

void generate_random(uint32_t *dst, size_t size, uint32_t seed, uint32_t mod)
{
	uint32_t state = seed | 1;
	for (size_t i = 0; i < size; i++) {
		dst[i] = xorshift32(&state) % mod;
	}
}

#define MAX_SORT_SIZE 2048

void test_sort(uint32_t *data, size_t size)
{
	ufbxt_assert(size <= MAX_SORT_SIZE);
	static size_t call_count = 0;
	static uint32_t uint_tmp_buffer[MAX_SORT_SIZE];
	static uint_pair pair_buffer[MAX_SORT_SIZE];
	static uint_pair pair_tmp_buffer[MAX_SORT_SIZE];
	call_count++;

	for (size_t i = 0; i < size; i++) {
		pair_buffer[i].a = data[i];
		pair_buffer[i].b = (uint32_t)i;
	}

	sort_uints(data, uint_tmp_buffer, size);
	sort_pairs_by_a(pair_buffer, pair_tmp_buffer, size);

	for (size_t i = 1; i < size; i++) {
		ufbxt_assert(data[i - 1] <= data[i]);
		ufbxt_assert(pair_buffer[i - 1].a <= pair_buffer[i].a);
		if (pair_buffer[i - 1].a == pair_buffer[i].a) {
			if (g_sort_mode != SORT_UNSTABLE_FUNCTION) {
				ufbxt_assert(pair_buffer[i - 1].b < pair_buffer[i].b);
			}
		}
	}

	for (size_t i = 0; i < size; i++) {
		uint32_t value = data[i];

		size_t index = find_uint(data, size, value);
		ufbxt_assert(index <= i);
		ufbxt_assert(data[index] == value);
		ufbxt_assert(index == find_pair_by_a(pair_buffer, size, value));
		if (index > 0) {
			ufbxt_assert(data[index - 1] < value);
		}

		size_t end = find_uint_end(data, size, index, value);
		ufbxt_assert(end > i);
		ufbxt_assert(data[end - 1] == value);
		if (end < size) {
			ufbxt_assert(data[end] > value);
		}
	}
	ufbxt_assert(find_uint(data, size, UINT32_MAX) == SIZE_MAX);

	sort_pairs_by_b(pair_buffer, pair_tmp_buffer, size);
	for (size_t i = 0; i < size; i++) {
		ufbxt_assert(pair_buffer[i].b == (uint32_t)i);
	}
}

void test_sort_strings(uint32_t *data, size_t size)
{
	assert(size <= MAX_SORT_SIZE);
	static size_t call_count = 0;
	static const char *str_buffer[MAX_SORT_SIZE];
	static char *str_tmp_buffer[MAX_SORT_SIZE];
	static char str_data_buffer[MAX_SORT_SIZE * 32];

	char *data_ptr = str_data_buffer, *data_end = data_ptr + sizeof(str_data_buffer);
	for (size_t i = 0; i < size; i++) {
		int len = snprintf(data_ptr, data_end - data_ptr, "%u", data[i]);
		ufbxt_assert(len > 0);
		str_buffer[i] = data_ptr;
		data_ptr += len + 1;
	}

	sort_strings(str_buffer, str_tmp_buffer, size);

	for (size_t i = 1; i < size; i++) {
		ufbxt_assert(strcmp(str_buffer[i - 1], str_buffer[i]) <= 0);
	}

	char find_str[128];
	for (size_t i = 0; i < size; i++) {
		int len = snprintf(find_str, sizeof(find_str), "%u", data[i]);
		ufbxt_assert(len > 0);

		size_t index = find_first_string(str_buffer, size, find_str);
		ufbxt_assert(index < size);
		ufbxt_assert(strcmp(str_buffer[index], find_str) == 0);
		if (index > 0) {
			ufbxt_assert(strcmp(str_buffer[index - 1], find_str) < 0);
		}
	}
}

// -- Bigdecimal

#define BIGDECIMAL_DIGITS 1024
#define BIGDECIMAL_SUFFIX 32

typedef struct {
	char digits[BIGDECIMAL_DIGITS + 2 + BIGDECIMAL_SUFFIX];
	size_t length;
} bigdecimal;

void bigdecimal_init(bigdecimal *d, int initial)
{
	ufbxt_assert(initial >= 0 && initial <= 9);
	d->digits[BIGDECIMAL_DIGITS + 1] = '\0';
	d->digits[BIGDECIMAL_DIGITS] = (char)(initial + '0');
	d->length = 1;
}

void bigdecimal_suffixf(bigdecimal *d, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(d->digits + BIGDECIMAL_DIGITS + 1, BIGDECIMAL_SUFFIX, fmt, args);
	va_end(args);
}

const char *bigdecimal_string(bigdecimal *d)
{
	for (size_t i = d->length; i > 0; i--) {
		if (d->digits[BIGDECIMAL_DIGITS - i + 1] != '0') {
			return d->digits + BIGDECIMAL_DIGITS - i + 1;
		}
	}
	return "";
}

void bigdecimal_mul(bigdecimal *d, int multiplicand)
{
	int carry = 0;
	for (uint32_t i = 0; i < d->length; i++) {
		int digit = d->digits[BIGDECIMAL_DIGITS - i] - '0';
		int product = digit * multiplicand + carry;
		d->digits[BIGDECIMAL_DIGITS - i] = (char)((product % 10) + '0');
		carry = product / 10;
	}
	if (carry) {
		ufbxt_assert(d->length < BIGDECIMAL_DIGITS);
		d->digits[BIGDECIMAL_DIGITS - d->length++] = (char)(carry + '0');
	}
}

void bigdecimal_add(bigdecimal *d, int addend)
{
	int carry = addend;
	for (uint32_t i = 0; i < d->length; i++) {
		int digit = d->digits[BIGDECIMAL_DIGITS - i] - '0';
		int sum = digit + carry;
		if (sum >= 0 && sum < 10) {
			d->digits[BIGDECIMAL_DIGITS - i] = (char)(sum + '0');
			carry = 0;
			break;
		} else if (sum < 0) {
			d->digits[BIGDECIMAL_DIGITS - i] = (char)((sum + 10) + '0');
			carry = -1;
		} else if (sum >= 10) {
			d->digits[BIGDECIMAL_DIGITS - i] = (char)((sum % 10) + '0');
			carry = 1;
		}
	}
	if (carry > 0) {
		ufbxt_assert(d->length < BIGDECIMAL_DIGITS);
		d->digits[BIGDECIMAL_DIGITS - d->length++] = (char)(carry + '0');
	} else if (carry < 0) {
		ufbxt_assert(0 && "Negative bigdecimal");
	}
}

static ufbxi_bigint_limb ufbxt_bigint_div_word(ufbxi_bigint *b, ufbxi_bigint_limb divisor)
{
	uint32_t new_length = 0;
	ufbxi_bigint_accum accum = 0;
	for (uint32_t i = b->length; i-- > 0; ) {
		accum = (accum << UFBXI_BIGINT_LIMB_BITS) | b->limbs[i];
		if (accum >= divisor) {
			ufbxi_bigint_accum quot = accum / divisor;
			ufbxi_bigint_accum rem = accum % divisor;

			b->limbs[i] = (ufbxi_bigint_limb)quot;
			if (quot > 0 && !new_length) {
				new_length = i + 1;
			}
			accum = rem;
		} else {
			b->limbs[i] = 0;
		}
	}
	b->length = new_length;
	return (ufbxi_bigint_limb)accum;
}

static void ufbxt_bigint_parse(ufbxi_bigint *b, const char *str)
{
	ufbxi_bigint_limb radix = 10;
	if (str[0] == '0' && str[1] == 'x') {
		radix = 16;
		str += 2;
	}

	b->length = 0;
	for (const char *c = str; *c; c++) {
		ufbxi_bigint_limb digit = UFBXI_BIGINT_LIMB_MAX;
		if (*c >= '0' && *c <= '9') {
			digit = (ufbxi_bigint_limb)(*c - '0');
		} else if (*c >= 'a' && *c <= 'z') {
			digit = (ufbxi_bigint_limb)(*c - 'a') + 10;
		}

		ufbxt_assert(digit < radix);
		ufbxi_bigint_mad(b, radix, digit);
	}
}

static char *ufbxt_bigint_format(char *buffer, size_t buffer_length, const ufbxi_bigint bi, ufbxi_bigint_limb radix)
{
	ufbxi_bigint_limb limbs[64];
	memcpy(limbs, bi.limbs, bi.length * sizeof(ufbxi_bigint_limb));
	ufbxi_bigint b = { limbs, sizeof(limbs) / sizeof(ufbxi_bigint_limb) };
	b.length = bi.length;

	const char *digits = "0123456789abcdef";

	size_t pos = buffer_length;
	buffer[--pos] = '\0';
	do {
		ufbxi_bigint_limb digit = ufbxt_bigint_div_word(&b, radix);
		ufbxt_assert(digit < radix);
		buffer[--pos] = digits[digit];
	} while (b.length > 0);

	return buffer + pos;
}

void ufbxt_check_bigint(ufbxi_bigint bi, const char *expected)
{
	ufbxi_bigint_limb radix = 10;
	if (expected[0] == '0' && expected[1] == 'x') {
		radix = 16;
		expected += 2;
	}

	char buffer[256];
	char *parsed = ufbxt_bigint_format(buffer, sizeof(buffer), bi, radix);
	if (strcmp(parsed, expected) != 0) {
		fprintf(stderr, "ufbxt_check_bigint() fail: got %s, expected %s\n",
			parsed, expected);
		ufbxt_assert(false);
	}

	// Leading zero is not allowed
	if (bi.length > 0) {
		ufbxt_assert(bi.limbs[bi.length - 1] != 0);
	}
}

void ufbxt_bigint_copy(ufbxi_bigint *dst, const ufbxi_bigint *src)
{
	memcpy(dst->limbs, src->limbs, src->length * sizeof(ufbxi_bigint_limb));
	dst->length = src->length;
	ufbxi_dev_assert(dst->capacity > src->length);
}

void test_bigint_basics()
{
	ufbxi_bigint_limb a_limbs[64];
	ufbxi_bigint a = ufbxi_bigint_array(a_limbs);

	ufbxt_bigint_parse(&a, "123");
	ufbxt_check_bigint(a, "123");

	ufbxt_bigint_parse(&a, "1230000000000000000000000000000000000000000456");
	ufbxt_check_bigint(a, "1230000000000000000000000000000000000000000456");

	ufbxt_bigint_parse(&a, "0xdead00beef00ab00cd00ef0012003400560079009a");
	ufbxt_check_bigint(a, "0xdead00beef00ab00cd00ef0012003400560079009a");

	ufbxt_bigint_parse(&a, "0x0000000000000000000000dead00beef00ab00cd00ef0012003400560079009a");
	ufbxt_check_bigint(a, "0xdead00beef00ab00cd00ef0012003400560079009a");
}

void test_bigint_mad()
{
	ufbxi_bigint_limb a_limbs[64];
	ufbxi_bigint a = ufbxi_bigint_array(a_limbs);

	ufbxt_bigint_parse(&a, "1000");
	ufbxi_bigint_mad(&a, 4, 321);
	ufbxt_check_bigint(a, "4321");

	ufbxt_bigint_parse(&a, "4000000000");
	ufbxi_bigint_mad(&a, 2, 0);
	ufbxt_check_bigint(a, "8000000000");

	ufbxt_bigint_parse(&a, "0xffffffffffffffffffffffffffffffff");
	ufbxi_bigint_mad(&a, UINT64_C(9223372036854775807), UINT64_C(9223372036854775807));
	ufbxt_check_bigint(a, "0x7fffffffffffffff00000000000000000000000000000000");

	ufbxt_bigint_parse(&a, "0");
	ufbxi_bigint_mad(&a, UINT64_C(1000000000000000000), UINT64_C(111222333344455566));
	ufbxi_bigint_mad(&a, UINT64_C(10000000000), UINT64_C(6777888999));
	ufbxt_check_bigint(a, "1112223333444555666777888999");
}

void test_bigint_pow5()
{
	ufbxi_bigint_limb a_limbs[64];
	ufbxi_bigint a = ufbxi_bigint_array(a_limbs);

	ufbxt_bigint_parse(&a, "1000");
	ufbxi_bigint_mul_pow5(&a, 1);
	ufbxt_check_bigint(a, "5000");

	ufbxt_bigint_parse(&a, "1");
	ufbxi_bigint_mul_pow5(&a, 20);
	ufbxt_check_bigint(a, "95367431640625");

	ufbxt_bigint_parse(&a, "1");
	ufbxi_bigint_mul_pow5(&a, 40);
	ufbxt_check_bigint(a, "9094947017729282379150390625");

	ufbxt_bigint_parse(&a, "1");
	ufbxi_bigint_mul_pow5(&a, 300);
	ufbxt_check_bigint(a,
		"49090934652977265530957719549862756429752155124994495651115491171871052547217158"
		"56460097884037331952277183571565131878513167918610424718902807514824108963452253"
		"10546445986192853894181098439730703830718994140625");

	ufbxt_bigint_parse(&a, "123");
	ufbxi_bigint_mul_pow5(&a, 0);
	ufbxt_check_bigint(a, "123");

	ufbxt_bigint_parse(&a, "10000000000010000000000010000000000");
	ufbxi_bigint_mul_pow5(&a, 1);
	ufbxt_check_bigint(a, "50000000000050000000000050000000000");
}

void test_bigint_shift_left()
{
	ufbxi_bigint_limb a_limbs[64];
	ufbxi_bigint a = ufbxi_bigint_array(a_limbs);

	ufbxt_bigint_parse(&a, "1");
	ufbxi_bigint_shift_left(&a, 3);
	ufbxt_check_bigint(a, "8");

	ufbxt_bigint_parse(&a, "0x80000000");
	ufbxi_bigint_shift_left(&a, 1);
	ufbxt_check_bigint(a, "0x100000000");

	ufbxt_bigint_parse(&a, "0x123456789abcdef0123456789abcdef0123456789abcdef");
	ufbxi_bigint_shift_left(&a, 20);
	ufbxt_check_bigint(a, "0x123456789abcdef0123456789abcdef0123456789abcdef00000");

	ufbxt_bigint_parse(&a, "123456789");
	ufbxi_bigint_shift_left(&a, 0);
	ufbxt_check_bigint(a, "123456789");

	ufbxt_bigint_parse(&a, "12345678900000000000000000000");
	ufbxi_bigint_shift_left(&a, 0);
	ufbxt_check_bigint(a, "12345678900000000000000000000");

	ufbxt_bigint_parse(&a, "1");
	ufbxi_bigint_shift_left(&a, 32);
	ufbxt_check_bigint(a, "0x100000000");

	ufbxt_bigint_parse(&a, "0xa");
	ufbxi_bigint_shift_left(&a, 48);
	ufbxt_check_bigint(a, "0xa000000000000");
}

void test_bigint_div_manual()
{
	ufbxi_bigint_limb a_limbs[64], b_limbs[64], c_limbs[64];
	ufbxi_bigint a = ufbxi_bigint_array(a_limbs);
	ufbxi_bigint b = ufbxi_bigint_array(b_limbs);
	ufbxi_bigint c = ufbxi_bigint_array(c_limbs);
	bool rem = false;

	ufbxt_bigint_parse(&a, "0x10000000000000000");
	ufbxt_bigint_parse(&b, "0x8000000000000000");
	rem = ufbxi_bigint_div(&c, &a, &b);
	ufbxt_check_bigint(c, "2");
	ufbxt_assert(!rem);
}

static void ufbxt_bigint_shift_right1(ufbxi_bigint *b)
{
	uint32_t length = b->length;
	b->limbs[length] = 0;
	for (uint32_t i = 0; i < length; i++) {
		b->limbs[i] = (b->limbs[i] >> 1) | (b->limbs[i + 1] << (UFBXI_BIGINT_LIMB_BITS - 1));
		if (b->limbs[i]) b->length = i + 1;
	}
}

static void ufbxt_check_bigint_div(const char *dividend, const char *divisor, const char *quotient, bool remainder)
{
	ufbxi_bigint_limb a_limbs[64], b_limbs[64], c_limbs[64];
	ufbxi_bigint a = ufbxi_bigint_array(a_limbs);
	ufbxi_bigint b = ufbxi_bigint_array(b_limbs);
	ufbxi_bigint c = ufbxi_bigint_array(c_limbs);

	ufbxt_bigint_parse(&a, dividend);
	ufbxt_bigint_parse(&b, divisor);

	ufbxt_assert(b.length >= 1);
	uint32_t shift = ufbxi_lzcnt64(b.limbs[b.length - 1]) - 32;
	if (b.length == 1) shift += UFBXI_BIGINT_LIMB_BITS;

	ufbxi_bigint_shift_left(&a, shift);
	ufbxi_bigint_shift_left(&b, shift);

	bool a_shifted = false;
	if (a.length && a.limbs[a.length - 1] >> (UFBXI_BIGINT_LIMB_BITS - 1) != 0) {
		ufbxi_bigint_shift_left(&a, 1);
		a_shifted = true;
	}

	bool rem = ufbxi_bigint_div(&c, &a, &b);

	if (a_shifted) {
		ufbxt_bigint_shift_right1(&c);
	}

	ufbxt_check_bigint(c, quotient);
	ufbxt_assert(rem == remainder);
}

void test_bigint_div()
{
	ufbxt_check_bigint_div("123", "10", "12", true);
	ufbxt_check_bigint_div("120", "10", "12", false);
	ufbxt_check_bigint_div("0xdeadbeef", "0x10000", "0xdead", true);
	ufbxt_check_bigint_div("82718061255302767487140869206996285356581211090087890625", "9094947017729282379150390625", "9094947017729282379150390625", false);
	ufbxt_check_bigint_div("82718061255302767487140869206996285356581211090087890625", "9094947017729282379150390624", "9094947017729282379150390626", true);
	ufbxt_check_bigint_div("82718061255302767487140869206996285356581211090087890625", "9094947017729282379150390626", "9094947017729282379150390624", true);
	ufbxt_check_bigint_div(
		"9173994463960286046443283581092555673948943761249553509941667449694421519688219440078102364094996072628225",
		"6277101735386680763495507056286727952620534092958556749825",
		"1461501637330902918282912995212100613184356876287", true);
	ufbxt_check_bigint_div(
		"0xffffffffffffffffffffffff00000000",
		"0x8000000000000000ffffffff",
		"0x1ffffffff", true);
	ufbxt_check_bigint_div(
		"0x7fffffff000000010000000000000000",
		"0x80000000fffffffefffffffe",
		"0xfffffffc", true);
	ufbxt_check_bigint_div(
		"0x7fffffff000000010000000000000000",
		"0x8000000000000001fffffffe",
		"0xfffffffd", true);
}

void ufbxt_check_float(const char *str)
{
	char *end;
	double ref_d = strtod(str, NULL);
	double slow_d = ufbxi_parse_double(str, strlen(str), &end, 0);
	double fast_d = ufbxi_parse_double(str, strlen(str), &end, UFBXI_PARSE_DOUBLE_ALLOW_FAST_PATH);
	float ref_f = strtof(str, NULL);
	float slow_f = (float)ufbxi_parse_double(str, strlen(str), &end, UFBXI_PARSE_DOUBLE_AS_BINARY32);

	if (isfinite(ref_d)) {
		if (slow_d != ref_d) {
			fprintf(stderr, "strtod() mismatch (slow): '%s': reference %.20g, ufbxc %.20g\n", str, ref_d, slow_d);
			ufbxt_assert(0);
		}
		if (fast_d != ref_d) {
			fprintf(stderr, "strtod() mismatch (fast): '%s': reference %.20g, ufbxc %.20g\n", str, ref_d, fast_d);
			ufbxt_assert(0);
		}
	} else {
		ufbxt_assert(!isfinite(fast_d));
		ufbxt_assert(!isfinite(slow_f));
	}
	if (isfinite(ref_f)) {
		if (slow_f != ref_f) {
			fprintf(stderr, "strtof() mismatch (slow): '%s': reference %.20g, ufbxc %.20g\n", str, ref_f, slow_f);
			ufbxt_assert(0);
		}
	} else {
		ufbxt_assert(!isfinite(slow_f));
	}
}

void ufbxt_check_nan(const char *str)
{
	char *end_d, *end_f;
	double slow_d = ufbxi_parse_double(str, strlen(str), &end_d, 0);
	float slow_f = (float)ufbxi_parse_double(str, strlen(str), &end_f, UFBXI_PARSE_DOUBLE_AS_BINARY32);
	ufbxt_assert(isnan(slow_d));
	ufbxt_assert(isnan(slow_f));
	ufbxt_assert(end_d == str + strlen(str));
	ufbxt_assert(end_f == str + strlen(str));
}

void ufbxt_check_inf(const char *str, int sign)
{
	char *end_d, *end_f;
	double slow_d = ufbxi_parse_double(str, strlen(str), &end_d, 0);
	float slow_f = (float)ufbxi_parse_double(str, strlen(str), &end_f, UFBXI_PARSE_DOUBLE_AS_BINARY32);
	ufbxt_assert(isinf(slow_d));
	ufbxt_assert(isinf(slow_f));
	ufbxt_assert(slow_d < 0 == sign < 0);
	ufbxt_assert(slow_f < 0 == sign < 0);
	ufbxt_assert(isinf(slow_f));
	ufbxt_assert(end_d == str + strlen(str));
	ufbxt_assert(end_f == str + strlen(str));
}

#define TEST_NINES \
	"9999999999999999999999999999999999999999" \
	"9999999999999999999999999999999999999999" \
	"9999999999999999999999999999999999999999" \
	"9999999999999999999999999999999999999999" \
	"9999999999999999999999999999999999999999"

#define TEST_ZEROS \
	"0000000000000000000000000000000000000000" \
	"0000000000000000000000000000000000000000" \
	"0000000000000000000000000000000000000000" \
	"0000000000000000000000000000000000000000" \
	"0000000000000000000000000000000000000000"

void test_double_parse()
{
	ufbxt_check_float("1");
	ufbxt_check_float("123.0");
	ufbxt_check_float("123.456");
	ufbxt_check_float("123e-6");
	ufbxt_check_float("-1.5");
	ufbxt_check_float("1112223333444.555666777888999");
	ufbxt_check_float("0");
	ufbxt_check_float("-0");
	ufbxt_check_float(".5");
	ufbxt_check_float("-.5");
	ufbxt_check_float("1e100");
	ufbxt_check_float("1e1000");
	ufbxt_check_float("1e-1000");
	ufbxt_check_float("1e10000");
	ufbxt_check_float("1e-10000");
	ufbxt_check_float("7.67844768714563e-239");
	ufbxt_check_float(TEST_NINES);
	ufbxt_check_float(TEST_NINES ".999999999999999999999999999999999999999");
	ufbxt_check_float(TEST_NINES "e+108");
	ufbxt_check_float(TEST_NINES "e+109");
	ufbxt_check_float(TEST_NINES "e+118");
	ufbxt_check_float(TEST_NINES "e+119");
	ufbxt_check_float(TEST_NINES "e+120");
	ufbxt_check_float(TEST_NINES "e+300");
	ufbxt_check_float(TEST_NINES "e-200");
	ufbxt_check_float(TEST_NINES "e-400");
	ufbxt_check_float(TEST_NINES "e-520");
	ufbxt_check_float(TEST_NINES "e-523");
	ufbxt_check_float(TEST_NINES "e-524");
	ufbxt_check_float(TEST_ZEROS "123");
	ufbxt_check_float(TEST_ZEROS ".123");
	ufbxt_check_float(TEST_ZEROS "." TEST_ZEROS "123");
	ufbxt_check_float(TEST_ZEROS "." TEST_ZEROS "123" TEST_ZEROS);
	ufbxt_check_float(TEST_ZEROS "." TEST_ZEROS TEST_ZEROS "123");
	ufbxt_check_float("241309881603643e20");
	ufbxt_check_float(".5.57999999993498");
	ufbxt_check_float("-71862.4328795732984723456847839347829321867347892347893274982374982349872136217381623872E-273");
	#if !defined(_MSC_VER)
		ufbxt_check_float("4656612873077392578125e-8");
	#endif
}

void test_double_parse_nan()
{
	ufbxt_check_nan("nan");
	ufbxt_check_nan("NAN");
	ufbxt_check_nan("NaN");
	ufbxt_check_nan("-nan");
	ufbxt_check_nan("+nan");
	ufbxt_check_nan("nan(1234)");
	ufbxt_check_nan("nan(0x123456789abcdef)");
	ufbxt_check_nan("nan(nan)");
	ufbxt_check_nan("nan(ind)");
	ufbxt_check_nan("nan(nans)");
	ufbxt_check_inf("inf", 1);
	ufbxt_check_inf("-inf", -1);
	ufbxt_check_inf("INF", 1);
	ufbxt_check_inf("INFINITY", 1);

	ufbxt_check_nan("1.#NAN");
	ufbxt_check_nan("0.#NAN12345678");
	ufbxt_check_nan("1.#IND");
	ufbxt_check_nan("-7.#NAN00");
	ufbxt_check_inf("1.#INF", 1);
	ufbxt_check_inf("-1.#INF", -1);
}

void test_double_parse_fmt(const char *fmt, int width, uint32_t bits)
{
	char buffer[128];
	printf("test_float_parse() %s %d %ubits\n", fmt, width, bits);

	uint32_t max_hi = 1 << bits;
	for (uint32_t hi = 0; hi < max_hi; hi++) {
		for (int32_t delta = -2; delta <= 2; delta++) {
			uint32_t bits_f = (hi << (32u - bits)) + (uint32_t)delta;
			uint64_t bits_d = ((uint64_t)hi << (64u - bits)) + (uint64_t)(int64_t)delta;

			float val_f;
			double val_d;
			memcpy(&val_f, &bits_f, sizeof(float));
			memcpy(&val_d, &bits_d, sizeof(double));

			if (isfinite(val_f)) {
				snprintf(buffer, sizeof(buffer), fmt, width, val_f);
				ufbxt_check_float(buffer);
			}
			if (isfinite(val_d)) {
				snprintf(buffer, sizeof(buffer), fmt, width, val_d);
				ufbxt_check_float(buffer);
			}
		}
	}
}

void test_double_parse_bits()
{
	uint32_t bits = 12;
	int max_width = 12;

	for (int width = 4; width <= max_width; width++) {
		test_double_parse_fmt("%.*f", width, bits);
	}
	for (int width = 4; width <= max_width; width++) {
		test_double_parse_fmt("%.*e", width, bits);
	}
}

void test_double_parse_decimal()
{
	bigdecimal pow2, pow5;
	bigdecimal_init(&pow2, 1);

	int max_pow2 = 128;
	int max_pow5 = 128;
	int min_exp = -32;
	int max_exp = 32;
	int max_delta = 8;

	size_t max_decimals = SIZE_MAX;
	// MSVC strtod() rounds numbers that don't fit in 64 bits incorrectly.
	#if defined(_MSC_VER)
		max_decimals = 19;
	#endif

	for (int p2 = 0; p2 < max_pow2; p2++) {
		memcpy(&pow5, &pow2, sizeof(bigdecimal));

		for (int p5 = 0; p5 < max_pow5; p5++) {

			if (pow5.length >= 2) {
				bigdecimal_add(&pow5, -max_delta);
				for (int d = -max_delta; d <= max_delta; d++) {

					for (int exp = min_exp; exp <= max_exp; exp++) {
						bigdecimal_suffixf(&pow5, "e%+d", exp);
						if (pow5.length > max_decimals) continue;

						const char *str = bigdecimal_string(&pow5);
						ufbxt_check_float(str);
					}

					bigdecimal_add(&pow5, 1);
				}
				bigdecimal_add(&pow5, -max_delta - 1);
			} else {
				for (int exp = min_exp; exp <= max_exp; exp++) {
					bigdecimal_suffixf(&pow5, "e%+d", exp);
					if (pow5.length > max_decimals) continue;

					const char *str = bigdecimal_string(&pow5);
					ufbxt_check_float(str);
				}
			}
				
			bigdecimal_mul(&pow5, 5);
		}

		bigdecimal_mul(&pow2, 2);
	}
}

void test_sorts()
{
	static uint32_t sort_buffer[MAX_SORT_SIZE];

	for (int i = 0; i < SORT_MODE_COUNT; i++) {
		g_sort_mode = (sort_mode)i;
		g_linear_size = 2;

		while (g_linear_size <= 64) {
			printf("test_sorts(): sort_mode=%d linear_size=%zu\n",
				(int)g_sort_mode, g_linear_size);

			for (size_t size = 0; size < MAX_SORT_SIZE; size += 1+ size/128 + size/512*32) {
				generate_linear(sort_buffer, size, 0, +1);
				test_sort(sort_buffer, size);
				generate_linear(sort_buffer, size, (uint32_t)size, -1);
				test_sort(sort_buffer, size);
				generate_random(sort_buffer, size, (uint32_t)size, 1+size%10);
				test_sort(sort_buffer, size);
				generate_random(sort_buffer, size, (uint32_t)size, UINT32_MAX);
				test_sort(sort_buffer, size);
			}

			{
				size_t size = MAX_SORT_SIZE;
				generate_linear(sort_buffer, size, 0, +1);
				test_sort_strings(sort_buffer, size);
				generate_linear(sort_buffer, size, (uint32_t)size, -1);
				test_sort_strings(sort_buffer, size);
				generate_random(sort_buffer, size, (uint32_t)size, 1+size%10);
				test_sort_strings(sort_buffer, size);
				generate_random(sort_buffer, size, (uint32_t)size, UINT32_MAX);
				test_sort_strings(sort_buffer, size);
			}

			g_linear_size += 1+g_linear_size/8;
		}
	}
}

#define UFBXT_TEST(name) { #name, &name }

typedef struct {
	const char *name;
	void (*func)();
} ufbxt_test;

ufbxt_test tests[] = {
	UFBXT_TEST(test_bigint_basics),
	UFBXT_TEST(test_bigint_mad),
	UFBXT_TEST(test_bigint_pow5),
	UFBXT_TEST(test_bigint_shift_left),
	UFBXT_TEST(test_bigint_div_manual),
	UFBXT_TEST(test_bigint_div),
	UFBXT_TEST(test_double_parse),
	UFBXT_TEST(test_double_parse_nan),
	UFBXT_TEST(test_double_parse_bits),
	UFBXT_TEST(test_double_parse_decimal),
	UFBXT_TEST(test_sorts),
};

int main(int argc, char **argv)
{
	const char *test_filter = NULL;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--test")) {
			if (++i < argc) {
				test_filter = argv[i];
			}
		}
	}

	size_t num_tests = sizeof(tests) / sizeof(*tests);
	for (size_t i = 0; i < num_tests; i++) {
		const ufbxt_test *test = &tests[i];
		if (test_filter && strcmp(test->name, test_filter) != 0) {
			continue;
		}

		printf("%s\n", test->name);
		test->func();
	}

	return 0;
}
