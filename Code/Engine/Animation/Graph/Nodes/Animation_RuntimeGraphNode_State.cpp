#include "Animation_RuntimeGraphNode_State.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "Base/Types/ScopedValue.h"

//-------------------------------------------------------------------------

namespace EE::Animation
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

    SampledEventRange StateNode::StartTransitionOut( GraphContext& context, bool isZeroDurationTransition )
    {
        EE_ASSERT( context.IsValid() );

        m_transitionState = TransitionState::TransitioningOut;

        // Since we update states before we register transitions, we need to mark all previous events as from the inactive branch
        context.GetSampledEventsBuffer()->MarkEventsAsFromInactiveBranch( m_sampledEventRange );

        // For zero-length transitions, we will not get another update so we need to emit the exit events now as well
        if ( isZeroDurationTransition )
        {
            TScopedGuardValue const g( context.m_branchState, BranchState::Inactive );
            SampleStateEvents( context );
        }

        return m_sampledEventRange;
    }

    void StateNode::SampleStateEvents( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        auto pStateDefinition = GetDefinition<StateNode>();

        SourcePath const sourcePath = GetNodePath( context );

        auto pSampledEventsBuffer = context.GetSampledEventsBuffer();

        // Sample Fixed Stage Events
        //-------------------------------------------------------------------------

        bool isInActiveBranch = context.m_branchState == BranchState::Active;

        if ( m_isFirstStateUpdate || ( m_transitionState == TransitionState::TransitioningIn && isInActiveBranch ) )
        {
            for ( auto const& entryEventID : pStateDefinition->m_entryEvents )
            {
                pSampledEventsBuffer->EmplaceGraphEvent( sourcePath, GraphEventType::Entry, entryEventID, isInActiveBranch );
            }
        }
        else if ( m_transitionState == TransitionState::None && isInActiveBranch )
        {
            for ( auto const& fullyInStateEventID : pStateDefinition->m_fullyInStateEvents )
            {
                pSampledEventsBuffer->EmplaceGraphEvent( sourcePath, GraphEventType::FullyInState, fullyInStateEventID, isInActiveBranch );
            }
        }
        else if ( m_transitionState == TransitionState::TransitioningOut )
        {
            for ( auto const& exitEventID : pStateDefinition->m_exitEvents )
            {
                pSampledEventsBuffer->EmplaceGraphEvent( sourcePath, GraphEventType::Exit, exitEventID, isInActiveBranch );
            }
        }

        // Sample Timed Events
        //-------------------------------------------------------------------------

        Seconds const elapsedTime = m_duration * m_currentTime.ToFloat();
        for ( auto const& timedEvent : pStateDefinition->m_timedElapsedEvents )
        {
            if ( timedEvent.m_comparisonOperator == TimedEvent::Comparison::GreaterThanEqual )
            {
                if ( elapsedTime >= timedEvent.m_timeValue )
                {
                    pSampledEventsBuffer->EmplaceGraphEvent( sourcePath, GraphEventType::Timed, timedEvent.m_ID, isInActiveBranch );
                }
            }
            else
            {
                if ( elapsedTime <= timedEvent.m_timeValue )
                {
                    pSampledEventsBuffer->EmplaceGraphEvent( sourcePath, GraphEventType::Timed, timedEvent.m_ID, isInActiveBranch );
                }
            }
        }

        Seconds const currentTimeRemaining = ( 1.0f - m_currentTime ) * m_duration;
        for ( auto const& timedEvent : pStateDefinition->m_timedRemainingEvents )
        {
            if ( timedEvent.m_comparisonOperator == TimedEvent::Comparison::GreaterThanEqual )
            {
                if ( currentTimeRemaining >= timedEvent.m_timeValue )
                {
                    pSampledEventsBuffer->EmplaceGraphEvent( sourcePath, GraphEventType::Timed, timedEvent.m_ID, isInActiveBranch );
                }
            }
            else
            {
                if ( currentTimeRemaining <= timedEvent.m_timeValue )
                {
                    pSampledEventsBuffer->EmplaceGraphEvent( sourcePath, GraphEventType::Timed, timedEvent.m_ID, isInActiveBranch );
                }
            }
        }

        //-------------------------------------------------------------------------

        m_sampledEventRange.m_endIdx = context.GetSampledEventsBuffer()->GetNumSampledEvents();
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

        // Sample graph events ( we need to track the sampled range for this node explicitly )
        SampleStateEvents( context );

        // Update the result range to take into account any sampled state events
        result.m_sampledEventRange = m_sampledEventRange;

        // Update layer context and return
        UpdateLayerContext( context );
        m_isFirstStateUpdate = false;
        return result;
    }

    void StateNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
        outState.WriteValue( m_transitionState );
        outState.WriteValue( m_isFirstStateUpdate );
    }

    bool StateNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        if( !PoseNode::RestoreGraphState( inState ))
        {
            return false;
        }

        inState.ReadValue( m_transitionState );
        inState.ReadValue( m_isFirstStateUpdate );

        return true;
    }
}