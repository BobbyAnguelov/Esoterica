//--------------------------------------------------------------------------------------------------
/*
	@file		types.h

	@author		Dirk Gregorius
	@version	0.1
	@date		09/12/2023

	Copyright(C) by D. Gregorius. All rights reserved.
*/
//--------------------------------------------------------------------------------------------------
#pragma once

#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>


//--------------------------------------------------------------------------------------------------
// Basic definitions
//--------------------------------------------------------------------------------------------------
#define ragnarok				public

#define RK_RESTRICT			__restrict
#define RK_FORCEINLINE		__forceinline

#define RK_ALIGN4			__declspec( align(   4 ) )
#define RK_ALIGN8			__declspec( align(   8 ) )
#define RK_ALIGN16			__declspec( align(  16 ) )
#define RK_ALIGN32			__declspec( align(  32 ) )
#define RK_ALIGN64			__declspec( align(  64 ) )
#define RK_ALIGN128			__declspec( align( 128 ) )

#define RK_COUNT_OF			_countof
#define RK_INTERFACE		__declspec( novtable )
#define RK_GLOBAL_CONSTANT	extern __declspec( selectany ) 


//--------------------------------------------------------------------------------------------------
// Basic types
//--------------------------------------------------------------------------------------------------
typedef int8_t				int8;
typedef int16_t				int16;
typedef int32_t				int32;
typedef int64_t				int64;

typedef uint8_t				uint8;
typedef uint16_t			uint16;
typedef uint32_t			uint32;
typedef uint64_t			uint64;

typedef float				float32;
typedef double				float64;

typedef intptr_t			intp;
typedef uintptr_t			uintp;


//--------------------------------------------------------------------------------------------------
// Basic type limits
//--------------------------------------------------------------------------------------------------
#define RK_F32_MIN			FLT_MIN
#define RK_F32_MAX			FLT_MAX
#define RK_F32_EPSILON		FLT_EPSILON

#define RK_F64_MIN			DBL_MIN
#define RK_F64_MAX			DBL_MAX
#define RK_F64_EPSILON		DBL_EPSILON
		
#define RK_S8_MIN			CHAR_MIN
#define RK_S8_MAX			CHAR_MAX
#define RK_S16_MIN			SHRT_MIN
#define RK_S16_MAX			SHRT_MAX
#define RK_S32_MIN			INT_MIN
#define RK_S32_MAX			INT_MAX
#define RK_S64_MIN			LLONG_MIN
#define RK_S64_MAX			LLONG_MAX
		
#define RK_U8_MAX			UCHAR_MAX
#define RK_U16_MAX			USHRT_MAX
#define RK_U32_MAX			UINT_MAX
#define RK_U64_MAX			ULLONG_MAX