#pragma once

#include "PlayerAction.h"
#include "System/Time/Timers.h"

//-------------------------------------------------------------------------
 
namespace EE::Player
{
    class JumpAction final : public Action
    {
    public:

        EE_PLAYER_ACTION_ID( JumpAction );

        virtual bool TryStartInternal( ActionContext const& ctx ) override;
        virtual Status UpdateInternal( ActionContext const& ctx ) override;
        virtual void StopInternal( ActionContext const& ctx, StopReason reason ) override;

        inline bool IsChargedJumpReady() const { return m_isChargedJumpReady; }

    private:

        ManualTimer             m_jumpTimer;
        bool                    m_isChargedJumpReady = false;

        float                   m_maxHeight = 6.0f;
        float                   m_previousHeight = 0.0f;
    };
}