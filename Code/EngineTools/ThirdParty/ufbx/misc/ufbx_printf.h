#ifndef UFBX_LIBC_H
#define UFBX_LIBC_H

#include <stddef.h>
#include <stdarg.h>

#if defined(__cplusplus)
extern "C" {
#endif

int ufbx_vsnprintf(char *buffer, size_t count, const char *format, va_list args);
int ufbx_snprintf(char *buffer, size_t count, const char *format, ...);

#if defined(__cplusplus)
}
#endif

#endif

