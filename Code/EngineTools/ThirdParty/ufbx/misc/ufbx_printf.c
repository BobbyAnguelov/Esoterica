#ifndef UFBX_PRINTF_C
#define UFBX_PRINTF_C

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

// -- print

typedef struct {
	char *dst;
	size_t length;
	size_t pos;
} print_buffer;

// Flags
#define PRINT_ALIGN_LEFT 0x1
#define PRINT_PAD_ZERO 0x2
#define PRINT_SIGN_PLUS 0x4
#define PRINT_SIGN_SPACE 0x8
#define PRINT_ALT 0x10
#define PRINT_64BIT 0x20
// Formatting flags
#define PRINT_SIGNED 0x100
#define PRINT_UPPERCASE 0x200
// Type
#define PRINT_INT 0x100
#define PRINT_CHAR 0x200
#define PRINT_STRING 0x400
// Radix
#define PRINT_RADIX_BIN 0x020000
#define PRINT_RADIX_OCT 0x080000
#define PRINT_RADIX_DEC 0x0a0000
#define PRINT_RADIX_HEX 0x100000

#define print_radix(flags) ((flags) >> 16)

static void print_pad(print_buffer *buf, uint32_t flags, size_t count)
{
	char pad_char = (flags & (PRINT_ALIGN_LEFT|PRINT_PAD_ZERO)) == PRINT_PAD_ZERO ? '0' : ' ';
	for (size_t i = 0; i < count; i++) {
		if (buf->dst && buf->pos < buf->length) buf->dst[buf->pos] = pad_char;
		buf->pos++;
	}
}

static void print_append(print_buffer *buf, size_t min_width, size_t max_width, uint32_t flags, const char *str)
{
	size_t width = 0;
	for (width = 0; width < max_width; width++) {
		if (!str[width]) break;
	}

	size_t pad = min_width > width ? min_width - width : 0;
	if (pad > 0 && (flags & PRINT_ALIGN_LEFT) == 0) {
		print_pad(buf, flags, pad);
	}

	for (size_t i = 0; i < width; i++) {
		if (buf->dst && buf->pos < buf->length) buf->dst[buf->pos] = str[i];
		buf->pos++;
	}

	if (pad > 0 && (flags & PRINT_ALIGN_LEFT) != 0) {
		print_pad(buf, flags, pad);
	}
}

static uint32_t print_fmt_flags(const char **p_fmt)
{
	const char *fmt = *p_fmt;
	uint32_t flags = 0;
	for (;;) {
		char c = *fmt;
		switch (c) {
		case '-': flags |= PRINT_ALIGN_LEFT; break;
		case '+': flags |= PRINT_SIGN_PLUS; break;
		case ' ': flags |= PRINT_SIGN_SPACE; break;
		case '0': flags |= PRINT_PAD_ZERO; break;
		case '#': flags |= PRINT_ALT; break;
		default:
			*p_fmt = fmt;
			return flags;
		}
		fmt++;
	}
	*p_fmt = fmt;
}

static size_t print_fmt_count(const char **p_fmt, size_t def, int count_arg)
{
	const char *fmt = *p_fmt;
	int count = -1;
	if (*fmt >= '0' && *fmt <= '9') {
		count = 0;
		while (*fmt >= '0' && *fmt <= '9') {
			count = count * 10 + (*fmt - '0');
			fmt++;
		}
	} else if (*fmt == '*') {
		fmt++;
		count = count_arg;
	}
	*p_fmt = fmt;
	return count >= 0 ? (size_t)count : def;
}

