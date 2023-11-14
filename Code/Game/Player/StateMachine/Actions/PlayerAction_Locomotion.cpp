#include "PlayerAction_Locomotion.h"
#include "Game/Player/Components/Component_MainPlayer.h"
#include "Game/Player/Camera/PlayerCameraController.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Base/Drawing/DebugDrawingSystem.h"
#include "Base/Input/InputSystem.h"
#include "Base/Math/MathUtils.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    constexpr static float      g_maxSprintSpeed = 7.5f;                      // meters/second
    constexpr static float      g_maxRunSpeed = 5.0f;                         // meters/second
    constexpr static float      g_maxCrouchSpeed = 3.0f;                      // meters/second
    constexpr static float      g_sprintStickAmplitude = 0.8f;                // [0,1]

    constexpr static float      g_idle_immediateStartThresholdAngle = Math::DegreesToRadians * 45.0f;
    constexpr static float      g_idle_minimumStickAmplitudeThreshold = 0.2f;
    constexpr static float      g_turnOnSpot_turnTime = 0.2f;
    constexpr static float      g_moving_detectStopTimer = 0.1f;
    constexpr static float      g_moving_detectTurnTimer = 0.2f;
    constexpr static float      g_stop_stopTime = 0.15f;
    constexpr static float      g_unnavigableSurfaceSlideThreshold = 0.35f;
    constexpr static float      g_turnOnSpotInterruptionTime = 0.2f;

    //-------------------------------------------------------------------------

    static float ConvertStickAmplitudeToSpeed( ActionContext const& ctx, float stickAmplitude )
    {
        float const baseSpeed = ctx.m_pPlayerComponent->m_sprintFlag ? g_maxSprintSpeed : ctx.m_pPlayerComponent->m_crouchFlag ? g_maxCrouchSpeed : g_maxRunSpeed;
        return stickAmplitude * baseSpeed;
    }

    //-------------------------------------------------------------------------

    bool LocomotionAction::TryStartInternal( ActionContext const& ctx )
    {
        Transform const characterWorldTransform = ctx.m_pCharacterComponent->GetWorldTransform();

        ctx.m_pAnimationController->SetCharacterState( AnimationController::CharacterState::Locomotion );
        ctx.m_pCharacterComponent->SetGravityMode( Physics::ControllerGravityMode::Acceleration );
        ctx.m_pCharacterComponent->TryMaintainVerticalMomentum();
        SetCrouchState( ctx, ctx.m_pPlayerComponent->m_crouchFlag );

        // Set initial state
        Vector const& characterVelocity = ctx.m_pCharacterComponent->GetCharacterVelocity();
        float const horizontalSpeed = characterVelocity.GetLength2();
        if ( horizontalSpeed > 0.1f )
        {
            RequestMoving( ctx, characterVelocity.Get2D() );
        }
        else
        {
            RequestIdle( ctx );
        }

        return true;
    }

    Action::Status LocomotionAction::UpdateInternal( ActionContext const& ctx )
    {
        auto const pControllerState = ctx.m_pInputState->GetControllerState();
        EE_ASSERT( pControllerState != nullptr );

        // Process inputs
        //-------------------------------------------------------------------------

        Vector const movementInputs = pControllerState->GetLeftAnalogStickValue();
        float const stickAmplitude = movementInputs.GetLength2();

        Vector const& camFwd = ctx.m_pCameraController->GetCameraRelativeForwardVector2D();
        Vector const& camRight = ctx.m_pCameraController->GetCameraRelativeRightVector2D();

        // Use last frame camera orientation
        Vector const stickDesiredForward = camFwd * movementInputs.GetY();
        Vector const stickDesiredRight = camRight * movementInputs.GetX();
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

            case LocomotionState::TurningOnSpot:
            {
                UpdateTurnOnSpot( ctx, stickInputVectorWS, stickAmplitude );
            }
            break;

            case LocomotionState::Starting:
            {
                UpdateStarting( ctx, stickInputVectorWS, stickAmplitude );
            }
            break;

            case LocomotionState::Moving:
            {
                UpdateMoving( ctx, stickInputVectorWS, stickAmplitude );
            }
            break;

            case LocomotionState::PlantingAndTurning:
            {
                UpdateMoving( ctx, stickInputVectorWS, stickAmplitude );
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

        /*if ( ctx.m_pCharacterController->GetFloorType() != CharacterPhysicsController::FloorType::Navigable && ctx.m_pCharacterComponent->GetCharacterVelocity().GetZ() < -Math::Epsilon )
        {
            m_unnavigableSurfaceSlideTimer.Update( ctx.GetDeltaTime() );
            if ( m_unnavigableSurfaceSlideTimer.GetElapsedTimeSeconds() > g_unnavigableSurfaceSlideThreshold )
            {
                isSliding = true;
                m_desiredFacing = ctx.m_pCharacterComponent->GetCharacterVelocity().GetNormalized2();
            }
        }
        else
        {
            m_unnavigableSurfaceSlideTimer.Reset();
        }*/

        // Update animation controller
        //-------------------------------------------------------------------------

        ctx.m_pAnimationController->SetSliding( isSliding );
        ctx.m_pAnimationController->SetCrouch( ctx.m_pPlayerComponent->m_crouchFlag );

        // Debug drawing
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( m_areVisualizationsEnabled )
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
    }

    void LocomotionAction::SetCrouchState( ActionContext const& ctx, bool isCrouchEnabled )
    {
        // Enable crouch
        if ( isCrouchEnabled )
        {
            ctx.m_pCharacterComponent->ResizeCapsule( ctx.m_pCharacterComponent->GetCapsuleRadius(), 0.2f );
            ctx.m_pPlayerComponent->m_crouchFlag = true;
        }
        else // Disable crouch
        {
            ctx.m_pCharacterComponent->ResetCapsuleSize();
            ctx.m_pPlayerComponent->m_crouchFlag = false;
        }
    }

    //-------------------------------------------------------------------------

    void LocomotionAction::RequestIdle( ActionContext const& ctx )
    {
        Transform const characterWorldTransform = ctx.m_pCharacterComponent->GetWorldTransform();

        m_desiredMovementVelocity = Vector::Zero;
        m_cachedFacing = Vector::Zero;
        m_desiredTurnDirection = Vector::Zero;
        m_desiredFacing = characterWorldTransform.GetForwardVector();

        ctx.m_pAnimationController->RequestIdle();

        m_state = LocomotionState::Idle;
    }

    void LocomotionAction::UpdateIdle( ActionContext const& ctx, Vector const& stickInputVectorWS, float stickAmplitude )
    {
        EE_ASSERT( m_state == LocomotionState::Idle );

        //-------------------------------------------------------------------------

        auto const pControllerState = ctx.m_pInputState->GetControllerState();
        EE_ASSERT( pControllerState != nullptr );
        if ( pControllerState->WasPressed( Input::ControllerButton::ThumbstickRight ) )
        {
            SetCrouchState( ctx, !ctx.m_pPlayerComponent->m_crouchFlag );
        }

        //-------------------------------------------------------------------------

        Transform const characterWorldTransform = ctx.m_pCharacterComponent->GetWorldTransform();
        Vector const characterForward = characterWorldTransform.GetForwardVector();

        m_desiredMovementVelocity = Vector::Zero;

        // If the stick direction is in the same direction the character is facing, go directly to start
        if ( stickAmplitude < g_idle_minimumStickAmplitudeThreshold )
        {
            ctx.m_pAnimationController->RequestIdle();
        }
        else // Trigger a turn on spot
        {
            Radians const deltaAngle = Math::GetAngleBetweenVectors( characterForward, stickInputVectorWS );
            if ( deltaAngle < g_idle_immediateStartThresholdAngle )
            {
                RequestStart( ctx, stickInputVectorWS, stickAmplitude );
            }
            else // turn on spot
            {
                RequestTurnOnSpot( ctx, stickInputVectorWS );
            }
        }
    }

    //-------------------------------------------------------------------------

    void LocomotionAction::RequestTurnOnSpot( ActionContext const& ctx, Vector const& desiredFacingDirection )
    {
        m_desiredMovementVelocity = Vector::Zero;
        m_cachedFacing = ctx.m_pCharacterComponent->GetForwardVector();
        m_desiredFacing = m_desiredTurnDirection = desiredFacingDirection.GetNormalized2();
        m_cachedFacing = Vector::Zero;

        ctx.m_pAnimationController->RequestTurnOnSpot( m_desiredFacing );

        // Start the interruption timer
        m_startDetectionTimer.Start( g_turnOnSpotInterruptionTime );

        m_state = LocomotionState::TurningOnSpot;
    }

    void LocomotionAction::UpdateTurnOnSpot( ActionContext const& ctx, Vector const& stickInputVectorWS, float stickAmplitude )
    {
        EE_ASSERT( m_state == LocomotionState::TurningOnSpot );

        #if EE_DEVELOPMENT_TOOLS
        if ( m_areVisualizationsEnabled )
        {
            Vector const characterPosition = ctx.m_pCharacterComponent->GetWorldTransform().GetTranslation();
            auto drawingCtx = ctx.GetDrawingContext();
            drawingCtx.DrawArrow( characterPosition, characterPosition + m_desiredTurnDirection, Colors::Orange, 3.0f );
        }
        #endif

        //-------------------------------------------------------------------------

        // If we have stick input check if we should trigger a start
        if ( stickAmplitude > 0.1f )
        {
            // Restart timer if stopped
            if ( !m_startDetectionTimer.IsRunning() )
            {
                m_startDetectionTimer.Start( g_turnOnSpotInterruptionTime );
            }
            else // If the timer elapses, trigger a start
            {
                if ( m_startDetectionTimer.Update( ctx.GetDeltaTime() ) )
                {
                    RequestStart( ctx, stickInputVectorWS, stickAmplitude );
                    return;
                }
            }
        }
        else // Stop the timer if no input
        {
            m_startDetectionTimer.Stop();
        }

        //-------------------------------------------------------------------------

        auto pGraphController = ctx.m_pAnimationController;
        if ( ctx.m_pAnimationController->IsTurningOnSpot() && pGraphController->IsLocomotionTransitionAllowed() )
        {
            RequestIdle( ctx );
        }
    }

    //-------------------------------------------------------------------------

    void LocomotionAction::RequestStart( ActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude )
    {
        Transform const characterWorldTransform = ctx.m_pCharacterComponent->GetWorldTransform();

        m_desiredMovementVelocity = stickInputVector;
        m_cachedFacing = stickInputVector;
        m_desiredTurnDirection = Vector::Zero;
        m_desiredFacing = stickInputVector;

        float const speed = ConvertStickAmplitudeToSpeed( ctx, stickAmplitude );
        ctx.m_pAnimationController->RequestStart( stickInputVector * speed );

        m_state = LocomotionState::Starting;
    }

    void LocomotionAction::UpdateStarting( ActionContext const& ctx, Vector const& stickInputVector, float stickAmplitude )
    {
        EE_ASSERT( m_state == LocomotionState::Starting );

        if ( ctx.m_pAnimationController->IsStarting() && ctx.m_pAnimationController->IsLocomotionTransitionAllowed() )
        {
            float const speed = ConvertStickAmplitudeToSpeed( ctx, stickAmplitude );
            if ( speed > 0.0f )
            {
                RequestMoving( ctx, stickInputVector * speed );
            }
            else
            {
                RequestStop( ctx );
            }
        }

        #if EE_DEVELOPMENT_TOOLS
        if ( m_areVisualizationsEnabled )
        {
            auto drawingCtx = ctx.GetDrawingContext();
            Vector const characterPosition = ctx.m_pCharacterComponent->GetWorldTransform().GetTranslation();
            drawingCtx.DrawArrow( characterPosition, characterPosition + m_desiredTurnDirection, Colors::Yellow, 3.0f );
        }
        #endif
    }

    //-------------------------------------------------------------------------

    void LocomotionAction::RequestMoving( ActionContext const& ctx, Vector const& initialVelocity )
    {
        Transform const characterWorldTransform = ctx.m_pCharacterComponent->GetWorldTransform();

        m_desiredMovementVelocity = initialVelocity;
        m_cachedFacing = Vector::Zero;
        m_desiredTurnDirection = Vector::Zero;
        m_desiredFacing = m_desiredMovementVelocity.GetNormalized3();

        ctx.m_pAnimationController->RequestMove( ctx.GetDeltaTime(), m_desiredMovementVelocity, m_desiredFacing );

        m_state = LocomotionState::Moving;
    }

    void LocomotionAction::UpdateMoving( ActionContext const& ctx, Vector const& stickInputVectorWS, float stickAmplitude )
    {
        auto const pControllerState = ctx.m_pInputState->GetControllerState();
        EE_ASSERT( pControllerState != nullptr );

        Transform const characterWorldTransform = ctx.m_pCharacterComponent->GetWorldTransform();

        //-------------------------------------------------------------------------

        if ( Math::IsNearZero( stickAmplitude ) )
        {
            // Check for stop
            // Start a stop timer but also keep the previous frames desires
            if ( m_stopDetectionTimer.IsRunning() )
            {
                if ( m_stopDetectionTimer.Update( ctx.GetDeltaTime() ) )
                {
                    RequestStop( ctx );
                    return;
                }
            }
            else
            {
                m_stopDetectionTimer.Start( g_moving_detectStopTimer );
            }

            // Keep existing locomotion parameters when we have no input
            ctx.m_pAnimationController->RequestMove( ctx.GetDeltaTime(), m_desiredMovementVelocity, m_desiredFacing );
        }
        else
        {
            // Clear the stop timer
            m_stopDetectionTimer.Stop();

            // Handle Sprinting
            //-------------------------------------------------------------------------

            if ( ctx.m_pPlayerComponent->m_sprintFlag )
            {
                if ( pControllerState->WasPressed( Input::ControllerButton::ThumbstickLeft ) )
                {
                    ctx.m_pPlayerComponent->m_sprintFlag = false;
                }
            }
            else // Not Sprinting
            {
                float const characterSpeed = ctx.m_pCharacterComponent->GetCharacterVelocity().GetLength2();
                if ( characterSpeed > 1.0f && pControllerState->WasPressed( Input::ControllerButton::ThumbstickLeft ) )
                {
                    ctx.m_pPlayerComponent->m_sprintFlag = true;
                    SetCrouchState( ctx, false );
                }

                if ( !ctx.m_pPlayerComponent->m_sprintFlag && pControllerState->WasPressed( Input::ControllerButton::ThumbstickRight ) )
                {
                    SetCrouchState( ctx, !ctx.m_pPlayerComponent->m_crouchFlag );
                }
            }

            // Calculate desired movement and facing
            //-------------------------------------------------------------------------

            float const speed = ConvertStickAmplitudeToSpeed( ctx, stickAmplitude );
            float const maxAngularVelocity = Math::DegreesToRadians * ctx.m_pPlayerComponent->GetAngularVelocityLimit( speed );
            float const maxAngularDeltaThisFrame = maxAngularVelocity * ctx.GetDeltaTime();

            #if EE_DEVELOPMENT_TOOLS
            m_debug_desiredSpeed = speed;
            m_debug_moveAngularVelocityLimit = maxAngularVelocity * Math::RadiansToDegrees;
            #endif

            Vector const characterForward = characterWorldTransform.GetForwardVector();
            Radians const deltaAngle = Math::GetAngleBetweenVectors( characterForward, stickInputVectorWS );
            if ( Math::Abs( deltaAngle.ToFloat() ) > maxAngularDeltaThisFrame )
            {
                Radians rotationAngle = maxAngularDeltaThisFrame;
                if ( Math::IsVectorToTheRight2D( stickInputVectorWS, characterForward ) )
                {
                    rotationAngle = -rotationAngle;
                }

                Quaternion const rotation( AxisAngle( Vector::WorldUp, rotationAngle ) );
                m_desiredMovementVelocity = rotation.RotateVector( characterForward ) * speed;
            }
            else
            {
                m_desiredMovementVelocity = stickInputVectorWS * speed;
            }

            m_desiredFacing = m_desiredMovementVelocity.IsZero2() ? ctx.m_pCharacterComponent->GetForwardVector() : m_desiredMovementVelocity.GetNormalized2();

            ctx.m_pAnimationController->RequestMove( ctx.GetDeltaTime(), m_desiredMovementVelocity, m_desiredFacing );
        }
    }

    //-------------------------------------------------------------------------

    void LocomotionAction::RequestStop( ActionContext const& ctx )
    {
        Transform const characterWorldTransform = ctx.m_pCharacterComponent->GetWorldTransform();

        m_desiredMovementVelocity = Vector::Zero;
        m_cachedFacing = Vector::Zero;
        m_desiredTurnDirection = Vector::Zero;
        m_desiredFacing = characterWorldTransform.GetForwardVector();
        ctx.m_pPlayerComponent->m_sprintFlag = false;

        ctx.m_pAnimationController->RequestStop( characterWorldTransform );

        m_state = LocomotionState::Stopping;
    }

    void LocomotionAction::UpdateStopping( ActionContext const& ctx, Vector const& stickInputVectorWS, float stickAmplitude )
    {
        if ( ctx.m_pAnimationController->IsIdle() )
        {
            RequestIdle( ctx );
        }
        else if ( stickAmplitude > 0.1f && ctx.m_pAnimationController->IsLocomotionTransitionAllowed() )
        {
            // TODO: handle starting directly from here
            RequestIdle( ctx );
        }
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void LocomotionAction::DrawDebugUI()
    {
        ImGuiX::TextSeparator( "Options" );

        ImGui::Checkbox( "Enable 3D Visualization", &m_areVisualizationsEnabled );

        ImGuiX::TextSeparator( "State Info" );

        ImGui::Text( "Current State:" );
        ImGui::SameLine();

        switch ( m_state )
        {
            case LocomotionState::Idle:
            {
                ImGui::Text( "Idle" );
            }
            break;

            case LocomotionState::TurningOnSpot:
            {
                ImGui::TextColored( Colors::Yellow.ToFloat4(), "Turn On Spot" );

                if ( m_startDetectionTimer.IsRunning() )
                {
                    ImGui::TextColored( Colors::LimeGreen.ToFloat4(), "Has Input!" );
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( "Start Detection Timer: " );
                    ImGui::SameLine();
                    ImGui::ProgressBar( m_startDetectionTimer.GetPercentageThrough().ToFloat(), ImVec2( -1, 0 ) );
                }
                else
                {
                    ImGui::Text( "No Input!" );
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( "Start Detection Timer: " );
                    ImGui::SameLine();
                    ImGui::ProgressBar( 0.0f, ImVec2( -1, 0 ) );
                }
            }
            break;

            case LocomotionState::Starting:
            {
                ImGui::Text( "Start" );
            }
            break;

            case LocomotionState::Moving:
            {
                ImGui::TextColored( Colors::LimeGreen.ToFloat4(), "Move" );

                if ( m_stopDetectionTimer.IsRunning() )
                {
                    ImGui::Text( "No Input!" );
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( "Stop Detection Timer: " );
                    ImGui::SameLine();
                    ImGui::ProgressBar( m_stopDetectionTimer.GetPercentageThrough().ToFloat(), ImVec2( -1, 0 ) );
                }
                else
                {
                    ImGui::Text( "Has Input!" );
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( "Stop Detection Timer: " );
                    ImGui::SameLine();
                    ImGui::ProgressBar( 0.0f, ImVec2( -1, 0 ) );

                    ImGui::Text( "Desired Speed: %.2fm/s", m_debug_desiredSpeed );
                    ImGui::Text( "Angular Velocity Limit: %.2fdeg/s", m_debug_moveAngularVelocityLimit );
                }
            }
            break;

            case LocomotionState::PlantingAndTurning:
            {
                ImGui::TextColored( Colors::Orange.ToFloat4(), "Planted Turn" );
            }
            break;

            case LocomotionState::Stopping:
            {
                ImGui::TextColored( Colors::Red.ToFloat4(), "Stop" );
            }
            break;
        };
    }
    #endif
}