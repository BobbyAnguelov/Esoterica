#ifndef SEMFUZZ_DECODE_FILE_H
#define SEMFUZZ_DECODE_FILE_H

#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <assert.h>

#include "string_table.h"

#if defined(_MSC_VER)
	#define FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
	#define FORCEINLINE inline __attribute__((always_inline))
#else
	#define FORCEINLINE
#endif

namespace semfuzz {

typedef uint16_t string_id;

enum {
	TYPE_C = 0x0,
	TYPE_Y = 0x1,
	TYPE_I = 0x2,
	TYPE_L = 0x3,
	TYPE_F = 0x4,
	TYPE_D = 0x5,
	TYPE_S = 0x6,
	TYPE_R = 0x7,
};

static const uint32_t binary_array_elem_size[] = { 1, 1, 4, 8, 4, 8, 0, 0 };
static const char *binary_array_type = "cbilfd";
static const char *binary_value_type = "CYILFDSR";
static const char zero_buffer[256] = { };

static int64_t array_buffer[0x4000];

enum {
    INST_FIELD = 0x0,

	INST_ARRAY_C = 0x1,
	INST_ARRAY_B = 0x2,
	INST_ARRAY_I = 0x3,
	INST_ARRAY_L = 0x4,
	INST_ARRAY_F = 0x5,
	INST_ARRAY_D = 0x6,

	INST_VALUE_C = 0x7,
	INST_VALUE_Y = 0x8,
	INST_VALUE_I = 0x9,
	INST_VALUE_L = 0xa,
	INST_VALUE_F = 0xb,
	INST_VALUE_D = 0xc,
	INST_VALUE_S = 0xe,
	INST_VALUE_R = 0xf,
};

static const uint8_t inst_category[] = { 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2 };

enum {
	ARRAY_PATTERN_RANDOM = 0x0,
	ARRAY_PATTERN_ASCENDING = 0x1,
	ARRAY_PATTERN_POLYGON = 0x2,
	ARRAY_PATTERN_REFCOUNT = 0x3,
};

#define NO_ARRAY 0xffff

struct Value {
    uint16_t type;
    uint16_t value;
    uint32_t next_sibling;
};

struct Field {
    string_id name;

    uint32_t first_child;
    uint32_t last_child;
    uint32_t next_sibling;

    uint32_t first_value;
    uint32_t last_value;
    uint32_t num_values;

    uint16_t array_type;
    uint16_t array_size;
    uint16_t array_param;
    uint16_t array_hash;
};

struct File {
    Field *fields = nullptr;
    size_t max_fields = 0;
    Value *values = nullptr;
    size_t max_values = 0;

    uint32_t num_fields = 0;
    uint32_t num_values = 0;

    uint16_t version = 0;
    uint32_t flags = 0;
    uint32_t temp_limit = 0;
    uint32_t result_limit = 0;

	uint32_t add_child(uint32_t parent_index, string_id name)
	{
		Field field = { };
		field.name = name;
        field.array_type = NO_ARRAY;

        if (num_fields == max_fields) return ~0u;
		uint32_t index = (uint32_t)num_fields;
        fields[index] = field;
        num_fields++;

		Field &parent = fields[parent_index];
		if (parent.first_child == 0) {
			parent.first_child = index;
			parent.last_child = index;
		} else {
			Field &prev = fields[parent.last_child];
			prev.next_sibling = index;
			parent.last_child = index;
		}
		return index;
	}

