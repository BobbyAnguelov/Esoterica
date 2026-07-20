/**
 * @file lzav.h
 *
 * @version 5.8
 *
 * @brief Self-contained header file for the "LZAV" in-memory data compression
 * and decompression algorithms.
 *
 * The source code is written in ISO C99, with full C++ compliance enabled
 * conditionally and automatically when compiled with a C++ compiler.
 *
 * Description is available at https://github.com/avaneev/lzav
 *
 * Email: aleksey.vaneev@gmail.com or info@voxengo.com
 *
 * LICENSE:
 *
 * Copyright (c) 2023-2025 Aleksey Vaneev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef LZAV_INCLUDED
#define LZAV_INCLUDED

#define LZAV_API_VER 0x204 ///< API version; unrelated to source code version.
#define LZAV_VER_STR "5.8" ///< LZAV source code version string.

/**
 * @def LZAV_FMT_MIN
 * @brief The minimal data format id supported by the decompressor. A value
 * of 3 can be defined externally to reduce the decompressor's code size.
 */

#if !defined( LZAV_FMT_MIN )
	#define LZAV_FMT_MIN 2
#endif // !defined( LZAV_FMT_MIN )

/**
 * @def LZAV_NS_CUSTOM
 * @brief If this macro is defined externally, all symbols will be placed
 * into the namespace specified by the macro, and won't be exported to the
 * global namespace. WARNING: if the defined value of the macro is empty, the
 * symbols will be placed into the global namespace anyway.
 */

/**
 * @def LZAV_NOEX
 * @brief Macro that defines the "noexcept" function specifier for the C++
 * environment.
 */

/**
 * @def LZAV_NULL
 * @brief Macro that defines the "nullptr" value for C++ guidelines
 * compliance.
 */

/**
 * @def LZAV_NS
 * @brief Macro that defines an actual implementation namespace in the C++
 * environment, with export of relevant symbols to the global namespace
 * (if @ref LZAV_NS_CUSTOM is undefined).
 */

/**
 * @def LZAV_MALLOC
 * @brief Macro defining the call to the memory allocation function.
 *
 * Can be defined externally if the standard `malloc` is unavailable, or if
 * the use of the `operator new[]` is undesired in a C++ environment.
 * The implementation must return a `T*` pointer aligned to 4 bytes, or
 * preferably, 16 bytes.
 *
 * The called function should have the "noexcept" or "throw()" specifier.
 *
 * @param s Allocation size, in bytes; a multiple of `sizeof( T )`.
 * @param T Allocation element type, for a C++ environment.
 */

/**
 * @def LZAV_FREE
 * @brief Macro defining the call to the memory free function.
 *
 * Can be defined externally if the standard `free` is unavailable, or if
 * the `operator delete[]` is undesired in a C++ environment.
 *
 * @param p Memory block pointer to free.
 */

/**
 * @def LZAV_DEF_MALLOC
 * @brief Macro denotes that the default memory allocator is being used.
 */

#if !defined( LZAV_MALLOC )

	#if defined( LZAV_FREE )
		#error LZAV: the LZAV_FREE is defined while the LZAV_MALLOC is not.
	#endif // defined( LZAV_FREE )

	#define LZAV_DEF_MALLOC

#else // !defined( LZAV_MALLOC )

	#if !defined( LZAV_FREE )
		#error LZAV: the LZAV_MALLOC is defined while the LZAV_FREE is not.
	#endif // !defined( LZAV_FREE )

#endif // !defined( LZAV_MALLOC )

#if defined( __cplusplus )

	#include <cstring>

	#if __cplusplus >= 201103L

		#include <cstdint>

		#define LZAV_NOEX noexcept
		#define LZAV_NULL nullptr

		#if defined( LZAV_DEF_MALLOC )
			#include <new>

			#define LZAV_MALLOC( s, T ) \
				new( std :: nothrow ) T[ s / sizeof( T )]

			#define LZAV_FREE( p ) delete[] p
		#endif // !defined( LZAV_DEF_MALLOC )

	#else // __cplusplus >= 201103L

		#include <stdint.h>

		#define LZAV_NOEX throw()
		#define LZAV_NULL NULL

		#if defined( LZAV_DEF_MALLOC )
			#include <cstdlib>

			#define LZAV_MALLOC( s, T ) (T*) std :: malloc( s )
			#define LZAV_FREE( p ) std :: free( p )
		#endif // !defined( LZAV_DEF_MALLOC )

	#endif // __cplusplus >= 201103L

	#if defined( LZAV_NS_CUSTOM )
		#define LZAV_NS LZAV_NS_CUSTOM
	#else // defined( LZAV_NS_CUSTOM )
		#define LZAV_NS lzav
	#endif // defined( LZAV_NS_CUSTOM )

#else // defined( __cplusplus )

	#include <string.h>
	#include <stdint.h>

	#define LZAV_NOEX
	#define LZAV_NULL NULL

	#if defined( LZAV_DEF_MALLOC )
		#include <stdlib.h>

		#define LZAV_MALLOC( s, T ) (T*) malloc( s )
		#define LZAV_FREE( p ) free( p )
	#endif // !defined( LZAV_DEF_MALLOC )

#endif // defined( __cplusplus )

#if SIZE_MAX < 0xFFFFFFFFU

	#error LZAV: the platform or the compiler has an incompatible size_t type.

#endif // size_t check

/**
 * @def LZAV_X86
 * @brief Macro that is defined if an `x86` or `x86_64` platform is detected.
 */

#if defined( i386 ) || defined( __i386 ) || defined( __i386__ ) || \
	defined( _X86_ ) || defined( __x86_64 ) || defined( __x86_64__ ) || \
	defined( __amd64 ) || defined( __amd64__ ) || defined( _M_IX86 ) || \
	( defined( _M_AMD64 ) && !defined( _M_ARM64EC ))

	#define LZAV_X86

#endif // x86 platform check

/**
 * @def LZAV_LITTLE_ENDIAN
 * @brief Endianness definition macro that can be used as a logical constant.
 * It always equals 0 if the C++20 `endian` is in use.
 */

/**
 * @def LZAV_COND_EC( vl, vb )
 * @brief Macro that emits either `vl` or `vb`, depending on the platform's
 * endianness.
 */

#if ( defined( __BYTE_ORDER__ ) && \
		__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ ) || \
	( defined( __BYTE_ORDER ) && __BYTE_ORDER == __LITTLE_ENDIAN ) || \
	defined( __LITTLE_ENDIAN__ ) || defined( _LITTLE_ENDIAN ) || \
	defined( LZAV_X86 ) || defined( _WIN32 ) || defined( _M_ARM ) || \
	defined( _M_ARM64EC )

	#define LZAV_LITTLE_ENDIAN 1

#elif ( defined( __BYTE_ORDER__ ) && \
		__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ ) || \
	( defined( __BYTE_ORDER ) && __BYTE_ORDER == __BIG_ENDIAN ) || \
	defined( __BIG_ENDIAN__ ) || defined( _BIG_ENDIAN ) || \
	defined( __SYSC_ZARCH__ ) || defined( __zarch__ ) || \
	defined( __s390x__ ) || defined( __sparc ) || defined( __sparc__ )

	#define LZAV_LITTLE_ENDIAN 0
	#define LZAV_COND_EC( vl, vb ) ( vb )

#elif defined( __cplusplus ) && __cplusplus >= 202002L

	#include <bit>

	#define LZAV_LITTLE_ENDIAN 0
	#define LZAV_COND_EC( vl, vb ) ( std :: endian :: native == \
		std :: endian :: little ? vl : vb )

#else // defined( __cplusplus )

	#warning LZAV: cannot determine endianness, assuming little-endian.

	#define LZAV_LITTLE_ENDIAN 1

#endif // defined( __cplusplus )

/**
 * @def LZAV_PTR32
 * @brief Macro that denotes that pointers are likely 32-bit (pointer overflow
 * checks are required).
 */

#if SIZE_MAX <= 0xFFFFFFFFU && \
	( !defined( UINTPTR_MAX ) || UINTPTR_MAX <= 0xFFFFFFFFU )

	#define LZAV_PTR32

#endif // 32-bit pointers check

/**
 * @def LZAV_ARCH64
 * @brief Macro that denotes the availability of 64-bit instructions.
 */

#if defined( __LP64__ ) || defined( _LP64 ) || !defined( LZAV_PTR32 ) || \
	defined( __x86_64__ ) || defined( __aarch64__ ) || \
	defined( _M_AMD64 ) || defined( _M_ARM64 )

	#define LZAV_ARCH64

#endif // 64-bit availability check

/**
 * @def LZAV_LONG_COPY
 * @brief Macro that permits the use of runs of 16-byte `memcpy` on platforms
 * where this should not cause performance issues.
 */

#if defined( LZAV_ARCH64 ) || defined( __SSE__ ) || defined( __ARM_NEON ) || \
	( defined( _M_IX86_FP ) && _M_IX86_FP >= 1 ) || \
	defined( __VEC__ ) || defined( __ALTIVEC__ ) || \
	defined( __wasm_simd128__ )

	#define LZAV_LONG_COPY

#endif // Long copy check

/**
 * @def LZAV_GCC_BUILTINS
 * @brief Macro that denotes the availability of GCC-style built-in functions.
 */

/**
 * @def LZAV_CPP_BIT
 * @brief Macro that denotes the availability of C++20 `bit` functions.
 */

#if defined( __GNUC__ ) || defined( __clang__ ) || \
	defined( __IBMC__ ) || defined( __IBMCPP__ ) || \
	defined( __COMPCERT__ ) || ( defined( __INTEL_COMPILER ) && \
		__INTEL_COMPILER >= 1300 && !defined( _MSC_VER ))

	#define LZAV_GCC_BUILTINS

#elif defined( __cplusplus ) && __cplusplus >= 202002L

	#include <bit>

	#define LZAV_CPP_BIT

#elif defined( _MSC_VER )

	#include <intrin.h> // For _BitScanForward.

#endif // defined( _MSC_VER )

/**
 * @def LZAV_IEC32( x )
 * @brief In-place endianness-correction macro for singular 32-bit variables.
 *
 * @param x The value to correct in-place.
 */

#if LZAV_LITTLE_ENDIAN

	#define LZAV_COND_EC( vl, vb ) ( vl )
	#define LZAV_IEC32( x ) (void) 0

#else // LZAV_LITTLE_ENDIAN

	#if defined( LZAV_GCC_BUILTINS )

		#define LZAV_IEC32( x ) x = LZAV_COND_EC( x, __builtin_bswap32( x ))

	#elif defined( _MSC_VER )

		#if defined( __cplusplus )
			#include <cstdlib>
		#else // defined( __cplusplus )
			#include <stdlib.h>
		#endif // defined( __cplusplus )

		#define LZAV_IEC32( x ) x = LZAV_COND_EC( x, _byteswap_ulong( x ))

	#elif defined( __cplusplus ) && __cplusplus >= 202302L

		#define LZAV_IEC32( x ) x = LZAV_COND_EC( x, std :: byteswap( x ))

	#else // defined( __cplusplus )

		#define LZAV_IEC32( x ) x = (uint32_t) LZAV_COND_EC( x, \
			x >> 24 | \
			( x & 0x00FF0000 ) >> 8 | \
			( x & 0x0000FF00 ) << 8 | \
			x << 24 )

	#endif // defined( __cplusplus )

