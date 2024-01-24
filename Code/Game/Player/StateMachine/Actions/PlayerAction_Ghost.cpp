#include "PlayerAction_Ghost.h"
#include "Game/Player/Camera/PlayerCameraController.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Base/Input/InputSystem.h"

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

        bool isThumbstickLeftDown = ctx.m_pInputSystem->GetController()->WasPressed( Input::InputID::Controller_ThumbstickLeft ) || ctx.m_pInputSystem->GetController()->IsHeldDown( Input::InputID::Controller_ThumbstickLeft );
        bool isThumbstickRightDown = ctx.m_pInputSystem->GetController()->WasPressed( Input::InputID::Controller_ThumbstickRight ) || ctx.m_pInputSystem->GetController()->IsHeldDown( Input::InputID::Controller_ThumbstickRight );
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
        auto const pControllerState = ctx.m_pInputSystem->GetController();
        EE_ASSERT( pControllerState != nullptr );

        // Calculate desired player displacement
        //-------------------------------------------------------------------------
        auto const& camFwd = ctx.m_pCameraController->GetCameraRelativeForwardVector();
        auto const& camRight = ctx.m_pCameraController->GetCameraRelativeRightVector();
        Vector const movementInputs = pControllerState->GetLeftStickValue();
        Vector const forward = camFwd * movementInputs.GetSplatY();
        Vector const right = camRight * movementInputs.GetSplatX();
        Vector desiredVelocity = ( forward + right ) * speed;
        Quaternion const deltaOrientation = Quaternion::Identity;

        float const zDelta = ( pControllerState->GetValue( Input::InputID::Controller_RightTrigger ) - pControllerState->GetValue( Input::InputID::Controller_LeftTrigger ) ) * speed;
        desiredVelocity.SetZ( desiredVelocity.GetZ() + zDelta );

        if( ctx.m_pInputSystem->GetController()->WasReleased( Input::InputID::Controller_ShoulderRight ) )
        {
            speed += 2.5f;
            speed = Math::Min( 40.0f, speed );
        }
        else if( ctx.m_pInputSystem->GetController()->WasReleased( Input::InputID::Controller_ShoulderLeft ) )
        {
            speed -= 2.5f;
            speed = Math::Max( 2.5f, speed );
        }

        // Run physic Prediction if required
        //-------------------------------------------------------------------------
        // nothing for now

        // Update animation controller
        //-------------------------------------------------------------------------

        ctx.m_pAnimationController->SetCharacterState( AnimationController::CharacterState::GhostMode );
        ctx.m_pAnimationController->SetAbilityDesiredMovement( ctx.GetDeltaTime(), desiredVelocity, camFwd.GetNormalized2() );
        
        bool isThumbstickLeftDown = ctx.m_pInputSystem->GetController()->WasPressed( Input::InputID::Controller_ThumbstickLeft ) || ctx.m_pInputSystem->GetController()->IsHeldDown( Input::InputID::Controller_ThumbstickLeft );
        bool isThumbstickRightDown = ctx.m_pInputSystem->GetController()->WasPressed( Input::InputID::Controller_ThumbstickRight ) || ctx.m_pInputSystem->GetController()->IsHeldDown( Input::InputID::Controller_ThumbstickRight );
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