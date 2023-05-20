#include "Animation_RuntimeGraphNode_Transition.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_RootMotionDebugger.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_CachedPose.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Blend.h"
#include "Engine/Animation/AnimationBlender.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void TransitionNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<TransitionNode>( context, options );
        context.SetNodePtrFromIndex( m_targetStateNodeIdx, pNode->m_pTargetNode );
        context.SetOptionalNodePtrFromIndex( m_durationOverrideNodeIdx, pNode->m_pDurationOverrideNode );
        context.SetOptionalNodePtrFromIndex( m_syncEventOffsetOverrideNodeIdx, pNode->m_pEventOffsetOverrideNode );
    }

    GraphPoseNodeResult TransitionNode::StartTransitionFromState( GraphContext& context, GraphPoseNodeResult sourceNodeResult, StateNode* pSourceState, bool startCachingSourcePose )
    {
        EE_ASSERT( pSourceState != nullptr && m_pSourceNode == nullptr && IsInitialized() );

        // Starting a transition out may generate additional state events so we need to update the sampled event range
        sourceNodeResult.m_sampledEventRange = pSourceState->StartTransitionOut( context );
        m_pSourceNode = pSourceState;
        m_sourceType = SourceType::State;

        if ( startCachingSourcePose )
        {
            StartCachingSourcePose( context );
        }

        return InitializeTargetStateAndUpdateTransition( context, sourceNodeResult );
    }

    GraphPoseNodeResult TransitionNode::StartTransitionFromTransition( GraphContext& context, GraphPoseNodeResult sourceNodeResult, TransitionNode* pSourceTransition, bool startCachingSourcePose )
    {
        EE_ASSERT( pSourceTransition != nullptr && m_pSourceNode == nullptr && IsInitialized() );

        // Starting a transition out may generate additional state events so we need to update the sampled event range
        SampledEventRange const newTargetEventRange = pSourceTransition->m_pTargetNode->StartTransitionOut( context );
        sourceNodeResult.m_sampledEventRange.m_endIdx = newTargetEventRange.m_endIdx;
        m_pSourceNode = pSourceTransition;
        m_sourceType = SourceType::Transition;

        if( startCachingSourcePose )
        {
            StartCachingSourcePose( context );
        }

        return InitializeTargetStateAndUpdateTransition( context, sourceNodeResult );
    }

    GraphPoseNodeResult TransitionNode::InitializeTargetStateAndUpdateTransition( GraphContext& context, GraphPoseNodeResult sourceNodeResult )
    {
        MarkNodeActive( context );

        GraphPoseNodeResult result;

        // Record source root motion index
        #if EE_DEVELOPMENT_TOOLS
        m_rootMotionActionIdxSource = context.GetRootMotionDebugger()->GetLastActionIndex();
        #endif

        // Layer context update
        //-------------------------------------------------------------------------

        GraphLayerContext sourceLayerCtx;
        if ( context.IsInLayer() )
        {
            sourceLayerCtx = context.m_layerContext;
            context.m_layerContext.ResetLayer();
        }

        // Cache source node pose
        RegisterSourceCachePoseTask( context, sourceNodeResult );

        // Unsynchronized update
        //-------------------------------------------------------------------------

        GraphPoseNodeResult targetNodeResult;

        auto pSettings = GetSettings<TransitionNode>();
        if ( !pSettings->IsSynchronized() )
        {
            if ( m_pEventOffsetOverrideNode != nullptr )
            {
                m_pEventOffsetOverrideNode->Initialize( context );
                m_syncEventOffset = m_pEventOffsetOverrideNode->GetValue<float>( context );
                m_pEventOffsetOverrideNode->Shutdown( context );
            }
            else
            {
                m_syncEventOffset = pSettings->m_syncEventOffset;
            }

            // If we have a sync offset or we need to match the source state time, we need to create a target state initial time
            bool const shouldMatchSourceTime = pSettings->ShouldMatchSourceTime();
            if ( shouldMatchSourceTime || !Math::IsNearZero( m_syncEventOffset ) )
            {
                SyncTrackTime targetStartEventSyncTime;

                // Calculate the target start position (if we need to match the source)
                if ( shouldMatchSourceTime )
                {
                    SyncTrack const& sourceSyncTrack = m_pSourceNode->GetSyncTrack();
                    SyncTrackTime sourceFromSyncTime = sourceSyncTrack.GetTime( m_pSourceNode->GetCurrentTime() );

                    // Set the matching event index if required
                    if ( pSettings->ShouldMatchSyncEventIndex() )
                    {
                        targetStartEventSyncTime.m_eventIdx = sourceFromSyncTime.m_eventIdx;
                    }
                    else if ( pSettings->ShouldMatchSyncEventID() )
                    {
                        // TODO: check if this will become a performance headache - initialization/shutdown should be cheap!
                        // If it becomes a headache - initialize it here and then conditionally initialize later... Our init time and update time will not match so that might be a problem for some nodes, but this option should be rarely used
                        m_pTargetNode->Initialize( context, targetStartEventSyncTime );
                        SyncTrack const& targetSyncTrack = m_pTargetNode->GetSyncTrack();
                        StringID const sourceSyncEventID = sourceSyncTrack.GetEventID( sourceFromSyncTime.m_eventIdx );
                        targetStartEventSyncTime.m_eventIdx = targetSyncTrack.GetEventIndexForID( sourceSyncEventID );
                        m_pTargetNode->Shutdown( context );
                    }

                    // Should we keep the source "From" percentage through
                    if ( pSettings->ShouldMatchSyncEventPercentage() )
                    {
                        targetStartEventSyncTime.m_percentageThrough = sourceFromSyncTime.m_percentageThrough;
                    }
                }

                // Apply the sync event offset
                float eventIdxOffset;
                float percentageThroughOffset = Math::ModF( m_syncEventOffset, &eventIdxOffset );
                targetStartEventSyncTime.m_eventIdx += (int32_t) eventIdxOffset;

                // Ensure we handle looping past the current event with the percentage offset
                targetStartEventSyncTime.m_percentageThrough = Math::ModF( targetStartEventSyncTime.m_percentageThrough + percentageThroughOffset, &eventIdxOffset );
                targetStartEventSyncTime.m_eventIdx += (int32_t) eventIdxOffset;

                // Initialize and update the target node
                m_pTargetNode->Initialize( context, targetStartEventSyncTime );
                m_pTargetNode->StartTransitionIn( context );

                // Use a zero time-step as we dont want to update the target on this update but we do want the target pose to be created!
                float const oldDeltaTime = context.m_deltaTime;
                context.m_deltaTime = 0.0f;
                targetNodeResult = m_pTargetNode->Update( context );
                context.m_deltaTime = oldDeltaTime;
            }
            else // Regular time update (not matched or has sync offset)
            {
                m_pTargetNode->Initialize( context, SyncTrackTime() );
                m_pTargetNode->StartTransitionIn( context );
                targetNodeResult = m_pTargetNode->Update( context );
            }

            // Set time and duration for this node
            m_currentTime = 0.0f;
            m_duration = m_pTargetNode->GetDuration();
            EE_ASSERT( m_duration != 0.0f );

            #if EE_DEVELOPMENT_TOOLS
            m_rootMotionActionIdxTarget = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Should we clamp how long the transition is active for?
            if ( pSettings->ShouldClampTransitionLength() )
            {
                float const remainingNodeTime = ( 1.0f - m_pSourceNode->GetCurrentTime() ) * m_pSourceNode->GetDuration();
                m_transitionLength = Math::Min( m_transitionLength, remainingNodeTime );
            }
        }

        // Use sync events to initialize the target state
        //-------------------------------------------------------------------------

        else // Synchronized transition
        {
            EE_ASSERT( m_pSourceNode != nullptr );
            SyncTrack const& sourceSyncTrack = m_pSourceNode->GetSyncTrack();

            // Calculate the source update sync range
            SyncTrackTimeRange sourceUpdateRange;
            sourceUpdateRange.m_startTime = sourceSyncTrack.GetTime( m_pSourceNode->GetPreviousTime() );
            sourceUpdateRange.m_endTime = sourceSyncTrack.GetTime( m_pSourceNode->GetCurrentTime() );

            // Calculate the sync event offset for this transition
            // We dont care about the percentage since we are synchronized to the source
            if ( m_pEventOffsetOverrideNode != nullptr )
            {
                m_pEventOffsetOverrideNode->Initialize( context );
                m_syncEventOffset = Math::Floor( m_pEventOffsetOverrideNode->GetValue<float>( context ) );
                m_pEventOffsetOverrideNode->Shutdown( context );
            }
            else
            {
                m_syncEventOffset = Math::Floor( pSettings->m_syncEventOffset );
            }

            // Start transition and update the target state
            SyncTrackTimeRange targetUpdateRange = sourceUpdateRange;
            targetUpdateRange.m_startTime.m_eventIdx += int32_t( m_syncEventOffset );
            targetUpdateRange.m_endTime.m_eventIdx += int32_t( m_syncEventOffset );

            m_pTargetNode->Initialize( context, targetUpdateRange.m_startTime );
            m_pTargetNode->StartTransitionIn( context );
            targetNodeResult = m_pTargetNode->Update( context, targetUpdateRange );

            #if EE_DEVELOPMENT_TOOLS
            m_rootMotionActionIdxTarget = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Update internal transition state
            m_syncTrack = sourceSyncTrack;
            m_previousTime = m_syncTrack.GetPercentageThrough( targetUpdateRange.m_startTime );
            m_currentTime = m_syncTrack.GetPercentageThrough( targetUpdateRange.m_endTime );
            m_duration = m_pTargetNode->GetDuration();
            EE_ASSERT( m_duration != 0.0f );

            // Calculate transition duration
            if ( pSettings->ShouldClampTransitionLength() )
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
                Percentage const sourceTransitionDuration = Percentage( pSettings->m_duration / m_pSourceNode->GetDuration() );
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

        // Calculate the blend weight, register pose task and update layer weights
        //-------------------------------------------------------------------------

        CalculateBlendWeight();
        RegisterPoseTasksAndUpdateRootMotion( context, sourceNodeResult, targetNodeResult, result );
        UpdateLayerWeights( context, sourceLayerCtx, context.m_layerContext );

        // Update internal time and events
        //-------------------------------------------------------------------------

        result.m_sampledEventRange = context.m_sampledEventsBuffer.BlendEventRanges( sourceNodeResult.m_sampledEventRange, targetNodeResult.m_sampledEventRange, m_blendWeight );

        return result;
    }

    //-------------------------------------------------------------------------

    void TransitionNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        PoseNode::InitializeInternal( context, initialTime );
        auto pSettings = GetSettings<TransitionNode>();
        m_syncEventOffset = 0;
        m_boneMask = BoneMask( context.m_pSkeleton );

        // Reset transition duration and progress
        Seconds transitionDuration = pSettings->m_duration;
        if ( m_pDurationOverrideNode != nullptr )
        {
            m_pDurationOverrideNode->Initialize( context );
            m_transitionLength = Math::Clamp( m_pDurationOverrideNode->GetValue<float>( context ), 0.0f, 10.0f );
            m_pDurationOverrideNode->Shutdown( context );
        }
        else
        {
            m_transitionLength = pSettings->m_duration;
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

    void TransitionNode::UpdateProgress( GraphContext& context, bool isInitializing )
    {
        // Handle source transition completion
        // Only allowed if we are not initializing a transition - if we are initializing, this means the source node has been updated and may have tasks registered
        if ( !isInitializing )
        {
            if ( IsSourceATransition() && GetSourceTransitionNode()->IsComplete( context ) )
            {
                EndSourceTransition( context );
            }
        }

        // Update the transition progress using the last frame time delta and store current time delta
        EE_ASSERT( m_transitionLength > 0.0f );
        m_transitionProgress = m_transitionProgress + Percentage( context.m_deltaTime / m_transitionLength );
        m_transitionProgress = Math::Clamp( m_transitionProgress, 0.0f, 1.0f );
    }

    void TransitionNode::UpdateProgressClampedSynchronized( GraphContext& context, SyncTrackTimeRange const& updateRange, bool isInitializing )
    {
        auto pSettings = GetSettings<TransitionNode>();
        EE_ASSERT( pSettings->ShouldClampTransitionLength() );

        // Handle source transition completion
        // Only allowed if we are not initializing a transition - if we are initializing, this means the source node has been updated and may have tasks registered
        if ( !isInitializing )
        {
            if ( IsSourceATransition() && GetSourceTransitionNode()->IsComplete( context ) )
            {
                EndSourceTransition( context );
            }
        }

        // Calculate the percentage complete over the clamped sync track range
        float const eventDistance = m_syncTrack.CalculatePercentageCovered( updateRange );
        m_transitionProgress = m_transitionProgress + ( eventDistance / m_transitionLength );
        m_transitionProgress = Math::Clamp( m_transitionProgress, 0.0f, 1.0f );
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
        auto pSettings = GetSettings<TransitionNode>();

        if ( sourceResult.HasRegisteredTasks() && targetResult.HasRegisteredTasks() )
        {
            outResult.m_rootMotionDelta = Blender::BlendRootMotionDeltas( sourceResult.m_rootMotionDelta, targetResult.m_rootMotionDelta, m_blendWeight, pSettings->m_rootMotionBlend );
            outResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::BlendTask>( GetNodeIndex(), sourceResult.m_taskIdx, targetResult.m_taskIdx, m_blendWeight );

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

    void TransitionNode::UpdateLayerWeights( GraphContext& context, GraphLayerContext const& sourceLayerContext, GraphLayerContext const& targetLayerContext )
    {
        EE_ASSERT( context.IsValid() );

        // Early out if we are not in a layer
        if ( !context.IsInLayer() )
        {
            return;
        }

        // Update layer weight
        //-------------------------------------------------------------------------

        context.m_layerContext.m_layerWeight = Math::Lerp( sourceLayerContext.m_layerWeight, targetLayerContext.m_layerWeight, m_blendWeight );

        // Update final bone mask
        //-------------------------------------------------------------------------

        if ( sourceLayerContext.m_pLayerMask != nullptr && targetLayerContext.m_pLayerMask != nullptr )
        {
            context.m_layerContext.m_pLayerMask = targetLayerContext.m_pLayerMask;
            context.m_layerContext.m_pLayerMask->BlendFrom( *sourceLayerContext.m_pLayerMask, m_blendWeight );
        }
        else // Only one bone mask is set
        {
            if ( sourceLayerContext.m_pLayerMask != nullptr )
            {
                // Keep the source bone mask
                if ( m_pTargetNode->IsOffState() )
                {
                    context.m_layerContext.m_pLayerMask = sourceLayerContext.m_pLayerMask;
                }
                else // Blend to no bone mask
                {
                    context.m_layerContext.m_pLayerMask = context.m_boneMaskPool.GetBoneMask();
                    context.m_layerContext.m_pLayerMask->ResetWeights( 1.0f );
                    context.m_layerContext.m_pLayerMask->BlendFrom( *sourceLayerContext.m_pLayerMask, m_blendWeight );
                }
            }
            else if ( targetLayerContext.m_pLayerMask != nullptr )
            {
                // Keep the target bone mask if the source is an off state
                if ( IsSourceAState() && GetSourceStateNode()->IsOffState() )
                {
                    context.m_layerContext.m_pLayerMask = targetLayerContext.m_pLayerMask;
                }
                else
                {
                    context.m_layerContext.m_pLayerMask = context.m_boneMaskPool.GetBoneMask();
                    context.m_layerContext.m_pLayerMask->ResetWeights( 1.0f );
                    context.m_layerContext.m_pLayerMask->BlendTo( *targetLayerContext.m_pLayerMask, m_blendWeight );
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    GraphPoseNodeResult TransitionNode::Update( GraphContext& context )
    {
        EE_ASSERT( IsInitialized() && !IsComplete( context ) );
        auto pSettings = GetSettings<TransitionNode>();

        if ( !IsSourceACachedPose() && pSettings->IsSynchronized() )
        {
            MarkNodeActive( context );

            SyncTrack const& sourceSyncTrack = m_pSourceNode->GetSyncTrack();
            SyncTrack const& targetSyncTrack = m_pTargetNode->GetSyncTrack();

            SyncTrackTimeRange updateRange;
            updateRange.m_startTime = m_syncTrack.GetTime( m_currentTime );

            // Update transition progress
            if ( pSettings->ShouldClampTransitionLength() )
            {
                auto const percentageTimeDeltaOnOldDuration = Percentage( context.m_deltaTime / m_duration );
                auto const estimatedToTime = Percentage::Clamp( m_currentTime + percentageTimeDeltaOnOldDuration, true );
                updateRange.m_endTime = m_syncTrack.GetTime( estimatedToTime );
                UpdateProgressClampedSynchronized( context, updateRange );
            }
            else
            {
                UpdateProgress( context );
            }

            // Calculate the update range for this frame
            auto const percentageTimeDelta = Percentage( context.m_deltaTime / m_duration );
            auto const toTime = Percentage::Clamp( m_currentTime + percentageTimeDelta, true );
            updateRange.m_endTime = m_syncTrack.GetTime( toTime );

            // Calculate the blend weight and update the sync track
            CalculateBlendWeight();
            m_syncTrack = SyncTrack( sourceSyncTrack, targetSyncTrack, m_blendWeight );

            // Set the duration of the transition to the target to ensure that any "state completed" nodes trigger correctly
            m_duration = m_pTargetNode->GetDuration();
            EE_ASSERT( m_duration != 0.0f );

            // Update source and target nodes and update internal state
            return UpdateSynchronized( context, updateRange );
        }
        else // Unsynchronized update
        {
            MarkNodeActive( context );

            // Update transition progress
            UpdateProgress( context );

            // Calculate the blend weight and set the duration of the transition to the target to ensure that any "state completed" nodes trigger correctly
            CalculateBlendWeight();
            m_duration = m_pTargetNode->GetDuration();
            EE_ASSERT( m_duration != 0.0f );

            // Update source and target nodes and update internal state
            return UpdateUnsynchronized( context );
        }
    }

    GraphPoseNodeResult TransitionNode::UpdateUnsynchronized( GraphContext& context )
    {
        GraphPoseNodeResult result;

        // Update the source
        //-------------------------------------------------------------------------

        GraphPoseNodeResult sourceNodeResult;

        if ( IsSourceACachedPose() )
        {
            EE_ASSERT( m_cachedPoseBufferID.IsValid() );
            sourceNodeResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::CachedPoseReadTask>( GetNodeIndex(), m_cachedPoseBufferID );
            sourceNodeResult.m_sampledEventRange = context.GetEmptySampledEventRange();

            #if EE_DEVELOPMENT_TOOLS
            m_rootMotionActionIdxSource = context.GetRootMotionDebugger()->RecordSampling( GetNodeIndex(), Transform::Identity );
            #endif
        }
        else // Update the source state
        {
            // Set the branch state and update the source node
            BranchState const previousBranchState = context.m_branchState;
            context.m_branchState = BranchState::Inactive;
            sourceNodeResult = m_pSourceNode->Update( context );
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
        GraphLayerContext sourceLayerCtx;
        if ( context.IsInLayer() )
        {
            sourceLayerCtx = context.m_layerContext;
            context.m_layerContext.ResetLayer();
        }

        GraphPoseNodeResult const targetNodeResult = m_pTargetNode->Update( context );

        // Calculate the new layer weights based on the transition progress
        UpdateLayerWeights( context, sourceLayerCtx, context.m_layerContext );

        #if EE_DEVELOPMENT_TOOLS
        m_rootMotionActionIdxTarget = context.GetRootMotionDebugger()->GetLastActionIndex();
        #endif

        // Register Blend tasks and update displacements
        //-------------------------------------------------------------------------

        RegisterPoseTasksAndUpdateRootMotion( context, sourceNodeResult, targetNodeResult, result );

        // Update internal time and events
        //-------------------------------------------------------------------------

        m_previousTime = m_currentTime;
        m_currentTime = m_currentTime + Percentage( context.m_deltaTime / m_duration );
        result.m_sampledEventRange = context.m_sampledEventsBuffer.BlendEventRanges( sourceNodeResult.m_sampledEventRange, targetNodeResult.m_sampledEventRange, m_blendWeight );

        return result;
    }

    GraphPoseNodeResult TransitionNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        EE_ASSERT( IsInitialized() && m_pSourceNode != nullptr && m_pSourceNode->IsInitialized() && !IsComplete( context ) );
        auto pSettings = GetSettings<TransitionNode>();

        if ( !IsSourceACachedPose() && pSettings->IsSynchronized() )
        {
            MarkNodeActive( context );

            // Update transition progress
            if ( pSettings->ShouldClampTransitionLength() )
            {
                UpdateProgressClampedSynchronized( context, updateRange );
            }
            else
            {
                UpdateProgress( context );
            }

            // Update sync track and duration
            CalculateBlendWeight();
            SyncTrack const& sourceSyncTrack = m_pSourceNode->GetSyncTrack();
            SyncTrack const& targetSyncTrack = m_pTargetNode->GetSyncTrack();
            m_syncTrack = SyncTrack( sourceSyncTrack, targetSyncTrack, m_blendWeight );

            // Set the duration of the transition to the target to ensure that any "state completed" nodes trigger correctly
            m_duration = m_pTargetNode->GetDuration();
            EE_ASSERT( m_duration != 0.0f );

            // Update source and target nodes and update internal state
            return UpdateSynchronized( context, updateRange );
        }
        else
        {
            return Update( context );
        }
    }

    GraphPoseNodeResult TransitionNode::UpdateSynchronized( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        GraphPoseNodeResult result;

        auto pSettings = GetSettings<TransitionNode>();

        // For asynchronous transitions, terminate them immediately, this is an edge case and usually signifies a bad setup
        if ( !pSettings->IsSynchronized() )
        {
            m_transitionProgress = 1.0f;

            #if EE_DEVELOPMENT_TOOLS
            context.LogError( GetNodeIndex(), "Transition to state terminated due to synchronous update, this may indicate a bad graph setup!" );
            #endif
        }

        // Update source in a synchronous manner
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
        else // Update the source state
        {
            // Update range is for the target - so remove the transition sync event offset to calculate the source update range
            int32_t const syncEventOffset = int32_t( m_syncEventOffset );
            SyncTrackTimeRange sourceUpdateRange;
            sourceUpdateRange.m_startTime = SyncTrackTime( updateRange.m_startTime.m_eventIdx - syncEventOffset, updateRange.m_startTime.m_percentageThrough );
            sourceUpdateRange.m_endTime = SyncTrackTime( updateRange.m_endTime.m_eventIdx - syncEventOffset, updateRange.m_endTime.m_percentageThrough );

            // Ensure that we actually clamp the end time to the end of the source node
            if ( pSettings->ShouldClampTransitionLength() && m_transitionProgress == 1.0f )
            {
                sourceUpdateRange.m_endTime.m_eventIdx = sourceUpdateRange.m_startTime.m_eventIdx;
                sourceUpdateRange.m_endTime.m_percentageThrough = 1.0f;
            }

            // Set the branch state and update the source node
            BranchState const previousBranchState = context.m_branchState;
            context.m_branchState = BranchState::Inactive;
            sourceNodeResult = m_pSourceNode->Update( context, sourceUpdateRange );
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
        GraphLayerContext sourceLayerCtx;
        if ( context.IsInLayer() )
        {
            sourceLayerCtx = context.m_layerContext;
            context.m_layerContext.ResetLayer();
        }

        GraphPoseNodeResult const targetNodeResult = m_pTargetNode->Update( context, updateRange );

        // Calculate the new layer weights based on the transition progress
        UpdateLayerWeights( context, sourceLayerCtx, context.m_layerContext );

        #if EE_DEVELOPMENT_TOOLS
        m_rootMotionActionIdxTarget = context.GetRootMotionDebugger()->GetLastActionIndex();
        #endif

        // Register Blend tasks and update displacements
        //-------------------------------------------------------------------------

        RegisterPoseTasksAndUpdateRootMotion( context, sourceNodeResult, targetNodeResult, result );

        // Update internal time and events
        //-------------------------------------------------------------------------

        m_previousTime = m_syncTrack.GetPercentageThrough( updateRange.m_startTime );
        m_currentTime = m_syncTrack.GetPercentageThrough( updateRange.m_endTime );
        result.m_sampledEventRange = context.m_sampledEventsBuffer.BlendEventRanges( sourceNodeResult.m_sampledEventRange, targetNodeResult.m_sampledEventRange, m_blendWeight );

        return result;
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