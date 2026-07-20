#pragma once

#include "Game/Player/StateMachine/PlayerAction.h"
#include "Base/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE
{
    class PlayerAction_Falling final : public PlayerAction
    {
    public:

        EE_PLAYER_ACTION_ID( PlayerAction_Falling );

        virtual bool TryStartInternal( PlayerActionContext const& ctx ) override;
        virtual Status UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate ) override;
        virtual void StopInternal( PlayerActionContext const& ctx, StopReason reason ) override;
    };
}