#pragma once

#include "Base/Types/Arrays.h"
#include "Base/Math/Math.h"
#include <intrin.h>
#include <immintrin.h>

//-------------------------------------------------------------------------

namespace EE::Memory::Allocators
{
    EE_BASE_API extern MemoryAllocator g_handleAllocator_L1;
    EE_BASE_API extern MemoryAllocator g_handleAllocator_L2;
    EE_BASE_API extern MemoryAllocator g_handleAllocator_Hint;
}

//-------------------------------------------------------------------------

namespace EE::Render
{
    // Hierarchical handle allocator.
    //
    // The allocator is organized as a stack of bitmask levels.
    // Level 0 is the shader facing slot bitmask: one uint64_t per 64-slot page, bit = 1 -> allocated.
    // Each higher level groups 64 pages of the level below into one new page: a bit at level k is 1 when the corresponding level-(k-1) page is fully allocated.
    // Every page also stores { prefix, suffix, maxRun } free-run hints, so the same scan code works at every level.
    //
    // Adding / removing hierarchy levels = changing NumHierarchyLevels (the members of the Level metadata struct below).
    //
    // Allocation always prefers the lowest available offset to maximize page occupancy.
    // Handle carries the allocation size so Deallocate needs no extra parameters.
    // Not thread-safe: each instance must be owned by a single thread.

    template <typename OffsetType, uint32_t NumHierarchyLevels = 3>
    class HandleAllocator
    {
        static_assert( sizeof( OffsetType ) == 2 || sizeof( OffsetType ) == 4, "OffsetType must be uint16_t or uint32_t" );
        static_assert( NumHierarchyLevels >= 1, "At least 1 level is required" );
        static_assert( NumHierarchyLevels <= 5, "Page spans are stored as uint32_t; 64^6 slots would overflow" );

    public:

        static constexpr OffsetType  s_invalidOffset = OffsetType( -1 );
        static constexpr uint32_t    s_maxAddressablePages = uint32_t( OffsetType( -1 ) / OffsetType( 64 ) );

        //-------------------------------------------------------------------------

        HandleAllocator() = default;
        HandleAllocator( HandleAllocator const& ) = delete;
        HandleAllocator& operator=( HandleAllocator const& ) = delete;
        HandleAllocator( HandleAllocator&& ) = default;
        HandleAllocator& operator=( HandleAllocator&& ) = default;

        struct Handle
        {
            inline bool IsValid() const { return m_offset != s_invalidOffset; }

            //-------------------------------------------------------------------------

            OffsetType               m_size = OffsetType( 0 );
            OffsetType               m_offset = s_invalidOffset;
        };

        //-------------------------------------------------------------------------

        inline void Initialize( uint32_t initialCapacityInPages )
        {
            EE_ASSERT( initialCapacityInPages > 0 );
            EE_ASSERT( initialCapacityInPages <= s_maxAddressablePages );

            m_isGrowable = true;

            ResizeLevels( initialCapacityInPages );

            // Mask off availability bits for pages beyond initialCapacityInPages
            uint32_t const lastWordPages = initialCapacityInPages % 64;
            if ( lastWordPages > 0 && !m_levels[0].m_availability.empty() )
            {
                uint64_t const validMask = ( 1ULL << lastWordPages ) - 1;
                m_levels[0].m_availability.back() &= validMask;
            }

            RebuildHintRange( 0, initialCapacityInPages - 1 );
        }

        inline void Shutdown()
        {
            EE_ASSERT( IsBitmaskEmpty() );

            for ( uint32_t level = 0; level < NumHierarchyLevels; ++level )
            {
                m_levels[level].m_slotMask.clear();
                m_levels[level].m_availability.clear();
                m_levels[level].m_pagePrefix.clear();
                m_levels[level].m_pageSuffix.clear();
                m_levels[level].m_pageMaxRun.clear();
                m_levels[level].m_dirty.clear();
            }
        }

        //-------------------------------------------------------------------------

        inline Handle Allocate( OffsetType numHandles )
        {
            EE_ASSERT( numHandles > 0 );

            OffsetType const offset = FindFreeRun( numHandles );

            if ( offset == s_invalidOffset )
            {
                // Growth not allowed - return invalid handle to signal out of memory
                if ( !m_isGrowable )
                {
                    return {};
                }

                // Try to grow the pool
                if ( !TryGrow( numHandles ) )
                {
                    EE_ASSERT( false ); // Out of addressable range
                    return {};
                }

                // Retry after growth - must succeed
                OffsetType const retryOffset = FindFreeRun( numHandles );
                if ( retryOffset == s_invalidOffset )
                {
                    EE_ASSERT( false ); // Growth succeeded but still out of addressable range
                    return {};
                }

                MarkAllocated( retryOffset, numHandles );
                return { numHandles, retryOffset };
            }

            MarkAllocated( offset, numHandles );
            return { numHandles, offset };
        }

        inline void Deallocate( Handle&& handle )
        {
            EE_ASSERT( handle.IsValid() );
            MarkFree( handle.m_offset, handle.m_size );
            handle = {};
        }

        //-------------------------------------------------------------------------

        inline uint32_t GetCapacityInPages() const
        {
            return uint32_t( m_levels[0].m_slotMask.size() );
        }

        inline uint64_t const* GetPageData() const
        {
            return m_levels[0].m_slotMask.data();
        }

        inline void SetIsGrowable( bool isGrowable ) { m_isGrowable = isGrowable; }
        inline bool IsGrowable() const { return m_isGrowable; }

    private:

        //-------------------------------------------------------------------------

        struct Level
        {
            TAlignedVector<uint64_t> m_slotMask{ Memory::Allocators::g_handleAllocator_L2 };
            TAlignedVector<uint64_t> m_availability{ Memory::Allocators::g_handleAllocator_L1 };
            TAlignedVector<uint32_t> m_pagePrefix{ Memory::Allocators::g_handleAllocator_Hint };
            TAlignedVector<uint32_t> m_pageSuffix{ Memory::Allocators::g_handleAllocator_Hint };
            TAlignedVector<uint32_t> m_pageMaxRun{ Memory::Allocators::g_handleAllocator_Hint };
            TAlignedVector<uint64_t> m_dirty{ Memory::Allocators::g_handleAllocator_Hint };
        };

