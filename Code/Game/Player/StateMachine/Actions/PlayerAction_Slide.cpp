#include "PlayerAction_Slide.h"
#include "Game/Player/Components/Component_MainPlayer.h"
#include "Game/Player/Physics/PlayerPhysicsController.h"
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
    static float g_slideDistance = 5.0f;                        // meters
    static float g_slideDuration = 0.9f;                        // seconds
    static Seconds g_tapTimeToTriggerSlide = 0.5f;              // seconds

    #if EE_DEVELOPMENT_TOOLS
    bool    g_debugSlideDistance = false;
    #endif

    //-------------------------------------------------------------------------

    bool SlideAction::TryStartInternal( ActionContext const& ctx )
    {
        if( ctx.m_pInputState->GetControllerState()->WasPressed( Input::ControllerButton::ThumbstickLeft ) )
        {
            m_stickPressedTimer.Start();
        }

        m_stickPressedTimer.Update( ctx.GetDeltaTime() );

        bool stickWasTapped = false;
        if( ctx.m_pInputState->GetControllerState()->WasReleased( Input::ControllerButton::ThumbstickLeft ) )
        {
            if( m_stickPressedTimer.GetElapsedTimeSeconds() < g_tapTimeToTriggerSlide )
            {
                stickWasTapped = true;
            }
            m_stickPressedTimer.Reset();
        }

        Vector const movementInputs = ctx.m_pInputState->GetControllerState()->GetLeftAnalogStickValue();
        bool const isSlideAvailable = !movementInputs.IsNearZero2() && !ctx.m_pPlayerComponent->m_crouchFlag && stickWasTapped;
        if( isSlideAvailable )
        {
            auto const pControllerState = ctx.m_pInputState->GetControllerState();
            EE_ASSERT( pControllerState != nullptr );

            auto const& camFwd = ctx.m_pCameraController->GetCameraRelativeForwardVector2D();
            auto const& camRight = ctx.m_pCameraController->GetCameraRelativeRightVector2D();
            auto const forward = camFwd * movementInputs.m_y;
            auto const right = camRight * movementInputs.m_x;
            m_slideDirection = ( forward + right ).GetNormalized2();

            m_slideDurationTimer.Start();
            m_isInSettle = false;

            ctx.m_pAnimationController->SetCharacterState( CharacterAnimationState::Ability );
            auto pAbilityAnimController = ctx.GetAnimSubGraphController<AbilityGraphController>();
            pAbilityAnimController->StartSlide();

            #if EE_DEVELOPMENT_TOOLS
            m_debugStartPosition = ctx.m_pCharacterComponent->GetPosition();
            #endif

            ctx.m_pPlayerComponent->m_sprintFlag = false;
            ctx.m_pPlayerComponent->m_crouchFlag = true;

            return true;
        }

        return false;
    }

    Action::Status SlideAction::UpdateInternal( ActionContext const& ctx )
    {
        auto const pControllerState = ctx.m_pInputState->GetControllerState();
        EE_ASSERT( pControllerState != nullptr );

        // Calculate desired player displacement
        //-------------------------------------------------------------------------
        Vector const desiredVelocity = m_slideDirection * ( g_slideDistance / g_slideDuration );
        Quaternion const deltaOrientation = Quaternion::Identity;

        // Run physic Prediction if required
        //-------------------------------------------------------------------------
        // nothing for now
        
        // Update animation controller
        //-------------------------------------------------------------------------
        auto pLocomotionGraphController = ctx.GetAnimSubGraphController<LocomotionGraphController>();
        pLocomotionGraphController->SetLocomotionDesires(ctx.GetDeltaTime(), desiredVelocity, ctx.m_pCharacterComponent->GetForwardVector() );

        if( m_slideDurationTimer.Update( ctx.GetDeltaTime() ) > g_slideDuration && !m_isInSettle )
        {
            m_hackSettleTimer.Start();
            m_isInSettle = true;
        }

        if( m_isInSettle )
        {
            pLocomotionGraphController->SetLocomotionDesires( ctx.GetDeltaTime(), Vector::Zero, ctx.m_pCharacterComponent->GetForwardVector() );

            if( m_hackSettleTimer.Update( ctx.GetDeltaTime() ) > 0.1f )
            {
                return Status::Completed;
            }
        }

        #if EE_DEVELOPMENT_TOOLS
        auto drawingCtx = ctx.m_pEntityWorldUpdateContext->GetDrawingContext();
        if( g_debugSlideDistance )
        {
            Vector const Origin = ctx.m_pCharacterComponent->GetPosition();
            drawingCtx.DrawArrow( m_debugStartPosition, m_debugStartPosition + m_slideDirection * g_slideDistance, Colors::HotPink );
        }
        #endif

        return Status::Uninterruptible;
    }

    void SlideAction::StopInternal( ActionContext const& ctx, StopReason reason )
    {

    }
}