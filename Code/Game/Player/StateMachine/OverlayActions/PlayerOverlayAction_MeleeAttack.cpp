#include "PlayerOverlayAction_MeleeAttack.h"
#include "Game/Player/Animation/PlayerAnimationController.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool PlayerOverlayAction_MeleeAttack::TryStartInternal( PlayerActionContext const& ctx )
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

    PlayerAction::Status PlayerOverlayAction_MeleeAttack::UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate )
    {
        if ( ctx.m_pAnimationController->IsHitReactionComplete() )
        {
            return PlayerAction::Status::Completed;
        }

        return PlayerAction::Status::Uninterruptible;
    }

    void PlayerOverlayAction_MeleeAttack::StopInternal( PlayerActionContext const& ctx, StopReason reason )
    {
        ctx.m_pAnimationController->ClearHitReaction();
        m_cooldownTimer.Start( 1.5f );
    }
}