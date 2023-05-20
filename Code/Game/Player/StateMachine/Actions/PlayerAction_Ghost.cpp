#include "PlayerAction_Ghost.h"
#include "Game/Player/Camera/PlayerCameraController.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Game/Player/Animation/PlayerGraphController_Ability.h"
#include "System/Input/InputSystem.h"

// hack for now
#include "Game/Player/Animation/PlayerGraphController_Locomotion.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Player
{
    bool GhostModeAction::TryStartInternal( ActionContext const& ctx )
    {
        bool isOnCooldown = false;
        if ( m_CooldownTimer.IsRunning() )
        {
            isOnCooldown = !m_CooldownTimer.Update( ctx.GetDeltaTime() );
        }

        bool isThumbstickLeftDown = ctx.m_pInputState->GetControllerState()->WasPressed( Input::ControllerButton::ThumbstickLeft ) || ctx.m_pInputState->GetControllerState()->IsHeldDown( Input::ControllerButton::ThumbstickLeft );
        bool isThumbstickRightDown = ctx.m_pInputState->GetControllerState()->WasPressed( Input::ControllerButton::ThumbstickRight ) || ctx.m_pInputState->GetControllerState()->IsHeldDown( Input::ControllerButton::ThumbstickRight );
        if ( !isOnCooldown && isThumbstickLeftDown && isThumbstickRightDown )
        {
            ctx.m_pCharacterComponent->EnableGhostMode( true );
            m_hackTimer.Reset();
            return true;
        }

        return false;
    }

    Action::Status GhostModeAction::UpdateInternal( ActionContext const& ctx )
    {
        auto const pControllerState = ctx.m_pInputState->GetControllerState();
        EE_ASSERT( pControllerState != nullptr );

        // Calculate desired player displacement
        //-------------------------------------------------------------------------
        auto const& camFwd = ctx.m_pCameraController->GetCameraRelativeForwardVector();
        auto const& camRight = ctx.m_pCameraController->GetCameraRelativeRightVector();
        Vector const movementInputs = pControllerState->GetLeftAnalogStickValue();
        Vector const forward = camFwd * movementInputs.GetSplatY();
        Vector const right = camRight * movementInputs.GetSplatX();
        Vector desiredVelocity = ( forward + right ) * speed;
        Quaternion const deltaOrientation = Quaternion::Identity;

        float const zDelta = ( pControllerState->GetRightTriggerValue() - pControllerState->GetLeftTriggerValue() ) * speed;
        desiredVelocity.SetZ( desiredVelocity.GetZ() + zDelta );

        if( ctx.m_pInputState->GetControllerState()->WasReleased( Input::ControllerButton::ShoulderRight ) )
        {
            speed += 2.5f;
            speed = Math::Min( 40.0f, speed );
        }
        else if( ctx.m_pInputState->GetControllerState()->WasReleased( Input::ControllerButton::ShoulderLeft ) )
        {
            speed -= 2.5f;
            speed = Math::Max( 2.5f, speed );
        }

        // Run physic Prediction if required
        //-------------------------------------------------------------------------
        // nothing for now
        
        // Update animation controller
        //-------------------------------------------------------------------------
        ctx.m_pAnimationController->SetCharacterState( CharacterAnimationState::DebugMode );
        auto pLocomotionGraphController = ctx.GetAnimSubGraphController<LocomotionGraphController>();
        pLocomotionGraphController->SetLocomotionDesires(ctx.GetDeltaTime(), desiredVelocity, camFwd.GetNormalized2() );
        
        bool isThumbstickLeftDown = ctx.m_pInputState->GetControllerState()->WasPressed( Input::ControllerButton::ThumbstickLeft ) || ctx.m_pInputState->GetControllerState()->IsHeldDown( Input::ControllerButton::ThumbstickLeft );
        bool isThumbstickRightDown = ctx.m_pInputState->GetControllerState()->WasPressed( Input::ControllerButton::ThumbstickRight ) || ctx.m_pInputState->GetControllerState()->IsHeldDown( Input::ControllerButton::ThumbstickRight );
        if( m_hackTimer.GetElapsedTimeSeconds() > 0.5f && isThumbstickLeftDown && isThumbstickRightDown )
        {
            return Status::Completed;
        }

        m_hackTimer.Update( ctx.GetDeltaTime() );

        return Status::Uninterruptible;
    }

    void GhostModeAction::StopInternal( ActionContext const& ctx, StopReason reason )
    {
        ctx.m_pCharacterComponent->EnableGhostMode( false );
        m_CooldownTimer.Start( 0.5f );
    }
}
#endif