#pragma once

#include "Base/ThirdParty/EA/eastl_Esoterica_TrackedAllocator.h"

#include "EASTL/vector.h"
#include "EASTL/fixed_vector.h"
#include "EASTL/array.h"
#include "EASTL/span.h"

#include <type_traits>
#include <utility>

//-------------------------------------------------------------------------

namespace EE
{
    //-------------------------------------------------------------------------
    // Most commonly used containers aliases
    //-------------------------------------------------------------------------

    template<typename T> using TVector = eastl::vector<T, eastl::TrackedAllocator>;
    template<typename T, eastl_size_t S> using TInlineVector = eastl::fixed_vector<T, S, true, eastl::TrackedAllocator>;
    template<typename T, eastl_size_t S> using TArray = eastl::array<T, S>;

    template<typename T> using TAlignedVector = eastl::vector<T, eastl::TrackedAlignedAllocator>;

    using Blob = TVector<uint8_t>;

    template<typename T> using TArrayView = eastl::span<T, size_t( -1 )>;

    //-------------------------------------------------------------------------
    // Simple utility functions to improve syntactic usage of container types
    //-------------------------------------------------------------------------

    // Find an element in a vector
    template<typename T, typename V>
    inline typename TVector<T>::const_iterator VectorFind( TVector<T> const& vector, V const& value )
    {
        return eastl::find( vector.begin(), vector.end(), value );
    }

    // Find an element in a vector
    // Usage: vectorFind( vector, value, [] ( T const& typeRef, V const& valueRef ) { ... } );
    template<typename T, typename V, typename Predicate>
    inline typename TVector<T>::const_iterator VectorFind( TVector<T> const& vector, V const& value, Predicate predicate )
    {
        return eastl::find( vector.begin(), vector.end(), value, eastl::forward<Predicate>( predicate ) );
    }

    // Find an element in a vector
    // Require non-const versions since we might want to modify the result
    template<typename T, typename V>
    inline typename TVector<T>::iterator VectorFind( TVector<T>& vector, V const& value )
    {
        return eastl::find( vector.begin(), vector.end(), value );
    }

    // Find an element in a vector
    // Usage: vectorFind( vector, value, [] ( T const& typeRef, V const& valueRef ) { ... } );
    // Require non-const versions since we might want to modify the result
    template<typename T, typename V, typename Predicate>
    inline typename TVector<T>::iterator VectorFind( TVector<T>& vector, V const& value, Predicate predicate )
    {
        return eastl::find( vector.begin(), vector.end(), value, eastl::forward<Predicate>( predicate ) );
    }

    template<typename T, typename V>
    inline bool VectorContains( TVector<T> const& vector, V const& value )
    {
        return eastl::find( vector.begin(), vector.end(), value ) != vector.end();
    }

    // Usage: VectorContains( vector, value, [] ( T const& typeRef, V const& valueRef ) { ... } );
    template<typename T, typename V, typename Predicate>
    inline bool VectorContains( TVector<T> const& vector, V const& value, Predicate predicate )
    {
        return eastl::find( vector.begin(), vector.end(), value, eastl::forward<Predicate>( predicate ) ) != vector.end();
    }

    // Usage: VectorContains( vector, [] ( T const& typeRef ) { ... } );
    template<typename T, typename V, typename Predicate>
    inline bool VectorContains( TVector<T> const& vector, Predicate predicate )
    {
        return eastl::find_if( vector.begin(), vector.end(), eastl::forward<Predicate>( predicate ) ) != vector.end();
    }

    template<typename T, typename V>
    inline int32_t VectorFindIndex( TVector<T> const& vector, V const& value )
    {
        auto iter = eastl::find( vector.begin(), vector.end(), value );
        if ( iter == vector.end() )
        {
            return InvalidIndex;
        }
        else
        {
            return (int32_t) ( iter - vector.begin() );
        }
    }

    // Usage: VectorFindIndex( vector, [] ( T const& typeRef, V const& valueRef) { ... } );
    template<typename T, typename V, typename Predicate>
    inline int32_t VectorFindIndex( TVector<T> const& vector, V const& value, Predicate predicate )
    {
        auto iter = eastl::find( vector.begin(), vector.end(), value, predicate );
        if ( iter == vector.end() )
        {
            return InvalidIndex;
        }
        else
        {
            return (int32_t) ( iter - vector.begin() );
        }
    }

    // Usage: VectorFindIndex( vector, [] ( T const& typeRef ) { ... } );
    template<typename T, typename Predicate>
    inline int32_t VectorFindIndexByPredicate( TVector<T> const& vector, Predicate predicate )
    {
        auto iter = eastl::find_if( vector.begin(), vector.end(), predicate );
        if ( iter == vector.end() )
        {
            return InvalidIndex;
        }
        else
        {
            return (int32_t) ( iter - vector.begin() );
        }
    }

    //-------------------------------------------------------------------------

