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

    static void GetYawPitchDeltas( Vector const& dir, Radians& outYaw, Radians& outPitch )
    {
        EE_ASSERT( dir.IsNormalized3() );
        outYaw = Math::GetYawAngleBetweenVectors( Vector::WorldForward, dir );
        outPitch = Math::GetPitchAngleBetweenNormalizedVectors( Vector::WorldForward, dir );
    }

    //-------------------------------------------------------------------------

    void FreeLookCameraComponent::Initialize()
    {
        CameraComponent::Initialize();
        ResetView();
    }

    void FreeLookCameraComponent::OnWorldTransformUpdated()
    {
        CameraComponent::OnWorldTransformUpdated();

        // Ensure that the yaw/pitch are kept in sync
        Vector const currentForwardDir = GetForwardVector();
        GetYawPitchDeltas( currentForwardDir, m_yaw, m_pitch );
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

    //-------------------------------------------------------------------------

    void FreeLookCameraComponent::ResetView()
    {
        SetPositionAndLookAtTarget( Vector( 0, -5, 5 ), Vector( 0, 0, 0 ) );
        OnTeleport();
    }

    void FreeLookCameraComponent::FocusOn( OBB const& bounds )
    {
        // TODO: Fix this!
        Vector const currentForward = GetForwardVector();
        Vector const newCameraPos = bounds.m_center - ( currentForward * 5 );
        SetPositionAndLookAtDirection( newCameraPos, currentForward );

        OnTeleport();
    }

    void FreeLookCameraComponent::FocusOn( AABB const& bounds )
    {
        // TODO: Fix this!
        Vector const currentForward = GetForwardVector();
        Vector const newCameraPos = bounds.m_center - ( currentForward * 5 );
        SetPositionAndLookAtDirection( newCameraPos, currentForward );

        OnTeleport();
    }

    void FreeLookCameraComponent::SetPositionAndLookAtTarget( Vector const& cameraPosition, Vector const& lookAtTarget )
    {
        if ( cameraPosition.IsNearEqual3( lookAtTarget ) )
        {
            m_yaw = m_pitch = 0.0f;
        }
        else // If we have a valid target adjust view direction
        {
            Vector const lookAtDir = ( lookAtTarget - cameraPosition ).GetNormalized3();
            GetYawPitchDeltas( lookAtDir, m_yaw, m_pitch );
        }

        //-------------------------------------------------------------------------

        Transform const newCameraTransform = CalculateFreeLookCameraTransform( cameraPosition, m_yaw, m_pitch );
        SetLocalTransform( newCameraTransform );

        OnTeleport();
    }

    void FreeLookCameraComponent::SetPositionAndLookAtDirection( Vector const& cameraPosition, Vector const& lookAtDir )
    {
        EE_ASSERT( lookAtDir.IsNormalized3() );

        GetYawPitchDeltas( lookAtDir, m_yaw, m_pitch );
        Transform const newCameraTransform = CalculateFreeLookCameraTransform( cameraPosition, m_yaw, m_pitch );
        SetLocalTransform( newCameraTransform );

        //-------------------------------------------------------------------------

        OnTeleport();
    }
}