	uint32_t add_value(uint32_t parent_index, uint16_t type, uint16_t value)
	{
        if (num_values == max_fields) return ~0u;
		uint32_t index = (uint32_t)num_values;
        values[index] = Value{ type, value };
        num_values++;

		Field &parent = fields[parent_index];
		parent.num_values++;
		if (parent.first_value == 0) {
			parent.first_value = index;
			parent.last_value = index;
		} else {
			Value &prev = values[parent.last_value];
			prev.next_sibling = index;
			parent.last_value = index;
		}
		return index;
	}
};

static uint16_t read_u16(const uint8_t *ptr)
{
    return (uint16_t)((uint32_t)ptr[0] | (uint32_t)ptr[1] << 8);
}

static uint32_t read_u32(const uint8_t *ptr)
{
    return ((uint32_t)ptr[0]
        | (uint32_t)ptr[1] << 8
        | (uint32_t)ptr[2] << 16
        | (uint32_t)ptr[3] << 24);
}

static bool read_fbb(File &file, const void *data, size_t size)
{
    file.num_values = 1;
    file.num_fields = 1;
    file.fields[0] = Field{};
    file.values[0] = Value{};

    uint32_t stack[17] = { 0 };

    const uint8_t *bytecode = (const uint8_t*)data;

    if (size < 14) return false;

    file.version = read_u16(bytecode + 0);
    file.flags = read_u32(bytecode + 2);
    file.temp_limit = read_u32(bytecode + 6);
    file.result_limit = read_u32(bytecode + 10);
    size_t pc = 14;

    while (pc + 2 <= size) {
        uint32_t inst_word = read_u16(bytecode + pc);
        uint32_t inst = inst_word & 0xf;
        uint32_t level = (inst_word >> 4) & 0xf;
#if 0
        if ((inst_word & 0xff) != ((inst_word >> 8) ^ 0xff)) {
            pc += 1;
            continue;
        }
#else
        uint32_t check = (inst_word >> 8) ^ 0xff;
        uint32_t check_inst = check & 0xf;
        uint32_t check_level = (check >> 4) & 0xf;
        if (level != check_level || inst_category[inst] != inst_category[check_inst]) {
            pc += 1;
            continue;
        }
#endif


        if (inst == INST_FIELD) {
            if (pc + 4 > size) break;
			if (level != 0 || stack[level] == 0) {
                uint32_t index = file.add_child(stack[level], read_u16(bytecode + pc + 1*2));
                if (index == ~0u) return false;
                stack[level + 1] = index;
            }
			pc += 4;
        } else if (inst >= INST_ARRAY_C && inst <= INST_ARRAY_D) {
            if (pc + 12 > size) break;
			if (level == 0 || stack[level] != 0) {
                uint32_t index = file.add_child(stack[level], read_u16(bytecode + pc + 1*2));
                if (index == ~0u) return false;
                Field &field = file.fields[index];
                field.array_size = read_u16(bytecode + pc + 2*2);
                field.array_param = read_u16(bytecode + pc + 3*2);
                field.array_hash = read_u16(bytecode + pc + 4*2);
                field.array_type = inst - INST_ARRAY_C;

                uint32_t index_min = file.add_value(index, 0, read_u16(bytecode + pc + 5*2));
                uint32_t index_max = file.add_value(index, 0, read_u16(bytecode + pc + 6*2));
                if (index_min == ~0u || index_max == ~0u) return false;
            }
			pc += 14;
        } else if (inst >= INST_VALUE_C && inst <= INST_VALUE_R) {
            if (pc + 4 > size) break;
			if (stack[level] != 0) {
                Field &field = file.fields[stack[level]];
                uint32_t index = file.add_value(stack[level], inst - INST_VALUE_C, read_u16(bytecode + pc + 1*2));
                if (index == ~0u) return false;
            }
			pc += 4;
        } else {
            pc += 2;
        }
    }

    return true;
}

static FORCEINLINE void store_u8(char *dst, uint32_t value)
{
	dst[0] = (char)(value & 0xff);
}

static FORCEINLINE void store_u16(char *dst, uint32_t value, bool big_endian)
{
	if (big_endian) {
		dst[0] = (char)((value >> 8) & 0xff);
		dst[1] = (char)((value >> 0) & 0xff);
	} else {
		dst[0] = (char)((value >> 0) & 0xff);
		dst[1] = (char)((value >> 8) & 0xff);
	}
}

static FORCEINLINE void store_u32(char *dst, uint32_t value, bool big_endian)
{
	if (big_endian) {
		dst[0] = (char)((value >> 24) & 0xff);
		dst[1] = (char)((value >> 16) & 0xff);
		dst[2] = (char)((value >> 8) & 0xff);
		dst[3] = (char)((value >> 0) & 0xff);
	} else {
#if defined(ASAN)
		dst[0] = (char)((value >> 0) & 0xff);
		dst[1] = (char)((value >> 8) & 0xff);
		dst[2] = (char)((value >> 16) & 0xff);
		dst[3] = (char)((value >> 24) & 0xff);
#else
        *(uint32_t*)dst = value;
#endif
	}
}

static void store_u64(char *dst, uint64_t value, bool big_endian)
{
	if (big_endian) {
		dst[0] = (char)((value >> 56) & 0xff);
		dst[1] = (char)((value >> 48) & 0xff);
		dst[2] = (char)((value >> 40) & 0xff);
		dst[3] = (char)((value >> 32) & 0xff);
		dst[4] = (char)((value >> 24) & 0xff);
		dst[5] = (char)((value >> 16) & 0xff);
		dst[6] = (char)((value >> 8) & 0xff);
		dst[7] = (char)((value >> 0) & 0xff);
	} else {
#if defined(ASAN)
		dst[0] = (char)((value >> 0) & 0xff);
		dst[1] = (char)((value >> 8) & 0xff);
		dst[2] = (char)((value >> 16) & 0xff);
		dst[3] = (char)((value >> 24) & 0xff);
		dst[4] = (char)((value >> 32) & 0xff);
		dst[5] = (char)((value >> 40) & 0xff);
		dst[6] = (char)((value >> 48) & 0xff);
		dst[7] = (char)((value >> 56) & 0xff);
#else
        *(uint64_t*)dst = value;
#endif
	}
}

struct Stream {
    char *start, *dst, *end;
    bool failed = false;
    bool big_endian = false;