        static constexpr uint64_t s_notFoundOffset = ~0ULL;

    private:

        //-------------------------------------------------------------------------

        // Slots per unit at a given level: level 0 units are single slots, level k units are 64^k slots (one level-(k-1) page).
        static constexpr uint64_t UnitSpan( uint32_t level )
        {
            return 1ULL << ( level * 6 );
        }

        // Slots per page (64 units) at a given level.
        static constexpr uint64_t PageSpan( uint32_t level )
        {
            return UnitSpan( level + 1 );
        }

        // Smallest exponent e >= 1 with base^e >= value. Base must be a power of two.
        EE_FORCE_INLINE static uint32_t CeilLogBase( uint64_t base, uint64_t value )
        {
            EE_ASSERT( base >= 2 && ( base & ( base - 1 ) ) == 0 );

            if ( value <= 1 )
            {
                return 1;
            }

            uint32_t const log2Base = 64U - uint32_t( _lzcnt_u64( base - 1 ) );
            uint32_t const bitWidth = 64U - uint32_t( _lzcnt_u64( value - 1 ) );
            return ( bitWidth + log2Base - 1 ) / log2Base;
        }

        EE_FORCE_INLINE static uint32_t CountTrailingZeros( uint64_t value )
        {
            EE_ASSERT( value != 0 );
            return uint32_t( _tzcnt_u64( value ) );
        }

        // Number of consecutive free slots at the START of a page.
        // Equivalent to: count leading allocated slots -> that many are NOT free, so trailing zeros in the allocated mask = leading free slots.
        EE_FORCE_INLINE static uint32_t LeadingFreeInPage( uint64_t pageMask )
        {
            if ( pageMask == 0 )
            {
                return 64;
            }
            return uint32_t( _tzcnt_u64( pageMask ) );
        }

        // Number of consecutive free slots at the END of a page.
        // Leading zeros in the allocated mask = trailing free slots.
        EE_FORCE_INLINE static uint32_t TrailingFreeInPage( uint64_t pageMask )
        {
            if ( pageMask == 0 )
            {
                return 64;
            }
            return uint32_t( _lzcnt_u64( pageMask ) );
        }

        //-------------------------------------------------------------------------

        inline void ResizeLevels( uint32_t numLeaves )
        {
            Level& leaf = m_levels[0];
            leaf.m_slotMask.resize( numLeaves, 0 );
            leaf.m_availability.resize( ( numLeaves + 63 ) / 64, ~0ULL );
            leaf.m_pagePrefix.resize( numLeaves, 64 ); // empty page: fully free
            leaf.m_pageSuffix.resize( numLeaves, 64 );
            leaf.m_pageMaxRun.resize( numLeaves, 64 );

            for ( uint32_t level = 1; level < NumHierarchyLevels; ++level )
            {
                uint64_t const leavesPerPage = UnitSpan( level );
                uint32_t const numPages = uint32_t( ( uint64_t( numLeaves ) + leavesPerPage - 1 ) / leavesPerPage );

                m_levels[level].m_pagePrefix.resize( numPages, 0 );
                m_levels[level].m_pageSuffix.resize( numPages, 0 );
                m_levels[level].m_pageMaxRun.resize( numPages, 0 );
                m_levels[level].m_dirty.resize( ( numPages + 63 ) / 64, 0 );

                // A shrink can leave stale dirty bits set for pages that were removed; mask off bits beyond numPages in the last word.
                uint32_t const numLastWordPages = numPages % 64;
                if ( numLastWordPages > 0 && !m_levels[level].m_dirty.empty() )
                {
                    m_levels[level].m_dirty.back() &= ( ( 1ULL << numLastWordPages ) - 1 );
                }
            }
        }

