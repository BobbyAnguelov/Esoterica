#include "Component_DebugCamera.h"

//-------------------------------------------------------------------------

namespace EE
{
    void DebugCameraComponent::AdjustMoveSpeed( float steps )
    {
        float const delta = ( s_moveSpeedStepSize * steps );
        SetMoveSpeed( m_moveSpeed + delta );
    }

    void DebugCameraComponent::OnTeleport()
    {
        m_directionChangeAccumulator = Vector::Zero;
    }

    void DebugCameraComponent::MoveCamera( Seconds deltaTime, float forward, float right, float up )
    {
        Vector const spatialInput( right, forward, up, 0 );

        Transform cameraTransform = GetWorldTransform();
        Vector const forwardDirection = cameraTransform.GetForwardVector();
        Vector const rightDirection = cameraTransform.GetRightVector();

        Vector const moveDelta = Vector( m_moveSpeed * deltaTime );
        Vector const deltaForward = spatialInput.GetSplatY() * moveDelta;
        Vector const deltaRight = spatialInput.GetSplatX() * moveDelta;

        // Calculate position delta
        Vector positionDelta = Vector::Zero;
        positionDelta = deltaRight * rightDirection;
        positionDelta += deltaForward * forwardDirection;
        positionDelta.m_w = 0.0f;

        // Update camera transform
        cameraTransform.AddTranslation( positionDelta );
        SetWorldTransform( cameraTransform );
    }

    void DebugCameraComponent::OrientCamera( Seconds deltaTime, Radians horizontalDelta, Radians verticalDelta )
    {
        // Update accumulator
        m_directionChangeAccumulator += Vector( horizontalDelta.ToFloat(), verticalDelta.ToFloat(), 0 );

        // Update view direction
        if ( !m_directionChangeAccumulator.IsNearZero2() )
        {
            float const maxAdjustmentPerFrame = s_maxAngularVelocity * deltaTime;

            Radians yawDelta = 0.0f;
            if ( m_directionChangeAccumulator.m_x < 0.0f )
            {
                yawDelta = Math::Max( -maxAdjustmentPerFrame, m_directionChangeAccumulator.m_x );
                m_directionChangeAccumulator.m_x = Math::Min( m_directionChangeAccumulator.m_x - horizontalDelta.ToFloat(), 0.0f );
            }
            else
            {
                yawDelta = Math::Min( maxAdjustmentPerFrame, m_directionChangeAccumulator.m_x );
                m_directionChangeAccumulator.m_x = Math::Max( m_directionChangeAccumulator.m_x - horizontalDelta.ToFloat(), 0.0f );
            }

            Radians pitchDelta = 0.0f;
            if ( m_directionChangeAccumulator.m_y < 0.0f )
            {
                verticalDelta = Math::Max( -maxAdjustmentPerFrame, m_directionChangeAccumulator.m_y );
                m_directionChangeAccumulator.m_y = Math::Min( m_directionChangeAccumulator.m_y - verticalDelta.ToFloat(), 0.0f );
            }
            else
            {
                verticalDelta = Math::Min( maxAdjustmentPerFrame, m_directionChangeAccumulator.m_y );
                m_directionChangeAccumulator.m_y = Math::Max( m_directionChangeAccumulator.m_y - verticalDelta.ToFloat(), 0.0f );
            }

            //-------------------------------------------------------------------------

            AdjustPitchAndYaw( horizontalDelta, verticalDelta );
        }
    }
}