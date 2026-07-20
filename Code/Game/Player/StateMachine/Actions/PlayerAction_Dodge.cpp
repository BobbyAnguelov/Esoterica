#include "PlayerAction_Dodge.h"
#include "Game/Player/Components/Component_PlayerCamera.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Base/Input/InputSystem.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE
{
    float   g_dashDistance = 6.0f;                        // meters
    float   g_dashDuration = 0.20f;                       // seconds
    float   g_dashCooldown = 0.2f;                        // seconds
    float   g_dashEnergyCost = 1.0f;                      // energy levels

    #if EE_DEVELOPMENT_TOOLS
    bool    g_debugDashDistance = false;
    #endif

    //-------------------------------------------------------------------------

    bool PlayerAction_Dodge::TryStartInternal( PlayerActionContext const& ctx )
    {
        //// Update cooldown timer
        //if ( m_cooldownTimer.IsRunning() )
        //{
        //    m_cooldownTimer.Update( ctx.GetDeltaTime() );
        //}

        //bool const isDashAllowed = m_cooldownTimer.IsComplete() && ctx.m_pPlayerComponent->HasEnoughEnergy( g_dashEnergyCost );
        //if( isDashAllowed && ctx.m_pInput->m_dash.WasPressed() )
        //{
        //    ctx.m_pPlayerComponent->ConsumeEnergy( g_dashEnergyCost );
        //    ctx.m_pCharacterComponent->SetGravityMode( Physics::ControllerGravityMode::NoGravity );

        //    // Use last frame camera orientation
        //    Vector movementInputs = ctx.m_pInput->m_move.GetValue();

        //    if ( movementInputs.GetLength2() > 0 )
        //    {
        //        auto const& camFwd = ctx.m_pCamera->GetCameraRelativeForwardVector2D();
        //        auto const& camRight = ctx.m_pCamera->GetCameraRelativeRightVector2D();
        //        auto const forward = camFwd * movementInputs.GetY();
        //        auto const right = camRight * movementInputs.GetX();
        //        m_dashDirection = ( forward + right ).GetNormalized2();
        //    }
        //    else
        //    {
        //        m_dashDirection = ctx.m_pCharacterComponent->GetForwardVector();
        //    }
        //    m_initialVelocity = ctx.m_pCharacterComponent->GetVelocity() * Vector( 1.f, 1.f, 0.f );

        //    m_dashDurationTimer.Start();
        //    m_isInSettle = false;

        //    ctx.m_pAnimationController->SetCharacterState( AnimationController::CharacterState::Ability );
        //    ctx.m_pAnimationController->StartDash();

        //    #if EE_DEVELOPMENT_TOOLS
        //    m_debugStartPosition = ctx.m_pCharacterComponent->GetPosition();
        //    #endif

        //    return true;
        //}

        return false;
    }

    PlayerAction::Status PlayerAction_Dodge::UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate )
    {
        // Calculate desired player displacement
        //-------------------------------------------------------------------------
        Vector const desiredVelocity = m_dashDirection * ( g_dashDistance / g_dashDuration );
        Quaternion const deltaOrientation = Quaternion::Identity;

        // Run physic Prediction if required
        //-------------------------------------------------------------------------
        // nothing for now
        
        // Update animation controller
        //-------------------------------------------------------------------------

        ctx.m_pAnimationController->SetAbilityDesiredMovement(ctx.GetDeltaTime(), desiredVelocity, ctx.m_pPlayer->GetForwardVector() );

        if( m_dashDurationTimer.Update( ctx.GetDeltaTime() ) > g_dashDuration && !m_isInSettle )
        {
            m_hackSettleTimer.Start();
            m_isInSettle = true;
        }

        if( m_isInSettle )
        {
            ctx.m_pAnimationController->SetAbilityDesiredMovement( ctx.GetDeltaTime(), m_initialVelocity, ctx.m_pPlayer->GetForwardVector() );

            if( m_hackSettleTimer.Update( ctx.GetDeltaTime() ) > 0.1f )
            {
                return Status::Completed;
            }
        }

        #if EE_DEVELOPMENT_TOOLS
        DebugDrawContext drawingCtx = ctx.m_pEntityWorldUpdateContext->GetDebugDrawContext();
        if( g_debugDashDistance )
        {
            Vector const Origin = ctx.m_pPlayer->GetPosition();
            drawingCtx.DrawArrow( m_debugStartPosition, m_debugStartPosition + m_dashDirection * g_dashDistance, Colors::HotPink );
        }
        #endif

        return Status::Uninterruptible;
    }

    void PlayerAction_Dodge::StopInternal( PlayerActionContext const& ctx, StopReason reason )
    {
        m_cooldownTimer.Start( g_dashCooldown );
    }
}