        // Recompute one page of `level` (>= 1) from its 64 level-(level-1) pages.
        // Children beyond the pool capacity count as fully-allocated pages.
        inline void FoldPage( uint32_t level, uint32_t pageIdx )
        {
            uint32_t const numChildren = uint32_t( m_levels[level - 1].m_pagePrefix.size() );
            uint32_t const firstChild = pageIdx * 64;
            uint64_t const childSpan = PageSpan( level - 1 );

            // SIMD fast path for full pages: all 64 children fully free or fully allocated.
            // A child is fully free if prefix == childSpan; fully allocated if maxRun == 0 (no free slots anywhere).
            if ( firstChild + 64 <= numChildren )
            {
                __m256i const spanBroadcast = _mm256_set1_epi32( int32_t( childSpan ) );
                __m256i const zero = _mm256_setzero_si256();
                __m256i allFree = _mm256_set1_epi32( -1 );
                __m256i allFull = _mm256_set1_epi32( -1 );

                uint32_t const* pPrefix = m_levels[level - 1].m_pagePrefix.data() + firstChild;
                uint32_t const* pMaxRun = m_levels[level - 1].m_pageMaxRun.data() + firstChild;

                // Aligned 256-bit loads require 32-byte-aligned addresses.
                EE_ASSERT( ( reinterpret_cast<uintptr_t>( pPrefix ) & 31U ) == 0 );
                EE_ASSERT( ( reinterpret_cast<uintptr_t>( pMaxRun ) & 31U ) == 0 );

                for ( uint32_t i = 0; i < 64; i += 8 )
                {
                    __m256i const prefix = _mm256_load_si256( reinterpret_cast<__m256i const*>( pPrefix + i ) );
                    __m256i const maxRun = _mm256_load_si256( reinterpret_cast<__m256i const*>( pMaxRun + i ) );

                    allFree = _mm256_and_si256( allFree, _mm256_cmpeq_epi32( prefix, spanBroadcast ) );
                    allFull = _mm256_and_si256( allFull, _mm256_cmpeq_epi32( maxRun, zero ) );
                }

                uint32_t const pageSpan = uint32_t( childSpan * 64 );

                if ( _mm256_movemask_epi8( allFree ) == -1 )
                {
                    m_levels[level].m_pagePrefix[pageIdx] = pageSpan;
                    m_levels[level].m_pageSuffix[pageIdx] = pageSpan;
                    m_levels[level].m_pageMaxRun[pageIdx] = pageSpan;
                    return;
                }

                if ( _mm256_movemask_epi8( allFull ) == -1 )
                {
                    m_levels[level].m_pagePrefix[pageIdx] = 0;
                    m_levels[level].m_pageSuffix[pageIdx] = 0;
                    m_levels[level].m_pageMaxRun[pageIdx] = 0;
                    return;
                }
            }

            // Scalar fold (partial or non-uniform page)
            uint32_t prefix = 0;
            uint32_t suffix = 0;
            uint32_t maxRun = 0;
            uint64_t span = 0;

            for ( uint32_t child = 0; child < 64; ++child )
            {
                uint32_t const childIdx = firstChild + child;

                uint32_t childPrefix = 0;
                uint32_t childSuffix = 0;
                uint32_t childMaxRun = 0;
                if ( childIdx < numChildren )
                {
                    childPrefix = m_levels[level - 1].m_pagePrefix[childIdx];
                    childSuffix = m_levels[level - 1].m_pageSuffix[childIdx];
                    childMaxRun = m_levels[level - 1].m_pageMaxRun[childIdx];
                }

                // Longest run: inside the child, or crossing the child boundary
                uint32_t const crossRun = suffix + childPrefix;
                if ( crossRun > maxRun )
                {
                    maxRun = crossRun;
                }
                if ( childMaxRun > maxRun )
                {
                    maxRun = childMaxRun;
                }

                // Prefix extends only through fully-free children
                if ( uint64_t( prefix ) == span )
                {
                    prefix = uint32_t( span + uint64_t( childPrefix ) );
                }

                // Suffix chains through fully-free children
                suffix = ( uint64_t( childSuffix ) == childSpan ) ? suffix + uint32_t( childSpan ) : childSuffix;

                span += childSpan;
            }

            m_levels[level].m_pagePrefix[pageIdx] = prefix;
            m_levels[level].m_pageSuffix[pageIdx] = suffix;
            m_levels[level].m_pageMaxRun[pageIdx] = maxRun;
        }

        // Rebuild the hierarchy page hints covering the leaf range [firstLeaf, lastLeaf] at every level.
        // Exact: each level folds the level below - loop order matters.
        inline void RebuildHintRange( uint32_t firstLeaf, uint32_t lastLeaf )
        {
            for ( uint32_t level = 1; level < NumHierarchyLevels; ++level )
            {
                uint64_t const leavesPerPage = UnitSpan( level );
                uint32_t const numPages = uint32_t( m_levels[level].m_pagePrefix.size() );
                uint32_t const firstPage = uint32_t( uint64_t( firstLeaf ) / leavesPerPage );
                uint32_t const lastPage = uint32_t( uint64_t( lastLeaf ) / leavesPerPage );

                for ( uint32_t page = firstPage; page <= lastPage && page < numPages; ++page )
                {
                    FoldPage( level, page );
                }
            }
        }

        // Mark the hierarchy pages covering the leaf range [firstLeaf, lastLeaf] stale.
        // Small alloc/free operations only pay this O(1) bookkeeping; the hints are folded lazily by FlushDirtyHints() before a hierarchy scan.
        inline void MarkHintsDirty( uint32_t firstLeaf, uint32_t lastLeaf )
        {
            if ( NumHierarchyLevels <= 1 )
            {
                return;
            }

            TAlignedVector<uint64_t>& dirty = m_levels[1].m_dirty;
            uint32_t const firstPage = firstLeaf / 64;
            uint32_t const lastPage = lastLeaf / 64;

            uint32_t const firstWord = firstPage >> 6;
            uint32_t const lastWord = lastPage >> 6;

            for ( uint32_t word = firstWord; word <= lastWord; ++word )
            {
                uint64_t mask = ~0ULL;
                if ( word == firstWord )
                {
                    mask &= ( ~0ULL << ( firstPage & 63 ) );
                }
                if ( word == lastWord )
                {
                    uint32_t const numLastBits = ( lastPage & 63 ) + 1;
                    mask &= ( numLastBits == 64 ) ? ~0ULL : ( ( 1ULL << numLastBits ) - 1 );
                }
                dirty[word] |= mask;
            }
        }

        // Fold all stale hierarchy pages, bottom-up.
        // A page whose folded value changed marks its parent page stale for the next level pass.
        inline void FlushDirtyHints()
        {
            for ( uint32_t level = 1; level < NumHierarchyLevels; ++level )
            {
                TAlignedVector<uint64_t>& dirty = m_levels[level].m_dirty;

                for ( uint32_t wordIdx = 0; wordIdx < uint32_t( dirty.size() ); ++wordIdx )
                {
                    uint64_t bits = dirty[wordIdx];
                    if ( bits == 0 )
                    {
                        continue;
                    }

                    dirty[wordIdx] = 0;

                    while ( bits )
                    {
                        uint32_t const bit = CountTrailingZeros( bits );
                        bits &= ~( 1ULL << bit );

                        uint32_t const pageIdx = wordIdx * 64 + bit;

                        uint32_t const oldPrefix = m_levels[level].m_pagePrefix[pageIdx];
                        uint32_t const oldSuffix = m_levels[level].m_pageSuffix[pageIdx];
                        uint32_t const oldMaxRun = m_levels[level].m_pageMaxRun[pageIdx];

                        FoldPage( level, pageIdx );

                        if ( m_levels[level].m_pagePrefix[pageIdx] != oldPrefix ||
                             m_levels[level].m_pageSuffix[pageIdx] != oldSuffix ||
                             m_levels[level].m_pageMaxRun[pageIdx] != oldMaxRun )
                        {
                            if ( level + 1 < NumHierarchyLevels )
                            {
                                uint32_t const parentPage = pageIdx / 64;
                                m_levels[level + 1].m_dirty[parentPage >> 6] |= ( 1ULL << ( parentPage & 63 ) );
                            }
                        }
                    }
                }
            }
        }

