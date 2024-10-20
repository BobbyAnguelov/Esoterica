//--------------------------------------------------------------------------------------------------
// memory.inl
//
// Copyright(C) by D. Gregorius. All rights reserved.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
// RkMemory
//--------------------------------------------------------------------------------------------------
inline void* rkAlloc( size_t Bytes )
	{
	return malloc( Bytes );
	}


//------------------------------------------`--------------------------------------------------------
inline void* rkRealloc( void* Address, size_t Bytes )
	{
	return realloc( Address, Bytes );
	}


//--------------------------------------------------------------------------------------------------
inline void rkFree( void* Address )
	{
	free( Address );
	}


//--------------------------------------------------------------------------------------------------
inline void* rkAlignedAlloc( size_t Bytes, size_t Alignement )
	{
	return _aligned_malloc( Bytes, Alignement );
	}


//--------------------------------------------------------------------------------------------------
inline void* rkAlignedRealloc( void* Address, size_t Bytes, size_t Alignement )
	{
	return _aligned_realloc( Address, Bytes, Alignement );
	}


//--------------------------------------------------------------------------------------------------
inline void rkAlignedFree( void* Address )
	{
	_aligned_free( Address );
	}


//--------------------------------------------------------------------------------------------------
inline void rkMemCpy( void* Dst, const void* Src, size_t Bytes )
	{
	memcpy( Dst, Src, Bytes );
	}


//--------------------------------------------------------------------------------------------------
inline void rkMemMove( void* Dst, const void* Src, size_t Bytes )
	{
	memmove( Dst, Src, Bytes );
	}


//--------------------------------------------------------------------------------------------------
inline void rkMemZero( void* Dst, size_t Bytes )
	{
	memset( Dst, 0, Bytes );
	}


//--------------------------------------------------------------------------------------------------
inline int rkMemCmp( const void* Lhs, const void* Rhs, size_t Bytes )
	{
	return memcmp( Lhs, Rhs, Bytes );
	}


//--------------------------------------------------------------------------------------------------
inline int rkStrLen( const char* String )
	{
	return static_cast< int >( strlen( String ) );
	}


//--------------------------------------------------------------------------------------------------
inline char* rkStrCpy( char* Dst, const char* Src )
	{
	return strcpy( Dst, Src );
	}


//--------------------------------------------------------------------------------------------------
inline char* rkStrCat( char* Dst, const char* Src )
	{
	return strcat( Dst, Src );
	}


//--------------------------------------------------------------------------------------------------
inline char* rkStrDup( const char* Src )
	{
	char* Dst = nullptr;
	if ( Src && *Src )
		{
		Dst = static_cast< char* >( rkAlloc( static_cast< size_t >( rkStrLen( Src ) + 1 ) ) );
		rkStrCpy( Dst, Src );
		}

	return Dst;
	}


//--------------------------------------------------------------------------------------------------
inline int rkStrCmp( const char* Lhs, const char* Rhs )
	{
	return strcmp( Lhs, Rhs );
	}


//--------------------------------------------------------------------------------------------------
inline int rkStrICmp( const char* Lhs, const char* Rhs )
	{
	return _stricmp( Lhs, Rhs ) ;
	}


//--------------------------------------------------------------------------------------------------
inline int rkAlign( size_t Offset, size_t Alignment )
	{
	return int( ( Offset + Alignment - 1 ) & ~( Alignment - 1 ) );
	}


//--------------------------------------------------------------------------------------------------
inline void* rkAlign( void* Address, size_t Alignment )
	{
	intp Offset = reinterpret_cast< intp >( Address );
	Offset = ( Offset + Alignment - 1 ) & ~( Alignment - 1 );
	RK_ASSERT( ( Offset & ( Alignment - 1 ) ) == 0 );

	return reinterpret_cast< void* >( Offset );
	}


//--------------------------------------------------------------------------------------------------
inline const void* rkAlign( const void* Address, size_t Alignment )
	{
	intp Offset = reinterpret_cast< intp >( Address );
	Offset = ( Offset + Alignment - 1 ) & ~( Alignment - 1 );
	RK_ASSERT( ( Offset & ( Alignment - 1 ) ) == 0 );

	return reinterpret_cast< const void* >( Offset );
	}


