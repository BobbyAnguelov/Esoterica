#include "PlayerAction_Dash.h"
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
    float   g_dashDistance = 6.0f;                        // meters
    float   g_dashDuration = 0.20f;                       // seconds
    float   g_dashCooldown = 0.2f;                        // seconds
    float   g_dashEnergyCost = 1.0f;                      // energy levels

    #if EE_DEVELOPMENT_TOOLS
    bool    g_debugDashDistance = false;
    #endif

    //-------------------------------------------------------------------------

    bool DashAction::TryStartInternal( ActionContext const& ctx )
    {
        //// Update cooldown timer
        //if ( m_cooldownTimer.IsRunning() )
        //{
        //    m_cooldownTimer.Update( ctx.GetDeltaTime() );
        //}

        //bool const isDashAllowed = m_cooldownTimer.IsComplete() && ctx.m_pPlayerComponent->HasEnoughEnergy( g_dashEnergyCost );
        //if( isDashAllowed && ctx.m_pInputState->GetControllerState()->WasPressed( Input::ControllerButton::FaceButtonRight ) )
        //{
        //    ctx.m_pPlayerComponent->ConsumeEnergy( g_dashEnergyCost );
        //    ctx.m_pCharacterController->DisableGravity();

        //    auto const pControllerState = ctx.m_pInputState->GetControllerState();
        //    EE_ASSERT( pControllerState != nullptr );

        //    // Use last frame camera orientation
        //    Vector movementInputs = pControllerState->GetLeftAnalogStickValue();

        //    if ( movementInputs.GetLength2() > 0 )
        //    {
        //        auto const& camFwd = ctx.m_pCameraController->GetCameraRelativeForwardVector2D();
        //        auto const& camRight = ctx.m_pCameraController->GetCameraRelativeRightVector2D();
        //        auto const forward = camFwd * movementInputs.GetY();
        //        auto const right = camRight * movementInputs.GetX();
        //        m_dashDirection = ( forward + right ).GetNormalized2();
        //    }
        //    else
        //    {
        //        m_dashDirection = ctx.m_pCharacterComponent->GetForwardVector();
        //    }
        //    m_initialVelocity = ctx.m_pCharacterComponent->GetCharacterVelocity() * Vector( 1.f, 1.f, 0.f );

        //    m_dashDurationTimer.Start();
        //    m_isInSettle = false;

        //    ctx.m_pAnimationController->SetCharacterState( CharacterAnimationState::Ability );
        //    auto pAbilityAnimController = ctx.GetAnimSubGraphController<AbilityGraphController>();
        //    pAbilityAnimController->StartDash();

        //    #if EE_DEVELOPMENT_TOOLS
        //    m_debugStartPosition = ctx.m_pCharacterComponent->GetPosition();
        //    #endif

        //    return true;
        //}

        return false;
    }

    Action::Status DashAction::UpdateInternal( ActionContext const& ctx )
    {
        auto const pControllerState = ctx.m_pInputState->GetControllerState();
        EE_ASSERT( pControllerState != nullptr );

        // Calculate desired player displacement
        //-------------------------------------------------------------------------
        Vector const desiredVelocity = m_dashDirection * ( g_dashDistance / g_dashDuration );
        Quaternion const deltaOrientation = Quaternion::Identity;

        // Run physic Prediction if required
        //-------------------------------------------------------------------------
        // nothing for now
        
        // Update animation controller
        //-------------------------------------------------------------------------
        auto pLocomotionGraphController = ctx.GetAnimSubGraphController<LocomotionGraphController>();
        pLocomotionGraphController->SetLocomotionDesires(ctx.GetDeltaTime(), desiredVelocity, ctx.m_pCharacterComponent->GetForwardVector() );

        if( m_dashDurationTimer.Update( ctx.GetDeltaTime() ) > g_dashDuration && !m_isInSettle )
        {
            m_hackSettleTimer.Start();
            m_isInSettle = true;
        }

        if( m_isInSettle )
        {
            pLocomotionGraphController->SetLocomotionDesires( ctx.GetDeltaTime(), m_initialVelocity, ctx.m_pCharacterComponent->GetForwardVector() );

            if( m_hackSettleTimer.Update( ctx.GetDeltaTime() ) > 0.1f )
            {
                return Status::Completed;
            }
        }

        #if EE_DEVELOPMENT_TOOLS
        Drawing::DrawContext drawingCtx = ctx.m_pEntityWorldUpdateContext->GetDrawingContext();
        if( g_debugDashDistance )
        {
            Vector const Origin = ctx.m_pCharacterComponent->GetPosition();
            drawingCtx.DrawArrow( m_debugStartPosition, m_debugStartPosition + m_dashDirection * g_dashDistance, Colors::HotPink );
        }
        #endif

        return Status::Uninterruptible;
    }

    void DashAction::StopInternal( ActionContext const& ctx, StopReason reason )
    {
        m_cooldownTimer.Start( g_dashCooldown );
    }
}