        //-------------------------------------------------------------------------

        // Searches one page of `level` for a free run of numSlots.
        // Returns the run's start offset or NotFoundOffset, and updates `carryFree` to the free slots at the END of the page (for chaining into the next page).
        inline uint64_t ScanPage( uint32_t level, uint32_t pageIdx, uint32_t numSlots, uint64_t& carryFree ) const
        {
            uint64_t const pageStart = uint64_t( pageIdx ) * PageSpan( level );

            if ( level == 0 )
            {
                uint64_t const pageMask = m_levels[0].m_slotMask[pageIdx];

                if ( pageMask == 0 )
                {
                    // Fully-free page: the run from the left continues through it
                    uint64_t const start = pageStart - uint64_t( carryFree );
                    carryFree += 64U;
                    return ( carryFree >= numSlots ) ? start : s_notFoundOffset;
                }

                uint64_t const freeBits = ~pageMask;
                uint32_t bitOffset = 0;

                while ( bitOffset < 64 )
                {
                    uint64_t const remainingFree = freeBits >> bitOffset;
                    if ( remainingFree == 0 )
                    {
                        break; // No free slots left in this page
                    }

                    uint32_t const skipAlloc = CountTrailingZeros( remainingFree );
                    bitOffset += skipAlloc;
                    if ( bitOffset >= 64 )
                    {
                        break;
                    }

                    uint64_t const freeFromHere = freeBits >> bitOffset;
                    uint32_t freeRun;
                    if ( freeFromHere == ~0ULL )
                    {
                        freeRun = 64 - bitOffset;
                    }
                    else
                    {
                        freeRun = CountTrailingZeros( ~freeFromHere );
                        if ( freeRun > 64 - bitOffset )
                        {
                            freeRun = 64 - bitOffset;
                        }
                    }

                    if ( bitOffset == 0 && carryFree > 0 )
                    {
                        // The run at bit 0 chains with the carry from the left
                        if ( carryFree + freeRun >= numSlots )
                        {
                            return pageStart - uint64_t( carryFree );
                        }
                    }
                    else
                    {
                        if ( freeRun >= numSlots )
                        {
                            return pageStart + uint64_t( bitOffset );
                        }
                    }

                    bitOffset += freeRun;
                }

                carryFree = m_levels[0].m_pageSuffix[pageIdx];
                return s_notFoundOffset;
            }

            // Higher level: skip or recurse using the child page hints
            uint32_t const numChildren = uint32_t( m_levels[level - 1].m_pagePrefix.size() );
            uint32_t const firstChild = pageIdx * 64;
            uint64_t const childSpan = PageSpan( level - 1 );

            for ( uint32_t child = 0; child < 64; ++child )
            {
                uint32_t const childIdx = firstChild + child;
                bool const isPastCapacity = ( childIdx >= numChildren );

                uint32_t childPrefix = 0;
                uint32_t childSuffix = 0;
                uint32_t childMaxRun = 0;
                if ( !isPastCapacity )
                {
                    childPrefix = m_levels[level - 1].m_pagePrefix[childIdx];
                    childSuffix = m_levels[level - 1].m_pageSuffix[childIdx];
                    childMaxRun = m_levels[level - 1].m_pageMaxRun[childIdx];
                }

                uint64_t const childStart = pageStart + uint64_t( child ) * childSpan;

                // Nothing inside this child can fit and no run comes from the left
                if ( carryFree == 0 && childMaxRun < numSlots )
                {
                    carryFree = childSuffix;
                    continue;
                }

                // The run crossing from the left completes inside this child
                if ( carryFree + childPrefix >= numSlots )
                {
                    return childStart - uint64_t( carryFree );
                }

                if ( isPastCapacity )
                {
                    carryFree = 0; // Missing child past capacity counts as fully allocated - terminates the carry
                    continue;
                }

                // Fully-free child: the run passes straight through without descent
                if ( carryFree > 0 && uint64_t( childPrefix ) == childSpan )
                {
                    carryFree += childSpan;
                    continue;
                }

                uint64_t const found = ScanPage( level - 1, childIdx, numSlots, carryFree );
                if ( found != s_notFoundOffset )
                {
                    return found;
                }
            }

            return s_notFoundOffset;
        }

        // Top-level scan: find the lowest offset with a free run of numSlots.
        inline OffsetType FindRunInHierarchy( uint32_t numSlots )
        {
            // Stale hints from small alloc/free operations must be folded before the hierarchy can be trusted for skip decisions.
            FlushDirtyHints();

            uint32_t const numLeaves = uint32_t( m_levels[0].m_slotMask.size() );
            uint32_t const topLevel = Math::Min( NumHierarchyLevels - 1u, CeilLogBase( 64, uint64_t( numLeaves ) ) );
            uint32_t const numTopPages = uint32_t( m_levels[topLevel].m_pagePrefix.size() );
            uint64_t const topSpan = PageSpan( topLevel );

            uint64_t carryFree = 0;
            for ( uint32_t pageIdx = 0; pageIdx < numTopPages; ++pageIdx )
            {
                uint32_t const pageMaxRun = m_levels[topLevel].m_pageMaxRun[pageIdx];
                uint32_t const pagePrefix = m_levels[topLevel].m_pagePrefix[pageIdx];
                uint32_t const pageSuffix = m_levels[topLevel].m_pageSuffix[pageIdx];
                uint64_t const pageStart = uint64_t( pageIdx ) * topSpan;

                if ( carryFree == 0 && pageMaxRun < numSlots )
                {
                    carryFree = pageSuffix;
                    continue;
                }

                if ( carryFree + pagePrefix >= numSlots )
                {
                    return OffsetType( pageStart - uint64_t( carryFree ) );
                }

                uint64_t const found = ScanPage( topLevel, pageIdx, numSlots, carryFree );
                if ( found != s_notFoundOffset )
                {
                    return OffsetType( found );
                }
            }

            return s_invalidOffset;
        }

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE static uint8_t ComputeMaxFreeRun( uint64_t pageMask )
        {
            uint64_t freeBits = ~pageMask;

            // Fully-free page - maxRun = 64, bail early
            if ( freeBits == ~0ULL )
            {
                return 64;
            }

            uint8_t maxRun = 0;
            while ( freeBits )
            {
                uint32_t const tz = CountTrailingZeros( freeBits );
                freeBits >>= tz;

                // Count the run of consecutive ones (free slots)
                // freeBits is non-zero and not ~0ULL, so ~freeBits is non-zero
                uint32_t const run = CountTrailingZeros( ~freeBits );
                freeBits >>= run;

                if ( run > maxRun )
                {
                    maxRun = uint8_t( run );
                    if ( maxRun == 64 )
                    {
                        break;
                    }
                }
            }

            return maxRun;
        }

