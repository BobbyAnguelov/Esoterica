#include "Animation_BoneMaskPool.h"
#include "Engine/Animation/AnimationSkeleton.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    BoneMaskBuffer::BoneMaskBuffer( Skeleton const* pPrimarySkeleton, SecondarySkeletonList const& secondarySkeletons )
    {
        m_masks.reserve( secondarySkeletons.size() + 1 );
        m_masks.emplace_back( pPrimarySkeleton );

        UpdateSecondarySkeletonList( secondarySkeletons );
    }

    void BoneMaskBuffer::UpdateSecondarySkeletonList( SecondarySkeletonList const& secondarySkeletons )
    {
        int32_t const numExistingSecondarySkeletons = int32_t( m_masks.size() ) - 1;
        int32_t const numRequiredSecondarySkeletons = int32_t( secondarySkeletons.size() );
        int32_t const numSecondarySkeletonsToUpdate = Math::Min( numRequiredSecondarySkeletons, numExistingSecondarySkeletons );

        for ( int32_t secondarySkeletonIdx = 0; secondarySkeletonIdx < numSecondarySkeletonsToUpdate; secondarySkeletonIdx++ )
        {
            // If the skeleton differs, then change it
            if ( m_masks[secondarySkeletonIdx + 1].GetSkeleton() != secondarySkeletons[secondarySkeletonIdx] )
            {
                m_masks[secondarySkeletonIdx + 1] = BoneMask( secondarySkeletons[secondarySkeletonIdx] );
            }
        }

        // If we need less masks, then just destroy the extras
        if ( numExistingSecondarySkeletons > numRequiredSecondarySkeletons )
        {
            int32_t const numPosesToRemove = numExistingSecondarySkeletons - numRequiredSecondarySkeletons;
            for ( int32_t i = 0; i < numPosesToRemove; i++ )
            {
                m_masks.pop_back();
            }
        }
        // If we need more masks, then create the excess
        else if ( numExistingSecondarySkeletons < numRequiredSecondarySkeletons )
        {
            for ( int32_t secondarySkeletonIdx = numExistingSecondarySkeletons; secondarySkeletonIdx < numRequiredSecondarySkeletons; secondarySkeletonIdx++ )
            {
                m_masks.emplace_back( secondarySkeletons[secondarySkeletonIdx] );
            }
        }
    }

    BoneMask const* BoneMaskBuffer::TryGetBoneMask( Skeleton const* pSkeleton ) const
    {
        BoneMask const* pMask = nullptr;

        for ( BoneMask const& boneMask : m_masks )
        {
            if ( boneMask.GetSkeleton() == pSkeleton )
            {
                pMask = &boneMask;
                break;
            }
        }

        return pMask;
    }

    void BoneMaskBuffer::ResetWeights( float weight )
    {
        for ( BoneMask& boneMask : m_masks )
        {
            boneMask.ResetWeights( weight );
        }
    }

    void BoneMaskBuffer::CopyFrom( BoneMaskSet const& maskSet )
    {
        for ( BoneMask& bufferBoneMask : m_masks )
        {
            BoneMask const* pMask = maskSet.TryGetBoneMask( bufferBoneMask.GetSkeleton() );
            if ( pMask != nullptr )
            {
                bufferBoneMask.CopyFrom( *pMask );
            }
            else
            {
                bufferBoneMask.ResetWeightsToOne();
            }
        }
    }

    void BoneMaskBuffer::ScaleWeights( float weight )
    {
        for ( BoneMask& boneMask : m_masks )
        {
            boneMask.ScaleWeights( weight );
        }
    }

    void BoneMaskBuffer::CombineWith( BoneMaskBuffer const* pBuffer )
    {
        EE_ASSERT( pBuffer != nullptr && pBuffer->m_isUsed );
        EE_ASSERT( pBuffer->m_masks.size() == m_masks.size() );

        int32_t const numMasks = (int32_t) m_masks.size();
        for ( int32_t i = 0; i < numMasks; i++ )
        {
            m_masks[i].CombineWith( pBuffer->m_masks[i] );
        }
    }

    void BoneMaskBuffer::BlendFrom( BoneMaskBuffer const* pSourceBuffer, float weight )
    {
        EE_ASSERT( pSourceBuffer != nullptr && pSourceBuffer->m_isUsed );
        EE_ASSERT( pSourceBuffer->m_masks.size() == m_masks.size() );

        int32_t const numMasks = (int32_t) m_masks.size();
        for ( int32_t i = 0; i < numMasks; i++ )
        {
            m_masks[i].BlendFrom( pSourceBuffer->m_masks[i], weight );
        }
    }

    //-------------------------------------------------------------------------

    BoneMaskPool::BoneMaskPool( Skeleton const* pPrimarySkeleton, SecondarySkeletonList const& secondarySkeletons )
        : m_pPrimarySkeleton( pPrimarySkeleton )
        , m_secondarySkeletons( secondarySkeletons )
        , m_firstFreePoolIdx( InvalidIndex )
    {
        EE_ASSERT( m_buffers.empty() && m_pPrimarySkeleton != nullptr );
        EE_ASSERT( Skeleton::ValidateSkeletonSetup( m_pPrimarySkeleton, secondarySkeletons ) );

        m_buffers.reserve( s_initialPoolSize * 2 );

        for ( auto i = 0; i < s_initialPoolSize; i++ )
        {
            m_buffers.emplace_back( pPrimarySkeleton, secondarySkeletons );
        }

        m_firstFreePoolIdx = 0;
    }

    BoneMaskPool::~BoneMaskPool()
    {
        #if EE_DEVELOPMENT_TOOLS
        PerformValidation();
        #endif

        m_buffers.clear();
        m_firstFreePoolIdx = InvalidIndex;
    }

    #if EE_DEVELOPMENT_TOOLS
    void BoneMaskPool::PerformValidation() const
    {
        // Validate that all buffers have been released!
        for ( auto const& slot : m_buffers )
        {
            EE_ASSERT( !slot.m_isUsed );
        }

        EE_ASSERT( m_firstFreePoolIdx == 0 );
    }
    #endif

    void BoneMaskPool::SetSecondarySkeletons( SecondarySkeletonList const& secondarySkeletons )
    {
        EE_ASSERT( Skeleton::ValidateSkeletonSetup( m_pPrimarySkeleton, secondarySkeletons ) );

        m_secondarySkeletons = secondarySkeletons;

        for ( BoneMaskBuffer& buffer : m_buffers )
        {
            buffer.UpdateSecondarySkeletonList( m_secondarySkeletons );
        }
    }

    int8_t BoneMaskPool::AcquireMaskBuffer( bool resetMask )
    {
        int32_t const currentPoolSize = (int32_t) m_buffers.size();
        EE_ASSERT( m_firstFreePoolIdx < currentPoolSize );

        // Set the mask as used
        int8_t bufferIdx = m_firstFreePoolIdx;
        EE_ASSERT( !m_buffers[bufferIdx].m_isUsed );
        m_buffers[bufferIdx].m_isUsed = true;

        if ( resetMask )
        {
            for ( BoneMask& mask : m_buffers[bufferIdx].m_masks )
            {
                mask.ResetWeights();
            }
        }

        // Update free idx
        int8_t const searchStartIdx = m_firstFreePoolIdx + 1;
        m_firstFreePoolIdx = InvalidIndex;
        for ( int8_t i = searchStartIdx; i < currentPoolSize; i++ )
        {
            if ( !m_buffers[i].m_isUsed )
            {
                m_firstFreePoolIdx = i;
                break;
            }
        }

        // Grow the pool if needed
        if ( m_firstFreePoolIdx == InvalidIndex )
        {
            size_t const newPoolSize = Math::Max( 127, currentPoolSize * 2 );
            size_t const numMasksToAdd = newPoolSize - currentPoolSize;

            for ( auto i = 0; i < numMasksToAdd; i++ )
            {
                m_buffers.emplace_back( m_pPrimarySkeleton, m_secondarySkeletons );
            }

            m_firstFreePoolIdx = (int8_t) currentPoolSize + 1;
            EE_ASSERT( m_firstFreePoolIdx < 127 );
        }

        // Return the allocate mask pool index
        EE_ASSERT( bufferIdx >= 0 && bufferIdx < m_buffers.size() );
        return bufferIdx;
    }

    void BoneMaskPool::ReleaseMaskBuffer( int8_t bufferIdx )
    {
        EE_ASSERT( bufferIdx < m_buffers.size() );
        EE_ASSERT( m_buffers[bufferIdx].m_isUsed );

        // Clear the flag
        m_buffers[bufferIdx].m_isUsed = false;

        // Update the free index
        if ( bufferIdx < m_firstFreePoolIdx )
        {
            m_firstFreePoolIdx = bufferIdx;
        }
    }
}