//--------------------------------------------------------------------------------------------------
inline bool rkIsAligned( const void* Address, size_t Alignment )
	{
	return ( reinterpret_cast< intp >( Address ) & ( Alignment - 1 ) ) == 0;
	}


//--------------------------------------------------------------------------------------------------
inline void* rkAddByteOffset( void* Address, size_t Bytes )
	{
	return static_cast< unsigned char* >( Address ) + Bytes;
	}


//--------------------------------------------------------------------------------------------------
inline const void* rkAddByteOffset( const void* Address, ptrdiff_t Offset )
	{
	return static_cast< const unsigned char* >( Address ) + Offset;
	}


//--------------------------------------------------------------------------------------------------
inline ptrdiff_t rkByteOffset( const void* Address1, const void* Address2 )
	{
	return static_cast< const unsigned char* >( Address2 ) - static_cast< const unsigned char* >( Address1 );
	}


//--------------------------------------------------------------------------------------------------
// Memory utilities
//--------------------------------------------------------------------------------------------------
template < typename T > inline
bool rkIsAligned( const T* Pointer, size_t Alignement )
	{
	return ( reinterpret_cast< uintptr_t >(Pointer)& ( Alignement - 1 ) ) == 0;
	}


//--------------------------------------------------------------------------------------------------
template < typename T > inline
void rkConstruct( T* Object )
	{
	RK_ASSERT( Object );
	new ( static_cast< void* >( Object ) ) T();
	}


//--------------------------------------------------------------------------------------------------
template < typename T > inline
void rkConstruct( int Count, T* Objects )
	{
	for ( int Index = 0; Index < Count; ++Index )
		{
		new ( static_cast< void* >( Objects + Index ) ) T();
		}
	}


//--------------------------------------------------------------------------------------------------
template < typename T >  inline
void rkConstruct( T* First, T* Last )
	{
	RK_ASSERT( Last >= First );
	while ( First != Last )
		{
		new ( static_cast< void* >( First++ ) ) T();
		}
	}


//--------------------------------------------------------------------------------------------------
template < typename T >  inline
void rkDestroy( T*, std::true_type )
	{
	// Intentionally empty!
	}


//--------------------------------------------------------------------------------------------------
template < typename T >  inline
void rkDestroy( T* Object, std::false_type )
	{
	if ( Object )
		{
		Object->~T();
		}
	}


//--------------------------------------------------------------------------------------------------
template < typename T >  inline
void rkDestroy( T* Object )
	{
	rkDestroy( Object, std::is_trivially_destructible< T >() );
	}


//--------------------------------------------------------------------------------------------------
template < typename T >  inline
void rkDestroy( int, T*, std::true_type )
	{
	// Intentionally empty!
	}


//--------------------------------------------------------------------------------------------------
template < typename T >  inline
void rkDestroy( int Count, T* Objects, std::false_type )
	{
	for ( int Index = 0; Index < Count; ++Index )
		{
		( Objects + Index )->~T();
		}
	}


//--------------------------------------------------------------------------------------------------
template < typename T >  inline
void rkDestroy( int Count, T* Objects )
	{
	rkDestroy( Count, Objects, std::is_trivially_destructible< T >() );
	}


//--------------------------------------------------------------------------------------------------
template < typename T >  inline
void rkDestroy( T*, T*, std::true_type )
	{
	// Intentionally empty!
	}


//--------------------------------------------------------------------------------------------------
template < typename T >  inline
void rkDestroy( T* First, T* Last, std::false_type )
	{
	while ( First != Last )
		{
		( First++ )->~T();
		}
	}


//--------------------------------------------------------------------------------------------------
template < typename T >  inline
void rkDestroy( T* First, T* Last )
	{
	RK_ASSERT( Last >= First );
	rkDestroy( First, Last, std::is_trivially_destructible< T >() );
	}


//--------------------------------------------------------------------------------------------------
template < typename T > inline
void rkCopyConstruct( T* Object, const T& Value )
	{
	RK_ASSERT( Object );
	new ( static_cast< void* >( Object ) ) T( Value );
	}


