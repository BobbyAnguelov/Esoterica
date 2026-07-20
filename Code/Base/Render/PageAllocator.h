
#pragma once

#include "Base/Memory/Memory.h"
#include "HandleAllocator.h"

namespace EE::Render
{
    // Memory pool backed by virtual memory, allocates items in 64 increment (each page is 64 items), guarantees stable pointers.
    // On platforms without virtual memory can be implemented with TVector<Page>, the interface is portable.
    template <typename T>
    class PageMemoryPool
    {
    public:

        inline void Initialize( uint32_t initialCapacityInPages, uint32_t numVirtualMemoryReservedElements = 1 << 30 )
        {
            EE_ASSERT( m_pPageMemory == nullptr );
            EE_ASSERT( ( numVirtualMemoryReservedElements % 64 ) == 0 );

            m_pageMemoryReserved = numVirtualMemoryReservedElements;
            m_pageMemoryComitted = initialCapacityInPages * 64;

            m_pPageMemory = Memory::VirtualMemoryReserve<T>( m_pageMemoryReserved );
            Memory::VirtualMemoryCommit( m_pPageMemory, m_pageMemoryComitted );
        }

        inline void Shutdown()
        {
            Memory::VirtualMemoryFree( m_pPageMemory, m_pageMemoryReserved, m_pageMemoryComitted );
        }

        inline void Commit( size_t requiredMemoryComitted )
        {
            EE_ASSERT( requiredMemoryComitted <= m_pageMemoryReserved );
            if ( m_pageMemoryComitted < requiredMemoryComitted )
            {
                Memory::VirtualMemoryCommit( m_pPageMemory + m_pageMemoryComitted, requiredMemoryComitted - m_pageMemoryComitted );
                m_pageMemoryComitted = requiredMemoryComitted;
            }
        }

        inline T* GetData()
        {
            return m_pPageMemory;
        }

        inline T const* GetData() const
        {
            return m_pPageMemory;
        }

        inline size_t GetPageMemoryComitted() const
        {
            return m_pageMemoryComitted;
        }

    private:

        T*     m_pPageMemory = nullptr;
        size_t m_pageMemoryComitted = 0;
        size_t m_pageMemoryReserved = 0;
    };

    // Allocates handles out of PageMemoryPool, guarantees stable pointers
    template<typename T, typename H>
    class PageAllocator
    {
    public:

        struct Handle
        {
            HandleAllocator<H>::Handle          m_handle;
            TArrayView<T>                       m_data;
        };

        inline void Initialize( uint32_t initialCapacityInPages, uint32_t numVirtualMemoryReservedElements = 1 << 30 )
        {
            EE_ASSERT( ( numVirtualMemoryReservedElements % 64 ) == 0 );

            m_memoryPool.Initialize( initialCapacityInPages, numVirtualMemoryReservedElements );
            m_handleAllocator.Initialize( initialCapacityInPages );
        }

        inline void Shutdown()
        {
            m_handleAllocator.Shutdown();
            m_memoryPool.Shutdown();
        }

        template<typename... Args>
        inline Handle Allocate( H numItems, Args&&... args )
        {
            EE_ASSERT( numItems );

            // Allocate ID
            //------------------------------------------------------------------
            typename HandleAllocator<H>::Handle handle = m_handleAllocator.Allocate( numItems );
            EE_ASSERT( handle.IsValid() );

            // Commit virtual memory if neededv
            //------------------------------------------------------------------
            size_t requiredMemoryComitted = m_handleAllocator.GetCapacityInPages() * 64;
            m_memoryPool.Commit( requiredMemoryComitted );

            // Construct the items and return the handle
            //------------------------------------------------------------------
            Handle result = {};
            result.m_handle = handle;
            result.m_data = TArrayView( m_memoryPool.GetData() + handle.m_offset, numItems );

            if constexpr ( !std::is_trivially_constructible_v<T> )
            {
                for ( T& item : result.m_data )
                {
                    new ( &item ) T( eastl::forward<Args>( args )... );
                }
            }

            return result;
        }

        inline void Deallocate( Handle&& handle )
        {
            EE_ASSERT( handle.m_handle.IsValid() );
            EE_ASSERT( handle.m_data.data() >= m_memoryPool.GetData() && handle.m_data.data() <= ( m_memoryPool.GetData() + m_memoryPool.GetPageMemoryComitted() ) );

            // Deallocate base handle
            //------------------------------------------------------------------
            m_handleAllocator.Deallocate( eastl::move( handle.m_handle ) );

            // Destruct items
            //------------------------------------------------------------------
            if constexpr ( !std::is_trivially_destructible_v<T> )
            {
                for ( T& item : handle.m_data )
                {
                    item.~T();
                }
            }

            handle = {};
        }

        template <typename Fn>
        inline void ForEachAllocatedItem( Fn fn ) const
        {
            for ( uint32_t pageIndex = 0; pageIndex < GetNumPages(); ++pageIndex )
            {
                uint64_t pageMask = GetPageData()[pageIndex];
                T const* pItemData = GetItemData( pageIndex );

                if ( pageMask )
                {
                    for ( uint64_t itemIndex = 0; itemIndex < GetNumItemsPerPage(); ++itemIndex )
                    {
                        uint64_t itemMask = 1ULL << itemIndex;

                        if ( pageMask & itemMask )
                        {
                            fn( pItemData + itemIndex );
                        }
                    }
                }
            }
        }

        inline uint32_t GetNumItemsPerPage() const { return 64; }
        inline size_t   GetCapacityInBytes() const { return GetNumPages() * GetNumItemsPerPage() * sizeof( T ); }
        inline uint32_t GetCapacityInItems() const { return GetNumPages() * GetNumItemsPerPage(); }
        inline uint32_t GetNumPages() const { return (uint32_t) m_handleAllocator.GetCapacityInPages(); }

        inline T const* GetItemData( size_t pageIndex ) const
        {
            EE_ASSERT( pageIndex < m_handleAllocator.GetCapacityInPages() );
            return m_memoryPool.GetData() + pageIndex * 64;
        }

        inline uint64_t const* GetPageData() const
        {
            return m_handleAllocator.GetPageData();
        }

        inline T* GetData()
        {
            return m_memoryPool.GetData();
        }

        inline T const* GetData() const
        {
            return m_memoryPool.GetData();
        }

    private:

        HandleAllocator<H>              m_handleAllocator;
        PageMemoryPool<T>               m_memoryPool;
    };
}
