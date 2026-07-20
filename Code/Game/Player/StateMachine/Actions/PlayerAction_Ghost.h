#pragma once
#include "Game/Player/StateMachine/PlayerAction.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class PlayerAction_GhostMode final : public PlayerAction
    {
    public:

        EE_PLAYER_ACTION_ID( PlayerAction_GhostMode );

        virtual bool TryStartInternal( PlayerActionContext const& ctx ) override;
        virtual Status UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate ) override;
        virtual void StopInternal( PlayerActionContext const& ctx, StopReason reason ) override;

    private:

        ManualCountdownTimer m_CooldownTimer;
        float speed = 10.0f;
    };
}
#endif