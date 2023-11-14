#include "PlayerAction_Falling.h"
#include "Game/Player/Camera/PlayerCameraController.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Base/Input/InputSystem.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    static constexpr float const g_maxAirControlAcceleration = 10.0f;          // meters/second squared
    static constexpr float const g_maxAirControlSpeed = 6.5f;                  // meters/second
    static constexpr float const g_FallingEmptySpaceRequired = 0.5f;           // meters

    //-------------------------------------------------------------------------

    bool FallingAction::TryStartInternal( ActionContext const& ctx )
    {
        if( ctx.m_pCharacterComponent->GetInAirTime() > 0.2f )
        {
            Physics::QueryRules const& filter = ctx.m_pCharacterComponent->GetQueryRules();

            float const capsuleHalfHeight = ctx.m_pCharacterComponent->GetCapsuleHalfHeight();
            float const capsuleRadius = ctx.m_pCharacterComponent->GetCapsuleRadius();
            Quaternion const capsuleOrientation = ctx.m_pCharacterComponent->GetOrientation();
            Vector const capsulePosition = ctx.m_pCharacterComponent->GetPosition();

            // Test for environment collision right below the player
            ctx.m_pPhysicsWorld->AcquireReadLock();
            Physics::SweepResults sweepResults;
            bool collided = ctx.m_pPhysicsWorld->CapsuleSweep( capsuleHalfHeight, capsuleRadius, capsuleOrientation.GetNormalized(), capsulePosition, -Vector::UnitZ, g_FallingEmptySpaceRequired, filter, sweepResults );
            ctx.m_pPhysicsWorld->ReleaseReadLock();

            if( collided )
            {
                return false;
            }

            ctx.m_pAnimationController->SetCharacterState( AnimationController::CharacterState::InAir );
            ctx.m_pCharacterComponent->SetGravityMode( Physics::ControllerGravityMode::Acceleration );

            return true;
        }

        return false;
    }

    Action::Status FallingAction::UpdateInternal( ActionContext const& ctx )
    {
        // Calculate desired player displacement
        //-------------------------------------------------------------------------
        auto const pControllerState = ctx.m_pInputState->GetControllerState();
        EE_ASSERT( pControllerState != nullptr );
        Vector const movementInputs = pControllerState->GetLeftAnalogStickValue();

        auto const& camFwd = ctx.m_pCameraController->GetCameraRelativeForwardVector2D();
        auto const& camRight = ctx.m_pCameraController->GetCameraRelativeRightVector2D();

        // Use last frame camera orientation
        Vector const currentVelocity = ctx.m_pCharacterComponent->GetCharacterVelocity();
        Vector const currentVelocity2D = currentVelocity * Vector( 1.0f, 1.0f, 0.0f );

        Vector const forward = camFwd * movementInputs.GetY();
        Vector const right = camRight * movementInputs.GetX();
        Vector const desiredMovementVelocity2D = ( forward + right ) * g_maxAirControlAcceleration * ctx.GetDeltaTime();
        Vector const facing = desiredMovementVelocity2D.IsZero2() ? ctx.m_pCharacterComponent->GetForwardVector() : desiredMovementVelocity2D.GetNormalized2();

        Vector resultingVelocity = currentVelocity2D + desiredMovementVelocity2D;
        float const speed2D = resultingVelocity.GetLength2();
        if( speed2D > g_maxAirControlSpeed )
        {
            resultingVelocity = resultingVelocity.GetNormalized2() * g_maxAirControlSpeed;
        }

        // Run physic Prediction if required
        //-------------------------------------------------------------------------
        // nothing for now

        // Update animation controller
        //-------------------------------------------------------------------------

        ctx.m_pAnimationController->SetInAirDesiredMovement( ctx.GetDeltaTime(), resultingVelocity, facing );
        
        if( ctx.m_pCharacterComponent->HasFloor() )
        {
            return Status::Completed;
        }

        return Status::Interruptible;
    }

    void FallingAction::StopInternal( ActionContext const& ctx, StopReason reason )
    {

    }
}