        //-------------------------------------------------------------------------

        inline bool IsBitmaskEmpty() const
        {
            for ( uint64_t const pageMask : m_levels[0].m_slotMask )
            {
                if ( pageMask != 0 )
                {
                    return false;
                }
            }
            return true;
        }

        //-------------------------------------------------------------------------

        inline OffsetType FindFreeRun( OffsetType numSlots )
        {
            uint32_t const numPages = uint32_t( m_levels[0].m_slotMask.size() );

            // For allocations smaller than a full page, use the single-page fast path.
            // For page-sized or larger allocations, always use the hint hierarchy scan.
            // An empty page would satisfy it but a cross-page gap at a lower offset must take priority for offset minimization.
            if ( numSlots < 64 )
            {
                // Single-page fast path: scan the availability map LSB-first
                uint32_t const numL1Words = uint32_t( m_levels[0].m_availability.size() );
                uint32_t const requiredRun = uint32_t( numSlots );

                for ( uint32_t l1WordIdx = 0; l1WordIdx < numL1Words; ++l1WordIdx )
                {
                    uint64_t l1Word = m_levels[0].m_availability[l1WordIdx];
                    if ( l1Word == 0 )
                    {
                        continue;
                    }

                    // Process each set bit (free page) LSB-first
                    while ( l1Word )
                    {
                        uint32_t const l1Bit = CountTrailingZeros( l1Word );
                        uint32_t const pageIdx = l1WordIdx * 64 + l1Bit;

                        if ( pageIdx >= numPages )
                        {
                            break;
                        }

                        // Skip pages that can't fit the request (max free run too small)
                        if ( m_levels[0].m_pageMaxRun[pageIdx] < requiredRun )
                        {
                            // Check for cross-page gap: trailing free of this page
                            // plus leading free of the next page might form a run >= N
                            if ( pageIdx + 1 < numPages )
                            {
                                uint32_t const trailing = TrailingFreeInPage( m_levels[0].m_slotMask[pageIdx] );
                                if ( trailing > 0 )
                                {
                                    uint32_t const nextLeading = LeadingFreeInPage( m_levels[0].m_slotMask[pageIdx + 1] );
                                    if ( trailing + nextLeading >= requiredRun )
                                    {
                                        return OffsetType( pageIdx * 64 + ( 64 - trailing ) );
                                    }
                                }
                            }

                            l1Word &= ~( 1ULL << l1Bit );
                            continue;
                        }

                        uint32_t const bitInPage = FindRunInPage( m_levels[0].m_slotMask[pageIdx], numSlots );
                        if ( bitInPage != ~0U )
                        {
                            return OffsetType( pageIdx * 64 + bitInPage );
                        }

                        // Clear this bit and continue to next free page
                        l1Word &= ~( 1ULL << l1Bit );
                    }
                }

                // No single page fits - fall through to hint hierarchy scan
            }

            // Multi-page scan: search for numSlots consecutive zero bits using the hint hierarchy - skips whole ranges whose max free run is too small.
            return FindRunInHierarchy( uint32_t( numSlots ) );
        }

        //-------------------------------------------------------------------------

        inline static uint32_t FindRunInPage( uint64_t pageMask, uint32_t numSlots )
        {
            uint64_t freeBits = ~pageMask;

            uint64_t pattern = ( numSlots == 64 ) ? ~0ULL : ( ( 1ULL << numSlots ) - 1 );

            for ( uint32_t bit = 0; bit <= 64 - numSlots; ++bit )
            {
                if ( ( freeBits & pattern ) == pattern )
                {
                    return bit;
                }
                pattern <<= 1;
            }

            return ~0U;
        }

        //-------------------------------------------------------------------------

