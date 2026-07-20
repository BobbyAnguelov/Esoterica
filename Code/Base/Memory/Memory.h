#pragma once

#include "Base/_Module/API.h"
#include "Base/Esoterica.h"
#include "Base/Utils/GlobalRegistryBase.h"
#include "EASTL/atomic.h"
#include <cstring>
#include <malloc.h>
#include <utility>

//-------------------------------------------------------------------------

#define EE_USE_CUSTOM_ALLOCATOR 1
#define EE_DEFAULT_ALIGNMENT 16 // natural alignment of rpmalloc

//-------------------------------------------------------------------------

#ifdef _WIN32

#define EE_STACK_ALLOC( x ) alloca( x )
#define EE_STACK_ARRAY_ALLOC( type, numElements ) reinterpret_cast<type*>( alloca( sizeof( type ) * numElements ) );

#else

#define EE_STACK_ALLOC( x )
#define EE_STACK_ARRAY_ALLOC( type, numElements )

#endif

//-------------------------------------------------------------------------

namespace EE
{
    namespace Memory
    {
        EE_BASE_API void Initialize();
        EE_BASE_API void Shutdown();

        // Check that we have an initialized thread heap for a given thread
        EE_BASE_API bool HasInitializedThreadHeap();

        // Initialize the thread heap for a given thread if the heap is not already initialized
        EE_BASE_API void InitializeThreadHeap();

        // Shutdown the thread heap for a given thread
        EE_BASE_API void ShutdownThreadHeap();

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE void MemsetZero( void* ptr, size_t size )
        {
            memset( ptr, 0, size );
        }

        template<typename T>
        EE_FORCE_INLINE void MemsetZero( T* ptr )
        {
            memset( ptr, 0, sizeof( T ) );
        }

        //-------------------------------------------------------------------------

        // Copies numBytes from pSrc to pDst bypassing the CPU cache and without synchronization.
        // both pDst and pSrc must be aligned to 32 bytes, this function will assert if any of these conditions are not met.
        // Results are not visible until WriteCombinedBarrier() is called.
        // Use this to copy TO write-combined memory from RAM.
        EE_BASE_API void CopyToWriteCombined( void* __restrict pDst_WriteCombined, void const* __restrict pSrc, size_t const numBytes );

        // Same as StreamingMemcpy() but copies FROM write-combined memory to RAM.
        EE_BASE_API void CopyFromWriteCombined( void* __restrict pDst, void const* __restrict pSrc_WriteCombined, size_t const numBytes );

        // Synchronizes all write-combined memcopies and makes results visible for the program
        EE_BASE_API void WriteCombinedBarrier();

        //-------------------------------------------------------------------------

        EE_BASE_API void* VirtualMemoryReserve( size_t size );
        EE_BASE_API void  VirtualMemoryCommit( void* pStart, size_t size );
        EE_BASE_API void  VirtualMemoryFree( void* pMemory, size_t reservedSize, size_t committedSize );

        template<typename T>
        inline T* VirtualMemoryReserve( size_t numElements )
        {
            return reinterpret_cast<T*>( VirtualMemoryReserve( numElements * sizeof( T ) ) );
        }

        template<typename T>
        inline void VirtualMemoryCommit( T* pStart, size_t numElements )
        {
            VirtualMemoryCommit( static_cast<void*>( pStart ), numElements * sizeof( T ) );
        }

        template<typename T>
        void VirtualMemoryFree( T* pMemory, size_t numElementsReserved, size_t numElementsCommitted )
        {
            VirtualMemoryFree( static_cast<void*>( pMemory ), numElementsReserved * sizeof( T ), numElementsCommitted * sizeof( T ) );
        }

        //-------------------------------------------------------------------------

        inline bool IsAligned( void const* p, size_t n )
        {
            return ( reinterpret_cast<uintptr_t>( p ) % n ) == 0;
        }

        template<typename T>
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

        EE_BASE_API size_t GetTotalRequestedMemory();
        EE_BASE_API size_t GetTotalAllocatedMemory();
        EE_BASE_API size_t GetVirtualMemoryCommitted();

        // Query the usable size of a previously allocated block
        EE_BASE_API size_t GetAllocationSize( void* pMemory );
    }

    //-------------------------------------------------------------------------
    // Global Memory Allocator
    //-------------------------------------------------------------------------

