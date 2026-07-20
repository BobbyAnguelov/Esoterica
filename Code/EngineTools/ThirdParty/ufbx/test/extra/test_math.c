#include "../../extra/ufbx_math.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

void ufbxt_assert_imp(bool cond, const char *message, int line)
{
	if (!cond) {
		fprintf(stderr, "unit_tests.c:%d: Assertion failed: %s\n", line, message);
		exit(1);
	}
}

#define ufbxt_assert(cond) do { ufbxt_assert_imp((cond), #cond, __LINE__); } while(0)

typedef struct ufbxt_double_iterator ufbxt_double_iterator;
struct ufbxt_double_iterator {
	uint32_t bits;
	int32_t max_delta;
	uint64_t max_hi;

	uint64_t hi;
	int32_t delta;
	double value;

	uint64_t iter_count;
	uint64_t child_count;
	ufbxt_double_iterator *parent;
	const char* name;
};

static ufbxt_double_iterator ufbxt_begin_double(const char *name, uint32_t bits, uint32_t delta)
{
	ufbxt_double_iterator it = { bits, (int32_t)delta };
	it.name = name;
	it.delta = -it.max_delta;
	it.max_hi = ((uint64_t)1 << bits) - 1;
	printf("%12s() ", name);
	return it;
}

static ufbxt_double_iterator ufbxt_begin_inner_double(ufbxt_double_iterator *parent, uint32_t bits, uint32_t delta)
{
	ufbxt_double_iterator it = { bits, (int32_t)delta };
	it.parent = parent;
	it.delta = -it.max_delta;
	it.max_hi = ((uint64_t)1 << bits) - 1;
	return it;
}

static bool ufbxt_next_double(ufbxt_double_iterator *it)
{
	if (it->delta < it->max_delta) {
		it->delta++;
	} else if (it->hi < it->max_hi) {
		it->delta = -it->max_delta;
		it->hi++;
		if (it->name && (it->hi & ((1 << (it->bits - 5)) - 1)) == 0) {
			putchar('.');
		}
	} else {
		if (it->child_count) it->iter_count = it->child_count;
		if (it->parent) {
			it->parent->child_count += it->iter_count;
		}
		if (it->name) {
			double count = it->iter_count;
			const char *unit = " ";
			if (count >= 1e9) {
				unit = "G";
				count /= 1e9;
			} else if (count >= 1e6) {
				unit = "M";
				count /= 1e6;
			} else if (count >= 1e3) {
				unit = "k";
				count /= 1e3;
			}
			printf(" %.1f%s\n", count, unit);
		}
		return false;
	}

	int64_t delta = it->delta;
	delta = delta * delta * delta;
	uint64_t bits_d = (it->hi << (64u - it->bits)) + (uint64_t)delta;
	memcpy(&it->value, &bits_d, sizeof(double));
	it->iter_count++;

	return true;
}

static void ufbxt_check_unary_exact(const char *name, double input, double reference, double result)
{
	bool ok = false;
	if (isnan(reference)) {
		ok = isnan(result) != 0;
	} else {
		ok = reference == result && 1.0 / reference == 1.0 / result;
	}
	if (!ok) {
		fprintf(stderr, " FAIL\n\n%s(%.16g) reference=%.16g result=%.16g\n", name, input, reference, result);
		exit(1);
	}
}

static void ufbxt_check_unary_relative(const char *name, double input, double reference, double result, double max_error)
{
	bool ok = false;
	if (isnan(reference)) {
		ok = isnan(result) != 0;
	} else if (!isfinite(reference)) {
		ok = reference == result;
	} else {
		double scale = fabs(reference);
		if (scale < 1e-300) scale = 1e-300;
		double error = (reference - result) / scale;
		ok = fabs(error) <= max_error;
	}
	if (!ok) {
		fprintf(stderr, " FAIL \n\n%s(%.16g) reference=%.16g result=%.16g\n", name, input, reference, result);
		exit(1);
	}
}

static void ufbxt_check_binary_exact(const char *name, double a, double b, double reference, double result)
{
	bool ok = false;
	if (isnan(reference)) {
		ok = isnan(result) != 0;
	} else {
		ok = reference == result && 1.0 / reference == 1.0 / result;
	}
	if (!ok) {
		fprintf(stderr, " FAIL \n\n%s(%.16g, %.16g) reference=%.16g result=%.16g\n", name, a, b, reference, result);
		exit(1);
	}
}

