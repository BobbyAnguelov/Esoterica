void ufbxt_assert_fail(const char *message);
#define ufbxt_assert(cond) do { if (!(cond)) ufbxt_assert_fail(#cond); } while (0)
#define ufbxh_assert(cond) ufbxt_assert(cond)
#define ufbx_assert(cond) ufbxt_assert(cond)

#define UFBX_NO_LIBC
#define UFBX_DEV
#define UFBX_NO_ASSERT
#include "../../ufbx.h"

#include "../../extra/ufbx_libc.h"
#include "../../extra/ufbx_math.h"
#include "../ufbx_malloc.h"
#include "../ufbx_printf.h"

#define isnan(v) ufbx_isnan(v)
#define vsnprintf ufbx_vsnprintf

#define strlen ufbx_strlen
#define memcpy ufbx_memcpy
#define memmove ufbx_memmove
#define memset ufbx_memset
#define memchr ufbx_memchr
#define memcmp ufbx_memcmp
#define strcmp ufbx_strcmp
#define strncmp ufbx_strncmp

#if defined(__wasm__)
	#define wasm_import(name) __attribute__((import_module("host"), import_name(name)))
	#define wasm_export(name) __attribute__((export_name(name)))
#else
	#define wasm_import(name)
	#define wasm_export(name)
#endif

int ufbxh_snprintf(char *buffer, size_t size, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = vsnprintf(buffer, size, fmt, args);
	va_end(args);
	return ret;
}

typedef struct {
	int unused;
} ufbxh_FILE;

int ufbxh_fflush(ufbxh_FILE *f);
int ufbxh_fprintf(ufbxh_FILE *f, const char *fmt, ...);

#include "../../test/check_scene.h"
#include "../../test/hash_scene.h"

wasm_import("hostError") void host_error(const char *message, size_t length);
wasm_import("hostVerbose") void host_verbose(const char *message, size_t length);
wasm_import("hostExit") void host_exit(int code);

wasm_import("hostOpenFile") int host_open_file(size_t index, const char *filename, size_t length);
wasm_import("hostReadFile") void host_read_file(size_t index, void *dst, size_t offset, size_t count);
wasm_import("hostCloseFile") void host_close_file(size_t index);

wasm_import("hostDump") void hostDump(const char *data, size_t length);

void verbosef(const char *fmt, ...)
{
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	int ret = vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (ret > 0) {
		host_verbose(buffer, (size_t)ret);
	}
}

void errorf(const char *fmt, ...)
{
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	int ret = vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (ret > 0) {
		host_error(buffer, (size_t)ret);
	}
}

char g_memory[32*1024*1024];

void ufbx_malloc_os_lock() { }
void ufbx_malloc_os_unlock() { }

void *ufbx_malloc_os_allocate(size_t size, size_t *p_allocated_size)
{
	static bool allocated = false;
	if (allocated) return NULL;
	allocated = true;

	*p_allocated_size = sizeof(g_memory);
	return g_memory;
}

bool ufbx_malloc_os_free(void *pointer, size_t allocated_size)
{
	return false;
}

typedef struct {
	bool used;
	size_t size;
	size_t offset;
	size_t index;
} wasm_file;

#define MAX_OPEN_FILES 64
wasm_file open_files[MAX_OPEN_FILES];

void *ufbx_stdio_open(const char *path, size_t path_len)
{
	size_t index = SIZE_MAX;
	for (size_t i = 0; i < MAX_OPEN_FILES; i++) {
		if (!open_files[i].used) {
			index = i;
			break;
		}
	}
	if (index == SIZE_MAX) return NULL;

	int size = host_open_file(index, path, path_len);
	if (size < 0) return NULL;

	verbosef("Opened file: %s (%d bytes)", path, size);

	wasm_file *file = &open_files[index];
	file->used = true;
	file->index = index;
	file->size = (size_t)size;
	file->offset = 0;
	return file;
}

size_t ufbx_stdio_read(void *file, void *data, size_t size)
{
	wasm_file *f = (wasm_file*)file;

	size_t max_read = f->size - f->offset;
	size_t to_read = size < max_read ? size : max_read;
	host_read_file(f->index, data, f->offset, to_read);
	f->offset += to_read;
	return to_read;
}

bool ufbx_stdio_skip(void *file, size_t size)
{
	wasm_file *f = (wasm_file*)file;
	if (f->size - f->offset < size) return false;
	f->offset += size;
	return true;
}