    template<typename T, typename V, eastl_size_t S>
    inline bool VectorContains( TInlineVector<T, S> const& vector, V const& value )
    {
        return eastl::find( vector.begin(), vector.end(), value ) != vector.end();
    }

    template<typename T, eastl_size_t S, typename V, typename Predicate>
    inline bool VectorContains( TInlineVector<T, S> const& vector, V const& value, Predicate predicate )
    {
        return eastl::find( vector.begin(), vector.end(), value, eastl::forward<Predicate>( predicate ) ) != vector.end();
    }

    // Find an element in a vector
    template<typename T, typename V, eastl_size_t S>
    inline typename TInlineVector<T, S>::const_iterator VectorFind( TInlineVector<T, S> const& vector, V const& value )
    {
        return eastl::find( vector.begin(), vector.end(), value );
    }

    // Find an element in a vector
    template<typename T, typename V, eastl_size_t S, typename Predicate>
    inline typename TInlineVector<T, S>::const_iterator VectorFind( TInlineVector<T, S> const& vector, V const& value, Predicate predicate )
    {
        return eastl::find( vector.begin(), vector.end(), value, eastl::forward<Predicate>( predicate ) );
    }

    // Find an element in a vector
    // Require non-const versions since we might want to modify the result
    template<typename T, typename V, eastl_size_t S>
    inline typename TInlineVector<T, S>::iterator VectorFind( TInlineVector<T, S>& vector, V const& value )
    {
        return eastl::find( vector.begin(), vector.end(), value );
    }

    // Find an element in a vector
    // Require non-const versions since we might want to modify the result
    template<typename T, typename V, eastl_size_t S, typename Predicate>
    inline typename TInlineVector<T, S>::iterator VectorFind( TInlineVector<T, S>& vector, V const& value, Predicate predicate )
    {
        return eastl::find( vector.begin(), vector.end(), value, eastl::forward<Predicate>( predicate ) );
    }

    template<typename T, typename V, eastl_size_t S>
    inline int32_t VectorFindIndex( TInlineVector<T, S> const& vector, V const& value )
    {
        auto iter = eastl::find( vector.begin(), vector.end(), value );
        if ( iter == vector.end() )
        {
            return InvalidIndex;
        }
        else
        {
            return (int32_t) ( iter - vector.begin() );
        }
    }

    // Usage: VectorFindIndex( vector, [] ( T const& typeRef, V const& valueRef) { ... } );
    template<typename T, typename V, eastl_size_t S, typename Predicate>
    inline int32_t VectorFindIndex( TInlineVector<T, S> const& vector, V const& value, Predicate predicate )
    {
        auto iter = eastl::find( vector.begin(), vector.end(), value, predicate );
        if ( iter == vector.end() )
        {
            return InvalidIndex;
        }
        else
        {
            return (int32_t) ( iter - vector.begin() );
        }
    }

    // Usage: VectorFindIndex( vector, [] ( T const& typeRef ) { ... } );
    template<typename T, eastl_size_t S, typename Predicate>
    inline int32_t VectorFindIndexByPredicate( TInlineVector<T, S> const& vector, Predicate predicate )
    {
        auto iter = eastl::find_if( vector.begin(), vector.end(), predicate );
        if ( iter == vector.end() )
        {
            return InvalidIndex;
        }
        else
        {
            return (int32_t) ( iter - vector.begin() );
        }
    }

    //-------------------------------------------------------------------------

    template<typename T>
    inline void VectorEmplaceBackUnique( TVector<T>& vector, T&& item )
    {
        if ( !VectorContains( vector, item ) )
        {
            vector.emplace_back( item );
        }
    }

    template<typename T>
    inline void VectorEmplaceBackUnique( TVector<T>& vector, T const& item )
    {
        if ( !VectorContains( vector, item ) )
        {
            vector.emplace_back( item );
        }
    }

    template<typename T, eastl_size_t S>
    inline void VectorEmplaceBackUnique( TInlineVector<T, S>& vector, T&& item )
    {
        if ( !VectorContains( vector, item ) )
        {
            vector.emplace_back( item );
        }
    }

    template<typename T, eastl_size_t S>
    inline void VectorEmplaceBackUnique( TInlineVector<T, S>& vector, T const& item )
    {
        if ( !VectorContains( vector, item ) )
        {
            vector.emplace_back( item );
        }
    }

    //-------------------------------------------------------------------------

    template<typename T, typename LessThanPredicate>
    int32_t VectorFindSortedInsertIndex( T const* pData, int32_t numElements, T const& item, LessThanPredicate isLessThan )
    {
        int32_t insertIdx = 0;
        for ( int32_t i = 0; i < numElements; i++ )
        {
            insertIdx++;

            if ( isLessThan( item, pData[i] ) )
            {
                insertIdx--;
                break;
            }
        }

        return insertIdx;
    }