#endif // LZAV_LITTLE_ENDIAN

/**
 * @def LZAV_LIKELY( x )
 * @brief Likelihood macro that is used for manually-guided
 * micro-optimization.
 *
 * @param x An expression that is likely to be evaluated to 1.
 */

/**
 * @def LZAV_UNLIKELY( x )
 * @brief Unlikelihood macro that is used for manually-guided
 * micro-optimization.
 *
 * @param x An expression that is unlikely to be evaluated to 1.
 */

#if defined( LZAV_GCC_BUILTINS )

	#define LZAV_LIKELY( x ) ( __builtin_expect( x, 1 ))
	#define LZAV_UNLIKELY( x ) ( __builtin_expect( x, 0 ))

#elif defined( __cplusplus ) && __cplusplus >= 202002L

	#define LZAV_LIKELY( x ) ( x ) [[likely]]
	#define LZAV_UNLIKELY( x ) ( x ) [[unlikely]]

#else // Likelihood macros

	#define LZAV_LIKELY( x ) ( x )
	#define LZAV_UNLIKELY( x ) ( x )

#endif // Likelihood macros

/**
 * @def LZAV_RESTRICT
 * @brief Macro that defines the "restrict" variable specifier.
 */

#if defined( LZAV_GCC_BUILTINS ) || defined( _MSC_VER )

	#define LZAV_RESTRICT __restrict

#elif !defined( __cplusplus )

	#define LZAV_RESTRICT restrict

#else // !defined( __cplusplus )

	#define LZAV_RESTRICT

#endif // !defined( __cplusplus )

/**
 * @def LZAV_PREFETCH( a )
 * @brief Memory address prefetch macro to preload data into the CPU cache.
 *
 * @param a Prefetch address.
 */

#if defined( LZAV_GCC_BUILTINS ) && !defined( __COMPCERT__ )

	#define LZAV_PREFETCH( a ) __builtin_prefetch( a, 0, 3 )

#elif defined( _MSC_VER ) && !defined( __INTEL_COMPILER ) && \
	defined( LZAV_X86 )

	#include <intrin.h>

	#define LZAV_PREFETCH( a ) _mm_prefetch( (const char*) ( a ), _MM_HINT_T0 )

#else // defined( _MSC_VER )

	#define LZAV_PREFETCH( a ) (void) 0

#endif // defined( _MSC_VER )

/**
 * @def LZAV_STATIC
 * @brief Macro that defines a function as "static".
 */

#if ( defined( __cplusplus ) && __cplusplus >= 201703L ) || \
	( defined( __STDC_VERSION__ ) && __STDC_VERSION__ >= 202311L )

	#define LZAV_STATIC [[maybe_unused]] static

#elif defined( LZAV_GCC_BUILTINS )

	#define LZAV_STATIC static __attribute__((unused))

#else // defined( LZAV_GCC_BUILTINS )

	#define LZAV_STATIC static

#endif // defined( LZAV_GCC_BUILTINS )

/**
 * @def LZAV_INLINE
 * @brief Macro that defines a function as inlinable at the compiler's
 * discretion.
 */

#define LZAV_INLINE LZAV_STATIC inline

/**
 * @def LZAV_INLINE_F
 * @brief Macro to force code inlining.
 */

#if defined( LZAV_GCC_BUILTINS )

	#define LZAV_INLINE_F LZAV_INLINE __attribute__((always_inline))

#elif defined( _MSC_VER )

	#define LZAV_INLINE_F LZAV_STATIC __forceinline

#else // defined( _MSC_VER )

	#define LZAV_INLINE_F LZAV_INLINE

#endif // defined( _MSC_VER )

/**
 * @def LZAV_NO_INLINE
 * @brief Macro that defines a function as not inlinable.
 */

#if defined( __cplusplus )

	#define LZAV_NO_INLINE LZAV_STATIC

#else // defined( __cplusplus )

	#define LZAV_NO_INLINE LZAV_INLINE

#endif // defined( __cplusplus )

#if defined( LZAV_NS )

namespace LZAV_NS {

using std :: memcpy;
using std :: memset;
using std :: size_t;

#if __cplusplus >= 201103L

	using std :: uint16_t;
	using std :: uint32_t;
	using uint8_t = unsigned char; ///< For C++ type aliasing compliance.

