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

        // Basic local space blend
        template<typename BlendFunction>
        static inline void LocalBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, Pose* pResultPose, bool isLayeredBlend );

        // Basic local space masked blend
        template<typename BlendFunction>
        static inline void LocalBlendMasked( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, BoneMask const* pBoneMask, Pose* pResultPose, bool isLayeredBlend );

        // Blend to a reference pose
        template<typename BlendFunction>
        static EE_FORCE_INLINE void LocalBlendToReferencePose( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, float const blendWeight, BoneMask const* pBoneMask, Pose* pResultPose );

        // Blend from a reference pose
        template<typename BlendFunction>
        static EE_FORCE_INLINE void LocalBlendFromReferencePose( Skeleton::LOD skeletonLOD, Pose const* pTargetPose, float const blendWeight, BoneMask const* pBoneMask, Pose* pResultPose );

    public:

        // Local Interpolative Blend
        EE_FORCE_INLINE static void LocalBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose );

        // Blend to a reference pose
        EE_FORCE_INLINE static void LocalBlendToReferencePose( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
        {
            LocalBlendToReferencePose<BlendFunction>( skeletonLOD, pSourcePose, blendWeight, pBoneMask, pResultPose );
        }

        // Blend from a reference pose
        EE_FORCE_INLINE static void LocalBlendFromReferencePose( Skeleton::LOD skeletonLOD, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
        {
            LocalBlendFromReferencePose<BlendFunction>( skeletonLOD, pTargetPose, blendWeight, pBoneMask, pResultPose );
        }

        // Global Space Interpolative Blend
        static void GlobalBlend( Skeleton::LOD skeletonLOD, Pose const* pBasePose, Pose const* pLayerPose, float layerWeight, BoneMask const* pBoneMask, Pose* pResultPose );

        // Local Additive Blend
        EE_FORCE_INLINE static void AdditiveBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose );

        // Local Additive Blend
        EE_FORCE_INLINE static void ApplyAdditiveToReferencePose( Skeleton::LOD skeletonLOD, Pose const* pAdditivePose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose );

        // Blend two root motion deltas together
        EE_FORCE_INLINE static Transform BlendRootMotionDeltas( Transform const& source, Transform const& target, float blendWeight, RootMotionBlendMode blendMode = RootMotionBlendMode::Blend );
    };

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE void Blender::LocalBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
    {
        if ( pBoneMask != nullptr )
        {
            LocalBlendMasked<BlendFunction>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose, false );
        }
        else
        {
            LocalBlend<BlendFunction>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pResultPose, false );
        }
    }

    EE_FORCE_INLINE void Blender::AdditiveBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
    {
        if ( pBoneMask != nullptr )
        {
            LocalBlendMasked<AdditiveBlendFunction>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose, true );
        }
        else
        {
            LocalBlend<AdditiveBlendFunction>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pResultPose, true );
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
    void Blender::LocalBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, Pose* pResultPose, bool isLayeredBlend )
    {
        EE_ASSERT( blendWeight >= 0.0f && blendWeight <= 1.0f );
        EE_ASSERT( pSourcePose != nullptr && pTargetPose != nullptr && pResultPose != nullptr );

        Pose::State const finalState = ( pSourcePose->IsAdditivePose() && pTargetPose->IsAdditivePose() ) ? Pose::State::AdditivePose : Pose::State::Pose;

        // Full in source
        if ( blendWeight == 0.0f )
        {
            // If the source pose is different from the result pose then copy the transforms
            if ( pSourcePose != pResultPose )
            {
                pResultPose->CopyFrom( pSourcePose );
            }
        }
        // Fully in target - If we're not blending on top of a pose, then we can skip the blend
        else if ( !isLayeredBlend && blendWeight == 1.0f )
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
                Transform const& sourceTransform = pSourcePose->m_parentSpaceTransforms[boneIdx];
                Transform const& targetTransform = pTargetPose->m_parentSpaceTransforms[boneIdx];
                Transform::DirectlySetRotation( pResultPose->m_parentSpaceTransforms[boneIdx], BlendFunction::BlendRotation( sourceTransform.GetRotation(), targetTransform.GetRotation(), blendWeight ) );
                Transform::DirectlySetTranslationScale( pResultPose->m_parentSpaceTransforms[boneIdx], BlendFunction::BlendTranslationAndScale( sourceTransform.GetTranslationAndScale(), targetTransform.GetTranslationAndScale(), blendWeight ) );
            }

            pResultPose->ClearModelSpaceTransforms();
        }

        //-------------------------------------------------------------------------

        pResultPose->m_state = finalState;
    }

    // Masked Local Blend
    template<typename BlendFunction>
    void Blender::LocalBlendMasked( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, BoneMask const* pBoneMask, Pose* pResultPose, bool isLayeredBlend )
    {
        EE_ASSERT( blendWeight >= 0.0f && blendWeight <= 1.0f );
        EE_ASSERT( pSourcePose != nullptr && pTargetPose != nullptr && pResultPose != nullptr );
        EE_ASSERT( pBoneMask != nullptr );

        Pose::State const finalState = ( pSourcePose->IsAdditivePose() && pTargetPose->IsAdditivePose() ) ? Pose::State::AdditivePose : Pose::State::Pose;

        // Fully in Source
        if ( blendWeight == 0.0f )
        {
            // If the source pose is different from the result pose then copy the transforms
            if ( pSourcePose != pResultPose )
            {
                pResultPose->CopyFrom( pSourcePose );
            }
        }
        else // Perform blend
        {
            int32_t const numBones = pResultPose->GetNumBones( skeletonLOD );
            for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                // If the bone has been masked out
                float const boneBlendWeight = blendWeight * pBoneMask->GetWeight( boneIdx );
                if ( boneBlendWeight == 0.0f )
                {
                    pResultPose->SetTransform( boneIdx, pSourcePose->GetTransform( boneIdx ) );
                }
                // If we're not blending on top of a pose, then we can skip the blend
                else if ( !isLayeredBlend && boneBlendWeight == 1.0f )
                {
                    pResultPose->SetTransform( boneIdx, pTargetPose->GetTransform( boneIdx ) );
                }
                else // Perform Blend
                {
                    Transform const& sourceTransform = pSourcePose->m_parentSpaceTransforms[boneIdx];
                    Transform const& targetTransform = pTargetPose->m_parentSpaceTransforms[boneIdx];
                    Transform::DirectlySetRotation( pResultPose->m_parentSpaceTransforms[boneIdx], BlendFunction::BlendRotation( sourceTransform.GetRotation(), targetTransform.GetRotation(), boneBlendWeight ) );
                    Transform::DirectlySetTranslationScale( pResultPose->m_parentSpaceTransforms[boneIdx], BlendFunction::BlendTranslationAndScale( sourceTransform.GetTranslationAndScale(), targetTransform.GetTranslationAndScale(), boneBlendWeight ) );
                }
            }

            pResultPose->ClearModelSpaceTransforms();
        }

        //-------------------------------------------------------------------------

        pResultPose->m_state = finalState;
    }

    // Blend to a reference pose
    template<typename BlendFunction>
    void Blender::LocalBlendToReferencePose( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
    {
        EE_ASSERT( blendWeight >= 0.0f && blendWeight <= 1.0f );
        EE_ASSERT( pSourcePose != nullptr && pResultPose != nullptr );

        Pose::State const finalState = ( pSourcePose->IsAdditivePose() ) ? Pose::State::AdditivePose : Pose::State::Pose;

        // Fully in source pose
        if ( blendWeight == 0.0f )
        {
            // If the source pose is different from the result pose then copy
            if ( pSourcePose != pResultPose )
            {
                pResultPose->CopyFrom( pSourcePose );
            }
        }
        // Fully in reference pose
        else if ( blendWeight == 1.0f )
        {
            pResultPose->Reset( ( finalState == Pose::State::AdditivePose ) ? Pose::Type::ZeroPose : Pose::Type::ReferencePose );
        }
        else // Blend
        {
            TVector<Transform> const& referencePose = pSourcePose->GetSkeleton()->GetLocalReferencePose();
            int32_t const numBones = pResultPose->GetNumBones( skeletonLOD );
            for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                Transform const& sourceTransform = pSourcePose->m_parentSpaceTransforms[boneIdx];
                Transform const& targetTransform = referencePose[boneIdx];
                Transform::DirectlySetRotation( pResultPose->m_parentSpaceTransforms[boneIdx], BlendFunction::BlendRotation( sourceTransform.GetRotation(), targetTransform.GetRotation(), blendWeight ) );
                Transform::DirectlySetTranslationScale( pResultPose->m_parentSpaceTransforms[boneIdx], BlendFunction::BlendTranslationAndScale( sourceTransform.GetTranslationAndScale(), targetTransform.GetTranslationAndScale(), blendWeight ) );
            }

            pResultPose->ClearModelSpaceTransforms();
        }

        //-------------------------------------------------------------------------

        pResultPose->m_state = finalState;
    }

    // Blend from a reference pose
    template<typename BlendFunction>
    void Blender::LocalBlendFromReferencePose( Skeleton::LOD skeletonLOD, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
    {
        EE_ASSERT( blendWeight >= 0.0f && blendWeight <= 1.0f );
        EE_ASSERT( pTargetPose != nullptr && pResultPose != nullptr );

        Pose::State const finalState = ( pTargetPose->IsAdditivePose() ) ? Pose::State::AdditivePose : Pose::State::Pose;

        // Fully in reference pose
        if ( blendWeight == 0.0f )
        {
            pResultPose->Reset( ( finalState == Pose::State::AdditivePose ) ? Pose::Type::ZeroPose : Pose::Type::ReferencePose );
        }
        // Fully in target pose
        else if ( blendWeight == 1.0f )
        {
            // If the source pose is different from the result pose then copy
            if ( pTargetPose != pResultPose )
            {
                pResultPose->CopyFrom( pTargetPose );
            }
        }
        else // Blend
        {
            TVector<Transform> const& referencePose = pTargetPose->GetSkeleton()->GetLocalReferencePose();
            int32_t const numBones = pResultPose->GetNumBones( skeletonLOD );
            for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                Transform const& sourceTransform = referencePose[boneIdx];
                Transform const& targetTransform = pTargetPose->m_parentSpaceTransforms[boneIdx];
                Transform::DirectlySetRotation( pResultPose->m_parentSpaceTransforms[boneIdx], BlendFunction::BlendRotation( sourceTransform.GetRotation(), targetTransform.GetRotation(), blendWeight ) );
                Transform::DirectlySetTranslationScale( pResultPose->m_parentSpaceTransforms[boneIdx], BlendFunction::BlendTranslationAndScale( sourceTransform.GetTranslationAndScale(), targetTransform.GetTranslationAndScale(), blendWeight ) );
            }

            pResultPose->ClearModelSpaceTransforms();
        }

        //-------------------------------------------------------------------------

        pResultPose->m_state = finalState;
    }

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE void Blender::ApplyAdditiveToReferencePose( Skeleton::LOD skeletonLOD, Pose const* pAdditivePose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
    {
        EE_ASSERT( blendWeight > 0.0f && blendWeight <= 1.0f );
        EE_ASSERT( pAdditivePose != nullptr && pResultPose != nullptr );
        EE_ASSERT( pAdditivePose->IsAdditivePose() );

        // Fully in reference pose
        if ( blendWeight == 0.0f )
        {
            pResultPose->Reset( Pose::Type::ReferencePose );
        }
        else // Blend
        {
            TVector<Transform> const& referencePose = pAdditivePose->GetSkeleton()->GetLocalReferencePose();
            int32_t const numBones = pResultPose->GetNumBones( skeletonLOD );
            for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                Transform const& sourceTransform = referencePose[boneIdx];
                Transform const& targetTransform = pAdditivePose->m_parentSpaceTransforms[boneIdx];
                Transform::DirectlySetRotation( pResultPose->m_parentSpaceTransforms[boneIdx], AdditiveBlendFunction::BlendRotation( sourceTransform.GetRotation(), targetTransform.GetRotation(), blendWeight ) );
                Transform::DirectlySetTranslationScale( pResultPose->m_parentSpaceTransforms[boneIdx], AdditiveBlendFunction::BlendTranslationAndScale( sourceTransform.GetTranslationAndScale(), targetTransform.GetTranslationAndScale(), blendWeight ) );
            }

            pResultPose->ClearModelSpaceTransforms();
        }

        //-------------------------------------------------------------------------

        pResultPose->m_state = Pose::State::Pose;
    }
}