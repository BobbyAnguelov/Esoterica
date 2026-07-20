#pragma once
#include "Engine/Animation/AnimationBoneMask.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class BoneMaskBuffer
    {
        friend class BoneMaskPool;

    public:

        BoneMaskBuffer( Skeleton const* pPrimarySkeleton, SecondarySkeletonList const& secondarySkeletons );

        BoneMask const* TryGetBoneMask( Skeleton const* pSkeleton ) const;

        // Mask Operations
        //-------------------------------------------------------------------------

        void ResetWeights( float weight );

        void CopyFrom( BoneMaskSet const& maskSet );

        void ScaleWeights( float weight );

        void CombineWith( BoneMaskBuffer const* pBuffer );

        void BlendFrom( BoneMaskBuffer const* pSourceBuffer, float weight );

    private:

        // Changes the set of poses we store
        void UpdateSecondarySkeletonList( SecondarySkeletonList const& secondarySkeletons );

    public:

        TInlineVector<BoneMask, 5>  m_masks;
        bool                        m_isUsed = false;
    };

    //-------------------------------------------------------------------------

    class BoneMaskPool
    {
        constexpr static int32_t const s_initialPoolSize = 5;

    public:

        BoneMaskPool( Skeleton const* pPrimarySkeleton, SecondarySkeletonList const& secondarySkeletons );
        BoneMaskPool( BoneMaskPool const& ) = delete;
        ~BoneMaskPool();

        BoneMaskPool& operator=( BoneMaskPool const& rhs ) = delete;

        #if EE_DEVELOPMENT_TOOLS
        void PerformValidation() const;
        #endif

        // Set the required secondary skeleton, will free any existing buffers for skeletons not in the list
        void SetSecondarySkeletons( SecondarySkeletonList const& secondarySkeletons );

        // Get a mask from the pool - pool has a max size of 127
        // By default mask are not reset so be careful what you do with the mask
        int8_t AcquireMaskBuffer( bool resetMask = false );

        // Release a mask back into the pool
        void ReleaseMaskBuffer( int8_t bufferIdx );

        // Get a used buffer
        EE_FORCE_INLINE BoneMaskBuffer* GetBuffer( int8_t bufferIdx )
        {
            EE_ASSERT( m_buffers[bufferIdx].m_isUsed );
            return &m_buffers[bufferIdx];
        }

        // Get a used buffer
        EE_FORCE_INLINE BoneMaskBuffer const* GetBuffer( int8_t bufferIdx ) const
        {
            EE_ASSERT( m_buffers[bufferIdx].m_isUsed );
            return &m_buffers[bufferIdx];
        }

    private:

        Skeleton const*                 m_pPrimarySkeleton = nullptr;
        SecondarySkeletonList           m_secondarySkeletons;
        TVector<BoneMaskBuffer>         m_buffers;
        int8_t                          m_firstFreePoolIdx = InvalidIndex;
    };
}