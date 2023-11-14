#include "AnimationBlender.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void Blender::GlobalBlend( Skeleton::LOD skeletonLOD, Pose const* pBasePose, Pose const* pLayerPose, float layerWeight, BoneMask const* pBoneMask, Pose* pResultPose )
    {
        EE_ASSERT( pBoneMask != nullptr ); // Global space blends require a bone mask!
        EE_ASSERT( pBasePose != nullptr && pLayerPose != nullptr && pResultPose != nullptr );
        EE_ASSERT( pBasePose->GetSkeleton() == pLayerPose->GetSkeleton() );

        #if EE_DEVELOPMENT_TOOLS
        if ( pBasePose->IsAdditivePose() || pLayerPose->IsAdditivePose() )
        {
            EE_LOG_WARNING( "Animation", "Global Blend", "Attempting a global blend with an additive pose! This is undefined behavior!" );
        }
        #endif

        // Check for early-out condition
        //-------------------------------------------------------------------------

        if ( layerWeight == 0.0f )
        {
            if ( pBasePose != pResultPose )
            {
                pResultPose->CopyFrom( pBasePose );
            }
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

        baseRotations[0] = pBasePose->m_localTransforms[0].GetRotation();
        layerRotations[0] = pLayerPose->m_localTransforms[0].GetRotation();

        for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
        {
            int32_t const parentIdx = parentIndices[boneIdx];
            baseRotations[boneIdx] = pBasePose->m_localTransforms[boneIdx].GetRotation() * baseRotations[parentIdx];
            layerRotations[boneIdx] = pLayerPose->m_localTransforms[boneIdx].GetRotation() * layerRotations[parentIdx];
        }

        // Blend the root separately - local space blend
        //-------------------------------------------------------------------------

        auto boneBlendWeight = pBoneMask->GetWeight( 0 );
        if ( boneBlendWeight != 0.0f )
        {
            Transform::DirectlySetTranslationScale( pResultPose->m_localTransforms[0], BlendFunction::BlendTranslationAndScale( pBasePose->m_localTransforms[0].GetTranslationAndScale(), pLayerPose->m_localTransforms[0].GetTranslationAndScale(), boneBlendWeight ) );
            resultRotations[0] = BlendFunction::BlendRotation( pBasePose->m_localTransforms[0].GetRotation(), pLayerPose->m_localTransforms[0].GetRotation(), boneBlendWeight );
        }
        else
        {
            resultRotations[0] = pBasePose->m_localTransforms[0].GetRotation();
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

                Transform::DirectlySetTranslationScale( pResultPose->m_localTransforms[boneIdx], BlendFunction::BlendTranslationAndScale( pBasePose->m_localTransforms[boneIdx].GetTranslationAndScale(), pLayerPose->m_localTransforms[boneIdx].GetTranslationAndScale(), boneBlendWeight ) );

                // Blend Rotation
                //-------------------------------------------------------------------------

                resultRotations[boneIdx] = BlendFunction::BlendRotation( baseRotations[boneIdx], layerRotations[boneIdx], boneBlendWeight);

                // Convert blended global space rotation to local space for the result pose
                int32_t const parentIdx = parentIndices[boneIdx];
                Quaternion const localRotation = Quaternion::Delta( resultRotations[parentIdx], resultRotations[boneIdx] );
                Transform::DirectlySetRotation( pResultPose->m_localTransforms[boneIdx], localRotation );
            }
        }

        // Blend the results of the global mask onto the base pose
        //-------------------------------------------------------------------------

        LocalBlend<BlendFunction>( skeletonLOD, pBasePose, pResultPose, layerWeight, pResultPose, false );
        pResultPose->ClearGlobalTransforms();
        pResultPose->m_state = Pose::State::Pose;
    }
}