#ifndef UFBX_OS_H_INCLUDED
#define UFBX_OS_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stddef.h>

#if !defined(UFBX_VERSION)
	#error "ufbx.h" must be included before "ufbx_os.h"
#endif

#ifndef ufbx_os_abi
#define ufbx_os_abi
#endif

#ifndef UFBX_OS_DEFAULT_MAX_THREADS
#define UFBX_OS_DEFAULT_MAX_THREADS 8
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct ufbx_os_thread_pool_opts {
	uint32_t _begin_zero;

	size_t max_threads;

	uint32_t _end_zero;
} ufbx_os_thread_pool_opts;

typedef struct ufbx_os_thread_pool ufbx_os_thread_pool;

ufbx_os_abi ufbx_os_thread_pool *ufbx_os_create_thread_pool(const ufbx_os_thread_pool_opts *user_opts);
ufbx_os_abi void ufbx_os_free_thread_pool(ufbx_os_thread_pool *pool);

ufbx_os_abi void ufbx_os_init_ufbx_thread_pool(ufbx_thread_pool *dst, ufbx_os_thread_pool *pool);

typedef struct ufbx_os_thread_pool_task ufbx_os_thread_pool_task;

typedef void ufbx_os_thread_pool_task_fn(void *user, uint32_t index);

ufbx_os_abi uint64_t ufbx_os_thread_pool_run(ufbx_os_thread_pool *pool, ufbx_os_thread_pool_task_fn *fn, void *user, uint32_t count);
ufbx_os_abi bool ufbx_os_thread_pool_try_wait(ufbx_os_thread_pool *pool, uint64_t task_id);
ufbx_os_abi void ufbx_os_thread_pool_wait(ufbx_os_thread_pool *pool, uint64_t task_id);

#if defined(__cplusplus)
}
#endif

#endif

#if defined(UFBX_OS_IMPLEMENTATION)
#ifndef UFBX_OS_H_IMPLEMENTED
#define UFBX_OS_H_IMPLEMENTED

#define ufbxos_assert(cond) ufbx_assert(cond)

#include <stdlib.h>
#include <string.h>

static void ufbxos_thread_pool_entry(ufbx_os_thread_pool *pool);

#if defined(_WIN32)

#if !defined(WIN32_LEAN_AND_MEAN)
	#define UFBXOS_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(NOMINMAX)
	#define UFBXOS_NOMINMAX
	#define NOMINMAX
#endif

#include <Windows.h>
#include <intrin.h>

#if defined(UFBXOS_LEAN_AND_MEAN)
	#undef WIN32_LEAN_AND_MEAN
#endif
#if defined(UFBXOS_NOMINMAX)
	#undef NOOMINMAX
#endif

typedef volatile long ufbxos_atomic_u32;
static uint32_t ufbxos_atomic_u32_load_relaxed(ufbxos_atomic_u32 *ptr) { return *(uint32_t*)ptr; }
static uint32_t ufbxos_atomic_u32_load(ufbxos_atomic_u32 *ptr) { return (uint32_t)_InterlockedOr(ptr, 0); }
static void ufbxos_atomic_u32_store(ufbxos_atomic_u32 *ptr, uint32_t value) { _InterlockedExchange(ptr, (long)value); }
static uint32_t ufbxos_atomic_u32_inc(ufbxos_atomic_u32 *ptr) { return (uint32_t)_InterlockedIncrement(ptr) - 1; }
static bool ufbxos_atomic_u32_cas(ufbxos_atomic_u32 *ptr, uint32_t ref, uint32_t value) { return (uint32_t)_InterlockedCompareExchange(ptr, (long)value, (long)ref) == ref; }

typedef volatile LONG64 ufbxos_atomic_u64;
static uint64_t ufbxos_atomic_u64_load(ufbxos_atomic_u64 *ptr) { return (uint64_t)_InterlockedCompareExchange64(ptr, 0, 0); }
static void ufbxos_atomic_u64_store(ufbxos_atomic_u64 *ptr, uint64_t value) { InterlockedExchange64(ptr, (LONG64)value); }
static bool ufbxos_atomic_u64_cas(ufbxos_atomic_u64 *ptr, uint64_t *ref, uint64_t value)
{
	uint64_t prev = *ref;
	uint64_t next = (uint64_t)_InterlockedCompareExchange64(ptr, (LONG64)value, (LONG64)prev);
	*ref = next;
	return prev == next;
}

typedef HANDLE ufbxos_os_semaphore;

static void ufbxos_os_semaphore_init(ufbxos_os_semaphore *os_sema, uint32_t max_count)
{
	*os_sema = CreateSemaphoreA(NULL, 0, (LONG)max_count, NULL);
}

static void ufbxos_os_semaphore_free(ufbxos_os_semaphore *os_sema)
{
	if (*os_sema == NULL) return;
	CloseHandle(*os_sema);
}

static void ufbxos_os_semaphore_wait(ufbxos_os_semaphore *os_sema)
{
	WaitForSingleObject(*os_sema, INFINITE);
}

static void ufbxos_os_semaphore_signal(ufbxos_os_semaphore *os_sema, uint32_t count)
{
	ReleaseSemaphore(*os_sema, (LONG)count, NULL);
}

typedef HANDLE ufbxos_os_thread;

static DWORD WINAPI ufbxos_os_thread_entry(LPVOID user)
{
	ufbxos_thread_pool_entry((ufbx_os_thread_pool*)user);
	return 0;
}