    // Find the insertion index in an already sorted array
    template<typename T, typename LessThanPredicate>
    inline int32_t VectorFindSortedInsertIndex( TVector<T>& vector, T&& item, LessThanPredicate isLessThan )
    {
        return VectorFindSortedInsertIndex( vector.data(), (int32_t) vector.size(), item, isLessThan );
    }

    // Find the insertion index in an already sorted array
    template<typename T, typename LessThanPredicate>
    inline int32_t VectorFindSortedInsertIndex( TVector<T>& vector, T const& item, LessThanPredicate isLessThan )
    {
        return VectorFindSortedInsertIndex( vector.data(), (int32_t) vector.size(), item, isLessThan );
    }

    // Find the insertion index in an already sorted array
    template<typename T, eastl_size_t S, typename LessThanPredicate>
    inline int32_t VectorFindSortedInsertIndex( TInlineVector<T, S>& vector, T&& item, LessThanPredicate isLessThan )
    {
        return VectorFindSortedInsertIndex( vector.data(), (int32_t) vector.size(), item, isLessThan );
    }

    // Find the insertion index in an already sorted array
    template<typename T, eastl_size_t S, typename LessThanPredicate>
    inline int32_t VectorFindSortedInsertIndex( TInlineVector<T, S>& vector, T const& item, LessThanPredicate isLessThan )
    {
        return VectorFindSortedInsertIndex( vector.data(), (int32_t) vector.size(), item, isLessThan );
    }

    //-------------------------------------------------------------------------

    template<typename T, typename LessThanPredicate>
    inline T& VectorInsertSorted( TVector<T>& vector, T&& item, LessThanPredicate isLessThan )
    {
        int32_t const insertIdx = VectorFindSortedInsertIndex( vector, item, isLessThan );
        vector.insert( vector.begin() + insertIdx, eastl::move( item ) );
        return vector[insertIdx];
    }

    template<typename T, typename LessThanPredicate>
    inline T& VectorInsertSorted( TVector<T>& vector, T const& item, LessThanPredicate isLessThan )
    {
        int32_t const insertIdx = VectorFindSortedInsertIndex( vector, item, isLessThan );
        vector.insert( vector.begin() + insertIdx, item );
        return vector[insertIdx];
    }

    template<typename T, eastl_size_t S, typename LessThanPredicate>
    inline T& VectorInsertSorted( TInlineVector<T, S>& vector, T&& item, LessThanPredicate isLessThan )
    {
        int32_t const insertIdx = VectorFindSortedInsertIndex( vector, item, isLessThan );
        vector.insert( vector.begin() + insertIdx, eastl::move( item ) );
        return vector[insertIdx];
    }

    template<typename T, eastl_size_t S, typename LessThanPredicate>
    inline T& VectorInsertSorted( TInlineVector<T, S>& vector, T const& item, LessThanPredicate isLessThan )
    {
        int32_t const insertIdx = VectorFindSortedInsertIndex( vector, item, isLessThan );
        vector.insert( vector.begin() + insertIdx, item );
        return vector[insertIdx];
    }

    //-------------------------------------------------------------------------

    template<typename T>
    inline void VectorEraseUnordered( TVector<T>& vector, size_t index )
    {
        if ( index < vector.size() )
        {
            eastl::swap( vector[index], vector.back() );
            vector.pop_back();
        }
    }

    template<typename T>
    inline void VectorEraseUnordered( TVector<T>& vector, typename TVector<T>::iterator&& iterator )
    {
        size_t index = iterator - vector.begin();
        VectorEraseUnordered( vector, index );
        iterator = {};
    }

    //-------------------------------------------------------------------------

    template<typename Iterator0, typename Iterator1>
    constexpr bool VectorEqual( Iterator0 first0, Iterator0 last0, Iterator1 first1, Iterator1 last1 )
    {
        if ( last0 - first0 != last1 - first1 )
        {
            return false;
        }

        for ( ; first0 != last0; ++first0, ++first1 )
        {
            if ( *first0 != *first1 )
            {
                return false;
            }
        }

        return true;
    }

    template<typename Iterator0, typename Iterator1, typename Predicate>
    constexpr bool VectorEqual( Iterator0 first0, Iterator0 last0, Iterator1 first1, Iterator1 last1, Predicate p )
    {
        if ( last0 - first0 != last1 - first1 )
        {
            return false;
        }

        for ( ; first0 != last0; ++first0, ++first1 )
        {
            if ( !p( *first0, *first1 ) )
            {
                return false;
            }
        }

        return true;
    }

    template<typename T>
    constexpr bool VectorEqual( TVector<T> const& vector0, TVector<T> const& vector1 )
    {
        return VectorEqual( vector0.begin(), vector0.end(), vector1.begin(), vector1.end() );
    }

    template<typename T, typename Predicate>
    constexpr bool VectorEqual( TVector<T> const& vector0, TVector<T> const& vector1, Predicate p )
    {
        return VectorEqual( vector0.begin(), vector0.end(), vector1.begin(), vector1.end(), p );
    }
}
