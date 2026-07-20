#ifndef UFBX_MALLOC_H_INCLUDED
#define UFBX_MALLOC_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

// -- API used by ufbx

void *ufbx_malloc(size_t size);
void *ufbx_realloc(void *ptr, size_t old_size, size_t new_size);
void ufbx_free(void *ptr, size_t old_size);

// -- External functions that must be implemented by the user

// Minimal implementation of this API could be:
//
// char memory[16*1024*1024];
// bool allocated = false;
// void *ufbx_libc_allocate(size_t size, size_t *p_allocated_size) {
//     if (allocated) return false;
//     allocated = true;
//     *p_allocated_size = sizeof(memory);
//     return memory;
// }
// bool ufbx_libc_free() {
//     return false;
// }
//

// Allocate memory from the OS, you can return more memory than requested via `p_allocated_size`.
void *ufbx_malloc_os_allocate(size_t size, size_t *p_allocated_size);

// Free memory returned by `ufbxc_os_allocate()`.
// Return `true` if the memory was freed. If you return `false` ufbxc will re-use the memory.
bool ufbx_malloc_os_free(void *pointer, size_t allocated_size);

// Optional: Required for thread safety
// You can define these to no-op if you don't care.
void ufbx_malloc_os_lock();
void ufbx_malloc_os_unlock();

#if defined(__cplusplus)
}
#endif

#endif
