#include "PlayerAction_Slide.h"
#include "Game/Player/Components/Component_PlayerCamera.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Base/Input/InputSystem.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool PlayerAction_Slide::TryStartInternal( PlayerActionContext const& ctx )
    {
        if( ctx.m_pPlayerState->m_sprintFlag && ctx.m_pInput->m_crouch.WasPressed() )
        {
            Vector const movementInputs = ctx.m_pInput->m_move.GetValue();
            Vector const& camFwd = ctx.m_pCamera->GetCameraRelativeForwardVector2D();
            Vector const& camRight = ctx.m_pCamera->GetCameraRelativeRightVector2D();
            Vector const forward = camFwd * movementInputs.GetY();
            Vector const right = camRight * movementInputs.GetX();
            m_slideDirection = ( forward + right ).GetNormalized2();

            ctx.m_pAnimationController->SetCharacterState( PlayerAnimationController::CharacterState::Ability );
            ctx.m_pAnimationController->StartSlide();

            #if EE_DEVELOPMENT_TOOLS
            m_debugStartPosition = ctx.m_pPlayer->GetPosition();
            #endif

            // Cancel sprint and enable crouch
            ctx.m_pPlayerState->m_sprintFlag = false;
            //ctx.m_pPlayerState->m_crouchFlag = true;

            return true;
        }

        //-------------------------------------------------------------------------

        return false;
    }

    PlayerAction::Status PlayerAction_Slide::UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate )
    {
        // Calculate desired player displacement
        //-------------------------------------------------------------------------

        Vector const desiredVelocity = m_slideDirection * 10.0f;
        Quaternion const deltaOrientation = Quaternion::Identity;

        // Run physic Prediction if required
        //-------------------------------------------------------------------------
        // nothing for now
        
        // Update animation controller
        //-------------------------------------------------------------------------

        auto pGraphController = ctx.m_pAnimationController;
        ctx.m_pAnimationController->SetAbilityDesiredMovement( ctx.GetDeltaTime(), desiredVelocity, ctx.m_pPlayer->GetForwardVector() );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        auto drawingCtx = ctx.m_pEntityWorldUpdateContext->GetDebugDrawContext();
        if( m_enableVisualizations )
        {
            Vector const Origin = ctx.m_pPlayer->GetPosition();
            drawingCtx.DrawArrow( m_debugStartPosition, m_debugStartPosition + m_slideDirection, Colors::HotPink );
        }
        #endif

        //-------------------------------------------------------------------------

        static StringID const slideTransitionMarkerID( "Slide" );
        if ( pGraphController->IsTransitionFullyAllowed( slideTransitionMarkerID ) )
        {
            return Status::Completed;
        }

        return pGraphController->IsTransitionConditionallyAllowed( slideTransitionMarkerID ) ? Status::Interruptible : Status::Uninterruptible;
    }

    void PlayerAction_Slide::StopInternal( PlayerActionContext const& ctx, StopReason reason )
    {

    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void PlayerAction_Slide::DrawDebugUI()
    {
        ImGui::Checkbox( "Enable Visualization", &m_enableVisualizations );
    }
    #endif
}