    namespace Memory
    {
        #if EE_DEVELOPMENT_TOOLS
        class alignas( 64 ) EE_BASE_API MemoryAllocator : public TGlobalRegistryBase<MemoryAllocator>
        {
            EE_GLOBAL_REGISTRY( MemoryAllocator );

        public:

            static MemoryAllocator* GetHead() { return s_pHead; }

        public:

            MemoryAllocator( char const* pName );
            ~MemoryAllocator();

            EE_FORCE_INLINE char const* GetName() const { return m_pName; }
            using TGlobalRegistryBase<MemoryAllocator>::GetNextItem;

            EE_FORCE_INLINE uint64_t GetNumBytes() const { return m_numBytes.load(); }
            EE_FORCE_INLINE uint64_t GetNumAllocations() const { return m_numAllocations.load(); }

            void* Alloc( size_t size, size_t alignment );
            void* Realloc( void* pMemory, size_t newSize, size_t originalAlignment );
            void Free( void*& pMemory );

            uint64_t ClaimAccumulatedBytes();
            uint64_t ClaimAccumulatedAllocations();

        private:

            // These atomics need to be FIRST in the memory layout - aligned to cache line!
            eastl::atomic<uint64_t>     m_numBytes = 0;
            eastl::atomic<uint64_t>     m_numAllocations = 0;
            eastl::atomic<uint64_t>     m_numBytesAccumulated = 0;
            eastl::atomic<uint64_t>     m_numAllocationsAccumulated = 0;

            char const*                 m_pName = nullptr;

            uint64_t                    m_padding[2] = {};
        };
        #else
        class EE_BASE_API MemoryAllocator
        {
        public:

            MemoryAllocator( char const* pName ) {};

            static void* Alloc( size_t size, size_t alignment );
            static void* Realloc( void* pMemory, size_t newSize, size_t originalAlignment );
            static void Free( void*& pMemory );
        };
        #endif

        // Global Allocator
        //-------------------------------------------------------------------------

        namespace Allocators
        {
            EE_BASE_API extern MemoryAllocator g_global;
        }
    }

    //-------------------------------------------------------------------------
    // Global Memory Management Functions
    //-------------------------------------------------------------------------

    [[nodiscard]] EE_FORCE_INLINE void* Alloc( size_t size, size_t alignment = EE_DEFAULT_ALIGNMENT )
    {
        return Memory::Allocators::g_global.Alloc( size, alignment );
    }

    [[nodiscard]] EE_FORCE_INLINE void* Realloc( void* pMemory, size_t newSize, size_t originalAlignment = EE_DEFAULT_ALIGNMENT )
    {
        return Memory::Allocators::g_global.Realloc( pMemory, newSize, originalAlignment );
    }

    EE_FORCE_INLINE void Free( void*& pMemory )
    {
        Memory::Allocators::g_global.Free( pMemory );
    }

    //-------------------------------------------------------------------------

    template<typename T, typename... ConstructorParams>
    [[nodiscard]] EE_FORCE_INLINE T* New( ConstructorParams&&... params )
    {
        void* pMemory = Alloc( sizeof( T ), alignof( T ) );
        EE_ASSERT( pMemory != nullptr );
        return new ( pMemory ) T( std::forward<ConstructorParams>( params )... );
    }

    template<typename T>
    EE_FORCE_INLINE void Delete( T*& pType )
    {
        if ( pType != nullptr )
        {
            pType->~T();
            Free( (void*&) pType );
        }
    }

    template<typename T>
    EE_FORCE_INLINE void Free( T*& pType )
    {
        Free( (void*&) pType );
    }

    //-------------------------------------------------------------------------

    template<typename T, typename... ConstructorParams>
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
            new ( &pArrayAddress[i] ) T( std::forward<ConstructorParams>( params )... );
        }

        // Record the number of array elements
        uint32_t* pNumElements = reinterpret_cast<uint32_t*>( pArrayAddress ) - 1;
        *pNumElements = uint32_t( numElements );

        return reinterpret_cast<T*>( pArrayAddress );
    }

    template<typename T>
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
            new ( &pArrayAddress[i] ) T( value );
        }

        // Record the number of array elements
        uint32_t* pNumElements = reinterpret_cast<uint32_t*>( pArrayAddress ) - 1;
        *pNumElements = uint32_t( numElements );

        return pArrayAddress;
    }

    template<typename T>
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
