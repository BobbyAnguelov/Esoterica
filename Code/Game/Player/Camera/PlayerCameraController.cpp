#include "PlayerCameraController.h"
#include "Engine/Camera/Components/Component_OrbitCamera.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "System/Input/InputSystem.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    CameraController::CameraController( OrbitCameraComponent* pCamera )
        : m_pCamera( pCamera )
    {
        EE_ASSERT( pCamera != nullptr );
    }

    void CameraController::UpdateCamera( EntityWorldUpdateContext const& ctx )
    {
        auto pInputSystem = ctx.GetSystem<Input::InputSystem>();
        auto const pControllerState = pInputSystem->GetControllerState();

        // Update camera rotation
        //-------------------------------------------------------------------------

        Vector cameraInputs = pControllerState->GetRightAnalogStickValue();
        Radians const maxAngularVelocityForThisFrame = Math::Pi * ctx.GetDeltaTime();
        cameraInputs *= (float) maxAngularVelocityForThisFrame;
        m_pCamera->AdjustOrbitAngle( cameraInputs.m_x, cameraInputs.m_y );

        //-------------------------------------------------------------------------

        m_cameraRelativeForwardVector = m_pCamera->GetCameraRelativeForwardVector();
        m_cameraRelativeRightVector = m_pCamera->GetCameraRelativeRightVector();
        m_cameraRelativeForwardVector2D = m_cameraRelativeForwardVector.GetNormalized2();
        m_cameraRelativeRightVector2D = m_cameraRelativeRightVector.GetNormalized2();
    }

    void CameraController::FinalizeCamera()
    {
        m_pCamera->FinalizeCameraPosition();
    }

    Vector const& CameraController::GetCameraPosition() const
    {
        return m_pCamera->GetPosition();
    }
}