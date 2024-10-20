#include "Animation_RuntimeGraphNode_State.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_LayerData.h"
#include "Base/Types/ScopedValue.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void StateNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<StateNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_childNodeIdx, pNode->m_pChildNode );
        context.SetOptionalNodePtrFromIndex( m_layerBoneMaskNodeIdx, pNode->m_pBoneMaskNode );
        context.SetOptionalNodePtrFromIndex( m_layerWeightNodeIdx, pNode->m_pLayerWeightNode );
        context.SetOptionalNodePtrFromIndex( m_layerRootMotionWeightNodeIdx, pNode->m_pLayerRootMotionWeightNode );
    }

    //-------------------------------------------------------------------------

    SyncTrack const& StateNode::GetSyncTrack() const
    {
        if ( IsValid() )
        {
            return m_pChildNode->GetSyncTrack();
        }
        else
        {
            return SyncTrack::s_defaultTrack;
        }
    }

    void StateNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );

        PoseNode::InitializeInternal( context, initialTime );
        m_transitionState = TransitionState::None;
        m_elapsedTimeInState = 0.0f;
        m_sampledEventRange = SampledEventRange();
        m_previousTime = m_currentTime = 0.0f;
        m_duration = 0.0f;

        if ( m_pChildNode != nullptr )
        {
            m_pChildNode->Initialize( context, initialTime );

            if ( m_pChildNode->IsValid() )
            {
                m_duration = m_pChildNode->GetDuration();
                m_previousTime = m_pChildNode->GetPreviousTime();
                m_currentTime = m_pChildNode->GetCurrentTime();
            }
        }

        if ( m_pBoneMaskNode != nullptr )
        {
            m_pBoneMaskNode->Initialize( context );
        }

        if ( m_pLayerWeightNode != nullptr )
        {
            m_pLayerWeightNode->Initialize( context );
        }

        // Flag this as the first update for this state, this will cause state entry events to be sampled for at least one update
        m_isFirstStateUpdate = true;
    }

    void StateNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        auto pStateDefinition = GetDefinition<StateNode>();

        //-------------------------------------------------------------------------

        if ( m_pBoneMaskNode != nullptr )
        {
            m_pBoneMaskNode->Shutdown( context );
        }

        if ( m_pLayerWeightNode != nullptr )
        {
            m_pLayerWeightNode->Shutdown( context );
        }

        if ( m_pChildNode != nullptr )
        {
            m_pChildNode->Shutdown( context );
        }

        m_transitionState = TransitionState::None;
        PoseNode::ShutdownInternal( context );
    }

    void StateNode::StartTransitionIn( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        m_transitionState = TransitionState::TransitioningIn;
    }

    SampledEventRange StateNode::StartTransitionOut( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        m_transitionState = TransitionState::TransitioningOut;

        // Since we update states before we register transitions, we need to ignore all previously sampled state events for this frame
        context.m_pSampledEventsBuffer->MarkOnlyStateEventsAsIgnored( m_sampledEventRange );

        // Since we update states before we register transitions, we need to mark all previous events as from the inactive branch
        context.m_pSampledEventsBuffer->MarkEventsAsFromInactiveBranch( m_sampledEventRange );

        // Resample state events
        SampleStateEvents( context );

        return m_sampledEventRange;
    }

    void StateNode::SampleStateEvents( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        auto pStateDefinition = GetDefinition<StateNode>();

        // Sample Fixed Stage Events
        //-------------------------------------------------------------------------

        bool isInActiveBranch = context.m_branchState == BranchState::Active;

        if ( m_isFirstStateUpdate || ( m_transitionState == TransitionState::TransitioningIn && isInActiveBranch ) )
        {
            for ( auto const& entryEventID : pStateDefinition->m_entryEvents )
            {
                context.m_pSampledEventsBuffer->EmplaceStateEvent( GetNodeIndex(), StateEventType::Entry, entryEventID, isInActiveBranch );
            }
        }
        else if ( m_transitionState == TransitionState::None && isInActiveBranch )
        {
            for ( auto const& executeEventID : pStateDefinition->m_executeEvents )
            {
                context.m_pSampledEventsBuffer->EmplaceStateEvent( GetNodeIndex(), StateEventType::FullyInState, executeEventID, isInActiveBranch );
            }
        }
        else if ( m_transitionState == TransitionState::TransitioningOut || !isInActiveBranch )
        {
            for ( auto const& exitEventID : pStateDefinition->m_exitEvents )
            {
                context.m_pSampledEventsBuffer->EmplaceStateEvent( GetNodeIndex(), StateEventType::Exit, exitEventID, isInActiveBranch );
            }
        }

        // Sample Timed Events
        //-------------------------------------------------------------------------

        for ( auto const& timedEvent : pStateDefinition->m_timedElapsedEvents )
        {
            if ( m_elapsedTimeInState >= timedEvent.m_timeValue )
            {
                context.m_pSampledEventsBuffer->EmplaceStateEvent( GetNodeIndex(), StateEventType::Timed, timedEvent.m_ID, isInActiveBranch );
            }
        }

        Seconds const currentTimeRemaining = ( 1.0f - m_currentTime ) * m_duration;
        for ( auto const& timedEvent : pStateDefinition->m_timedRemainingEvents )
        {
            if ( currentTimeRemaining <= timedEvent.m_timeValue )
            {
                context.m_pSampledEventsBuffer->EmplaceStateEvent( GetNodeIndex(), StateEventType::Timed, timedEvent.m_ID, isInActiveBranch );
            }
        }

        //-------------------------------------------------------------------------

        m_sampledEventRange.m_endIdx = context.m_pSampledEventsBuffer->GetNumSampledEvents();
    }

    void StateNode::UpdateLayerContext( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        // Early out if we are not in a layer
        if ( !context.IsInLayer() )
        {
            return;
        }

        // Update layer weights
        //-------------------------------------------------------------------------

        auto pStateDefinition = GetDefinition<StateNode>();
        if ( pStateDefinition->m_isOffState )
        {
            context.m_pLayerContext->m_layerWeight = 0.0f;
            context.m_pLayerContext->m_rootMotionLayerWeight = 0.0f;
        }
        else
        {
            if ( m_pLayerWeightNode != nullptr )
            {
                context.m_pLayerContext->m_layerWeight *= Math::Clamp( m_pLayerWeightNode->GetValue<float>( context ), 0.0f, 1.0f );
            }

            if ( m_pLayerRootMotionWeightNode != nullptr )
            {
                context.m_pLayerContext->m_rootMotionLayerWeight *= Math::Clamp( m_pLayerRootMotionWeightNode->GetValue<float>( context ), 0.0f, 1.0f );
            }
        }

        // Update bone mask task list
        //-------------------------------------------------------------------------

        if ( m_pBoneMaskNode != nullptr )
        {
            auto pBoneMaskTaskList = m_pBoneMaskNode->GetValue<BoneMaskTaskList const*>( context );
            if ( pBoneMaskTaskList != nullptr )
            {
                // If we dont have a bone mask task list, use a copy of the state's task list
                if ( !context.m_pLayerContext->m_layerMaskTaskList.HasTasks() )
                {
                    context.m_pLayerContext->m_layerMaskTaskList.CopyFrom( *pBoneMaskTaskList );
                }
                else // If we already have a bone mask set, combine the bone masks
                {
                    context.m_pLayerContext->m_layerMaskTaskList.CombineWith( *pBoneMaskTaskList );
                }
            }
        }
    }

    GraphPoseNodeResult StateNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        EE_ASSERT( context.IsValid() );

        MarkNodeActive( context );

        // Set the result to a valid event range since we are recording it
        GraphPoseNodeResult result;
        m_sampledEventRange = context.GetEmptySampledEventRange();

        // Update child
        if ( m_pChildNode != nullptr && m_pChildNode->IsValid() )
        {
            result = m_pChildNode->Update( context, pUpdateRange );
            m_duration = m_pChildNode->GetDuration();
            m_previousTime = m_pChildNode->GetPreviousTime();
            m_currentTime = m_pChildNode->GetCurrentTime();
            m_sampledEventRange = result.m_sampledEventRange;
        }

        // Track time spent in state
        m_elapsedTimeInState += context.m_deltaTime;

        // Sample graph events ( we need to track the sampled range for this node explicitly )
        SampleStateEvents( context );

        // Update the result range to take into account any sampled state events
        result.m_sampledEventRange = m_sampledEventRange;

        // Update layer context and return
        UpdateLayerContext( context );
        m_isFirstStateUpdate = false;
        return result;
    }

    #if EE_DEVELOPMENT_TOOLS
    void StateNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_elapsedTimeInState );
        outState.WriteValue( m_transitionState );
        outState.WriteValue( m_isFirstStateUpdate );
    }

    void StateNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );
        inState.ReadValue( m_elapsedTimeInState );
        inState.ReadValue( m_transitionState );
        inState.ReadValue( m_isFirstStateUpdate );
    }
    #endif
}