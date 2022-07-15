#pragma once
#include "PlayerAction.h"
#include "System\Time\Timers.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Player
{
    class DebugModeAction final : public Action
    {
    public:

        EE_PLAYER_ACTION_ID( DebugModeAction );

        virtual bool TryStartInternal( ActionContext const& ctx ) override;
        virtual Status UpdateInternal( ActionContext const& ctx ) override;
        virtual void StopInternal( ActionContext const& ctx, StopReason reason ) override;

    private:

        ManualTimer m_HackTimer;
        ManualCountdownTimer m_CooldownTimer;
        float speed = 10.0f;
    };
}
#endif