	#if defined( LZAV_ARCH64 )
		using std :: uint64_t;
	#endif // defined( LZAV_ARCH64 )

#endif // __cplusplus >= 201103L

namespace enum_wrapper {

#endif // defined( LZAV_NS )

/**
 * @brief Decompression error codes.
 */

enum LZAV_ERROR
{
	LZAV_E_PARAMS = -1, ///< Incorrect function parameters.
	LZAV_E_SRCOOB = -2, ///< Source buffer out-of-bounds error.
	LZAV_E_DSTOOB = -3, ///< Destination buffer out-of-bounds error.
	LZAV_E_REFOOB = -4, ///< Back-reference out-of-bounds error.
	LZAV_E_DSTLEN = -5, ///< Decompressed length mismatch error.
	LZAV_E_UNKFMT = -6, ///< Unknown data format or mref error.
	LZAV_E_PTROVR = -7 ///< Pointer overflow error.
};

#if defined( LZAV_NS )

} // namespace enum_wrapper

using namespace enum_wrapper;

#endif // defined( LZAV_NS )

/**
 * @brief Compression algorithm parameters.
 */

enum LZAV_PARAM
{
	LZAV_WIN_LEN = ( 1 << 21 ), ///< The LZ77 window length, in bytes.
	LZAV_LIT_FIN = 9, ///< The number of literals required at finish.
	LZAV_OFS_MIN = 8, ///< The minimal reference offset to use.
	LZAV_OFS_TH1 = ( 1 << 10 ) - 1, ///< Reference offset threshold 1.
	LZAV_OFS_TH2 = ( 1 << 15 ) - 1, ///< Reference offset threshold 2.
	LZAV_MR5_THR = ( 1 << 18 ), ///< `srclen` threshold to use `mref=5`.
	LZAV_FMT_CUR = 3 ///< The data format identifier used by the compressor.
};

/**
 * @brief Data match length finding function.
 *
 * This function finds the number of continuously-matching leading bytes
 * between two buffers. This function is well-optimized for a wide variety of
 * compilers and platforms.
 *
 * @param p1 Pointer to buffer 1.
 * @param p2 Pointer to buffer 2.
 * @param ml The maximal number of bytes to match.
 * @param o The initial offset, can be greater than `ml`.
 * @return The number of matching leading bytes, not less than `o`.
 */

LZAV_INLINE_F size_t lzav_match_len( const uint8_t* const p1,
	const uint8_t* const p2, const size_t ml, size_t o ) LZAV_NOEX
{
#if defined( LZAV_ARCH64 )

	size_t o2 = o + 7;

	while LZAV_LIKELY( o2 < ml )
	{
		uint64_t v1, v2, vd;
		memcpy( &v1, p1 + o, 8 );
		memcpy( &v2, p2 + o, 8 );
		vd = v1 ^ v2;

		if( vd != 0 )
		{
		#if defined( LZAV_GCC_BUILTINS )

			return( o + (size_t) ( LZAV_COND_EC(
				__builtin_ctzll( vd ), __builtin_clzll( vd )) >> 3 ));

		#elif defined( LZAV_CPP_BIT )

			return( o + (size_t) ( LZAV_COND_EC(
				std :: countr_zero( vd ), std :: countl_zero( vd )) >> 3 ));

		#elif defined( _MSC_VER )

			unsigned long i;
			_BitScanForward64( &i, (unsigned __int64) vd );

			return( o + ( i >> 3 ));

		#else // defined( _MSC_VER )

			#if !LZAV_LITTLE_ENDIAN
				const uint64_t sw = vd >> 32 | vd << 32;
				const uint64_t sw2 =
					( sw & (uint64_t) 0xFFFF0000FFFF0000 ) >> 16 |
					( sw & (uint64_t) 0x0000FFFF0000FFFF ) << 16;
				vd = LZAV_COND_EC( vd,
					( sw2 & (uint64_t) 0xFF00FF00FF00FF00 ) >> 8 |
					( sw2 & (uint64_t) 0x00FF00FF00FF00FF ) << 8 );
			#endif // !LZAV_LITTLE_ENDIAN

			const uint64_t m = (uint64_t) 0x0101010101010101;

			return( o + (((( vd ^ ( vd - 1 )) & ( m - 1 )) * m ) >> 56 ));

		#endif // defined( _MSC_VER )
		}

		o2 += 8;
		o += 8;
	}

	// At most 7 bytes left.

	if LZAV_LIKELY( o + 3 < ml )
	{

#else // defined( LZAV_ARCH64 )

	size_t o2 = o + 3;

	while LZAV_LIKELY( o2 < ml )
	{

#endif // defined( LZAV_ARCH64 )

		uint32_t v1, v2, vd;
		memcpy( &v1, p1 + o, 4 );
		memcpy( &v2, p2 + o, 4 );
		vd = v1 ^ v2;

		if( vd != 0 )
		{
		#if defined( LZAV_GCC_BUILTINS )

			return( o + (size_t) ( LZAV_COND_EC(
				__builtin_ctz( vd ), __builtin_clz( vd )) >> 3 ));

		#elif defined( LZAV_CPP_BIT )

			return( o + (size_t) ( LZAV_COND_EC(
				std :: countr_zero( vd ), std :: countl_zero( vd )) >> 3 ));

		#elif defined( _MSC_VER )

			unsigned long i;
			_BitScanForward( &i, (unsigned long) vd );

			return( o + ( i >> 3 ));

		#else // defined( _MSC_VER )

			LZAV_IEC32( vd );
			const uint32_t m = 0x01010101;

			return( o + (((( vd ^ ( vd - 1 )) & ( m - 1 )) * m ) >> 24 ));

		#endif // defined( _MSC_VER )
		}

		o2 += 4;
		o += 4;
	}

	// At most 3 bytes left.

	if( o < ml )
	{
		if( p1[ o ] != p2[ o ])
		{
			return( o );
		}

		if( ++o < ml )
		{
			if( p1[ o ] != p2[ o ])
			{
				return( o );
			}

			if( ++o < ml )
			{
				if( p1[ o ] != p2[ o ])
				{
					return( o );
				}
			}
		}
	}

	return( ml );
}

/**
 * @brief Data match length finding function in the reverse direction.
 *
 * Note that the function assumes `p1[ -1 ] == p2[ -1 ]`.
 *
 * @param p1 The origin pointer to buffer 1.
 * @param p2 The origin pointer to buffer 2.
 * @param ml The maximal number of bytes to back-match. Cannot be 0.
 * @return The number of matching prior bytes, not including the origin
 * position.
 */

LZAV_INLINE_F size_t lzav_match_len_r1( const uint8_t* p1, const uint8_t* p2,
	const size_t ml ) LZAV_NOEX
{
	if( ml != 1 )
	{
		const uint8_t* const p1s = p1;
		const uint8_t* const p1e = p1 - ml + 1;
		p1--;

		while( p1 > p1e )
		{
			uint16_t v1, v2;
			memcpy( &v1, p1 - 2, 2 );
			memcpy( &v2, p2 - 3, 2 );

			const uint32_t vd = (uint32_t) ( v1 ^ v2 );

			if( vd != 0 )
			{
				return( (size_t) ( p1s - p1 +
					( LZAV_COND_EC( vd & 0xFF00, vd & 0x00FF ) == 0 )));
			}

			p1 -= 2;
			p2 -= 2;
		}

		if( p1 == p1e && p1[ -1 ] != p2[ -2 ])
		{
			return( (size_t) ( p1s - p1 ));
		}
	}

	return( ml );
}

/**
 * @brief Internal LZAV block header writing function (data format 3).
 *
 * This internal function writes a block to the output buffer. It can be used
 * in custom compression algorithms.
 *
 * Data format 3.
 *
 * A "raw" compressed data consists of any quantity of unnumbered "blocks".
 * A block starts with a header byte, followed by several optional bytes.
 * Bits 4-5 of the header specify the block's type.
 *
 * CC00LLLL: literal block (1-6 bytes). `LLLL` is the literal length.
 *
 * OO01RRRR: 10-bit offset block (2-7 bytes). `RRRR` is the reference length.
 *
 * OO10RRRR: 15-bit offset block (3-8 bytes). 3 offset carry bits.
 *
 * OO11RRRR: 21-bit offset block (4-9 bytes). 5 offset carry bits.
 *
 * If `LLLL` or `RRRR` equals 0, a value of 16 is assumed, and an additional
 * length byte follows. If, in a block, this additional byte's highest bit
 * is 1, one more length byte follows that defines the higher bits of the
 * length (up to 4 bytes). In a reference block, additional length bytes
 * follow the offset bytes. `CC` is a reference offset carry value (additional
 * 2 lowest bits of the offset of the next reference block). Block types 2 and
 * 3 include more carry bits (in the highest bits of the offset byte).
 *
 * Note that reference offsets can be much larger than the @ref LZAV_WIN_LEN
 * constant. This is due to offset carry bits, which create a dynamic LZ77
 * window instead of a fixed-length one. In practice, this encoding scheme
 * covers 99.5% of the offsets in the compressor's hash-table at any given
 * time.
 *
 * The overall compressed data are prefixed with a byte whose lower 4 bits
 * contain the minimal reference length (`mref`), and the highest 4 bits
 * contain the data format identifier. The compressed data always finish with
 * @ref LZAV_LIT_FIN literals. The lzav_write_fin_3() function should be used
 * to finalize compression.
 *
 * Except for the last block, a literal block is always followed by a
 * reference block.
 *
 * @param op Output buffer pointer.
 * @param lc Literal length, in bytes.
 * @param rc Reference length, in bytes, not less than `mref`.
 * @param d Reference offset, in bytes. Should not be less than
 * @ref LZAV_OFS_MIN; it may be less than `rc`, permitting overlapped copying.
 * @param ipa Literals anchor pointer.
 * @param cbpp Pointer to the pointer to the latest offset carry block header.
 * Cannot be 0, but the contained pointer can be 0 (the initial value).
 * @param cshp Pointer to the offset carry shift.
 * @param mref Minimal reference length, in bytes, used by the compression
 * algorithm.
 * @return Incremented output buffer pointer.
 */

LZAV_INLINE_F uint8_t* lzav_write_blk_3( uint8_t* LZAV_RESTRICT op,
	const size_t lc, size_t rc, size_t d,
	const uint8_t* LZAV_RESTRICT const ipa, uint8_t** const cbpp,
	int* const cshp, const size_t mref ) LZAV_NOEX
{
	// Perform offset carry to a previous block (`csh` may be zero).

	const int csh = *cshp;
	rc = rc + 1 - mref;
	const size_t dc = ( d << 8 ) >> csh;
	d >>= csh;
	**cbpp |= (uint8_t) dc;

	if LZAV_UNLIKELY( lc != 0 )
	{
		// Write a literal block.

		const size_t cv = d << 6; // Offset carry value in the literal block.
		d >>= 2;

		if LZAV_LIKELY( lc < 9 )
		{
			*op = (uint8_t) ( cv | lc );

			memcpy( op + 1, ipa, 8 );
			op += lc + 1;
		}
		else
		if LZAV_LIKELY( lc < 16 )
		{
			*op = (uint8_t) ( cv | lc );

			memcpy( op + 1, ipa, 16 );
			op += lc + 1;
		}
		else
		if( lc < 33 )
		{
			const uint16_t ov = (uint16_t) LZAV_COND_EC(
				( lc - 16 ) << 8 | ( cv & 0xFF ), cv << 8 | ( lc - 16 ));

			memcpy( op, &ov, 2 );

			memcpy( op + 2, ipa, 16 );
			memcpy( op + 18, ipa + 16, 16 );
			op += lc + 2;
		}
		else
		{
			op[ 0 ] = (uint8_t) cv;

			size_t lcw = lc - 16;

			while( lcw > 127 )
			{
				op[ 1 ] = (uint8_t) ( 0x80 | lcw );
				lcw >>= 7;
				op++;
			}

			op[ 1 ] = (uint8_t) lcw;
			op += 2;

			memcpy( op, ipa, lc );
			op += lc;
		}
	}

	// Write a reference block.

	static const int ocsh[ 4 ] = { 0, 0, 3, 5 };
	const size_t bt = (size_t) 1 + ( d > LZAV_OFS_TH1 ) + ( d > LZAV_OFS_TH2 );

	uint32_t ov = (uint32_t) ( d << 6 | bt << 4 );
	uint8_t* opbt = op + bt;
	*cshp = ocsh[ bt ];
	*cbpp = opbt;

	if LZAV_LIKELY( rc < 16 )
	{
		ov |= (uint32_t) rc;

		LZAV_IEC32( ov );
		memcpy( op, &ov, 4 );

		return( opbt + 1 );
	}

	LZAV_IEC32( ov );
	rc -= 16;
	memcpy( op, &ov, 4 );

	if LZAV_LIKELY( rc < 128 )
	{
		opbt[ 1 ] = (uint8_t) rc;
		return( opbt + 2 );
	}

	do
	{
		opbt[ 1 ] = (uint8_t) ( 0x80 | rc );
		rc >>= 7;
		opbt++;
	} while( rc > 127 );

	opbt[ 1 ] = (uint8_t) rc;
	return( opbt + 2 );
}

/**
 * @brief Internal LZAV finishing function (data format 3).
 *
 * This internal function writes finishing literal block(s) to the output
 * buffer. It can be used in custom compression algorithms.
 *
 * Data format 3.
 *
 * @param op Output buffer pointer.
 * @param lc Literal length, in bytes. Not less than @ref LZAV_LIT_FIN.
 * @param ipa Literals anchor pointer.
 * @return Incremented output buffer pointer.
 */

LZAV_INLINE_F uint8_t* lzav_write_fin_3( uint8_t* LZAV_RESTRICT op,
	const size_t lc, const uint8_t* LZAV_RESTRICT const ipa ) LZAV_NOEX
{
	size_t lcw = lc;

	if( lc > 15 )
	{
		*op = 0;
		op++;

		lcw -= 16;

		while( lcw > 127 )
		{
			*op = (uint8_t) ( 0x80 | lcw );
			lcw >>= 7;
			op++;
		}
	}

	*op = (uint8_t) lcw;
	op++;

	memcpy( op, ipa, lc );
	return( op + lc );
}

/**
 * @brief Function returns the buffer size required for the minimal reference
 * length of 5.
 *
 * @param srclen The length of the source data to be compressed.
 * @return The required allocation size for the destination compression
 * buffer. Always a positive value.
 */

LZAV_INLINE_F int lzav_compress_bound_mref5( const int srclen ) LZAV_NOEX
{
	if( srclen <= 0 )
	{
		return( 16 );
	}

	const int l2 = srclen / ( 16 + 5 );

	return(( srclen - l2 * 5 + 15 ) / 16 * 2 - l2 + srclen + 16 );
}

/**
 * @brief Function returns the buffer size required for the minimal reference
 * length of 6.
 *
 * @param srclen The length of the source data to be compressed.
 * @return The required allocation size for the destination compression
 * buffer. Always a positive value.
 */

LZAV_INLINE_F int lzav_compress_bound_mref6( const int srclen ) LZAV_NOEX
{
	if( srclen <= 0 )
	{
		return( 16 );
	}

	const int k = 16 + 127 + 1;
	const int l2 = srclen / ( k + 6 );

	return(( srclen - l2 * 6 + k - 1 ) / k * 2 - l2 + srclen + 16 );
}

/**
 * @brief Function returns the buffer size required for LZAV compression.
 *
 * @param srclen The length of the source data to be compressed.
 * @return The required allocation size for the destination compression
 * buffer. Always a positive value.
 */

LZAV_INLINE_F int lzav_compress_bound( const int srclen ) LZAV_NOEX
{
	if( srclen < LZAV_MR5_THR )
	{
		return( lzav_compress_bound_mref5( srclen ));
	}
	else
	{
		return( lzav_compress_bound_mref6( srclen ));
	}
}

/**
 * @brief Function returns the buffer size required for the higher-ratio LZAV
 * compression.
 *
 * @param srclen The length of the source data to be compressed.
 * @return The required allocation size for the destination compression
 * buffer. Always a positive value.
 */

LZAV_INLINE_F int lzav_compress_bound_hi( const int srclen ) LZAV_NOEX
{
	return( lzav_compress_bound_mref5( srclen ));
}

/**
 * @brief Hash-table initialization function.
 *
 * This function initializes the hash-table by replicating the contents of the
 * specified tuple value. 
 *
 * @param[out] ht Hash-table pointer.
 * @param htsize Hash-table size. The size should be a power of 2 value, not
 * less than 64 bytes.
 * @param[in] initv Pointer to an initialized 8-byte tuple.
 */

LZAV_INLINE_F void lzav_ht_init( uint8_t* LZAV_RESTRICT const ht,
	const size_t htsize, const uint32_t* LZAV_RESTRICT const initv ) LZAV_NOEX
{
	memcpy( ht, initv, 8 );
	memcpy( ht + 8, initv, 8 );
	memcpy( ht + 16, ht, 16 );
	memcpy( ht + 32, ht, 16 );
	memcpy( ht + 48, ht, 16 );

	uint8_t* LZAV_RESTRICT const hte = ht + htsize;
	uint8_t* LZAV_RESTRICT htc = ht + 64;

	while LZAV_LIKELY( htc != hte )
	{
		memcpy( htc, ht, 16 );
		memcpy( htc + 16, ht + 16, 16 );
		memcpy( htc + 32, ht + 32, 16 );
		memcpy( htc + 48, ht + 48, 16 );
		htc += 64;
	}
}

/**
 * @brief Calculates a hash value for the specified input words.
 *
 * @param iw1 Input word 1.
 * @param iw2 Input word 2.
 * @param sh Hash value shift, in bits. Should be chosen so that `32-sh` is
 * equal to hash-table's log2 size.
 * @param hmask Hash value mask.
 * @return Masked hash value.
 */

LZAV_INLINE_F uint32_t lzav_hash( const uint32_t iw1, const uint32_t iw2,
	const int sh, const uint32_t hmask ) LZAV_NOEX
{
	uint32_t Seed1 = 0x243F6A88;
	uint32_t hval = 0x85A308D3;

	Seed1 ^= iw1;
	hval ^= iw2;
	hval *= Seed1;
	hval >>= sh;

	return( hval & hmask );
}

/**
 * @brief Loads a secondary input word.
 *
 * The function loads a 16- or 8-bit value, depending on the `mref` parameter.
 * This function relies heavily on forced code inlining for performance.
 * Endianness correction is not applied.
 *
 * @param[out] ov Pointer to the variable that receives the loaded value.
 * @param ip Input pointer.
 * @param mref Minimal reference length, in bytes. Only the values 5 and 6 are
 * supported.
 */

LZAV_INLINE_F void lzav_load_w2( uint16_t* LZAV_RESTRICT const ov,
	const uint8_t* LZAV_RESTRICT const ip, const size_t mref ) LZAV_NOEX
{
	if( mref == 5 )
	{
		*ov = *ip;
	}
	else
	{
		memcpy( ov, ip, 2 );
	}
}

/**
 * @brief LZAV compression function with an external buffer option.
 *
 * The function performs in-memory data compression using the LZAV compression
 * algorithm and data format. The function produces "raw" compressed data
 * without a header containing the data length, identifier, or checksum.
 *
 * The function relies on forced code inlining meaning its multiple calls
 * throughout the code may increase the code size considerably. It is
 * suggested to wrap the call to this function in a non-inlined function.
 *
 * Note that the compression algorithm and its output on the same source data
 * may differ between LZAV versions, and may differ between big- and
 * little-endian systems. However, decompression of compressed data produced
 * by any prior compressor version will remain possible.
 *
 * @param[in] src Source (uncompressed) data pointer, can be 0 if `srclen`
 * equals 0. Address alignment is unimportant.
 * @param[out] dst Destination (compressed data) buffer pointer. The allocated
 * size should be at least lzav_compress_bound() bytes. Address alignment is
 * unimportant. Should be different from `src`.
 * @param srclen Source data length, in bytes, can be 0: in this case, the
 * compressed length is assumed to be 0 as well.
 * @param dstlen Destination buffer's capacity, in bytes.
 * @param extbuf External buffer to use for the hash-table; set to null for
 * the function to manage memory itself (via the standard `malloc`). Supplying
 * a pre-allocated buffer is useful if compression is performed often during
 * an application's operation: this reduces memory allocation overhead and
 * fragmentation. Note that the access to the supplied buffer is not
 * implicitly thread-safe. Buffer's address must be aligned to 4 bytes.
 * @param extbuflen The capacity of the `extbuf`, in bytes; should be a
 * power-of-2 value. Used as the hash-table size if `extbuf` is null.
 * The capacity should not be less than `4*srclen`, and for the default
 * compression ratio should not be greater than 1 MiB. The same `extbuflen`
 * value can be used for any smaller source data. Using smaller `extbuflen`
 * values reduces the compression ratio and, at the same time, increases the
 * compression speed. This aspect can be utilized on memory-constrained and
 * low-performance processors.
 * @param mref The minimal back-reference length, in bytes. Only 5 and 6
 * values are supported.
 * @return Length of the compressed data, in bytes. Returns 0 if `srclen` is
 * less than or equal to 0, if `dstlen` is too small, if buffer pointers are
 * invalid, or if there is not enough memory.
 */

LZAV_INLINE_F int lzav_compress( const void* const src, void* const dst,
	const int srclen, const int dstlen, void* const extbuf,
	const int extbuflen, const size_t mref ) LZAV_NOEX
{
	if(( srclen <= 0 ) | ( src == LZAV_NULL ) | ( dst == LZAV_NULL ) |
		( src == dst ) | (( mref != 5 ) & ( mref != 6 )))
	{
		return( 0 );
	}

	if(( mref == 5 && dstlen < lzav_compress_bound_mref5( srclen )) ||
		( mref == 6 && dstlen < lzav_compress_bound_mref6( srclen )))
	{
		return( 0 );
	}

	uint8_t* op = (uint8_t*) dst; // Destination (compressed data) pointer.
	*op = (uint8_t) ( LZAV_FMT_CUR << 4 | mref ); // Write prefix byte.
	op++;

	if( srclen < 16 )
	{
		// Handle a very short source data.

		*op = (uint8_t) srclen;
		op++;

		memcpy( op, src, (size_t) srclen );

		if( srclen > LZAV_LIT_FIN - 1 )
		{
			return( 2 + srclen );
		}

		memset( op + srclen, 0, (size_t) ( LZAV_LIT_FIN - srclen ));
		return( 2 + LZAV_LIT_FIN );
	}

	uint32_t stack_buf[ 2048 ]; // On-stack hash-table.
	uint32_t* alloc_buf = LZAV_NULL; // Hash-table allocated on heap.

	uint8_t* LZAV_RESTRICT ht =
		(uint8_t*) stack_buf; // The actual hash-table pointer.

	size_t htsize; // Hash-table's size in bytes (power-of-2).
	htsize = ( 1 << 7 ) * sizeof( uint32_t ) * 4;

	size_t htsizem; // Maximal hash-table size.

	if( extbuf == LZAV_NULL )
	{
		htsizem = ( extbuflen > 0 ? (size_t) extbuflen : 1 << 20 );
	}
	else
	{
		htsizem = ( extbuflen > (int) sizeof( stack_buf ) ?
			(size_t) extbuflen : sizeof( stack_buf ));
	}

	while( htsize < ( (size_t) srclen << 2 ))
	{
		const size_t htsize2 = htsize << 1;

		if( htsize2 > htsizem )
		{
			break;
		}

		htsize = htsize2;
	}

	if( htsize > sizeof( stack_buf ))
	{
		if( extbuf == LZAV_NULL )
		{
			alloc_buf = LZAV_MALLOC( htsize, uint32_t );

			if( alloc_buf == LZAV_NULL )
			{
				return( 0 );
			}

			ht = (uint8_t*) alloc_buf;
		}
		else
		{
			ht = (uint8_t*) extbuf;
		}
	}

	const uint32_t hmask = (uint32_t) (( htsize - 1 ) ^ 15 ); // Hash mask.
	const uint8_t* ip = (const uint8_t*) src; // Source data pointer.
	const uint8_t* const ipe = ip + srclen - LZAV_LIT_FIN; // End pointer.
	const uint8_t* const ipet = ipe - 15 + LZAV_LIT_FIN; // Hashing threshold,
		// avoids I/O OOB.
	const uint8_t* ipa = ip; // Literals anchor pointer.

	// Initialize the hash-table. Each hash-table item consists of 2 tuples
	// (4 initial match bytes; 32-bit source data offset). Start at offset 1
	// for non-zero back-match length.

	uint32_t initv[ 2 ] = { 0, 1 };
	ip++;
	memcpy( initv, ip, 4 );

	lzav_ht_init( ht, htsize, initv );

	uint8_t* cbp = op; // Pointer to the latest offset carry block header.
	int csh = 0; // Offset carry shift.

	size_t mavg = 100 << 17; // Running average of hash match rate (*2^11).
		// Two-factor average of success (64) multiplied by reference length.

	while LZAV_LIKELY( ip < ipet )
	{
		// Hash source data (endianness is minimally important for compression
		// efficiency).

		uint32_t iw1;
		uint16_t iw2, ww2;
		memcpy( &iw1, ip, 4 );
		lzav_load_w2( &iw2, ip + 4, mref );

		// Hash-table access.

		uint32_t* LZAV_RESTRICT hp = (uint32_t*) ( ht + lzav_hash( iw1, iw2,
			12, hmask ));

		uint32_t ipo = (uint32_t) ( ip - (const uint8_t*) src );
		const uint32_t hw1 = hp[ 0 ]; // Tuple 1's match word.

		size_t wpo; // At window offset.
		const uint8_t* wp; // At window pointer.
		const uint8_t* ip0; // `ip` save variable.
		size_t d, ml, rc, lc;

		// Find source data in hash-table tuples.

		if LZAV_LIKELY( iw1 != hw1 )
		{
			if LZAV_LIKELY( iw1 != hp[ 2 ])
			{
				goto _no_match;
			}

			wpo = hp[ 3 ];
			lzav_load_w2( &ww2, (const uint8_t*) src + wpo + 4, mref );

			if LZAV_UNLIKELY( iw2 != ww2 )
			{
				goto _no_match;
			}
		}
		else
		{
			wpo = hp[ 1 ];
			lzav_load_w2( &ww2, (const uint8_t*) src + wpo + 4, mref );

			if LZAV_UNLIKELY( iw2 != ww2 )
			{
				if( iw1 != hp[ 2 ])
				{
					goto _no_match;
				}

				wpo = hp[ 3 ];
				lzav_load_w2( &ww2, (const uint8_t*) src + wpo + 4, mref );

				if( iw2 != ww2 )
				{
					goto _no_match;
				}
			}
		}

		// Source data and hash-table entry matched.

		d = (size_t) ipo - wpo; // Reference offset (distance).
		ml = (size_t) ( ipe - ip ); // Max reference match length. Make sure
			// `LZAV_LIT_FIN` literals remain on finish.

		if LZAV_UNLIKELY( d < LZAV_OFS_MIN )
		{
			// Small offsets may be inefficient.

			goto _d_oob;
		}

		LZAV_PREFETCH( ip - 2 );

		if LZAV_LIKELY( d > 31 )
		{
			if LZAV_LIKELY( iw1 == hw1 ) // Replace tuple, or insert.
			{
				hp[ 1 ] = ipo;
			}
			else
			{
				hp[ 2 ] = hw1;
				hp[ 3 ] = hp[ 1 ];
				hp[ 0 ] = iw1;
				hp[ 1 ] = ipo;
			}
		}

		wp = (const uint8_t*) src + wpo;
		rc = lzav_match_len( ip, wp, ml, mref );

		ip0 = ip;
		lc = (size_t) ( ip - ipa );

		if LZAV_UNLIKELY( lc != 0 && ip[ -1 ] == wp[ -1 ])
		{
			// Try to consume literals by finding a match at a back-position.

			ml = lzav_match_len_r1( ip, wp, ( lc < wpo ? lc : wpo ));
			lc -= ml;
			rc += ml;
			ip -= ml;
		}

		if LZAV_LIKELY( d < (size_t) LZAV_WIN_LEN << csh << (( lc != 0 ) * 2 ))
		{
			// Update hash-table with 1 skipped position.

			memcpy( &iw1, ip0 + 2, 4 );
			lzav_load_w2( &iw2, ip0 + 6, mref );

			hp = (uint32_t*) ( ht + lzav_hash( iw1, iw2, 12, hmask ));
			ipo += 2;
			mavg -= mavg >> 10;

			hp[ 2 ] = hp[ 0 ];
			hp[ 3 ] = hp[ 1 ];

			ip += rc;
			wp = ipa;

			hp[ 0 ] = iw1;
			hp[ 1 ] = ipo;

			ipa = ip;
			mavg += rc << 7;

			op = lzav_write_blk_3( op, lc, rc, d, wp, &cbp, &csh, mref );
			continue;
		}

		ip = ip0;

	_d_oob:
		ip++;
		continue;

	_no_match:
		wp = ip;
		hp[ 2 ] = iw1;

		mavg -= mavg >> 11;
		ip++;

		hp[ 3 ] = ipo;

		if( mavg < ( 200 << 10 ) && wp != ipa ) // Speed-up threshold.
		{
			// Advance faster on data which is harder to compress.

			ip += 1 + ( ipo & 1 ); // Simple dithering.

			if LZAV_UNLIKELY( mavg < ( 130 << 10 ))
			{
				ip++;

				if LZAV_UNLIKELY( mavg < ( 100 << 10 ))
				{
					ip += (size_t) 100 - ( mavg >> 10 ); // Gradually faster.
				}
			}
		}
	}

	op = lzav_write_fin_3( op, (size_t) ( ipe - ipa + LZAV_LIT_FIN ), ipa );

	LZAV_FREE( alloc_buf );

	return( (int) ( op - (uint8_t*) dst ));
}

/**
 * @brief Wrapper function for lzav_compress() with `mref` equal to 5.
 *
 * See the lzav_compress() function for a more detailed description.
 *
 * Note that the lzav_compress_bound_mref5() function should be used to obtain
 * the bound size of the `dst` buffer.
 *
 * @param[in] src Source (uncompressed) data pointer.
 * @param[out] dst Destination (compressed data) buffer pointer. The allocated
 * size should be at least lzav_compress_bound() bytes large.
 * @param srclen Source data length, in bytes.
 * @param dstlen Destination buffer's capacity, in bytes.
 * @return The length of the compressed data, in bytes.
 * @param extbuf External buffer to use for the hash-table.
 * @param extbuflen The capacity of the `extbuf`, in bytes.
 * @return Length of the compressed data, in bytes.
 */

LZAV_NO_INLINE int lzav_compress_mref5( const void* const src,
	void* const dst, const int srclen, const int dstlen, void* const extbuf,
	const int extbuflen ) LZAV_NOEX
{
	return( lzav_compress( src, dst, srclen, dstlen, extbuf, extbuflen, 5 ));
}

/**
 * @brief Wrapper function for lzav_compress() with `mref` equal to 6.
 *
 * See the lzav_compress() function for a more detailed description.
 *
 * Note that the lzav_compress_bound_mref6() function should be used to obtain
 * the bound size of the `dst` buffer.
 *
 * @param[in] src Source (uncompressed) data pointer.
 * @param[out] dst Destination (compressed data) buffer pointer. The allocated
 * size should be at least lzav_compress_bound() bytes large.
 * @param srclen Source data length, in bytes.
 * @param dstlen Destination buffer's capacity, in bytes.
 * @return The length of the compressed data, in bytes.
 * @param extbuf External buffer to use for the hash-table.
 * @param extbuflen The capacity of the `extbuf`, in bytes.
 * @return Length of the compressed data, in bytes.
 */

LZAV_NO_INLINE int lzav_compress_mref6( const void* const src,
	void* const dst, const int srclen, const int dstlen, void* const extbuf,
	const int extbuflen ) LZAV_NOEX
{
	return( lzav_compress( src, dst, srclen, dstlen, extbuf, extbuflen, 6 ));
}

/**
 * @brief Default LZAV compression function.
 *
 * The function performs in-memory data compression using the LZAV compression
 * algorithm, with the default settings.
 *
 * See the lzav_compress() function for a more detailed description.
 *
 * @param[in] src Source (uncompressed) data pointer.
 * @param[out] dst Destination (compressed data) buffer pointer. The allocated
 * size should be at least lzav_compress_bound() bytes large.
 * @param srclen Source data length, in bytes.
 * @param dstlen Destination buffer's capacity, in bytes.
 * @return Length of the compressed data, in bytes. Returns 0 if `srclen` is
 * less than or equal to 0, if `dstlen` is too small, or if there is not
 * enough memory.
 */

LZAV_INLINE_F int lzav_compress_default( const void* const src,
	void* const dst, const int srclen, const int dstlen ) LZAV_NOEX
{
	if( srclen < LZAV_MR5_THR )
	{
		return( lzav_compress_mref5( src, dst, srclen, dstlen,
			LZAV_NULL, 0 ));
	}
	else
	{
		return( lzav_compress_mref6( src, dst, srclen, dstlen,
			LZAV_NULL, 0 ));
	}
}

/**
 * @brief Calculates the estimated LZAV block's size.
 *
 * @param lc Literal count, in bytes.
 * @param d Reference offset.
 * @param csh Carry shift bit count.
 * @return Estimated block size.
 */

LZAV_INLINE_F size_t lzav_est_blksize( const size_t lc, size_t d,
	const int csh ) LZAV_NOEX
{
	const int lb = ( lc != 0 );
	d >>= csh;
	d >>= lb * 2;

	return( lc + (size_t) lb + ( lc > 15 ) + 2 +
		( d > LZAV_OFS_TH1 ) + ( d > LZAV_OFS_TH2 ));
}

/**
 * @brief Inserts a tuple into a hash-table item.
 *
 * @param hp Pointer to the hash-table item.
 * @param ti0 Offset of tuple 0.
 * @param mti Maximal tuple offset.
 * @param iw1 Initial source bytes.
 * @param ipo Source bytes offset.
 */

LZAV_INLINE_F void lzav_ht_insert( uint32_t* const hp, size_t ti0,
	const size_t mti, const uint32_t iw1, const uint32_t ipo ) LZAV_NOEX
{
	ti0 = ( ti0 == 0 ? mti : ti0 - 2 );
	hp[ ti0 ] = iw1;
	hp[ ti0 + 1 ] = ipo;
	hp[ mti + 3 ] = (uint32_t) ti0;
}

/**
 * @brief Higher-ratio LZAV compression function (much slower).
 *
 * The function performs in-memory data compression using the higher-ratio
 * LZAV compression algorithm.
 *
 * @param[in] src Source (uncompressed) data pointer.
 * @param[out] dst Destination (compressed data) buffer pointer. The allocated
 * size should be at least lzav_compress_bound_hi() bytes.
 * @param srclen Source data length, in bytes.
 * @param dstlen Destination buffer's capacity, in bytes.
 * @return Length of the compressed data, in bytes. Returns 0 if `srclen` is
 * less than or equal to 0, if `dstlen` is too small, if buffer pointers are
 * invalid, or if there is not enough memory.
 */

LZAV_NO_INLINE int lzav_compress_hi( const void* const src, void* const dst,
	const int srclen, const int dstlen ) LZAV_NOEX
{
	if(( srclen <= 0 ) | ( src == LZAV_NULL ) | ( dst == LZAV_NULL ) |
		( src == dst ) | ( dstlen < lzav_compress_bound_hi( srclen )))
	{
		return( 0 );
	}

	const size_t mref = 5; // Minimal reference length, in bytes.

	uint8_t* op = (uint8_t*) dst; // Destination (compressed data) pointer.
	*op = (uint8_t) ( LZAV_FMT_CUR << 4 | mref ); // Write prefix byte.
	op++;

	if( srclen < 16 )
	{
		// Handle a very short source data.

		*op = (uint8_t) srclen;
		op++;

		memcpy( op, src, (size_t) srclen );

		if( srclen > LZAV_LIT_FIN - 1 )
		{
			return( 2 + srclen );
		}

		memset( op + srclen, 0, (size_t) ( LZAV_LIT_FIN - srclen ));
		return( 2 + LZAV_LIT_FIN );
	}

	size_t htsize; // Hash-table's size in bytes (power-of-2).
	htsize = ( 1 << 7 ) * sizeof( uint32_t ) * 2 * 8;

	while( htsize != ( 1 << 23 ) && htsize < ( (size_t) srclen << 2 ))
	{
		htsize <<= 1;
	}

	uint32_t* const alloc_buf =
		LZAV_MALLOC( htsize, uint32_t ); // Hash-table allocated on heap.

	if( alloc_buf == LZAV_NULL )
	{
		return( 0 );
	}

	uint8_t* LZAV_RESTRICT const ht = (uint8_t*) alloc_buf; // Hash-table ptr.

	const size_t mti = 12; // Maximal tuple offset, inclusive.
	const uint32_t hmask = (uint32_t) (( htsize - 1 ) ^ 63 ); // Hash mask.
	const uint8_t* ip = (const uint8_t*) src; // Source data pointer.
	const uint8_t* const ipe = ip + srclen - LZAV_LIT_FIN; // End pointer.
	const uint8_t* const ipet = ipe - 15 + LZAV_LIT_FIN; // Hashing threshold,
		// avoids I/O OOB.
	const uint8_t* ipa = ip; // Literals anchor pointer.

	// Initialize the hash-table. Each hash-table item consists of 8 tuples
	// (4 initial match bytes; 32-bit source data offset). The last value of
	// the last tuple is used as head tuple offset (an even value). Start at
	// offset 2 for non-zero back-match length.

	uint32_t initv[ 2 ] = { 0, 2 };
	ip += 2;
	memcpy( initv, ip, 4 );

	lzav_ht_init( ht, htsize, initv );

	uint8_t* cbp = op; // Pointer to the latest offset carry block header.
	int csh = 0; // Offset carry shift.

	size_t prc = 0; // Length of a previously found match.
	size_t pd = 0; // Distance of a previously found match.
	const uint8_t* pip = ip; // Source pointer of a previously found match.

	while LZAV_LIKELY( ip < ipet )
	{
		// Hash source data (endianness is minimally important for compression
		// efficiency).

		uint32_t iw1;
		memcpy( &iw1, ip, 4 );

		// Hash-table access.

		uint32_t* LZAV_RESTRICT hp = (uint32_t*) ( ht +
			lzav_hash( iw1, ip[ 4 ], 8, hmask ));

		const uint32_t ipo = (uint32_t) ( ip - (const uint8_t*) src );
		size_t ti0 = hp[ mti + 3 ]; // Head tuple offset.

		// Find source data in hash-table tuples, in up to 7 previous
		// positions.

		const size_t mle = (size_t) ( ipe - ip ); // Match length bound.
		size_t rc = 1; // Best found match length-4, 1 - not found.
		size_t d = 0; // Best found reference offset (distance).
		size_t ti = ti0;
		int i;

		// Match-finder.

		for( i = 0; i < 7; i++ )
		{
			const uint8_t* const wp0 = (const uint8_t*) src + hp[ ti + 1 ];
			const uint32_t ww1 = hp[ ti ];
			const size_t d0 = (size_t) ( ip - wp0 );
			ti = ( ti == mti ? 0 : ti + 2 );

			if( iw1 == ww1 )
			{
				// Disallow reference copy overlap by using `d0` as max
				// match length. Make sure `LZAV_LIT_FIN` literals remain
				// on finish.

				const size_t ml = lzav_match_len( ip, wp0,
					( d0 > mle ? mle : d0 ), 4 );

				if( ml > rc )
				{
					d = d0;
					rc = ml;
				}
			}
		}

		if LZAV_LIKELY( d != rc )
		{
			// Update hash-table entry, if there was no match, or if the match
			// is not an adjacent replication.

			lzav_ht_insert( hp, ti0, mti, iw1, ipo );
		}

		if(( rc < mref + ( d > ( 1 << 18 )) + ( d > ( 1 << 22 ))) |
			( d < LZAV_OFS_MIN ))
		{
			ip++;
			continue;
		}

		// Source data and hash-table entry match of suitable length.

		const uint8_t* const wp = ip - d;
		const uint8_t* const ip1 = ip + 1;
		size_t lc = (size_t) ( ip - ipa );

		if LZAV_UNLIKELY( lc != 0 && ip[ -1 ] == wp[ -1 ])
		{
			// Try to consume literals by finding a match at back-position.

			size_t ml = (size_t) ( wp - (const uint8_t*) src );

			if LZAV_LIKELY( ml > lc )
			{
				ml = lc;
			}

			ml = lzav_match_len_r1( ip, wp, ml );
			lc -= ml;
			rc += ml;
			ip -= ml;
		}

		if LZAV_UNLIKELY( d >= (size_t) LZAV_WIN_LEN << csh <<
			(( lc != 0 ) * 2 ))
		{
			goto _d_oob;
		}

		if( prc == 0 )
		{
			// Save match for a later comparison.

		_save_match:
			prc = rc;
			pd = d;
			pip = ip;
			ip = ip1;
			continue;

		_d_oob:
			// `d` is out of bounds.

			ip = ip1;

			if LZAV_LIKELY( d != rc )
			{
				continue;
			}

			lzav_ht_insert( hp, ti0, mti, iw1, ipo );
			continue;
		}

		// Block size overhead estimation, and comparison with a previously
		// found match.

		const size_t plc = (size_t) ( pip - ipa );
		const size_t ov = lzav_est_blksize( lc, d, csh );
		const size_t pov = lzav_est_blksize( plc, pd, csh );

		if LZAV_LIKELY( prc * ov > rc * pov )
		{
			op = lzav_write_blk_3( op, plc, prc, pd, ipa, &cbp, &csh, mref );

			ipa = pip + prc;

			if LZAV_LIKELY( ipa > ip || d >= (size_t) LZAV_WIN_LEN << csh <<
				(( ip - ipa != 0 ) * 2 ))
			{
				prc = 0;
				ip = ( ipa > ip1 ? ipa : ip1 );
				continue;
			}

			// A winning previous match does not overlap a current match.

			goto _save_match;
		}

		op = lzav_write_blk_3( op, lc, rc, d, ipa, &cbp, &csh, mref );

		// Update hash-table with 1 skipped position.

		memcpy( &iw1, ip + 4, 4 );
		hp = (uint32_t*) ( ht + lzav_hash( iw1, ip[ 8 ], 8, hmask ));

		lzav_ht_insert( hp, hp[ mti + 3 ], mti, iw1,
			(uint32_t) ( ip + 4 - (const uint8_t*) src ));

		ip += rc;
		prc = 0;
		ipa = ip;
	}

	if( prc != 0 )
	{
		op = lzav_write_blk_3( op, (size_t) ( pip - ipa ), prc, pd, ipa, &cbp,
			&csh, mref );

		ipa = pip + prc;
	}

	op = lzav_write_fin_3( op, (size_t) ( ipe - ipa + LZAV_LIT_FIN ), ipa );

	LZAV_FREE( alloc_buf );

	return( (int) ( op - (uint8_t*) dst ));
}

/**
 * @def LZAV_LOAD32( a )
 * @brief Defines `bv` and loads a 32-bit unsigned value from memory, with
 * endianness correction.
 *
 * @param a Memory address.
 */

/**
 * @def LZAV_SET_IPD_CV( x, v, sh )
 * @brief Defines `ipd` as a pointer to the back-reference, checks bounds,
 * and updates the carry bit variables.
 *
 * @param x Reference offset.
 * @param v Next `cv` value.
 * @param sh Next `csh` value.
 */

/**
 * @brief Internal LZAV decompression function (data format 3).
 *
 * The function decompresses "raw" data previously compressed into the LZAV
 * data format 3.
 *
 * This function should not be called directly, since it does not check the
 * format identifier.
 *
 * @param[in] src Source (compressed) data pointer.
 * @param[out] dst Destination (decompressed data) buffer pointer.
 * @param srclen Source data length, in bytes.
 * @param dstlen Expected destination data length, in bytes.
 * @param[out] pwl Pointer to a variable that receives the number of bytes
 * written to the destination buffer (until error or end of buffer).
 * @return Length of the decompressed data, in bytes, or any negative value if
 * an error occurred.
 */

LZAV_NO_INLINE int lzav_decompress_3( const void* const src, void* const dst,
	const int srclen, const int dstlen, int* const pwl ) LZAV_NOEX
{
	if LZAV_UNLIKELY( srclen < 10 )
	{
		*pwl = 0;
		return( LZAV_E_SRCOOB );
	}

	const uint8_t* LZAV_RESTRICT ip =
		(const uint8_t*) src; // Compressed data pointer.

	const uint8_t* const ipe = ip + srclen; // Compressed data boundary ptr.
	const uint8_t* const ipet = ipe - 9; // Block header read threshold.
	uint8_t* op = (uint8_t*) dst; // Destination (decompressed data) pointer.
	uint8_t* const ope = op + dstlen; // Destination boundary pointer.
	uint8_t* opet = ope - 63; // Threshold for fast copy to destination.
	const size_t mref1 = (size_t) ( *ip & 15 ) - 1; // Minimal ref length - 1.

	if LZAV_UNLIKELY( mref1 > 5 )
	{
		*pwl = 0;
		return( LZAV_E_UNKFMT );
	}

	*pwl = dstlen;
	size_t bh; // Current block header, updated in each branch.
	size_t cv = 0; // Reference offset carry value.
	int csh = 0; // Reference offset carry shift.

	#define LZAV_LOAD32( a ) \
		uint32_t bv; \
		memcpy( &bv, a, 4 ); \
		LZAV_IEC32( bv )

	#define LZAV_SET_IPD_CV( x, v, sh ) \
		const size_t d = ( x ) << csh | cv; \
		const size_t md = (size_t) ( op - (uint8_t*) dst ); \
		csh = ( sh ); \
		cv = ( v ); \
		ipd = op - d; \
		if LZAV_UNLIKELY( d > md ) \
			goto _err_refoob

	if LZAV_UNLIKELY(( ope < opet ) | ( ipe < ipet - 16 ) |
		( ipe < ipet - 64 ))
	{
		opet = LZAV_NULL;
	}

	ip++; // Advance beyond prefix byte.

	bh = *ip;

	while LZAV_LIKELY( ip < ipet )
	{
		size_t cc; // Byte copy count.

		if LZAV_LIKELY(( bh & 0x30 ) != 0 ) // Block type != 0.
		{
			const uint8_t* ipd; // Source data pointer.
			size_t bt; // Block type.

		_refblk:
			ip++;
			bt = ( bh >> 4 ) & 3;

			LZAV_LOAD32( ip );

			static const size_t om[ 4 ] = { 0, 0x3FF, 0x7FFF, 0x1FFFFF };
			static const size_t cvm[ 4 ] = { 0, 0, 7, 31 };
			static const int ocsh[ 4 ] = { 0, 0, 3, 5 };

			const int bt8 = (int) ( bt << 3 );
			const int ncsh = ocsh[ bt ];

			LZAV_SET_IPD_CV(( bh >> 6 | bv << 2 ) & om[ bt ],
				( bv >> ( bt8 - ncsh )) & cvm[ bt ], ncsh );

			ip += bt;
			bv >>= bt8;

			LZAV_PREFETCH( ipd );

			uint8_t* opcc = op + mref1;
			cc = bh & 15;

			if LZAV_LIKELY( cc != 0 ) // True, if no additional length byte.
			{
				opcc += cc;
				bh = bv & 0xFF;

				if LZAV_LIKELY( op < opet )
				{
					if LZAV_LIKELY( d > 15 )
					{
						memcpy( op, ipd, 16 );
						memcpy( op + 16, ipd + 16, 4 );
						op = opcc;
						continue;
					}

					if LZAV_LIKELY( d > 7 )
					{
						memcpy( op, ipd, 8 );
						memcpy( op + 8, ipd + 8, 8 );
						memcpy( op + 16, ipd + 16, 4 );
						op = opcc;
						continue;
					}

					goto _err_refoob;
				}
			}
			else
			{
				bh = bv;
				ip++;
				opcc += 16 + ( bh & 0x7F );

				if LZAV_UNLIKELY(( bh & 0x80 ) != 0 )
				{
					int sh = 7;

					do
					{
						bh = *ip;
						ip++;
						opcc += ( bh & 0x7F ) << sh;

						if( sh == 28 ) // No more than 4 additional bytes.
						{
							break;
						}

						sh += 7;

					} while(( bh & 0x80 ) != 0 );
				}

				bh = *ip;

				#if defined( LZAV_PTR32 )
				if LZAV_UNLIKELY( opcc < op )
				{
					goto _err_ptrovr;
				}
				#endif // defined( LZAV_PTR32 )

				#if defined( LZAV_LONG_COPY )
				if LZAV_LIKELY(( opcc < opet ) & ( d > 15 ))
				{
					do
					{
						memcpy( op, ipd, 16 );
						memcpy( op + 16, ipd + 16, 16 );
						memcpy( op + 32, ipd + 32, 16 );
						memcpy( op + 48, ipd + 48, 16 );
						op += 64;
						ipd += 64;
					} while LZAV_LIKELY( op < opcc );

					op = opcc;
					continue;
				}
				#endif // defined( LZAV_LONG_COPY )

				if LZAV_LIKELY(( opcc < opet ) & ( d > 7 ))
				{
					do
					{
						memcpy( op, ipd, 8 );
						memcpy( op + 8, ipd + 8, 8 );
						memcpy( op + 16, ipd + 16, 8 );
						memcpy( op + 24, ipd + 24, 8 );
						op += 32;
						ipd += 32;
					} while LZAV_LIKELY( op < opcc );

					op = opcc;
					continue;
				}
			}

			if LZAV_UNLIKELY( d < 8 )
			{
				goto _err_refoob;
			}

			if LZAV_UNLIKELY( opcc > ope )
			{
				goto _err_dstoob_ref;
			}

			while( op < opcc - 15 )
			{
				memcpy( op, ipd, 8 );
				memcpy( op + 8, ipd + 8, 8 );
				op += 16;
				ipd += 16;
			}

			while( op != opcc )
			{
				*op = *ipd;
				ipd++;
				op++;
			}

			continue;

		_err_dstoob_ref:
			while( op != ope )
			{
				*op = *ipd;
				ipd++;
				op++;
			}

			return( LZAV_E_DSTOOB );
		}

		const uint8_t* LZAV_RESTRICT ipd; // Source data pointer.

		size_t ncv = bh >> 6; // Additional offset carry bits.
		ip++;
		cc = bh & 15;

		if LZAV_LIKELY( cc != 0 ) // True, if no additional length byte.
		{
			ipd = ip;
			ncv <<= csh;
			ip += cc;
			csh += 2;
			cv |= ncv;

			if LZAV_LIKELY(( op < opet ) & ( ipd < ipet - 16 ))
			{
				bh = *ip;
				memcpy( op, ipd, 16 );
				op += cc;

				goto _refblk; // Reference block follows, if not EOS.
			}
		}
		else
		{
			bh = *ip;
			ncv <<= csh;
			cc = bh & 0x7F;
			csh += 2;
			cv |= ncv;
			ip++;

			if LZAV_UNLIKELY(( bh & 0x80 ) != 0 )
			{
				int sh = 7;

				do
				{
					bh = *ip;
					ip++;
					cc |= ( bh & 0x7F ) << sh;

					if( sh == 28 ) // No more than 4 additional bytes.
					{
						break;
					}

					sh += 7;

				} while(( bh & 0x80 ) != 0 );
			}

			cc += 16;
			ipd = ip;
			ip += cc;

			uint8_t* const opcc = op + cc;

			#if defined( LZAV_PTR32 )
			if LZAV_UNLIKELY(( ip < ipd ) | ( opcc < op ))
			{
				goto _err_ptrovr;
			}
			#endif // defined( LZAV_PTR32 )

			if LZAV_LIKELY(( opcc < opet ) & ( ip < ipet - 64 ))
			{
				#if defined( LZAV_LONG_COPY )
				do
				{
					memcpy( op, ipd, 16 );
					memcpy( op + 16, ipd + 16, 16 );
					memcpy( op + 32, ipd + 32, 16 );
					memcpy( op + 48, ipd + 48, 16 );
					op += 64;
					ipd += 64;
				} while LZAV_LIKELY( op < opcc );
				#else // defined( LZAV_LONG_COPY )
				do
				{
					memcpy( op, ipd, 8 );
					memcpy( op + 8, ipd + 8, 8 );
					memcpy( op + 16, ipd + 16, 8 );
					memcpy( op + 24, ipd + 24, 8 );
					op += 32;
					ipd += 32;
				} while LZAV_LIKELY( op < opcc );
				#endif // defined( LZAV_LONG_COPY )

				bh = *ip;
				op = opcc;

				goto _refblk; // Reference block follows, if not EOS.
			}
		}

		uint8_t* const opcc = op + cc;

		if LZAV_UNLIKELY( opcc > ope )
		{
			goto _err_dstoob_lit;
		}

		if LZAV_LIKELY( ip < ipe )
		{
			memcpy( op, ipd, cc );
			bh = *ip;
			op = opcc;
			continue;
		}

		if LZAV_UNLIKELY( ip != ipe )
		{
			goto _err_srcoob_lit;
		}

		memcpy( op, ipd, cc );
		op = opcc;
		break;

	_err_srcoob_lit:
		cc = (size_t) ( ipe - ipd );

		if( cc < (size_t) ( ope - op ))
		{
			memcpy( op, ipd, cc );
			*pwl = (int) ( op + cc - (uint8_t*) dst );
		}
		else
		{
			memcpy( op, ipd, (size_t) ( ope - op ));
		}

		return( LZAV_E_SRCOOB );

	_err_dstoob_lit:
		if LZAV_UNLIKELY( ip > ipe )
		{
			goto _err_srcoob_lit;
		}

		memcpy( op, ipd, (size_t) ( ope - op ));
		return( LZAV_E_DSTOOB );
	}

	if LZAV_UNLIKELY( op != ope )
	{
		goto _err_dstlen;
	}

	return( (int) ( op - (uint8_t*) dst ));

_err_refoob:
	*pwl = (int) ( op - (uint8_t*) dst );
	return( LZAV_E_REFOOB );

_err_dstlen:
	*pwl = (int) ( op - (uint8_t*) dst );
	return( LZAV_E_DSTLEN );

#if defined( LZAV_PTR32 )
_err_ptrovr:
	*pwl = (int) ( op - (uint8_t*) dst );
	return( LZAV_E_PTROVR );
#endif // defined( LZAV_PTR32 )
}

#if LZAV_FMT_MIN < 3

/**
 * @brief Internal LZAV decompression function (data format 2).
 *
 * Function decompresses "raw" data previously compressed into the LZAV data
 * format 2.
 *
 * This function should not be called directly since it does not check the
 * format identifier.
 *
 * @param[in] src Source (compressed) data pointer.
 * @param[out] dst Destination (decompressed data) buffer pointer.
 * @param srclen Source data length, in bytes.
 * @param dstlen Expected destination data length, in bytes.
 * @param[out] pwl Pointer to variable that receives the number of bytes
 * written to the destination buffer (until error or end of buffer).
 * @return The length of decompressed data, in bytes, or any negative value if
 * some error happened.
 */

LZAV_NO_INLINE int lzav_decompress_2( const void* const src, void* const dst,
	const int srclen, const int dstlen, int* const pwl ) LZAV_NOEX
{
	if LZAV_UNLIKELY( srclen < 7 )
	{
		*pwl = 0;
		return( LZAV_E_SRCOOB );
	}

	const uint8_t* ip = (const uint8_t*) src; // Compressed data pointer.
	const uint8_t* const ipe = ip + srclen; // Compressed data boundary ptr.
	const uint8_t* const ipet = ipe - 6; // Block header read threshold.
	uint8_t* op = (uint8_t*) dst; // Destination (decompressed data) pointer.
	uint8_t* const ope = op + dstlen; // Destination boundary pointer.
	uint8_t* opet = ope - 63; // Threshold for fast copy to destination.
	*pwl = dstlen;
	const size_t mref1 = (size_t) ( *ip & 15 ) - 1; // Minimal ref length - 1.
	size_t bh; // Current block header, updated in each branch.
	size_t cv = 0; // Reference offset carry value.
	int csh = 0; // Reference offset carry shift.

	if LZAV_UNLIKELY(( ope < opet ) | ( ipe < ipet - 16 ) |
		( ipe < ipet - 64 ))
	{
		opet = LZAV_NULL;
	}

	ip++; // Advance beyond prefix byte.

	bh = *ip;

	while LZAV_LIKELY( ip < ipet )
	{
		const uint8_t* ipd; // Source data pointer.
		size_t cc; // Byte copy count.
		size_t bt; // Block type.

		if LZAV_LIKELY(( bh & 0x30 ) != 0 ) // Block type != 0.
		{
		_refblk:
			bt = ( bh >> 4 ) & 3;
			ip++;
			const int bt8 = (int) ( bt << 3 );

			LZAV_LOAD32( ip );
			ip += bt;

		#if defined( LZAV_X86 )

			static const uint32_t om[ 4 ] = { 0, 0xFF, 0xFFFF, 0xFFFFFF };
			static const int ocsh[ 4 ] = { 0, 0, 0, 3 };

			const uint32_t o = bv & om[ bt ];
			bv >>= bt8;

			const int wcsh = ocsh[ bt ];

			LZAV_SET_IPD_CV(( bh >> 6 | o << 2 ) & 0x7FFFFF, o >> 21, wcsh );

		#else // defined( LZAV_X86 )

			// Memory accesses on RISC are less efficient here.

			const size_t o = bv & (( (uint32_t) 1 << bt8 ) - 1 );
			bv >>= bt8;

			LZAV_SET_IPD_CV(( bh >> 6 | o << 2 ) & 0x7FFFFF, o >> 21,
				( bt == 3 ? 3 : 0 ));

		#endif // defined( LZAV_X86 )

			LZAV_PREFETCH( ipd );

			cc = bh & 15;

			if LZAV_LIKELY( cc != 0 ) // True, if no additional length byte.
			{
				cc += mref1;
				bh = bv & 0xFF;

				if LZAV_LIKELY( op < opet )
				{
					if LZAV_LIKELY( d > 15 )
					{
						memcpy( op, ipd, 16 );
						memcpy( op + 16, ipd + 16, 4 );
						op += cc;
						continue;
					}

					if LZAV_LIKELY( d > 7 )
					{
						memcpy( op, ipd, 8 );
						memcpy( op + 8, ipd + 8, 8 );
						op += cc;
						continue;
					}

					if( d > 3 )
					{
						memcpy( op, ipd, 4 );
						memcpy( op + 4, ipd + 4, 4 );
						op += cc;
						continue;
					}

					goto _err_refoob;
				}

				if LZAV_UNLIKELY( cc > d )
				{
					goto _err_refoob;
				}

				uint8_t* const opcc = op + cc;

				if LZAV_UNLIKELY( opcc > ope )
				{
					goto _err_dstoob_ref;
				}

				memcpy( op, ipd, cc );
				op = opcc;
				continue;
			}
			else
			{
				bh = bv & 0xFF;
				ip++;
				cc = 16 + mref1 + bh;

				if LZAV_UNLIKELY( bh == 255 )
				{
					cc += *ip;
					ip++;
				}

				uint8_t* const opcc = op + cc;
				bh = *ip;

				if LZAV_LIKELY(( opcc < opet ) & ( d > 15 ))
				{
					do
					{
						memcpy( op, ipd, 16 );
						memcpy( op + 16, ipd + 16, 16 );
						memcpy( op + 32, ipd + 32, 16 );
						memcpy( op + 48, ipd + 48, 16 );
						op += 64;
						ipd += 64;
					} while LZAV_LIKELY( op < opcc );

					op = opcc;
					continue;
				}

				if LZAV_UNLIKELY( cc > d )
				{
					goto _err_refoob;
				}

				if LZAV_UNLIKELY( opcc > ope )
				{
					goto _err_dstoob_ref;
				}

				memcpy( op, ipd, cc );
				op = opcc;
				continue;
			}

		_err_dstoob_ref:
			memcpy( op, ipd, (size_t) ( ope - op ));
			return( LZAV_E_DSTOOB );
		}

		size_t ncv = bh >> 6; // Additional offset carry bits.
		ip++;
		cc = bh & 15;

		if LZAV_LIKELY( cc != 0 ) // True, if no additional length byte.
		{
			ipd = ip;
			ncv <<= csh;
			ip += cc;
			csh += 2;
			cv |= ncv;

			if LZAV_LIKELY(( op < opet ) & ( ipd < ipet - 16 ))
			{
				bh = *ip;
				memcpy( op, ipd, 16 );
				op += cc;

				goto _refblk; // Reference block follows, if not EOS.
			}
		}
		else
		{
			bh = *ip;
			ncv <<= csh;
			cc = bh & 0x7F;
			csh += 2;
			cv |= ncv;
			ip++;

			if LZAV_UNLIKELY(( bh & 0x80 ) != 0 )
			{
				int sh = 7;

				do
				{
					bh = *ip;
					ip++;
					cc |= ( bh & 0x7F ) << sh;

					if( sh == 28 ) // No more than 4 additional bytes.
					{
						break;
					}

					sh += 7;

				} while(( bh & 0x80 ) != 0 );

				cc &= 0x7FFFFFFF; // For malformed data.
			}

			cc += 16;
			ipd = ip;
			ip += cc;

			uint8_t* const opcc = op + cc;

			#if defined( LZAV_PTR32 )
			if LZAV_UNLIKELY(( ip < ipd ) | ( opcc < op ))
			{
				goto _err_ptrovr;
			}
			#endif // defined( LZAV_PTR32 )

			if LZAV_LIKELY(( opcc < opet ) & ( ip < ipet - 64 ))
			{
				do
				{
					memcpy( op, ipd, 16 );
					memcpy( op + 16, ipd + 16, 16 );
					memcpy( op + 32, ipd + 32, 16 );
					memcpy( op + 48, ipd + 48, 16 );
					op += 64;
					ipd += 64;
				} while LZAV_LIKELY( op < opcc );

				bh = *ip;
				op = opcc;

				goto _refblk; // Reference block follows, if not EOS.
			}
		}

		uint8_t* const opcc = op + cc;

		if LZAV_UNLIKELY( opcc > ope )
		{
			goto _err_dstoob_lit;
		}

		if LZAV_LIKELY( ip < ipe )
		{
			bh = *ip;
			memcpy( op, ipd, cc );
			op = opcc;
			continue;
		}

		if LZAV_UNLIKELY( ip != ipe )
		{
			goto _err_srcoob_lit;
		}

		memcpy( op, ipd, cc );
		op = opcc;
		break;

	_err_srcoob_lit:
		cc = (size_t) ( ipe - ipd );

		if( cc < (size_t) ( ope - op ))
		{
			memcpy( op, ipd, cc );
			*pwl = (int) ( op + cc - (uint8_t*) dst );
		}
		else
		{
			memcpy( op, ipd, (size_t) ( ope - op ));
		}

		return( LZAV_E_SRCOOB );

	_err_dstoob_lit:
		if LZAV_UNLIKELY( ip > ipe )
		{
			goto _err_srcoob_lit;
		}

		memcpy( op, ipd, (size_t) ( ope - op ));
		return( LZAV_E_DSTOOB );
	}

	if LZAV_UNLIKELY( op != ope )
	{
		goto _err_dstlen;
	}

	return( (int) ( op - (uint8_t*) dst ));

_err_refoob:
	*pwl = (int) ( op - (uint8_t*) dst );
	return( LZAV_E_REFOOB );

_err_dstlen:
	*pwl = (int) ( op - (uint8_t*) dst );
	return( LZAV_E_DSTLEN );

#if defined( LZAV_PTR32 )
_err_ptrovr:
	*pwl = (int) ( op - (uint8_t*) dst );
	return( LZAV_E_PTROVR );
#endif // defined( LZAV_PTR32 )
}

#endif // LZAV_FMT_MIN < 3

#undef LZAV_LOAD32
#undef LZAV_SET_IPD_CV

/**
 * @brief LZAV decompression function (partial).
 *
 * The function decompresses "raw" data previously compressed into the LZAV
 * data format, for partial or recovery decompression. For example, this
 * function can be used to decompress only an initial segment of a larger data
 * block.
 *
 * @param[in] src Source (compressed) data pointer; can be 0 if `srclen` is 0.
 * Address alignment is unimportant.
 * @param[out] dst Destination (decompressed data) buffer pointer. Address
 * alignment is unimportant. Should be different from `src`.
 * @param srclen Source data length, in bytes; can be 0.
 * @param dstlen Destination buffer length, in bytes; can be 0.
 * @return Length of the decompressed data, in bytes. Always a non-negative
 * value (error codes are not returned).
 */

LZAV_INLINE_F int lzav_decompress_partial( const void* const src,
	void* const dst, const int srclen, const int dstlen ) LZAV_NOEX
{
	if( srclen <= 0 || src == LZAV_NULL || dst == LZAV_NULL || src == dst ||
		dstlen <= 0 )
	{
		return( 0 );
	}

	const int fmt = *(const uint8_t*) src >> 4;
	int dl = 0;

	if( fmt == 3 )
	{
		lzav_decompress_3( src, dst, srclen, dstlen, &dl );
	}

#if LZAV_FMT_MIN < 3
	else
	if( fmt == 2 )
	{
		lzav_decompress_2( src, dst, srclen, dstlen, &dl );
	}
#endif // LZAV_FMT_MIN < 3

	return( dl );
}

/**
 * @brief LZAV decompression function.
 *
 * The function decompresses "raw" data previously compressed into the LZAV
 * data format.
 *
 * Note that while the function does perform checks to avoid OOB memory
 * accesses, and checks for decompressed data length equality, this is not a
 * strict guarantee of valid decompression. In cases where the compressed data
 * is stored in long-term storage without embedded data integrity mechanisms
 * (e.g., a database without RAID 1 guarantee, a binary container without
 * a digital signature or CRC), then a checksum (hash) of the original
 * uncompressed data should be stored, and then evaluated against that of
 * the decompressed data. Also, a separate checksum (hash) of
 * an application-defined header, which contains uncompressed and compressed
 * data lengths, should be checked before decompression. A high-performance
 * "komihash" hash function can be used to obtain a hash value of the data.
 *
 * @param[in] src Source (compressed) data pointer; can be 0 if `srclen` is 0.
 * Address alignment is unimportant.
 * @param[out] dst Destination (decompressed data) buffer pointer. Address
 * alignment is unimportant. Should be different from `src`.
 * @param srclen Source data length, in bytes; can be 0.
 * @param dstlen Expected destination data length, in bytes; can be 0. Should
 * not be confused with the actual size of the destination buffer (which may
 * be larger).
 * @return Length of the decompressed data, in bytes, or any negative value if
 * an error occurred. Always returns a negative value if the resulting
 * decompressed data length differs from `dstlen`. This means that error
 * result handling requires just a check for a negative return value (see the
 * LZAV_ERROR enum for possible values).
 */

LZAV_INLINE_F int lzav_decompress( const void* const src, void* const dst,
	const int srclen, const int dstlen ) LZAV_NOEX
{
	if( srclen < 0 )
	{
		return( LZAV_E_PARAMS );
	}

	if( srclen == 0 )
	{
		return( dstlen == 0 ? 0 : LZAV_E_PARAMS );
	}

	if( src == LZAV_NULL || dst == LZAV_NULL || src == dst || dstlen <= 0 )
	{
		return( LZAV_E_PARAMS );
	}

	const int fmt = *(const uint8_t*) src >> 4;

	if( fmt == 3 )
	{
		int tmp;
		return( lzav_decompress_3( src, dst, srclen, dstlen, &tmp ));
	}

#if LZAV_FMT_MIN < 3
	if( fmt == 2 )
	{
		int tmp;
		return( lzav_decompress_2( src, dst, srclen, dstlen, &tmp ));
	}
#endif // LZAV_FMT_MIN < 3

	return( LZAV_E_UNKFMT );
}

#if defined( LZAV_NS )

} // namespace LZAV_NS

#if !defined( LZAV_NS_CUSTOM )

namespace {

using namespace LZAV_NS :: enum_wrapper;
using LZAV_NS :: lzav_compress_bound_mref5;
using LZAV_NS :: lzav_compress_bound_mref6;
using LZAV_NS :: lzav_compress_bound;
using LZAV_NS :: lzav_compress_bound_hi;
using LZAV_NS :: lzav_compress;
using LZAV_NS :: lzav_compress_mref5;
using LZAV_NS :: lzav_compress_mref6;
using LZAV_NS :: lzav_compress_default;
using LZAV_NS :: lzav_compress_hi;
using LZAV_NS :: lzav_decompress_partial;
using LZAV_NS :: lzav_decompress;

} // namespace

#endif // !defined( LZAV_NS_CUSTOM )

#endif // defined( LZAV_NS )

// Defines for Doxygen.

#if !defined( LZAV_NS_CUSTOM )
	#define LZAV_NS_CUSTOM
#endif // !defined( LZAV_NS_CUSTOM )

#if !defined( LZAV_NS )
	#define LZAV_NS
	#undef LZAV_NS
#endif // !defined( LZAV_NS )

#if defined( LZAV_DEF_MALLOC )
	#undef LZAV_MALLOC
	#undef LZAV_FREE
	#undef LZAV_DEF_MALLOC
#endif // defined( LZAV_DEF_MALLOC )

#undef LZAV_NS_CUSTOM
#undef LZAV_NOEX
#undef LZAV_NULL
#undef LZAV_X86
#undef LZAV_LITTLE_ENDIAN
#undef LZAV_COND_EC
#undef LZAV_LONG_COPY
#undef LZAV_GCC_BUILTINS
#undef LZAV_CPP_BIT
#undef LZAV_IEC32
#undef LZAV_LIKELY
#undef LZAV_UNLIKELY
#undef LZAV_RESTRICT
#undef LZAV_PREFETCH
#undef LZAV_STATIC
#undef LZAV_INLINE
#undef LZAV_INLINE_F
#undef LZAV_NO_INLINE

#endif // LZAV_INCLUDED
