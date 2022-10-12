#include "PlayerAction_Locomotion.h"
#include "Game/Player/Components/Component_MainPlayer.h"
#include "Game/Player/Physics/PlayerPhysicsController.h"
#include "Game/Player/Camera/PlayerCameraController.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Game/Player/Animation/PlayerGraphController_Locomotion.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "System/Drawing/DebugDrawingSystem.h"
#include "System/Input/InputSystem.h"
#include "System/Math/MathHelpers.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    constexpr static float    g_maxSprintSpeed = 7.5f;                      // meters/second
    constexpr static float    g_maxRunSpeed = 5.0f;                         // meters/second
    constexpr static float    g_maxCrouchSpeed = 3.0f;                      // meters/second
    constexpr static float    g_timeToTriggerSprint = 1.5f;                 // seconds
    constexpr static float    g_timeToTriggerCrouch = 0.5f;                 // seconds
    constexpr static float    g_sprintStickAmplitude = 0.8f;                // [0,1]

    constexpr static float    g_idle_immediateStartThresholdAngle = Math::DegreesToRadians * 45.0f;
    constexpr static float    g_idle_minimumStickAmplitudeThreshold = 0.2f;
    constexpr static float    g_turnOnSpot_turnTime = 0.2f;
    constexpr static float    g_moving_detectStopTimer = 0.2f;
    constexpr static float    g_moving_detectTurnTimer = 0.2f;
    constexpr static float    g_stop_stopTime = 0.15f;

    #if EE_DEVELOPMENT_TOOLS
    constexpr static bool     g_debugDrawInputs = false;
    #endif

    //-------------------------------------------------------------------------

    bool LocomotionAction::TryStartInternal( ActionContext const& ctx )
    {
        Transform const characterWorldTransform = ctx.m_pCharacterComponent->GetWorldTransform();
        Vector const& characterVelocity = ctx.m_pCharacterComponent->GetCharacterVelocity();

        ctx.m_pAnimationController->SetCharacterState( CharacterAnimationState::Locomotion );
        ctx.m_pCharacterController->EnableGravity( characterVelocity.m_z );
        ctx.m_pCharacterController->EnableProjectionOntoFloor();
        ctx.m_pCharacterController->EnableStepHeight();

        if ( ctx.m_pCharacterComponent->GetCharacterVelocity().GetLength2() > g_maxRunSpeed )
        {
            ctx.m_pPlayerComponent->m_sprintFlag = true;
        }

        m_desiredHeading = characterVelocity.Get2D();
        m_desiredFacing = characterWorldTransform.GetForwardVector();
        m_cachedFacing = Vector::Zero;
        m_desiredTurnDirection = Vector::Zero;

        // Set initial state
        if ( characterVelocity.GetLength2() > 0.1f )
        {
            m_state = LocomotionState::Moving;
        }
        else
        {
            m_state = LocomotionState::Idle;
        }

        return true;
    }

    Action::Status LocomotionAction::UpdateInternal( ActionContext const& ctx )
    {
        auto const pControllerState = ctx.m_pInputState->GetControllerState();
        EE_ASSERT( pControllerState != nullptr );

        // Handle crouch
        //-------------------------------------------------------------------------

        if ( pControllerState->IsHeldDown( Input::ControllerButton::ThumbstickLeft ) )
        {
            if ( !m_crouchTimer.IsRunning() )
            {
                m_crouchTimer.Start( g_timeToTriggerCrouch );
            }
            if ( !m_crouchToggled && m_crouchTimer.Update( ctx.GetDeltaTime() ) )
            {
                ctx.m_pPlayerComponent->m_crouchFlag = !ctx.m_pPlayerComponent->m_crouchFlag;
                m_crouchToggled = true;
            }
        }
        else
        {
            m_crouchToggled = false;
            m_crouchTimer.Stop();
        }

        // Process inputs
        //-------------------------------------------------------------------------

        Vector const movementInputs = pControllerState->GetLeftAnalogStickValue();
        float const stickAmplitude = movementInputs.GetLength2();

        Vector const& camFwd = ctx.m_pCameraController->GetCameraRelativeForwardVector2D();
        Vector const& camRight = ctx.m_pCameraController->GetCameraRelativeRightVector2D();

        // Use last frame camera orientation
        Vector const stickDesiredForward = camFwd * movementInputs.m_y;
        Vector const stickDesiredRight = camRight * movementInputs.m_x;
        Vector const stickInputVectorWS = ( stickDesiredForward + stickDesiredRight );

        // Handle player state
        //-------------------------------------------------------------------------

        switch ( m_state )
        {
            case LocomotionState::Idle:
            {
                UpdateIdle( ctx, stickInputVectorWS, stickAmplitude );
            }
            break;

            case LocomotionState::TurnOnSpot:
            {
                UpdateTurnOnSpot( ctx, stickInputVectorWS, stickAmplitude );
            }
            break;

            case LocomotionState::Moving:
            {
                UpdateMoving( ctx, stickInputVectorWS, stickAmplitude );
            }
            break;

            case LocomotionState::PlantedTurn:
            {
                //UpdateMoving( ctx, stickInputVector, stickAmplitude );
            }
            break;

            case LocomotionState::Stopping:
            {
                UpdateStopping( ctx, stickInputVectorWS, stickAmplitude );
            }
            break;
        };

        // Handle unnavigable surfaces
        //-------------------------------------------------------------------------

        bool isSliding = false;

        if ( ctx.m_pCharacterController->GetFloorType() != CharacterPhysicsController::FloorType::Navigable && ctx.m_pCharacterComponent->GetCharacterVelocity().m_z < -Math::Epsilon )
        {
            m_slideTimer.Update( ctx.GetDeltaTime() );
            if ( m_slideTimer.GetElapsedTimeSeconds() > 0.35f )
            {
                isSliding = true;
                m_desiredFacing = ctx.m_pCharacterComponent->GetCharacterVelocity().GetNormalized2();
            }
        }
        else
        {
            m_slideTimer.Reset();
        }

        // Update animation controller
        //-------------------------------------------------------------------------

        auto pAnimController = ctx.GetAnimSubGraphController<LocomotionGraphController>();
        pAnimController->SetLocomotionDesires( ctx.GetDeltaTime(), m_desiredHeading, m_desiredFacing );
        pAnimController->SetSliding( isSliding );
        pAnimController->SetCrouch( ctx.m_pPlayerComponent->m_crouchFlag );

        // Debug drawing
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( g_debugDrawInputs )
        {
            Transform const characterWorldTransform = ctx.m_pCharacterComponent->GetWorldTransform();
            Vector const characterPosition = characterWorldTransform.GetTranslation();

            auto drawingCtx = ctx.GetDrawingContext();
            drawingCtx.DrawArrow( characterPosition, characterPosition + characterWorldTransform.GetForwardVector(), Colors::GreenYellow, 2.0f );
            drawingCtx.DrawArrow( characterPosition, characterPosition + stickInputVectorWS, Colors::White, 2.0f );
        }
        #endif

        return Status::Interruptible;
    }

    void LocomotionAction::StopInternal( ActionContext const& ctx, StopReason reason )
    {
        ctx.m_pPlayerComponent->m_sprintFlag = false;
        ctx.m_pPlayerComponent->m_crouchFlag = false;
        m_sprintTimer.Stop();
    }

    void LocomotionAction::UpdateIdle( ActionContext const& ctx, Vector const& stickInputVectorWS, float stickAmplitude )
    {
        Transform const characterWorldTransform = ctx.m_pCharacterComponent->GetWorldTransform();
        Vector const characterForward = characterWorldTransform.GetForwardVector();

        m_desiredHeading = Vector::Zero;

        if ( stickAmplitude < g_idle_minimumStickAmplitudeThreshold )
        {
            return;
        }

        //-------------------------------------------------------------------------

        Radians const deltaAngle = Math::GetAngleBetweenVectors( characterForward, stickInputVectorWS );
        if ( deltaAngle < g_idle_immediateStartThresholdAngle )
        {
            m_state = LocomotionState::Moving;
            UpdateMoving( ctx, stickInputVectorWS, stickAmplitude );
        }
        else // turn on spot
        {
            m_state = LocomotionState::TurnOnSpot;
            m_cachedFacing = ctx.m_pCharacterComponent->GetForwardVector();
            m_desiredTurnDirection = stickInputVectorWS.GetNormalized2();
            m_generalTimer.Start( g_turnOnSpot_turnTime );
            UpdateTurnOnSpot( ctx, stickInputVectorWS, stickAmplitude );
        }
    }

    void LocomotionAction::UpdateTurnOnSpot( ActionContext const& ctx, Vector const& stickInputVectorWS, float stickAmplitude )
    {
        EE_ASSERT( m_generalTimer.IsRunning() );

        m_desiredHeading = Vector::Zero;

        if ( m_generalTimer.Update( ctx.GetDeltaTime() ) )
        {
            m_state = LocomotionState::Idle;
        }
        else
        {
            m_desiredFacing = Vector::SLerp( m_cachedFacing, m_desiredTurnDirection, m_generalTimer.GetPercentageThrough() );

            #if EE_DEVELOPMENT_TOOLS
            Transform const characterWorldTransform = ctx.m_pCharacterComponent->GetWorldTransform();
            auto drawingCtx = ctx.GetDrawingContext();
            float remainingTime = m_generalTimer.GetRemainingTime().ToFloat();
            InlineString const str( InlineString::CtorSprintf(), "Turn On Spot Timer: %.2fs left", remainingTime );
            drawingCtx.DrawText3D( characterWorldTransform.GetTranslation() + Vector( 0, 0, 1.0f ), str.c_str(), Colors::White, Drawing::FontSmall );
            #endif
        }
    }

    void LocomotionAction::UpdateMoving( ActionContext const& ctx, Vector const& stickInputVectorWS, float stickAmplitude )
    {
        auto const pControllerState = ctx.m_pInputState->GetControllerState();
        EE_ASSERT( pControllerState != nullptr );

        Transform const characterWorldTransform = ctx.m_pCharacterComponent->GetWorldTransform();
        Vector const characterForward = characterWorldTransform.GetForwardVector();

        // Check for stop
        //-------------------------------------------------------------------------

        if ( Math::IsNearZero( stickAmplitude ) )
        {
            // Start a stop timer but also keep the previous frames desires
            if ( m_generalTimer.IsRunning() )
            {
                if ( m_generalTimer.Update( ctx.GetDeltaTime() ) )
                {
                    m_state = LocomotionState::Stopping;
                    m_generalTimer.Start( g_stop_stopTime );
                }
                else
                {
                    #if EE_DEVELOPMENT_TOOLS
                    auto drawingCtx = ctx.GetDrawingContext();
                    float remainingTime = m_generalTimer.GetRemainingTime().ToFloat();
                    InlineString const str( InlineString::CtorSprintf(), "Check for stop Timer: %.2fs left", remainingTime );
                    drawingCtx.DrawText3D( characterWorldTransform.GetTranslation() + Vector( 0, 0, 1.0f ), str.c_str(), Colors::White, Drawing::FontSmall );
                    #endif
                }
            }
            else
            {
                m_generalTimer.Start( g_moving_detectStopTimer );
            }

            // We're done with the move since we are checking if we should stop
            return;
        }
        else // Clear the stop timer
        {
            m_generalTimer.Stop();
        }

        // Check for sprint
        //-------------------------------------------------------------------------

        if ( !ctx.m_pPlayerComponent->m_crouchFlag )
        {
            if ( stickAmplitude > g_sprintStickAmplitude ) // TODO: check if we are colliding with something
            {
                if ( !ctx.m_pPlayerComponent->m_sprintFlag )
                {
                    if ( !m_sprintTimer.IsRunning() )
                    {
                        m_sprintTimer.Start( g_timeToTriggerSprint );
                    }

                    if ( m_sprintTimer.Update( ctx.GetDeltaTime() ) )
                    {
                        ctx.m_pPlayerComponent->m_sprintFlag = true;
                    }
                }
            }
            else
            {
                m_sprintTimer.Stop();
                ctx.m_pPlayerComponent->m_sprintFlag = false;
            }
        }

        // Calculate desired heading and facing
        //-------------------------------------------------------------------------

        float const speed = ctx.m_pPlayerComponent->m_sprintFlag ? g_maxSprintSpeed : ctx.m_pPlayerComponent->m_crouchFlag ? g_maxCrouchSpeed : g_maxRunSpeed;
        float const maxAngularVelocity = Math::DegreesToRadians * ctx.m_pPlayerComponent->GetAngularVelocityLimit( speed );
        float const maxAngularDeltaThisFrame = maxAngularVelocity * ctx.GetDeltaTime();

        Radians const deltaAngle = Math::GetAngleBetweenVectors( characterForward, stickInputVectorWS );
        if ( Math::Abs( deltaAngle.ToFloat() ) > maxAngularDeltaThisFrame )
        {
            Radians rotationAngle = maxAngularDeltaThisFrame;
            if ( Math::IsVectorToTheRight2D( stickInputVectorWS, characterForward ) )
            {
                rotationAngle = -rotationAngle;
            }

            Quaternion const rotation( AxisAngle( Vector::WorldUp, rotationAngle ) );
            m_desiredHeading = rotation.RotateVector( characterForward ) * speed;
        }
        else
        {
            m_desiredHeading = stickInputVectorWS * speed;
        }

        m_desiredFacing = m_desiredHeading.IsZero2() ? ctx.m_pCharacterComponent->GetForwardVector() : m_desiredHeading.GetNormalized2();
    }

    void LocomotionAction::UpdateStopping( ActionContext const& ctx, Vector const& stickInputVectorWS, float stickAmplitude )
    {
        EE_ASSERT( m_generalTimer.IsRunning() );

        m_desiredHeading = Vector::Zero;

        if ( m_generalTimer.Update( ctx.GetDeltaTime() ) )
        {
            m_state = LocomotionState::Idle;
        }
        else
        {
            #if EE_DEVELOPMENT_TOOLS
            Transform const characterWorldTransform = ctx.m_pCharacterComponent->GetWorldTransform();
            auto drawingCtx = ctx.GetDrawingContext();
            float remainingTime = m_generalTimer.GetRemainingTime().ToFloat();
            InlineString const str( InlineString::CtorSprintf(), "Stop Timer: %.2fs left", remainingTime );
            drawingCtx.DrawText3D( characterWorldTransform.GetTranslation() + Vector( 0, 0, 1.0f ), str.c_str(), Colors::White, Drawing::FontSmall );
            #endif
        }
    }
}