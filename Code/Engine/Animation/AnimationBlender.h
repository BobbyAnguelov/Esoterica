#pragma once

#include "Engine/_Module/API.h"
#include "AnimationBoneMask.h"
#include "System/Animation/AnimationPose.h"
#include "System/Math/Quaternion.h"
#include "System/Types/BitFlags.h"
#include "System/TypeSystem/RegisteredType.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    enum class PoseBlendOptions
    {
        EE_REGISTER_ENUM

        Additive = 0,
        GlobalSpace,
    };

    enum class RootMotionBlendMode
    {
        EE_REGISTER_ENUM

        Blend = 0,
        Additive,
        IgnoreSource,
        IgnoreTarget
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API Blender
    {
    private:

        struct InterpolativeBlender
        {
            inline static Quaternion BlendRotation( Quaternion const& quat0, Quaternion const& quat1, float t )
            {
                return Quaternion::SLerp( quat0, quat1, t );
            }

            inline static Vector BlendTranslation( Vector const& trans0, Vector const& trans1, float t )
            {
                return Vector::Lerp( trans0, trans1, t );
            }

            inline static Vector BlendScale( Vector const& scale0, Vector const& scale1, float t )
            {
                return Vector::Lerp( scale0, scale1, t );
            }
        };

        struct AdditiveBlender
        {
            inline static Quaternion BlendRotation( Quaternion const& quat0, Quaternion const& quat1, float t )
            {
                Quaternion const targetQuat = quat0 * quat1;
                return Quaternion::SLerp( quat0, targetQuat, t );
            }

            inline static Vector BlendTranslation( Vector const& trans0, Vector const& trans1, float t )
            {
                return Vector::MultiplyAdd( trans1, Vector( t ), trans0 ).SetW1();
            }

            inline static Vector BlendScale( Vector const& scale0, Vector const& scale1, float t )
            {
                return Vector::MultiplyAdd( scale1, Vector( t ), scale0 ).SetW1();
            }
        };

        struct BlendWeight
        {
            inline static float GetBlendWeight( float const blendWeight, BoneMask const* pBoneMask, int32_t const boneIdx )
            {
                return blendWeight;
            }
        };

        struct BoneWeight
        {
            inline static float GetBlendWeight( float const blendWeight, BoneMask const* pBoneMask, int32_t const boneIdx )
            {
                return blendWeight * pBoneMask->GetWeight( boneIdx );
            }
        };

    public:

        static void Blend( Pose const* pSourcePose, Pose const* pTargetPose, float blendWeight, TBitFlags<PoseBlendOptions> blendOptions, BoneMask const* pBoneMask, Pose* pResultPose );

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
                    result.SetScale( Vector::One );
                }
                else // Regular blend
                {
                    result.SetRotation( InterpolativeBlender::BlendRotation( source.GetRotation(), target.GetRotation(), blendWeight ) );
                    result.SetTranslation( InterpolativeBlender::BlendTranslation( source.GetTranslation(), target.GetTranslation(), blendWeight ).SetW1() );
                    result.SetScale( Vector::One );
                }
            }

            return result;
        }
    };
}