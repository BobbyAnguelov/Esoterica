#include "Component_OrbitCamera.h"

//-------------------------------------------------------------------------

namespace EE
{
    void OrbitCameraComponent::Initialize()
    {
        CameraComponent::Initialize();
        ResetCamera();
    }

    void OrbitCameraComponent::FinalizeCameraPosition()
    {
        Transform const& parentTransform = GetSpatialParentWorldTransform();
        Transform camWorldTransform = m_orbitTransform;

        // Rotation offset is relative to camera to avoid pops when target rotates
        Vector const orbitTarget = parentTransform.GetTranslation() + camWorldTransform.RotateVector( m_orbitTargetOffset );

        camWorldTransform.AddTranslation( orbitTarget );
        SetWorldTransform( camWorldTransform );
    }

    void OrbitCameraComponent::AdjustOrbitAngle( Radians deltaHorizontalAngle, Radians deltaVerticalAngle )
    {
        if ( Math::IsNearZero( (float) deltaVerticalAngle ) && Math::IsNearZero( (float) deltaVerticalAngle ) )
        {
            return;
        }

        // Adjust yaw and pitch based on input
        m_yaw -= deltaHorizontalAngle;
        m_yaw.ClampPositive360();

        m_pitch += deltaVerticalAngle;
        m_pitch.Clamp( Degrees( -45 ), Degrees( 45 ) );

        CalculateCameraTransform();
    }

    void OrbitCameraComponent::AdjustOrbitTargetOffset( Float3 newTargetOffset )
    {
        m_orbitTargetOffset = newTargetOffset;
    }

    void OrbitCameraComponent::AdjustOrbitDistance( float newDistance )
    {
        m_orbitDistance = newDistance;
        Vector const newCameraPosition = m_orbitTransform.GetRotation().RotateVector( Vector::WorldBackward ) * m_orbitDistance;
        m_orbitTransform.SetTranslation( newCameraPosition );
    }

    void OrbitCameraComponent::ResetCamera()
    {
        m_orbitTargetOffset = m_defaultOrbitTargetOffset;
        m_orbitDistance = m_defaultOrbitDistance;

        // Align to parent orientation
        if ( HasSpatialParent() )
        {
            Transform const& parentWorldTransform = GetSpatialParentWorldTransform();
            m_yaw = parentWorldTransform.GetRotation().ToEulerAngles().GetYaw();
            m_pitch = 0.0f;

        }
        else // Just align to world forward
        {
            m_pitch = m_yaw = 0.0f;
        }

        CalculateCameraTransform();
    }

    void OrbitCameraComponent::CalculateCameraTransform()
    {
        auto const rotZ = Quaternion( Float3::WorldUp, m_yaw );
        auto const rotX = Quaternion( Float3::WorldRight, m_pitch );
        auto const rotTotal = rotX * rotZ;

        Vector const orbitCameraPosition = rotTotal.RotateVector( Vector::WorldBackward ) * m_orbitDistance;
        m_orbitTransform = Transform( rotTotal, orbitCameraPosition );
    }
}