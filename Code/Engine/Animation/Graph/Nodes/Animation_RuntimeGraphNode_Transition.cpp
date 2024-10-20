#include "Animation_RuntimeGraphNode_Transition.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_RootMotionDebugger.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_CachedPose.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Blend.h"
#include "Engine/Animation/AnimationBlender.h"


//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void TransitionNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<TransitionNode>( context, options );
        context.SetNodePtrFromIndex( m_targetStateNodeIdx, pNode->m_pTargetNode );
        context.SetOptionalNodePtrFromIndex( m_durationOverrideNodeIdx, pNode->m_pDurationOverrideNode );
        context.SetOptionalNodePtrFromIndex( m_syncEventOffsetOverrideNodeIdx, pNode->m_pEventOffsetOverrideNode );
        context.SetOptionalNodePtrFromIndex( m_startBoneMaskNodeIdx, pNode->m_pStartBoneMaskNode );
        context.SetOptionalNodePtrFromIndex( m_targetSyncIDNodeIdx, pNode->m_pTargetSyncIDNode );
    }

    GraphPoseNodeResult TransitionNode::StartTransitionFromState( GraphContext& context, SyncTrackTimeRange const* pUpdateRange, GraphPoseNodeResult const& sourceNodeResult, StateNode* pSourceState, bool startCachingSourcePose )
    {
        EE_ASSERT( pSourceState != nullptr && m_pSourceNode == nullptr && WasInitialized() );

        m_pSourceNode = pSourceState;
        m_sourceType = SourceType::State;

        if ( startCachingSourcePose )
        {
            StartCachingSourcePose( context );
        }

        return InitializeTargetStateAndUpdateTransition( context, pUpdateRange, sourceNodeResult );
    }

    GraphPoseNodeResult TransitionNode::StartTransitionFromTransition( GraphContext& context, SyncTrackTimeRange const* pUpdateRange, GraphPoseNodeResult const& sourceNodeResult, TransitionNode* pSourceTransition, bool startCachingSourcePose )
    {
        EE_ASSERT( pSourceTransition != nullptr && m_pSourceNode == nullptr && WasInitialized() );

        m_pSourceNode = pSourceTransition;
        m_sourceType = SourceType::Transition;

        if( startCachingSourcePose )
        {
            StartCachingSourcePose( context );
        }

        return InitializeTargetStateAndUpdateTransition( context, pUpdateRange, sourceNodeResult );
    }

    GraphPoseNodeResult TransitionNode::InitializeTargetStateAndUpdateTransition( GraphContext& context, SyncTrackTimeRange const* pUpdateRange, GraphPoseNodeResult sourceNodeResult )
    {
        auto pDefinition = GetDefinition<TransitionNode>();

        //-------------------------------------------------------------------------

        MarkNodeActive( context );

        GraphPoseNodeResult result;

        // Record source root motion index
        #if EE_DEVELOPMENT_TOOLS
        m_rootMotionActionIdxSource = context.GetRootMotionDebugger()->GetLastActionIndex();
        #endif

        // Starting a transition out may generate additional state events so we need to update the sampled event range
        auto StartTransitionOutForSource = [&] ()
        {
            if ( m_sourceType == SourceType::State )
            {
                sourceNodeResult.m_sampledEventRange = GetSourceStateNode()->StartTransitionOut( context );
            }
            else
            {
                SampledEventRange const newTargetEventRange = GetSourceTransitionNode()->m_pTargetNode->StartTransitionOut( context );
                sourceNodeResult.m_sampledEventRange.m_endIdx = newTargetEventRange.m_endIdx;
            }
        };

        // Layer context update
        //-------------------------------------------------------------------------

        GraphLayerContext targetLayerContext;
        GraphLayerContext* pSourceLayerCtx = nullptr;
        if ( context.IsInLayer() )
        {
            pSourceLayerCtx = context.m_pLayerContext;
            context.m_pLayerContext = &targetLayerContext;
        }

        // Cache source node pose
        RegisterSourceCachePoseTask( context, sourceNodeResult );

        // Use sync events to initialize the target state
        //-------------------------------------------------------------------------

        SyncTrackTimeRange targetUpdateRange;
        GraphPoseNodeResult targetNodeResult;

        if ( pDefinition->IsSynchronized() || pUpdateRange != nullptr )
        {
            EE_ASSERT( m_pSourceNode != nullptr );
            SyncTrack const& sourceSyncTrack = m_pSourceNode->GetSyncTrack();

            // Calculate the source update sync range
            SyncTrackTimeRange sourceUpdateRange;
            sourceUpdateRange.m_startTime = sourceSyncTrack.GetTime( m_pSourceNode->GetPreviousTime() );
            sourceUpdateRange.m_endTime = sourceSyncTrack.GetTime( m_pSourceNode->GetCurrentTime() );

            // Calculate the sync event offset for this transition
            // We dont care about the percentage since we are synchronized to the source
            // Only apply sync offset if we are not part of a sync'ed update
            if ( pUpdateRange == nullptr )
            {
                if ( m_pEventOffsetOverrideNode != nullptr )
                {
                    m_pEventOffsetOverrideNode->Initialize( context );
                    m_syncEventOffset = Math::Floor( m_pEventOffsetOverrideNode->GetValue<float>( context ) );
                    m_pEventOffsetOverrideNode->Shutdown( context );
                }
                else
                {
                    m_syncEventOffset = Math::Floor( pDefinition->m_syncEventOffset );
                }
            }
            else
            {
                m_syncEventOffset = 0;
            }

            // Start transition and update the target state
            targetUpdateRange = sourceUpdateRange;
            targetUpdateRange.m_startTime.m_eventIdx += int32_t( m_syncEventOffset );
            targetUpdateRange.m_endTime.m_eventIdx += int32_t( m_syncEventOffset );

            // Transition out - this will resample any source state events so that the target state machine has all the correct state events
            StartTransitionOutForSource();

            // Initialize the target node
            m_pTargetNode->Initialize( context, targetUpdateRange.m_startTime );

            // Start transition in and update target
            m_pTargetNode->StartTransitionIn( context );
            targetNodeResult = m_pTargetNode->Update( context, &targetUpdateRange );

            #if EE_DEVELOPMENT_TOOLS
            m_rootMotionActionIdxTarget = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Calculate transition duration
            if ( pDefinition->ShouldClampTransitionLength() )
            {
                // Calculate the real end of the source state i.e. the percentage through for the end of the last sync event
                SyncTrackTime const sourceRealEndSyncTime = sourceSyncTrack.GetEndTime();
                Percentage const sourceRealEndTime = sourceSyncTrack.GetPercentageThrough( sourceRealEndSyncTime );
                Percentage const sourceCurrentTime = sourceSyncTrack.GetPercentageThrough( sourceUpdateRange.m_startTime );

                // Calculate the delta between the current position and the real end of the source
                float deltaBetweenCurrentTimeAndRealEndTime = 0.0f;
                if ( sourceRealEndTime > sourceCurrentTime )
                {
                    deltaBetweenCurrentTimeAndRealEndTime = sourceRealEndTime - sourceCurrentTime;
                }
                else
                {
                    deltaBetweenCurrentTimeAndRealEndTime = ( 1.0f - ( sourceCurrentTime - sourceRealEndTime ) );
                }

                // If the end of the source occurs before the transition completes, then we need to clamp the transition duration
                Seconds const sourceDuration = m_pSourceNode->GetDuration();
                if ( sourceDuration > 0.0f )
                {
                    Percentage const sourceTransitionDuration = Percentage( pDefinition->m_duration / m_pSourceNode->GetDuration() );
                    bool const shouldClamp = deltaBetweenCurrentTimeAndRealEndTime < sourceTransitionDuration;

                    // Calculate the target end position for this transition
                    SyncTrackTime transitionEndSyncTime;
                    if ( shouldClamp )
                    {
                        // Clamp to the last sync event in the source
                        transitionEndSyncTime = sourceRealEndSyncTime;
                    }
                    else
                    {
                        // Clamp to the estimated end position after the transition time
                        Percentage const sourceEndTimeAfterTransition = ( sourceCurrentTime + sourceTransitionDuration ).GetNormalizedTime();
                        transitionEndSyncTime = sourceSyncTrack.GetTime( sourceEndTimeAfterTransition );
                    }

                    // Calculate the transition duration in terms of event distance and update the progress for this transition
                    m_transitionLength = sourceSyncTrack.CalculatePercentageCovered( sourceUpdateRange.m_startTime, transitionEndSyncTime );
                }
            }
        }

        // Unsynchronized Transition
        //-------------------------------------------------------------------------

        else
        {
            // Try get sync event offset
            if ( m_pEventOffsetOverrideNode != nullptr )
            {
                m_pEventOffsetOverrideNode->Initialize( context );
                m_syncEventOffset = m_pEventOffsetOverrideNode->GetValue<float>( context );
                m_pEventOffsetOverrideNode->Shutdown( context );
            }
            else
            {
                m_syncEventOffset = pDefinition->m_syncEventOffset;
            }

            // If we have a sync offset or we need to match the source state time, we need to create a target state initial time
            bool const shouldMatchSourceTime = pDefinition->ShouldMatchSourceTime();
            if ( shouldMatchSourceTime || !Math::IsNearZero( m_syncEventOffset ) )
            {
                SyncTrackTime targetStartEventSyncTime;

                // Calculate the target start position (if we need to match the source)
                if ( shouldMatchSourceTime )
                {
                    SyncTrack const& sourceSyncTrack = m_pSourceNode->GetSyncTrack();
                    SyncTrackTime sourceFromSyncTime = sourceSyncTrack.GetTime( m_pSourceNode->GetCurrentTime() );

                    // Set the matching event index if required
                    if ( pDefinition->ShouldMatchSyncEventIndex() )
                    {
                        targetStartEventSyncTime.m_eventIdx = sourceFromSyncTime.m_eventIdx;
                    }
                    else if ( pDefinition->ShouldMatchSyncEventID() )
                    {
                        // Get the sync event ID to match
                        StringID eventIDToMatch;
                        if ( m_pTargetSyncIDNode != nullptr )
                        {
                            m_pTargetSyncIDNode->Initialize( context );
                            eventIDToMatch = m_pTargetSyncIDNode->GetValue<StringID>( context );
                            m_pTargetSyncIDNode->Shutdown( context );
                        }
                        else
                        {
                            eventIDToMatch = sourceSyncTrack.GetEventID( sourceFromSyncTime.m_eventIdx );
                        }

                        // Get the target sync track time
                        if ( eventIDToMatch.IsValid() )
                        {
                            // TODO: check if this will become a performance headache - initialization/shutdown should be cheap!
                            // If it becomes a headache - initialize it here and then conditionally initialize later... Our init time and update time will not match so that might be a problem for some nodes, but this option should be rarely used
                            m_pTargetNode->Initialize( context, targetStartEventSyncTime );
                            SyncTrack const& targetSyncTrack = m_pTargetNode->GetSyncTrack();
                            targetStartEventSyncTime.m_eventIdx = pDefinition->ShouldPreferClosestSyncEventID() ? targetSyncTrack.GetClosestEventIndexForID( sourceFromSyncTime, eventIDToMatch ) : targetSyncTrack.GetEventIndexForID( eventIDToMatch );
                            m_pTargetNode->Shutdown( context );
                        }
                    }

                    // Should we keep the source "From" percentage through
                    if ( pDefinition->ShouldMatchSyncEventPercentage() )
                    {
                        targetStartEventSyncTime.m_percentageThrough = sourceFromSyncTime.m_percentageThrough;
                    }
                }

                // Apply the sync event offset
                float eventIdxOffset;
                float percentageThroughOffset = Math::ModF( m_syncEventOffset, eventIdxOffset );
                targetStartEventSyncTime.m_eventIdx += (int32_t) eventIdxOffset;

                // Ensure we handle looping past the current event with the percentage offset
                targetStartEventSyncTime.m_percentageThrough = Math::ModF( targetStartEventSyncTime.m_percentageThrough + percentageThroughOffset, eventIdxOffset );
                targetStartEventSyncTime.m_eventIdx += (int32_t) eventIdxOffset;

                // Transition out - this will resample any source state events so that the target state machine has all the correct state events
                StartTransitionOutForSource();

                // Initialize the target node
                m_pTargetNode->Initialize( context, targetStartEventSyncTime );

                // Start transition in and update target node
                // Use a zero time-step as we dont want to update the target on this update but we do want the target pose to be created!
                m_pTargetNode->StartTransitionIn( context );
                float const oldDeltaTime = context.m_deltaTime;
                context.m_deltaTime = 0.0f;
                targetNodeResult = m_pTargetNode->Update( context, nullptr );
                context.m_deltaTime = oldDeltaTime;
            }
            else // Regular time update (not matched or has sync offset)
            {
                // Transition out - this will resample any source state events so that the target state machine has all the correct state events
                StartTransitionOutForSource();

                m_pTargetNode->Initialize( context, SyncTrackTime() );

                // Start transition in and update target
                m_pTargetNode->StartTransitionIn( context );
                targetNodeResult = m_pTargetNode->Update( context, nullptr );
            }

            #if EE_DEVELOPMENT_TOOLS
            m_rootMotionActionIdxTarget = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Should we clamp how long the transition is active for?
            if ( pDefinition->ShouldClampTransitionLength() )
            {
                Seconds const sourceDuration = m_pSourceNode->GetDuration();
                if ( sourceDuration > 0.0f )
                {
                    float const remainingNodeTime = ( 1.0f - m_pSourceNode->GetCurrentTime() ) * sourceDuration;
                    m_transitionLength = Math::Min( m_transitionLength, remainingNodeTime );
                }
            }
        }

        // Calculate the blend weight, register pose task and update layer weights
        //-------------------------------------------------------------------------

        CalculateBlendWeight();
        RegisterPoseTasksAndUpdateRootMotion( context, sourceNodeResult, targetNodeResult, result );

        if ( context.IsInLayer() )
        {
            // Calculate the new layer weights based on the transition progress
            UpdateLayerContext( pSourceLayerCtx, &targetLayerContext );

            // Restore original context
            context.m_pLayerContext = pSourceLayerCtx;
        }

        // Update internal time and events
        //-------------------------------------------------------------------------

        if ( pDefinition->IsSynchronized() || pUpdateRange != nullptr )
        {
            // Create the blended sync track
            SyncTrack const& sourceSyncTrack = m_pSourceNode->GetSyncTrack();
            SyncTrack const& targetSyncTrack = m_pTargetNode->GetSyncTrack();
            m_syncTrack = SyncTrack( sourceSyncTrack, targetSyncTrack, m_blendWeight );

            m_blendedDuration = SyncTrack::CalculateDurationSynchronized( m_pSourceNode->GetDuration(), m_pTargetNode->GetDuration(), sourceSyncTrack.GetNumEvents(), targetSyncTrack.GetNumEvents(), m_syncTrack.GetNumEvents(), m_blendWeight );
            m_previousTime = m_syncTrack.GetPercentageThrough( targetUpdateRange.m_startTime );
            m_currentTime = m_syncTrack.GetPercentageThrough( targetUpdateRange.m_endTime );
        }
        else
        {
            m_previousTime = 0.0f;
            m_currentTime = 0.0f;
            m_blendedDuration = Math::Lerp( IsSourceACachedPose() ? Seconds( 0.0f ) : m_pSourceNode->GetDuration(), m_pTargetNode->GetDuration(), m_blendWeight );
        }

        // Set the exposed-duration of the transition to the target to ensure that any "state completed" nodes trigger correctly
        m_duration = m_pTargetNode->GetDuration();

        // Generate the blended event range
        result.m_sampledEventRange = context.m_pSampledEventsBuffer->BlendEventRanges( sourceNodeResult.m_sampledEventRange, targetNodeResult.m_sampledEventRange, m_blendWeight );

        return result;
    }

    //-------------------------------------------------------------------------

    void TransitionNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        PoseNode::InitializeInternal( context, initialTime );
        auto pDefinition = GetDefinition<TransitionNode>();
        m_syncEventOffset = 0;

        // Reset transition duration and progress
        if ( m_pDurationOverrideNode != nullptr )
        {
            m_pDurationOverrideNode->Initialize( context );
            m_transitionLength = Math::Clamp( m_pDurationOverrideNode->GetValue<float>( context ), 0.0f, 10.0f );
            m_pDurationOverrideNode->Shutdown( context );
        }
        else
        {
            m_transitionLength = pDefinition->m_duration;
        }

        m_transitionProgress = 0.0f;
        m_blendWeight = 0.0f;
    }

    void TransitionNode::ShutdownInternal( GraphContext& context )
    {
        // Release cached pose buffers
        if ( m_cachedPoseBufferID.IsValid() )
        {
            context.m_pTaskSystem->DestroyCachedPose( m_cachedPoseBufferID );
            m_cachedPoseBufferID.Clear();
        }

        // Clear transition flags from target
        m_pTargetNode->SetTransitioningState( StateNode::TransitionState::None );
        m_currentTime = 1.0f;

        // Shutdown source node
        if ( m_pSourceNode != nullptr )
        {
            if ( IsSourceATransition() )
            {
                EndSourceTransition( context );
            }

            m_pSourceNode->Shutdown( context );
            m_pSourceNode = nullptr;
        }
        else
        {
            EE_ASSERT( IsSourceACachedPose() );
        }

        PoseNode::ShutdownInternal( context );
    }

    void TransitionNode::EndSourceTransition( GraphContext& context )
    {
        EE_ASSERT( IsSourceATransition() );

        auto pSourceTransitionNode = GetSourceTransitionNode();
        auto pSourceTransitionTargetState = pSourceTransitionNode->m_pTargetNode;

        // Set the source node to the target state of the source transition
        m_pSourceNode->Shutdown( context );
        m_pSourceNode = pSourceTransitionTargetState;
        m_sourceType = SourceType::State;

        // We need to explicitly set the transition state of the completed transition's target state as 
        // the shutdown of the transition will set it none. This will cause the state machine to potentially
        // transition to that state erroneously!
        GetSourceStateNode()->SetTransitioningState( StateNode::TransitionState::TransitioningOut );
    }

    //-------------------------------------------------------------------------

    bool TransitionNode::IsComplete( GraphContext& context ) const
    {
        if ( m_transitionLength <= 0.0f )
        {
            return true;
        }

        return ( m_transitionProgress + Percentage( context.m_deltaTime / m_transitionLength ).ToFloat() ) >= 1.0f;
    }

    //-------------------------------------------------------------------------

    void TransitionNode::StartCachingSourcePose( GraphContext& context )
    {
        EE_ASSERT( !m_cachedPoseBufferID.IsValid() );
        m_cachedPoseBufferID = context.m_pTaskSystem->CreateCachedPose();
        EE_ASSERT( m_cachedPoseBufferID.IsValid() );
    }

    void TransitionNode::NotifyNewTransitionStarting( GraphContext& context, StateNode* pTargetStateNode, TInlineVector<StateNode const *, 20> const& forceableFutureTargetStates )
    {
        if ( IsSourceATransition() )
        {
            auto pSourceTransitionNode = GetSourceTransitionNode();

            // If the source transition is to the new target state, we need to cancel the transition and use the cached pose
            StateNode* pSourceTransitionTargetState = pSourceTransitionNode->m_pTargetNode;
            if ( pSourceTransitionTargetState == pTargetStateNode )
            {
                EE_ASSERT( m_cachedPoseBufferID.IsValid() ); // Ensure we were caching a pose
                m_sourceType = SourceType::CachedPose;

                // We also need to explicitly shutdown the source transition target state as by default we dont shutdown target states when shutting down a transition
                pSourceTransitionTargetState->Shutdown( context );

                // Shutdown the source transition
                m_pSourceNode->Shutdown( context );
                m_pSourceNode = nullptr;

            }
            // If the source transition is to a future forceable state, we need to cache the result
            else if ( !m_cachedPoseBufferID.IsValid() )
            {
                if ( VectorContains( forceableFutureTargetStates, pSourceTransitionNode->m_pTargetNode ) )
                {
                    StartCachingSourcePose( context );
                }
            }
        }
        else if( IsSourceAState() )
        {
            if ( m_pSourceNode == pTargetStateNode )
            {
                EE_ASSERT( m_cachedPoseBufferID.IsValid() ); // Ensure we were caching a pose
                m_sourceType = SourceType::CachedPose;
                m_pSourceNode->Shutdown( context );
                m_pSourceNode = nullptr;
            }
            else // Check if we should start caching the source pose
            {
                if ( !m_cachedPoseBufferID.IsValid() )
                {
                    if ( VectorContains( forceableFutureTargetStates, GetSourceStateNode() ) )
                    {
                        StartCachingSourcePose( context );
                    }
                }
            }
        }
        else // Source is a cached pose
        {
            // Do Nothing
        }

        //-------------------------------------------------------------------------

        // If the source is still a transition node, notify it that we are starting a new transition
        if ( IsSourceATransition() )
        {
            auto pSourceTransitionNode = GetSourceTransitionNode();
            pSourceTransitionNode->NotifyNewTransitionStarting( context, pTargetStateNode, forceableFutureTargetStates );
        }
    }

    void TransitionNode::RegisterSourceCachePoseTask( GraphContext& context, GraphPoseNodeResult& sourceNodeResult )
    {
        if ( sourceNodeResult.HasRegisteredTasks() )
        {
            // If we should cache, register the write task here
            if ( m_cachedPoseBufferID.IsValid() )
            {
                sourceNodeResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::CachedPoseWriteTask>( GetNodeIndex(), sourceNodeResult.m_taskIdx, m_cachedPoseBufferID );
            }
        }
    }

    //-------------------------------------------------------------------------

    void TransitionNode::RegisterPoseTasksAndUpdateRootMotion( GraphContext& context, GraphPoseNodeResult const& sourceResult, GraphPoseNodeResult const& targetResult, GraphPoseNodeResult& outResult )
    {
        auto pDefinition = GetDefinition<TransitionNode>();

        if ( sourceResult.HasRegisteredTasks() && targetResult.HasRegisteredTasks() )
        {
            outResult.m_rootMotionDelta = Blender::BlendRootMotionDeltas( sourceResult.m_rootMotionDelta, targetResult.m_rootMotionDelta, m_blendWeight, pDefinition->m_rootMotionBlend );

            float poseBlendWeight = m_blendWeight;
            BoneMaskTaskList* pBoneMaskTaskList = nullptr;

            if ( m_pStartBoneMaskNode != nullptr )
            {
                EE_ASSERT( pDefinition->m_boneMaskBlendInTimePercentage > 0.0f && pDefinition->m_boneMaskBlendInTimePercentage <= 1.0f );

                // Blend weights
                //-------------------------------------------------------------------------

                float boneMaskBlendWeight = 0.0f;
                if ( m_transitionProgress >= pDefinition->m_boneMaskBlendInTimePercentage )
                {
                    poseBlendWeight = 1.0f;
                    boneMaskBlendWeight = ( m_transitionProgress - pDefinition->m_boneMaskBlendInTimePercentage ) / ( 1.0f - pDefinition->m_boneMaskBlendInTimePercentage );
                }
                else
                {
                    poseBlendWeight = m_transitionProgress / pDefinition->m_boneMaskBlendInTimePercentage;
                    boneMaskBlendWeight = 0.0f;
                }

                // Create bone mask task list
                //-------------------------------------------------------------------------

                m_boneMaskTaskList.Reset();
                auto pSourceBoneMaskTaskList = m_pStartBoneMaskNode->GetValue<BoneMaskTaskList const*>( context );
                EE_ASSERT( pSourceBoneMaskTaskList != nullptr );
                m_boneMaskTaskList.CopyFrom( *pSourceBoneMaskTaskList );
                int8_t sourceTaskIdx = m_boneMaskTaskList.GetLastTaskIdx();
                m_boneMaskTaskList.EmplaceTask( 1.0f );
                m_boneMaskTaskList.EmplaceTask( BoneMaskTask( sourceTaskIdx, sourceTaskIdx + 1, boneMaskBlendWeight ) );

                //-------------------------------------------------------------------------

                pBoneMaskTaskList = &m_boneMaskTaskList;
            }

            outResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::BlendTask>( GetNodeIndex(), sourceResult.m_taskIdx, targetResult.m_taskIdx, poseBlendWeight, pBoneMaskTaskList );

            //-------------------------------------------------------------------------

            #if EE_DEVELOPMENT_TOOLS
            context.GetRootMotionDebugger()->RecordBlend( GetNodeIndex(), m_rootMotionActionIdxSource, m_rootMotionActionIdxTarget, outResult.m_rootMotionDelta );
            #endif
        }
        else
        {
            if ( sourceResult.HasRegisteredTasks() )
            {
                outResult.m_taskIdx = sourceResult.m_taskIdx;
                outResult.m_rootMotionDelta = sourceResult.m_rootMotionDelta;
            }
            else // Keep target result
            {
                outResult.m_taskIdx = targetResult.m_taskIdx;
                outResult.m_rootMotionDelta = targetResult.m_rootMotionDelta;
            }
        }
    }

    void TransitionNode::UpdateLayerContext( GraphLayerContext* pSourceAndResultLayerContext, GraphLayerContext const* pTargetLayerContext )
    {
        EE_ASSERT( pSourceAndResultLayerContext != nullptr && pTargetLayerContext != nullptr );

        // Update layer weights
        //-------------------------------------------------------------------------

        pSourceAndResultLayerContext->m_layerWeight = Math::Lerp( pSourceAndResultLayerContext->m_layerWeight, pTargetLayerContext->m_layerWeight, m_blendWeight );
        pSourceAndResultLayerContext->m_rootMotionLayerWeight = Math::Lerp( pSourceAndResultLayerContext->m_rootMotionLayerWeight, pTargetLayerContext->m_rootMotionLayerWeight, m_blendWeight );

        // Update final bone mask
        //-------------------------------------------------------------------------

        if ( pSourceAndResultLayerContext->m_layerMaskTaskList.HasTasks() && pTargetLayerContext->m_layerMaskTaskList.HasTasks() )
        {
            pSourceAndResultLayerContext->m_layerMaskTaskList.BlendTo( pTargetLayerContext->m_layerMaskTaskList, m_blendWeight );
        }
        else // Only one bone mask is set
        {
            if ( pSourceAndResultLayerContext->m_layerMaskTaskList.HasTasks() )
            {
                // Keep the source mask from the source state while blending out
                if ( m_pTargetNode->IsOffState() )
                {
                    // Do nothing
                }
                else // Blend to no bone mask (all weights = 1.0f)
                {
                    pSourceAndResultLayerContext->m_layerMaskTaskList.BlendToGeneratedMask( 1.0f, m_blendWeight );
                }
            }
            else if ( pTargetLayerContext->m_layerMaskTaskList.HasTasks() )
            {
                // Keep the target bone mask on the whole way through the blend
                if ( IsSourceAState() && GetSourceStateNode()->IsOffState() )
                {
                    pSourceAndResultLayerContext->m_layerMaskTaskList = pTargetLayerContext->m_layerMaskTaskList;
                }
                else // Blend from no mask (From all weights = 1.0f)
                {
                    pSourceAndResultLayerContext->m_layerMaskTaskList = pTargetLayerContext->m_layerMaskTaskList;
                    pSourceAndResultLayerContext->m_layerMaskTaskList.BlendFromGeneratedMask( 1.0f, m_blendWeight );
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    GraphPoseNodeResult TransitionNode::Update( GraphContext& context, SyncTrackTimeRange const* pExternallySuppliedUpdateRange )
    {
        EE_ASSERT( WasInitialized() && !IsComplete( context ) );
        if ( !IsSourceACachedPose() )
        {
            EE_ASSERT( m_pSourceNode != nullptr && m_pSourceNode->WasInitialized() );
        }

        auto pDefinition = GetDefinition<TransitionNode>();

        MarkNodeActive( context );

        // Calculate update range and whether to sync or not
        //-------------------------------------------------------------------------
        
        bool shouldRunSyncUpdate = false;
        SyncTrackTimeRange updateRange;

        // If we have a supplied sync update range, that means the parent state machine is being driven via a sync update so just used the specified range directly
        if ( pExternallySuppliedUpdateRange != nullptr )
        {
            shouldRunSyncUpdate = true;
            updateRange = *pExternallySuppliedUpdateRange;
        }
        else // No parent sync time so calculate the desired update range
        {
            if ( !IsSourceACachedPose() && pDefinition->IsSynchronized() )
            {
                shouldRunSyncUpdate = true;

                // Calculate the update range for this frame
                Percentage const percentageTimeDelta = ( m_blendedDuration > 0.0f ) ? Percentage( context.m_deltaTime / m_blendedDuration ) : Percentage( 0.0f );
                Percentage const toTime = Percentage::Clamp( m_currentTime + percentageTimeDelta, true );
                updateRange.m_startTime = m_syncTrack.GetTime( m_currentTime );
                updateRange.m_endTime = m_syncTrack.GetTime( toTime );
            }
            else // Run unsynced update
            {
                shouldRunSyncUpdate = false;
            }
        }

        // Update transition progress
        //-------------------------------------------------------------------------

        EE_ASSERT( m_transitionLength > 0.0f );

        // Handle source transition completion
        if ( IsSourceATransition() && GetSourceTransitionNode()->IsComplete( context ) )
        {
            EndSourceTransition( context );
        }

        // Calculate the percentage complete over the clamped sync track range
        if ( shouldRunSyncUpdate && pDefinition->ShouldClampTransitionLength() )
        {
            float const eventDistance = m_syncTrack.CalculatePercentageCovered( updateRange );
            m_transitionProgress = m_transitionProgress + ( eventDistance / m_transitionLength );
        }
        else // Update the transition progress using the last frame time delta and store current time delta
        {
            m_transitionProgress = m_transitionProgress + Percentage( context.m_deltaTime / m_transitionLength );
        }

        m_transitionProgress = Math::Clamp( m_transitionProgress, 0.0f, 1.0f );

        // Calculate blend weight
        //-------------------------------------------------------------------------

        CalculateBlendWeight();

        // Update the source state
        //-------------------------------------------------------------------------

        GraphPoseNodeResult sourceNodeResult;

        if ( IsSourceACachedPose() )
        {
            sourceNodeResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::CachedPoseReadTask>( GetNodeIndex(), m_cachedPoseBufferID );
            sourceNodeResult.m_sampledEventRange = context.GetEmptySampledEventRange();

            #if EE_DEVELOPMENT_TOOLS
            m_rootMotionActionIdxSource = context.GetRootMotionDebugger()->RecordSampling( GetNodeIndex(), Transform::Identity );
            #endif
        }
        else // Source is a transition or state
        {
            // Set the branch state
            BranchState const previousBranchState = context.m_branchState;
            context.m_branchState = BranchState::Inactive;

            // Update source node
            if ( shouldRunSyncUpdate )
            {
                // Update range is for the target - so remove the transition sync event offset to calculate the source update range
                int32_t const syncEventOffset = int32_t( m_syncEventOffset );
                SyncTrackTimeRange sourceUpdateRange;
                sourceUpdateRange.m_startTime = SyncTrackTime( updateRange.m_startTime.m_eventIdx - syncEventOffset, updateRange.m_startTime.m_percentageThrough );
                sourceUpdateRange.m_endTime = SyncTrackTime( updateRange.m_endTime.m_eventIdx - syncEventOffset, updateRange.m_endTime.m_percentageThrough );

                // Ensure that we actually clamp the end time to the end of the source node
                if ( pDefinition->ShouldClampTransitionLength() && m_transitionProgress == 1.0f )
                {
                    sourceUpdateRange.m_endTime.m_eventIdx = sourceUpdateRange.m_startTime.m_eventIdx;
                    sourceUpdateRange.m_endTime.m_percentageThrough = 1.0f;
                }

                sourceNodeResult = m_pSourceNode->Update( context, &sourceUpdateRange );
            }
            else
            {
                sourceNodeResult = m_pSourceNode->Update( context, nullptr );
            }

            // Restore branch state
            context.m_branchState = previousBranchState;

            #if EE_DEVELOPMENT_TOOLS
            m_rootMotionActionIdxSource = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Cache source node pose
            RegisterSourceCachePoseTask( context, sourceNodeResult );
        }

        // Update the target state
        //-------------------------------------------------------------------------

        // Record source layer ctx and reset the layer ctx for target state
        GraphLayerContext targetLayerContext;
        GraphLayerContext* pSourceLayerCtx = nullptr;
        if ( context.IsInLayer() )
        {
            pSourceLayerCtx = context.m_pLayerContext;
            context.m_pLayerContext = &targetLayerContext;
        }

        // Update target state node
        GraphPoseNodeResult const targetNodeResult = m_pTargetNode->Update( context, shouldRunSyncUpdate ? &updateRange : nullptr );

        if ( context.IsInLayer() )
        {
            // Calculate the new layer weights based on the transition progress
            UpdateLayerContext( pSourceLayerCtx, &targetLayerContext );

            // Restore original context
            context.m_pLayerContext = pSourceLayerCtx;
        }

        #if EE_DEVELOPMENT_TOOLS
        m_rootMotionActionIdxTarget = context.GetRootMotionDebugger()->GetLastActionIndex();
        #endif

        // Register Blend tasks and update displacements
        //-------------------------------------------------------------------------

        GraphPoseNodeResult finalResult;
        RegisterPoseTasksAndUpdateRootMotion( context, sourceNodeResult, targetNodeResult, finalResult );

        // Update internal time and events
        //-------------------------------------------------------------------------

        if ( shouldRunSyncUpdate )
        {
            // Create the blended sync track
            SyncTrack const& sourceSyncTrack = m_pSourceNode->GetSyncTrack();
            SyncTrack const& targetSyncTrack = m_pTargetNode->GetSyncTrack();
            m_syncTrack = SyncTrack( sourceSyncTrack, targetSyncTrack, m_blendWeight );

            m_blendedDuration = SyncTrack::CalculateDurationSynchronized( m_pSourceNode->GetDuration(), m_pTargetNode->GetDuration(), sourceSyncTrack.GetNumEvents(), targetSyncTrack.GetNumEvents(), m_syncTrack.GetNumEvents(), m_blendWeight );
            m_previousTime = m_syncTrack.GetPercentageThrough( updateRange.m_startTime );
            m_currentTime = m_syncTrack.GetPercentageThrough( updateRange.m_endTime );
        }
        else
        {
            m_blendedDuration = Math::Lerp( IsSourceACachedPose() ? Seconds( 0.0f ) : m_pSourceNode->GetDuration(), m_pTargetNode->GetDuration(), m_blendWeight );
            if ( m_blendedDuration > 0.0f )
            {
                Percentage const deltaPercentage = Percentage( context.m_deltaTime / m_blendedDuration );
                m_previousTime = m_currentTime;
                m_currentTime = ( m_currentTime + deltaPercentage ).GetClamped( true );
            }
            else
            {
                m_previousTime = m_currentTime = 1.0f;
            }
        }

        // Set the exposed-duration of the transition to the target to ensure that any "state completed" nodes trigger correctly
        m_duration = m_pTargetNode->GetDuration();

        // Calculate the blended sampled event range
        finalResult.m_sampledEventRange = context.m_pSampledEventsBuffer->BlendEventRanges( sourceNodeResult.m_sampledEventRange, targetNodeResult.m_sampledEventRange, m_blendWeight );

        //-------------------------------------------------------------------------

        return finalResult;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void TransitionNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );

        outState.WriteValue( m_transitionProgress );
        outState.WriteValue( m_transitionLength );
        outState.WriteValue( m_syncEventOffset );
        outState.WriteValue( m_blendWeight );
        outState.WriteValue( m_cachedPoseBufferID );
        outState.WriteValue( m_sourceType );

        outState.WriteValue( m_pSourceNode->GetNodeIndex() );
    }

    void TransitionNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );

        inState.ReadValue( m_transitionProgress );
        inState.ReadValue( m_transitionLength );
        inState.ReadValue( m_syncEventOffset );
        inState.ReadValue( m_blendWeight );
        inState.ReadValue( m_cachedPoseBufferID );
        inState.ReadValue( m_sourceType );

        int16_t sourceNodeIdx = InvalidIndex;
        inState.ReadValue( sourceNodeIdx );
        m_pSourceNode = inState.GetNode<PoseNode>( sourceNodeIdx );
    }
    #endif
}