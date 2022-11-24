#include "PlayerActionStateMachine.h"
#include "System/Log.h"

// Actions
#include "Game/Player/StateMachine/Actions/PlayerAction_Locomotion.h"
#include "Game/Player/StateMachine/Actions/PlayerAction_Jump.h"
#include "Game/Player/StateMachine/Actions/PlayerAction_Falling.h"
#include "Game/Player/StateMachine/Actions/PlayerAction_Dash.h"
#include "Game/Player/StateMachine/OverlayActions/PlayerOverlayAction_Shoot.h"
#include "Actions/PlayerAction_Slide.h"
#include "Actions/PlayerAction_DebugMode.h"
#include "Actions/PlayerAction_Interact.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    ActionStateMachine::ActionStateMachine( ActionContext const& context )
        : m_actionContext( context )
    {
        //-------------------------------------------------------------------------
        // Overlay Actions
        //-------------------------------------------------------------------------

        m_overlayActions.emplace_back( EE::New<ShootOverlayAction>() );

        //-------------------------------------------------------------------------
        // Base Actions
        //-------------------------------------------------------------------------

        m_baseActions[Locomotion] = EE::New<LocomotionAction>();
        m_baseActions[Falling] = EE::New<FallingAction>();
        m_baseActions[Jump] = EE::New<JumpAction>();
        m_baseActions[Dash] = EE::New<DashAction>();
        m_baseActions[Slide] = EE::New<SlideAction>();
        m_baseActions[Interact] = EE::New<InteractAction>();

        #if EE_DEVELOPMENT_TOOLS
        m_baseActions[DebugMode] = EE::New<DebugModeAction>();
        #endif

        //-------------------------------------------------------------------------
        // State Transitions
        //-------------------------------------------------------------------------

        m_actionTransitions[Locomotion].emplace_back( Falling, Transition::Availability::Always );
        m_actionTransitions[Locomotion].emplace_back( Jump, Transition::Availability::Always );
        m_actionTransitions[Locomotion].emplace_back( Dash, Transition::Availability::Always );
        m_actionTransitions[Locomotion].emplace_back( Slide, Transition::Availability::Always );
        m_actionTransitions[Locomotion].emplace_back( Interact, Transition::Availability::Always );

        m_actionTransitions[Falling].emplace_back( Dash, Transition::Availability::Always );

        m_actionTransitions[Jump].emplace_back( Dash, Transition::Availability::Always );
        m_actionTransitions[Jump].emplace_back( Falling, Transition::Availability::OnlyOnCompleted );

        //-------------------------------------------------------------------------
        // Global Transitions
        //-------------------------------------------------------------------------

        m_lowPriorityGlobalTransitions.emplace_back( Falling, Transition::Availability::OnlyOnCompleted );
        m_lowPriorityGlobalTransitions.emplace_back( Locomotion, Transition::Availability::OnlyOnCompleted );

        #if EE_DEVELOPMENT_TOOLS
        m_highPriorityGlobalTransitions.emplace_back( DebugMode, Transition::Availability::Always );
        #endif
    }

    ActionStateMachine::~ActionStateMachine()
    {
        for ( auto& pAction : m_overlayActions )
        {
            EE::Delete( pAction );
        }
        m_overlayActions.clear();

        for ( auto& pAction : m_baseActions )
        {
            EE::Delete( pAction );
        }

        //-------------------------------------------------------------------------

        for ( auto i = 0; i < NumActions; i++ )
        {
            m_actionTransitions[i].clear();
        }

        m_highPriorityGlobalTransitions.clear();
        m_lowPriorityGlobalTransitions.clear();

        m_activeBaseActionID = InvalidAction;

        //-------------------------------------------------------------------------

        for ( auto pBaseAction : m_baseActions )
        {
            EE_ASSERT( pBaseAction == nullptr );
        }

        EE_ASSERT( m_overlayActions.empty() );
    }

    //-------------------------------------------------------------------------

    void ActionStateMachine::Update()
    {
        EE_ASSERT( m_actionContext.IsValid() );

        //-------------------------------------------------------------------------

        auto EvaluateTransitions = [this] ( Action::Status const activeStateStatus, TInlineVector<Transition, 6> const& transitions )
        {
            EE_ASSERT( activeStateStatus != Action::Status::Uninterruptible );

            for ( auto const& transition : transitions )
            {
                if ( !transition.IsAvailable( m_activeBaseActionID, activeStateStatus ) )
                {
                    continue;
                }

                if ( m_baseActions[transition.m_targetActionID]->TryStart( m_actionContext ) )
                {
                    #if EE_DEVELOPMENT_TOOLS
                    m_actionLog.emplace_back( m_actionContext.m_pEntityWorldUpdateContext->GetFrameID(), m_baseActions[m_activeBaseActionID]->GetName(), ( activeStateStatus == Action::Status::Completed ) ? LoggedStatus::ActionCompleted : LoggedStatus::ActionInterrupted );
                    m_actionLog.emplace_back( m_actionContext.m_pEntityWorldUpdateContext->GetFrameID(), m_baseActions[transition.m_targetActionID]->GetName(), LoggedStatus::ActionStarted );
                    #endif

                    // Stop the currently active state
                    Action::StopReason const stopReason = ( activeStateStatus == Action::Status::Completed ) ? Action::StopReason::Completed : Action::StopReason::Interrupted;
                    m_baseActions[m_activeBaseActionID]->Stop( m_actionContext, stopReason );
                    m_activeBaseActionID = InvalidAction;

                    //-------------------------------------------------------------------------

                    // Start the new state
                    m_activeBaseActionID = transition.m_targetActionID;
                    Action::Status const newActionStatus = m_baseActions[m_activeBaseActionID]->Update( m_actionContext );
                    EE_ASSERT( newActionStatus != Action::Status::Completed ); // Why did you instantly completed the action you just started, this is likely a mistake!

                    return true;
                }
            }

            return false;
        };

        //-------------------------------------------------------------------------
        // Evaluate active base action
        //-------------------------------------------------------------------------

        if ( m_activeBaseActionID != InvalidAction )
        {
            // Evaluate the current active state
            ActionID const initialStateID = m_activeBaseActionID;
            Action::Status const status = m_baseActions[m_activeBaseActionID]->Update( m_actionContext );
            if ( status != Action::Status::Uninterruptible )
            {
                // Evaluate high priority global transitions
                bool transitionFired = EvaluateTransitions( status, m_highPriorityGlobalTransitions );

                // Evaluate local transitions
                if ( !transitionFired )
                {
                    transitionFired = EvaluateTransitions( status, m_actionTransitions[initialStateID] );
                }

                // Evaluate low priority global transitions
                if ( !transitionFired )
                {
                    transitionFired = EvaluateTransitions( status, m_lowPriorityGlobalTransitions );
                }

                // Stop the current state if it completed and we didnt fire any transition
                if ( !transitionFired && status == Action::Status::Completed )
                {
                    m_baseActions[m_activeBaseActionID]->Stop( m_actionContext, Action::StopReason::Completed );
                    m_activeBaseActionID = InvalidAction;
                }
            }
        }

        // Handle the case where the state completed and we had no valid transition and end up in an invalid state
        if ( m_activeBaseActionID == InvalidAction )
        {
            if ( !m_isFirstUpdate )
            {
                EE_LOG_ERROR( "Player", "State Machine", "Ended up with no state, starting default state!" );
            }

            m_activeBaseActionID = DefaultAction;
            bool const tryStartResult = m_baseActions[m_activeBaseActionID]->TryStart( m_actionContext );
            EE_ASSERT( tryStartResult ); // The default state MUST always be able to start

            Action::Status const newActionStatus = m_baseActions[m_activeBaseActionID]->Update( m_actionContext );
            EE_ASSERT( newActionStatus == Action::Status::Interruptible ); // Why did you instantly completed the action you just started, this is likely a mistake!

            #if EE_DEVELOPMENT_TOOLS
            m_actionLog.emplace_back( m_actionContext.m_pEntityWorldUpdateContext->GetFrameID(), m_baseActions[m_activeBaseActionID]->GetName(), LoggedStatus::ActionStarted );
            #endif
        }

        //-------------------------------------------------------------------------
        // Evaluate overlay actions
        //-------------------------------------------------------------------------

        for ( auto pOverlayAction : m_overlayActions )
        {
            // Update running actions
            if ( pOverlayAction->IsActive() )
            {
                if ( pOverlayAction->Update( m_actionContext ) == Action::Status::Completed )
                {
                    pOverlayAction->Stop( m_actionContext, Action::StopReason::Completed );

                    #if EE_DEVELOPMENT_TOOLS
                    m_actionLog.emplace_back( m_actionContext.m_pEntityWorldUpdateContext->GetFrameID(), pOverlayAction->GetName(), LoggedStatus::ActionCompleted, false );
                    #endif
                }
            }
            // Try to start action
            else if ( pOverlayAction->TryStart( m_actionContext ) )
            {
                Action::Status const newActionStatus = pOverlayAction->Update( m_actionContext );
                EE_ASSERT( newActionStatus == Action::Status::Interruptible ); // Why did you instantly completed the action you just started, this is likely a mistake!

                #if EE_DEVELOPMENT_TOOLS
                m_actionLog.emplace_back( m_actionContext.m_pEntityWorldUpdateContext->GetFrameID(), pOverlayAction->GetName(), LoggedStatus::ActionStarted, false );
                #endif
            }
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( m_actionLog.size() > 500 )
        {
            m_actionLog.erase( m_actionLog.begin(), m_actionLog.end() - 100 );
        }
        #endif

        m_isFirstUpdate = false;
    }

    void ActionStateMachine::ForceStopAllRunningActions()
    {
        if ( m_activeBaseActionID != InvalidAction )
        {
            m_baseActions[m_activeBaseActionID]->Stop( m_actionContext, Action::StopReason::Interrupted );
            m_activeBaseActionID = InvalidAction;
        }

        for ( auto pOverlayAction : m_overlayActions )
        {
            if ( pOverlayAction->IsActive() )
            {
                pOverlayAction->Stop( m_actionContext, Action::StopReason::Interrupted );
            }
        }
    }
}