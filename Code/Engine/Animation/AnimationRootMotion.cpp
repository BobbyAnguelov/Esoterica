#include "AnimationRootMotion.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void RootMotionData::Clear()
    {
        m_transforms.clear();
        m_numFrames = 0;
        m_averageLinearVelocity = 0.0f;
        m_averageAngularVelocity = 0.0f;
        m_totalDelta = Transform::Identity;
    }

    Transform RootMotionData::GetTransform( FrameTime const& frameTime ) const
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( frameTime.GetFrameIndex() < m_numFrames );
        EE_ASSERT( !m_transforms.empty() );

        Transform displacementTransform( NoInit );

        if ( m_transforms.size() == 1 )
        {
            displacementTransform = m_transforms[0];
        }
        else
        {
            if ( frameTime.IsExactlyAtKeyFrame() )
            {
                displacementTransform = m_transforms[frameTime.GetFrameIndex()];
            }
            else // Read interpolated transform
            {
                Transform const& frameStartTransform = m_transforms[frameTime.GetFrameIndex()];
                Transform const& frameEndTransform = m_transforms[frameTime.GetFrameIndex() + 1];
                displacementTransform = Transform::SLerp( frameStartTransform, frameEndTransform, frameTime.GetPercentageThrough() );
            }
        }

        return displacementTransform;
    }

    Transform RootMotionData::GetDelta( Percentage fromTime, Percentage toTime ) const
    {
        EE_ASSERT( !m_transforms.empty() );

        Transform delta( NoInit );

        if ( m_transforms.size() == 1 )
        {
            delta = Transform::Identity;
        }
        else
        {
            if ( fromTime <= toTime )
            {
                delta = GetDeltaNoLooping( fromTime, toTime );
            }
            else
            {
                Transform const preLoopDelta = GetDeltaNoLooping( fromTime, 1.0f );
                Transform const postLoopDelta = GetDeltaNoLooping( 0.0f, toTime );
                delta = postLoopDelta * preLoopDelta;
            }
        }

        return delta;
    }

    Transform RootMotionData::GetDeltaNoLooping( Percentage fromTime, Percentage toTime ) const
    {
        EE_ASSERT( !m_transforms.empty() );

        Transform delta( NoInit );

        if ( m_transforms.size() == 1 )
        {
            delta = Transform::Identity;
        }
        else
        {
            Transform const startTransform = GetTransform( fromTime );
            Transform const endTransform = GetTransform( toTime );
            delta = Transform::DeltaNoScale( startTransform, endTransform );
        }

        return delta;
    }

    Transform RootMotionData::SampleRootMotion( SamplingMode mode, Transform const& currentWorldTransform, Percentage startTime, Percentage endTime ) const
    {
        EE_ASSERT( !m_transforms.empty() );

        Transform delta( NoInit );

        if ( m_transforms.size() == 1 )
        {
            delta = Transform::Identity;
        }
        else
        {
            if ( mode == SamplingMode::WorldSpace )
            {
                Transform const desiredFinalTransform = GetTransform( endTime );
                delta = Transform::DeltaNoScale( currentWorldTransform, desiredFinalTransform );
            }
            else
            {
                delta = GetDelta( startTime, endTime );
            }
        }

        return delta;
    }

    Vector RootMotionData::GetIncomingMovementDirection2DAtFrame( int32_t frameIdx ) const
    {
        EE_ASSERT( frameIdx >= 0 && frameIdx < m_numFrames );

        Vector result;

        if ( m_transforms.empty() )
        {
            result = Vector::WorldForward;
        }
        else if ( m_transforms.size() == 1 )
        {
            result = m_transforms[0].GetRotation().RotateVector( Vector::WorldForward );
        }
        else
        {
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
        }

        return result;
    }

    Quaternion RootMotionData::GetIncomingMovementOrientation2DAtFrame( int32_t frameIdx ) const
    {
        EE_ASSERT( frameIdx >= 0 && frameIdx < m_numFrames );

        Quaternion result;

        if ( m_transforms.empty() )
        {
            result == Quaternion::Identity;
        }
        else if ( m_transforms.size() == 1 )
        {
            result = m_transforms[0].GetRotation();
        }
        else
        {
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
                    result = Quaternion::FromRotationBetweenUnitVectors( Vector::WorldForward, movementDelta.GetNormalized2() );
                }
            }
        }

        return result;
    }

    Vector RootMotionData::GetOutgoingMovementDirection2DAtFrame( int32_t frameIdx ) const
    {
        EE_ASSERT( frameIdx >= 0 && frameIdx < m_numFrames );

        Vector result;

        if ( m_transforms.empty() )
        {
            result = Vector::WorldForward;
        }
        else if ( m_transforms.size() == 1 )
        {
            result = m_transforms[0].GetRotation().RotateVector( Vector::WorldForward );
        }
        else
        {
            if ( frameIdx == ( m_numFrames - 1 ) )
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
        }

        return result;
    }

    Quaternion RootMotionData::GetOutgoingMovementOrientation2DAtFrame( int32_t frameIdx ) const
    {
        EE_ASSERT( frameIdx >= 0 && frameIdx < m_numFrames );

        Quaternion result;

        if ( m_transforms.empty() )
        {
            result = Quaternion::Identity;
        }
        else if ( m_transforms.size() == 1 )
        {
            result = m_transforms[0].GetRotation();
        }
        else
        {
            if ( frameIdx == ( m_numFrames - 1 ) )
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
                    result = Quaternion::FromRotationBetweenUnitVectors( Vector::WorldForward, movementDelta.GetNormalized2() );
                }
            }
        }

        return result;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void RootMotionData::DrawDebug( DebugDrawContext& ctx, Transform const& worldTransform, Color color0, Color color1 ) const
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