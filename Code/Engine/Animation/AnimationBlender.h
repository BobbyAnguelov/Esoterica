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

            EE_FORCE_INLINE static Vector BlendFloatChannel( Vector const &fc0, Vector const &fc1, float const &t )
            {
                return Vector::Lerp( fc0, fc1, t );
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

            EE_FORCE_INLINE static Vector BlendFloatChannel( Vector const &fc0, Vector const &fc1, float const &t )
            {
                return Vector::MultiplyAdd( fc1, Vector( t ), fc0 );
            }
        };

    private:

        // Basic parent space blend
        template<typename BlendFunction>
        static inline void ParentSpaceBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, Pose* pResultPose, bool isLayeredBlend );

        // Basic parent space masked blend
        template<typename BlendFunction>
        static inline void ParentSpaceBlendMasked( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, BoneMask const* pBoneMask, Pose* pResultPose, bool isLayeredBlend );

        // Blend float channels
        template<typename BlendFunction>
        static inline void BlendFloatChannels( FloatChannelSetValues const &source, FloatChannelSetValues const &target, float const blendWeight, FloatChannelSetValues &result, bool isLayeredBlend );

    public:

        // Parent space blend
        EE_FORCE_INLINE static void ParentSpaceBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
        {
            if ( pBoneMask != nullptr )
            {
                ParentSpaceBlendMasked<BlendFunction>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose, false );
            }
            else
            {
                ParentSpaceBlend<BlendFunction>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pResultPose, false );
            }
        }

        // Parent space blend
        EE_FORCE_INLINE static void ParentSpaceOverlayBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
        {
            if ( pBoneMask != nullptr )
            {
                ParentSpaceBlendMasked<BlendFunction>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose, true );
            }
            else
            {
                ParentSpaceBlend<BlendFunction>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pResultPose, true );
            }
        }

        // Model space blend
        static void ModelSpaceBlend( Skeleton::LOD skeletonLOD, Pose const* pBasePose, Pose const* pLayerPose, float layerWeight, BoneMask const* pBoneMask, Pose* pResultPose );

        // Parent space Additive blend
        EE_FORCE_INLINE static void AdditiveBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
        {
            EE_ASSERT( pTargetPose->IsAdditivePose() );

            if ( pBoneMask != nullptr )
            {
                ParentSpaceBlendMasked<AdditiveBlendFunction>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose, true );
            }
            else
            {
                ParentSpaceBlend<AdditiveBlendFunction>( skeletonLOD, pSourcePose, pTargetPose, blendWeight, pResultPose, true );
            }
        }

        // Parent space Additive blend
        EE_FORCE_INLINE static void ApplyAdditiveToReferencePose( Skeleton::LOD skeletonLOD, Pose const* pAdditivePose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose );

        // Blend two root motion deltas together
        EE_FORCE_INLINE static Transform BlendRootMotionDeltas( Transform const& source, Transform const& target, float blendWeight, RootMotionBlendMode blendMode = RootMotionBlendMode::Blend );

        // Apply an additive float channel value set on top of a zero value set
        static void ApplyAdditiveFloatChannelsToReferencePose( FloatChannelSetValues const &additive, float const blendWeight, FloatChannelSetValues &result );
    };

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

    // Parent Space Blend
    template<typename BlendFunction>
    void Blender::ParentSpaceBlend( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, Pose* pResultPose, bool isLayeredBlend )
    {
        EE_ASSERT( blendWeight >= 0.0f && blendWeight <= 1.0f );
        EE_ASSERT( pSourcePose != nullptr && pTargetPose != nullptr && pResultPose != nullptr );
        EE_ASSERT( pSourcePose->IsPoseSet() && pTargetPose->IsPoseSet() );

        Pose::State const finalState = ( pSourcePose->IsAdditivePose() && pTargetPose->IsAdditivePose() ) ? Pose::State::AdditivePose : Pose::State::ParentSpacePose;

        // Fully in source
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

        int32_t const numFloatChannelSets = pSourcePose->GetNumFloatChannelSets();
        for ( int32_t setIdx = 0; setIdx < numFloatChannelSets; setIdx++ )
        {
            BlendFloatChannels<BlendFunction>( pSourcePose->m_floatChannelSetValues[setIdx], pTargetPose->m_floatChannelSetValues[setIdx], blendWeight, pResultPose->m_floatChannelSetValues[setIdx], true );
        }

        //-------------------------------------------------------------------------

        pResultPose->m_state = finalState;
    }

    // Masked Parent Space Blend
    template<typename BlendFunction>
    void Blender::ParentSpaceBlendMasked( Skeleton::LOD skeletonLOD, Pose const* pSourcePose, Pose const* pTargetPose, float const blendWeight, BoneMask const* pBoneMask, Pose* pResultPose, bool isLayeredBlend )
    {
        EE_ASSERT( blendWeight >= 0.0f && blendWeight <= 1.0f );
        EE_ASSERT( pSourcePose != nullptr && pTargetPose != nullptr && pResultPose != nullptr );
        EE_ASSERT( pSourcePose->IsPoseSet() && pTargetPose->IsPoseSet() );
        EE_ASSERT( pBoneMask != nullptr );

        Pose::State const finalState = ( pSourcePose->IsAdditivePose() && pTargetPose->IsAdditivePose() ) ? Pose::State::AdditivePose : Pose::State::ParentSpacePose;

        // Check early out conditions
        //-------------------------------------------------------------------------

        if ( blendWeight == 0.0f )
        {
            if ( pSourcePose != pResultPose )
            {
                pResultPose->CopyFrom( pSourcePose );
                return;
            }
        }

        if ( !isLayeredBlend && blendWeight == 1.0f )
        {
            if ( pTargetPose != pResultPose )
            {
                pResultPose->CopyFrom( pTargetPose );
                return;
            }
        }

        if ( pBoneMask != nullptr )
        {
            if ( pBoneMask->IsZeroWeightMask() )
            {
                if ( pSourcePose != pResultPose )
                {
                    pResultPose->CopyFrom( pSourcePose );
                }
                return;
            }
        }

        //-------------------------------------------------------------------------

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

        int32_t const numFloatChannelSets = pSourcePose->GetNumFloatChannelSets();
        for ( int32_t setIdx = 0; setIdx < numFloatChannelSets; setIdx++ )
        {
            BlendFloatChannels<BlendFunction>( pSourcePose->m_floatChannelSetValues[setIdx], pTargetPose->m_floatChannelSetValues[setIdx], blendWeight, pResultPose->m_floatChannelSetValues[setIdx], true );
        }

        //-------------------------------------------------------------------------

        pResultPose->m_state = finalState;
    }

    //-------------------------------------------------------------------------

    // Float channel blend
    template<typename BlendFunction>
    void Blender::BlendFloatChannels( FloatChannelSetValues const &source, FloatChannelSetValues const &target, float const blendWeight, FloatChannelSetValues &result, bool isLayeredBlend )
    {
        EE_ASSERT( source.m_pSet == target.m_pSet && source.m_pSet == result.m_pSet );
        EE_ASSERT( source.m_values.size() == target.m_values.size() && blendWeight >= 0.0f && blendWeight <= 1.0f );

        // Full in source
        if ( blendWeight == 0.0f )
        {
            // If the source pose is different from the result pose then copy the transforms
            if ( &source != &result )
            {
                result.CopyFrom( source );
            }
        }
        // Fully in target - If we're not blending on top of a pose, then we can skip the blend
        if ( !isLayeredBlend && blendWeight == 1.0f )
        {
            // If the source pose is different from the result pose then copy
            if ( &target != &result )
            {
                result.CopyFrom( target );
            }
        }
        else // Blend
        {
            bool hasNonZeroValue = false;
            int32_t const numWeights = (int32_t) source.m_values.size();
            for ( int32_t i = 0; i < numWeights; i += 4 )
            {
                Vector const vSource( &source.m_values[i] );
                Vector const vTarget( &target.m_values[i] );
                Vector const vResult = BlendFunction::BlendFloatChannel( vSource, vTarget, blendWeight );
                hasNonZeroValue |= !vResult.IsNearZero4();
                vResult.Store( &result.m_values[i] );
            }

            result.m_state = hasNonZeroValue ? FloatChannelSetValues::State::Set : FloatChannelSetValues::State::Unset;
        }
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
            pResultPose->Reset( Pose::Init::ReferencePose );
        }
        else // Blend
        {
            TVector<Transform> const& referencePose = pAdditivePose->GetSkeleton()->GetParentSpaceReferencePose();
            int32_t const numBones = pResultPose->GetNumBones( skeletonLOD );

            if ( pBoneMask != nullptr )
            {
                for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
                {
                    Transform const& sourceTransform = referencePose[boneIdx];

                    // If the bone has been masked out
                    float const boneBlendWeight = blendWeight * pBoneMask->GetWeight( boneIdx );
                    if ( boneBlendWeight == 0.0f )
                    {
                        pResultPose->SetTransform( boneIdx, sourceTransform );
                    }
                    else
                    {
                        Transform const& targetTransform = pAdditivePose->m_parentSpaceTransforms[boneIdx];
                        Transform::DirectlySetRotation( pResultPose->m_parentSpaceTransforms[boneIdx], AdditiveBlendFunction::BlendRotation( sourceTransform.GetRotation(), targetTransform.GetRotation(), boneBlendWeight ) );
                        Transform::DirectlySetTranslationScale( pResultPose->m_parentSpaceTransforms[boneIdx], AdditiveBlendFunction::BlendTranslationAndScale( sourceTransform.GetTranslationAndScale(), targetTransform.GetTranslationAndScale(), boneBlendWeight ) );
                    }
                }
            }
            else // No mask
            {
                for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
                {
                    Transform const& sourceTransform = referencePose[boneIdx];
                    Transform const& targetTransform = pAdditivePose->m_parentSpaceTransforms[boneIdx];
                    Transform::DirectlySetRotation( pResultPose->m_parentSpaceTransforms[boneIdx], AdditiveBlendFunction::BlendRotation( sourceTransform.GetRotation(), targetTransform.GetRotation(), blendWeight ) );
                    Transform::DirectlySetTranslationScale( pResultPose->m_parentSpaceTransforms[boneIdx], AdditiveBlendFunction::BlendTranslationAndScale( sourceTransform.GetTranslationAndScale(), targetTransform.GetTranslationAndScale(), blendWeight ) );
                }
            }

        }

        //-------------------------------------------------------------------------

        pResultPose->ClearModelSpaceTransforms();
        pResultPose->m_state = Pose::State::ParentSpacePose;

        //-------------------------------------------------------------------------

        int32_t const numFloatChannelSets = pAdditivePose->GetNumFloatChannelSets();
        for ( int32_t setIdx = 0; setIdx < numFloatChannelSets; setIdx++ )
        {
            if ( pAdditivePose->m_floatChannelSetValues[setIdx].IsUnset() )
            {
                continue;
            }

            ApplyAdditiveFloatChannelsToReferencePose( pAdditivePose->m_floatChannelSetValues[setIdx], blendWeight, pResultPose->m_floatChannelSetValues[setIdx] );
        }
    }
}