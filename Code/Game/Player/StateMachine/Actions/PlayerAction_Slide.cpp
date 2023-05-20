#include "PlayerAction_Slide.h"
#include "Game/Player/Components/Component_MainPlayer.h"
#include "Game/Player/Camera/PlayerCameraController.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Game/Player/Animation/PlayerGraphController_Ability.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "System/Input/InputSystem.h"
#include "System/Drawing/DebugDrawing.h"

// hack for now
#include "Game/Player/Animation/PlayerGraphController_Locomotion.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    bool SlideAction::TryStartInternal( ActionContext const& ctx )
    {
        if( ctx.m_pPlayerComponent->m_sprintFlag && ctx.m_pInputState->GetControllerState()->WasPressed( Input::ControllerButton::FaceButtonLeft ) )
        {
            auto const pControllerState = ctx.m_pInputState->GetControllerState();
            EE_ASSERT( pControllerState != nullptr );

            Vector const movementInputs = ctx.m_pInputState->GetControllerState()->GetLeftAnalogStickValue();
            Vector const& camFwd = ctx.m_pCameraController->GetCameraRelativeForwardVector2D();
            Vector const& camRight = ctx.m_pCameraController->GetCameraRelativeRightVector2D();
            Vector const forward = camFwd * movementInputs.GetY();
            Vector const right = camRight * movementInputs.GetX();
            m_slideDirection = ( forward + right ).GetNormalized2();

            ctx.m_pAnimationController->SetCharacterState( CharacterAnimationState::Ability );
            auto pAbilityAnimController = ctx.GetAnimSubGraphController<AbilityGraphController>();
            pAbilityAnimController->StartSlide();

            #if EE_DEVELOPMENT_TOOLS
            m_debugStartPosition = ctx.m_pCharacterComponent->GetPosition();
            #endif

            // Cancel sprint and enable crouch
            ctx.m_pPlayerComponent->m_sprintFlag = false;
            ctx.m_pPlayerComponent->m_crouchFlag = true;

            return true;
        }

        //-------------------------------------------------------------------------

        return false;
    }

    Action::Status SlideAction::UpdateInternal( ActionContext const& ctx )
    {
        auto const pControllerState = ctx.m_pInputState->GetControllerState();
        EE_ASSERT( pControllerState != nullptr );

        // Calculate desired player displacement
        //-------------------------------------------------------------------------

        Vector const desiredVelocity = m_slideDirection * 10.0f;
        Quaternion const deltaOrientation = Quaternion::Identity;

        // Run physic Prediction if required
        //-------------------------------------------------------------------------
        // nothing for now
        
        // Update animation controller
        //-------------------------------------------------------------------------

        auto pLocomotionGraphController = ctx.GetAnimSubGraphController<LocomotionGraphController>();
        pLocomotionGraphController->SetLocomotionDesires(ctx.GetDeltaTime(), desiredVelocity, ctx.m_pCharacterComponent->GetForwardVector() );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        auto drawingCtx = ctx.m_pEntityWorldUpdateContext->GetDrawingContext();
        if( m_enableVisualizations )
        {
            Vector const Origin = ctx.m_pCharacterComponent->GetPosition();
            drawingCtx.DrawArrow( m_debugStartPosition, m_debugStartPosition + m_slideDirection, Colors::HotPink );
        }
        #endif

        //-------------------------------------------------------------------------

        if ( pLocomotionGraphController->IsTransitionFullyAllowed() )
        {
            return Status::Completed;
        }

        return pLocomotionGraphController->IsTransitionConditionallyAllowed() ? Status::Interruptible : Status::Uninterruptible;
    }

    void SlideAction::StopInternal( ActionContext const& ctx, StopReason reason )
    {

    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void SlideAction::DrawDebugUI()
    {
        ImGui::Checkbox( "Enable Visualization", &m_enableVisualizations );
    }
    #endif
}