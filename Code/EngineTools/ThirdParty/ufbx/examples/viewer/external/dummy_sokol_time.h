#if defined(SOKOL_IMPL) && !defined(SOKOL_TIME_IMPL)
#define SOKOL_TIME_IMPL
#endif
#ifndef SOKOL_TIME_INCLUDED
/*
    sokol_time.h    -- simple cross-platform time measurement

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_TIME_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following defines with your own implementations:
    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_TIME_API_DECL - public function declaration prefix (default: extern)
    SOKOL_API_DECL      - same as SOKOL_TIME_API_DECL
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_time.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_TIME_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    void stm_setup();
        Call once before any other functions to initialize sokol_time
        (this calls for instance QueryPerformanceFrequency on Windows)

    uint64_t stm_now();
        Get current point in time in unspecified 'ticks'. The value that
        is returned has no relation to the 'wall-clock' time and is
        not in a specific time unit, it is only useful to compute
        time differences.

    uint64_t stm_diff(uint64_t new, uint64_t old);
        Computes the time difference between new and old. This will always
        return a positive, non-zero value.

    uint64_t stm_since(uint64_t start);
        Takes the current time, and returns the elapsed time since start
        (this is a shortcut for "stm_diff(stm_now(), start)")

    uint64_t stm_laptime(uint64_t* last_time);
        This is useful for measuring frame time and other recurring
        events. It takes the current time, returns the time difference
        to the value in last_time, and stores the current time in
        last_time for the next call. If the value in last_time is 0,
        the return value will be zero (this usually happens on the
        very first call).

    uint64_t stm_round_to_common_refresh_rate(uint64_t duration)
        This oddly named function takes a measured frame time and
        returns the closest "nearby" common display refresh rate frame duration
        in ticks. If the input duration isn't close to any common display
        refresh rate, the input duration will be returned unchanged as a fallback.
        The main purpose of this function is to remove jitter/inaccuracies from
        measured frame times, and instead use the display refresh rate as
        frame duration.

    Use the following functions to convert a duration in ticks into
    useful time units:

    double stm_sec(uint64_t ticks);
    double stm_ms(uint64_t ticks);
    double stm_us(uint64_t ticks);
    double stm_ns(uint64_t ticks);
        Converts a tick value into seconds, milliseconds, microseconds
        or nanoseconds. Note that not all platforms will have nanosecond
        or even microsecond precision.

    Uses the following time measurement functions under the hood:

    Windows:        QueryPerformanceFrequency() / QueryPerformanceCounter()
    MacOS/iOS:      mach_absolute_time()
    emscripten:     performance.now()
    Linux+others:   clock_gettime(CLOCK_MONOTONIC)

    zlib/libpng license

    Copyright (c) 2018 Andre Weissflog

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.

        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.

        3. This notice may not be removed or altered from any source
        distribution.
*/
#define SOKOL_TIME_INCLUDED (1)
#include <stdint.h>

#if defined(SOKOL_API_DECL) && !defined(SOKOL_TIME_API_DECL)
#define SOKOL_TIME_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_TIME_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_TIME_IMPL)
#define SOKOL_TIME_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_TIME_API_DECL __declspec(dllimport)
#else
#define SOKOL_TIME_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

SOKOL_TIME_API_DECL void stm_setup(void);
SOKOL_TIME_API_DECL uint64_t stm_now(void);
SOKOL_TIME_API_DECL uint64_t stm_diff(uint64_t new_ticks, uint64_t old_ticks);
SOKOL_TIME_API_DECL uint64_t stm_since(uint64_t start_ticks);
SOKOL_TIME_API_DECL uint64_t stm_laptime(uint64_t* last_time);
SOKOL_TIME_API_DECL uint64_t stm_round_to_common_refresh_rate(uint64_t frame_ticks);
SOKOL_TIME_API_DECL double stm_sec(uint64_t ticks);
SOKOL_TIME_API_DECL double stm_ms(uint64_t ticks);
SOKOL_TIME_API_DECL double stm_us(uint64_t ticks);
SOKOL_TIME_API_DECL double stm_ns(uint64_t ticks);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // SOKOL_TIME_INCLUDED

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_TIME_IMPL
#define SOKOL_TIME_IMPL_INCLUDED (1)
#include <string.h> /* memset */

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif

extern uint64_t dummy_stm_time_ns;

SOKOL_API_IMPL void stm_setup(void)
{
    if (dummy_stm_time_ns == 0) {
        dummy_stm_time_ns = 1;
    }
}

SOKOL_API_IMPL uint64_t stm_now(void)
{
    return dummy_stm_time_ns;
}

SOKOL_API_IMPL uint64_t stm_diff(uint64_t new_ticks, uint64_t old_ticks)
{
    return new_ticks - old_ticks;
}

SOKOL_API_IMPL uint64_t stm_since(uint64_t start_ticks)
{
    return stm_now() - start_ticks;
}

SOKOL_API_IMPL uint64_t stm_laptime(uint64_t* last_time)
{
    uint64_t dt = 0;
    uint64_t now = stm_now();
    if (0 != *last_time) {
        dt = stm_diff(now, *last_time);
    }
    *last_time = now;
    return dt;
}

SOKOL_API_IMPL uint64_t stm_round_to_common_refresh_rate(uint64_t frame_ticks)
{
    return frame_ticks;
}

SOKOL_API_IMPL double stm_sec(uint64_t ticks)
{
    return (double)ticks * 1e-9;
}

SOKOL_API_IMPL double stm_ms(uint64_t ticks)
{
    return (double)ticks * 1e-6;
}

SOKOL_API_IMPL double stm_us(uint64_t ticks)
{
    return (double)ticks * 1e-3;
}

SOKOL_API_IMPL double stm_ns(uint64_t ticks)
{
    return (double)ticks;
}

#endif /* SOKOL_TIME_IMPL */