    void indent(uint32_t count)
    {
        for (uint32_t i = 0; i < count; i++) {
            if (dst == end) failed = true;
            if (failed) return;
            *dst++ = ' ';
        }
    }

    void write(const char *str)
    {
        if (failed) return;
        size_t length = strlen(str);
        if (length >= (size_t)(end - dst)) {
            failed = true;
            return;
        }
        memcpy(dst, str, length);
        dst += length;
    }

    void write(const char *str, size_t length)
    {
        if (failed) return;
        if (length >= (size_t)(end - dst)) {
            failed = true;
            return;
        }
        memcpy(dst, str, length);
        dst += length;
    }

    void format(const char *fmt, ...)
    {
        if (failed) return;

        va_list args;
        va_start(args, fmt);
        int written = vsnprintf(dst, end - dst, fmt, args);
        va_end(args);
        if (written < 0 || written >= end - dst) failed = true;
        if (!failed) {
			dst += written;
        }
    }

    FORCEINLINE void write_u8(uint32_t value)
    {
        if (end - dst < 1) failed = true;
        if (failed) return;
        store_u8(dst, value);
        dst += 1;
    }

    FORCEINLINE void write_u16(uint32_t value)
    {
        if (end - dst < 2) failed = true;
        if (failed) return;
        store_u16(dst, value, big_endian);
        dst += 2;
    }

    FORCEINLINE void write_u32(uint32_t value)
    {
        if (end - dst < 4) failed = true;
        if (failed) return;
        store_u32(dst, value, big_endian);
        dst += 4;
    }

    FORCEINLINE void write_u64(uint64_t value)
    {
        if (end - dst < 8) failed = true;
        if (failed) return;
        store_u64(dst, value, big_endian);
        dst += 8;
    }

    FORCEINLINE void write_f32(float value)
    {
        uint32_t bits = 0;
        memcpy(&bits, &value, 4);
        write_u32(bits);
    }

