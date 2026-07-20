#pragma once

#include "Game/Player/StateMachine/PlayerAction.h"
#include "Base/Time/Timers.h"
#include "Base/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE
{
    class PlayerAction_Slide final : public PlayerAction
    {
    public:

        EE_PLAYER_ACTION_ID( PlayerAction_Slide );

        virtual bool TryStartInternal( PlayerActionContext const& ctx ) override;
        virtual Status UpdateInternal( PlayerActionContext const& ctx, bool isFirstUpdate ) override;
        virtual void StopInternal( PlayerActionContext const& ctx, StopReason reason ) override;

    private:

        #if EE_DEVELOPMENT_TOOLS
        virtual void DrawDebugUI() override;
        #endif

    private:

        Vector                      m_slideDirection = Vector::Zero;

        #if EE_DEVELOPMENT_TOOLS
        Vector                      m_debugStartPosition = Vector::Zero;
        bool                        m_enableVisualizations = true;
        #endif
    };
}