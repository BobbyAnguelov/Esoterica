#ifndef UFBXT_CPUTIME_H_INCLUDED
#define UFBXT_CPUTIME_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint64_t os_tick;
	uint64_t cpu_tick;
} cputime_sync_point;

typedef struct {
	cputime_sync_point begin, end;
	uint64_t os_freq;
	uint64_t cpu_freq;
	double rcp_os_freq;
	double rcp_cpu_freq;
} cputime_sync_span;

extern const cputime_sync_span *cputime_default_sync;

void cputime_begin_init();
void cputime_end_init();
void cputime_init();

void cputime_begin_sync(cputime_sync_span *span);
void cputime_end_sync(cputime_sync_span *span);

uint64_t cputime_cpu_tick();
uint64_t cputime_os_tick();

double cputime_cpu_delta_to_sec(const cputime_sync_span *span, uint64_t cpu_delta);
double cputime_os_delta_to_sec(const cputime_sync_span *span, uint64_t os_delta);
double cputime_cpu_tick_to_sec(const cputime_sync_span *span, uint64_t cpu_tick);
double cputime_os_tick_to_sec(const cputime_sync_span *span, uint64_t os_tick);

#ifdef __cplusplus
}
#endif

#endif

#if defined(CPUTIME_IMPLEMENTATION)
#ifndef CPUTIME_IMEPLEMENTED
#define CPUTIME_IMEPLEMENTED

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CPUTIME_STANDARD_C) || defined(UFBX_STANDARD_C)

#include <time.h>

void cputime_sync_now(cputime_sync_point *sync, int accuracy)
{
	uint64_t tick = (uint64_t)clock();
	sync->cpu_tick = tick;
	sync->os_tick = tick;
}

uint64_t cputime_cpu_tick()
{
	return (uint64_t)clock();
}

uint64_t cputime_os_tick()
{
	return (uint64_t)clock();
}

static uint64_t cputime_os_freq()
{
	return (uint64_t)CLOCKS_PER_SEC;
}

static void cputime_os_wait()
{
}

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#if defined(_MSC_VER)
	#include <intrin.h>
#else
	#include <x86intrin.h>
#endif

void cputime_sync_now(cputime_sync_point *sync, int accuracy)
{
	uint64_t best_delta = UINT64_MAX;
	uint64_t os_tick = 0, cpu_tick = 0;

	int runs = accuracy ? accuracy : 100;
	for (int i = 0; i < runs; i++) {
		LARGE_INTEGER begin, end;
		QueryPerformanceCounter(&begin);
		uint64_t cycle = __rdtsc();
		QueryPerformanceCounter(&end);

		uint64_t delta = end.QuadPart - begin.QuadPart;
		if (delta < best_delta) {
			os_tick = (begin.QuadPart + end.QuadPart) / 2;
			cpu_tick = cycle;
		}

		if (delta == 0) break;
	}

	sync->cpu_tick = cpu_tick;
	sync->os_tick = os_tick;
}

uint64_t cputime_cpu_tick()
{
	return __rdtsc();
}

uint64_t cputime_os_tick()
{
	LARGE_INTEGER res;
	QueryPerformanceCounter(&res);
	return res.QuadPart;
}

static uint64_t cputime_os_freq()
{
	LARGE_INTEGER res;
	QueryPerformanceFrequency(&res);
	return res.QuadPart;
}

static void cputime_os_wait()
{
	Sleep(1);
}

#else

#include <time.h>

#if (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
	#include <x86intrin.h>
	#define cputime_imp_timestamp() (uint64_t)(__rdtsc())
#else
	static uint64_t cputime_imp_timestamp()
	{
		struct timespec time;
		clock_gettime(CLOCK_MONOTONIC, &time);
		return (uint64_t)time.tv_sec*UINT64_C(1000000000) + (uint64_t)time.tv_nsec;
	}
#endif