    FORCEINLINE void write_f64(double value)
    {
        uint64_t bits = 0;
        memcpy(&bits, &value, 8);
        write_u64(bits);
    }

    FORCEINLINE char *reserve_patch(uint32_t size)
    {
        if (end - dst < size) failed = true;
        if (failed) return NULL;
        char *ptr = dst;
        dst += size;
        return ptr;
    }
};

static int64_t decode_word(uint32_t index)
{
    return (int64_t)(int16_t)(uint16_t)index;
}

static int64_t decode_int(uint32_t index)
{
    uint32_t sign = index >> 15;
    uint32_t exp = (index >> 13) & 0x3;
    int64_t mantissa = index & 0x1fff;
    int64_t value = 0;
    if (exp == 0) {
        value = mantissa;
    } else if (exp == 1) {
        value = 0x2000 + mantissa * 0x3;
	} else if (exp == 2) {
        value = 0x8000 + mantissa * 0x3fc;
	} else {
        value = 0x800000 + mantissa * 0x3fc00;
	}
    return sign ? -value : value;
}

static int64_t decode_long(uint32_t index)
{
    uint32_t sign = index >> 15;
    uint32_t exp = (index >> 13) & 0x3;
    int64_t mantissa = index & 0x1fff;
    int64_t value = 0;
    if (exp == 0) {
        value = mantissa;
    } else if (exp == 1) {
        value = 0x2000 + mantissa * 0x3;
	} else if (exp == 2) {
        value = 0x8000 + mantissa * 0x40000;
	} else {
        value = INT64_C(0x80000000) + mantissa * INT64_C(0x4000000000000);
	}
    return sign ? -value : value;
}

static double decode_float(uint32_t index)
{
    uint32_t sign = index >> 15;
    int32_t exp = ((int32_t)((index >> 8) & 0x7f) - 64);
    if (exp == -64) return sign ? -0.0 : 0.0;
    if (exp == 63) return (index & 0xff) ? (sign ? -INFINITY : INFINITY) : NAN;
    double mantissa = (double(index & 0xff) / 256.0) * 0.5 + 0.5;
    double value = ldexp(mantissa, exp);
    return sign ? -value : value;
}

static int64_t decode_integer(uint32_t type, uint32_t index)
{
    if (type == TYPE_C) {
        return index & 0xff;
    } else if (type == TYPE_Y) {
        return decode_word(index);
    } else if (type == TYPE_I) {
        return decode_int(index);
    } else if (type == TYPE_L) {
        return decode_long(index);
    } else if (type == TYPE_F) {
        return (int64_t)decode_float(index);
    } else if (type == TYPE_D) {
        return (int64_t)decode_float(index);
    } else {
        return 0;
    }
}

struct rng_state {
    uint64_t x[2];
};

uint64_t rng_u64(rng_state &state)
{
	uint64_t t = state.x[0];
	uint64_t const s = state.x[1];
	state.x[0] = s;
	t ^= t << 23;
	t ^= t >> 18;
	t ^= s ^ (s >> 5);
	state.x[1] = t;
	return t + s;
}

void rng_warmup(rng_state &state)
{
    rng_u64(state);
    rng_u64(state);
    rng_u64(state);
}

int64_t rng_i64_range(rng_state &state, int64_t min_v, int64_t max_v, uint64_t rotate)
{
    if (min_v >= max_v) return min_v;
    uint64_t range = (uint64_t)(max_v - min_v + 1);
    return (int64_t)(min_v + (rng_u64(state) + rotate) % range);
}

double rng_f64(rng_state &state)
{
    return (double)rng_u64(state) * 5.421010862427522e-20;
}

double rng_f64_range(rng_state &state, double min_v, double max_v)
{
    double t = rng_f64(state);
    return min_v * (1.0 - t) + max_v * t;
}

struct NumberCache {
    static constexpr uint32_t size = 0x10000;

