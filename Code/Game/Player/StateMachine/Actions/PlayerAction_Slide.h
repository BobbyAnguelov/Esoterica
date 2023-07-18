#pragma once

#include "Game/Player/StateMachine/PlayerAction.h"
#include "Base/Time/Timers.h"
#include "Base/Math/Vector.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class SlideAction final : public Action
    {
    public:

        EE_PLAYER_ACTION_ID( SlideAction );

        virtual bool TryStartInternal( ActionContext const& ctx ) override;
        virtual Status UpdateInternal( ActionContext const& ctx ) override;
        virtual void StopInternal( ActionContext const& ctx, StopReason reason ) override;

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