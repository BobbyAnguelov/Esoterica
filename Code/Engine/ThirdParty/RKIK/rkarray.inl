//--------------------------------------------------------------------------------------------------
// array.inl	
//
// Copyright(C) by D. Gregorius. All rights reserved.
//--------------------------------------------------------------------------------------------------
#include <algorithm>


//--------------------------------------------------------------------------------------------------
// RkArray
//--------------------------------------------------------------------------------------------------
template < typename T >
RkArray< T >::RkArray()
	: mFirst( nullptr )
	, mLast( nullptr )
	, mEnd( nullptr )
	{

	}


//--------------------------------------------------------------------------------------------------
template < typename T >
RkArray< T >::RkArray( RkArray&& Other )
	{
	mFirst = Other.mFirst;
	mLast = Other.mLast;
	mEnd = Other.mEnd;

	Other.mFirst = nullptr;
	Other.mLast = nullptr;
	Other.mEnd = nullptr;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
RkArray< T >::RkArray( const RkArray& Other )
	: mFirst( nullptr )
	, mLast( nullptr )
	, mEnd( nullptr )
	{
	Reserve( Other.Size() );
	
	rkUninitializedCopy( Other.Begin(), Other.End(), Begin() );
	mLast = mFirst + Other.Size();
	}


//--------------------------------------------------------------------------------------------------
// template < typename T > inline
// RkArray< T >::RkArray( std::initializer_list< T > List )
// 	: mFirst( nullptr )
// 	, mLast( nullptr )
// 	, mEnd( nullptr )
// 	{
// 	Resize( static_cast< int >( List.size() ) );
// 	rkCopy( List.begin(), List.end(), mFirst );
// 	}


//--------------------------------------------------------------------------------------------------
template < typename T >
RkArray< T >::RkArray( const T* First, const T* Last )
	: mFirst( nullptr )
	, mLast( nullptr )
	, mEnd( nullptr )
	{
	if ( Last > First )
		{
		Resize( static_cast< int >( Last - First ) );
		rkCopy( First, Last, mFirst );
		}
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
RkArray< T >::RkArray( int Count, const T& Value )
	: mFirst( nullptr )
	, mLast( nullptr )
	, mEnd( nullptr )
	{
	if ( Count > 0 )
		{
		Resize( Count );
		rkFill( mFirst, mLast, Value );
		}
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
RkArray< T >::~RkArray()
	{
	// Deallocate
	rkDestroy( mFirst, mLast );
	rkAlignedFree( mFirst );
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
RkArray< T >& RkArray< T >::operator=( RkArray&& Other )
	{
	if ( this != &Other )
		{
		// Deallocate
		rkDestroy( mFirst, mLast );
		rkAlignedFree( mFirst );
		
		// Move
		mFirst = Other.mFirst;
		mLast = Other.mLast;
		mEnd = Other.mEnd;

		Other.mFirst = nullptr;
		Other.mLast = nullptr;
		Other.mEnd = nullptr;
		}

	return *this;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
RkArray< T >& RkArray< T >::operator=( const RkArray& Other )
	{
	if ( this != &Other )
		{
		Resize( Other.Size() );
		rkCopy( Other.Begin(), Other.End(), Begin() );
		}

	return *this;
	}


//--------------------------------------------------------------------------------------------------
// template < typename T >
// RkArray< T >& RkArray<T>::operator=( std::initializer_list< T > List )
// 	{
// 	Resize( static_cast< int >( List.size() ) );
// 	rkCopy( List.begin(), List.end(), Begin() );
// 
// 	return *this;
// 	}


//--------------------------------------------------------------------------------------------------
template < typename T >
T& RkArray< T >::operator[]( int Index )
	{
	RK_ASSERT( 0 <= Index && Index < Size() );
	return *( mFirst + Index );
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
const T& RkArray< T >::operator[]( int Index ) const
	{
	RK_ASSERT( 0 <= Index && Index < Size() );
	return *( mFirst + Index );
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
int RkArray< T >::Size() const
	{
	return static_cast< int >( mLast - mFirst );
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
int RkArray< T >::Capacity() const
	{
	return static_cast< int >( mEnd - mFirst );
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
bool RkArray< T >::Empty() const
	{
	return mLast == mFirst;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
void RkArray< T >::Clear()
	{
	rkDestroy( mFirst, mLast );
	mLast = mFirst;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
void RkArray< T >::Reserve( int Count )
	{
	RK_ASSERT( Count >= 0 );
	if ( Count > Capacity() )
		{
		// Allocate new buffers
		static_assert( alignof( T ) <= 16 );
		T* First = static_cast< T* >( rkAlignedAlloc( Count * sizeof( T ), 16 ) );
		T* Last = First + Size();
		T* End = First + Count;

		// Move old buffer
		if ( !Empty() )
			{
			rkUninitializedMove( mFirst, mLast, First );
			}

		rkDestroy( mFirst, mLast );
		rkAlignedFree( mFirst );
		
		// Save new buffers
		mFirst = First;
		mLast = Last;
		mEnd = End;
		}
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
void RkArray< T >::Resize( int Count )
	{
	Reserve( Count );

	rkDestroy( Size() - Count, mFirst + Count );
	rkConstruct( Count - Size(), mLast );
	mLast = mFirst + Count;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
int RkArray< T >::PushBack()
	{
	if ( mLast == mEnd )
		{
		// Try to grow 50%
		Reserve( Capacity() ? Capacity() + Capacity() / 2 : InitialCapacity );
		}
	rkConstruct( mLast++ );

	return Size() - 1;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
int RkArray< T >::PushBack( const T& Value )
	{
	if ( mFirst <= std::addressof( Value ) && std::addressof( Value ) < mLast )
		{
		// The object is part of the array. Growing the
		// array might destroy it and invalidate the reference
		int Index = int( std::addressof( Value ) - mFirst );
		if ( mLast == mEnd )
			{
			// Try to grow 50%
			Reserve( Capacity() ? Capacity() + Capacity() / 2 : InitialCapacity );
			}

		rkCopyConstruct( mLast++, mFirst[ Index ] );
		}
	else
		{
		if ( mLast == mEnd )
			{
			// Try to grow 50%
			Reserve( Capacity() ? Capacity() + Capacity() / 2 : InitialCapacity );
			}

		rkCopyConstruct( mLast++, Value );
		}

	return Size() - 1;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
inline void RkArray< T >::PushBack( int Count, const T* List )
	{
	PushBack( List, List + Count );
	}


//--------------------------------------------------------------------------------------------------
template <typename T>
inline void RkArray< T >::PushBack( const T* First, const T* Last )
	{
	int Count = int( Last - First );
	if ( Count > 0 )
		{
		// DIRK_TODO: Handle self append!
		RK_ASSERT( Last < mFirst || First >= mEnd );

		Resize( Size() + Count );
		rkCopy( First, Last, mLast - Count );
		}
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
inline void RkArray< T >::PushBack( const RkArray< int >& Other )
	{
	PushBack( Other.Begin(), Other.End() );
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
inline void RkArray< T >::Insert( int Where, const T& Value )
	{
	// DIRK_TODO: Handle insertion of elements inside array...
	RK_ASSERT( std::addressof( Value ) < mFirst || mLast < std::addressof( Value ) );

	// As in vector::insert() the insertion index can be one larger 
	// than the vector size indicating insertion at the end.
	RK_ASSERT( 0 <= Where && Where <= Size() );
	if ( 0 <= Where && Where <= Size() )
		{
		// Add new object
		PushBack();

		// Shift down
		for ( int Index = Size() - 1; Index > Where; --Index )
			{
			mFirst[ Index ] = std::move( mFirst[ Index - 1 ] );
			}

		// Insert 
		mFirst[ Where ] = Value;
		}
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
void RkArray< T >::PopBack()
	{
	RK_ASSERT( !Empty() );
	rkDestroy( --mLast );
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
void RkArray<T>::Remove( int Where )
	{
	if ( 0 <= Where && Where < Size() )
		{
		T* First = mFirst + Where;
		for ( T* Iterator = First; ++Iterator != mLast; )
			{
			*First++ = std::move( *Iterator );
			}

		rkDestroy( First, mLast );
		mLast = First;
		}
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
void RkArray< T >::Remove( const T& Value )
	{
	T* First = std::remove( mFirst, mLast, Value );

	rkDestroy( First, mLast );
	mLast = First;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
bool RkArray< T >::Contains( const T& Value ) const
	{
	return std::find( mFirst, mLast, Value ) != mLast;
	}


//--------------------------------------------------------------------------------------------------
template < typename T > 
int RkArray< T >::IndexOf( const T& Value ) const
	{
	int Index = static_cast< int >( std::distance( mFirst, std::find( mFirst, mLast, Value ) ) );
	if ( Index == Size() )
		{
		Index = -1;
		}

	return Index;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
T& RkArray< T >::Front()
	{
	RK_ASSERT( !Empty() );
	return *mFirst;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
const T& RkArray< T >::Front() const
	{
	RK_ASSERT( !Empty() );
	return *mFirst;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
T& RkArray< T >::Back()
	{
	RK_ASSERT( !Empty() );
	return *( mLast - 1 );
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
const T& RkArray< T >::Back() const
	{
	RK_ASSERT( !Empty() );
	return *( mLast - 1 );
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
T* RkArray< T >::Begin()
	{
	return mFirst;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
const T* RkArray< T >::Begin() const
	{
	return mFirst;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
T* RkArray< T >::End()
	{
	return mLast;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
const T* RkArray< T >::End() const
	{
	return mLast;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
T* RkArray< T >::Data()
	{
	return mFirst;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
const T* RkArray< T >::Data() const
	{
	return mFirst;
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
bool RkArray< T >::operator==( const RkArray& Other ) const
	{
	return Size() == Other.Size() && std::equal( mFirst, mLast, Other.mFirst );
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
bool RkArray< T >::operator!=( const RkArray& Other ) const
	{
	return !( *this == Other );
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
void rkDeleteAll( T** First, T** Last )
	{
	while ( First != Last )
		{
		delete* First++;
		}
	}


//--------------------------------------------------------------------------------------------------
template < typename T >
void rkDeleteAll( RkArray< T* >& Array )
	{
	rkDeleteAll( Array.Begin(), Array.End() );
	}
