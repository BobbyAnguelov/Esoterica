#pragma once

#include "Engine/_Module/API.h"
#include "AnimationBoneMask.h"
#include "Engine/Animation/AnimationPose.h"
#include "Base/Math/Quaternion.h"
#include "Base/Types/BitFlags.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    enum class RootMotionBlendMode : uint8_t
    {
        EE_REFLECT_ENUM

        Blend = 0,
        Additive,
        IgnoreSource,
        IgnoreTarget
    };

    

    // Commonly need state enum
    enum class BlendState : uint8_t
    {
        None = 0,
        BlendingIn,
        BlendingOut,

        // Optional values to provide more context in some scenarios
        FullyIn,
        FullyOut = None,
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API Blender
    {
    private:

        struct BlendFunction
        {
            EE_FORCE_INLINE static Quaternion BlendRotation( Quaternion const& quat0, Quaternion const& quat1, float t )
            {
                return Quaternion::SLerp( quat0, quat1, t );
            }

            EE_FORCE_INLINE static Vector BlendTranslationAndScale( Vector const& translationScale0, Vector const& translationScale1, float t )
            {
                return Vector::Lerp( translationScale0, translationScale1, t );
            }
        };

        struct BlendFunctionFastSLerp
        {
            EE_FORCE_INLINE static Quaternion BlendRotation( Quaternion const& quat0, Quaternion const& quat1, float t )
            {
                return Quaternion::FastSLerp( quat0, quat1, t );
            }

            EE_FORCE_INLINE static Vector BlendTranslationAndScale( Vector const& translationScale0, Vector const& translationScale1, float t )
            {
                return Vector::Lerp( translationScale0, translationScale1, t );
            }
        };

        struct AdditiveBlendFunction
        {
            EE_FORCE_INLINE static Quaternion BlendRotation( Quaternion const& quat0, Quaternion const& quat1, float t )
            {
                Quaternion const targetQuat = quat1 * quat0;
                return Quaternion::SLerp( quat0, targetQuat, t );
            }

            EE_FORCE_INLINE static Vector BlendTranslationAndScale( Vector const& translationScale0, Vector const& translationScale1, float t )
            {
                return Vector::MultiplyAdd( translationScale1, Vector( t ), translationScale0 );
            }
        };

    private:

        // Basic local space blend - the early out is a useful optimization for non-additive blends
        template<typename BlendFunction>
        static inline void LocalBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, Pose* pResultPose, bool canEarlyOutOfBlend );

        // Basic local space masked blend - the early out is a useful optimization for non-additive blends
        template<typename BlendFunction>
        static inline void LocalBlendMasked( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, BoneMask const* pBoneMask, Pose* pResultPose, bool canEarlyOutOfPerBoneBlend );

    public:

        // Local Interpolative Blend
        EE_FORCE_INLINE static void LocalBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose, bool useNLerp = false );

        // Global Space Interpolative Blend
        static void GlobalBlend( Skeleton::LOD skeletonLOD, Pose const* pBasePose, Pose const* pLayerPose, float layerWeight, BoneMask const* pBoneMask, Pose* pResultPose );

        // Local Additive Blend
        EE_FORCE_INLINE static void AdditiveBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose );

        // Blend two root motion deltas together
        EE_FORCE_INLINE static Transform BlendRootMotionDeltas( Transform const& source, Transform const& target, float blendWeight, RootMotionBlendMode blendMode = RootMotionBlendMode::Blend );
    };

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE void Blender::LocalBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose, bool useFastSLerp )
    {
        // Fully in Source
        if ( blendWeight == 0.0f )
        {
            Pose::State const finalState = ( pSourcePose->IsAdditivePose() && pTargetPose->IsAdditivePose() ) ? Pose::State::AdditivePose : Pose::State::Pose;

            // If the source pose is different from the result pose then copy the transforms
            if ( pSourcePose != pResultPose )
            {
                pResultPose->CopyFrom( pSourcePose );
            }

            pResultPose->m_state = finalState;
        }
        else // Perform the blend
        {
            if ( useFastSLerp )
            {
                if ( pBoneMask != nullptr )
                {
                    LocalBlendMasked<BlendFunctionFastSLerp>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose, true );
                }
                else
                {
                    LocalBlend<BlendFunctionFastSLerp>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pResultPose, true );
                }
            }
            else
            {
                if ( pBoneMask != nullptr )
                {
                    LocalBlendMasked<BlendFunction>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose, true );
                }
                else
                {
                    LocalBlend<BlendFunction>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pResultPose, true );
                }
            }
        }
    }

    EE_FORCE_INLINE void Blender::AdditiveBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
    {
        // Fully in Source
        if ( blendWeight == 0.0f )
        {
            Pose::State const finalState = ( pSourcePose->IsAdditivePose() && pTargetPose->IsAdditivePose() ) ? Pose::State::AdditivePose : Pose::State::Pose;

            // If the source pose is different from the result pose then copy the transforms
            if ( pSourcePose != pResultPose )
            {
                pResultPose->CopyFrom( pSourcePose );
            }

            pResultPose->m_state = finalState;
        }
        else // Perform the blend
        {
            if ( pBoneMask != nullptr )
            {
                LocalBlendMasked<AdditiveBlendFunction>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose, false );
            }
            else
            {
                LocalBlend<AdditiveBlendFunction>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pResultPose, false );
            }
        }
    }

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE Transform Blender::BlendRootMotionDeltas( Transform const& source, Transform const& target, float blendWeight, RootMotionBlendMode blendMode )
    {
        Transform result;

        if ( blendWeight <= 0.0f || blendMode == RootMotionBlendMode::IgnoreTarget )
        {
            result = source;
        }
        else if ( blendWeight >= 1.0f || blendMode == RootMotionBlendMode::IgnoreSource )
        {
            result = target;
        }
        else
        {
            if ( blendMode == RootMotionBlendMode::Additive )
            {
                result.SetRotation( AdditiveBlendFunction::BlendRotation( source.GetRotation(), target.GetRotation(), blendWeight ) );
                result.SetTranslationAndScale( AdditiveBlendFunction::BlendTranslationAndScale( source.GetTranslation(), target.GetTranslation(), blendWeight ).GetWithW1() );
            }
            else // Regular blend
            {
                result.SetRotation( BlendFunction::BlendRotation( source.GetRotation(), target.GetRotation(), blendWeight ) );
                result.SetTranslationAndScale( BlendFunction::BlendTranslationAndScale( source.GetTranslation(), target.GetTranslation(), blendWeight ).GetWithW1() );
            }
        }

        return result;
    }

    //-------------------------------------------------------------------------

    // Local Blend
    template<typename BlendFunction>
    void Blender::LocalBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, Pose* pResultPose, bool canEarlyOutOfBlend )
    {
        EE_ASSERT( blendWeight > 0.0f && blendWeight <= 1.0f );
        EE_ASSERT( pSourcePose != nullptr && pTargetPose != nullptr && pResultPose != nullptr );

        Pose::State const finalState = ( pSourcePose->IsAdditivePose() && pTargetPose->IsAdditivePose() ) ? Pose::State::AdditivePose : Pose::State::Pose;

        // Fully in target
        if ( canEarlyOutOfBlend && blendWeight == 1.0f )
        {
            // If the source pose is different from the result pose then copy
            if ( pTargetPose != pResultPose )
            {
                pResultPose->CopyFrom( pTargetPose );
            }
        }
        else // Blend
        {
            int32_t const numBones = pResultPose->GetNumBones( skeletonLOD );
            for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                Transform const& sourceTransform = pSourcePose->m_localTransforms[boneIdx];
                Transform const& targetTransform = pTargetPose->m_localTransforms[boneIdx];
                Transform::DirectlySetRotation( pResultPose->m_localTransforms[boneIdx], BlendFunction::BlendRotation( sourceTransform.GetRotation(), targetTransform.GetRotation(), blendWeight ) );
                Transform::DirectlySetTranslationScale( pResultPose->m_localTransforms[boneIdx], BlendFunction::BlendTranslationAndScale( sourceTransform.GetTranslationAndScale(), targetTransform.GetTranslationAndScale(), blendWeight ) );
            }

            pResultPose->ClearGlobalTransforms();
        }

        //-------------------------------------------------------------------------

        pResultPose->m_state = finalState;
    }

    // Masked Local Blend
    template<typename BlendFunction>
    void Blender::LocalBlendMasked( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, BoneMask const* pBoneMask, Pose* pResultPose, bool canEarlyOutOfPerBoneBlend )
    {
        EE_ASSERT( blendWeight > 0.0f && blendWeight <= 1.0f );
        EE_ASSERT( pSourcePose != nullptr && pTargetPose != nullptr && pResultPose != nullptr );
        EE_ASSERT( pBoneMask != nullptr );

        int32_t const numBones = pResultPose->GetNumBones( skeletonLOD );
        for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            // If the bone has been masked out
            float const boneBlendWeight = blendWeight * pBoneMask->GetWeight( boneIdx );
            if ( boneBlendWeight == 0.0f )
            {
                pResultPose->SetTransform( boneIdx, pSourcePose->GetTransform( boneIdx ) );
            }
            else if ( canEarlyOutOfPerBoneBlend && boneBlendWeight == 1.0f )
            {
                pResultPose->SetTransform( boneIdx, pTargetPose->GetTransform( boneIdx ) );
            }
            else // Perform Blend
            {
                Transform const& sourceTransform = pSourcePose->m_localTransforms[boneIdx];
                Transform const& targetTransform = pTargetPose->m_localTransforms[boneIdx];
                Transform::DirectlySetRotation( pResultPose->m_localTransforms[boneIdx], BlendFunction::BlendRotation( sourceTransform.GetRotation(), targetTransform.GetRotation(), boneBlendWeight ) );
                Transform::DirectlySetTranslationScale( pResultPose->m_localTransforms[boneIdx], BlendFunction::BlendTranslationAndScale( sourceTransform.GetTranslationAndScale(), targetTransform.GetTranslationAndScale(), boneBlendWeight ) );
            }
        }

        //-------------------------------------------------------------------------

        pResultPose->ClearGlobalTransforms();
        pResultPose->m_state = ( pSourcePose->IsAdditivePose() && pTargetPose->IsAdditivePose() ) ? Pose::State::AdditivePose : Pose::State::Pose;
    }
}