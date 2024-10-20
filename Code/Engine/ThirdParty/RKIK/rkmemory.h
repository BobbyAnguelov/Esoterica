//--------------------------------------------------------------------------------------------------
/*
	@file		memory.h

	@author		Dirk Gregorius
	@version	0.1
	@date		09/12/2023

	Copyright(C) by D. Gregorius. All rights reserved.
*/
//--------------------------------------------------------------------------------------------------
#pragma once

#include "rkassert.h"
#include "rktypes.h"

#include <new.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <type_traits>


//--------------------------------------------------------------------------------------------------
// Memory functions
//--------------------------------------------------------------------------------------------------
void* rkAlloc( size_t Bytes );
void* rkRealloc( void* Address, size_t Bytes );
void rkFree( void* Address );
void* rkAlignedAlloc( size_t Bytes, size_t Alignement );
void* rkAlignedRealloc( void* Address, size_t Bytes, size_t Alignement );
void rkAlignedFree( void* Address );

void rkMemCpy( void* Dst, const void* Src, size_t Bytes );
void rkMemMove( void* Dst, const void* Src, size_t Bytes );
void rkMemZero( void* Dst, size_t Bytes );
int rkMemCmp( const void* Lhs, const void* Rhs, size_t Bytes );

int rkStrLen( const char* String );
char* rkStrCpy( char* Dst, const char* Src );
char* rkStrCat( char* Dst, const char* Src );
char* rkStrDup( const char* Src );
int rkStrCmp( const char* Lhs, const char* Rhs );
int rkStrICmp( const char* Lhs, const char* Rhs );

int rkAlign( size_t Offset, size_t Alignment );
void* rkAlign( void* Address, size_t Alignment );
const void* rkAlign( const void* Address, size_t Alignment );
bool rkIsAligned( const void* Address, size_t Alignment );
void* rkAddByteOffset( void* Address, size_t Bytes );
const void* rkAddByteOffset( const void* Address, ptrdiff_t Offset );
ptrdiff_t rkByteOffset( const void* Address1, const void* Address2 );


//--------------------------------------------------------------------------------------------------
// Memory utilities
//--------------------------------------------------------------------------------------------------
template < typename T > bool rkIsAligned( const T* Pointer, size_t Alignement = __alignof( T ) );

template < typename T > void rkConstruct( T* Object );
template < typename T > void rkConstruct( int Count, T* Objects );
template < typename T > void rkConstruct( T* First, T* Last );
template < typename T > void rkDestroy( T* Object );
template < typename T > void rkDestroy( int Count, T* Objects );
template < typename T > void rkDestroy( T* First, T* Last );

template < typename T > void rkCopyConstruct( T* Object, const T& Value );
template < typename T > void rkMoveConstruct( T* Object, T&& Value );

template < typename T > void rkCopy( const T* First, const T* Last, T* Dest );
template < typename T > void rkMove( T* First, T* Last, T* Dest );
template < typename T > void rkFill( T* First, T* Last, const T& Value );
template < typename T > void rkUninitializedCopy( const T* First, const T* Last, T* Dest );
template < typename T > void rkUninitializedMove( T* First, T* Last, T* Dest );
template < typename T > void rkUninitializedFill( T* First, T* Last, const T& Value );


#include "rkmemory.inl"

