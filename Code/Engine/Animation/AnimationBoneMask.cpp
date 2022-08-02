#include "AnimationBoneMask.h"
#include "Engine/Animation/AnimationSkeleton.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    BoneMask::BoneMask( Skeleton const* pSkeleton )
        : m_pSkeleton( pSkeleton )
    {
        EE_ASSERT( pSkeleton != nullptr );
        m_weights.resize( pSkeleton->GetNumBones(), 0.0f );
    }

    BoneMask::BoneMask( Skeleton const* pSkeleton, float fixedWeight, float rootMotionWeight )
        : m_pSkeleton( pSkeleton )
    {
        EE_ASSERT( pSkeleton != nullptr );

        EE_ASSERT( fixedWeight >= 0.0f && fixedWeight <= 1.0f );
        m_weights.resize( pSkeleton->GetNumBones(), fixedWeight );

        EE_ASSERT( rootMotionWeight >= 0.0f && rootMotionWeight <= 1.0f );
        m_rootMotionWeight = rootMotionWeight;
    }

    BoneMask::BoneMask( BoneMask const& rhs )
    {
        EE_ASSERT( rhs.IsValid() );
        m_pSkeleton = rhs.m_pSkeleton;
        m_weights = rhs.m_weights;
        m_rootMotionWeight = rhs.m_rootMotionWeight;
    }

    BoneMask::BoneMask( BoneMask&& rhs )
    {
        EE_ASSERT( rhs.IsValid() );
        m_pSkeleton = rhs.m_pSkeleton;
        m_weights.swap( rhs.m_weights );
        m_rootMotionWeight = rhs.m_rootMotionWeight;
    }

    BoneMask::BoneMask( Skeleton const* pSkeleton, BoneMaskDefinition const& definition, float rootMotionWeight )
        : m_pSkeleton( pSkeleton )
    {
        EE_ASSERT( pSkeleton != nullptr && rootMotionWeight >= 0.0f && rootMotionWeight <= 1.0f );
        m_weights.resize( pSkeleton->GetNumBones() );
        ResetWeights( definition, rootMotionWeight );
    }

    //-------------------------------------------------------------------------

    bool BoneMask::IsValid() const
    {
        return m_pSkeleton != nullptr && !m_weights.empty() && m_weights.size() == m_pSkeleton->GetNumBones();
    }

    BoneMask& BoneMask::operator=( BoneMask const& rhs )
    {
        m_pSkeleton = rhs.m_pSkeleton;
        m_weights = rhs.m_weights;
        m_rootMotionWeight = rhs.m_rootMotionWeight;
        return *this;
    }

    BoneMask& BoneMask::operator=( BoneMask&& rhs )
    {
        m_pSkeleton = rhs.m_pSkeleton;
        m_weights.swap( rhs.m_weights );
        m_rootMotionWeight = rhs.m_rootMotionWeight;
        return *this;
    }

    //-------------------------------------------------------------------------

    void BoneMask::ResetWeights( float fixedWeight, float rootMotionWeight )
    {
        EE_ASSERT( fixedWeight >= 0.0f && fixedWeight <= 1.0f );
        for ( auto& weight : m_weights )
        {
            weight = fixedWeight;
        }

        EE_ASSERT( rootMotionWeight >= 0.0f && rootMotionWeight <= 1.0f );
        m_rootMotionWeight = rootMotionWeight;
    }

    void BoneMask::ResetWeights( TVector<float> const& weights, float rootMotionWeight )
    {
        EE_ASSERT( m_pSkeleton != nullptr );

        EE_ASSERT( weights.size() == m_pSkeleton->GetNumBones() );
        m_weights = weights;

        EE_ASSERT( rootMotionWeight >= 0.0f && rootMotionWeight <= 1.0f );
        m_rootMotionWeight = rootMotionWeight;
    }

    void BoneMask::ResetWeights( BoneMaskDefinition const& definition, float rootMotionWeight, bool shouldFeatherIntermediateBones )
    {
        EE_ASSERT( m_pSkeleton != nullptr );

        if ( shouldFeatherIntermediateBones )
        {
            // Set all weights to -1 so we know what needs to be feathered!
            for ( auto& weight : m_weights )
            {
                weight = -1.0f;
            }

            // Root cannot be set to -1, since we cannot guarantee it will be part of the supplied weights
            m_weights[0] = 0.0f;
        }
        else
        {
            ResetWeights();
        }

        // Relatively expensive remap
        for ( auto const& boneWeight : definition.GetWeights() )
        {
            EE_ASSERT( boneWeight.m_weight >= 0.0f && boneWeight.m_weight <= 1.0f );
            int32_t const boneIdx = m_pSkeleton->GetBoneIndex( boneWeight.m_boneID );
            EE_ASSERT( boneIdx != InvalidIndex );
            m_weights[boneIdx] = boneWeight.m_weight;
        }

        // Feather intermediate weights
        if ( shouldFeatherIntermediateBones )
        {
            TInlineVector<int32_t, 50> boneChainIndices;

            for ( int32_t boneIdx = m_pSkeleton->GetNumBones() - 1; boneIdx > 0; boneIdx-- )
            {
                // Check for zero chains
                if ( m_weights[boneIdx] == -1 )
                {
                    boneChainIndices.clear();
                    boneChainIndices.emplace_back( boneIdx );

                    int32_t parentBoneIdx = m_pSkeleton->GetParentBoneIndex( boneIdx );
                    while ( parentBoneIdx != InvalidIndex )
                    {
                        // Exit when we encounter the first parent with a weight
                        if ( m_weights[parentBoneIdx] != -1 )
                        {
                            break;
                        }

                        boneChainIndices.emplace_back( parentBoneIdx );
                        parentBoneIdx = m_pSkeleton->GetParentBoneIndex( parentBoneIdx );
                    }

                    // Set all weights in the chain to 0.0f
                    for ( auto i : boneChainIndices )
                    {
                        m_weights[i] = 0.0f;
                    }
                }
                // Check for feather chains
                else if ( m_weights[m_pSkeleton->GetParentBoneIndex( boneIdx )] == -1 )
                {
                    float endWeight = m_weights[boneIdx];
                    EE_ASSERT( endWeight != -1.0f );
                    float startWeight = 0.0f;

                    boneChainIndices.clear();
                    boneChainIndices.emplace_back( boneIdx );

                    int32_t parentBoneIdx = m_pSkeleton->GetParentBoneIndex( boneIdx );
                    while ( parentBoneIdx != InvalidIndex )
                    {
                        boneChainIndices.emplace_back( parentBoneIdx );

                        // Exit when we encounter the first parent with a weight
                        if ( m_weights[parentBoneIdx] != -1 )
                        {
                            startWeight = m_weights[parentBoneIdx];
                            break;
                        }

                        parentBoneIdx = m_pSkeleton->GetParentBoneIndex( parentBoneIdx );
                    }

                    // Interpolate all weights in the chain
                    float featherWeight = startWeight;
                    int32_t const numBonesInChain = (int32_t) boneChainIndices.size();

                    for ( int32_t i = numBonesInChain - 2; i > 0; i-- )
                    {
                        float const percentageThrough = float( i ) / ( numBonesInChain - 1 );
                        m_weights[boneChainIndices[i]] = Math::Lerp( endWeight, startWeight, percentageThrough );
                    }
                }
            }
        }

        // Set root motion weight
        EE_ASSERT( rootMotionWeight >= 0.0f && rootMotionWeight <= 1.0f );
        m_rootMotionWeight = rootMotionWeight;
    }

    BoneMask& BoneMask::operator*=( BoneMask const& rhs )
    {
        int32_t const numWeights = (int32_t) m_weights.size();
        for ( auto i = 0; i < numWeights; i++ )
        {
            m_weights[i] *= rhs.m_weights[i];
        }

        m_rootMotionWeight *= rhs.m_rootMotionWeight;

        return *this;
    }

    void BoneMask::BlendFrom( BoneMask const& source, float blendWeight )
    {
        EE_ASSERT( source.m_pSkeleton == m_pSkeleton && m_weights.size() == source.m_weights.size() && blendWeight >= 0.0f && blendWeight <= 1.0f );

        auto const numWeights = m_weights.size();
        for ( auto i = 0; i < numWeights; i++ )
        {
            m_weights[i] = Math::Lerp( source.m_weights[i], m_weights[i], blendWeight );
        }

        m_rootMotionWeight = Math::Lerp( source.m_rootMotionWeight, m_rootMotionWeight, blendWeight );
    }

    void BoneMask::BlendTo( BoneMask const& target, float blendWeight )
    {
        EE_ASSERT( target.m_pSkeleton == m_pSkeleton && m_weights.size() == target.m_weights.size() && blendWeight >= 0.0f && blendWeight <= 1.0f );

        auto const numWeights = m_weights.size();
        for ( auto i = 0; i < numWeights; i++ )
        {
            m_weights[i] = Math::Lerp( m_weights[i], target[i], blendWeight );
        }

        m_rootMotionWeight = Math::Lerp( m_rootMotionWeight, target.m_rootMotionWeight, blendWeight );
    }

    //-------------------------------------------------------------------------

    BoneMaskPool::BoneMaskPool( Skeleton const* pSkeleton )
        : m_firstFreePoolIdx( InvalidIndex )
    {
        EE_ASSERT( m_pool.empty() && pSkeleton != nullptr );

        for ( auto i = 0; i < s_initialPoolSize; i++ )
        {
            m_pool.emplace_back( EE::New<BoneMask>( pSkeleton ) );
        }

        m_firstFreePoolIdx = 0;
    }

    BoneMaskPool::~BoneMaskPool()
    {
        for ( auto pMask : m_pool )
        {
            EE::Delete( pMask );
        }

        m_pool.clear();
        m_firstFreePoolIdx = InvalidIndex;
    }

    BoneMask* BoneMaskPool::GetBoneMask()
    {
        EE_ASSERT( m_firstFreePoolIdx < m_pool.size() );
        auto pMask = m_pool[m_firstFreePoolIdx];
        m_firstFreePoolIdx++;

        // If we reached the end of the pool, grow it
        if ( m_firstFreePoolIdx == m_pool.size() )
        {
            size_t const numMasksToAdd = m_pool.size() / 2;
            for ( auto i = 0; i < numMasksToAdd; i++ )
            {
                m_pool.emplace_back( EE::New<BoneMask>( m_pool[0]->GetSkeleton() ) );
            }
        }

        EE_ASSERT( pMask != nullptr );
        return pMask;
    }
}