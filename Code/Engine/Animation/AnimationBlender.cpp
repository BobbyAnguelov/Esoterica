#include "AnimationBlender.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    // Local Blend
    template<typename Blender>
    void BlenderLocal( Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, Pose* pResultPose, bool canFullyOptimizeBlend )
    {
        EE_ASSERT( blendWeight >= 0.0f && blendWeight <= 1.0f );
        EE_ASSERT( pSourcePose != nullptr && pTargetPose != nullptr && pResultPose != nullptr );

        // Fully in Source
        if ( blendWeight == 0.0f )
        {
            // If the source pose is the same as the result pose, then do nothing
            if ( pSourcePose == pResultPose )
            {
                return;
            }
            else
            {
                pResultPose->CopyFrom( pSourcePose );
            }
        }
        // Fully in target
        else if ( canFullyOptimizeBlend && blendWeight == 1.0f )
        {
            // If the source pose is the same as the result pose, then do nothing
            if ( pTargetPose == pResultPose )
            {
                return;
            }
            else
            {
                pResultPose->CopyFrom( pTargetPose );
            }
        }
        else // Blend
        {
            int32_t const numBones = pResultPose->GetNumBones();
            for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                Transform const& sourceTransform = pSourcePose->GetTransform( boneIdx );
                Transform const& targetTransform = pTargetPose->GetTransform( boneIdx );

                // Blend translations
                Vector const translation = Blender::BlendTranslation( sourceTransform.GetTranslation(), targetTransform.GetTranslation(), blendWeight );
                pResultPose->SetTranslation( boneIdx, translation );

                // Blend scales
                float const scale = Blender::BlendScale( sourceTransform.GetScale(), targetTransform.GetScale(), blendWeight );
                pResultPose->SetScale( boneIdx, scale );

                // Blend rotations
                Quaternion const rotation = Blender::BlendRotation( sourceTransform.GetRotation(), targetTransform.GetRotation(), blendWeight );
                pResultPose->SetRotation( boneIdx, rotation );
            }
        }
    }

    // Masked Local Blend
    template<typename Blender>
    void BlenderLocal( Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, BoneMask const* pBoneMask, Pose* pResultPose, bool canFullyOptimizeBlend )
    {
        EE_ASSERT( blendWeight >= 0.0f && blendWeight <= 1.0f );
        EE_ASSERT( pSourcePose != nullptr && pTargetPose != nullptr && pResultPose != nullptr );
        EE_ASSERT( pBoneMask != nullptr );
        EE_ASSERT( pBoneMask->GetNumWeights() == pSourcePose->GetSkeleton()->GetNumBones() );

        int32_t const numBones = pResultPose->GetNumBones();
        for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            // If the bone has been masked out
            float const boneBlendWeight = blendWeight * pBoneMask->GetWeight( boneIdx );
            if ( boneBlendWeight == 0.0f )
            { 
                pResultPose->SetTransform( boneIdx, pSourcePose->GetTransform( boneIdx ) );
            }
            if ( canFullyOptimizeBlend && boneBlendWeight == 1.0f )
            {
                pResultPose->SetTransform( boneIdx, pTargetPose->GetTransform( boneIdx ) );
            }
            else // Perform Blend
            {
                Transform const& sourceTransform = pSourcePose->GetTransform( boneIdx );
                Transform const& targetTransform = pTargetPose->GetTransform( boneIdx );

                // Blend translations
                Vector const translation = Blender::BlendTranslation( sourceTransform.GetTranslation(), targetTransform.GetTranslation(), boneBlendWeight );
                pResultPose->SetTranslation( boneIdx, translation );

                // Blend scales
                float const scale = Blender::BlendScale( sourceTransform.GetScale(), targetTransform.GetScale(), boneBlendWeight );
                pResultPose->SetScale( boneIdx, scale );

                // Blend rotations
                Quaternion const rotation = Blender::BlendRotation( sourceTransform.GetRotation(), targetTransform.GetRotation(), boneBlendWeight );
                pResultPose->SetRotation( boneIdx, rotation );
            }
        }
    }

    //-------------------------------------------------------------------------

    // Masked Global Blend
    template<typename Blender>
    void BlenderGlobal( Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
    {
        static auto const rootBoneIndex = 0;
        EE_ASSERT( blendWeight >= 0.0f && blendWeight <= 1.0f );
        EE_ASSERT( pSourcePose != nullptr && pTargetPose != nullptr && pResultPose != nullptr );
        EE_ASSERT( pBoneMask != nullptr );
        EE_ASSERT( pBoneMask->GetNumWeights() == pSourcePose->GetSkeleton()->GetNumBones() );

        // Blend the root separately - local space blend
        //-------------------------------------------------------------------------

        auto boneBlendWeight = blendWeight * pBoneMask->GetWeight( rootBoneIndex );
        if ( boneBlendWeight != 0.0f )
        {
            Vector const translation = Blender::BlendTranslation( pSourcePose->GetTransform( rootBoneIndex ).GetTranslation(), pTargetPose->GetTransform( rootBoneIndex ).GetTranslation(), boneBlendWeight );
            pResultPose->SetTranslation( rootBoneIndex, translation );

            float const scale = Blender::BlendScale( pSourcePose->GetTransform( rootBoneIndex ).GetScale(), pTargetPose->GetTransform( rootBoneIndex ).GetScale(), boneBlendWeight );
            pResultPose->SetScale( rootBoneIndex, scale );

            Quaternion const rotation = Blender::BlendRotation( pSourcePose->GetTransform( rootBoneIndex ).GetRotation(), pTargetPose->GetTransform( rootBoneIndex ).GetRotation(), boneBlendWeight );
            pResultPose->SetRotation( rootBoneIndex, rotation );
        }

        // Blend global space poses together and convert back to local space
        //-------------------------------------------------------------------------

        auto const& parentIndices = pSourcePose->GetSkeleton()->GetParentBoneIndices();
        auto const numBones = pResultPose->GetNumBones();
        for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
        {
            // Use the source local pose for masked out bones
            boneBlendWeight = blendWeight * pBoneMask->GetWeight( boneIdx );
            if ( boneBlendWeight == 0.0f )
            {
                pResultPose->SetTransform( boneIdx, pSourcePose->GetTransform( boneIdx ) );
            }
            else // Perform Blend
            {
                // Blend translations - translation blending is done in local space
                Vector const translation = Blender::BlendTranslation( pSourcePose->GetTransform( boneIdx ).GetTranslation(), pTargetPose->GetTransform( boneIdx ).GetTranslation(), boneBlendWeight );
                pResultPose->SetTranslation( boneIdx, translation );

                // Blend scales - scale blending is done in local space
                float const scale = Blender::BlendScale( pSourcePose->GetTransform( boneIdx ).GetScale(), pTargetPose->GetTransform( boneIdx ).GetScale(), boneBlendWeight );
                pResultPose->SetScale( boneIdx, scale );

                //-------------------------------------------------------------------------

                auto const parentIdx = parentIndices[boneIdx];
                auto const parentBoneBlendWeight = blendWeight * pBoneMask->GetWeight( parentIdx );

                // If the bone weights are the same i.e. inherited, then perform a local space blend
                if ( Math::IsNearEqual( boneBlendWeight, parentBoneBlendWeight ) )
                {
                    Quaternion const rotation = Blender::BlendRotation( pSourcePose->GetTransform( boneIdx ).GetRotation(), pTargetPose->GetTransform( boneIdx ).GetRotation(), boneBlendWeight );
                    pResultPose->SetRotation( boneIdx, rotation );
                }
                else // Perform a global space blend for this bone
                {
                    // TODO: If the calls to GetGlobalSpaceTransform() get expensive optimize this by creating a global space bone cache
                    Transform const sourceGlobalTransform = pSourcePose->GetGlobalTransform( boneIdx );
                    Transform const targetGlobalTransform = pTargetPose->GetGlobalTransform( boneIdx );
                    Quaternion const rotation = Blender::BlendRotation( sourceGlobalTransform.GetRotation(), targetGlobalTransform.GetRotation(), boneBlendWeight );

                    // Convert blended global space rotation to local space for the result pose
                    Quaternion const parentRotation = pResultPose->GetGlobalTransform( parentIdx ).GetRotation();
                    Quaternion const localRotation = rotation * parentRotation.GetConjugate();
                    pResultPose->SetRotation( boneIdx, localRotation );
                }
            }
        }
    }
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void Blender::Blend( Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
    {
        pResultPose->ClearGlobalTransforms();

        if ( pBoneMask != nullptr )
        {
            BlenderLocal<InterpolativeBlender>( pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose, true );
        }
        else
        {
            BlenderLocal<InterpolativeBlender>( pSourcePose, pTargetPose, blendWeight, pResultPose, true );
        }
    }

    void Blender::BlendAdditive( Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
    {
        pResultPose->ClearGlobalTransforms();

        if ( pBoneMask != nullptr )
        {
            BlenderLocal<AdditiveBlender>( pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose, false );
        }
        else
        {
            BlenderLocal<AdditiveBlender>( pSourcePose, pTargetPose, blendWeight, pResultPose, false );
        }
    }

    void Blender::BlendGlobal( Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
    {
        EE_ASSERT( pBoneMask != nullptr ); // Global space blends require a bone mask!

        pResultPose->ClearGlobalTransforms();
        BlenderGlobal<InterpolativeBlender>( pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose );
    }
}