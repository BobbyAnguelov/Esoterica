#pragma once

#include "Game/Player/StateMachine/PlayerAction.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class MeleeAttackAction final : public OverlayAction
    {
    public:

        EE_PLAYER_ACTION_ID( MeleeAttackAction );

        virtual bool TryStartInternal( ActionContext const& ctx ) override;
        virtual Status UpdateInternal( ActionContext const& ctx ) override;
        virtual void StopInternal( ActionContext const& ctx, StopReason reason ) override;

    private:

        ManualCountdownTimer        m_cooldownTimer;
    };
}