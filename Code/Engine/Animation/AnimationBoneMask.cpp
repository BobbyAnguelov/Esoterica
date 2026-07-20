#include "AnimationBoneMask.h"
#include "AnimationSkeleton.h"
#include "Base/Math/Lerp.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void BoneWeightList::Clear()
    {
        m_skeletonID.Clear();
        m_boneIDs.clear();
        m_weights.clear();
    }

    bool BoneWeightList::IsValid() const
    {
        if ( !m_skeletonID.IsValid() )
        {
            return false;
        }

        if ( m_boneIDs.size() != m_weights.size() )
        {
            return false;
        }

        for ( auto& boneID : m_boneIDs )
        {
            if ( !boneID.IsValid() )
            {
                return false;
            }
        }

        for ( auto& boneWeight : m_weights )
        {
            if ( boneWeight < 0.0f || boneWeight > 1.0f )
            {
                return false;
            }
        }

        return true;
    }

    bool BoneWeightList::RemoveDuplicateWeights()
    {
        bool fixupPerformed = false;

        for ( int32_t i = 0; i < m_boneIDs.size(); i++ )
        {
            for ( int32_t j = i + 1; j < m_boneIDs.size(); j++ )
            {
                if ( m_boneIDs[i] == m_boneIDs[j] )
                {
                    m_boneIDs.erase_unsorted( m_boneIDs.begin() + j );
                    j--;
                    fixupPerformed = true;
                }
            }
        }

        return fixupPerformed;
    }

    bool BoneWeightList::RemoveWeightsForMissingBones( Skeleton const* pSkeleton )
    {
        bool fixupPerformed = false;

        if ( m_boneIDs.size() != m_weights.size() )
        {
            size_t newSize = Math::Min( m_boneIDs.size(), m_weights.size() );
            m_boneIDs.resize( newSize );
            m_weights.resize( newSize, 0.0f );
            fixupPerformed = true;
        }

        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < m_boneIDs.size(); i++ )
        {
            bool isInvalidWeight = false;
            if ( !m_boneIDs[i].IsValid() )
            {
                isInvalidWeight = true;
            }

            if ( pSkeleton->GetBoneIndex( m_boneIDs[i] ) == InvalidIndex )
            {
                isInvalidWeight = true;
            }

            if ( m_weights[i] < 0.0f || m_weights[i] > 1.0f )
            {
                isInvalidWeight = true;
            }

            //-------------------------------------------------------------------------

            if ( isInvalidWeight )
            {
                m_boneIDs.erase( m_boneIDs.begin() + i );
                m_weights.erase( m_weights.begin() + i );
                i--;

                fixupPerformed = true;
            }
        }

        return fixupPerformed;
    }

    void BoneWeightList::SetWeightForBone( StringID boneID, float weight )
    {
        EE_ASSERT( m_boneIDs.size() == m_weights.size() );
        EE_ASSERT( weight >= 0.0f && weight <= 1.0f );

        int32_t numWeights = (int32_t) m_weights.size();
        for ( int32_t i = 0; i < numWeights; i++ )
        {
            if ( m_boneIDs[i] == boneID )
            {
                m_weights[i] = weight;
                return;
            }
        }

        m_boneIDs.emplace_back( boneID );
        m_weights.emplace_back( weight );
    }

    void BoneWeightList::UnsetWeightForBone( StringID boneID )
    {
        EE_ASSERT( m_boneIDs.size() == m_weights.size() );

        int32_t numWeights = (int32_t) m_weights.size();
        for ( int32_t i = 0; i < numWeights; i++ )
        {
            if ( m_boneIDs[i] == boneID )
            {
                VectorEraseUnordered( m_boneIDs, i );
                VectorEraseUnordered( m_weights, i );
                return;
            }
        }
    }

    float BoneWeightList::GetWeightForBone( StringID boneID ) const
    {
        EE_ASSERT( m_boneIDs.size() == m_weights.size() );

        int32_t numWeights = (int32_t) m_weights.size();
        for ( int32_t i = 0; i < numWeights; i++ )
        {
            if ( m_boneIDs[i] == boneID )
            {
                return m_weights[i];
            }
        }

        return 0.0f;
    }

    bool BoneWeightList::HasWeightForBone( StringID boneID ) const
    {
        EE_ASSERT( m_boneIDs.size() == m_weights.size() );

        int32_t numWeights = (int32_t) m_weights.size();
        for ( int32_t i = 0; i < numWeights; i++ )
        {
            if ( m_boneIDs[i] == boneID )
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    BoneMask const* BoneMaskSet::TryGetBoneMask( Skeleton const* pSkeleton ) const
    {
        BoneMask const* pMask = nullptr;

        if ( m_primaryMask.GetSkeleton() == pSkeleton )
        {
            pMask = &m_primaryMask;
            return pMask;
        }

        //-------------------------------------------------------------------------

        for ( auto const& secondaryBoneMask : m_secondaryMasks )
        {
            if ( secondaryBoneMask.GetSkeleton() == pSkeleton )
            {
                pMask = &secondaryBoneMask;
                break;
            }
        }

        return pMask;
    }

    //-------------------------------------------------------------------------

    BoneMask::BoneMask( Skeleton const* pSkeleton )
        : m_pSkeleton( pSkeleton )
    {
        EE_ASSERT( m_pSkeleton != nullptr );
        EE_ASSERT( m_pSkeleton->GetNumBones() > 0 );

        int32_t const numWeightsToAllocate = GetNumRequiredWeights();
        m_weights.resize( numWeightsToAllocate, 0.0f );
        m_weightInfo = WeightInfo::Zero;
    }

    BoneMask::BoneMask( Skeleton const* pSkeleton, float fixedWeight )
        : m_pSkeleton( pSkeleton )
    {
        ResetWeights( fixedWeight );
    }

    BoneMask::BoneMask( BoneMask const& rhs )
    {
        EE_ASSERT( rhs.IsValid() );
        m_pSkeleton = rhs.m_pSkeleton;
        m_weights = rhs.m_weights;
        m_weightInfo = rhs.m_weightInfo;
    }

    BoneMask::BoneMask( BoneMask&& rhs )
    {
        EE_ASSERT( rhs.IsValid() );
        EE_ASSERT( rhs.m_pSkeleton != nullptr && rhs.m_pSkeleton->GetNumBones() > 0 );
        m_pSkeleton = rhs.m_pSkeleton;
        m_weights.swap( rhs.m_weights );
        m_weightInfo = rhs.m_weightInfo;
    }

    BoneMask::BoneMask( Skeleton const* pSkeleton, TVector<float> const& weights )
        : m_pSkeleton( pSkeleton )
    {
        EE_ASSERT( m_pSkeleton != nullptr );
        EE_ASSERT( m_pSkeleton->GetNumBones() > 0 );

        int32_t const numWeightsToAllocate = GetNumRequiredWeights();
        m_weights.resize( numWeightsToAllocate );
        ResetWeights( weights );
    }

    BoneMask::BoneMask( Skeleton const* pSkeleton, BoneWeightList const& weightList, TInlineVector<StringID, 10>* pOutMissingBones )
        : m_pSkeleton( pSkeleton )
    {
        EE_ASSERT( m_pSkeleton != nullptr );
        EE_ASSERT( m_pSkeleton->GetNumBones() > 0 );
        EE_ASSERT( weightList.IsValid() );

        if ( pOutMissingBones != nullptr )
        {
            pOutMissingBones->clear();
        }

        // Create weight storage
        //-------------------------------------------------------------------------

        int32_t const numWeightsToAllocate = GetNumRequiredWeights();
        m_weights.resize( numWeightsToAllocate, 0 );
        m_weightInfo = WeightInfo::Zero;

        // Handle empty bone weight list
        //-------------------------------------------------------------------------

        if ( weightList.GetNumWeights() == 0 )
        {
            ResetWeights( 1.0f );
        }

        // Create intermediate weights
        //-------------------------------------------------------------------------

        int32_t const numWeights = BoneMask::GetNumRequiredWeights();
        TInlineVector<float, 300> weights;
        weights.resize( numWeights, -1.0f );

        int32_t const numWeightsInList = weightList.GetNumWeights();
        for ( int32_t i = 0; i < numWeightsInList; i++ )
        {
            EE_ASSERT( weightList.m_weights[i] >= 0.0f && weightList.m_weights[i] <= 1.0f );
            EE_ASSERT( weightList.m_boneIDs[i].IsValid() );

            int32_t const boneIdx = pSkeleton->GetBoneIndex( weightList.m_boneIDs[i] );
            if ( boneIdx != InvalidIndex )
            {
                weights[boneIdx] = weightList.m_weights[i];
            }
            else // If we should return the list of missing bone, add the missing bone to the list
            {
                if ( pOutMissingBones != nullptr )
                {
                    pOutMissingBones->emplace_back( weightList.m_boneIDs[i] );
                }
            }
        }

        ResetWeights( weights );
    }

    bool BoneMask::IsValid() const
    {
        return m_pSkeleton != nullptr && !m_weights.empty() && m_weights.size() == GetNumRequiredWeights();
    }

    int32_t BoneMask::GetNumRequiredWeights() const
    {
        int32_t const numBones = m_pSkeleton->GetNumBones();
        return Math::CeilingToInt32( float( numBones ) / 4.0f ) * 4;
    }

    BoneMask& BoneMask::operator=( BoneMask const& rhs )
    {
        m_pSkeleton = rhs.m_pSkeleton;
        m_weights = rhs.m_weights;
        m_weightInfo = rhs.m_weightInfo;

        EE_ASSERT( m_weights.size() % 4 == 0 );

        return *this;
    }

    BoneMask& BoneMask::operator=( BoneMask&& rhs )
    {
        m_pSkeleton = rhs.m_pSkeleton;
        m_weights.swap( rhs.m_weights );
        m_weightInfo = rhs.m_weightInfo;

        EE_ASSERT( m_weights.size() % 4 == 0 );

        return *this;
    }

    void BoneMask::ResetWeightsToZero()
    {
        EE_ASSERT( IsValid() );

        if ( m_weightInfo != WeightInfo::Zero )
        {
            Memory::MemsetZero( m_weights.data(), m_weights.size() );
            m_weightInfo = WeightInfo::Zero;
        }
    }

    void BoneMask::ResetWeightsToOne()
    {
        EE_ASSERT( IsValid() );

        if ( m_weightInfo != WeightInfo::One )
        {
            for ( auto& weight : m_weights )
            {
                weight = 1.0f;
            }

            m_weightInfo = WeightInfo::One;
        }
    }

    void BoneMask::ResetWeights( float fixedWeight )
    {
        EE_ASSERT( m_pSkeleton != nullptr );
        EE_ASSERT( m_pSkeleton->GetNumBones() > 0 );
        EE_ASSERT( fixedWeight >= 0.0f && fixedWeight <= 1.0f );

        int32_t const numWeightsToAllocate = GetNumRequiredWeights();
        m_weights.resize( numWeightsToAllocate );

        //-------------------------------------------------------------------------

        if ( fixedWeight == 0.0f )
        {
            return ResetWeightsToZero();
        }
        else if ( fixedWeight == 1.0f )
        {
            return ResetWeightsToOne();
        }
        else
        {
            for ( auto& weight : m_weights )
            {
                weight = fixedWeight;
            }

            SetWeightInfo( fixedWeight );
        }
    }

    void BoneMask::ResetWeights( float const* pWeights )
    {
        EE_ASSERT( m_pSkeleton != nullptr );

        int32_t const numWeights = GetNumRequiredWeights();
        m_weights.resize( numWeights );

        TInlineVector<float, 255> originalWeights;
        originalWeights.resize( numWeights );

        memcpy( m_weights.data(), pWeights, sizeof( float ) * numWeights );
        memcpy( originalWeights.data(), pWeights, sizeof( float ) * numWeights );

        //-------------------------------------------------------------------------

        // Feather intermediate weights
        TInlineVector<int32_t, 50> boneChainIndices;

        for ( int32_t boneIdx = m_pSkeleton->GetNumBones() - 1; boneIdx > 0; boneIdx-- )
        {
            // Check for zero chains
            if ( m_weights[boneIdx] == -1 )
            {
                boneChainIndices.clear();
                boneChainIndices.emplace_back( boneIdx );

                float chainWeight = 0.0f;
                int32_t parentBoneIdx = m_pSkeleton->GetParentBoneIndex( boneIdx );
                while ( parentBoneIdx != InvalidIndex )
                {
                    // Exit when we encounter the first parent with a weight
                    if ( originalWeights[parentBoneIdx] != -1 )
                    {
                        chainWeight = originalWeights[parentBoneIdx];
                        break;
                    }

                    boneChainIndices.emplace_back( parentBoneIdx );
                    parentBoneIdx = m_pSkeleton->GetParentBoneIndex( parentBoneIdx );
                }

                // Do not update the root bone via this operation
                if ( parentBoneIdx == InvalidIndex )
                {
                    boneChainIndices.pop_back();
                }

                // Set all weights in the chain to 0.0f
                for ( auto i : boneChainIndices )
                {
                    m_weights[i] = chainWeight;
                }
            }
            // Check for feather chains
            else if ( m_weights[m_pSkeleton->GetParentBoneIndex( boneIdx )] == -1 )
            {
                float endWeight = m_weights[boneIdx];
                EE_ASSERT( endWeight != -1.0f );
                float startWeight = -1.0f;

                boneChainIndices.clear();
                boneChainIndices.emplace_back( boneIdx );

                int32_t parentBoneIdx = m_pSkeleton->GetParentBoneIndex( boneIdx );
                while ( parentBoneIdx != InvalidIndex )
                {
                    boneChainIndices.emplace_back( parentBoneIdx );

                    // Exit when we encounter the first parent with a weight
                    if ( originalWeights[parentBoneIdx] != -1 )
                    {
                        startWeight = originalWeights[parentBoneIdx];
                        break;
                    }

                    parentBoneIdx = m_pSkeleton->GetParentBoneIndex( parentBoneIdx );
                }

                // Interpolate all weights in the chain
                int32_t const numBonesInChain = (int32_t) boneChainIndices.size();
                for ( int32_t i = numBonesInChain - 2; i > 0; i-- )
                {
                    float const percentageThrough = float( i ) / ( numBonesInChain - 1 );
                    if ( startWeight != -1 )
                    {
                        m_weights[boneChainIndices[i]] = Math::Lerp( endWeight, startWeight, percentageThrough );
                    }
                    else
                    {
                        m_weights[boneChainIndices[i]] = 0.0f;
                    }
                }
            }
        }

        // Explicitly update root weight since user may have left it unset
        if ( m_weights[0] == -1.0f )
        {
            m_weights[0] = 0.0f;
        }

        // Weight info
        //-------------------------------------------------------------------------

        m_weightInfo = WeightInfo::Zero;
        float fixedWeight = m_weights[0];
        for ( int32_t i = 1; i < m_pSkeleton->GetNumBones(); i++ )
        {
            if ( m_weights[i] != fixedWeight )
            {
                m_weightInfo = WeightInfo::Mixed;
                break;
            }
        }

        if ( m_weightInfo != WeightInfo::Mixed )
        {
            SetWeightInfo( fixedWeight );
        }
    }

    BoneMask& BoneMask::operator*=( BoneMask const& rhs )
    {
        int32_t const numWeights = (int32_t) m_weights.size();
        for ( auto i = 0; i < numWeights; i++ )
        {
            m_weights[i] *= rhs.m_weights[i];
        }
        return *this;
    }

    void BoneMask::BlendFrom( BoneMask const& source, float blendWeight )
    {
        EE_ASSERT( source.m_pSkeleton == m_pSkeleton && m_weights.size() == source.m_weights.size() && blendWeight >= 0.0f && blendWeight <= 1.0f );

        // If we are blending with ourselves, do nothing
        if ( &source == this )
        {
            return;
        }

        // If this the mask types are the same and not mixed weight, then this is a no-op
        if ( source.m_weightInfo != WeightInfo::Mixed && source.m_weightInfo == m_weightInfo )
        {
            return;
        }

        // We are asking to be fully in this mask
        if ( Math::IsNearEqual( blendWeight, 1.0f ) )
        {
            return;
        }

        // We are asking to be fully in the source mask
        if ( Math::IsNearEqual( blendWeight, 0.0f ) )
        {
            m_weights = source.m_weights;
            m_weightInfo = source.m_weightInfo;
            return;
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( m_weights.size() % 4 == 0 );

        Vector const vBlendWeight( blendWeight );
        size_t const numWeights = m_weights.size();
        for ( size_t i = 0; i < numWeights; i += 4 )
        {
            Vector const vSource( &source.m_weights[i] );
            Vector const vTarget( &m_weights[i] );
            Vector const vResult = Vector::Lerp( vSource, vTarget, blendWeight );
            vResult.Store( &m_weights[i] );
        }

        //-------------------------------------------------------------------------

        m_weightInfo = WeightInfo::Mixed;
    }

    void BoneMask::BlendTo( BoneMask const& target, float blendWeight )
    {
        EE_ASSERT( target.m_pSkeleton == m_pSkeleton && m_weights.size() == target.m_weights.size() && blendWeight >= 0.0f && blendWeight <= 1.0f );

        // If we are blending with ourselves, do nothing
        if ( &target == this )
        {
            return;
        }

        // If this the mask weights are the same and not mixed weight, then this is a no-op
        if ( target.m_weightInfo != WeightInfo::Mixed && target.m_weightInfo == m_weightInfo )
        {
            return;
        }

        // We are asking to be fully in this mask
        if ( Math::IsNearEqual( blendWeight, 0.0f ) )
        {
            return;
        }

        // We are asking to be fully in the source mask
        if ( Math::IsNearEqual( blendWeight, 1.0f ) )
        {
            m_weights = target.m_weights;
            m_weightInfo = target.m_weightInfo;
            return;
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( m_weights.size() % 4 == 0 );

        Vector const vBlendWeight( blendWeight );
        size_t const numWeights = m_weights.size();
        for ( size_t i = 0; i < numWeights; i += 4 )
        {
            Vector const vSource( &m_weights[i]  );
            Vector const vTarget( &target.m_weights[i] );
            Vector const vResult = Vector::Lerp( vSource, vTarget, blendWeight );
            vResult.Store( &m_weights[i] );
        }

        m_weightInfo = WeightInfo::Mixed;
    }

    void BoneMask::ScaleWeights( float scale )
    {
        EE_ASSERT( IsValid() && scale >= 0.0f && scale <= 1.0f );

        if ( scale == 1.0f )
        {
            return;
        }

        if ( scale == 0.0f )
        {
            ResetWeights();
            return;
        }

        //-------------------------------------------------------------------------

        Vector vScale( scale );
        size_t const numWeights = m_weights.size();
        for ( size_t i = 0; i < numWeights; i += 4 )
        {
            Vector const vWeights( &m_weights[i] );
            Vector const vScaledWeights = vWeights * vScale;
            vScaledWeights.Store( &m_weights[i] );
        }
    }

    //-------------------------------------------------------------------------

    bool BoneMaskSetDefinition::IsValid() const
    {
        if ( !m_ID.IsValid() )
        {
            return false;
        }

        if ( !m_primaryWeightList.IsValid() )
        {
            return false;
        }

        for ( auto& weightList : m_secondaryWeightLists )
        {
            if ( !weightList.IsValid() )
            {
                return false;
            }
        }

        return true;
    }
}