#include "PlayerCameraController.h"
#include "Engine/Camera/Components/Component_OrbitCamera.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Input/InputSystem.h"
#include "Base/Profiling.h"

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
        EE_PROFILE_FUNCTION_GAMEPLAY();

        auto pInputSystem = ctx.GetSystem<Input::InputSystem>();
        auto const pControllerState = pInputSystem->GetController();

        // Update camera rotation
        //-------------------------------------------------------------------------

        Vector cameraInputs = pControllerState->GetRightStickValue();
        Radians const maxAngularVelocityForThisFrame = Math::Pi * ctx.GetDeltaTime();
        cameraInputs *= (float) maxAngularVelocityForThisFrame;
        m_pCamera->AdjustOrbitAngle( cameraInputs.GetX(), cameraInputs.GetY() );

        //-------------------------------------------------------------------------

        m_cameraRelativeForwardVector = m_pCamera->GetCameraRelativeForwardVector();
        m_cameraRelativeRightVector = m_pCamera->GetCameraRelativeRightVector();
        m_cameraRelativeForwardVector2D = m_cameraRelativeForwardVector.GetNormalized2();
        m_cameraRelativeRightVector2D = m_cameraRelativeRightVector.GetNormalized2();
    }

    void CameraController::FinalizeCamera()
    {
        EE_PROFILE_FUNCTION_GAMEPLAY();
        m_pCamera->FinalizeCameraPosition();
    }

    Vector const& CameraController::GetCameraPosition() const
    {
        return m_pCamera->GetPosition();
    }
}