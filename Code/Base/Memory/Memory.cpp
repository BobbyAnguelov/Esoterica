#include "Memory.h"

//-------------------------------------------------------------------------

#if EE_USE_CUSTOM_ALLOCATOR
    #include "Base/ThirdParty/rpmalloc/rpmalloc.h"
#else
    #include <stdlib.h>
#endif

//-------------------------------------------------------------------------
// Note: We dont globally overload the new or delete operators
//-------------------------------------------------------------------------
// Right now we use the default system allocators for new/delete calls and static allocations
// If in the future, we hit performance issues with allocation then we can consider globally overloading those operators
//-------------------------------------------------------------------------

namespace EE
{
    namespace Memory
    {
        static bool g_isMemorySystemInitialized = false;
        static rpmalloc_config_t g_rpmallocConfig;

        //-------------------------------------------------------------------------

        static void CustomAssert( char const* pMessage )
        {
            EE_TRACE_HALT( pMessage );
        }

        void Initialize()
        {
            EE_ASSERT( !g_isMemorySystemInitialized );

            // Init config
            memset( &g_rpmallocConfig, 0, sizeof( rpmalloc_config_t ) );
            g_rpmallocConfig.error_callback = &CustomAssert;

            #if EE_USE_CUSTOM_ALLOCATOR
            rpmalloc_initialize_config( &g_rpmallocConfig );
            #endif

            g_isMemorySystemInitialized = true;
        }

        void Shutdown()
        {
            EE_ASSERT( g_isMemorySystemInitialized );
            g_isMemorySystemInitialized = false;

            #if EE_USE_CUSTOM_ALLOCATOR
            rpmalloc_finalize();
            #endif
        }

        void InitializeThreadHeap()
        {
            // Since our tasks are not bound to a specific thread and we may alloc on one and free on another. This prevents us from calling thread finalize when we shutdown a thread
            // as we can not guarantee that we have freed everything that may have been allocated from this thread.
            // This is not a problem since on application shutdown, we call rpmalloc_finalize, which will release the thread heaps
            #if EE_USE_CUSTOM_ALLOCATOR
            rpmalloc_thread_initialize();
            #endif
        }

        void ShutdownThreadHeap()
        {
            #if EE_USE_CUSTOM_ALLOCATOR
            rpmalloc_thread_finalize( 1 );
            #endif
        }

        //-------------------------------------------------------------------------

        size_t GetTotalRequestedMemory()
        {
            #if EE_USE_CUSTOM_ALLOCATOR
            rpmalloc_global_statistics_t stats;
            rpmalloc_global_statistics( &stats );
            return stats.mapped;
            #else 
            return 0;
            #endif
        }

        size_t GetTotalAllocatedMemory()
        {
            #if EE_USE_CUSTOM_ALLOCATOR
            rpmalloc_global_statistics_t stats;
            rpmalloc_global_statistics( &stats );
            return stats.mapped_total;
            #else
            return 0;
            #endif
        }
    }

    //-------------------------------------------------------------------------

    void* Alloc( size_t size, size_t alignment )
    {
        EE_ASSERT( EE::Memory::g_isMemorySystemInitialized );

        if ( size == 0 ) return nullptr;

        void* pMemory = nullptr;

        #if EE_USE_CUSTOM_ALLOCATOR
        pMemory = rpaligned_alloc( alignment, size );
        #elif _WIN32
        pMemory = _aligned_malloc( size, alignment );
        #endif

        EE_ASSERT( Memory::IsAligned( pMemory, alignment ) );
        return pMemory;
    }

    void* Realloc( void* pMemory, size_t newSize, size_t originalAlignment )
    {
        EE_ASSERT( EE::Memory::g_isMemorySystemInitialized );

        void* pReallocatedMemory = nullptr;

        #if EE_USE_CUSTOM_ALLOCATOR
        pReallocatedMemory = rprealloc( pMemory, newSize );
        #elif _WIN32
        pReallocatedMemory = _aligned_realloc( pMemory, newSize, originalAlignment );
        #endif

        EE_ASSERT( pReallocatedMemory != nullptr );
        return pReallocatedMemory;
    }

    void Free( void*& pMemory )
    {
        EE_ASSERT( EE::Memory::g_isMemorySystemInitialized );

        #if EE_USE_CUSTOM_ALLOCATOR
        rpfree( (uint8_t*) pMemory );
        #elif _WIN32
        _aligned_free( pMemory );
        #endif

        pMemory = nullptr;
    }
}