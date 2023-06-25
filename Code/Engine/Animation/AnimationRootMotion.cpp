#include "AnimationRootMotion.h"
#include "System/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void RootMotionData::Clear()
    {
        m_transforms.clear();
        m_averageLinearVelocity = 0.0f;
        m_averageAngularVelocity = 0.0f;
        m_totalDelta = Transform::Identity;
    }

    Vector RootMotionData::GetIncomingMovementDirection2DAtFrame( int32_t frameIdx ) const
    {
        EE_ASSERT( frameIdx >= 0 && frameIdx < m_transforms.size() );

        Vector result;

        if ( frameIdx == 0 )
        {
            result = m_transforms[frameIdx].GetRotation().RotateVector( Vector::WorldForward );
        }
        else
        {
            Vector const movementDelta = m_transforms[frameIdx].GetTranslation() - m_transforms[frameIdx - 1].GetTranslation();
            if ( movementDelta.IsNearZero2() )
            {
                result = m_transforms[frameIdx].GetRotation().RotateVector( Vector::WorldForward );
            }
            else
            {
                result = movementDelta.GetNormalized2();
            }
        }

        return result;
    }

    Quaternion RootMotionData::GetIncomingMovementOrientation2DAtFrame( int32_t frameIdx ) const
    {
        EE_ASSERT( frameIdx >= 0 && frameIdx < m_transforms.size() );

        Quaternion result;

        if ( frameIdx == 0 )
        {
            result = m_transforms[frameIdx].GetRotation();
        }
        else
        {
            Vector const movementDelta = m_transforms[frameIdx].GetTranslation() - m_transforms[frameIdx - 1].GetTranslation();
            if ( movementDelta.IsNearZero2() )
            {
                result = m_transforms[frameIdx].GetRotation();
            }
            else
            {
                result = Quaternion::FromRotationBetweenNormalizedVectors( Vector::WorldForward, movementDelta.GetNormalized2() );
            }
        }

        return result;
    }

    Vector RootMotionData::GetOutgoingMovementDirection2DAtFrame( int32_t frameIdx ) const
    {
        int32_t const numFrames = (int32_t) m_transforms.size();
        EE_ASSERT( frameIdx >= 0 && frameIdx < numFrames );

        Vector result;

        if ( frameIdx == ( numFrames - 1 ) )
        {
            result = m_transforms[frameIdx].GetRotation().RotateVector( Vector::WorldForward );
        }
        else
        {
            Vector const movementDelta = m_transforms[frameIdx + 1].GetTranslation() - m_transforms[frameIdx].GetTranslation();
            if ( movementDelta.IsNearZero2() )
            {
                result = m_transforms[frameIdx].GetRotation().RotateVector( Vector::WorldForward );
            }
            else
            {
                result = movementDelta.GetNormalized2();
            }
        }

        return result;
    }

    Quaternion RootMotionData::GetOutgoingMovementOrientation2DAtFrame( int32_t frameIdx ) const
    {
        int32_t const numFrames = (int32_t) m_transforms.size();
        EE_ASSERT( frameIdx >= 0 && frameIdx < numFrames );

        Quaternion result;

        if ( frameIdx == ( numFrames - 1 ) )
        {
            result = m_transforms[frameIdx].GetRotation();
        }
        else
        {
            Vector const movementDelta = m_transforms[frameIdx + 1].GetTranslation() - m_transforms[frameIdx].GetTranslation();
            if ( movementDelta.IsNearZero2() )
            {
                result = m_transforms[frameIdx].GetRotation();
            }
            else
            {
                result = Quaternion::FromRotationBetweenNormalizedVectors( Vector::WorldForward, movementDelta.GetNormalized2() );
            }
        }

        return result;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void RootMotionData::DrawDebug( Drawing::DrawContext& ctx, Transform const& worldTransform, Color color0, Color color1 ) const
    {
        constexpr static float const axisSize = 0.025f;
        constexpr static float const axisThickness = 4.0f;

        if ( m_transforms.empty() )
        {
            return;
        }

        auto previousWorldRootMotionTransform = m_transforms[0] * worldTransform;
        ctx.DrawAxis( previousWorldRootMotionTransform, axisSize, axisThickness );

        auto const numTransforms = m_transforms.size();
        for ( auto i = 1; i < numTransforms; i++ )
        {
            auto const worldRootMotionTransform = m_transforms[i] * worldTransform;
            ctx.DrawLine( previousWorldRootMotionTransform.GetTranslation(), worldRootMotionTransform.GetTranslation(), ( i % 2 == 0 ) ? color0 : color1, 1.5f );
            ctx.DrawAxis( worldRootMotionTransform, axisSize, axisThickness );
            previousWorldRootMotionTransform = worldRootMotionTransform;
        }
    }
    #endif
}