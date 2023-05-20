#pragma once

#include "Engine/_Module/API.h"
#include "AnimationBoneMask.h"
#include "Engine/Animation/AnimationPose.h"
#include "System/Math/Quaternion.h"
#include "System/Types/BitFlags.h"
#include "System/TypeSystem/ReflectedType.h"

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

    enum class PoseBlendMode : uint8_t
    {
        EE_REFLECT_ENUM

        Interpolative, // Regular blend
        Additive,
        InterpolativeGlobalSpace,
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

        struct InterpolativeBlender
        {
            EE_FORCE_INLINE static Quaternion BlendRotation( Quaternion const& quat0, Quaternion const& quat1, float t )
            {
                return Quaternion::SLerp( quat0, quat1, t );
            }

            EE_FORCE_INLINE static Vector BlendTranslation( Vector const& trans0, Vector const& trans1, float t )
            {
                return Vector::Lerp( trans0, trans1, t );
            }

            EE_FORCE_INLINE static float BlendScale( float const scale0, float const scale1, float t )
            {
                return Math::Lerp( scale0, scale1, t );
            }
        };

        struct AdditiveBlender
        {
            EE_FORCE_INLINE static Quaternion BlendRotation( Quaternion const& quat0, Quaternion const& quat1, float t )
            {
                Quaternion const targetQuat = quat1 * quat0;
                return Quaternion::SLerp( quat0, targetQuat, t );
            }

            EE_FORCE_INLINE static Vector BlendTranslation( Vector const& trans0, Vector const& trans1, float t )
            {
                return Vector::MultiplyAdd( trans1, Vector( t ), trans0 ).SetW1();
            }

            EE_FORCE_INLINE static float BlendScale( float const scale0, float const scale1, float t )
            {
                return scale0 + ( scale1 * t );
            }
        };

    public:

        // Local Interpolative Blend
        static void Blend( Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose );

        // Local Additive Blend
        static void BlendAdditive( Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose );

        // Global Space Interpolative Blend
        static void BlendGlobal( Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose );

        // Blend Entry
        EE_FORCE_INLINE static void Blend( PoseBlendMode blendMode, Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, BoneMask const* pBoneMask, Pose* pResultPose )
        {
            switch ( blendMode )
            {
                case PoseBlendMode::Interpolative:
                {
                    Blend( pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose );
                }
                break;

                case PoseBlendMode::Additive:
                {
                    BlendAdditive( pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose );
                }
                break;

                case PoseBlendMode::InterpolativeGlobalSpace:
                {
                    BlendGlobal( pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose );
                }
                break;
            }
        }

        //-------------------------------------------------------------------------

        inline static Transform BlendRootMotionDeltas( Transform const& source, Transform const& target, float blendWeight, RootMotionBlendMode blendMode = RootMotionBlendMode::Blend )
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
                    result.SetRotation( AdditiveBlender::BlendRotation( source.GetRotation(), target.GetRotation(), blendWeight ) );
                    result.SetTranslation( AdditiveBlender::BlendTranslation( source.GetTranslation(), target.GetTranslation(), blendWeight ) );
                    result.SetScale( 1.0f );
                }
                else // Regular blend
                {
                    result.SetRotation( InterpolativeBlender::BlendRotation( source.GetRotation(), target.GetRotation(), blendWeight ) );
                    result.SetTranslation( InterpolativeBlender::BlendTranslation( source.GetTranslation(), target.GetTranslation(), blendWeight ).SetW1() );
                    result.SetScale( 1.0f );
                }
            }

            return result;
        }
    };
}