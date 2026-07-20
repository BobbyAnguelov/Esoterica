#pragma once

#include "Base/Types/Arrays.h"
#include "Base/Math/Math.h"
#include <intrin.h>

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
    // 2-level handle allocator: bitmask + availability + per-page max-free-run hints.
    //
    // Level 1 (L1) - Page availability map:
    //   TVector<uint64_t>, each uint64_t covers 64 pages.
    //   Bit = 1 -> page has at least one free slot. Bit = 0 -> page is full.
    //
    // Level 2 (L2) - Slot bitmask:
    //   TVector<uint64_t>, one uint64_t per 64-slot page.
    //   Bit = 1 -> slot is allocated. Bit = 0 -> slot is free.
    //
    // Per-page hint - Max free run:
    //   TVector<uint8_t>, one byte per page.
    //   Stores the length of the longest contiguous run of free slots in the page.
    //   Used to skip fragmented pages without probing L2 via FindRunInPage.
    //
    // Allocation always prefers the lowest available offset to maximize page occupancy.
    // Handle carries the allocation size so Deallocate needs no extra parameters.

    template <typename OffsetType>
    class HandleAllocator
    {
        static_assert( sizeof( OffsetType ) == 2 || sizeof( OffsetType ) == 4, "OffsetType must be uint16_t or uint32_t" );

    public:

        static constexpr OffsetType  InvalidOffset = OffsetType( -1 );
        static constexpr uint32_t    MaxAddressablePages = uint32_t( OffsetType( -1 ) / OffsetType( 64 ) );

        struct Handle
        {
            OffsetType               m_size = OffsetType( 0 );
            OffsetType               m_offset = InvalidOffset;

            inline bool IsValid() const { return m_offset != InvalidOffset; }
        };

        //-------------------------------------------------------------------------

        inline void Initialize( uint32_t initialCapacityInPages )
        {
            EE_ASSERT( initialCapacityInPages > 0 );
            EE_ASSERT( initialCapacityInPages <= MaxAddressablePages );

            m_pageSlotMask.resize( initialCapacityInPages, 0 );
            m_pageMaxFreeRun.resize( initialCapacityInPages, 64 ); // All pages empty -> maxFreeRun = 64

            uint32_t const numL1Words = ( initialCapacityInPages + 63 ) / 64;
            m_pageAvailability.resize( numL1Words, ~0ULL );

            // Mask off L1 bits for pages beyond initialCapacityInPages
            uint32_t const lastWordPages = initialCapacityInPages % 64;
            if ( lastWordPages > 0 && !m_pageAvailability.empty() )
            {
                uint64_t const validMask = ( lastWordPages == 64 ) ? ~0ULL : ( ( 1ULL << lastWordPages ) - 1 );
                m_pageAvailability.back() &= validMask;
            }
        }

        inline void Shutdown()
        {
            EE_ASSERT( IsBitmaskEmpty() );
            m_pageAvailability.clear();
            m_pageMaxFreeRun.clear();
            m_pageSlotMask.clear();
        }

        //-------------------------------------------------------------------------

        inline Handle Allocate( OffsetType numHandles )
        {
            EE_ASSERT( numHandles > 0 );

            OffsetType const offset = FindFreeRun( numHandles );

            if ( offset == InvalidOffset )
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
                if ( retryOffset == InvalidOffset )
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
            return uint32_t( m_pageSlotMask.size() );
        }

        inline uint64_t const* GetPageData() const
        {
            return m_pageSlotMask.data();
        }

        inline void SetIsGrowable( bool isGrowable ) { m_isGrowable = isGrowable; }
        inline bool IsGrowable() const { return m_isGrowable; }

        //-------------------------------------------------------------------------

    private:

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE static uint32_t CountTrailingZeros( uint64_t value )
        {
            EE_ASSERT( value != 0 );
            return uint32_t( _tzcnt_u64( value ) );
        }

        // Number of consecutive free slots at the START of a page.
        // Equivalent to: count leading allocated slots -> that many are NOT free,
        // so trailing zeros in the allocated mask = leading free slots.
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

        EE_FORCE_INLINE static uint8_t ComputeMaxFreeRun( uint64_t pageMask )
        {
            // pageMask: 1 = allocated, 0 = free
            // Returns length of longest contiguous run of zero bits
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
            for ( uint64_t const pageMask : m_pageSlotMask )
            {
                if ( pageMask != 0 )
                {
                    return false;
                }
            }
            return true;
        }

        //-------------------------------------------------------------------------

        inline OffsetType FindFreeRun( OffsetType numSlots ) const
        {
            uint32_t const numPages = uint32_t( m_pageSlotMask.size() );

            // For allocations smaller than a full page, use the single-page fast
            // path. For exactly page-sized allocations, always use the multi-page
            // scan - an empty page would satisfy it but a cross-page gap at a lower
            // offset must take priority for offset minimization.
            if ( numSlots < 64 )
            {
                // Single-page fast path: scan L1 for pages with free slots, check each
                uint32_t const numL1Words = uint32_t( m_pageAvailability.size() );
                uint32_t const requiredRun = uint32_t( numSlots );

                for ( uint32_t l1WordIdx = 0; l1WordIdx < numL1Words; ++l1WordIdx )
                {
                    uint64_t l1Word = m_pageAvailability[l1WordIdx];
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
                        if ( m_pageMaxFreeRun[pageIdx] < requiredRun )
                        {
                            // Check for cross-page gap: trailing free of this page
                            // plus leading free of the next page might form a run >= N
                            if ( pageIdx + 1 < numPages )
                            {
                                uint32_t const trailing = TrailingFreeInPage( m_pageSlotMask[pageIdx] );
                                if ( trailing > 0 )
                                {
                                    uint32_t const nextLeading = LeadingFreeInPage( m_pageSlotMask[pageIdx + 1] );
                                    if ( trailing + nextLeading >= requiredRun )
                                    {
                                        return OffsetType( pageIdx * 64 + ( 64 - trailing ) );
                                    }
                                }
                            }

                            l1Word &= ~( 1ULL << l1Bit );
                            continue;
                        }

                        uint32_t const bitInPage = FindRunInPage( m_pageSlotMask[pageIdx], numSlots );
                        if ( bitInPage != ~0u )
                        {
                            return OffsetType( pageIdx * 64 + bitInPage );
                        }

                        // Clear this bit and continue to next free page
                        l1Word &= ~( 1ULL << l1Bit );
                    }
                }

                // No single page fits - fall through to multi-page scan
            }

            // Multi-page scan: search for numSlots consecutive zero bits across all pages.
            // Process page-at-a-time using _tzcnt_u64 to skip runs of allocated/free slots.
            uint64_t const totalSlots = uint64_t( numPages ) * 64;

            if ( uint64_t( numSlots ) > totalSlots )
            {
                return InvalidOffset;
            }

            uint64_t numConsecutiveZeros = 0;
            uint64_t runStart = 0;

            for ( uint32_t pageIdx = 0; pageIdx < numPages; ++pageIdx )
            {
                uint64_t const pageMask = m_pageSlotMask[pageIdx];

                // Fully-free page
                if ( pageMask == 0 )
                {
                    if ( numConsecutiveZeros == 0 )
                    {
                        runStart = uint64_t( pageIdx ) * 64;
                    }
                    numConsecutiveZeros += 64;

                    if ( numConsecutiveZeros >= numSlots )
                    {
                        return OffsetType( runStart );
                    }
                    continue;
                }

                // Fully-allocated page - reset run
                if ( m_pageMaxFreeRun[pageIdx] == 0 )
                {
                    numConsecutiveZeros = 0;
                    continue;
                }

                // Partially-full page - find free/allocated boundaries with _tzcnt_u64
                uint64_t freeBits = ~pageMask;
                uint32_t bitOffset = 0;

                while ( bitOffset < 64 )
                {
                    uint64_t const remainingFree = freeBits >> bitOffset;
                    if ( remainingFree == 0 )
                    {
                        numConsecutiveZeros = 0;
                        break; // No free slots left in this page
                    }

                    // Skip allocated slots to reach the next free slot
                    uint32_t const skipAlloc = CountTrailingZeros( remainingFree );
                    if ( skipAlloc > 0 )
                    {
                        numConsecutiveZeros = 0;
                        bitOffset += skipAlloc;
                        if ( bitOffset >= 64 )
                        {
                            break;
                        }
                    }

                    // Count consecutive free slots from this position
                    uint64_t const freeFromHere = freeBits >> bitOffset;
                    uint32_t freeRun;

                    if ( freeFromHere == ~0ULL )
                    {
                        // Rest of page is all free
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

                    if ( numConsecutiveZeros == 0 )
                    {
                        runStart = uint64_t( pageIdx ) * 64 + bitOffset;
                    }
                    numConsecutiveZeros += freeRun;

                    if ( numConsecutiveZeros >= numSlots )
                    {
                        return OffsetType( runStart );
                    }

                    bitOffset += freeRun;
                }
            }

            return InvalidOffset;
        }

        //-------------------------------------------------------------------------

        inline static uint32_t FindRunInPage( uint64_t pageMask, uint32_t numSlots )
        {
            // pageMask: 1 = allocated, 0 = free
            // Invert: 1 = free, 0 = allocated
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

            return ~0u;
        }

        //-------------------------------------------------------------------------

        inline void MarkAllocated( OffsetType offset, OffsetType size )
        {
            uint64_t const start = uint64_t( offset );
            uint64_t const end = start + uint64_t( size );

            uint64_t slot = start;
            while ( slot < end )
            {
                uint32_t const pageIdx = uint32_t( slot / 64 );
                uint32_t const bitInPage = uint32_t( slot % 64 );
                uint32_t const numBitsLeftInPage = 64 - bitInPage;
                uint64_t const remaining = end - slot;
                uint32_t const numBitsToSet = uint32_t( Math::Min( uint64_t( numBitsLeftInPage ), remaining ) );

                uint64_t mask = ( numBitsToSet == 64 ) ? ~0ULL : ( ( 1ULL << numBitsToSet ) - 1 );
                mask <<= bitInPage;

                uint64_t& pageMask = m_pageSlotMask[pageIdx];
                EE_ASSERT( ( pageMask & mask ) == 0 ); // No double-allocation
                pageMask |= mask;

                m_pageMaxFreeRun[pageIdx] = ComputeMaxFreeRun( pageMask );

                // If page is now full, clear its L1 bit
                if ( pageMask == ~0ULL )
                {
                    SetL1Bit( pageIdx, false );
                }

                slot += numBitsToSet;
            }
        }

        //-------------------------------------------------------------------------

        inline void MarkFree( OffsetType offset, OffsetType size )
        {
            uint64_t const start = uint64_t( offset );
            uint64_t const end = start + uint64_t( size );

            uint64_t slot = start;
            while ( slot < end )
            {
                uint32_t const pageIdx = uint32_t( slot / 64 );
                uint32_t const bitInPage = uint32_t( slot % 64 );
                uint32_t const bitsLeftInPage = 64 - bitInPage;
                uint64_t const remaining = end - slot;
                uint32_t const bitsToClear = uint32_t( Math::Min( uint64_t( bitsLeftInPage ), remaining ) );

                uint64_t mask = ( bitsToClear == 64 ) ? ~0ULL : ( ( 1ULL << bitsToClear ) - 1 );
                mask <<= bitInPage;

                uint64_t& pageMask = m_pageSlotMask[pageIdx];
                EE_ASSERT( ( pageMask & mask ) == mask );

                bool const wasFull = ( pageMask == ~0ULL );
                pageMask &= ~mask;

                m_pageMaxFreeRun[pageIdx] = ComputeMaxFreeRun( pageMask );

                // If page was full and is no longer full, set its L1 bit
                if ( wasFull )
                {
                    SetL1Bit( pageIdx, true );
                }

                slot += bitsToClear;
            }

            // Only pages in [firstPage, lastPage] could have transitioned to empty.
            // Since TryShrink runs after every deallocation, empty pages cannot
            // accumulate at the tail - start scanning from the highest page touched.
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

            uint32_t const numPages = uint32_t( m_pageSlotMask.size() );
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

            // If the scan start isn't at the tail, pages after it are non-empty
            // (invariant: previous TryShrink calls removed any empty tail pages).
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
                if ( m_pageSlotMask[p] == 0 )
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

            m_pageSlotMask.resize( newNumPages );
            m_pageMaxFreeRun.resize( newNumPages );

            // Shrink L1
            uint32_t const newNumL1Words = ( newNumPages + 63 ) / 64;
            m_pageAvailability.resize( newNumL1Words );

            // Mask off L1 bits for pages beyond newNumPages in the last word
            uint32_t const numLastWordPages = newNumPages % 64;
            if ( numLastWordPages > 0 && !m_pageAvailability.empty() )
            {
                uint64_t const validMask = ( 1ULL << numLastWordPages ) - 1;
                m_pageAvailability.back() &= validMask;
            }
        }

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE void SetL1Bit( uint32_t pageIdx, bool hasFreeSlots )
        {
            uint32_t const l1WordIdx = pageIdx / 64;
            uint32_t const l1Bit = pageIdx % 64;
            uint64_t& l1Word = m_pageAvailability[l1WordIdx];

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

            uint32_t const numCurrentPages = uint32_t( m_pageSlotMask.size() );

            // Don't exceed the maximum addressable offset
            if ( numCurrentPages >= MaxAddressablePages )
            {
                return false;
            }

            // Compute how many new pages we need to guarantee we can fit numSlots
            uint32_t const slotsNeeded = uint32_t( numSlots );
            uint32_t const pagesNeeded = ( slotsNeeded + 63 ) / 64;

            // Grow by at least that many pages, clamped to max addressable
            uint32_t const growPages = Math::Max( 1u, pagesNeeded );
            uint32_t const newNumPages = Math::Min( numCurrentPages + growPages, MaxAddressablePages );

            if ( newNumPages <= numCurrentPages )
            {
                return false;
            }

            // Expand L2
            m_pageSlotMask.resize( newNumPages, 0 );

            // Expand max free run hints - new pages are empty (maxFreeRun = 64)
            m_pageMaxFreeRun.resize( newNumPages, 64 );

            // Expand L1
            uint32_t const newNumL1Words = ( newNumPages + 63 ) / 64;
            uint32_t const oldNumL1Words = uint32_t( m_pageAvailability.size() );

            if ( newNumL1Words > oldNumL1Words )
            {
                m_pageAvailability.resize( newNumL1Words, ~0ULL );

                // New pages are all free - set their L1 bits
                // Mask off bits for pages beyond newNumPages in the last word
                uint32_t const numLastWordPages = newNumPages % 64;
                if ( numLastWordPages > 0 && !m_pageAvailability.empty() )
                {
                    uint64_t const validMask = ( 1ULL << numLastWordPages ) - 1;
                    m_pageAvailability.back() &= validMask;
                }
            }
            else
            {
                // No new L1 words needed - set L1 bits for the newly added pages
                for ( uint32_t page = numCurrentPages; page < newNumPages; ++page )
                {
                    SetL1Bit( page, true );
                }
            }

            return true;
        }

        //-------------------------------------------------------------------------

        TAlignedVector<uint64_t>    m_pageAvailability{ Memory::Allocators::g_handleAllocator_L1 };        // L1: 1 bit per page, 1 = has free slots
        TAlignedVector<uint64_t>    m_pageSlotMask{ Memory::Allocators::g_handleAllocator_L2 };            // L2: 1 bit per slot, 1 = allocated
        TAlignedVector<uint8_t>     m_pageMaxFreeRun{ Memory::Allocators::g_handleAllocator_Hint };        // Per-page hint: longest free run (0-64)
        bool                        m_isGrowable = true;
    };
}