        // Fully covers numPages pages [firstLeaf, firstLeaf + numPages) with allocated slots: slot masks = ~0, page hints zeroed, availability bits cleared in bulk.
        // 256-bit stores for the wide path.
        inline void MarkInteriorPagesAllocated( uint32_t firstLeaf, uint32_t numPages )
        {
            Level& leaf = m_levels[0];
            uint32_t const endLeaf = firstLeaf + numPages;

            __m256i const allOnes = _mm256_set1_epi64x( -1 );
            __m256i const zero = _mm256_setzero_si256();

            uint32_t page = firstLeaf;

            // Scalar prologue: advance to an 8-aligned page index so the wide path below can use aligned 256-bit loads/stores.
            while ( page < endLeaf && ( page & 7 ) != 0 )
            {
                EE_ASSERT( leaf.m_slotMask[page] == 0 ); // No double-allocation
                leaf.m_slotMask[page] = ~0ULL;
                leaf.m_pagePrefix[page] = 0;
                leaf.m_pageSuffix[page] = 0;
                leaf.m_pageMaxRun[page] = 0;
                ++page;
            }

            while ( endLeaf - page >= 8 )
            {
                // Aligned 256-bit loads/stores require 32-byte-aligned addresses.
                EE_ASSERT( ( reinterpret_cast<uintptr_t>( leaf.m_slotMask.data() + page ) & 31U ) == 0 );
                EE_ASSERT( ( reinterpret_cast<uintptr_t>( leaf.m_pagePrefix.data() + page ) & 31U ) == 0 );

                // No double-allocation check for all 8 pages
                __m256i const loaded0 = _mm256_load_si256( reinterpret_cast<__m256i const*>( leaf.m_slotMask.data() + page ) );
                __m256i const loaded1 = _mm256_load_si256( reinterpret_cast<__m256i const*>( leaf.m_slotMask.data() + page + 4 ) );
                EE_ASSERT( _mm256_testz_si256( loaded0, loaded0 ) != 0 && _mm256_testz_si256( loaded1, loaded1 ) != 0 );

                _mm256_store_si256( reinterpret_cast<__m256i*>( leaf.m_slotMask.data() + page ), allOnes );
                _mm256_store_si256( reinterpret_cast<__m256i*>( leaf.m_slotMask.data() + page + 4 ), allOnes );
                _mm256_store_si256( reinterpret_cast<__m256i*>( leaf.m_pagePrefix.data() + page ), zero );
                _mm256_store_si256( reinterpret_cast<__m256i*>( leaf.m_pageSuffix.data() + page ), zero );
                _mm256_store_si256( reinterpret_cast<__m256i*>( leaf.m_pageMaxRun.data() + page ), zero );

                page += 8;
            }

            while ( page < endLeaf )
            {
                EE_ASSERT( leaf.m_slotMask[page] == 0 ); // No double-allocation
                leaf.m_slotMask[page] = ~0ULL;
                leaf.m_pagePrefix[page] = 0;
                leaf.m_pageSuffix[page] = 0;
                leaf.m_pageMaxRun[page] = 0;
                ++page;
            }

            // Clear availability bits for the range in bulk
            uint32_t const firstWord = firstLeaf / 64;
            uint32_t const lastWord = ( endLeaf - 1 ) / 64;
            for ( uint32_t word = firstWord; word <= lastWord; ++word )
            {
                uint64_t mask = ~0ULL;
                if ( word == firstWord )
                {
                    mask &= ( ~0ULL << ( firstLeaf & 63 ) );
                }
                if ( word == lastWord )
                {
                    uint32_t const lastBit = ( endLeaf & 63 );
                    mask &= ( lastBit == 0 ) ? ~0ULL : ( ( 1ULL << lastBit ) - 1 );
                }
                leaf.m_availability[word] &= ~mask;
            }
        }

        // Frees numPages pages [firstLeaf, firstLeaf + numPages) that are fully allocated: slot masks cleared, page hints = 64, availability bits set in bulk. 
        // 256-bit stores for the wide path.
        inline void MarkInteriorPagesFree( uint32_t firstLeaf, uint32_t numPages )
        {
            Level& leaf = m_levels[0];
            uint32_t const endLeaf = firstLeaf + numPages;

            __m256i const allOnes = _mm256_set1_epi64x( -1 );
            __m256i const zero = _mm256_setzero_si256();
            __m256i const sixtyFour = _mm256_set1_epi32( 64 );

            uint32_t page = firstLeaf;

            // Scalar prologue: advance to an 8-aligned page index so the wide path below can use aligned 256-bit loads/stores.
            while ( page < endLeaf && ( page & 7 ) != 0 )
            {
                EE_ASSERT( leaf.m_slotMask[page] == ~0ULL );
                leaf.m_slotMask[page] = 0;
                leaf.m_pagePrefix[page] = 64;
                leaf.m_pageSuffix[page] = 64;
                leaf.m_pageMaxRun[page] = 64;
                ++page;
            }

            while ( endLeaf - page >= 8 )
            {
                // Aligned 256-bit loads/stores require 32-byte-aligned addresses.
                EE_ASSERT( ( reinterpret_cast<uintptr_t>( leaf.m_slotMask.data() + page ) & 31U ) == 0 );
                EE_ASSERT( ( reinterpret_cast<uintptr_t>( leaf.m_pagePrefix.data() + page ) & 31U ) == 0 );

                // Pages must have been fully allocated
                __m256i const loaded0 = _mm256_load_si256( reinterpret_cast<__m256i const*>( leaf.m_slotMask.data() + page ) );
                __m256i const loaded1 = _mm256_load_si256( reinterpret_cast<__m256i const*>( leaf.m_slotMask.data() + page + 4 ) );
                EE_ASSERT( _mm256_testc_si256( loaded0, allOnes ) != 0 && _mm256_testc_si256( loaded1, allOnes ) != 0 );

                _mm256_store_si256( reinterpret_cast<__m256i*>( leaf.m_slotMask.data() + page ), zero );
                _mm256_store_si256( reinterpret_cast<__m256i*>( leaf.m_slotMask.data() + page + 4 ), zero );
                _mm256_store_si256( reinterpret_cast<__m256i*>( leaf.m_pagePrefix.data() + page ), sixtyFour );
                _mm256_store_si256( reinterpret_cast<__m256i*>( leaf.m_pageSuffix.data() + page ), sixtyFour );
                _mm256_store_si256( reinterpret_cast<__m256i*>( leaf.m_pageMaxRun.data() + page ), sixtyFour );

                page += 8;
            }

            while ( page < endLeaf )
            {
                EE_ASSERT( leaf.m_slotMask[page] == ~0ULL );
                leaf.m_slotMask[page] = 0;
                leaf.m_pagePrefix[page] = 64;
                leaf.m_pageSuffix[page] = 64;
                leaf.m_pageMaxRun[page] = 64;
                ++page;
            }

            // Set availability bits for the range in bulk
            uint32_t const firstWord = firstLeaf / 64;
            uint32_t const lastWord = ( endLeaf - 1 ) / 64;
            for ( uint32_t word = firstWord; word <= lastWord; ++word )
            {
                uint64_t mask = ~0ULL;
                if ( word == firstWord )
                {
                    mask &= ( ~0ULL << ( firstLeaf & 63 ) );
                }
                if ( word == lastWord )
                {
                    uint32_t const lastBit = ( endLeaf & 63 );
                    mask &= ( lastBit == 0 ) ? ~0ULL : ( ( 1ULL << lastBit ) - 1 );
                }
                leaf.m_availability[word] |= mask;
            }
        }

