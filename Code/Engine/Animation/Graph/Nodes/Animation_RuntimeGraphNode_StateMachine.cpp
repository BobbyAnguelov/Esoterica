#include "Animation_RuntimeGraphNode_StateMachine.h"
#include "Animation_RuntimeGraphNode_Transition.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void StateMachineNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<StateMachineNode>( context, options );

        for ( auto& stateDefinition : m_stateDefinition )
        {
            StateInfo& state = pNode->m_states.emplace_back();
            context.SetNodePtrFromIndex( stateDefinition.m_stateNodeIdx, state.m_pStateNode );
            context.SetOptionalNodePtrFromIndex( stateDefinition.m_entryConditionNodeIdx, state.m_pEntryConditionNode );

            for ( auto& transitionDefinition : stateDefinition.m_transitionDefinition )
            {
                TransitionInfo& transition = state.m_transitions.emplace_back();
                transition.m_targetStateIdx = transitionDefinition.m_targetStateIdx;
                transition.m_canBeForced = transitionDefinition.m_canBeForced;
                state.m_hasForceableTransitions |= transition.m_canBeForced;

                context.SetNodePtrFromIndex( transitionDefinition.m_transitionNodeIdx, transition.m_pTransitionNode );
                context.SetNodePtrFromIndex( transitionDefinition.m_conditionNodeIdx, transition.m_pConditionNode );
            }
        }
    }

    //-------------------------------------------------------------------------

    SyncTrack const& StateMachineNode::GetSyncTrack() const
    {
        EE_ASSERT( IsValid() );
        if ( m_pActiveTransition != nullptr )
        {
            return m_pActiveTransition->GetSyncTrack();
        }
        else
        {
            return m_states[m_activeStateIndex].m_pStateNode->GetSyncTrack();
        }
    }

    bool StateMachineNode::IsValid() const
    {
        return PoseNode::IsValid() && m_activeStateIndex >= 0 && m_activeStateIndex < m_states.size();
    }

    StateMachineNode::StateIndex StateMachineNode::SelectDefaultState( GraphContext& context ) const
    {
        EE_ASSERT( context.IsValid() );
        auto pDefinition = GetDefinition<StateMachineNode>();

        StateIndex selectedStateIndex = pDefinition->m_defaultStateIndex;
        auto const numStates = (int16_t) m_states.size();

        // NOTE: we need to initialize all conditions in advance to ensure that the value caching works
        for ( auto i = 0; i < numStates; i++ )
        {
            if ( m_states[i].m_pEntryConditionNode != nullptr )
            {
                m_states[i].m_pEntryConditionNode->Initialize( context );
            }
        }

        // Check all conditions
        for ( StateIndex i = 0; i < numStates; i++ )
        {
            if ( m_states[i].m_pEntryConditionNode != nullptr )
            {
                if ( m_states[i].m_pEntryConditionNode->GetValue<bool>( context ) )
                {
                    selectedStateIndex = i;
                    break;
                }
            }
        }

        // Shutdown all conditions
        for ( auto i = 0; i < numStates; i++ )
        {
            if ( m_states[i].m_pEntryConditionNode != nullptr )
            {
                m_states[i].m_pEntryConditionNode->Shutdown( context );
            }
        }

        return selectedStateIndex;
    }

    void StateMachineNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() && m_activeStateIndex == InvalidIndex && m_pActiveTransition == nullptr );

        PoseNode::InitializeInternal( context, initialTime );

        // Determine default state and initialize it
        m_activeStateIndex = SelectDefaultState( context );
        EE_ASSERT( m_activeStateIndex != InvalidIndex );
        StateNode* pActiveState = m_states[m_activeStateIndex].m_pStateNode;
        pActiveState->Initialize( context, initialTime );

        // Initialize the base animation graph node
        m_duration = pActiveState->GetDuration();
        m_previousTime = pActiveState->GetPreviousTime();
        m_currentTime = pActiveState->GetCurrentTime();

        InitializeTransitionConditions( context );
    }

    void StateMachineNode::InitializeTransitionConditions( GraphContext& context )
    {
        for ( auto const& transition : m_states[m_activeStateIndex].m_transitions )
        {
            if ( transition.m_pConditionNode != nullptr )
            {
                transition.m_pConditionNode->Initialize( context );
            }
        }
    }

    void StateMachineNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_activeStateIndex != InvalidIndex );

        if ( m_pActiveTransition != nullptr )
        {
            EE_ASSERT( m_pActiveTransition->IsInitialized() );
            m_pActiveTransition->Shutdown( context );
        }

        ShutdownTransitionConditions( context );

        m_states[m_activeStateIndex].m_pStateNode->Shutdown( context );
        m_activeStateIndex = InvalidIndex;
        m_pActiveTransition = nullptr;

        PoseNode::ShutdownInternal( context );
    }

    void StateMachineNode::ShutdownTransitionConditions( GraphContext& context )
    {
        for ( auto const& transition : m_states[m_activeStateIndex].m_transitions )
        {
            if ( transition.m_pConditionNode != nullptr )
            {
                transition.m_pConditionNode->Shutdown( context );
            }
        }
    }

    void StateMachineNode::EvaluateTransitions( GraphContext& context, SyncTrackTimeRange const* pUpdateRange, GraphPoseNodeResult& sourceNodeResult, int8_t sourceTasksStartMarker )
    {
        auto const& currentlyActiveStateInfo = m_states[m_activeStateIndex];

        //-------------------------------------------------------------------------
        // Check for a valid transition
        //-------------------------------------------------------------------------

        int32_t transitionIdx = InvalidIndex;
        int32_t const numTransitions = (int32_t) currentlyActiveStateInfo.m_transitions.size();
        for ( int32_t i = 0; i < numTransitions; i++ )
        {
            auto const& transition = currentlyActiveStateInfo.m_transitions[i];
            EE_ASSERT( transition.m_targetStateIdx != InvalidIndex );

            // Disallow any transitions to already transitioning states unless this is a forced transition, this will prevent infinite transition loops
            if ( !transition.m_canBeForced && m_states[transition.m_targetStateIdx].m_pStateNode->IsTransitioning() )
            {
                continue;
            }

            // Check if the conditions for this transition are satisfied, if they are start a new transition
            if ( transition.m_pConditionNode != nullptr && transition.m_pConditionNode->GetValue<bool>( context ) )
            {
                transitionIdx = i;
                break;
            }
        }

        //-------------------------------------------------------------------------
        // Start new transition
        //-------------------------------------------------------------------------

        if ( transitionIdx != InvalidIndex )
        {
            EE_ASSERT( transitionIdx >= 0 && transitionIdx < currentlyActiveStateInfo.m_transitions.size() );

            auto const& transition = currentlyActiveStateInfo.m_transitions[transitionIdx];

            // Handle forced transitions
            //-------------------------------------------------------------------------

            TInlineVector<StateNode const*, 20> forceableTargetStatesUsingCachedPoses;
            StateInfo const& targetStateInfo = m_states[transition.m_targetStateIdx];
            if ( targetStateInfo.m_hasForceableTransitions )
            {
                for ( auto const& transitionInfo : targetStateInfo.m_transitions )
                {
                    if ( transitionInfo.m_canBeForced )
                    {
                        forceableTargetStatesUsingCachedPoses.emplace_back( m_states[transitionInfo.m_targetStateIdx].m_pStateNode );
                    }
                }
            }

            // Check if we have forceable transition back to our current state, if so we need to immediately start caching the source pose
            bool const startCachingSourcePose = VectorContains( forceableTargetStatesUsingCachedPoses, currentlyActiveStateInfo.m_pStateNode );

            // Notify current transition of the new transition about to start
            if ( m_pActiveTransition != nullptr )
            {
                m_pActiveTransition->NotifyNewTransitionStarting( context, targetStateInfo.m_pStateNode, forceableTargetStatesUsingCachedPoses );
            }

            // Start the new transition
            //-------------------------------------------------------------------------
            // Note: the transition will initialize the target state

            transition.m_pTransitionNode->Initialize( context, SyncTrackTime() );

            // Initialize target state based on transition settings and what the source is (state or transition)
            TransitionNode::StartOptions startOptions( sourceNodeResult );
            startOptions.m_pUpdateRange = pUpdateRange;
            startOptions.m_isSourceATransition = ( m_pActiveTransition != nullptr );
            startOptions.m_pSourceNode = startOptions.m_isSourceATransition ? (PoseNode*) m_pActiveTransition : m_states[m_activeStateIndex].m_pStateNode;
            startOptions.m_sourceTasksStartMarker = sourceTasksStartMarker;
            startOptions.m_startCachingSourcePose = startCachingSourcePose;

            sourceNodeResult = transition.m_pTransitionNode->InitializeTargetStateAndUpdateTransition( context, startOptions );

            m_pActiveTransition = transition.m_pTransitionNode;

            // Update state data to that of the new active state
            ShutdownTransitionConditions( context );
            m_activeStateIndex = transition.m_targetStateIdx;
            InitializeTransitionConditions( context );

            m_duration = m_states[m_activeStateIndex].m_pStateNode->GetDuration();
            m_previousTime = m_states[m_activeStateIndex].m_pStateNode->GetPreviousTime();
            m_currentTime = m_states[m_activeStateIndex].m_pStateNode->GetCurrentTime();
        }
    }

    GraphPoseNodeResult StateMachineNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        EE_ASSERT( context.IsValid() );
        MarkNodeActive( context );

        #if EE_DEVELOPMENT_TOOLS
        int16_t const startSampledEventIdx = context.m_pSampledEventsBuffer->GetNumSampledEvents();
        #endif

        // Record the task system marker before we evaluate the source state, this is used to rollback the task system if needed
        int8_t const taskIndexMarker = context.m_pTaskSystem->GetCurrentTaskIndexMarker();

        // Check active transition
        if ( m_pActiveTransition != nullptr )
        {
            if ( m_pActiveTransition->IsComplete( context ) )
            {
                m_pActiveTransition->Shutdown( context );
                m_pActiveTransition = nullptr;
            }
        }

        // If we are fully in a state, update the state directly
        GraphPoseNodeResult result;
        if ( m_pActiveTransition == nullptr )
        {
            auto pActiveState = m_states[m_activeStateIndex].m_pStateNode;
            result = pActiveState->Update( context, pUpdateRange );

            // Update node time
            m_duration = pActiveState->GetDuration();
            m_previousTime = pActiveState->GetPreviousTime();
            m_currentTime = pActiveState->GetCurrentTime();
        }
        else // Update the transition
        {
            result = m_pActiveTransition->Update( context, pUpdateRange );

            // Update node time
            m_duration = m_pActiveTransition->GetDuration();
            m_previousTime = m_pActiveTransition->GetPreviousTime();
            m_currentTime = m_pActiveTransition->GetCurrentTime();
        }

        EE_ASSERT( context.m_pSampledEventsBuffer->IsValidRange( result.m_sampledEventRange ) );

        // Check for a valid transition, if one exists start it
        if ( context.m_branchState == BranchState::Active )
        {
            EvaluateTransitions( context, pUpdateRange, result, taskIndexMarker );
        }

        #if EE_DEVELOPMENT_TOOLS
        EE_ASSERT( result.m_sampledEventRange.m_startIdx == startSampledEventIdx );
        #endif

        return result;
    }

    #if EE_DEVELOPMENT_TOOLS
    void StateMachineNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_activeStateIndex );

        bool const hasActiveTransition = m_pActiveTransition != nullptr;
        outState.WriteValue( hasActiveTransition );

        if ( hasActiveTransition )
        {
            bool transitionSerialized = false;

            // NOTE: global transitions are shared across multiple states, you absolutely need to early out so that you dont write the transition indices multiple times
            for ( uint16_t stateIdx = 0; stateIdx < m_states.size(); stateIdx++ )
            {
                for ( uint16_t transitionIdx = 0; transitionIdx < m_states[stateIdx].m_transitions.size(); transitionIdx++ )
                {
                    if ( m_pActiveTransition == m_states[stateIdx].m_transitions[transitionIdx].m_pTransitionNode )
                    {
                        outState.WriteValue( stateIdx );
                        outState.WriteValue( transitionIdx );
                        transitionSerialized = true;
                        break;
                    }
                }

                if ( transitionSerialized )
                {
                    break;
                }
            }

            EE_ASSERT( transitionSerialized );
        }
    }

    void StateMachineNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );
        inState.ReadValue( m_activeStateIndex );

        bool hasActiveTransition = false;
        inState.ReadValue( hasActiveTransition );

        if ( hasActiveTransition )
        {
            uint16_t stateIdx;
            inState.ReadValue( stateIdx );
            uint16_t transitionIdx;
            inState.ReadValue( transitionIdx );
            m_pActiveTransition = m_states[stateIdx].m_transitions[transitionIdx].m_pTransitionNode;
        }
    }
    #endif
}