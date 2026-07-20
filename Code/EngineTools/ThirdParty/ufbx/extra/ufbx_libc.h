#ifndef UFBX_UFBX_LIBC_H_INCLUDED
#define UFBX_UFBX_LIBC_H_INCLUDED

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

ufbx_libc_abi size_t ufbx_strlen(const char *str);
ufbx_libc_abi void *ufbx_memcpy(void *dst, const void *src, size_t count);
ufbx_libc_abi void *ufbx_memmove(void *dst, const void *src, size_t count);
ufbx_libc_abi void *ufbx_memset(void *dst, int ch, size_t count);
ufbx_libc_abi const void *ufbx_memchr(const void *ptr, int value, size_t count);
ufbx_libc_abi int ufbx_memcmp(const void *a, const void *b, size_t count);
ufbx_libc_abi int ufbx_strcmp(const char *a, const char *b);
ufbx_libc_abi int ufbx_strncmp(const char *a, const char *b, size_t count);

#if defined(__cplusplus)
}
#endif

#endif
