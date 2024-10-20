#include "ResourceDescriptor_AnimationSkeleton.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    bool BoneMaskDefinition::IsValid() const
    {
        if ( !m_ID.IsValid() )
        {
            return false;
        }

        for ( auto& boneWeight : m_weights )
        {
            if ( !boneWeight.m_boneID.IsValid() || boneWeight.m_weight < 0.0f || boneWeight.m_weight > 1.0f )
            {
                return false;
            }
        }

        return true;
    }

    void BoneMaskDefinition::GenerateSerializedBoneMask( Skeleton const* pSkeleton, BoneMask::SerializedData& outData, TVector<StringID>* pOutDetectedMissingBones ) const
    {
        EE_ASSERT( pSkeleton != nullptr );
        EE_ASSERT( IsValid() );

        int32_t const numWeights = BoneMask::CalculateNumWeightsToSet( pSkeleton->GetNumBones() );

        outData.m_ID = m_ID;
        outData.m_weights.resize( numWeights, -1.0f );

        if ( pOutDetectedMissingBones != nullptr )
        {
            pOutDetectedMissingBones->clear();
        }

        for ( BoneWeight const& boneWeight : m_weights )
        {
            EE_ASSERT( boneWeight.m_weight >= 0.0f && boneWeight.m_weight <= 1.0f );
            int32_t const boneIdx = pSkeleton->GetBoneIndex( boneWeight.m_boneID );
            if ( boneIdx != InvalidIndex )
            {
                outData.m_weights[boneIdx] = boneWeight.m_weight;
            }
            else // If we should return the list of missing bone, add the missing bone to the list
            {
                if ( pOutDetectedMissingBones != nullptr )
                {
                    pOutDetectedMissingBones->emplace_back( boneWeight.m_boneID );
                }
            }
        }
    }
}