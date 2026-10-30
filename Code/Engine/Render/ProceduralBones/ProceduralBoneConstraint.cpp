#include "ProceduralBoneConstraint.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    TInlineVector<ProceduralBoneConstraint::BoneChainElement, 20> ProceduralBoneConstraint::CalculateModelSpaceTransformsForChain( SkeletalMesh const* pMesh, TVector<Transform> const& parentSpaceTransforms, int32_t chainEndBoneIdx ) const
    {
        EE_ASSERT( pMesh != nullptr );
        EE_ASSERT( parentSpaceTransforms.size() == pMesh->GetNumBones() );
        EE_ASSERT( pMesh->IsValidBoneIndex( chainEndBoneIdx ) );

        // Get the entire chain
        TInlineVector<int32_t, 20> boneChainIndices;
        int32_t boneIdx = chainEndBoneIdx;
        while ( boneIdx != InvalidIndex )
        {
            boneChainIndices.emplace_back( boneIdx );
            boneIdx = pMesh->GetParentBoneIndex( boneIdx );
        }
        EE_ASSERT( boneChainIndices.back() == 0 );

        // Fill out chain
        TInlineVector<BoneChainElement, 20> chain;
        int32_t const numBonesInChain = int32_t( boneChainIndices.size() );
        chain.reserve( numBonesInChain );
        for ( int32_t i = numBonesInChain - 1; i >= 0; i-- )
        {
            boneIdx = boneChainIndices[i];
            chain.emplace_back( boneIdx, pMesh->GetBoneID( boneIdx ), parentSpaceTransforms[boneIdx], parentSpaceTransforms[boneIdx] );
        }

        // Calculate model space transforms
        for ( int32_t i = 1; i < numBonesInChain; i++ )
        {
            chain[i].m_modelSpaceTransform = chain[i].m_parentSpaceTransform * chain[i - 1].m_modelSpaceTransform;
        }

        return chain;
    }
}