#pragma once
#include "Game/Player/StateMachine/PlayerAction.h"

//-------------------------------------------------------------------------

namespace EE
{
    class PlayerOverlayAction_TimeDilation final : public OverlayPlayerAction
    {
    public:

        EE_PLAYER_ACTION_ID( PlayerOverlayAction_TimeDilation );

        virtual bool TryStartInternal( PlayerActionContext const& ctx ) override;
        virtual Status UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate ) override;
        virtual void StopInternal( PlayerActionContext const& ctx, StopReason reason ) override;
    };
}