        //-------------------------------------------------------------------------

        inline void MarkAllocated( OffsetType offset, OffsetType size )
        {
            uint64_t const start = uint64_t( offset );
            uint64_t const end = start + uint64_t( size );

            uint32_t const firstLeaf = uint32_t( start / 64 );
            uint32_t const lastLeaf = uint32_t( ( end - 1 ) / 64 );

            Level& leaf = m_levels[0];

            // Single-page range
            if ( firstLeaf == lastLeaf )
            {
                uint32_t const numBits = uint32_t( end - start );
                uint64_t const mask = ( ( numBits == 64 ) ? ~0ULL : ( ( 1ULL << numBits ) - 1 ) ) << ( start & 63 );

                uint64_t& pageMask = leaf.m_slotMask[firstLeaf];
                EE_ASSERT( ( pageMask & mask ) == 0 ); // No double-allocation
                pageMask |= mask;

                leaf.m_pageMaxRun[firstLeaf] = ComputeMaxFreeRun( pageMask );
                leaf.m_pagePrefix[firstLeaf] = LeadingFreeInPage( pageMask );
                leaf.m_pageSuffix[firstLeaf] = TrailingFreeInPage( pageMask );

                if ( pageMask == ~0ULL )
                {
                    SetAvailabilityBit( firstLeaf, false );
                }

                MarkHintsDirty( firstLeaf, lastLeaf );
                return;
            }

            uint32_t const startBit = uint32_t( start & 63 );
            uint32_t const endBit = uint32_t( end & 63 );

            uint32_t interiorFirst = firstLeaf;
            uint32_t interiorLast = lastLeaf;

            // Partial first page
            if ( startBit != 0 )
            {
                uint64_t& pageMask = leaf.m_slotMask[firstLeaf];
                uint64_t const mask = ~0ULL << startBit;
                EE_ASSERT( ( pageMask & mask ) == 0 ); // No double-allocation
                pageMask |= mask;

                leaf.m_pageMaxRun[firstLeaf] = ComputeMaxFreeRun( pageMask );
                leaf.m_pagePrefix[firstLeaf] = LeadingFreeInPage( pageMask );
                leaf.m_pageSuffix[firstLeaf] = TrailingFreeInPage( pageMask );

                if ( pageMask == ~0ULL )
                {
                    SetAvailabilityBit( firstLeaf, false );
                }

                ++interiorFirst;
            }

            // Partial last page
            if ( endBit != 0 )
            {
                uint64_t& pageMask = leaf.m_slotMask[lastLeaf];
                uint64_t const mask = ( 1ULL << endBit ) - 1;
                EE_ASSERT( ( pageMask & mask ) == 0 ); // No double-allocation
                pageMask |= mask;

                leaf.m_pageMaxRun[lastLeaf] = ComputeMaxFreeRun( pageMask );
                leaf.m_pagePrefix[lastLeaf] = LeadingFreeInPage( pageMask );
                leaf.m_pageSuffix[lastLeaf] = TrailingFreeInPage( pageMask );

                if ( pageMask == ~0ULL )
                {
                    SetAvailabilityBit( lastLeaf, false );
                }

                --interiorLast;
            }

            // Interior pages become fully allocated
            if ( interiorFirst <= interiorLast )
            {
                MarkInteriorPagesAllocated( interiorFirst, interiorLast - interiorFirst + 1 );
            }

            MarkHintsDirty( firstLeaf, lastLeaf );
        }

        //-------------------------------------------------------------------------

        inline void MarkFree( OffsetType offset, OffsetType size )
        {
            uint64_t const start = uint64_t( offset );
            uint64_t const end = start + uint64_t( size );

            uint32_t const firstLeaf = uint32_t( start / 64 );
            uint32_t const lastLeaf = uint32_t( ( end - 1 ) / 64 );

            Level& leaf = m_levels[0];

            // Single-page range
            if ( firstLeaf == lastLeaf )
            {
                uint32_t const numBits = uint32_t( end - start );
                uint64_t const mask = ( ( numBits == 64 ) ? ~0ULL : ( ( 1ULL << numBits ) - 1 ) ) << ( start & 63 );

                uint64_t& pageMask = leaf.m_slotMask[firstLeaf];
                EE_ASSERT( ( pageMask & mask ) == mask );

                bool const wasFull = ( pageMask == ~0ULL );
                pageMask &= ~mask;

                leaf.m_pageMaxRun[firstLeaf] = ComputeMaxFreeRun( pageMask );
                leaf.m_pagePrefix[firstLeaf] = LeadingFreeInPage( pageMask );
                leaf.m_pageSuffix[firstLeaf] = TrailingFreeInPage( pageMask );

                if ( wasFull )
                {
                    SetAvailabilityBit( firstLeaf, true );
                }
            }
            else
            {
                uint32_t const startBit = uint32_t( start & 63 );
                uint32_t const endBit = uint32_t( end & 63 );

                uint32_t interiorFirst = firstLeaf;
                uint32_t interiorLast = lastLeaf;

                // Partial first page
                if ( startBit != 0 )
                {
                    uint64_t& pageMask = leaf.m_slotMask[firstLeaf];
                    uint64_t const mask = ~0ULL << startBit;
                    EE_ASSERT( ( pageMask & mask ) == mask );

                    bool const wasFull = ( pageMask == ~0ULL );
                    pageMask &= ~mask;

                    leaf.m_pageMaxRun[firstLeaf] = ComputeMaxFreeRun( pageMask );
                    leaf.m_pagePrefix[firstLeaf] = LeadingFreeInPage( pageMask );
                    leaf.m_pageSuffix[firstLeaf] = TrailingFreeInPage( pageMask );

                    if ( wasFull )
                    {
                        SetAvailabilityBit( firstLeaf, true );
                    }

                    ++interiorFirst;
                }

                // Partial last page
                if ( endBit != 0 )
                {
                    uint64_t& pageMask = leaf.m_slotMask[lastLeaf];
                    uint64_t const mask = ( 1ULL << endBit ) - 1;
                    EE_ASSERT( ( pageMask & mask ) == mask );

                    bool const wasFull = ( pageMask == ~0ULL );
                    pageMask &= ~mask;

                    leaf.m_pageMaxRun[lastLeaf] = ComputeMaxFreeRun( pageMask );
                    leaf.m_pagePrefix[lastLeaf] = LeadingFreeInPage( pageMask );
                    leaf.m_pageSuffix[lastLeaf] = TrailingFreeInPage( pageMask );

                    if ( wasFull )
                    {
                        SetAvailabilityBit( lastLeaf, true );
                    }

                    --interiorLast;
                }

                // Interior pages become empty
                if ( interiorFirst <= interiorLast )
                {
                    MarkInteriorPagesFree( interiorFirst, interiorLast - interiorFirst + 1 );
                }
            }

            MarkHintsDirty( firstLeaf, lastLeaf );

            // Only pages in [firstPage, lastPage] could have transitioned to empty.
            // Since TryShrink runs after every deallocation, empty pages cannot accumulate at the tail - start scanning from the highest page touched.
            if ( m_isGrowable )
            {
                uint32_t const lastAffectedPage = uint32_t( ( end - 1 ) / 64 );
                TryShrink( lastAffectedPage );
            }
        }

