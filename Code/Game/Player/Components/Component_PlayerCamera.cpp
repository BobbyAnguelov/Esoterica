#include "Component_PlayerCamera.h"
#include "Engine/Camera/Components/Component_Camera.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Math/MathUtils.h"
#include "Base/Profiling.h"
#include "imgui.h"

//-------------------------------------------------------------------------

namespace EE
{
    void PlayerCameraComponent::Initialize()
    {
        CameraComponent::Initialize();
        ResetCamera();
    }

    void PlayerCameraComponent::Update( EntityWorldUpdateContext const& ctx, Float2 const& cameraInputs )
    {
        EE_PROFILE_FUNCTION_GAMEPLAY();

        // Update camera rotation
        //-------------------------------------------------------------------------

        Vector adjustedCameraInputs = cameraInputs;
        Radians const maxAngularVelocityForThisFrame = Math::Pi * ctx.GetDeltaTime();
        adjustedCameraInputs *= (float) maxAngularVelocityForThisFrame;
        SetOrbitAngle( adjustedCameraInputs.GetX(), adjustedCameraInputs.GetY() );

        //-------------------------------------------------------------------------

        m_cameraRelativeForwardVector = GetForwardVector();
        m_cameraRelativeRightVector = GetRightVector();
        m_cameraRelativeForwardVector2D = m_cameraRelativeForwardVector.GetNormalized2();
        m_cameraRelativeRightVector2D = m_cameraRelativeRightVector.GetNormalized2();
    }

    void PlayerCameraComponent::SetOrbitAngle( Radians deltaHorizontalAngle, Radians deltaVerticalAngle )
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

    void PlayerCameraComponent::SetOrbitTargetOffset( Float3 newTargetOffset )
    {
        m_orbitTargetOffset = newTargetOffset;
    }

    void PlayerCameraComponent::SetOrbitDistance( float newDistance )
    {
        m_orbitDistance = newDistance;
        Vector const newCameraPosition = m_orbitTransform.GetRotation().RotateVector( Vector::WorldBackward ) * m_orbitDistance;
        m_orbitTransform.SetTranslation( newCameraPosition );
    }

    void PlayerCameraComponent::ResetCamera()
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

    void PlayerCameraComponent::CalculateCameraTransform()
    {
        auto const rotZ = Quaternion( Vector::AxisYaw, m_yaw );
        auto const rotX = Quaternion( Vector::AxisPitch, m_pitch );
        auto const rotTotal = rotX * rotZ;

        Vector const orbitCameraPosition = rotTotal.RotateVector( Vector::WorldBackward ) * m_orbitDistance;
        m_orbitTransform = Transform( rotTotal, orbitCameraPosition );
    }

    void PlayerCameraComponent::FinalizeCamera()
    {
        EE_PROFILE_FUNCTION_GAMEPLAY();

        Transform const& parentTransform = GetSpatialParentWorldTransform();
        Transform camWorldTransform = m_orbitTransform;

        // Rotation offset is relative to camera to avoid pops when target rotates
        Vector const orbitTarget = parentTransform.GetTranslation() + camWorldTransform.RotateVector( m_orbitTargetOffset );

        camWorldTransform.AddTranslation( orbitTarget );
        SetWorldTransform( camWorldTransform );
    }

    Vector const& PlayerCameraComponent::GetCameraPosition() const
    {
        return GetPosition();
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void PlayerCameraComponent::DrawDebugUI()
    {
        ImGui::Text( "Orbit Offset: %s", Math::ToString( m_orbitTargetOffset ).c_str() );
        ImGui::Text( "Orbit Distance: %.2fm", m_orbitDistance );
    }
    #endif
}