#pragma once

#include "Game/Player/StateMachine/PlayerAction.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class FallingAction final : public Action
    {
    public:

        EE_PLAYER_ACTION_ID( FallingAction );

        virtual bool TryStartInternal( ActionContext const& ctx ) override;
        virtual Status UpdateInternal( ActionContext const& ctx, bool isFirstUpdate ) override;
        virtual void StopInternal( ActionContext const& ctx, StopReason reason ) override;
    };
}