#ifndef UFBX_UFBX_LIBC_C_INCLUDED
#define UFBX_UFBX_LIBC_C_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef ufbx_libc_abi
    #if defined(UFBX_STATIC)
        #define ufbx_libc_abi static
    #else
        #define ufbx_libc_abi
    #endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif

ufbx_libc_abi size_t ufbx_strlen(const char *str)
{
	size_t length = 0;
	while (str[length]) {
		length++;
	}
	return length;
}

ufbx_libc_abi void *ufbx_memcpy(void *dst, const void *src, size_t count)
{
	char *d = (char*)dst;
	const char *s = (const char*)src;
	for (size_t i = 0; i < count; i++) {
		d[i] = s[i];
	}
	return dst;
}

ufbx_libc_abi void *ufbx_memmove(void *dst, const void *src, size_t count)
{
	char *d = (char*)dst;
	const char *s = (const char*)src;
	if ((uintptr_t)d < (uintptr_t)s) {
		for (size_t i = 0; i < count; i++) {
			d[i] = s[i];
		}
	} else {
		for (size_t i = count; i-- > 0; ) {
			d[i] = s[i];
		}
	}
	return dst;
}

ufbx_libc_abi void *ufbx_memset(void *dst, int ch, size_t count)
{
	char *d = (char*)dst;
	char c = (char)ch;
	for (size_t i = 0; i < count; i++) {
		d[i] = c;
	}
	return dst;
}

ufbx_libc_abi const void *ufbx_memchr(const void *ptr, int value, size_t num)
{
	const char *p = (const char*)ptr;
	char c = (char)value;
	for (size_t i = 0; i < num; i++) {
		if (p[i] == c) {
			return p + i;
		}
	}
	return NULL;
}

ufbx_libc_abi int ufbx_memcmp(const void *a, const void *b, size_t count)
{
	const char *pa = (const char*)a;
	const char *pb = (const char*)b;
	for (size_t i = 0; i < count; i++) {
		if (pa[i] != pb[i]) {
			return (unsigned char)pa[i] < (unsigned char)pb[i] ? -1 : 1;
		}
	}
	return 0;
}

ufbx_libc_abi int ufbx_strcmp(const char *a, const char *b)
{
	const char *pa = (const char*)a;
	const char *pb = (const char*)b;
	for (size_t i = 0; ; i++) {
		if (pa[i] != pb[i]) {
			return (unsigned char)pa[i] < (unsigned char)pb[i] ? -1 : 1;
		} else if (pa[i] == 0) {
			return 0;
		}
	}
}

ufbx_libc_abi int ufbx_strncmp(const char *a, const char *b, size_t count)
{
	const char *pa = (const char*)a;
	const char *pb = (const char*)b;
	for (size_t i = 0; i < count; i++) {
		if (pa[i] != pb[i]) {
			return (unsigned char)pa[i] < (unsigned char)pb[i] ? -1 : 1;
		} else if (pa[i] == 0) {
			return 0;
		}
	}
	return 0;
}

#if defined(__cplusplus)
}
#endif

#endif

