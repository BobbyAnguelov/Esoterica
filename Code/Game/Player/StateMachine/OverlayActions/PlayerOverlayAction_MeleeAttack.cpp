#include "PlayerOverlayAction_MeleeAttack.h"
#include "Game/Player/Components/Component_MainPlayer.h"
#include "Game/Player/Camera/PlayerCameraController.h"
#include "Game/Player/Animation/PlayerAnimationController.h"
#include "Engine/Physics/Components/Component_PhysicsCharacter.h"
#include "Base/Input/InputSystem.h"
#include "Base/Drawing/DebugDrawing.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    bool MeleeAttackAction::TryStartInternal( ActionContext const& ctx )
    {
        // Update cooldown timer
        if ( m_cooldownTimer.IsRunning() )
        {
            m_cooldownTimer.Update( ctx.GetDeltaTime() );
        }

        //-------------------------------------------------------------------------

        if( m_cooldownTimer.IsComplete() )
        {
            ctx.m_pAnimationController->TriggerHitReaction();
            return true;
        }

        //-------------------------------------------------------------------------

        return false;
    }

    Action::Status MeleeAttackAction::UpdateInternal( ActionContext const& ctx )
    {
        if ( ctx.m_pAnimationController->IsHitReactionComplete() )
        {
            return Action::Status::Completed;
        }

        return Action::Status::Uninterruptible;
    }

    void MeleeAttackAction::StopInternal( ActionContext const& ctx, StopReason reason )
    {
        ctx.m_pAnimationController->ClearHitReaction();
        m_cooldownTimer.Start( 1.5f );
    }
}