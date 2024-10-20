#pragma once
#include "Game/Player/StateMachine/PlayerAction.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class AimOverlayAction final : public OverlayAction
    {
    public:

        EE_PLAYER_ACTION_ID( AimOverlayAction );

        virtual bool TryStartInternal( ActionContext const& ctx ) override;
        virtual Status UpdateInternal( ActionContext const& ctx, bool isFirstUpdate ) override;
        virtual void StopInternal( ActionContext const& ctx, StopReason reason ) override;
    };
}