    int64_t int_value[size];
    char int_str[size][64];

    uint64_t float_bits[size];
    char float_str[size][64];

    static uint32_t hash_u64(uint64_t value)
    {
        uint64_t x = value;
		x ^= x >> 32;
		x *= 0xd6e8feb86659fd93U;
		x ^= x >> 32;
		x *= 0xd6e8feb86659fd93U;
		x ^= x >> 32;
		return (uint32_t)x;
    }

    void init()
    {
        {
			uint32_t index = hash_u64(0) & (size - 1);
			char *dst = int_str[index];
			snprintf(dst, 64, "%lld", (long long)0);
        }

        {
			uint32_t index = hash_u64(0) & (size - 1);
			char *dst = float_str[index];
			snprintf(dst, 64, "%g", 0.0);
        }
    }

    char *format_int(int64_t value)
    {
        uint32_t index = hash_u64((uint64_t)value) & (size - 1);
        char *dst = int_str[index];
        if (int_value[index] != value) {
            snprintf(dst, 64, "%lld", (long long)value);
            int_value[index] = value;
        }
        return dst;
    }

    char *format_float(double value)
    {
        uint64_t bits = 0;
        memcpy(&bits, &value, sizeof(double));
        uint32_t index = hash_u64(bits) & (size - 1);
        char *dst = float_str[index];
        if (float_bits[index] != bits) {
            snprintf(dst, 64, "%g", value);
            float_bits[index] = bits;
        }
        return dst;
    }
};

static const char *get_string(string_id id)
{
	if (id & 0x8000) {
		return unknown_string_table[(id & 0x7fff) % UNKNOWN_STRING_TABLE_SIZE];
	} else {
		return string_table[id % STRING_TABLE_SIZE];
	}
}

static double min(double a, double b) { return a < b ? a : b; }
static double max(double a, double b) { return a < b ? b : a; }

static void generate_int_array(int64_t *dst, int64_t min_v, int64_t max_v, uint32_t size, uint32_t hash, uint32_t pattern, uint32_t param)
{
    if (size == 0) return;

	rng_state rng = { (uint64_t)hash + 1, 1 };
	rng_warmup(rng);
    if (pattern == ARRAY_PATTERN_RANDOM) {
        for (uint32_t i = 0; i < size; i++) {
            dst[i] = rng_i64_range(rng, min_v, max_v, param);
        }
    } else if (pattern == ARRAY_PATTERN_ASCENDING) {
        int64_t t = min_v;
        int64_t step = (int64_t)((uint64_t)(max_v - min_v) / size);
        int64_t range = step * ((param & 0xf) / 8);
        dst[0] = min_v;
        for (uint32_t i = 1; i < size - 1; i++) {
            int64_t lo = i * step, hi = lo + range;
            dst[i] = rng_i64_range(rng, lo, hi, param);
        }
        dst[size - 1] = max_v;
    } else if (pattern == ARRAY_PATTERN_POLYGON) {
        uint32_t faces_left = param;
        uint32_t dst_i = 0;
        while (faces_left > 1 && size - dst_i > 3) {
            uint32_t indices_left = size - dst_i;
            double ratio = (double)indices_left / (double)faces_left;
            double r = rng_f64_range(rng, max(1.0, ratio * 0.75), max(1.0, min(ratio * 1.25, indices_left)));
            uint32_t face_size = (uint32_t)round(r);

            if ((size - (dst_i + face_size)) < 3) {
                face_size = size - dst_i;
            }

            for (uint32_t i = 0; i < face_size - 1; i++) {
                dst[dst_i + i] = rng_i64_range(rng, 0, max_v, 0);
            }
			dst[dst_i + face_size - 1] = rng_i64_range(rng, min_v, -1, 0);
            dst_i += face_size;

            faces_left -= 1;
        }

		for (uint32_t i = dst_i; i < size - 1; i++) {
			dst[i] = rng_i64_range(rng, 0, max_v, 0);
		}
		dst[size - 1] = rng_i64_range(rng, min_v, -1, 0);
    } else if (pattern == ARRAY_PATTERN_REFCOUNT) {
        uint32_t refs_left = param;

        for (uint32_t i = 0; i < size; i++) {
            uint32_t indices_left = size - i;

            if (refs_left > 0 && indices_left == 1) {
                dst[i] = refs_left;
            } else if (refs_left > indices_left) {
				double ratio = (double)refs_left / (double)indices_left;
				double r = rng_f64_range(rng, max(1.0, ratio * 0.5), min(ratio * 1.5, refs_left - indices_left));
                uint32_t refs = (uint32_t)round(r);
				dst[i] = refs;
                refs_left -= refs;
            } else {
                dst[i] = 1;
                if (refs_left > 0) {
					refs_left -= 1;
                }
            }
        }
    }
}

struct AsciiWriter {
    Stream stream;
    const File &file;
    NumberCache *number_cache;

