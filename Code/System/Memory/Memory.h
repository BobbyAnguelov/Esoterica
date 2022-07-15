#pragma once

#include "System/_Module/API.h"
#include "System/Esoterica.h"
#include <cstring>
#include <malloc.h>
#include <utility>

//-------------------------------------------------------------------------

#define EE_USE_CUSTOM_ALLOCATOR 1
#define EE_DEFAULT_ALIGNMENT 8

//-------------------------------------------------------------------------

#ifdef _WIN32

    #define EE_STACK_ALLOC(x) alloca( x )
    #define EE_STACK_ARRAY_ALLOC(type, numElements) reinterpret_cast<type*>( alloca( sizeof(type) * numElements ) );

#else

    #define EE_STACK_ALLOC(x)
    #define EE_STACK_ARRAY_ALLOC(type, numElements)

#endif

//-------------------------------------------------------------------------

namespace EE
{
    namespace Memory
    {
        EE_SYSTEM_API void Initialize();
        EE_SYSTEM_API void Shutdown();

        EE_SYSTEM_API void InitializeThreadHeap();
        EE_SYSTEM_API void ShutdownThreadHeap();

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE void MemsetZero( void* ptr, size_t size )
        {
            memset( ptr, 0, size );
        }

        template <typename T>
        EE_FORCE_INLINE void MemsetZero( T* ptr )
        {
            memset( ptr, 0, sizeof( T ) );
        }

        //-------------------------------------------------------------------------

        inline bool IsAligned( void const* p, size_t n )
        {
            return ( reinterpret_cast<uintptr_t>( p ) % n ) == 0;
        }

        template <typename T>
        inline bool IsAligned( T const* p )
        {
            return ( reinterpret_cast<uintptr_t>( p ) % alignof( T ) ) == 0;
        }

        EE_FORCE_INLINE size_t CalculatePaddingForAlignment( uintptr_t addressOffset, size_t requiredAlignment )
        {
            return ( requiredAlignment - ( addressOffset % requiredAlignment ) ) % requiredAlignment;
        }

        EE_FORCE_INLINE size_t CalculatePaddingForAlignment( void* address, size_t requiredAlignment )
        {
            return CalculatePaddingForAlignment( reinterpret_cast<uintptr_t>( address ), requiredAlignment );
        }

        //-------------------------------------------------------------------------

        EE_SYSTEM_API size_t GetTotalRequestedMemory();
        EE_SYSTEM_API size_t GetTotalAllocatedMemory();
    }

    //-------------------------------------------------------------------------
    // Global Memory Management Functions
    //-------------------------------------------------------------------------

    [[nodiscard]] EE_SYSTEM_API void* Alloc( size_t size, size_t alignment = EE_DEFAULT_ALIGNMENT );
    [[nodiscard]] EE_SYSTEM_API void* Realloc( void* pMemory, size_t newSize, size_t originalAlignment = EE_DEFAULT_ALIGNMENT );
    EE_SYSTEM_API void Free( void*& pMemory );

    //-------------------------------------------------------------------------

    template< typename T, typename ... ConstructorParams >
    [[nodiscard]] EE_FORCE_INLINE T* New( ConstructorParams&&... params )
    {
        void* pMemory = Alloc( sizeof( T ), alignof( T ) );
        EE_ASSERT( pMemory != nullptr );
        return new( pMemory ) T( std::forward<ConstructorParams>( params )... );
    }

    template< typename T >
    EE_FORCE_INLINE void Delete( T*& pType )
    {
        if ( pType != nullptr )
        {
            pType->~T();
            Free( (void*&) pType );
        }
    }

    template< typename T >
    EE_FORCE_INLINE void Free( T*& pType )
    {
        Free( (void*&) pType );
    }

    //-------------------------------------------------------------------------

    template< typename T, typename ... ConstructorParams >
    [[nodiscard]] EE_FORCE_INLINE T* NewArray( size_t const numElements, ConstructorParams&&... params )
    {
        size_t const requiredAlignment = std::max( alignof( T ), size_t( 16 ) );
        size_t const requiredExtraMemory = std::max( requiredAlignment, size_t( 4 ) );
        size_t const requiredMemory = sizeof( T ) * numElements + requiredExtraMemory;

        uint8_t* pOriginalAddress = pOriginalAddress = (uint8_t*) Alloc( requiredMemory, requiredAlignment );
        EE_ASSERT( pOriginalAddress != nullptr );

        // Call required type constructors
        T* pArrayAddress = reinterpret_cast<T*>( pOriginalAddress + requiredExtraMemory );
        for ( size_t i = 0; i < numElements; i++ )
        {
            new( &pArrayAddress[i] ) T( std::forward<ConstructorParams>( params )... );
        }

        // Record the number of array elements
        uint32_t* pNumElements = reinterpret_cast<uint32_t*>( pArrayAddress ) - 1;
        *pNumElements = uint32_t( numElements );

        return reinterpret_cast<T*>( pArrayAddress );
    }

    template< typename T >
    [[nodiscard]] EE_FORCE_INLINE T* NewArray( size_t const numElements, T const& value )
    {
        size_t const requiredAlignment = std::max( alignof( T ), size_t( 16 ) );
        size_t const requiredExtraMemory = std::max( requiredAlignment, size_t( 4 ) );
        size_t const requiredMemory = sizeof( T ) * numElements + requiredExtraMemory;

        uint8_t* pOriginalAddress = pOriginalAddress = (uint8_t*) Alloc( requiredMemory, requiredAlignment );
        EE_ASSERT( pOriginalAddress != nullptr );

        // Call required type constructors
        T* pArrayAddress = reinterpret_cast<T*>( pOriginalAddress + requiredExtraMemory );
        for ( size_t i = 0; i < numElements; i++ )
        {
            new( &pArrayAddress[i] ) T( value );
        }

        // Record the number of array elements
        uint32_t* pNumElements = reinterpret_cast<uint32_t*>( pArrayAddress ) - 1;
        *pNumElements = uint32_t( numElements );

        return pArrayAddress;
    }

    template< typename T >
    EE_FORCE_INLINE void DeleteArray( T*& pArray )
    {
        size_t const requiredAlignment = std::max( alignof( T ), size_t( 16 ) );
        size_t const requiredExtraMemory = std::max( requiredAlignment, size_t( 4 ) );

        // Get number of elements in array and call destructor on each entity
        uint32_t const numElements = *( reinterpret_cast<uint32_t*>( pArray ) - 1 );
        for ( uint32_t i = 0; i < numElements; i++ )
        {
            pArray[i].~T();
        }

        uint8_t* pOriginalAddress = reinterpret_cast<uint8_t*>( pArray ) - requiredExtraMemory;
        Free( (void*&) pOriginalAddress );
        pArray = nullptr;
    }
}