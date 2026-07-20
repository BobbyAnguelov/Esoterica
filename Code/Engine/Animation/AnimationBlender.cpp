#include "AnimationBlender.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void Blender::ModelSpaceBlend( Skeleton::LOD skeletonLOD, Pose const* pBasePose, Pose const* pLayerPose, float layerWeight, BoneMask const* pBoneMask, Pose* pResultPose )
    {
        EE_ASSERT( pBoneMask != nullptr ); // Global space blends require a bone mask!
        EE_ASSERT( pBasePose != nullptr && pLayerPose != nullptr && pResultPose != nullptr );
        EE_ASSERT( pBasePose->GetSkeleton() == pLayerPose->GetSkeleton() );
        EE_ASSERT( !pBasePose->IsAdditivePose() );
        EE_ASSERT( !pLayerPose->IsAdditivePose() );
        EE_ASSERT( pBasePose != pResultPose ); // Base pose is used right at the end for the final result so it must not be modified!!!

        // Check for early-out condition
        //-------------------------------------------------------------------------

        if ( layerWeight == 0.0f )
        {
            pResultPose->CopyFrom( pBasePose );
            return;
        }

        if ( pBoneMask != nullptr && pBoneMask->IsZeroWeightMask() )
        {
            pResultPose->CopyFrom( pBasePose );
            return;
        }

        // Calculate global rotations for base and layer poses
        //-------------------------------------------------------------------------

        TVector<int32_t> const& parentIndices = pBasePose->GetSkeleton()->GetParentBoneIndices();
        int32_t const numBones = pResultPose->GetNumBones( skeletonLOD );

        TInlineVector<Quaternion, 200> baseRotations;
        TInlineVector<Quaternion, 200> layerRotations;
        TInlineVector<Quaternion, 200> resultRotations;

        baseRotations.resize( numBones );
        layerRotations.resize( numBones );
        resultRotations.resize( numBones );

        baseRotations[0] = pBasePose->m_parentSpaceTransforms[0].GetRotation();
        layerRotations[0] = pLayerPose->m_parentSpaceTransforms[0].GetRotation();

        for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
        {
            int32_t const parentIdx = parentIndices[boneIdx];
            baseRotations[boneIdx] = pBasePose->m_parentSpaceTransforms[boneIdx].GetRotation() * baseRotations[parentIdx];
            layerRotations[boneIdx] = pLayerPose->m_parentSpaceTransforms[boneIdx].GetRotation() * layerRotations[parentIdx];
        }

        // Blend the root separately - local space blend
        //-------------------------------------------------------------------------

        float boneBlendWeight = pBoneMask->GetWeight( 0 );
        if ( boneBlendWeight != 0.0f )
        {
            Transform::DirectlySetTranslationScale( pResultPose->m_parentSpaceTransforms[0], BlendFunction::BlendTranslationAndScale( pBasePose->m_parentSpaceTransforms[0].GetTranslationAndScale(), pLayerPose->m_parentSpaceTransforms[0].GetTranslationAndScale(), boneBlendWeight ) );
            resultRotations[0] = BlendFunction::BlendRotation( pBasePose->m_parentSpaceTransforms[0].GetRotation(), pLayerPose->m_parentSpaceTransforms[0].GetRotation(), boneBlendWeight );
        }
        else
        {
            resultRotations[0] = pBasePose->m_parentSpaceTransforms[0].GetRotation();
        }

        // Blend global space poses together and convert back to local space
        //-------------------------------------------------------------------------

        for ( int32_t boneIdx = 1; boneIdx < numBones; boneIdx++ )
        {
            // Use the source local pose for masked out bones
            boneBlendWeight = pBoneMask->GetWeight( boneIdx );
            if ( boneBlendWeight == 0.0f )
            {
                resultRotations[boneIdx] = baseRotations[boneIdx];
                pResultPose->SetTransform( boneIdx, pBasePose->GetTransform( boneIdx ) );
            }
            else // Perform Blend
            {
                // Blend translations
                //-------------------------------------------------------------------------
                // Translation blending is done in local space

                Transform::DirectlySetTranslationScale( pResultPose->m_parentSpaceTransforms[boneIdx], BlendFunction::BlendTranslationAndScale( pBasePose->m_parentSpaceTransforms[boneIdx].GetTranslationAndScale(), pLayerPose->m_parentSpaceTransforms[boneIdx].GetTranslationAndScale(), boneBlendWeight ) );

                // Blend Rotation
                //-------------------------------------------------------------------------

                resultRotations[boneIdx] = BlendFunction::BlendRotation( baseRotations[boneIdx], layerRotations[boneIdx], boneBlendWeight);

                // Convert blended global space rotation to local space for the result pose
                int32_t const parentIdx = parentIndices[boneIdx];
                Quaternion const localRotation = Quaternion::Delta( resultRotations[parentIdx], resultRotations[boneIdx] );
                Transform::DirectlySetRotation( pResultPose->m_parentSpaceTransforms[boneIdx], localRotation );
            }
        }

        // Blend the results of the global mask onto the base pose
        //-------------------------------------------------------------------------

        ParentSpaceBlend<BlendFunction>( skeletonLOD, pBasePose, pResultPose, layerWeight, pResultPose, false );
        pResultPose->ClearModelSpaceTransforms();
        pResultPose->m_state = Pose::State::ParentSpacePose;
    }

    void Blender::ApplyAdditiveFloatChannelsToReferencePose( FloatChannelSetValues const &additive, float const blendWeight, FloatChannelSetValues &result )
    {
        EE_ASSERT( additive.m_pSet == result.m_pSet );
        EE_ASSERT( result.m_values.size() == additive.m_values.size() && blendWeight >= 0.0f && blendWeight <= 1.0f );

        // Full in source
        if ( blendWeight == 0.0f )
        {
            result.Reset();
        }
        else if ( blendWeight == 1.0f )
        {
            result.CopyFrom( additive );
        }
        else // Additive Blend
        {
            bool hasNonZeroValue = false;
            Vector const vBlendWeight( blendWeight );
            int32_t const numWeights = (int32_t) result.m_values.size();
            for ( int32_t i = 0; i < numWeights; i += 4 )
            {
                Vector const vAdditive( &additive.m_values[i] );
                Vector const vResult = vBlendWeight * vAdditive;
                hasNonZeroValue |= !vResult.IsNearZero4();
                vResult.Store( &result.m_values[i] );
            }

            result.m_state = hasNonZeroValue ? FloatChannelSetValues::State::Set : FloatChannelSetValues::State::Unset;
        }
    }
}