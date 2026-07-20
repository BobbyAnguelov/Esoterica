#include "Memory.h"

//-------------------------------------------------------------------------

#if EE_USE_CUSTOM_ALLOCATOR
#include "Base/ThirdParty/rpmalloc/rpmalloc.h"
#else
#include <stdlib.h>
#endif

#include <immintrin.h>

#ifdef _WIN32
// TODO: Get rid of this
#include <windows.h>
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

        #if EE_USE_CUSTOM_ALLOCATOR
        static rpmalloc_config_t g_rpmallocConfig;
        #endif

        //-------------------------------------------------------------------------

        static void CustomAssert( char const* pMessage )
        {
            EE_TRACE_HALT( pMessage );
        }

        void Initialize()
        {
            EE_ASSERT( !g_isMemorySystemInitialized );

            // Init config
            #if EE_USE_CUSTOM_ALLOCATOR
            memset( &g_rpmallocConfig, 0, sizeof( rpmalloc_config_t ) );
            g_rpmallocConfig.error_callback = &CustomAssert;

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

        bool HasInitializedThreadHeap()
        {
            // Since our tasks are not bound to a specific thread and we may alloc on one and free on another. This prevents us from calling thread finalize when we shutdown a thread
            // as we can not guarantee that we have freed everything that may have been allocated from this thread.
            // This is not a problem since on application shutdown, we call rpmalloc_finalize, which will release the thread heaps
            #if EE_USE_CUSTOM_ALLOCATOR
            return rpmalloc_is_thread_initialized() != 0;
            #else
            return true;
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

        void CopyToWriteCombined( void* __restrict pDst_WriteCombined, void const* __restrict pSrc, size_t const numBytes )
        {
            EE_ASSERT( ( ( numBytes % 4 ) == 0 ) );
            EE_ASSERT( ( intptr_t( pDst_WriteCombined ) & 31 ) == 0 );
            EE_ASSERT( ( intptr_t( pSrc ) & 31 ) == 0 );

            size_t const numBlocks256 = numBytes / 32;
            size_t const unrolledCopiesStart256 = numBlocks256 - ( numBlocks256 % 4 );

            // The amount of 128 bit copies should always be 0 or 1, because 2 128 bit copies are handled by a 256 bit copy.
            size_t const numBlocks128 = ( numBytes % 32 ) / 16;
            EE_ASSERT( numBlocks128 <= 1 );

            // The amount of 32 bit copies should always be 0, 1, 2 or 3 because 4 32 bit copiies are handled by a 128 bit copy.
            size_t const numBlocks32 = ( numBytes - numBlocks256 * 32 - numBlocks128 * 16 ) / 4;
            EE_ASSERT( numBlocks32 <= 3 );

            __m256i* __restrict pDst256 = static_cast<__m256i * __restrict>( pDst_WriteCombined );
            __m256i const* __restrict pSrc256 = static_cast<__m256i const* __restrict>( pSrc );

            __m128i* __restrict pDst128 = reinterpret_cast<__m128i * __restrict>( pDst256 + numBlocks256 );
            __m128i const* __restrict pSrc128 = reinterpret_cast<__m128i const* __restrict>( pSrc256 + numBlocks256 );

            int32_t* __restrict pDst32 = reinterpret_cast<int32_t * __restrict>( pDst128 + numBlocks128 );
            int32_t const* __restrict pSrc32 = reinterpret_cast<int32_t const* __restrict>( pSrc128 + numBlocks128 );

            // Validate pointer bounds
            EE_ASSERT( reinterpret_cast<uint8_t * __restrict>( pDst32 + numBlocks32 ) == ( static_cast<uint8_t * __restrict>( pDst_WriteCombined ) + numBytes ) );
            EE_ASSERT( reinterpret_cast<uint8_t const* __restrict>( pSrc32 + numBlocks32 ) == ( static_cast<uint8_t const* __restrict>( pSrc ) + numBytes ) );

            // Copy unrolled 256-bit blocks
            for ( size_t blockIndex = 0; blockIndex < unrolledCopiesStart256; blockIndex += 4 )
            {
                __m256i const block0 = _mm256_load_si256( pSrc256 + blockIndex + 0 );
                __m256i const block1 = _mm256_load_si256( pSrc256 + blockIndex + 1 );
                __m256i const block2 = _mm256_load_si256( pSrc256 + blockIndex + 2 );
                __m256i const block3 = _mm256_load_si256( pSrc256 + blockIndex + 3 );

                _mm256_stream_si256( pDst256 + blockIndex + 0, block0 );
                _mm256_stream_si256( pDst256 + blockIndex + 1, block1 );
                _mm256_stream_si256( pDst256 + blockIndex + 2, block2 );
                _mm256_stream_si256( pDst256 + blockIndex + 3, block3 );
            }

            // Copy remaining 256-bit blocks
            for ( size_t blockIndex = unrolledCopiesStart256; blockIndex < numBlocks256; ++blockIndex )
            {
                _mm256_stream_si256( pDst256 + blockIndex, _mm256_load_si256( pSrc256 + blockIndex ) );
            }

            // Copy remaining 128-bit block. It should have up to 1 block.
            if ( numBlocks128 )
            {
                _mm_stream_si128( pDst128, _mm_load_si128( pSrc128 ) );
            }

            // Copy remaining 32-bit blocks. It should have up to 3 blocks.
            for ( size_t blockIndex = 0; blockIndex < numBlocks32; ++blockIndex )
            {
                _mm_stream_si32( pDst32 + blockIndex, pSrc32[blockIndex] );
            }
        }

        void CopyFromWriteCombined( void* __restrict pDst, void const* __restrict pSrc_WriteCombined, size_t const numBytes )
        {
            EE_ASSERT( ( ( numBytes % 4 ) == 0 ) );
            EE_ASSERT( ( intptr_t( pDst ) & 31 ) == 0 );
            EE_ASSERT( ( intptr_t( pSrc_WriteCombined ) & 31 ) == 0 );

            size_t const numBlocks256 = numBytes / 32;
            size_t const unrolledCopiesStart256 = numBlocks256 - ( numBlocks256 % 4 );

            // The amount of 128 bit copies should always be 0 or 1, because 2 128 bit copies are handled by a 256 bit copy.
            size_t const numBlocks128 = ( numBytes % 32 ) / 16;
            EE_ASSERT( numBlocks128 <= 1 );

            // The amount of 32 bit copies should always be 0, 1, 2 or 3 because 4 32 bit copiies are handled by a 128 bit copy.
            size_t const numBlocks32 = ( numBytes - numBlocks256 * 32 - numBlocks128 * 16 ) / 4;
            EE_ASSERT( numBlocks32 <= 3 );

            __m256i* __restrict pDst256 = static_cast<__m256i * __restrict>( pDst );
            __m256i const* __restrict pSrc256 = static_cast<__m256i const* __restrict>( pSrc_WriteCombined );

            __m128i* __restrict pDst128 = reinterpret_cast<__m128i * __restrict>( pDst256 + numBlocks256 );
            __m128i const* __restrict pSrc128 = reinterpret_cast<__m128i const* __restrict>( pSrc256 + numBlocks256 );

            int32_t* __restrict pDst32 = reinterpret_cast<int32_t * __restrict>( pDst128 + numBlocks128 );
            int32_t const* __restrict pSrc32 = reinterpret_cast<int32_t const* __restrict>( pSrc128 + numBlocks128 );

            // Validate pointer bounds
            EE_ASSERT( reinterpret_cast<uint8_t * __restrict>( pDst32 + numBlocks32 ) == ( static_cast<uint8_t * __restrict>( pDst ) + numBytes ) );
            EE_ASSERT( reinterpret_cast<uint8_t const* __restrict>( pSrc32 + numBlocks32 ) == ( static_cast<uint8_t const* __restrict>( pSrc_WriteCombined ) + numBytes ) );

            // Copy unrolled 256-bit blocks
            for ( size_t blockIndex = 0; blockIndex < unrolledCopiesStart256; blockIndex += 4 )
            {
                __m256i const block0 = _mm256_stream_load_si256( pSrc256 + blockIndex + 0 );
                __m256i const block1 = _mm256_stream_load_si256( pSrc256 + blockIndex + 1 );
                __m256i const block2 = _mm256_stream_load_si256( pSrc256 + blockIndex + 2 );
                __m256i const block3 = _mm256_stream_load_si256( pSrc256 + blockIndex + 3 );

                _mm256_store_si256( pDst256 + blockIndex + 0, block0 );
                _mm256_store_si256( pDst256 + blockIndex + 1, block1 );
                _mm256_store_si256( pDst256 + blockIndex + 2, block2 );
                _mm256_store_si256( pDst256 + blockIndex + 3, block3 );
            }

            // Copy remaining 256-bit blocks
            for ( size_t blockIndex = unrolledCopiesStart256; blockIndex < numBlocks256; ++blockIndex )
            {
                _mm256_store_si256( pDst256 + blockIndex, _mm256_stream_load_si256( pSrc256 + blockIndex ) );
            }

            // Copy remaining 128-bit block. It should have up to 1 block.
            if ( numBlocks128 )
            {
                _mm_store_si128( pDst128, _mm_stream_load_si128( pSrc128 ) );
            }

            // Copy remaining 32-bit blocks. It should have up to 3 blocks.
            for ( size_t blockIndex = 0; blockIndex < numBlocks32; ++blockIndex )
            {
                *( pDst32 + blockIndex ) = pSrc32[blockIndex];
            }
        }

        void WriteCombinedBarrier()
        {
            _mm_sfence();
        }

        //-------------------------------------------------------------------------

        static int64_t volatile s_totalVirtualMemoryCommitted = 0;

        void* VirtualMemoryReserve( size_t size )
        {
            EE_ASSERT( size );
            return VirtualAlloc( 0, size, MEM_RESERVE, PAGE_READWRITE );
        }

        void VirtualMemoryCommit( void* pStart, size_t size )
        {
            EE_ASSERT( pStart );
            EE_ASSERT( size );
            VirtualAlloc( pStart, size, MEM_COMMIT, PAGE_READWRITE );
            InterlockedAdd64( &s_totalVirtualMemoryCommitted, size );
        }

        void VirtualMemoryFree( void* pMemory, size_t reservedSize, size_t committedSize )
        {
            EE_ASSERT( pMemory );
            EE_ASSERT( reservedSize );
            EE_ASSERT( committedSize );
            VirtualFree( pMemory, 0, MEM_RELEASE );
            InterlockedAdd64( &s_totalVirtualMemoryCommitted, -int64_t( committedSize ) );
        }

        //-------------------------------------------------------------------------

        size_t GetTotalRequestedMemory()
        {
            #if EE_USE_CUSTOM_ALLOCATOR
            rpmalloc_global_statistics_t stats;
            rpmalloc_global_statistics( &stats );
            return stats.mapped + s_totalVirtualMemoryCommitted;
            #else 
            return 0;
            #endif
        }

        size_t GetTotalAllocatedMemory()
        {
            #if EE_USE_CUSTOM_ALLOCATOR
            rpmalloc_global_statistics_t stats;
            rpmalloc_global_statistics( &stats );
            return stats.mapped_total + s_totalVirtualMemoryCommitted;
            #else
            return 0;
            #endif
        }

        size_t GetAllocationSize( void* pMemory )
        {
            #if EE_USE_CUSTOM_ALLOCATOR
            return rpmalloc_usable_size( pMemory );
            #else
            return 0;
            #endif
        }

        size_t GetVirtualMemoryCommitted()
        {
            return size_t( s_totalVirtualMemoryCommitted );
        }

        //-------------------------------------------------------------------------

        namespace Allocators
        {
            MemoryAllocator g_global( "Global" );
        }
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    Memory::MemoryAllocator::MemoryAllocator( char const* pName )
        : m_pName( pName )
    {
        EE_ASSERT( !EE::Memory::g_isMemorySystemInitialized );
    }

    Memory::MemoryAllocator::~MemoryAllocator()
    {
        // These should echo the memory leak asserts
        EE_ASSERT( m_numAllocations.load() == 0 );
        EE_ASSERT( m_numBytes.load() == 0 );
    }

    uint64_t Memory::MemoryAllocator::ClaimAccumulatedBytes()
    {
        return m_numBytesAccumulated.exchange( 0 );
    }

    uint64_t Memory::MemoryAllocator::ClaimAccumulatedAllocations()
    {
        return m_numAllocationsAccumulated.exchange( 0 );
    }
    #endif

    void* Memory::MemoryAllocator::Alloc( size_t size, size_t alignment )
    {
        EE_ASSERT( EE::Memory::g_isMemorySystemInitialized );

        if ( size == 0 ) return nullptr;

        void* pMemory = nullptr;

        #if EE_USE_CUSTOM_ALLOCATOR
        pMemory = rpaligned_alloc( alignment, size );

        #if EE_DEVELOPMENT_TOOLS
        size_t usableSize = rpmalloc_usable_size( pMemory );
        m_numBytes.fetch_add( usableSize );
        m_numAllocations.fetch_add( 1 );

        m_numBytesAccumulated.fetch_add( usableSize );
        m_numAllocationsAccumulated.fetch_add( 1 );
        #endif

        #else
        pMemory = _aligned_malloc( size, alignment );
        #endif

        EE_ASSERT( Memory::IsAligned( pMemory, alignment ) );
        return pMemory;
    }

    void* Memory::MemoryAllocator::Realloc( void* pMemory, size_t newSize, size_t originalAlignment )
    {
        EE_ASSERT( EE::Memory::g_isMemorySystemInitialized );

        void* pReallocatedMemory = nullptr;

        #if EE_USE_CUSTOM_ALLOCATOR

        #if EE_DEVELOPMENT_TOOLS
        if ( pMemory )
        {
            size_t usableSize = rpmalloc_usable_size( pMemory );
            m_numBytes.fetch_sub( usableSize );
            m_numAllocations.fetch_sub( 1 );
        }
        #endif

        pReallocatedMemory = rprealloc( pMemory, newSize );
        #else
        pReallocatedMemory = _aligned_realloc( pMemory, newSize, originalAlignment );
        #endif

        #if EE_USE_CUSTOM_ALLOCATOR
        #if EE_DEVELOPMENT_TOOLS
        size_t usableSize = rpmalloc_usable_size( pReallocatedMemory );
        m_numBytes.fetch_add( usableSize );
        m_numAllocations.fetch_add( 1 );

        m_numBytesAccumulated.fetch_add( usableSize );
        m_numAllocationsAccumulated.fetch_add( 1 );
        #endif
        #endif

        EE_ASSERT( pReallocatedMemory != nullptr );
        return pReallocatedMemory;
    }

    void Memory::MemoryAllocator::Free( void*& pMemory )
    {
        EE_ASSERT( EE::Memory::g_isMemorySystemInitialized );

        if ( !pMemory )
        {
            return;
        }

        #if EE_USE_CUSTOM_ALLOCATOR
        #if EE_DEVELOPMENT_TOOLS
        size_t usableSize = rpmalloc_usable_size( pMemory );
        m_numBytes.fetch_sub( usableSize );
        m_numAllocations.fetch_sub( 1 );
        #endif

        rpfree( (uint8_t*) pMemory );
        #else
        _aligned_free( pMemory );
        #endif

        pMemory = nullptr;
    }
}