        //-------------------------------------------------------------------------

        inline void TryShrink( uint32_t lastAffectedPage )
        {
            EE_ASSERT( m_isGrowable );

            uint32_t const numPages = uint32_t( m_levels[0].m_slotMask.size() );
            if ( numPages <= 1 )
            {
                return;
            }

            // Only pages at or beyond lastAffectedPage could be newly empty.
            // Start scanning from there, capped at the current tail.
            uint32_t scanStart = lastAffectedPage;
            if ( scanStart >= numPages )
            {
                scanStart = numPages - 1;
            }

            // If the scan start isn't at the tail, pages after it are non-empty (invariant: previous TryShrink calls removed any empty tail pages).
            // Nothing to shrink unless the scan starts at the last page.
            if ( scanStart != numPages - 1 )
            {
                return;
            }

            // Walk backwards from the last page, count consecutive empty pages.
            // Stop at page 0 - keep at least 1 page to avoid re-growth churn.
            uint32_t numEmptyTailPages = 0;
            for ( uint32_t p = numPages - 1; p > 0; --p )
            {
                if ( m_levels[0].m_slotMask[p] == 0 )
                {
                    ++numEmptyTailPages;
                }
                else
                {
                    break;
                }
            }

            if ( numEmptyTailPages == 0 )
            {
                return;
            }

            uint32_t const newNumPages = numPages - numEmptyTailPages;

            ResizeLevels( newNumPages );

            // Mask off availability bits for pages beyond newNumPages
            uint32_t const numLastWordPages = newNumPages % 64;
            if ( numLastWordPages > 0 && !m_levels[0].m_availability.empty() )
            {
                uint64_t const validMask = ( 1ULL << numLastWordPages ) - 1;
                m_levels[0].m_availability.back() &= validMask;
            }

            // Rebuild the hierarchy page hints along the shrink boundary (children beyond the new capacity now count as fully-allocated).
            RebuildHintRange( newNumPages - 1, newNumPages - 1 );
        }

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE void SetAvailabilityBit( uint32_t pageIdx, bool hasFreeSlots )
        {
            uint32_t const l1WordIdx = pageIdx / 64;
            uint32_t const l1Bit = pageIdx % 64;
            uint64_t& l1Word = m_levels[0].m_availability[l1WordIdx];

            if ( hasFreeSlots )
            {
                l1Word |= ( 1ULL << l1Bit );
            }
            else
            {
                l1Word &= ~( 1ULL << l1Bit );
            }
        }

        //-------------------------------------------------------------------------

        inline bool TryGrow( OffsetType numSlots )
        {
            EE_ASSERT( m_isGrowable );

            uint32_t const numCurrentPages = uint32_t( m_levels[0].m_slotMask.size() );

            // Don't exceed the maximum addressable offset
            if ( numCurrentPages >= s_maxAddressablePages )
            {
                return false;
            }

            // Compute how many new pages we need to guarantee we can fit numSlots
            uint32_t const slotsNeeded = uint32_t( numSlots );
            uint32_t const pagesNeeded = ( slotsNeeded + 63 ) / 64;

            // Grow by at least that many pages, clamped to max addressable
            uint32_t const growPages = Math::Max( 1u, pagesNeeded );
            uint32_t const newNumPages = Math::Min( numCurrentPages + growPages, s_maxAddressablePages );

            if ( newNumPages <= numCurrentPages )
            {
                return false;
            }

            // Expand all hierarchy levels - new pages are empty (all free)
            ResizeLevels( newNumPages );

            // Set availability bits for the newly added pages
            for ( uint32_t page = numCurrentPages; page < newNumPages; ++page )
            {
                SetAvailabilityBit( page, true );
            }

            // Mask off availability bits beyond newNumPages
            uint32_t const numLastWordPages = newNumPages % 64;
            if ( numLastWordPages > 0 && !m_levels[0].m_availability.empty() )
            {
                uint64_t const validMask = ( 1ULL << numLastWordPages ) - 1;
                m_levels[0].m_availability.back() &= validMask;
            }

            // New leaves are empty (all free) - rebuild their hierarchy page hints
            RebuildHintRange( numCurrentPages, newNumPages - 1 );

            return true;
        }

    private:

        //-------------------------------------------------------------------------

        Level                       m_levels[NumHierarchyLevels];
        bool                        m_isGrowable = true;
    };
}
