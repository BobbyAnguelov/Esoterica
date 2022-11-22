#include "Animation_RuntimeGraphNode_State.h"
#include "System/Types/ScopedValue.h"
#include "Engine/Animation/AnimationBoneMask.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void StateNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<StateNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_childNodeIdx, pNode->m_pChildNode );
        context.SetOptionalNodePtrFromIndex( m_layerBoneMaskNodeIdx, pNode->m_pBoneMaskNode );
        context.SetOptionalNodePtrFromIndex( m_layerWeightNodeIdx, pNode->m_pLayerWeightNode );
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
        auto pStateSettings = GetSettings<StateNode>();

        // Ensure that we ALWAYS get the exit events when we destroy the state
        for ( auto const& exitEventID : pStateSettings->m_exitEvents )
        {
            context.m_sampledEventsBuffer.EmplaceStateEvent( GetNodeIndex(), StateEventType::Exit, exitEventID, false );
        }

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

    void StateNode::StartTransitionOut( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        m_transitionState = TransitionState::TransitioningOut;

        // Since we update states before we register transitions, we need to ignore all previously sampled state events for this frame
        context.m_sampledEventsBuffer.MarkOnlyStateEventsAsIgnored( m_sampledEventRange );

        // Since we update states before we register transitions, we need to mark all previous events as from the inactive branch
        context.m_sampledEventsBuffer.MarkEventsAsFromInactiveBranch( m_sampledEventRange );

        // Resample state events
        SampleStateEvents( context );
    }

    void StateNode::SampleStateEvents( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        auto pStateSettings = GetSettings<StateNode>();

        // Sample Fixed Stage Events
        //-------------------------------------------------------------------------

        bool isInActiveBranch = context.m_branchState == BranchState::Active;

        if ( m_isFirstStateUpdate || ( m_transitionState == TransitionState::TransitioningIn && isInActiveBranch ) )
        {
            for ( auto const& entryEventID : pStateSettings->m_entryEvents )
            {
                context.m_sampledEventsBuffer.EmplaceStateEvent( GetNodeIndex(), StateEventType::Entry, entryEventID, isInActiveBranch );
            }
        }
        else if ( m_transitionState == TransitionState::None && isInActiveBranch )
        {
            for ( auto const& executeEventID : pStateSettings->m_executeEvents )
            {
                context.m_sampledEventsBuffer.EmplaceStateEvent( GetNodeIndex(), StateEventType::FullyInState, executeEventID, isInActiveBranch );
            }
        }
        else if ( m_transitionState == TransitionState::TransitioningOut || !isInActiveBranch )
        {
            for ( auto const& exitEventID : pStateSettings->m_exitEvents )
            {
                context.m_sampledEventsBuffer.EmplaceStateEvent( GetNodeIndex(), StateEventType::Exit, exitEventID, isInActiveBranch );
            }
        }

        // Sample Timed Events
        //-------------------------------------------------------------------------

        Seconds const currentTimeElapsed( m_duration * m_currentTime.ToFloat() );
        for ( auto const& timedEvent : pStateSettings->m_timedElapsedEvents )
        {
            if ( currentTimeElapsed >= timedEvent.m_timeValue )
            {
                context.m_sampledEventsBuffer.EmplaceStateEvent( GetNodeIndex(), StateEventType::Timed, timedEvent.m_ID, isInActiveBranch );
            }
        }

        Seconds const currentTimeRemaining = ( 1.0f - m_currentTime ) * m_duration;
        for ( auto const& timedEvent : pStateSettings->m_timedRemainingEvents )
        {
            if ( currentTimeRemaining <= timedEvent.m_timeValue )
            {
                context.m_sampledEventsBuffer.EmplaceStateEvent( GetNodeIndex(), StateEventType::Timed, timedEvent.m_ID, isInActiveBranch );
            }
        }

        //-------------------------------------------------------------------------

        m_sampledEventRange.m_endIdx = context.m_sampledEventsBuffer.GetNumSampledEvents();
    }

    void StateNode::UpdateLayerContext( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        // Early out if we are not in a layer
        if ( !context.m_layerContext.IsSet() )
        {
            return;
        }

        // Update layer weight
        //-------------------------------------------------------------------------

        auto pStateSettings = GetSettings<StateNode>();
        if ( pStateSettings->m_isOffState )
        {
            context.m_layerContext.m_layerWeight = 0.0f;
        }
        else if ( m_pLayerWeightNode != nullptr )
        {
            context.m_layerContext.m_layerWeight *= Math::Clamp( m_pLayerWeightNode->GetValue<float>( context ), 0.0f, 1.0f );
        }

        // Update bone mask
        //-------------------------------------------------------------------------

        if ( m_pBoneMaskNode != nullptr )
        {
            auto pBoneMask = m_pBoneMaskNode->GetValue<BoneMask const*>( context );
            if ( pBoneMask != nullptr )
            {
                // If we dont have a bone mask set, use a copy of the state's mask
                if ( context.m_layerContext.m_pLayerMask == nullptr )
                {
                    context.m_layerContext.m_pLayerMask = context.m_boneMaskPool.GetBoneMask();
                    context.m_layerContext.m_pLayerMask->CopyFrom( *pBoneMask );
                }
                else // If we already have a bone mask set, combine the bone masks
                {
                    context.m_layerContext.m_pLayerMask->CombineWith( *pBoneMask );
                }
            }
        }
    }

    GraphPoseNodeResult StateNode::Update( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        MarkNodeActive( context );

        // Set the result to a valid event range since we are recording it
        GraphPoseNodeResult result;
        m_sampledEventRange = SampledEventRange( context.m_sampledEventsBuffer.GetNumSampledEvents() );

        // Update child
        if ( m_pChildNode != nullptr && m_pChildNode->IsValid() )
        {
            result = m_pChildNode->Update( context );
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

    GraphPoseNodeResult StateNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        EE_ASSERT( context.IsValid() );

        MarkNodeActive( context );

        // Set the result to a valid event range since we are recording it
        GraphPoseNodeResult result;
        m_sampledEventRange = SampledEventRange( context.m_sampledEventsBuffer.GetNumSampledEvents() );

        // Update child
        if ( m_pChildNode != nullptr && m_pChildNode->IsValid() )
        {
            result = m_pChildNode->Update( context, updateRange );
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