#include "Animation_RuntimeGraphNode_Events.h"
#include "Animation_RuntimeGraphNode_State.h"
#include "Engine/Animation/Events/AnimationEvent_ID.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    template<typename T>
    static SampledEvent const* GetDominantEvent( SampledEventsBuffer const& sampledEvents, SampledEventRange const& searchRange, bool preferHigherPercentageThrough )
    {
        SampledEvent const* pDominantSampledEvent = nullptr;

        for ( auto i = searchRange.m_startIdx; i < searchRange.m_endIdx; i++ )
        {
            auto pSampledEvent = &sampledEvents[i];
            T const* pEvent = pSampledEvent->TryGetEvent<T>();
            if ( pEvent != nullptr )
            {
                bool const isDominantEventSet = ( pDominantSampledEvent != nullptr );
                bool shouldUpdateEvent = !isDominantEventSet;
                if ( isDominantEventSet )
                {
                    // If we have higher weight, use this event as the reference
                    if ( pSampledEvent->GetWeight() > pDominantSampledEvent->GetWeight() )
                    {
                        shouldUpdateEvent = true;
                    }
                    else if ( pSampledEvent->GetWeight() == pDominantSampledEvent->GetWeight() )
                    {
                        // If the source node is the same, update the event selection based on percentage through preference
                        bool const isSameSourceNode = pSampledEvent->GetSourceNodeIndex() == pDominantSampledEvent->GetSourceNodeIndex();
                        if ( isSameSourceNode )
                        {
                            bool const newEventHasHigherPercentageThrough = pSampledEvent->GetPercentageThrough() > pDominantSampledEvent->GetPercentageThrough();
                            if ( preferHigherPercentageThrough && newEventHasHigherPercentageThrough )
                            {
                                shouldUpdateEvent = true;
                            }
                            else if ( !preferHigherPercentageThrough && !newEventHasHigherPercentageThrough )
                            {
                                shouldUpdateEvent = true;
                            }
                        }
                    }
                }

                if ( shouldUpdateEvent )
                {
                    pDominantSampledEvent = pSampledEvent;
                }
            }
        }

        //-------------------------------------------------------------------------

        return pDominantSampledEvent;
    }

    //-------------------------------------------------------------------------

    void GenericEventConditionNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<GenericEventConditionNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_sourceStateNodeIdx, pNode->m_pSourceStateNode );
    }

    void GenericEventConditionNode::InitializeInternal( GraphContext& context )
    {
        BoolValueNode::InitializeInternal( context );
        if ( m_pSourceStateNode != nullptr )
        {
            EE_ASSERT( m_pSourceStateNode->IsInitialized() );
        }

        m_result = false;
    }

    void GenericEventConditionNode::ShutdownInternal( GraphContext& context )
    {
        BoolValueNode::ShutdownInternal( context );
    }

    void GenericEventConditionNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && pOutValue != nullptr );

        // Is the Result up to date?
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            m_result = TryMatchTags( context );
        }

        // Set Result
        *( (bool*) pOutValue ) = m_result;
    }

    bool GenericEventConditionNode::TryMatchTags( GraphContext& context ) const
    {
        auto pNodeSettings = GetSettings<GenericEventConditionNode>();
        int32_t const numEventIDs = (int32_t) pNodeSettings->m_eventIDs.size();
        auto foundIDs = EE_STACK_ARRAY_ALLOC( bool, numEventIDs );
        Memory::MemsetZero( foundIDs, sizeof( bool ) * numEventIDs );

        // Calculate event search range
        //-------------------------------------------------------------------------

        bool const shouldSearchAllEvents = ( m_pSourceStateNode == nullptr );

        SampledEventRange searchRange( 0, 0 );
        if ( shouldSearchAllEvents || context.m_layerContext.m_isCurrentlyInLayer )
        {
            // If we dont have a child node set, search all sampled events for the event
            searchRange.m_endIdx = context.m_sampledEventsBuffer.GetNumSampledEvents();
        }
        else // Only search the sampled event range for the child node
        {
            searchRange = m_pSourceStateNode->GetSampledEventRange();
        }

        // Perform search
        //-------------------------------------------------------------------------

        for ( auto i = searchRange.m_startIdx; i != searchRange.m_endIdx; i++ )
        {
            StringID foundID;

            SampledEvent const& sampledEvent = context.m_sampledEventsBuffer[i];

            // Animation Event
            if ( sampledEvent.IsAnimationEvent() )
            {
                if ( pNodeSettings->m_searchMode != EventSearchMode::OnlySearchStateEvents )
                {
                    if ( auto pIDEvent = sampledEvent.TryGetEvent<IDEvent>() )
                    {
                        foundID = pIDEvent->GetID();
                    }
                }
            }
            else // State event
            {
                if ( pNodeSettings->m_searchMode != EventSearchMode::OnlySearchAnimEvents )
                {
                    foundID = sampledEvent.GetStateEventID();
                }
            }

            //-------------------------------------------------------------------------

            if ( foundID.IsValid() )
            {
                // Check against the set of events we need to match
                for ( auto t = 0; t < numEventIDs; t++ )
                {
                    if ( pNodeSettings->m_eventIDs[t] == foundID )
                    {
                        // 
                        if ( pNodeSettings->m_operator == Operator::Or )
                        {
                            return true;
                        }
                        else
                        {
                            foundIDs[t] = true;
                            break;
                        }
                    }
                }
            }
        }

        // Ensure that all events have been found
        for ( auto t = 0; t < numEventIDs; t++ )
        {
            if ( !foundIDs[t] )
            {
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    void GenericEventPercentageThroughNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<GenericEventPercentageThroughNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_sourceStateNodeIdx, pNode->m_pSourceStateNode );
    }

    void GenericEventPercentageThroughNode::InitializeInternal( GraphContext& context )
    {
        FloatValueNode::InitializeInternal( context );
        if ( m_pSourceStateNode != nullptr )
        {
            EE_ASSERT( m_pSourceStateNode->IsInitialized() );
        }

        m_result = false;
    }

    void GenericEventPercentageThroughNode::ShutdownInternal( GraphContext& context )
    {
        FloatValueNode::ShutdownInternal( context );
    }

    void GenericEventPercentageThroughNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && pOutValue != nullptr );
        auto pSettings = GetSettings<GenericEventPercentageThroughNode>();

        // Is the Result up to date?
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            // Calculate event search range
            //-------------------------------------------------------------------------

            bool const shouldSearchAllEvents = ( m_pSourceStateNode == nullptr );

            SampledEventRange searchRange( 0, 0 );
            if ( shouldSearchAllEvents || context.m_layerContext.m_isCurrentlyInLayer )
            {
                // If we dont have a child node set, search all sampled events for the event
                searchRange.m_endIdx = context.m_sampledEventsBuffer.GetNumSampledEvents();
            }
            else // Only search the sampled event range for the source state node
            {
                searchRange = m_pSourceStateNode->GetSampledEventRange();
            }

            // Search sampled events for all footstep events sampled this frame (we may have multiple, even From the same source)
            //-------------------------------------------------------------------------

            SampledEvent const* pDominantSampledEvent = GetDominantEvent<IDEvent>( context.m_sampledEventsBuffer, searchRange, pSettings->m_preferHighestPercentageThrough );

            // Check event data if the specified event is found
            //-------------------------------------------------------------------------

            m_result = -1.0f;

            if ( pDominantSampledEvent != nullptr )
            {
                auto pDominantIDEvent = pDominantSampledEvent->GetEvent<IDEvent>();
                if ( pDominantIDEvent->GetID() == pSettings->m_eventID )
                {
                    m_result = pDominantSampledEvent->GetPercentageThrough();
                }
            }
        }

        *( (float*) pOutValue ) = m_result;
    }

    //-------------------------------------------------------------------------

    void FootEventConditionNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FootEventConditionNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_sourceStateNodeIdx, pNode->m_pSourceStateNode );
    }

    void FootEventConditionNode::InitializeInternal( GraphContext& context )
    {
        BoolValueNode::InitializeInternal( context );
        if ( m_pSourceStateNode != nullptr )
        {
            EE_ASSERT( m_pSourceStateNode->IsInitialized() );
        }

        m_result = false;
    }

    void FootEventConditionNode::ShutdownInternal( GraphContext& context )
    {
        BoolValueNode::ShutdownInternal( context );
    }

    void FootEventConditionNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && pOutValue != nullptr );
        auto pSettings = GetSettings<FootEventConditionNode>();

        // Is the Result up to date?
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            // Calculate event search range
            //-------------------------------------------------------------------------

            bool const shouldSearchAllEvents = ( m_pSourceStateNode == nullptr );

            SampledEventRange searchRange( 0, 0 );
            if ( shouldSearchAllEvents || context.m_layerContext.m_isCurrentlyInLayer )
            {
                // If we dont have a child node set, search all sampled events for the event
                searchRange.m_endIdx = context.m_sampledEventsBuffer.GetNumSampledEvents();
            }
            else // Only search the sampled event range for the child node
            {
                searchRange = m_pSourceStateNode->GetSampledEventRange();
            }

            // Search sampled events for all footstep events sampled this frame (we may have multiple, even From the same source)
            //-------------------------------------------------------------------------

            SampledEvent const* pDominantSampledEvent = GetDominantEvent<FootEvent>( context.m_sampledEventsBuffer, searchRange, pSettings->m_preferHighestPercentageThrough );

            // Check event data if the specified event is found
            //-------------------------------------------------------------------------

            m_result = false;
            if ( pDominantSampledEvent != nullptr )
            {
                auto pDominantFootstepEvent = pDominantSampledEvent->GetEvent<FootEvent>();

                auto const Foot = pDominantFootstepEvent->GetFootPhase();
                switch ( pSettings->m_phaseCondition )
                {
                    case FootEvent::PhaseCondition::LeftFootDown:
                    m_result = ( Foot == FootEvent::Phase::LeftFootDown );
                    break;

                    case FootEvent::PhaseCondition::RightFootDown:
                    m_result = ( Foot == FootEvent::Phase::RightFootDown );
                    break;

                    case FootEvent::PhaseCondition::LeftFootPassing:
                    m_result = ( Foot == FootEvent::Phase::LeftFootPassing );
                    break;

                    case FootEvent::PhaseCondition::RightFootPassing:
                    m_result = ( Foot == FootEvent::Phase::RightFootPassing );
                    break;

                    case FootEvent::PhaseCondition::LeftPhase:
                    m_result = ( Foot == FootEvent::Phase::RightFootPassing || Foot == FootEvent::Phase::LeftFootDown );
                    break;

                    case FootEvent::PhaseCondition::RightPhase:
                    m_result = ( Foot == FootEvent::Phase::LeftFootPassing || Foot == FootEvent::Phase::RightFootDown );
                    break;
                }
            }
        }

        *( (bool*) pOutValue ) = m_result;
    }

    //-------------------------------------------------------------------------

    void FootstepEventPercentageThroughNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<FootstepEventPercentageThroughNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_sourceStateNodeIdx, pNode->m_pSourceStateNode );
    }

    void FootstepEventPercentageThroughNode::InitializeInternal( GraphContext& context )
    {
        FloatValueNode::InitializeInternal( context );
        if ( m_pSourceStateNode != nullptr )
        {
            EE_ASSERT( m_pSourceStateNode->IsInitialized() );
        }

        m_result = false;
    }

    void FootstepEventPercentageThroughNode::ShutdownInternal( GraphContext& context )
    {
        FloatValueNode::ShutdownInternal( context );
    }

    void FootstepEventPercentageThroughNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && pOutValue != nullptr );
        auto pSettings = GetSettings<FootEventConditionNode>();

        // Is the Result up to date?
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            // Calculate event search range
            //-------------------------------------------------------------------------

            bool const shouldSearchAllEvents = ( m_pSourceStateNode == nullptr );

            SampledEventRange searchRange( 0, 0 );
            if ( shouldSearchAllEvents || context.m_layerContext.m_isCurrentlyInLayer )
            {
                // If we dont have a child node set, search all sampled events for the event
                searchRange.m_endIdx = context.m_sampledEventsBuffer.GetNumSampledEvents();
            }
            else // Only search the sampled event range for the source state node
            {
                searchRange = m_pSourceStateNode->GetSampledEventRange();
            }

            // Search sampled events for all footstep events sampled this frame (we may have multiple, even From the same source)
            //-------------------------------------------------------------------------

            SampledEvent const* pDominantSampledEvent = GetDominantEvent<FootEvent>( context.m_sampledEventsBuffer, searchRange, pSettings->m_preferHighestPercentageThrough );

            // Check event data if the specified event is found
            //-------------------------------------------------------------------------

            m_result = -1.0f;
            if ( pDominantSampledEvent != nullptr )
            {
                auto pDominantFootstepEvent = pDominantSampledEvent->GetEvent<FootEvent>();

                auto const Foot = pDominantFootstepEvent->GetFootPhase();
                switch ( pSettings->m_phaseCondition )
                {
                    case FootEvent::PhaseCondition::LeftFootDown:
                    {
                        if ( Foot == FootEvent::Phase::LeftFootDown )
                        {
                            m_result = pDominantSampledEvent->GetPercentageThrough();
                        }
                    }
                    break;

                    case FootEvent::PhaseCondition::RightFootDown:
                    {
                        if ( Foot == FootEvent::Phase::RightFootDown )
                        {
                            m_result = pDominantSampledEvent->GetPercentageThrough();
                        }
                    }
                    break;

                    case FootEvent::PhaseCondition::LeftFootPassing:
                    {
                        if ( Foot == FootEvent::Phase::LeftFootPassing )
                        {
                            m_result = pDominantSampledEvent->GetPercentageThrough();
                        }
                    }
                    break;

                    case FootEvent::PhaseCondition::RightFootPassing:
                    {
                        if ( Foot == FootEvent::Phase::RightFootPassing )
                        {
                            m_result = pDominantSampledEvent->GetPercentageThrough();
                        }
                    }
                    break;

                    case FootEvent::PhaseCondition::LeftPhase:
                    {
                        if ( Foot == FootEvent::Phase::RightFootPassing || Foot == FootEvent::Phase::LeftFootDown )
                        {
                            m_result = pDominantSampledEvent->GetPercentageThrough();
                        }
                    }
                    break;

                    case FootEvent::PhaseCondition::RightPhase:
                    {
                        if ( Foot == FootEvent::Phase::LeftFootPassing || Foot == FootEvent::Phase::RightFootDown )
                        {
                            m_result = pDominantSampledEvent->GetPercentageThrough();
                        }
                    }
                    break;
                }
            }
        }

        *( (float*) pOutValue ) = m_result;
    }

    //-------------------------------------------------------------------------

    void SyncEventConditionNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<SyncEventConditionNode>( context, options );
        context.SetNodePtrFromIndex( m_sourceStateNodeIdx, pNode->m_pSourceStateNode );
    }

    void SyncEventConditionNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pSourceStateNode != nullptr );
        BoolValueNode::InitializeInternal( context );
        m_result = false;
    }

    void SyncEventConditionNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pSourceStateNode != nullptr );
        BoolValueNode::ShutdownInternal( context );
    }

    void SyncEventConditionNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pSourceStateNode != nullptr );
        auto pSettings = GetSettings<SyncEventConditionNode>();

        // Is the Result up to date?
        if ( !WasUpdated( context ) )
        {
            auto const& sourceStateSyncTrack = m_pSourceStateNode->GetSyncTrack();
            auto currentSyncTime = sourceStateSyncTrack.GetTime( m_pSourceStateNode->GetCurrentTime() );

            switch ( pSettings->m_triggerMode )
            {
                case TriggerMode::ExactlyAtEventIndex:
                {
                    m_result = ( currentSyncTime.m_eventIdx == pSettings->m_syncEventIdx );
                }
                break;

                case TriggerMode::GreaterThanEqualToEventIndex:
                {
                    m_result = ( currentSyncTime.m_eventIdx >= pSettings->m_syncEventIdx );
                }
                break;
            }

            MarkNodeActive( context );
        }

        // Set Result
        *( (bool*) pOutValue ) = m_result;
    }

    //-------------------------------------------------------------------------

    void CurrentSyncEventNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<CurrentSyncEventNode>( context, options );
        context.SetNodePtrFromIndex( m_sourceStateNodeIdx, pNode->m_pSourceStateNode );
    }

    void CurrentSyncEventNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pSourceStateNode != nullptr );
        FloatValueNode::InitializeInternal( context );
        m_result = 0.0f;
    }

    void CurrentSyncEventNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pSourceStateNode != nullptr );
        FloatValueNode::ShutdownInternal( context );
    }

    void CurrentSyncEventNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pSourceStateNode != nullptr );

        // Is the Result up to date?
        if ( !WasUpdated( context ) )
        {
            auto const& sourceStateSyncTrack = m_pSourceStateNode->GetSyncTrack();
            auto currentSyncTime = sourceStateSyncTrack.GetTime( m_pSourceStateNode->GetCurrentTime() );
            m_result = (float) currentSyncTime.m_eventIdx;
            MarkNodeActive( context );
        }

        // Set Result
        *( (float*) pOutValue ) = m_result;
    }
}