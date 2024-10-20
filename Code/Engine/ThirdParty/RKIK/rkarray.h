//--------------------------------------------------------------------------------------------------
/*
	@file		array.h

	@author		Dirk Gregorius
	@version	0.1
	@date		09/12/2023

	Copyright(C) by D. Gregorius. All rights reserved.
*/
//--------------------------------------------------------------------------------------------------
#pragma once

#include "rkassert.h"
#include "rkmemory.h"

//--------------------------------------------------------------------------------------------------
// RkArray 
//--------------------------------------------------------------------------------------------------
template < typename T >
class RkArray
	{
	public:
		RkArray();
		RkArray( RkArray&& Other );
		RkArray( const RkArray& Other );
		RkArray( const T* First, const T* Last );
		explicit RkArray( int Count, const T& Value = T() );
		~RkArray();

		RkArray& operator=( RkArray&& Other );
		RkArray& operator=( const RkArray& Other );

		T& operator[]( int Index );
		const T& operator[]( int Index ) const;

		int Size() const;
		int Capacity() const;
		bool Empty() const;

		void Clear();
		void Reserve( int Count );
		void Resize( int Count );

		int PushBack();
		int PushBack( const T& Value );
		void PushBack( int Count, const T* List );
		void PushBack( const T* First, const T* Last );
		void PushBack( const RkArray< int >& Other );
		void Insert( int Where, const T& Value );

		void PopBack();
		void Remove( int Where );
		void Remove( const T& Value );
		
		bool Contains( const T& Value ) const;
		int IndexOf( const T& Value ) const;

		T& Front();
		const T& Front() const;
		T& Back();
		const T& Back() const;
		T* Begin();
		const T* Begin() const;
		T* End();
		const T* End() const;

		T* Data();
		const T* Data() const;

		bool operator==( const RkArray& Other ) const;
		bool operator!=( const RkArray& Other ) const;

	protected:
		enum { InitialCapacity = 16 };

		T* mFirst;
		T* mLast;
		T* mEnd;

		// STL like iterators to enable range based loop support. (Do not use directly!)
		inline friend T* begin( RkArray& Array ) { return Array.Begin(); }
		inline friend const T* begin( const RkArray& Array ) { return Array.Begin(); }
		inline friend T* end( RkArray& Array ) { return Array.End(); }
		inline friend const T* end( const RkArray& Array ) { return Array.End(); }
	};

// Utilities
template < typename T >
void rkDeleteAll( T** First, T** Last );
template < typename T >
void rkDeleteAll( RkArray< T* >& Array );


#include "rkarray.inl"
