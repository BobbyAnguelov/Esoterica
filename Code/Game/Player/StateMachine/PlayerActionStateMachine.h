#pragma once

#include "Game/Player/StateMachine/PlayerAction.h"

//-------------------------------------------------------------------------

namespace EE
{
    class PlayerActionStateMachine
    {
        // Hack for Fake UI
        friend class PlayerHudDebugView;

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
            LogEntry( uint64_t frameID, char const* pActionName, LoggedStatus status, bool isBaseAction = true )
                : m_frameID( frameID )
                , m_pActionName( pActionName )
                , m_status( status )
                , m_isBaseAction( isBaseAction )
            {
                EE_ASSERT( status != LoggedStatus::None && pActionName != nullptr );
            }

            uint64_t        m_frameID;
            char const*     m_pActionName;
            LoggedStatus    m_status;
            bool            m_isBaseAction;
        };
        #endif

    public:

        // List of all the state machine states
        //-------------------------------------------------------------------------

        enum ActionID : int8_t
        {
            InvalidAction = -1,
            Locomotion = 0,
            Falling,
            Jump,
            Dash,
            Slide,
            Interact,

            #if EE_DEVELOPMENT_TOOLS
            DebugMode,
            #endif

            NumActions,
            DefaultAction = Locomotion,
        };

        enum OverlayActionID : int8_t
        {
            InvalidOverlayAction = -1,
            Weapon = 0,
            MeleeAttack,
            TimeDilation,

            NumOverlayActions,
        };

        // Transitions in the state machine
        //-------------------------------------------------------------------------

        struct Transition
        {
            enum class Availability : uint8_t
            {
                Always,
                OnlyOnCompleted
            };

            Transition( ActionID actionID, Availability availability = Availability::Always )
                : m_targetActionID( actionID )
                , m_availability( availability )
            {}

            inline bool IsAvailable( ActionID activeActionID, PlayerAction::Status actionStatus ) const
            {
                return m_targetActionID != activeActionID && ( ( m_availability == Availability::Always ) || ( actionStatus == PlayerAction::Status::Completed ) );
            }

            ActionID            m_targetActionID;
            Availability        m_availability;
        };

    public:

        PlayerActionStateMachine( PlayerActionContext const& context );
        ~PlayerActionStateMachine();

        void Update();
        void ForceStopAllRunningActions();

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebugUI() const;
        #endif

    private:

        PlayerActionContext const&                              m_actionContext;
        ActionID                                                m_activeBaseActionID = InvalidAction;

        TArray<PlayerAction*, NumActions>                       m_baseActions;
        TArray<TInlineVector<Transition, 6>, NumActions>        m_actionTransitions;
        TInlineVector<Transition, 6>                            m_highPriorityGlobalTransitions;
        TInlineVector<Transition, 6>                            m_lowPriorityGlobalTransitions;

        TArray<OverlayPlayerAction*, NumOverlayActions>         m_overlayActions;
        bool                                                    m_isFirstUpdate = true;

        #if EE_DEVELOPMENT_TOOLS
        TVector<LogEntry>                                       m_actionLog;
        #endif
    };
}