void cputime_sync_now(cputime_sync_point *sync, int accuracy)
{
	uint64_t best_delta = UINT64_MAX;
	uint64_t os_tick, cpu_tick;

	struct timespec begin, end;

	int runs = accuracy ? accuracy : 100;
	for (int i = 0; i < runs; i++) {
		clock_gettime(CLOCK_REALTIME, &begin);
		uint64_t cycle = cputime_imp_timestamp();
		clock_gettime(CLOCK_REALTIME, &end);

		uint64_t begin_ns = (uint64_t)begin.tv_sec*UINT64_C(1000000000) + (uint64_t)begin.tv_nsec;
		uint64_t end_ns = (uint64_t)end.tv_sec*UINT64_C(1000000000) + (uint64_t)end.tv_nsec;

		uint64_t delta = end_ns - begin_ns;
		if (delta < best_delta) {
			os_tick = (begin_ns + end_ns) / 2;
			cpu_tick = cycle;
		}

		if (delta == 0) break;
	}

	sync->cpu_tick = cpu_tick;
	sync->os_tick = os_tick;
}

uint64_t cputime_cpu_tick()
{
	return cputime_imp_timestamp();
}

uint64_t cputime_os_tick()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec*UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static uint64_t cputime_os_freq()
{
	return UINT64_C(1000000000);
}

static void cputime_os_wait()
{
	struct timespec duration;
	duration.tv_sec = 0;
	duration.tv_nsec = 1000000000l;
	nanosleep(&duration, NULL);
}

#endif

static cputime_sync_span g_cputime_sync;
const cputime_sync_span *cputime_default_sync = &g_cputime_sync;

void cputime_begin_init()
{
	cputime_begin_sync(&g_cputime_sync);
}

void cputime_end_init()
{
	cputime_end_sync(&g_cputime_sync);
}

void cputime_init()
{
	cputime_begin_init();
	cputime_end_init();
}

void cputime_begin_sync(cputime_sync_span *span)
{
	cputime_sync_now(&span->begin, 0);
}

void cputime_end_sync(cputime_sync_span *span)
{
	uint64_t os_freq = cputime_os_freq();

	uint64_t min_span = os_freq / 1000;
	uint64_t os_tick = cputime_os_tick();
	while (os_tick - span->begin.os_tick <= min_span) {
		cputime_os_wait();
		os_tick = cputime_os_tick();
	}

	cputime_sync_now(&span->end, 0);
	uint64_t len_os = span->end.os_tick - span->begin.os_tick;
	uint64_t len_cpu = span->end.cpu_tick - span->begin.cpu_tick;
	double cpu_freq = (double)len_cpu / (double)len_os * (double)os_freq;

	span->os_freq = os_freq;
	span->cpu_freq = (uint64_t)cpu_freq;
	span->rcp_os_freq = 1.0 / (double)os_freq;
	span->rcp_cpu_freq = 1.0 / cpu_freq;
}

double cputime_cpu_delta_to_sec(const cputime_sync_span *span, uint64_t cpu_delta)
{
	if (!span) span = &g_cputime_sync;
	return (double)cpu_delta * span->rcp_cpu_freq;
}

double cputime_os_delta_to_sec(const cputime_sync_span *span, uint64_t os_delta)
{
	if (!span) span = &g_cputime_sync;
	return (double)os_delta * span->rcp_os_freq;
}

double cputime_cpu_tick_to_sec(const cputime_sync_span *span, uint64_t cpu_tick)
{
	if (!span) span = &g_cputime_sync;
	return (double)(cpu_tick - span->begin.cpu_tick) * span->rcp_cpu_freq;
}

double cputime_os_tick_to_sec(const cputime_sync_span *span, uint64_t os_tick)
{
	if (!span) span = &g_cputime_sync;
	return (double)(os_tick - span->begin.os_tick) * span->rcp_os_freq;
}

#ifdef __cplusplus
}
#endif

#endif
#endif
