#pragma once

#include "Game/Player/StateMachine/PlayerAction.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE
{
    class PlayerOverlayAction_MeleeAttack final : public OverlayPlayerAction
    {
    public:

        EE_PLAYER_ACTION_ID( PlayerOverlayAction_MeleeAttack );

        virtual bool TryStartInternal( PlayerActionContext const& ctx ) override;
        virtual Status UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate ) override;
        virtual void StopInternal( PlayerActionContext const& ctx, StopReason reason ) override;

    private:

        ManualCountdownTimer        m_cooldownTimer;
    };
}