uint64_t ufbx_stdio_size(void *file)
{
	wasm_file *f = (wasm_file*)file;
	return f->size;
}

void ufbx_stdio_close(void *file)
{
	wasm_file *f = (wasm_file*)file;
	memset(f, 0, sizeof(wasm_file));
}

void ufbxt_assert_fail(const char *message)
{
	errorf("ufbxc_assert() fail: %s\n", message);
	host_exit(1);
}

static char dump_buffer[4096];
size_t dump_offset = 0;

int ufbxh_fflush(ufbxh_FILE *f)
{
	ufbxh_assert((ptrdiff_t)f == 0xDF);

	hostDump((char*)dump_buffer, dump_offset);

	dump_offset = 0;
	return 0;
}

int ufbxh_fprintf(ufbxh_FILE *f, const char *fmt, ...)
{
	ufbxh_assert((ptrdiff_t)f == 0xDF);

	if (dump_offset >= sizeof(dump_buffer) / 2) {
		ufbxh_fflush(f);
	}

	va_list args;
	va_start(args, fmt);
	int ret = vsnprintf(dump_buffer + dump_offset, sizeof(dump_buffer) - dump_offset, fmt, args);
	va_end(args);

	ufbxh_assert(dump_offset + (size_t)ret < sizeof(dump_buffer));
	dump_offset += (size_t)ret;

	return 0;
}

wasm_export("hashAlloc")
void *hash_alloc(size_t size)
{
	return ufbx_malloc(size);
}

wasm_export("hashFree")
void hash_free(void *pointer)
{
	ufbx_free(pointer, SIZE_MAX);
}

wasm_export("hashScene")
int hash_scene(uint64_t *p_hash, const void *data, size_t size, const char *filename, int frame, ufbxh_FILE *dump_file)
{
	ufbx_load_opts opts = { 0 };
	opts.load_external_files = true;
	opts.ignore_missing_external_files = true;
	opts.evaluate_caches = true;
	opts.evaluate_skinning = true;
	opts.target_axes = ufbx_axes_right_handed_y_up;
	opts.target_unit_meters = 1.0f;
	opts.filename.data = filename;
	opts.filename.length = strlen(filename);

	verbosef("Loading scene... %s (%zu bytes)", filename, size);

	ufbx_error error;
	ufbx_scene *scene = ufbx_load_memory(data, size, &opts, &error);
	if (!scene) {
		char err[512];
		verbosef("Failed to load the scene: %u", error.type);
		ufbx_format_error(err, sizeof(err), &error);
		errorf("%s\n", err);
		return 1;
	}

	verbosef("Checking scene...");
	ufbxt_check_scene(scene);
	verbosef("Check OK!");

	if (frame > 0) {
		ufbx_evaluate_opts eval_opts = { 0 };
		eval_opts.evaluate_caches = true;
		eval_opts.evaluate_skinning = true;
		eval_opts.load_external_files = true;

		double time = scene->anim->time_begin + frame / scene->settings.frames_per_second;
		ufbx_scene *state = ufbx_evaluate_scene(scene, NULL, time, NULL, &error);
		if (!state) {
			errorf("Failed to evaluate scene: %s\n", error.description.data);
			return 2;
		}

		ufbxt_check_scene(state);
		ufbx_free_scene(scene);
		scene = state;
	}

	verbosef("Hashing scene...");
	uint64_t hash = ufbxt_hash_scene(scene, dump_file);

	if (dump_file) {
		ufbxh_fflush(dump_file);
	}

	ufbx_free_scene(scene);

	*p_hash = hash;

	return 0;
}

#undef strlen
#undef memcpy
#undef memmove
#undef memset
#undef memchr
#undef memcmp
#undef strcmp
#undef strncmp

#if defined(WASM64_MULTI3_WORKAROUND)
__int128 __multi3(__int128 a, __int128 b) {
	__int128 r = 0;
	for (size_t i = 0; i < 128; i++) {
		if (a & 0x1) r += b;
		a >>= 1;
		b <<= 1;
	}
	return r;
}
#endif

#include "../../ufbx.c"
#include "../../extra/ufbx_math.c"
#include "../../extra/ufbx_libc.c"
#include "../ufbx_printf.c"
#include "../ufbx_malloc.c"
