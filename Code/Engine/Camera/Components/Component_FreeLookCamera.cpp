#include "Component_FreeLookCamera.h"
#include "System/Math/MathHelpers.h"

//-------------------------------------------------------------------------

namespace EE
{
    static Transform CalculateFreeLookCameraTransform( Vector const& position, Radians yaw, Radians pitch )
    {
        auto const rotZ = Quaternion( Float3::WorldUp, yaw );
        auto const rotX = Quaternion( Float3::WorldRight, pitch );
        auto const rotTotal = rotX * rotZ;
        return Transform( rotTotal, position );
    }

    void FreeLookCameraComponent::AdjustPitchAndYaw( Radians headingDelta, Radians pitchDelta )
    {
        // Adjust heading and pitch based on input
        m_yaw += headingDelta;
        m_yaw.ClampPositive360();

        m_pitch += pitchDelta;
        m_pitch.ClampPositive360();

        Transform const newCameraTransform = CalculateFreeLookCameraTransform( GetLocalTransform().GetTranslation(), m_yaw, m_pitch );
        SetLocalTransform( newCameraTransform );
    }

    void FreeLookCameraComponent::SetPositionAndLookatTarget( Vector const& cameraPosition, Vector const& lookatTarget )
    {
        if ( cameraPosition.IsNearEqual3( lookatTarget ) )
        {
            Transform worldTransform = GetWorldTransform();
            worldTransform.SetTranslation( cameraPosition );
            SetWorldTransform( worldTransform );
        }
        else // If we have a valid target adjust view direction
        {
            Transform const iwt = GetWorldTransform().GetInverse();
            Vector const localCameraPos = iwt.TransformPoint( cameraPosition );
            Vector const localCameraTarget = iwt.TransformPoint( lookatTarget );

            Vector const camForward = GetLocalTransform().GetForwardVector();
            Vector const camNewForward = ( localCameraTarget - localCameraPos ).GetNormalized3();

            Radians const horizontalDelta = Math::GetYawAngleBetweenVectors( camForward, camNewForward );
            Radians const verticalDelta = Math::GetPitchAngleBetweenNormalizedVectors( camForward, camNewForward );

            //-------------------------------------------------------------------------

            m_yaw += horizontalDelta;
            m_yaw.ClampPositive360();

            m_pitch += verticalDelta;
            m_pitch.ClampPositive360();

            Transform const newCameraTransform = CalculateFreeLookCameraTransform( localCameraPos, m_yaw, m_pitch );
            SetLocalTransform( newCameraTransform );
        }
    }
}