//--------------------------------------------------------------------------------------------------
template < typename T > inline
void rkMoveConstruct( T* Object, T&& Value )
	{
	RK_ASSERT( Object );
	new ( static_cast< void* >( Object ) ) T( std::move( Value ) );
	}


//--------------------------------------------------------------------------------------------------
template< typename T > inline
void rkCopy( const T* First, const T* Last, T* Dest, std::true_type )
	{
	rkMemCpy( Dest, First, ( Last - First ) * sizeof( T ) );
	}


//--------------------------------------------------------------------------------------------------
template< typename T > inline
void rkCopy( const T* First, const T* Last, T* Dest, std::false_type )
	{
	while ( First != Last )
		{
		*Dest++ = *First++;
		}
	}


//--------------------------------------------------------------------------------------------------
template< typename T > inline
void rkCopy( const T* First, const T* Last, T* Dest )
	{
	RK_ASSERT( Last >= First );
	rkCopy( First, Last, Dest, std::is_trivially_copyable< T >() );
	}


//--------------------------------------------------------------------------------------------------
template< typename T > inline
void rkMove( T* First, T* Last, T* Dest, std::true_type )
	{
	rkMemMove( Dest, First, ( Last - First ) * sizeof( T ) );
	}


//--------------------------------------------------------------------------------------------------
template< typename T > inline
void rkMove( T* First, T* Last, T* Dest, std::false_type )
	{
	while ( First != Last )
		{
		*Dest++ = std::move( *First++ );
		}
	}


//--------------------------------------------------------------------------------------------------
template< typename T > inline
void rkMove( T* First, T* Last, T* Dest )
	{
	RK_ASSERT( Last >= First );
	rkMove( First, Last, Dest, std::is_trivially_move_assignable< T >() );
	}


//--------------------------------------------------------------------------------------------------
template< typename T > inline
void rkFill( T* First, T* Last, const T& Value )
	{
	RK_ASSERT( Last >= First );
	while ( First != Last )
		{
		*First++ = Value;
		}
	}


//--------------------------------------------------------------------------------------------------
template< typename T > inline
void rkUninitializedCopy( const T* First, const T* Last, T* Dest, std::true_type )
	{
	rkMemCpy( Dest, First, ( Last - First ) * sizeof( T ) );
	}


//--------------------------------------------------------------------------------------------------
template< typename T > inline
void rkUninitializedCopy( const T* First, const T* Last, T* Dest, std::false_type )
	{
	while ( First != Last )
		{
		new ( static_cast< void* >( std::addressof( *Dest++ ) ) ) T( *First++ );
		}
	}


//--------------------------------------------------------------------------------------------------
template< typename T > inline
void rkUninitializedCopy( const T* First, const T* Last, T* Dest )
	{
	RK_ASSERT( Last >= First );
	rkUninitializedCopy( First, Last, Dest, std::is_trivially_copy_constructible< T >() );
	}


//--------------------------------------------------------------------------------------------------
template< typename T > inline
void rkUninitializedMove( T* First, T* Last, T* Dest, std::true_type )
	{
	rkMemMove( Dest, First, ( Last - First ) * sizeof( T ) );
	}


//--------------------------------------------------------------------------------------------------
template< typename T > inline
void rkUninitializedMove( T* First, T* Last, T* Dest, std::false_type )
	{
	for ( ; First != Last; ++First, ++Dest )
		{
		new ( static_cast< void* >( Dest ) ) T( std::move( *First ) );
		}
	}


//--------------------------------------------------------------------------------------------------
template< typename T > inline
void rkUninitializedMove( T* First, T* Last, T* Dest )
	{
	RK_ASSERT( Last >= First );
	rkUninitializedMove( First, Last, Dest, std::is_trivially_move_constructible< T >() );
	}


//--------------------------------------------------------------------------------------------------
template< typename T > inline
void rkUninitializedFill( T* First, T* Last, const T& Value )
	{
	RK_ASSERT( Last >= First );
	while ( First != Last )
		{
		new ( static_cast< void* >( First++ ) ) T( Value );
		}
	}