#include "PlayerAction_Jump.h"
#include "Game/Player/Components/Component_PlayerCamera.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Base/Input/InputSystem.h"
#include "Base/Math/Easing.h"

//-------------------------------------------------------------------------

namespace EE
{
    constinit static float const g_smallJumpDistance = 3.0f;                   // meters/second
    constinit static float const g_bigJumpDistance = 10.0f;                    // meters/second
    constinit static float const g_bigJumpEnergyCost = 1.0f;                   // energy levels
    constinit static float const g_maxAirControlAcceleration = 10.0f;          // meters/second squared
    constinit static float const g_maxAirControlSpeed = 6.5f;                  // meters/second
    constinit static float const g_bigJumpHoldTime = 0.3f;                     // seconds

    //-------------------------------------------------------------------------

    bool PlayerAction_Jump::TryStartInternal( PlayerActionContext const& ctx )
    {
        if( ctx.m_pInput->m_jump.WasReleased() )
        {
            ctx.m_pAnimationController->SetCharacterState( PlayerAnimationController::CharacterState::Ability );
            ctx.m_pAnimationController->StartJump();
            ctx.m_pPlayer->SetGravityScale( 0.0f );

            if( m_isChargedJumpReady )
            {
                ctx.m_pPlayerState->ConsumeEnergy( g_bigJumpEnergyCost );
                m_isPerformingChargedJump = true;
            }
            else
            {
                m_isPerformingChargedJump = false;
            }

            m_previousProgressThroughJump = 0.0f;
            m_jumpTimer.Start();

            return true;
        }
        else // Check hold state
        {
            m_isChargedJumpReady = false;
            if( ctx.m_pInput->m_jump.IsHeld() )
            {
                Seconds jumpHoldTime = ctx.m_pInput->m_jump.GetHoldTime();
                if( jumpHoldTime > g_bigJumpHoldTime && ctx.m_pPlayerState->HasRequiredEnergy( g_bigJumpEnergyCost ) )
                {
                    m_isChargedJumpReady = true;
                }
            }
        }

        return false;
    }

    PlayerAction::Status PlayerAction_Jump::UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate )
    {
        float g_bigJumpTimeToApex = Math::Sqrt( 2 * g_bigJumpDistance / 30 );
        float g_smallJumpTimeToApex = Math::Sqrt( 2 * g_smallJumpDistance / 30 );

        // check if we had a collision
        //-------------------------------------------------------------------------

        float const jumpTime = ( m_isPerformingChargedJump ? g_bigJumpTimeToApex : g_smallJumpTimeToApex );

        if( m_jumpTimer.GetElapsedTimeSeconds() >= jumpTime )
        {
            return Status::Completed;
        }
        else
        {
            m_jumpTimer.Update( ctx.GetDeltaTime() );

            float progress = ( m_jumpTimer.GetElapsedTimeSeconds() / jumpTime );
            float delta = Math::Easing::Evaluate( Math::Easing::Operation::InSine, progress ) - Math::Easing::Evaluate( Math::Easing::Operation::InSine, m_previousProgressThroughJump );

            float deltaHeight = Math::Max( 0.01f, delta * ( m_isPerformingChargedJump ? g_bigJumpDistance : g_smallJumpDistance ) );
            float verticalVelocity = deltaHeight / ctx.GetDeltaTime();
            m_previousProgressThroughJump = progress;

            // Calculate desired player displacement
            //-------------------------------------------------------------------------

            Vector const movementInputs = ctx.m_pInput->m_move.GetValue();
            auto const& camFwd = ctx.m_pCamera->GetCameraRelativeForwardVector2D();
            auto const& camRight = ctx.m_pCamera->GetCameraRelativeRightVector2D();

            // Use last frame camera orientation
            Vector const currentVelocity = ctx.m_pPlayer->GetVelocity();
            Vector const currentVelocity2D = currentVelocity * Vector( 1.0f, 1.0f, 0.0f );

            Vector const forward = camFwd * movementInputs.GetSplatY();
            Vector const right = camRight * movementInputs.GetSplatX();
            Vector const desiredMovementVelocity2D = ( forward + right ) * g_maxAirControlAcceleration * ctx.GetDeltaTime();

            Vector resultingVelocity = currentVelocity2D + desiredMovementVelocity2D;
            float const length = resultingVelocity.GetLength2();
            if( length > g_maxAirControlSpeed )
            {
                resultingVelocity = resultingVelocity.GetNormalized2() * g_maxAirControlSpeed;
            }
            resultingVelocity.SetZ( verticalVelocity );

            Vector const facing = desiredMovementVelocity2D.IsZero2() ? ctx.m_pPlayer->GetForwardVector() : desiredMovementVelocity2D.GetNormalized2();

            // Update animation controller
            //-------------------------------------------------------------------------

            ctx.m_pAnimationController->SetAbilityDesiredMovement( ctx.GetDeltaTime(), resultingVelocity, facing );
        }

        //-------------------------------------------------------------------------

        return Status::Uninterruptible;
    }

    void PlayerAction_Jump::StopInternal( PlayerActionContext const& ctx, StopReason reason )
    {
        ctx.m_pPlayer->SetGravityScale( 1.0f );
    }
}