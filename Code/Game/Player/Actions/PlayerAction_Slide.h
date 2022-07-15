#pragma once

#include "PlayerAction.h"
#include "System\Time\Timers.h"
#include "System\Math\Vector.h"

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

        ManualTimer                 m_stickPressedTimer;
        ManualTimer                 m_slideDurationTimer;
        ManualTimer                 m_hackSettleTimer;
        Vector                      m_slideDirection = Vector::Zero;
        bool                        m_isInSettle = false;

        #if EE_DEVELOPMENT_TOOLS
        Vector                      m_debugStartPosition = Vector::Zero;
        #endif
    };
}