static uint32_t print_fmt_type(const char **p_fmt)
{
	const char *fmt = *p_fmt;
	size_t size = sizeof(int);
	switch (*fmt) {
	case 'l':
		size = sizeof(long);
		if (*++fmt == 'l') {
			size = sizeof(long long);
			fmt++;
		}
		break;
	case 'I':
		fmt++;
		size = sizeof(size_t);
		if (fmt[1] == '3' && fmt[2] == '2') {
			fmt += 2;
			size = sizeof(uint32_t);
		} else if (fmt[1] == '6' && fmt[2] == '4') {
			fmt += 2;
			size = sizeof(uint64_t);
		}
		break;
	case 'h': fmt++; if (*fmt == 'h') fmt++; break;
	case 'z': fmt++; size = sizeof(size_t); break;
	case 'j': fmt++; size = sizeof(intmax_t); break;
	case 't': fmt++; size = sizeof(ptrdiff_t); 
	}

	uint32_t flags = 0;
	switch (*fmt++) {
	case 'd': case 'i': flags = PRINT_INT | PRINT_SIGNED | PRINT_RADIX_DEC; break;
	case 'u': flags = PRINT_INT | PRINT_RADIX_DEC; break;
	case 'x': flags = PRINT_INT | PRINT_RADIX_HEX; break;
	case 'X': flags = PRINT_INT | PRINT_UPPERCASE | PRINT_RADIX_HEX; break;
	case 'o': flags = PRINT_INT | PRINT_RADIX_OCT; break;
	case 'b': flags = PRINT_INT | PRINT_RADIX_BIN; break;
	case 's': flags = PRINT_STRING; break;
	case 'c': flags = PRINT_CHAR; break;
	case 'p': flags = PRINT_INT | PRINT_RADIX_HEX; size = sizeof(void*); break;
	}

	if (size == 8) flags |= PRINT_64BIT;
	*p_fmt = fmt;

	return flags;
}

static char *print_format_int(char *buffer, uint32_t flags, uint64_t value)
{
	bool sign = false;
	if (flags & PRINT_64BIT) {
		if ((flags & PRINT_SIGNED) != 0 && (int64_t)value < 0) {
			value = -(int64_t)value;
			sign = true;
		}
	} else {
		if ((flags & PRINT_SIGNED) != 0 && (int32_t)value < 0) {
			value = -(int32_t)value;
			sign = true;
		}
	}

	const char *chars = (flags & PRINT_UPPERCASE) != 0 ? "0123456789ABCDEFX" : "0123456789abcdefx";
	uint32_t radix = print_radix(flags);
	*--buffer = '\0';
	do {
		uint64_t digit = (uint32_t)(value % radix);
		value = value / radix;
		*--buffer = chars[digit];
	} while (value > 0);

	if (radix == 16 && (flags & PRINT_ALT) != 0) {
		*--buffer = chars[16];
		*--buffer = '0';
	}

	if (sign) {
		*--buffer = '-';
	} else if (flags & (PRINT_SIGN_PLUS|PRINT_SIGN_SPACE)) {
		*--buffer = (flags & PRINT_SIGN_PLUS) != 0 ? '+' : ' ';
	}

	return buffer;
}

static void vprint(print_buffer *buf, const char *fmt, va_list args)
{
	char buffer[96];
	for (const char *p = fmt; *p;) {
		if (*p == '%' && *++p != '%') {
			uint32_t flags = print_fmt_flags(&p);
			size_t min_width = print_fmt_count(&p, 0, *p == '*' ? va_arg(args, int) : -1);
			size_t max_width = SIZE_MAX;
			if (*p == '.') {
				p++;
				max_width = print_fmt_count(&p, max_width, *p == '*' ? va_arg(args, int) : -1);
			}
			flags |= print_fmt_type(&p);

			if (flags & PRINT_STRING) {
				const char *str = va_arg(args, const char*);
				print_append(buf, min_width, max_width, flags, str);
			} else if (flags & PRINT_INT) {
				uint64_t value = (flags & PRINT_64BIT) != 0 ? va_arg(args, uint64_t) : va_arg(args, uint32_t);
				char *str = print_format_int(buffer + sizeof(buffer), flags, value);
				print_append(buf, min_width, max_width, flags, str);
			} else if (flags & PRINT_CHAR) {
				char ch = (char)va_arg(args, int);
				print_append(buf, min_width, max_width, 0, &ch);
			}
		} else {
			if (buf->dst && buf->pos < buf->length) buf->dst[buf->pos] = *p;
			buf->pos++;
			p++;
		}
	}
	if (buf->length && buf->dst) {
		size_t end = buf->pos <= buf->length - 1 ? buf->pos : buf->length - 1;
		buf->dst[end] = '\0';
	}
}

#if defined(__cplusplus)
extern "C" {
#endif

int ufbx_vsnprintf(char *buffer, size_t count, const char *format, va_list args)
{
	print_buffer buf = { buffer, count };
	vprint(&buf, format, args);
	return (int)buf.pos;
}

int ufbx_snprintf(char *buffer, size_t count, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int len = ufbx_vsnprintf(buffer, count, format, args);
	va_end(args);
	return len;
}

#if defined(__cplusplus)
}
#endif

#endif