static bool ufbxos_os_thread_start(ufbxos_os_thread *os_thread, ufbx_os_thread_pool *pool)
{
	*os_thread = CreateThread(NULL, 0, &ufbxos_os_thread_entry, pool, 0, NULL);
	return *os_thread != NULL;
}

static void ufbxos_os_thread_join(ufbxos_os_thread *os_thread)
{
	if (*os_thread == NULL) return;
	WaitForSingleObject(*os_thread, INFINITE);
	CloseHandle(*os_thread);
}

static size_t ufbxos_os_get_logical_cores(void)
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return (size_t)info.dwNumberOfProcessors;
}

static void ufbxos_os_yield(void)
{
	Sleep(0);
}

#else

#if defined(__GNUC__) || defined(__clang__)
typedef volatile uint32_t ufbxos_atomic_u32;
static uint32_t ufbxos_atomic_u32_load_relaxed(ufbxos_atomic_u32 *ptr) { uint32_t r; (void)ptr; __atomic_load(ptr, &r, __ATOMIC_RELAXED); return r; }
static uint32_t ufbxos_atomic_u32_load(ufbxos_atomic_u32 *ptr) { uint32_t r; (void)ptr; __atomic_load(ptr, &r, __ATOMIC_SEQ_CST); return r; }
static void ufbxos_atomic_u32_store(ufbxos_atomic_u32 *ptr, uint32_t value) { (void)ptr; __atomic_store(ptr, &value, __ATOMIC_SEQ_CST); }
static uint32_t ufbxos_atomic_u32_inc(ufbxos_atomic_u32 *ptr) { (void)ptr; return __atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST); }
static bool ufbxos_atomic_u32_cas(ufbxos_atomic_u32 *ptr, uint32_t ref, uint32_t value) { (void)ptr; (void)ref; return __atomic_compare_exchange(ptr, &ref, &value, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

#if defined(__i386__) || defined(__i386)

#if defined(__clang__)
	// No way to convince that the pointer is aligned, even with __builtin_assume_aligned()
	#pragma clang diagnostic push
	#if __has_warning("-Wsync-alignment")
		#pragma clang diagnostic ignored "-Wsync-alignment"
	#endif
#endif

typedef volatile uint64_t ufbxos_atomic_u64;
static bool ufbxos_atomic_u64_cas(ufbxos_atomic_u64 *ptr, uint64_t *ref, uint64_t value) {
	uint64_t prev = *ref;
	uint64_t next = __sync_val_compare_and_swap(ptr, *ref, value);
	*ref = next;
	return prev == next;
}
static uint64_t ufbxos_atomic_u64_load(ufbxos_atomic_u64 *ptr) {
	uint64_t r = 0;
	ufbxos_atomic_u64_cas(ptr, &r, 0);
	return r;
}
static void ufbxos_atomic_u64_store(ufbxos_atomic_u64 *ptr, uint64_t value) {
	uint64_t ref = ufbxos_atomic_u64_load(ptr);
	while (!ufbxos_atomic_u64_cas(ptr, &ref, value)) { }
}

#if defined(__clang__)
	#pragma clang diagnostic pop
#endif

#else
typedef volatile uint64_t ufbxos_atomic_u64;
static uint64_t ufbxos_atomic_u64_load(ufbxos_atomic_u64 *ptr) { uint64_t r; (void)ptr; __atomic_load(ptr, &r, __ATOMIC_SEQ_CST); return r; }
static void ufbxos_atomic_u64_store(ufbxos_atomic_u64 *ptr, uint64_t value) { (void)ptr; __atomic_store(ptr, &value, __ATOMIC_SEQ_CST); }
static bool ufbxos_atomic_u64_cas(ufbxos_atomic_u64 *ptr, uint64_t *ref, uint64_t value) { (void)ptr; (void)ref; return __atomic_compare_exchange(ptr, ref, &value, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }
#endif

#else
	#error "Unsupported compiler"
#endif

#if defined(__MACH__)

#include <dispatch/dispatch.h>

typedef dispatch_semaphore_t ufbxos_os_semaphore;

static void ufbxos_os_semaphore_init(ufbxos_os_semaphore *os_sema, uint32_t max_count)
{
	(void)max_count;
    *os_sema = dispatch_semaphore_create(0);
}

static void ufbxos_os_semaphore_free(ufbxos_os_semaphore *os_sema)
{
    dispatch_release(*os_sema);
}

static void ufbxos_os_semaphore_wait(ufbxos_os_semaphore *os_sema)
{
    dispatch_semaphore_wait(*os_sema, DISPATCH_TIME_FOREVER);
}

static void ufbxos_os_semaphore_signal(ufbxos_os_semaphore *os_sema, uint32_t count)
{
	for (uint32_t i = 0; i < count; i++) {
        dispatch_semaphore_signal(*os_sema);
	}
}

#else

#include <semaphore.h>

typedef sem_t ufbxos_os_semaphore;

static void ufbxos_os_semaphore_init(ufbxos_os_semaphore *os_sema, uint32_t max_count)
{
	(void)max_count;
	sem_init(os_sema, 0, 0);
}

static void ufbxos_os_semaphore_free(ufbxos_os_semaphore *os_sema)
{
	sem_destroy(os_sema);
}

static void ufbxos_os_semaphore_wait(ufbxos_os_semaphore *os_sema)
{
	sem_wait(os_sema);
}

static void ufbxos_os_semaphore_signal(ufbxos_os_semaphore *os_sema, uint32_t count)
{
	for (uint32_t i = 0; i < count; i++) {
		sem_post(os_sema);
	}
}

#endif

#include <pthread.h>

typedef pthread_t ufbxos_os_thread;

static void *ufbxos_os_thread_entry(void *user)
{
	ufbxos_thread_pool_entry((ufbx_os_thread_pool*)user);
	return NULL;
}

static bool ufbxos_os_thread_start(ufbxos_os_thread *os_thread, ufbx_os_thread_pool *pool)
{
	int res = pthread_create(os_thread, NULL, &ufbxos_os_thread_entry, pool);
	if (res == 0) return false;
	return true;
}

static void ufbxos_os_thread_join(ufbxos_os_thread *os_thread)
{
	pthread_join(*os_thread, NULL);
}

#include <sched.h>
#include <unistd.h>

static void ufbxos_os_yield(void)
{
	sched_yield();
}

static size_t ufbxos_os_get_logical_cores(void)
{
	return (size_t)sysconf(_SC_NPROCESSORS_ONLN);
}

#endif

#define UFBXOS_OS_WAIT_NOTIFY 0

#if UFBXOS_OS_WAIT_NOTIFY

#pragma comment(lib, "synchronization.lib")

static void ufbxos_os_atomic_wait32(ufbxos_atomic_u32 *ptr, uint32_t value)
{
	WaitOnAddress(ptr, &value, 4, INFINITE);
}

static void ufbxos_os_atomic_wait64(ufbxos_atomic_u64 *ptr, uint64_t value)
{
	WaitOnAddress(ptr, &value, 8, INFINITE);
}

static void ufbxos_os_atomic_notify32(ufbxos_atomic_u32 *ptr)
{
	WakeByAddressAll((PVOID)ptr);
}

static void ufbxos_os_atomic_notify64(ufbxos_atomic_u64 *ptr)
{
	WakeByAddressAll((PVOID)ptr);
}

#endif

// --

#define UFBXOS_WAIT_SEMA_MAX_WAITERS 0x7fff

typedef struct {
	// [0:15]  waiters
	// [15:30] sleepers
	// [30]    signaled
	// [32:64] revision
	ufbxos_atomic_u64 state;
	ufbxos_os_semaphore os_semaphore[2];
} ufbxos_wait_sema;

typedef struct {
	uint32_t waiters;
	uint32_t sleepers;
	bool signaled;
	uint32_t revision;
} ufbxos_wait_sema_state;

static ufbxos_wait_sema_state ufbxos_wait_sema_decode(uint64_t state)
{
	ufbxos_wait_sema_state s;
	uint32_t state_lo = (uint32_t)state;
	s.waiters = (state_lo >> 0) & 0x7fff;
	s.sleepers = (state_lo >> 15) & 0x7fff;
	s.signaled = ((state_lo >> 30) & 0x1) != 0;
	s.revision = (uint32_t)(state >> 32);
	return s;
}

static uint64_t ufbxos_wait_sema_encode(ufbxos_wait_sema_state s)
{
	return (uint64_t)(s.waiters | s.sleepers << 15 | (s.signaled ? 1u : 0u) << 30)
		| (uint64_t)s.revision << 32;
}

typedef struct {
	// [0:8]   sema_index
	// [8:32]  hash
	// [32:64] sema_revision
	ufbxos_atomic_u64 state;
} ufbxos_wait_entry;

typedef struct {
	uint32_t sema_index;
	uint32_t hash;
	uint32_t sema_revision;
} ufbxos_wait_entry_state;

static ufbxos_wait_entry_state ufbxos_wait_entry_decode(uint64_t state)
{
	ufbxos_wait_entry_state s;
	uint32_t state_lo = (uint32_t)state;
	s.sema_index = (state_lo >> 0) & 0xff;
	s.hash = (state_lo >> 8) & 0xffffff;
	s.sema_revision = (uint32_t)(state >> 32);
	return s;
}

static uint64_t ufbxos_wait_entry_encode(ufbxos_wait_entry_state s)
{
	return (uint64_t)(s.sema_index | s.hash << 8) | (uint64_t)s.sema_revision << 32;
}

typedef struct {
	// [0:16] refcount
	// [16:64] cycle
	ufbxos_atomic_u64 state;

	ufbxos_atomic_u64 next;

	ufbx_os_thread_pool_task_fn *fn;
	void *user;
	uint32_t count;
	ufbxos_atomic_u32 counter;

} ufbxos_task;

typedef struct {
	uint32_t refcount;
	uint64_t cycle;
} ufbxos_task_state;

static ufbxos_task_state ufbxos_task_decode(uint64_t state)
{
	ufbxos_task_state s;
	uint32_t state_lo = (uint32_t)state;
	s.refcount = (state_lo >> 0) & 0xffff;
	s.cycle = state >> 16;
	return s;
}

static uint64_t ufbxos_task_encode(ufbxos_task_state s)
{
	return (uint64_t)s.refcount | (s.cycle << 16);
}

#define UFBXOS_WAIT_SEMA_MAX_COUNT 64
#define UFBXOS_WAIT_SEMA_SCAN 16

#define UFBXOS_WAIT_MAP_SIZE 128
#define UFBXOS_WAIT_MAP_SCAN 2

#define UFBXOS_TASK_FREE_BIT ((uint64_t)1<<63)

struct ufbx_os_thread_pool {
	ufbxos_atomic_u32 wait_sema_lock;
	ufbxos_atomic_u32 wait_sema_count;
	ufbxos_wait_sema wait_semas[UFBXOS_WAIT_SEMA_MAX_COUNT];

	ufbxos_wait_entry wait_map[UFBXOS_WAIT_MAP_SIZE];

	uint32_t num_tasks;
	uint32_t task_cycle_shift;
	uint32_t task_index_mask;
	ufbxos_task *tasks;

	ufbxos_atomic_u64 task_alloc_head;
	ufbxos_atomic_u64 task_work_tail;
	ufbxos_atomic_u64 task_work_head;
	ufbxos_atomic_u32 task_init_count;

	uint32_t num_threads;
	ufbxos_os_thread *threads;
};

static uint32_t ufbxos_wait_sema_create(ufbx_os_thread_pool *pool)
{
	if (ufbxos_atomic_u32_cas(&pool->wait_sema_lock, 0, 1)) {
		uint32_t index = ufbxos_atomic_u32_load(&pool->wait_sema_count);
		if (index >= UFBXOS_WAIT_SEMA_MAX_COUNT) {
			ufbxos_atomic_u32_store(&pool->wait_sema_lock, 0);
			return 0;
		}

		ufbxos_wait_sema *sema = &pool->wait_semas[index];

		ufbxos_os_semaphore_init(&sema->os_semaphore[0], UFBXOS_WAIT_SEMA_MAX_WAITERS);
		ufbxos_os_semaphore_init(&sema->os_semaphore[1], UFBXOS_WAIT_SEMA_MAX_WAITERS);

		ufbxos_atomic_u32_cas(&pool->wait_sema_count, index, index + 1);
		ufbxos_atomic_u32_store(&pool->wait_sema_lock, 0);
		return index + 1;
	}

	return 0;
}

static uint32_t ufbxos_wait_sema_alloc(ufbx_os_thread_pool *pool, uint32_t hint, uint32_t *p_revision)
{
	uint32_t count = ufbxos_atomic_u32_load(&pool->wait_sema_count);
	uint32_t scan = count < UFBXOS_WAIT_SEMA_SCAN ? count : UFBXOS_WAIT_SEMA_SCAN;
	ufbxos_assert(count > 0);

	for (uint32_t attempt = 0; attempt < 4; attempt++) {
		if (attempt == 1 && ufbxos_atomic_u32_load_relaxed(&pool->wait_sema_count) < UFBXOS_WAIT_SEMA_MAX_COUNT) {
			ufbxos_wait_sema_create(pool);
		}

		uint32_t index = hint % count;
		for (uint32_t i = 0; i < scan; i++) {
			ufbxos_wait_sema *sema = &pool->wait_semas[index];
			uint64_t old_state = ufbxos_atomic_u64_load(&sema->state);
			for (;;) {
				ufbxos_wait_sema_state s = ufbxos_wait_sema_decode(old_state);

				if (attempt <= 0 && s.waiters > 0) break;
				if (attempt <= 1 && s.sleepers > 0) break;
				if (attempt <= 2 && s.signaled) break;
				if (s.waiters >= UFBXOS_WAIT_SEMA_MAX_WAITERS) break;
				s.waiters += 1;

				if (ufbxos_atomic_u64_cas(&sema->state, &old_state, ufbxos_wait_sema_encode(s))) {
					*p_revision = s.revision;
					return index + 1;
				}
			}

			if (++index >= count) index = 0;
		}
	}

	return 0;
}

static bool ufbxos_wait_sema_outdated(ufbx_os_thread_pool *pool, uint32_t index, uint32_t revision)
{
	ufbxos_wait_sema *sema = &pool->wait_semas[index - 1];
	uint64_t old_state = ufbxos_atomic_u64_load(&sema->state);
	ufbxos_wait_sema_state s = ufbxos_wait_sema_decode(old_state);
	return s.revision != revision;
}

static bool ufbxos_wait_sema_join(ufbx_os_thread_pool *pool, uint32_t index, uint32_t revision)
{
	ufbxos_wait_sema *sema = &pool->wait_semas[index - 1];
	uint64_t old_state = ufbxos_atomic_u64_load(&sema->state);
	for (;;) {
		ufbxos_wait_sema_state s = ufbxos_wait_sema_decode(old_state);

		if (s.revision != revision) return false;
		if (s.waiters >= UFBXOS_WAIT_SEMA_MAX_WAITERS) return false;
		s.waiters += 1;

		if (ufbxos_atomic_u64_cas(&sema->state, &old_state, ufbxos_wait_sema_encode(s))) {
			return true;
		}
	}
}

static void ufbxos_wait_sema_wait(ufbx_os_thread_pool *pool, uint32_t index, uint32_t revision)
{
	ufbxos_wait_sema *sema = &pool->wait_semas[index - 1];

	ufbxos_os_semaphore_wait(&sema->os_semaphore[revision & 1]);

	uint64_t old_state = ufbxos_atomic_u64_load(&sema->state);
	for (;;) {
		ufbxos_wait_sema_state s = ufbxos_wait_sema_decode(old_state);

		bool do_signal = false;
		s.sleepers -= 1;
		if (s.sleepers == 0 && s.signaled) {
			s.sleepers = s.waiters;
			s.waiters = 0;
			s.revision++;
			s.signaled = false;
			do_signal = true;
		}

		if (ufbxos_atomic_u64_cas(&sema->state, &old_state, ufbxos_wait_sema_encode(s))) {
			if (do_signal && s.sleepers > 0) {
				ufbxos_os_semaphore_signal(&sema->os_semaphore[(s.revision - 1) & 1], s.sleepers);
			}
			break;
		}
	}
}

static void ufbxos_wait_sema_signal(ufbx_os_thread_pool *pool, uint32_t index, uint32_t revision)
{
	ufbxos_wait_sema *sema = &pool->wait_semas[index - 1];

	uint64_t old_state = ufbxos_atomic_u64_load(&sema->state);
	for (;;) {
		ufbxos_wait_sema_state s = ufbxos_wait_sema_decode(old_state);

		if (s.revision != revision) break;
		if (s.signaled) break;

		bool do_signal = false;
		if (s.sleepers == 0) {
			s.sleepers = s.waiters;
			s.waiters = 0;
			s.revision++;
			do_signal = true;
		} else {
			s.signaled = true;
		}

		if (ufbxos_atomic_u64_cas(&sema->state, &old_state, ufbxos_wait_sema_encode(s))) {
			if (do_signal && s.sleepers > 0) {
				ufbxos_os_semaphore_signal(&sema->os_semaphore[(s.revision - 1) & 1], s.sleepers);
			}
			break;
		}
	}
}

static void ufbxos_wait_sema_leave(ufbx_os_thread_pool *pool, uint32_t index, uint32_t revision)
{
	ufbxos_wait_sema *sema = &pool->wait_semas[index - 1];
	uint64_t old_state = ufbxos_atomic_u64_load(&sema->state);
	for (;;) {
		ufbxos_wait_sema_state s = ufbxos_wait_sema_decode(old_state);

		if (s.revision != revision) {
			ufbxos_wait_sema_wait(pool, index, revision);
			return;
		}
		s.waiters -= 1;

		if (ufbxos_atomic_u64_cas(&sema->state, &old_state, ufbxos_wait_sema_encode(s))) {

			// If we were the last waiter signal the semaphore so it won't stay dangling
			if (s.waiters == 0) {
				ufbxos_wait_sema_signal(pool, index, revision);
			}

			return;
		}
	}
}

static uint32_t ufbxos_hash_ptr(const void *ptr)
{
	uint32_t x = (uint32_t)(((uintptr_t)ptr >> 2) ^ ((uintptr_t)ptr >> 30));
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
	if (x == 0) x = 1;
    return x;
}

static bool ufbxos_hash_is(uint32_t ref_hash, uint32_t hash)
{
	hash = hash & 0x7fffff;
	if (ref_hash & 0x800000) {
		return false;
	} else {
		return ref_hash == hash;
	}
}

static bool ufbxos_hash_contains(uint32_t ref_hash, uint32_t hash)
{
	hash = hash & 0x7fffff;
	if (ref_hash & 0x800000) {
		return (ref_hash & (1u << (hash % 23u))) != 0;
	} else {
		return ref_hash == hash;
	}
}

static uint32_t ufbxos_hash_insert(uint32_t ref_hash, uint32_t hash)
{
	hash = hash & 0x7fffff;
	if (ref_hash == 0 || hash == ref_hash) return hash;
	if ((ref_hash & 0x800000) == 0) {
		ref_hash = 0x800000 | (1u << (ref_hash % 23));
	}
	ref_hash |= 1u << (hash % 23u);
	return ref_hash;
}

static bool ufbxos_atomic_try_wait(ufbx_os_thread_pool *pool, uint32_t *p_sema_index, uint32_t *p_sema_revision, uint32_t hash, uint32_t attempt)
{
	for (uint32_t i = 0; i < UFBXOS_WAIT_MAP_SCAN; i++) {
		uint32_t index = (hash + i) & (UFBXOS_WAIT_MAP_SIZE - 1);
		ufbxos_wait_entry *entry = &pool->wait_map[index];
		uint64_t old_state = ufbxos_atomic_u64_load(&entry->state);

		for (;;) {
			ufbxos_wait_entry_state s = ufbxos_wait_entry_decode(old_state);

			bool ok = false;
			if (ufbxos_hash_is(s.hash, hash)) ok = true;
			if (attempt >= 1 && s.hash == 0) ok = true;
			if (attempt >= 2 && ufbxos_hash_contains(s.hash, hash)) ok = true;
			if (attempt >= 3) ok = false;
			if (!ok) break;

			if (s.sema_index == 0 || ufbxos_wait_sema_outdated(pool, s.sema_index, s.sema_revision)) {
				uint32_t revision = 0;
				s.hash = ufbxos_hash_insert(s.hash, hash);
				s.sema_index = ufbxos_wait_sema_alloc(pool, index, &revision);
				s.sema_revision = revision;
				if (s.sema_index != 0 && ufbxos_atomic_u64_cas(&entry->state, &old_state, ufbxos_wait_entry_encode(s))) {
					*p_sema_index = s.sema_index;
					*p_sema_revision = s.sema_revision;
					return true;
				} else {
					ufbxos_wait_sema_leave(pool, s.sema_index, s.sema_revision);
				}
			} else {
				uint32_t old_hash = s.hash;
				s.hash = ufbxos_hash_insert(s.hash, hash);
				if (s.hash == old_hash || ufbxos_atomic_u64_cas(&entry->state, &old_state, ufbxos_wait_entry_encode(s))) {
					if (ufbxos_wait_sema_join(pool, s.sema_index, s.sema_revision)) {
						*p_sema_index = s.sema_index;
						*p_sema_revision = s.sema_revision;
						return true;
					} else {
						break;
					}
				}
			}
		}
	}

	// All semas are full, pause for a bit
	if (attempt == 3) {
		ufbxos_os_yield();
	}

	return false;
}

#if 0
static void ufbxos_atomic_wait32(ufbx_os_thread_pool *pool, ufbxos_atomic_u32 *ptr, uint32_t value)
{
#if UFBXOS_OS_WAIT_NOTIFY
	ufbxos_os_atomic_wait32(ptr, value);
#else
	uint32_t sema_index = 0;
	uint32_t sema_revision = 0;
	uint32_t hash = ufbxos_hash_ptr((const void*)ptr);
	for (uint32_t attempt = 0; attempt < 4; attempt++) {
		if (ufbxos_atomic_u32_load(ptr) != value) return;
		if (ufbxos_atomic_try_wait(pool, &sema_index, &sema_revision, hash, attempt)) {
			if (ufbxos_atomic_u32_load(ptr) != value) {
				ufbxos_wait_sema_leave(pool, sema_index, sema_revision);
			} else {
				ufbxos_wait_sema_wait(pool, sema_index, sema_revision);
			}
			break;
		}
	}
#endif
}
#endif

static void ufbxos_atomic_wait64(ufbx_os_thread_pool *pool, ufbxos_atomic_u64 *ptr, uint64_t value)
{
#if UFBXOS_OS_WAIT_NOTIFY
	ufbxos_os_atomic_wait64(ptr, value);
#else
	uint32_t sema_index = 0;
	uint32_t sema_revision = 0;
	uint32_t hash = ufbxos_hash_ptr((const void*)ptr);
	for (uint32_t attempt = 0; attempt < 4; attempt++) {
		if (ufbxos_atomic_u64_load(ptr) != value) return;
		if (ufbxos_atomic_try_wait(pool, &sema_index, &sema_revision, hash, attempt)) {
			if (ufbxos_atomic_u64_load(ptr) != value) {
				ufbxos_wait_sema_leave(pool, sema_index, sema_revision);
			} else {
				ufbxos_wait_sema_wait(pool, sema_index, sema_revision);
			}
			break;
		}
	}
#endif
}

static void ufbxos_atomic_notify(ufbx_os_thread_pool *pool, const void *ptr)
{
	uint32_t hash = ufbxos_hash_ptr(ptr);
	for (uint32_t i = 0; i < UFBXOS_WAIT_MAP_SCAN; i++) {
		uint32_t index = (hash + i) & (UFBXOS_WAIT_MAP_SIZE - 1);
		ufbxos_wait_entry *entry = &pool->wait_map[index];
		uint64_t old_state = ufbxos_atomic_u64_load(&entry->state);
		if (old_state == 0) continue;

		for (;;) {
			ufbxos_wait_entry_state s = ufbxos_wait_entry_decode(old_state);
			if (!ufbxos_hash_contains(s.hash, hash)) break;
			if (ufbxos_atomic_u64_cas(&entry->state, &old_state, 0)) {
				ufbxos_wait_sema_signal(pool, s.sema_index, s.sema_revision);
				break;
			}
		}
	}
}

#if 0
static void ufbxos_atomic_notify32(ufbx_os_thread_pool *pool, ufbxos_atomic_u32 *ptr)
{
#if UFBXOS_OS_WAIT_NOTIFY
	ufbxos_os_atomic_notify32(ptr);
#else
	ufbxos_atomic_notify(pool, (const void*)ptr);
#endif
}
#endif

static void ufbxos_atomic_notify64(ufbx_os_thread_pool *pool, ufbxos_atomic_u64 *ptr)
{
#if UFBXOS_OS_WAIT_NOTIFY
	ufbxos_os_atomic_notify64(ptr);
#else
	ufbxos_atomic_notify(pool, (const void*)ptr);
#endif
}

static uint32_t ufbxos_task_index(ufbx_os_thread_pool *pool, uint64_t task_id)
{
	return (uint32_t)task_id & pool->task_index_mask;
}

static uint64_t ufbxos_task_cycle(ufbx_os_thread_pool *pool, uint64_t task_id)
{
	return task_id >> pool->task_cycle_shift;
}

static uint64_t ufbxos_task_id(ufbx_os_thread_pool *pool, uint32_t index, uint64_t cycle)
{
	ufbxos_assert(index <= pool->task_index_mask);
	return (cycle << pool->task_cycle_shift) | index;
}

static uint64_t ufbxos_push_task(ufbx_os_thread_pool *pool, ufbx_os_thread_pool_task_fn *fn, void *user, uint32_t count)
{
	uint64_t task_id = ufbxos_atomic_u64_load(&pool->task_alloc_head);
	for (;;) {
		if (task_id == 0) {
			if (ufbxos_atomic_u32_load_relaxed(&pool->task_init_count) < pool->num_tasks) {
				uint32_t index = ufbxos_atomic_u32_inc(&pool->task_init_count);
				if (index < pool->num_tasks) {
					task_id = index;
					break;
				}
			}

			ufbxos_atomic_wait64(pool, &pool->task_alloc_head, 0);
			continue;
		}

		ufbxos_task *task = &pool->tasks[ufbxos_task_index(pool, task_id)];
		uint64_t next_id = ufbxos_atomic_u64_load(&task->next);
		if (ufbxos_atomic_u64_cas(&pool->task_alloc_head, &task_id, next_id & ~UFBXOS_TASK_FREE_BIT)) {
			ufbxos_assert(next_id & UFBXOS_TASK_FREE_BIT);
			break;
		}
	}

	uint32_t task_index = ufbxos_task_index(pool, task_id);
	// uint64_t task_cycle = ufbxos_task_cycle(pool, task_id);
	ufbxos_task *task = &pool->tasks[task_index];

	task->fn = fn;
	task->user = user;
	task->count = count;
	ufbxos_atomic_u32_store(&task->counter, 0);
	ufbxos_atomic_u64_store(&task->next, task_id);
	ufbxos_atomic_notify64(pool, &task->next);
	uint64_t work_head = ufbxos_atomic_u64_load(&pool->task_work_head);
	for (;;) {
		ufbxos_task *prev = &pool->tasks[ufbxos_task_index(pool, work_head)];

		if (ufbxos_atomic_u64_cas(&pool->task_work_head, &work_head, task_id)) {
			ufbxos_atomic_u64_store(&prev->next, task_id);
			ufbxos_atomic_notify64(pool, &prev->next);
			break;
		}
	}

	return task_id;
}

static void ufbxos_thread_pool_entry(ufbx_os_thread_pool *pool)
{
	uint64_t task_id = 0;
	for (;;) {
		uint32_t task_index = ufbxos_task_index(pool, task_id);
		uint64_t task_cycle = ufbxos_task_cycle(pool, task_id);
		ufbxos_task *task = &pool->tasks[task_index];

		bool ok = false;
		uint64_t old_state = ufbxos_atomic_u64_load(&task->state);
		for (;;) {
			ufbxos_task_state s = ufbxos_task_decode(old_state);
			if (s.cycle > task_cycle) break;
			if (s.refcount >= 0xffff) break;

			s.refcount += 1;
			if (ufbxos_atomic_u64_cas(&task->state, &old_state, ufbxos_task_encode(s))) {
				ok = true;
				break;
			}
		}

		uint64_t free_task_id = 0;
		if (ok) {
			ufbx_os_thread_pool_task_fn *fn = task->fn;

			void *user = task->user;
			uint32_t count = task->count;
			for (;;) {
				uint32_t index = ufbxos_atomic_u32_inc(&task->counter);
				if (index >= count) break;
				if (fn == NULL) return;
				fn(user, index);
			}

			old_state = ufbxos_atomic_u64_load(&task->state);
			for (;;) {
				ufbxos_task_state s = ufbxos_task_decode(old_state);
				s.refcount -= 1;
				if (s.refcount == 0) {
					s.cycle += 1;
				}
				if (ufbxos_atomic_u64_cas(&task->state, &old_state, ufbxos_task_encode(s))) {
					if (s.refcount == 0) {
						ufbxos_atomic_notify64(pool, &task->state);
						// TODO: if (cycle <= max_cycle)
						free_task_id = ufbxos_task_id(pool, task_index, s.cycle);
					}
					break;
				}
			}
		}

		uint64_t prev_task_id = task_id;
		for (;;) {
			task_id = ufbxos_atomic_u64_load(&pool->task_work_tail);
			if (task_id != prev_task_id) break;

			task_id = ufbxos_atomic_u64_load(&task->next);
			if (task_id == prev_task_id) {
				ufbxos_atomic_wait64(pool, &task->next, prev_task_id);
				continue;
			}

			uint64_t ref_task_id = prev_task_id;
			if (ufbxos_atomic_u64_cas(&pool->task_work_tail, &ref_task_id, task_id)) {
				break;
			}
		}

		if (free_task_id != 0) {
			uint64_t prev_head = ufbxos_atomic_u64_load(&pool->task_alloc_head);
			for (;;) {
				ufbxos_atomic_u64_store(&task->next, prev_head | UFBXOS_TASK_FREE_BIT);
				ufbxos_atomic_notify64(pool, &task->next);
				if (ufbxos_atomic_u64_cas(&pool->task_alloc_head, &prev_head, free_task_id)) {
					break;
				}
			}
		}
	}
}

// -- API

#if defined(__cplusplus)
extern "C" {
#endif

ufbx_os_abi ufbx_os_thread_pool *ufbx_os_create_thread_pool(const ufbx_os_thread_pool_opts *user_opts)
{
	ufbx_os_thread_pool_opts opts;
	if (user_opts) {
		memcpy(&opts, user_opts, sizeof(ufbx_os_thread_pool_opts));
	} else {
		memset(&opts, 0, sizeof(ufbx_os_thread_pool_opts));
	}

	ufbx_os_thread_pool *pool = (ufbx_os_thread_pool*)calloc(1, sizeof(ufbx_os_thread_pool));
	for (uint32_t i = 0; i < 2; i++) {
		ufbxos_wait_sema_create(pool);
	}

	pool->num_tasks = 256;
	pool->task_index_mask = pool->num_tasks - 1;
	pool->task_cycle_shift = 8;
	pool->tasks = (ufbxos_task*)calloc(pool->num_tasks, sizeof(ufbxos_task));

	size_t num_threads = opts.max_threads;
	if (num_threads == 0) {
		num_threads = ufbxos_os_get_logical_cores();
		if (num_threads > UFBX_OS_DEFAULT_MAX_THREADS) {
			num_threads = UFBX_OS_DEFAULT_MAX_THREADS;
		}
	}
	if (num_threads > UINT32_MAX) {
		num_threads = UINT32_MAX;
	}
	pool->num_threads = (uint32_t)num_threads;

	pool->threads = (ufbxos_os_thread*)calloc(pool->num_threads, sizeof(ufbxos_os_thread));
	for (uint32_t i = 0; i < pool->num_threads; i++) {
		ufbxos_os_thread_start(&pool->threads[i], pool);
	}

	ufbxos_atomic_u32_store(&pool->task_init_count, 1);

	return pool;
}

ufbx_os_abi void ufbx_os_free_thread_pool(ufbx_os_thread_pool *pool)
{
	if (!pool) return;

	ufbxos_push_task(pool, NULL, NULL, pool->num_threads);
	for (size_t i = 0; i < pool->num_threads; i++) {
		ufbxos_os_thread_join(&pool->threads[i]);
	}

	uint32_t sema_count = ufbxos_atomic_u32_load(&pool->wait_sema_count);
	for (uint32_t i = 0; i < sema_count; i++) {
		ufbxos_os_semaphore_free(&pool->wait_semas[i].os_semaphore[0]);
		ufbxos_os_semaphore_free(&pool->wait_semas[i].os_semaphore[1]);
	}

	free(pool->tasks);
	free(pool->threads);
	free(pool);
}

ufbx_os_abi uint64_t ufbx_os_thread_pool_run(ufbx_os_thread_pool *pool, ufbx_os_thread_pool_task_fn *fn, void *user, uint32_t count)
{
	ufbxos_assert(fn != NULL);
	return ufbxos_push_task(pool, fn, user, count);
}

ufbx_os_abi bool ufbx_os_thread_pool_try_wait(ufbx_os_thread_pool *pool, uint64_t task_id)
{
	uint32_t task_index = ufbxos_task_index(pool, task_id);
	uint64_t task_cycle = ufbxos_task_cycle(pool, task_id);
	ufbxos_task *task = &pool->tasks[task_index];
	ufbxos_task_state s = ufbxos_task_decode(ufbxos_atomic_u64_load(&task->state));
	return s.cycle > task_cycle;
}

ufbx_os_abi void ufbx_os_thread_pool_wait(ufbx_os_thread_pool *pool, uint64_t task_id)
{
	uint32_t task_index = ufbxos_task_index(pool, task_id);
	uint64_t task_cycle = ufbxos_task_cycle(pool, task_id);
	ufbxos_task *task = &pool->tasks[task_index];
	for (;;) {
		uint64_t state = ufbxos_atomic_u64_load(&task->state);
		ufbxos_task_state s = ufbxos_task_decode(state);
		if (s.cycle > task_cycle) return;
		ufbxos_atomic_wait64(pool, &task->state, state);
	}
}

typedef struct {
	uint64_t task_id;
	uint32_t start_index; 
	ufbx_thread_pool_context ctx;
} ufbxos_pool_group;

typedef struct {
	union {
		ufbxos_pool_group group;
		char padding[64];
	} groups[UFBX_THREAD_GROUP_COUNT];
	void *allocation;
} ufbxos_pool_ctx;

static void ufbxos_ufbx_task(void *user, uint32_t index)
{
	ufbxos_pool_group *group = (ufbxos_pool_group*)user;
	ufbx_thread_pool_run_task(group->ctx, group->start_index + index);
}

static bool ufbxos_ufbx_thread_pool_init(void *user, ufbx_thread_pool_context ctx, const ufbx_thread_pool_info *info)
{
	(void)info;
	(void)user;
	void *allocation = calloc(1, sizeof(ufbxos_pool_ctx) + 128);
	if (!allocation) return false;

	char *data = (char*)allocation + (((uintptr_t)-(intptr_t)allocation) & 63);
	ufbxos_pool_ctx *up = (ufbxos_pool_ctx*)data;
	up->allocation = allocation;

	ufbx_thread_pool_set_user_ptr(ctx, up);

	return true;
}

static void ufbxos_ufbx_thread_pool_run(void *user, ufbx_thread_pool_context ctx, uint32_t group, uint32_t start_index, uint32_t count)
{
	ufbx_os_thread_pool *pool = (ufbx_os_thread_pool*)user;
	ufbxos_pool_ctx *up = (ufbxos_pool_ctx*)ufbx_thread_pool_get_user_ptr(ctx);
	ufbxos_pool_group *ug = &up->groups[group].group;

	ug->start_index = start_index;
	ug->ctx = ctx;
	ug->task_id = ufbx_os_thread_pool_run(pool, &ufbxos_ufbx_task, ug, count);
}

static void ufbxos_ufbx_thread_pool_wait(void *user, ufbx_thread_pool_context ctx, uint32_t group, uint32_t max_index)
{
	(void)max_index;
	ufbx_os_thread_pool *pool = (ufbx_os_thread_pool*)user;
	ufbxos_pool_ctx *up = (ufbxos_pool_ctx*)ufbx_thread_pool_get_user_ptr(ctx);
	ufbxos_pool_group *ug = &up->groups[group].group;

	ufbx_os_thread_pool_wait(pool, ug->task_id);
}

static void ufbxos_ufbx_thread_pool_free(void *user, ufbx_thread_pool_context ctx)
{
	(void)user;
	ufbxos_pool_ctx *up = (ufbxos_pool_ctx*)ufbx_thread_pool_get_user_ptr(ctx);
	free(up->allocation);
}

ufbx_os_abi void ufbx_os_init_ufbx_thread_pool(ufbx_thread_pool *dst, ufbx_os_thread_pool *pool)
{
	if (!dst || !pool) return;

	memset(dst, 0, sizeof(ufbx_thread_pool));
	dst->user = pool;
	dst->init_fn = &ufbxos_ufbx_thread_pool_init;
	dst->run_fn = &ufbxos_ufbx_thread_pool_run;
	dst->wait_fn = &ufbxos_ufbx_thread_pool_wait;
	dst->free_fn = &ufbxos_ufbx_thread_pool_free;
}

#if defined(__cplusplus)
}
#endif

#endif
#endif