static void ufbxt_check_binary_relative(const char *name, double a, double b, double reference, double result, double max_error)
{
	bool ok = false;
	if (isnan(reference)) {
		ok = isnan(result) != 0;
	} else if (!isfinite(reference)) {
		ok = reference == result;
	} else {
		double scale = fabs(reference);
		if (scale < 1e-300) scale = 1e-300;
		double error = (reference - result) / scale;
		ok = fabs(error) <= max_error;
	}
	if (!ok) {
		fprintf(stderr, " FAIL \n\n%s(%.16g, %.16g) reference=%.16g result=%.16g\n", name, a, b, reference, result);
		exit(1);
	}
}

int main(int argc, char **argv)
{
#if 1
	uint32_t bits = 18;
	uint32_t delta = 16;
	uint32_t binary_bits = 9;
	uint32_t binary_delta = 4;
#else
	uint32_t bits = 19;
	uint32_t delta = 1024;
	uint32_t binary_bits = 10;
	uint32_t binary_delta = 32;
#endif

	// -- Unary, exact

	for (ufbxt_double_iterator it = ufbxt_begin_double("fabs", bits, delta); ufbxt_next_double(&it); ) {
		double input = it.value;
		double reference = fabs(input);
		double result = ufbx_fabs(input);
		double max_error = 1e-15;
		ufbxt_check_unary_exact("fabs", input, reference, result);
	}

	for (ufbxt_double_iterator it = ufbxt_begin_double("rint", bits, delta); ufbxt_next_double(&it); ) {
		double input = it.value;
		double reference = rint(input);
		double result = ufbx_rint(input);
		double max_error = 1e-15;
		ufbxt_check_unary_exact("rint", input, reference, result);
	}

	for (ufbxt_double_iterator it = ufbxt_begin_double("ceil", bits, delta); ufbxt_next_double(&it); ) {
		double input = it.value;
		double reference = ceil(input);
		double result = ufbx_ceil(input);
		double max_error = 1e-15;
		ufbxt_check_unary_exact("ceil", input, reference, result);
	}

	for (ufbxt_double_iterator it = ufbxt_begin_double("sqrt", bits, delta); ufbxt_next_double(&it); ) {
		double input = it.value;
		double reference = sqrt(input);
		double result = ufbx_sqrt(input);
		ufbxt_check_unary_exact("sqrt", input, reference, result);
	}
	for (ufbxt_double_iterator it = ufbxt_begin_double("isnan", bits, delta); ufbxt_next_double(&it); ) {
		double input = it.value;
		bool reference = isnan(input) != 0;
		bool result = ufbx_isnan(input) != 0;
		ufbxt_assert(reference == result);
	}

	// -- Unary, approximate

	for (ufbxt_double_iterator it = ufbxt_begin_double("sin", bits, delta); ufbxt_next_double(&it); ) {
		double input = it.value;
		double reference = sin(input);
		double result = ufbx_sin(input);
		double max_error = 1e-15;
		ufbxt_check_unary_relative("sin", input, reference, result, max_error);
	}

	for (ufbxt_double_iterator it = ufbxt_begin_double("cos", bits, delta); ufbxt_next_double(&it); ) {
		double input = it.value;
		double reference = cos(input);
		double result = ufbx_cos(input);
		double max_error = 1e-15;
		ufbxt_check_unary_relative("cos", input, reference, result, max_error);
	}

	for (ufbxt_double_iterator it = ufbxt_begin_double("tan", bits, delta); ufbxt_next_double(&it); ) {
		double input = it.value;
		double reference = tan(input);
		double result = ufbx_tan(input);
		double max_error = 1e-15;
		ufbxt_check_unary_relative("tan", input, reference, result, max_error);
	}

	for (ufbxt_double_iterator it = ufbxt_begin_double("asin", bits, delta); ufbxt_next_double(&it); ) {
		double input = it.value;
		double reference = asin(input);
		double result = ufbx_asin(input);
		double max_error = 1e-15;
		ufbxt_check_unary_relative("asin", input, reference, result, max_error);
	}

	for (ufbxt_double_iterator it = ufbxt_begin_double("acos", bits, delta); ufbxt_next_double(&it); ) {
		double input = it.value;
		double reference = acos(input);
		double result = ufbx_acos(input);
		double max_error = 1e-15;
		ufbxt_check_unary_relative("acos", input, reference, result, max_error);
	}

	for (ufbxt_double_iterator it = ufbxt_begin_double("atan", bits, delta); ufbxt_next_double(&it); ) {
		double input = it.value;
		double reference = atan(input);
		double result = ufbx_atan(input);
		double max_error = 1e-15;
		ufbxt_check_unary_relative("atan", input, reference, result, max_error);
	}

	// -- Binary, exact

	for (ufbxt_double_iterator at = ufbxt_begin_double("fmin", binary_bits, binary_delta); ufbxt_next_double(&at); ) {
		for (ufbxt_double_iterator bt = ufbxt_begin_inner_double(&at, binary_bits, binary_delta); ufbxt_next_double(&bt); ) {
			double a = at.value;
			double b = bt.value;
			double reference = fmin(a, b);
			double result = ufbx_fmin(a, b);
			if (isnan(a) || isnan(b)) continue;
			if (a == 0.0 && b == 0.0) {
				ufbxt_assert(result == 0.0);
				continue;
			}
			ufbxt_check_binary_exact("fmin", a, b, reference, result);
		}
	}

	for (ufbxt_double_iterator at = ufbxt_begin_double("fmax", binary_bits, binary_delta); ufbxt_next_double(&at); ) {
		for (ufbxt_double_iterator bt = ufbxt_begin_inner_double(&at, binary_bits, binary_delta); ufbxt_next_double(&bt); ) {
			double a = at.value;
			double b = bt.value;
			double reference = fmax(a, b);
			double result = ufbx_fmax(a, b);
			if (isnan(a) || isnan(b)) continue;
			if (a == 0.0 && b == 0.0) {
				ufbxt_assert(result == 0.0);
				continue;
			}
			ufbxt_check_binary_exact("fmax", a, b, reference, result);
		}
	}

	for (ufbxt_double_iterator at = ufbxt_begin_double("copysign", binary_bits, binary_delta); ufbxt_next_double(&at); ) {
		for (ufbxt_double_iterator bt = ufbxt_begin_inner_double(&at, binary_bits, binary_delta); ufbxt_next_double(&bt); ) {
			double a = at.value;
			double b = bt.value;
			double reference = copysign(a, b);
			double result = ufbx_copysign(a, b);
			ufbxt_check_binary_exact("copysign", a, b, reference, result);
		}
	}

	for (ufbxt_double_iterator at = ufbxt_begin_double("nextafter", binary_bits, binary_delta); ufbxt_next_double(&at); ) {
		for (ufbxt_double_iterator bt = ufbxt_begin_inner_double(&at, binary_bits, binary_delta); ufbxt_next_double(&bt); ) {
			double a = at.value;
			double b = bt.value;
			double reference = nextafter(a, b);
			double result = ufbx_nextafter(a, b);
			if (b == 0) continue;
			ufbxt_check_binary_exact("nextafter", a, b, reference, result);
		}
	}

	// -- Binary, approximate

	for (ufbxt_double_iterator at = ufbxt_begin_double("atan2", binary_bits, binary_delta); ufbxt_next_double(&at); ) {
		for (ufbxt_double_iterator bt = ufbxt_begin_inner_double(&at, binary_bits, binary_delta); ufbxt_next_double(&bt); ) {
			double a = at.value;
			double b = bt.value;
			double reference = atan2(a, b);
			double result = ufbx_atan2(a, b);
			double max_error = 1e-15;
			ufbxt_check_binary_relative("atan2", a, b, reference, result, max_error);
		}
	}

	for (ufbxt_double_iterator at = ufbxt_begin_double("pow", binary_bits, binary_delta); ufbxt_next_double(&at); ) {
		for (ufbxt_double_iterator bt = ufbxt_begin_inner_double(&at, binary_bits, binary_delta); ufbxt_next_double(&bt); ) {
			double a = at.value;
			double b = bt.value;
			double reference = pow(a, b);
			double result = ufbx_pow(a, b);
			double max_error = 1e-15;
			ufbxt_check_binary_relative("pow", a, b, reference, result, max_error);
		}
	}
}


#include "../../extra/ufbx_math.c"
