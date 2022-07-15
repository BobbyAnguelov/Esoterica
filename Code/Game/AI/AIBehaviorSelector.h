#pragma once

#include "Game/AI/Behaviors/AIBehavior.h"

//-------------------------------------------------------------------------

namespace EE::AI
{
    class BehaviorSelector
    {
        friend class AIDebugView;

        #if EE_DEVELOPMENT_TOOLS
        enum class LoggedStatus
        {
            None = 0,
            ActionStarted,
            ActionCompleted,
            ActionInterrupted,
        };

        struct LogEntry
        {
            LogEntry( uint64_t frameID, char const* pName, LoggedStatus status, bool isBaseAction = true )
                : m_frameID( frameID )
                , m_pBehaviorName( pName )
                , m_status( status )
                , m_isBaseAction( isBaseAction )
            {
                EE_ASSERT( status != LoggedStatus::None && pName != nullptr );
            }

            uint64_t          m_frameID;
            char const*     m_pBehaviorName;
            LoggedStatus    m_status;
            bool            m_isBaseAction;
        };
        #endif

    public:

        BehaviorSelector( BehaviorContext const& context );
        ~BehaviorSelector();

        // Update the currently active actions
        void Update();

    private:

        BehaviorContext const&                                    m_actionContext;
        Behavior*                                                 m_pActiveBehavior = nullptr;
        TInlineVector<Behavior*, 10>                              m_behaviors;

        #if EE_DEVELOPMENT_TOOLS
        TVector<LogEntry>                                         m_actionLog;
        #endif
    };
}