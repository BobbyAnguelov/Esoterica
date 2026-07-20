#include "PlayerAction_Ghost.h"
#include "Game/Player/Components/Component_PlayerCamera.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Base/Input/InputSystem.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    bool PlayerAction_GhostMode::TryStartInternal( PlayerActionContext const& ctx )
    {
        bool isOnCooldown = false;
        if ( m_CooldownTimer.IsRunning() )
        {
            isOnCooldown = !m_CooldownTimer.Update( ctx.GetDeltaTime() );
        }

        if ( !isOnCooldown && ctx.m_pInput->m_ghostMode.WasPressed() )
        {
            ctx.m_pPlayer->EnableGhostMode( true );
            return true;
        }

        return false;
    }

    PlayerAction::Status PlayerAction_GhostMode::UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate )
    {
        // Calculate desired player displacement
        //-------------------------------------------------------------------------
        
        auto const& camFwd = ctx.m_pCamera->GetCameraRelativeForwardVector();
        auto const& camRight = ctx.m_pCamera->GetCameraRelativeRightVector();
        Vector const movementInputs = ctx.m_pInput->m_move.GetValue();
        Vector const forward = camFwd * movementInputs.GetSplatY();
        Vector const right = camRight * movementInputs.GetSplatX();
        Vector desiredVelocity = ( forward + right ) * speed;
        Quaternion const deltaOrientation = Quaternion::Identity;

        // Z Movement
        //-------------------------------------------------------------------------

        float const zDelta = ( ctx.m_pInput->m_ghostModeMoveUp.GetValue() - ctx.m_pInput->m_ghostModeMoveDown.GetValue() ) * speed;
        desiredVelocity.SetZ( desiredVelocity.GetZ() + zDelta );

        // Speed adjustment
        //-------------------------------------------------------------------------

        if( ctx.m_pInput->m_ghostModeIncreaseSpeed.IsHeld() )
        {
            speed += 2.5f;
            speed = Math::Min( 40.0f, speed );
        }

        if( ctx.m_pInput->m_ghostModeDecreaseSpeed.IsHeld() )
        {
            speed -= 2.5f;
            speed = Math::Max( 2.5f, speed );
        }

        // Run physic Prediction if required
        //-------------------------------------------------------------------------
        // nothing for now

        // Update animation controller
        //-------------------------------------------------------------------------

        ctx.m_pAnimationController->SetCharacterState( PlayerAnimationController::CharacterState::GhostMode );
        ctx.m_pAnimationController->SetAbilityDesiredMovement( ctx.GetDeltaTime(), desiredVelocity, camFwd.GetNormalized2() );

        if( !isFirstUpdate && ctx.m_pInput->m_ghostMode.WasPressed() )
        {
            return Status::Completed;
        }

        return Status::Uninterruptible;
    }

    void PlayerAction_GhostMode::StopInternal( PlayerActionContext const& ctx, StopReason reason )
    {
        ctx.m_pPlayer->EnableGhostMode( false );
        m_CooldownTimer.Start( 0.5f );
    }
}
#endif