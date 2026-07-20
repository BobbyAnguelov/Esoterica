#include "PlayerAction_Falling.h"
#include "Game/Player/Components/Component_PlayerCamera.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Base/Input/InputSystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    static constexpr float const g_maxAirControlAcceleration = 10.0f;          // meters/second squared
    static constexpr float const g_maxAirControlSpeed = 6.5f;                  // meters/second
    static constexpr float const g_FallingEmptySpaceRequired = 0.5f;           // meters

    //-------------------------------------------------------------------------

    bool PlayerAction_Falling::TryStartInternal( PlayerActionContext const& ctx )
    {
        if( ctx.m_pPlayer->GetInAirTime() > 0.2f )
        {
            Physics::CastQuery query( ctx.m_pPlayer->GetQueryRules() );

            float const capsuleHalfHeight = ctx.m_pPlayer->GetCapsuleHalfHeight();
            float const capsuleRadius = ctx.m_pPlayer->GetCapsuleRadius();
            Quaternion const capsuleOrientation = ctx.m_pPlayer->GetOrientation();
            Vector const capsulePosition = ctx.m_pPlayer->GetPosition();

            // Test for environment collision right below the player
            ctx.m_pPhysicsWorld->LockRead();

            bool collided = ctx.m_pPhysicsWorld->CapsuleCast( capsuleHalfHeight, capsuleRadius, capsuleOrientation.GetNormalized(), capsulePosition, -Vector::UnitZ, g_FallingEmptySpaceRequired, query );
            ctx.m_pPhysicsWorld->UnlockRead();

            if( collided )
            {
                return false;
            }

            ctx.m_pAnimationController->SetCharacterState( PlayerAnimationController::CharacterState::InAir );
            ctx.m_pPlayer->ResetGravityScale();

            return true;
        }

        return false;
    }

    PlayerAction::Status PlayerAction_Falling::UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate )
    {
        // Calculate desired player displacement
        //-------------------------------------------------------------------------
    
        Vector const movementInputs = ctx.m_pInput->m_move.GetValue();

        auto const& camFwd = ctx.m_pCamera->GetCameraRelativeForwardVector2D();
        auto const& camRight = ctx.m_pCamera->GetCameraRelativeRightVector2D();

        // Use last frame camera orientation
        Vector const currentVelocity = ctx.m_pPlayer->GetVelocity();
        Vector const currentVelocity2D = currentVelocity * Vector( 1.0f, 1.0f, 0.0f );

        Vector const forward = camFwd * movementInputs.GetY();
        Vector const right = camRight * movementInputs.GetX();
        Vector const desiredMovementVelocity2D = ( forward + right ) * g_maxAirControlAcceleration * ctx.GetDeltaTime();
        Vector const facing = desiredMovementVelocity2D.IsZero2() ? ctx.m_pPlayer->GetForwardVector() : desiredMovementVelocity2D.GetNormalized2();

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
        
        if( ctx.m_pPlayer->HasFloor() )
        {
            return Status::Completed;
        }

        return Status::Interruptible;
    }

    void PlayerAction_Falling::StopInternal( PlayerActionContext const& ctx, StopReason reason )
    {

    }
}