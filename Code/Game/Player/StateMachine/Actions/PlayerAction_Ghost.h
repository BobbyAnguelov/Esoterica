#pragma once
#include "Game/Player/StateMachine/PlayerAction.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Player
{
    class GhostModeAction final : public Action
    {
    public:

        EE_PLAYER_ACTION_ID( GhostModeAction );

        virtual bool TryStartInternal( ActionContext const& ctx ) override;
        virtual Status UpdateInternal( ActionContext const& ctx, bool isFirstUpdate ) override;
        virtual void StopInternal( ActionContext const& ctx, StopReason reason ) override;

    private:

        ManualCountdownTimer m_CooldownTimer;
        float speed = 10.0f;
    };
}
#endif