    void write_string(string_id id)
    {
        stream.write(get_string(id));
    }

    void write_escaped_string(string_id id)
    {
        const char *str = get_string(id);

        stream.write("\"");
        while (*str) {
            char c = *str++;
            if (stream.dst == stream.end) stream.failed = true;
            if (stream.failed) return;
            if (c == '"') {
                stream.write("&amp;");
            } else {
                *stream.dst++ = c;
            }
        }
        stream.write("\"");
    }

    void write_field(uint32_t index, uint32_t indent)
    {
        const Field &field = file.fields[index];

        stream.indent(indent * 4);
		write_string(field.name);
        stream.write(": ");
        if (field.array_type != NO_ARRAY) {
			uint32_t size = field.array_size & 0x3fff;
            if (size >= 4096) size = 4096;
            uint32_t pattern = field.array_size >> 14;

            stream.format("*%u {\n", size);
            stream.indent((indent + 1) * 4);
            stream.write("a: ");

            rng_state rng = { (uint64_t)field.array_hash + 1, 1 };
			rng_warmup(rng);

            Value min_val = file.values[field.first_value];
            Value max_val = file.values[field.last_value];

            if (field.array_type == TYPE_F || field.array_type == TYPE_D) {
                double min_v = decode_float(min_val.value);
                double max_v = decode_float(max_val.value);
                if (min_v > max_v) {
                    std::swap(min_v, max_v);
                }
                for (uint32_t i = 0; i < size; i++) {
                    if (i != 0) stream.write(", ");
                    stream.write(number_cache->format_float(rng_f64_range(rng, min_v, max_v)));
                }
            } else {
                int64_t min_v = decode_integer(field.array_type, min_val.value);
                int64_t max_v = decode_integer(field.array_type, max_val.value);
                if (min_v > max_v) {
                    std::swap(min_v, max_v);
                }
                generate_int_array(array_buffer, min_v, max_v, size, field.array_hash, pattern, field.array_param);
                for (uint32_t i = 0; i < size; i++) {
                    if (i != 0) stream.write(", ");
                    stream.write(number_cache->format_int(array_buffer[i]));
                }
            }

            stream.write("\n");
			stream.indent(indent * 4);
            stream.write("}\n");
        } else {
            uint32_t value_ix = field.first_value;
            while (value_ix) {
                const Value &value = file.values[value_ix];
                if (value_ix != field.first_value) stream.write(", ");

                if (value.type == TYPE_S || value.type == TYPE_R) {
                    write_escaped_string(value.value);
                } else if (value.type == TYPE_F || value.type == TYPE_D) {
                    stream.write(number_cache->format_float(decode_float(value.value)));
                } else if (value.type == TYPE_C) {
                    stream.format("%c", (char)(value.value & 0x7f));
                } else {
                    stream.write(number_cache->format_int(decode_integer(value.type, value.value)));
                }

                value_ix = value.next_sibling;
            }

            if (field.first_child != 0) {
                stream.write(" {\n");
                uint32_t child = field.first_child;
                while (child) {
                    write_field(child, indent + 1);
                    child = file.fields[child].next_sibling;
                }
				stream.indent(indent * 4);
				stream.write("}\n");
            } else {
				stream.write("\n");
            }
        }

        if (indent == 0) {
            stream.write("\n");
        }
    }
};

struct BinaryWriter {
    Stream stream;
    const File &file;

