#include "PlayerAction_Falling.h"
#include "Game/Player/Physics/PlayerPhysicsController.h"
#include "Game/Player/Camera/PlayerCameraController.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Physics/PhysicsScene.h"
#include "System/Input/InputSystem.h"

// hack for now
#include "Game/Player/Animation/PlayerGraphController_Locomotion.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    static float    g_maxAirControlAcceleration = 10.0f;          // meters/second squared
    static float    g_maxAirControlSpeed = 6.5f;                  // meters/second
    static float    g_FallingEmptySpaceRequired = 1.5f;           // meters

    //-------------------------------------------------------------------------

    bool FallingAction::TryStartInternal( ActionContext const& ctx )
    {
        if( ctx.m_pCharacterController->GetInAirTime() > 0.0f )
        {
            Physics::QueryFilter filter;
            filter.SetLayerMask( Physics::CreateLayerMask( Physics::Layers::Environment ) );
            filter.AddIgnoredEntity( ctx.m_pCharacterComponent->GetEntityID() );

            float const capsuleCylinderPortionHalfHeight = ctx.m_pCharacterComponent->GetCapsuleCylinderPortionHalfHeight();
            float const capsuleRadius = ctx.m_pCharacterComponent->GetCapsuleRadius();
            Quaternion const capsuleOrientation = ctx.m_pCharacterComponent->GetCapsuleOrientation();
            Vector const capsulePosition = ctx.m_pCharacterComponent->GetCapsulePosition();

            // Test for environment collision right below the player
            ctx.m_pPhysicsScene->AcquireReadLock();
            Physics::SweepResults sweepResults;
            bool collided = ctx.m_pPhysicsScene->CapsuleSweep( capsuleCylinderPortionHalfHeight, capsuleRadius, capsuleOrientation, capsulePosition, -Vector::UnitZ, g_FallingEmptySpaceRequired, filter, sweepResults );
            ctx.m_pPhysicsScene->ReleaseReadLock();

            if( collided )
            {
                return false;
            }

            ctx.m_pAnimationController->SetCharacterState( CharacterAnimationState::Falling );
            ctx.m_pCharacterController->EnableGravity( ctx.m_pCharacterComponent->GetCharacterVelocity().m_z );
            ctx.m_pCharacterController->DisableStepHeight();
            ctx.m_pCharacterController->DisableProjectionOntoFloor();
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

        Vector const forward = camFwd * movementInputs.m_y;
        Vector const right = camRight * movementInputs.m_x;
        Vector const desiredHeadingVelocity2D = ( forward + right ) * g_maxAirControlAcceleration * ctx.GetDeltaTime();
        Vector const facing = desiredHeadingVelocity2D.IsZero2() ? ctx.m_pCharacterComponent->GetForwardVector() : desiredHeadingVelocity2D.GetNormalized2();

        Vector resultingVelocity = currentVelocity2D + desiredHeadingVelocity2D;
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
        auto pLocomotionGraphController = ctx.GetAnimSubGraphController<LocomotionGraphController>();
        pLocomotionGraphController->SetLocomotionDesires( ctx.GetDeltaTime(), resultingVelocity, facing );
        
        if( ctx.m_pCharacterController->HasFloor() )
        {
            return Status::Completed;
        }

        return Status::Interruptible;
    }

    void FallingAction::StopInternal( ActionContext const& ctx, StopReason reason )
    {

    }
}