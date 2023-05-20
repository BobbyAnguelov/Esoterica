#include "Animation_RuntimeGraphNode_Events.h"
#include "Animation_RuntimeGraphNode_State.h"
#include "Engine/Animation/Events/AnimationEvent_ID.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void IDEventConditionNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<IDEventConditionNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_sourceStateNodeIdx, pNode->m_pSourceStateNode );
    }

    void IDEventConditionNode::InitializeInternal( GraphContext& context )
    {
        BoolValueNode::InitializeInternal( context );
        if ( m_pSourceStateNode != nullptr )
        {
            EE_ASSERT( m_pSourceStateNode->IsInitialized() );
        }

        m_result = false;
    }

    void IDEventConditionNode::ShutdownInternal( GraphContext& context )
    {
        BoolValueNode::ShutdownInternal( context );
    }

    void IDEventConditionNode::GetValueInternal( GraphContext& context, void* pOutValue )
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

    bool IDEventConditionNode::TryMatchTags( GraphContext& context ) const
    {
        auto pNodeSettings = GetSettings<IDEventConditionNode>();
        int32_t const numEventIDs = (int32_t) pNodeSettings->m_eventIDs.size();
        auto foundIDs = EE_STACK_ARRAY_ALLOC( bool, numEventIDs );
        Memory::MemsetZero( foundIDs, sizeof( bool ) * numEventIDs );

        // Calculate event search range
        //-------------------------------------------------------------------------

        bool const shouldSearchAllEvents = ( m_pSourceStateNode == nullptr );

        SampledEventRange searchRange( 0, 0 );
        if ( shouldSearchAllEvents || context.IsInLayer() )
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

            if ( sampledEvent.IsIgnored() )
            {
                continue;
            }

            // Skip events from inactive branch if so requested
            if ( pNodeSettings->m_onlyCheckEventsFromActiveBranch && !sampledEvent.IsFromActiveBranch() )
            {
                continue;
            }

            // Animation Event
            if ( sampledEvent.IsAnimationEvent() )
            {
                if ( pNodeSettings->m_searchRule != SearchRule::OnlySearchStateEvents )
                {
                    if ( auto pIDEvent = sampledEvent.TryGetEvent<IDEvent>() )
                    {
                        foundID = pIDEvent->GetID();
                    }
                }
            }
            else // State event
            {
                if ( pNodeSettings->m_searchRule != SearchRule::OnlySearchAnimEvents )
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
            if ( shouldSearchAllEvents || context.IsInLayer() )
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

            float foundPercentageThrough = 0.0f;
            float highestWeightFound = -1.0f;
            bool eventFound = false;

            for ( auto i = searchRange.m_startIdx; i < searchRange.m_endIdx; i++ )
            {
                auto pSampledEvent = &context.m_sampledEventsBuffer[i];
                if ( pSampledEvent->IsIgnored() || pSampledEvent->IsStateEvent() || ( pSettings->m_onlyCheckEventsFromActiveBranch && !pSampledEvent->IsFromActiveBranch() ) )
                {
                    continue;
                }

                //-------------------------------------------------------------------------

                if ( auto pEvent = pSampledEvent->TryGetEvent<IDEvent>() )
                {
                    if ( pSettings->m_eventID != pEvent->GetID() )
                    {
                        continue;
                    }

                    //-------------------------------------------------------------------------

                    bool updateEvent = false;

                    // If we already have a found event then apply priority rule
                    if ( eventFound )
                    {
                        if ( pSettings->m_priorityRule == EventPriorityRule::HighestWeight )
                        {
                            if ( pSampledEvent->GetWeight() >= highestWeightFound )
                            {
                                updateEvent = true;
                            }
                        }
                        else // Prefer higher percentage through
                        {
                            if ( pSampledEvent->GetPercentageThrough().ToFloat() >= foundPercentageThrough )
                            {
                                updateEvent = true;
                            }
                        }
                    }
                    else // Just update the found data
                    {
                        updateEvent = true;
                    }

                    //-------------------------------------------------------------------------

                    if ( updateEvent )
                    {
                        eventFound = true;
                        foundPercentageThrough = pSampledEvent->GetPercentageThrough().ToFloat();
                        highestWeightFound = pSampledEvent->GetWeight();
                    }
                }
            }

            // Check event data if the specified event is found
            //-------------------------------------------------------------------------

            m_result = -1.0f;

            if ( eventFound )
            {
                m_result = foundPercentageThrough;
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
            if ( shouldSearchAllEvents || context.IsInLayer() )
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

            bool eventFound = false;
            m_result = false;

            for ( auto i = searchRange.m_startIdx; i < searchRange.m_endIdx; i++ )
            {
                auto pSampledEvent = &context.m_sampledEventsBuffer[i];
                if ( pSampledEvent->IsIgnored() || pSampledEvent->IsStateEvent() || ( pSettings->m_onlyCheckEventsFromActiveBranch && !pSampledEvent->IsFromActiveBranch() ) )
                {
                    continue;
                }

                //-------------------------------------------------------------------------

                if ( auto pEvent = pSampledEvent->TryGetEvent<FootEvent>() )
                {
                    auto const foot = pEvent->GetFootPhase();
                    switch ( pSettings->m_phaseCondition )
                    {
                        case FootEvent::PhaseCondition::LeftFootDown:
                        {
                            m_result = ( foot == FootEvent::Phase::LeftFootDown );
                            eventFound = true;
                        }
                        break;

                        case FootEvent::PhaseCondition::RightFootDown:
                        {
                            m_result = ( foot == FootEvent::Phase::RightFootDown );
                            eventFound = true;
                        }
                        break;

                        case FootEvent::PhaseCondition::LeftFootPassing:
                        {
                            m_result = ( foot == FootEvent::Phase::LeftFootPassing );
                            eventFound = true;
                        }
                        break;

                        case FootEvent::PhaseCondition::RightFootPassing:
                        {
                            m_result = ( foot == FootEvent::Phase::RightFootPassing );
                            eventFound = true;
                        }
                        break;

                        case FootEvent::PhaseCondition::LeftPhase:
                        {
                            m_result = ( foot == FootEvent::Phase::RightFootPassing || foot == FootEvent::Phase::LeftFootDown );
                            eventFound = true;
                        }
                        break;

                        case FootEvent::PhaseCondition::RightPhase:
                        {
                            m_result = ( foot == FootEvent::Phase::LeftFootPassing || foot == FootEvent::Phase::RightFootDown );
                            eventFound = true;
                        }
                        break;
                    }
                }

                //-------------------------------------------------------------------------

                if ( eventFound )
                {
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
        auto pSettings = GetSettings<FootstepEventPercentageThroughNode>();

        // Is the Result up to date?
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            // Calculate event search range
            //-------------------------------------------------------------------------

            bool const shouldSearchAllEvents = ( m_pSourceStateNode == nullptr );

            SampledEventRange searchRange( 0, 0 );
            if ( shouldSearchAllEvents || context.IsInLayer() )
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

            float foundPercentageThrough = 0.0f;
            float highestWeightFound = -1.0f;
            bool eventFound = false;

            for ( auto i = searchRange.m_startIdx; i < searchRange.m_endIdx; i++ )
            {
                auto pSampledEvent = &context.m_sampledEventsBuffer[i];
                if ( pSampledEvent->IsIgnored() || pSampledEvent->IsStateEvent() || ( pSettings->m_onlyCheckEventsFromActiveBranch && !pSampledEvent->IsFromActiveBranch() ) )
                {
                    continue;
                }

                //-------------------------------------------------------------------------

                if ( auto pEvent = pSampledEvent->TryGetEvent<FootEvent>() )
                {
                    auto const foot = pEvent->GetFootPhase();
                    switch ( pSettings->m_phaseCondition )
                    {
                        case FootEvent::PhaseCondition::LeftFootDown:
                        {
                            if ( foot != FootEvent::Phase::LeftFootDown )
                            {
                                continue;
                            }
                        }
                        break;

                        case FootEvent::PhaseCondition::RightFootDown:
                        {
                            if ( foot != FootEvent::Phase::RightFootDown )
                            {
                                continue;
                            }
                        }
                        break;

                        case FootEvent::PhaseCondition::LeftFootPassing:
                        {
                            if ( foot != FootEvent::Phase::LeftFootPassing )
                            {
                                continue;
                            }
                        }
                        break;

                        case FootEvent::PhaseCondition::RightFootPassing:
                        {
                            if ( foot != FootEvent::Phase::RightFootPassing )
                            {
                                continue;
                            }
                        }
                        break;

                        case FootEvent::PhaseCondition::LeftPhase:
                        {
                            if ( foot != FootEvent::Phase::RightFootPassing && foot != FootEvent::Phase::LeftFootDown )
                            {
                                continue;
                            }
                        }
                        break;

                        case FootEvent::PhaseCondition::RightPhase:
                        {
                            if ( foot != FootEvent::Phase::LeftFootPassing && foot != FootEvent::Phase::RightFootDown )
                            {
                                continue;
                            }
                        }
                        break;
                    }

                    //-------------------------------------------------------------------------

                    bool updateEvent = false;

                    // If we already have a found event then apply priority rule
                    if ( eventFound )
                    {
                        if ( pSettings->m_priorityRule == EventPriorityRule::HighestWeight )
                        {
                            if ( pSampledEvent->GetWeight() >= highestWeightFound )
                            {
                                updateEvent = true;
                            }
                        }
                        else // Prefer higher percentage through
                        {
                            if ( pSampledEvent->GetPercentageThrough().ToFloat() >= foundPercentageThrough )
                            {
                                updateEvent = true;
                            }
                        }
                    }
                    else // Just update the found data
                    {
                        updateEvent = true;
                    }

                    //-------------------------------------------------------------------------

                    if ( updateEvent )
                    {
                        eventFound = true;
                        foundPercentageThrough = pSampledEvent->GetPercentageThrough().ToFloat();
                        highestWeightFound = pSampledEvent->GetWeight();
                    }
                }
            }

            // Check event data if the specified event is found
            //-------------------------------------------------------------------------

            m_result = -1.0f;

            if ( eventFound )
            {
                m_result = foundPercentageThrough;
            }
        }

        *( (float*) pOutValue ) = m_result;
    }

    //-------------------------------------------------------------------------

    void SyncEventIndexConditionNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<SyncEventIndexConditionNode>( context, options );
        context.SetNodePtrFromIndex( m_sourceStateNodeIdx, pNode->m_pSourceStateNode );
    }

    void SyncEventIndexConditionNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pSourceStateNode != nullptr );
        BoolValueNode::InitializeInternal( context );
        m_result = false;
    }

    void SyncEventIndexConditionNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pSourceStateNode != nullptr );
        BoolValueNode::ShutdownInternal( context );
    }

    void SyncEventIndexConditionNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pSourceStateNode != nullptr );
        auto pSettings = GetSettings<SyncEventIndexConditionNode>();

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

    //-------------------------------------------------------------------------

    void TransitionEventConditionNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<TransitionEventConditionNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_sourceStateNodeIdx, pNode->m_pSourceStateNode );
    }

    void TransitionEventConditionNode::InitializeInternal( GraphContext& context )
    {
        BoolValueNode::InitializeInternal( context );
        if ( m_pSourceStateNode != nullptr )
        {
            EE_ASSERT( m_pSourceStateNode->IsInitialized() );
        }

        m_result = false;
    }

    void TransitionEventConditionNode::ShutdownInternal( GraphContext& context )
    {
        BoolValueNode::ShutdownInternal( context );
    }

    void TransitionEventConditionNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && pOutValue != nullptr );
        auto pSettings = GetSettings<TransitionEventConditionNode>();

        // Is the Result up to date?
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            // Calculate event search range
            //-------------------------------------------------------------------------

            bool const shouldSearchAllEvents = ( m_pSourceStateNode == nullptr );

            SampledEventRange searchRange( 0, 0 );
            if ( shouldSearchAllEvents || context.IsInLayer() )
            {
                // If we dont have a child node set, search all sampled events for the event
                searchRange.m_endIdx = context.m_sampledEventsBuffer.GetNumSampledEvents();
            }
            else // Only search the sampled event range for the child node
            {
                searchRange = m_pSourceStateNode->GetSampledEventRange();
            }

            // Search sampled events for all events sampled this frame (we may have multiple, even From the same source)
            //-------------------------------------------------------------------------

            bool eventFound = false;
            TransitionMarker markerFound = TransitionMarker::AllowTransition;

            for ( auto i = searchRange.m_startIdx; i < searchRange.m_endIdx; i++ )
            {
                auto pSampledEvent = &context.m_sampledEventsBuffer[i];
                if ( pSampledEvent->IsIgnored() || pSampledEvent->IsStateEvent() || ( pSettings->m_onlyCheckEventsFromActiveBranch && !pSampledEvent->IsFromActiveBranch() ) )
                {
                    continue;
                }

                if ( auto pEvent = pSampledEvent->TryGetEvent<TransitionEvent>() )
                {
                    if ( pSettings->m_markerIDToMatch != pEvent->GetMarkerID() )
                    {
                       continue;
                    }

                    eventFound = true;
                    auto const eventMarker = pEvent->GetMarker();

                    // We return the most restrictive marker found
                    if ( eventMarker > markerFound )
                    {
                        markerFound = eventMarker;
                    }
                }
            }

            // Check found marker against node condition
            //-------------------------------------------------------------------------

            m_result = false;

            if ( eventFound )
            {
                switch ( pSettings->m_markerCondition )
                {
                    case TransitionMarkerCondition::AnyAllowed:
                    m_result = ( markerFound != TransitionMarker::BlockTransition );
                    break;

                    case TransitionMarkerCondition::FullyAllowed:
                    m_result = ( markerFound == TransitionMarker::AllowTransition );
                    break;

                    case TransitionMarkerCondition::ConditionallyAllowed:
                    m_result = ( markerFound == TransitionMarker::ConditionallyAllowTransition );
                    break;

                    case TransitionMarkerCondition::Blocked:
                    m_result = ( markerFound == TransitionMarker::BlockTransition );
                    break;
                }
            }
        }

        *( (bool*) pOutValue ) = m_result;
    }
}