    void write_field(uint32_t index, uint32_t indent)
    {
        const Field &field = file.fields[index];

        const char *name = get_string(field.name);
        size_t name_len = strlen(name);

        char *p_end_offset = NULL, *p_values_len = NULL;
        uint32_t num_values = field.array_type != NO_ARRAY ? 1 : field.num_values;

        if (file.version >= 7500) {
            p_end_offset = stream.reserve_patch(8);
            stream.write_u64(num_values);
            p_values_len = stream.reserve_patch(8);
        } else {
            p_end_offset = stream.reserve_patch(4);
            stream.write_u32(num_values);
            p_values_len = stream.reserve_patch(4);
        }
		stream.write_u8((uint8_t)name_len);

        stream.write(name, name_len);

        if (stream.failed) return;
        char *p_values_begin = stream.reserve_patch(0);

        if (field.array_type != NO_ARRAY) {
			uint32_t size = field.array_size & 0x3fff;
            if (size >= 7200) size = 7200;
            uint32_t pattern = field.array_size >> 14;

            stream.write_u8(binary_array_type[field.array_type]);
            stream.write_u32(size);
            stream.write_u32(0);
            stream.write_u32(binary_array_elem_size[field.array_type] * size);

            rng_state rng = { (uint64_t)field.array_hash + 1, 1 };
            rng_warmup(rng);

            Value min_val = file.values[field.first_value];
            Value max_val = file.values[field.last_value];

            uint32_t arr_type = field.array_type;
            if (arr_type == TYPE_F || arr_type == TYPE_D) {
                double min_v = decode_float(min_val.value);
                double max_v = decode_float(max_val.value);
                if (min_v > max_v) {
                    std::swap(min_v, max_v);
                }
                for (uint32_t i = 0; i < size; i++) {
                    double v = rng_f64_range(rng, min_v, max_v);
                    if (arr_type == TYPE_F) {
                        stream.write_f32((float)v);
                    } else {
                        stream.write_f64(v);
                    }
                }
            } else {
                int64_t min_v = decode_integer(field.array_type, min_val.value);
                int64_t max_v = decode_integer(field.array_type, max_val.value);
                if (min_v > max_v) {
                    std::swap(min_v, max_v);
                }
                generate_int_array(array_buffer, min_v, max_v, size, field.array_hash, pattern, field.array_param);
                for (uint32_t i = 0; i < size; i++) {
                    int64_t v = array_buffer[i];
                    if (arr_type == TYPE_C) {
						stream.write_u8(v & 0xff);
                    } else if (arr_type == TYPE_Y) {
						stream.write_u8(v & 0xff);
                    } else if (arr_type == TYPE_I) {
						stream.write_u32((uint32_t)(int32_t)v);
                    } else if (arr_type == TYPE_L) {
						stream.write_u64((uint64_t)v);
                    }
                }
            }

        } else {
            uint32_t value_ix = field.first_value;
            while (value_ix) {
                const Value &value = file.values[value_ix];

                if (value.type == TYPE_S || value.type == TYPE_R) {
                    const char *str = get_string(value.value & (string_id)~0x4000);
                    size_t len = strlen(str);
                    stream.write_u8(value.type == TYPE_S ? 'S' : 'R');
                    stream.write_u32((uint32_t)len);

                    if (value.value & 0x4000) {
                        size_t split_pos = SIZE_MAX;
                        for (size_t i = 0; i + 2 <= len; i++) {
                            if (str[i] == ':' && str[i + 1] == ':') {
                                split_pos = i;
                            }
                        }

                        if (split_pos != SIZE_MAX) {
							stream.write(str + split_pos + 2, len - (split_pos + 2));
							stream.write("\x00\x01", 2);
							stream.write(str, split_pos);
                        } else {
							stream.write(str, len);
                        }
                    } else {
						stream.write(str, len);
                    }
                } else if (value.type == TYPE_C) {
                    stream.write_u8('C');
                    stream.write_u8(value.value & 0xff);
                } else if (value.type == TYPE_Y) {
                    stream.write_u8('Y');
                    stream.write_u16((uint16_t)decode_word(value.value));
                } else if (value.type == TYPE_I) {
                    stream.write_u8('I');
                    stream.write_u32((uint32_t)decode_int(value.value));
                } else if (value.type == TYPE_L) {
                    stream.write_u8('L');
                    stream.write_u64((uint64_t)decode_long(value.value));
                } else if (value.type == TYPE_F) {
                    double v = decode_float(value.value);
                    stream.write_u8('F');
                    stream.write_f32((float)v);
                } else if (value.type == TYPE_D) {
                    double v = decode_float(value.value);
                    stream.write_u8('D');
                    stream.write_f64(v);
                }

                value_ix = value.next_sibling;
            }

        }

		if (file.version >= 7500) {
            store_u64(p_values_len, (uint64_t)(stream.dst - p_values_begin), stream.big_endian);
		} else {
            store_u32(p_values_len, (uint32_t)(stream.dst - p_values_begin), stream.big_endian);
        }


		if (field.first_child != 0) {
			uint32_t child = field.first_child;
			while (child) {
				write_field(child, indent + 1);
				child = file.fields[child].next_sibling;
			}

			stream.write(zero_buffer, file.version >= 7500 ? 25 : 13);
		}

		if (file.version >= 7500) {
            store_u64(p_end_offset, (uint64_t)(stream.dst - stream.start), stream.big_endian);
		} else {
            store_u32(p_end_offset, (uint32_t)(stream.dst - stream.start), stream.big_endian);
        }
    }
};

static size_t write_ascii(char *dst, size_t dst_size, const File &file, NumberCache *number_cache)
{
    AsciiWriter w = { { dst, dst, dst + dst_size }, file, number_cache };
    w.stream.format("; FBX %u.%u.0 project file\n", file.version / 1000 % 10, file.version / 100 % 10);
    w.stream.write("; ----------------------------------------------------\n\n");

    const Field &top = file.fields[0];
	uint32_t child = top.first_child;
	while (child) {
		w.write_field(child, 0);
		child = file.fields[child].next_sibling;
	}

    if (!w.stream.failed && w.stream.dst != w.stream.end) {
        *w.stream.dst = '\0';
        return w.stream.dst - w.stream.start;
    } else {
        w.stream.failed = true;
        return 0;
    }
}

static size_t write_binary(char *dst, size_t dst_size, const File &file)
{
    BinaryWriter w = { { dst, dst, dst + dst_size }, file };
    w.stream.big_endian = (file.flags & 0x01000000) != 0;
    w.stream.write("Kaydara FBX Binary  \x00\x1a", 22);
    w.stream.write_u8(w.stream.big_endian ? 1 : 0);
    w.stream.write_u32(file.version);

    const Field &top = file.fields[0];
	uint32_t child = top.first_child;
	while (child) {
		w.write_field(child, 0);
		child = file.fields[child].next_sibling;
	}
    w.stream.write(zero_buffer, file.version >= 7500 ? 4*25 : 4*13);

    if (!w.stream.failed && w.stream.dst != w.stream.end) {
        return w.stream.dst - w.stream.start;
    } else {
        w.stream.failed = true;
        return 0;
    }
}

}

#endif
