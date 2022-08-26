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

    GraphPoseNodeResult TransitionNode::StartTransitionFromState( GraphContext& context, InitializationOptions const& options, StateNode* pSourceState )
    {
        EE_ASSERT( pSourceState != nullptr && m_pSourceNode == nullptr && !IsInitialized() );

        PoseNode::Initialize( context, SyncTrackTime() );
        pSourceState->StartTransitionOut( context );
        m_pSourceNode = pSourceState;
        m_sourceType = SourceType::State;

        return InitializeTargetStateAndUpdateTransition( context, options );
    }

    GraphPoseNodeResult TransitionNode::StartTransitionFromTransition( GraphContext& context, InitializationOptions const& options, TransitionNode* pSourceTransition, bool bForceTransition )
    {
        EE_ASSERT( pSourceTransition != nullptr && m_pSourceNode == nullptr && !IsInitialized() );

        auto pSettings = GetSettings<TransitionNode>();
        if ( bForceTransition )
        {
            EE_ASSERT( pSettings->IsForcedTransitionAllowed() && pSourceTransition->m_cachedPoseBufferID.IsValid() );

            // Transfer all cached pose IDs from the source transition and therefore the lifetime ownership of the cached buffers
            m_sourceCachedPoseBufferID = pSourceTransition->m_cachedPoseBufferID;
            pSourceTransition->m_cachedPoseBufferID.Clear();
            pSourceTransition->TransferAdditionalPoseBufferIDs( m_inheritedCachedPoseBufferIDs );

            // Force stop existing transition and start a transition from state
            auto pCurrentlyActiveState = pSourceTransition->m_pTargetNode;
            pSourceTransition->Shutdown( context );
            return StartTransitionFromState( context, options, pCurrentlyActiveState );
        }
        else
        {
            PoseNode::Initialize( context, SyncTrackTime() );
            pSourceTransition->m_pTargetNode->StartTransitionOut( context );
            m_pSourceNode = pSourceTransition;
            m_sourceType = SourceType::Transition;

            return InitializeTargetStateAndUpdateTransition( context, options );
        }
    }

    GraphPoseNodeResult TransitionNode::InitializeTargetStateAndUpdateTransition( GraphContext& context, InitializationOptions const& options )
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
        if ( context.m_layerContext.IsSet() )
        {
            sourceLayerCtx = context.m_layerContext;
            context.m_layerContext.BeginLayer();
        }

        // Unsynchronized update
        //-------------------------------------------------------------------------

        GraphPoseNodeResult targetNodeResult;

        auto pSettings = GetSettings<TransitionNode>();
        if ( !pSettings->IsSynchronized() )
        {
            m_currentTime = 0.0f;
            m_duration = Math::Lerp( m_pSourceNode->GetDuration(), m_pTargetNode->GetDuration(), m_transitionProgress );

            float floatSyncEventOffset = pSettings->m_syncEventOffset;
            if ( m_pEventOffsetOverrideNode != nullptr )
            {
                m_pEventOffsetOverrideNode->Initialize( context );
                floatSyncEventOffset = m_pEventOffsetOverrideNode->GetValue<float>( context );
                m_pEventOffsetOverrideNode->Shutdown( context );
            }

            m_syncEventOffset = Math::FloorToInt( floatSyncEventOffset );

            SyncTrackTime targetStartEventSyncTime;
            if ( pSettings->ShouldKeepEventIndex() || pSettings->ShouldKeepEventPercentage() || !Math::IsNearZero( floatSyncEventOffset ) )
            {
                // Calculate the target start position
                if ( pSettings->ShouldKeepEventIndex() || pSettings->ShouldKeepEventPercentage() )
                {
                    SyncTrack const& SourceSyncTrack = m_pSourceNode->GetSyncTrack();
                    SyncTrackTime sourceFromSyncTime = SourceSyncTrack.GetTime( m_pSourceNode->GetPreviousTime() );

                    // Should we keep the source event index
                    if ( pSettings->ShouldKeepEventIndex() )
                    {
                        targetStartEventSyncTime.m_eventIdx = sourceFromSyncTime.m_eventIdx;
                    }

                    // Should we keep the source "From" percentage through
                    if ( pSettings->ShouldKeepEventPercentage() )
                    {
                        targetStartEventSyncTime.m_percentageThrough = sourceFromSyncTime.m_percentageThrough;
                    }
                }

                // Apply the sync event offset
                float eventIdxOffset;
                float percentageThroughOffset = Math::ModF( floatSyncEventOffset, &eventIdxOffset );
                targetStartEventSyncTime.m_eventIdx += (int32_t) eventIdxOffset;

                // Ensure we handle looping past the current event with the percentage offset
                targetStartEventSyncTime.m_percentageThrough = Math::ModF( targetStartEventSyncTime.m_percentageThrough + percentageThroughOffset, &eventIdxOffset );
                targetStartEventSyncTime.m_eventIdx += (int32_t) eventIdxOffset;
            }

            // Initialize and update the target node
            m_pTargetNode->Initialize( context, targetStartEventSyncTime );
            m_pTargetNode->StartTransitionIn( context );
            targetNodeResult = m_pTargetNode->Update( context );

            #if EE_DEVELOPMENT_TOOLS
            m_rootMotionActionIdxTarget = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Clamp duration
            if ( pSettings->ShouldClampDuration() )
            {
                float const remainingNodeTime = ( 1.0f - m_pSourceNode->GetCurrentTime() ) * m_pSourceNode->GetDuration();
                m_transitionDuration = Math::Min( m_transitionDuration, remainingNodeTime );
            }

            UpdateProgress( context, true );
        }

        // Use sync events to initialize the target state
        //-------------------------------------------------------------------------

        else
        {
            EE_ASSERT( m_pSourceNode != nullptr );
            SyncTrack const& sourceSyncTrack = m_pSourceNode->GetSyncTrack();

            // Calculate the source update sync range
            SyncTrackTimeRange sourceUpdateRange;
            sourceUpdateRange.m_startTime = sourceSyncTrack.GetTime( m_pSourceNode->GetPreviousTime() );
            sourceUpdateRange.m_endTime = sourceSyncTrack.GetTime( m_pSourceNode->GetCurrentTime() );

            // Calculate the sync event offset for this transition
            // We dont care about the percentage since, we are synchronized to the source
            m_syncEventOffset = Math::FloorToInt( pSettings->m_syncEventOffset );
            if ( m_pEventOffsetOverrideNode != nullptr )
            {
                m_pEventOffsetOverrideNode->Initialize( context );
                m_syncEventOffset = Math::FloorToInt( m_pEventOffsetOverrideNode->GetValue<float>( context ) );
                m_pEventOffsetOverrideNode->Shutdown( context );
            }

            // Start transition and update the target state
            SyncTrackTimeRange targetUpdateRange = sourceUpdateRange;
            targetUpdateRange.m_startTime.m_eventIdx += m_syncEventOffset;
            targetUpdateRange.m_endTime.m_eventIdx += m_syncEventOffset;

            m_pTargetNode->Initialize( context, targetUpdateRange.m_startTime );
            m_pTargetNode->StartTransitionIn( context );
            targetNodeResult = m_pTargetNode->Update( context, targetUpdateRange );

            #if EE_DEVELOPMENT_TOOLS
            m_rootMotionActionIdxTarget = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Update internal transition state
            SyncTrack const& TargetSyncTrack = m_pTargetNode->GetSyncTrack();
            m_syncTrack = SyncTrack( sourceSyncTrack, TargetSyncTrack, m_transitionProgress );
            m_duration = SyncTrack::CalculateDurationSynchronized( m_pSourceNode->GetDuration(), m_pTargetNode->GetDuration(), sourceSyncTrack.GetNumEvents(), TargetSyncTrack.GetNumEvents(), m_syncTrack.GetNumEvents(), m_transitionProgress );
            m_previousTime = m_syncTrack.GetPercentageThrough( targetUpdateRange.m_startTime );
            m_currentTime = m_syncTrack.GetPercentageThrough( targetUpdateRange.m_endTime );

            // Calculate transition duration
            if ( pSettings->ShouldClampDuration() )
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
                m_transitionDuration = sourceSyncTrack.CalculatePercentageCovered( sourceUpdateRange.m_startTime, transitionEndSyncTime );
                UpdateProgressClampedSynchronized( context, sourceUpdateRange, true );
            }
            else // Transition duration is still time based
            {
                m_transitionDuration = pSettings->m_duration;
                UpdateProgress( context, true );
            }
        }

        // Record target ctx and reset ctx back to parent
        GraphLayerContext targetLayerCtx;
        if ( context.m_layerContext.IsSet() )
        {
            targetLayerCtx = context.m_layerContext;
            context.m_layerContext = sourceLayerCtx;
        }

        // Register Blend tasks and update displacements
        //-------------------------------------------------------------------------

        CalculateBlendWeight();
        RegisterPoseTasksAndUpdateRootMotion( context, options.m_sourceNodeResult, targetNodeResult, result );

        // Update internal time and events
        //-------------------------------------------------------------------------

        result.m_sampledEventRange = CombineAndUpdateEvents( context.m_sampledEventsBuffer, options.m_sourceNodeResult.m_sampledEventRange, targetNodeResult.m_sampledEventRange, m_blendWeight );
        UpdateLayerContext( context, sourceLayerCtx, targetLayerCtx );

        // Pose Caching
        //-------------------------------------------------------------------------

        if ( options.m_shouldCachePose )
        {
            EE_ASSERT( !m_cachedPoseBufferID.IsValid() );
            m_cachedPoseBufferID = context.m_pTaskSystem->CreateCachedPose();
            EE_ASSERT( m_cachedPoseBufferID.IsValid() );

            // If we have a valid task, cache it
            if ( result.HasRegisteredTasks() )
            {
                result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::CachedPoseWriteTask>( GetNodeIndex(), result.m_taskIdx, m_cachedPoseBufferID );
            }
        }

        return result;
    }

    void TransitionNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        PoseNode::InitializeInternal( context, initialTime );
        auto pSettings = GetSettings<TransitionNode>();
        m_syncEventOffset = Math::FloorToInt( pSettings->m_syncEventOffset );
        m_boneMask = BoneMask( context.m_pSkeleton );

        // Reset transition duration and progress
        Seconds transitionDuration = pSettings->m_duration;
        if ( m_pDurationOverrideNode != nullptr )
        {
            m_pDurationOverrideNode->Initialize( context );
            m_transitionDuration = Math::Clamp( m_pDurationOverrideNode->GetValue<float>( context ), 0.0f, 10.0f );
            m_pDurationOverrideNode->Shutdown( context );
        }
        else
        {
            m_transitionDuration = pSettings->m_duration;
        }

        m_transitionProgress = 0.0f;
        m_blendWeight = 0.0f;
        m_sourceCachedPoseBlendWeight = 0.0f;
        m_pivotOffset = Vector::Zero;
        m_shouldApplyPivotOffset = false;
        m_isFirstTaskRegistrationUpdate = true;
    }

    void TransitionNode::ShutdownInternal( GraphContext& context )
    {
        // Release cached pose buffers
        if ( m_cachedPoseBufferID.IsValid() )
        {
            context.m_pTaskSystem->DestroyCachedPose( m_cachedPoseBufferID );
            m_cachedPoseBufferID.Clear();
        }

        if ( m_sourceCachedPoseBufferID.IsValid() )
        {
            context.m_pTaskSystem->DestroyCachedPose( m_sourceCachedPoseBufferID );
            m_sourceCachedPoseBufferID.Clear();
        }

        for ( auto const& InheritedBufferID : m_inheritedCachedPoseBufferIDs )
        {
            context.m_pTaskSystem->DestroyCachedPose( InheritedBufferID );
        }
        m_inheritedCachedPoseBufferIDs.clear();

        // Clear transition flags from target
        m_pTargetNode->SetTransitioningState( StateNode::TransitionState::None );
        m_currentTime = 1.0f;

        if ( IsSourceATransition() )
        {
            EndSourceTransition( context );
        }

        m_pSourceNode->Shutdown( context );
        m_pSourceNode = nullptr;

        PoseNode::ShutdownInternal( context );
    }

    void TransitionNode::DeactivateBranch( GraphContext& context )
    {
        if ( IsValid() )
        {
            PoseNode::DeactivateBranch( context );
            m_pSourceNode->DeactivateBranch( context );
            m_pTargetNode->DeactivateBranch( context );
        }
    }

    void TransitionNode::EndSourceTransition( GraphContext& context )
    {
        EE_ASSERT( IsSourceATransition() && static_cast<TransitionNode*>( m_pSourceNode )->m_pTargetNode->IsInitialized() );

        auto SourceTargetState = static_cast<TransitionNode*>( m_pSourceNode )->m_pTargetNode;
        m_pSourceNode->Shutdown( context );
        m_pSourceNode = SourceTargetState;
        static_cast<StateNode*>( m_pSourceNode )->SetTransitioningState( StateNode::TransitionState::TransitioningOut );
        m_sourceType = SourceType::State;
    }

    //-------------------------------------------------------------------------

    void TransitionNode::UpdateProgress( GraphContext& context, bool isInitializing )
    {
        // Handle source transition completion
        // Only allowed if we are not initializing a transition - if we are initializing, this means the source node has been updated and may have tasks registered
        if ( !isInitializing )
        {
            if ( IsSourceATransition() && static_cast<TransitionNode*>( m_pSourceNode )->IsComplete() )
            {
                EndSourceTransition( context );
            }
        }

        // Update the transition progress using the last frame time delta and store current time delta
        if ( m_transitionDuration <= 0.0f )
        {
            m_transitionProgress = 1.0f;
        }
        else
        {
            m_transitionProgress = m_transitionProgress + Percentage( context.m_deltaTime / m_transitionDuration );
            m_transitionProgress = Math::Clamp( m_transitionProgress, 0.0f, 1.0f );
        }
    }

    void TransitionNode::UpdateProgressClampedSynchronized( GraphContext& context, SyncTrackTimeRange const& updateRange, bool isInitializing )
    {
        auto pSettings = GetSettings<TransitionNode>();
        EE_ASSERT( pSettings->ShouldClampDuration() );

        // Handle source transition completion
        // Only allowed if we are not initializing a transition - if we are initializing, this means the source node has been updated and may have tasks registered
        if ( !isInitializing )
        {
            if ( IsSourceATransition() && static_cast<TransitionNode*>( m_pSourceNode )->IsComplete() )
            {
                EndSourceTransition( context );
            }
        }

        // Calculate the percentage complete over the clamped sync track range
        float const eventDistance = m_syncTrack.CalculatePercentageCovered( updateRange );
        m_transitionProgress = m_transitionProgress + ( eventDistance / m_transitionDuration );
        m_transitionProgress = Math::Clamp( m_transitionProgress, 0.0f, 1.0f );
    }

    void TransitionNode::UpdateCachedPoseBufferIDState( GraphContext& context )
    {
        // Free them immediately as they were only useful on the same update as they were entered into the array
        for ( auto const& InheritedBufferID : m_inheritedCachedPoseBufferIDs )
        {
            context.m_pTaskSystem->DestroyCachedPose( InheritedBufferID );
        }
        m_inheritedCachedPoseBufferIDs.clear();

        //-------------------------------------------------------------------------

        static float const CachedPoseBlendTime = 0.1f; // ~3 Frames

        if ( m_sourceCachedPoseBufferID.IsValid() )
        {
            m_sourceCachedPoseBlendWeight = Math::Min( m_sourceCachedPoseBlendWeight + context.m_deltaTime / CachedPoseBlendTime, 1.0f );

            // If the blend is finished, release the cached pose
            if ( m_sourceCachedPoseBlendWeight >= 1.0f )
            {
                context.m_pTaskSystem->DestroyCachedPose( m_sourceCachedPoseBufferID );
                m_sourceCachedPoseBufferID.Clear();
            }
        }
    }

    void TransitionNode::TransferAdditionalPoseBufferIDs( TInlineVector<UUID, 2>& outInheritedCachedPoseBufferIDs )
    {
        // Transfer my cached buffer
        if ( m_cachedPoseBufferID.IsValid() )
        {
            outInheritedCachedPoseBufferIDs.emplace_back( m_cachedPoseBufferID );
            m_cachedPoseBufferID.Clear();
        }

        // Transfer my source cached pose buffer
        if ( m_sourceCachedPoseBufferID.IsValid() )
        {
            outInheritedCachedPoseBufferIDs.emplace_back( m_sourceCachedPoseBufferID );
            m_sourceCachedPoseBufferID.Clear();
        }

        // Transfer my inherited buffer IDS
        if ( m_inheritedCachedPoseBufferIDs.size() > 0 )
        {
            outInheritedCachedPoseBufferIDs.insert( outInheritedCachedPoseBufferIDs.end(), m_inheritedCachedPoseBufferIDs.begin(), m_inheritedCachedPoseBufferIDs.end() );
            m_inheritedCachedPoseBufferIDs.clear();
        }

        // Collect any additional pose buffer IDs
        if ( IsSourceATransition() )
        {
            auto pSourceTransition = static_cast<TransitionNode*>( m_pSourceNode );
            pSourceTransition->TransferAdditionalPoseBufferIDs( outInheritedCachedPoseBufferIDs );
        }

        #if EE_DEVELOPMENT_TOOLS
        for ( auto& ID : outInheritedCachedPoseBufferIDs )
        {
            EE_ASSERT( ID.IsValid() );
        }
        #endif
    }

    void TransitionNode::RegisterPoseTasksAndUpdateRootMotion( GraphContext& context, GraphPoseNodeResult const& sourceResult, GraphPoseNodeResult const& targetResult, GraphPoseNodeResult& outResult )
    {
        auto pSettings = GetSettings<TransitionNode>();

        if ( sourceResult.HasRegisteredTasks() && targetResult.HasRegisteredTasks() )
        {
            outResult.m_rootMotionDelta = Blender::BlendRootMotionDeltas( sourceResult.m_rootMotionDelta, targetResult.m_rootMotionDelta, m_blendWeight, pSettings->m_rootMotionBlend );

            if ( pSettings->m_blendPivotBoneID.IsValid() )
            {
                if ( m_shouldApplyPivotOffset )
                {
                    outResult.m_rootMotionDelta.AddTranslation( -m_pivotOffset );
                    m_shouldApplyPivotOffset = false;
                }

                outResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::PivotBlendTask>( GetNodeIndex(), sourceResult.m_taskIdx, targetResult.m_taskIdx, m_blendWeight, pSettings->m_blendPivotBoneID, &m_pivotOffset, m_isFirstTaskRegistrationUpdate );
                m_shouldApplyPivotOffset = m_isFirstTaskRegistrationUpdate;
            }
            else
            {
                outResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::BlendTask>( GetNodeIndex(), sourceResult.m_taskIdx, targetResult.m_taskIdx, m_blendWeight );
            }

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

        m_isFirstTaskRegistrationUpdate = false;
    }

    //-------------------------------------------------------------------------

    GraphPoseNodeResult TransitionNode::Update( GraphContext& context )
    {
        EE_ASSERT( IsInitialized() && m_pSourceNode != nullptr && m_pSourceNode->IsInitialized() && !IsComplete() );
        auto pSettings = GetSettings<TransitionNode>();

        UpdateCachedPoseBufferIDState( context );

        if ( pSettings->IsSynchronized() )
        {
            MarkNodeActive( context );

            SyncTrack const& sourceSyncTrack = m_pSourceNode->GetSyncTrack();
            SyncTrack const& targetSyncTrack = m_pTargetNode->GetSyncTrack();

            SyncTrackTimeRange updateRange;
            updateRange.m_startTime = m_syncTrack.GetTime( m_currentTime );

            // Update transition progress
            if ( pSettings->ShouldClampDuration() )
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

            // Update sync track and duration
            CalculateBlendWeight();
            m_syncTrack = SyncTrack( sourceSyncTrack, targetSyncTrack, m_blendWeight );
            m_duration = SyncTrack::CalculateDurationSynchronized( m_pSourceNode->GetDuration(), m_pTargetNode->GetDuration(), sourceSyncTrack.GetNumEvents(), targetSyncTrack.GetNumEvents(), m_syncTrack.GetNumEvents(), m_blendWeight );

            // Update source and target nodes and update internal state
            return UpdateSynchronized( context, updateRange );
        }
        else // Unsynchronized update
        {
            MarkNodeActive( context );

            // Update transition progress
            UpdateProgress( context );

            // Update sync track and duration
            CalculateBlendWeight();
            m_duration = Math::Lerp( m_pSourceNode->GetDuration(), m_pTargetNode->GetDuration(), m_blendWeight );

            // Update source and target nodes and update internal state
            return UpdateUnsynchronized( context );
        }
    }

    GraphPoseNodeResult TransitionNode::UpdateUnsynchronized( GraphContext& context )
    {
        GraphPoseNodeResult result;

        auto pSettings = GetSettings<TransitionNode>();

        // Layer context
        //-------------------------------------------------------------------------

        GraphLayerContext parentLayerCtx, sourceLayerCtx, targetLayerCtx;
        if ( context.m_layerContext.IsSet() )
        {
            parentLayerCtx = context.m_layerContext;
            context.m_layerContext.BeginLayer();
        }

        // Update the source
        //-------------------------------------------------------------------------

        // Register the source cached task if it exists
        TaskIndex const cachedSourceNodeTaskIdx = ( m_sourceCachedPoseBufferID.IsValid() ) ? context.m_pTaskSystem->RegisterTask<Tasks::CachedPoseReadTask>( GetNodeIndex(), m_sourceCachedPoseBufferID ) : InvalidIndex;

        // Set the branch state and update the source node
        BranchState const previousBranchState = context.m_branchState;
        context.m_branchState = BranchState::Inactive;
        GraphPoseNodeResult sourceNodeResult = m_pSourceNode->Update( context );

        #if EE_DEVELOPMENT_TOOLS
        m_rootMotionActionIdxSource = context.GetRootMotionDebugger()->GetLastActionIndex();
        #endif

        // If we have a source cached pose and we have registered tasks, register a blend
        if ( sourceNodeResult.HasRegisteredTasks() && cachedSourceNodeTaskIdx != InvalidIndex )
        {
            sourceNodeResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::BlendTask>( GetNodeIndex(), cachedSourceNodeTaskIdx, sourceNodeResult.m_taskIdx, m_sourceCachedPoseBlendWeight );
        }
        else
        {
            sourceNodeResult.m_taskIdx = ( cachedSourceNodeTaskIdx != InvalidIndex ) ? cachedSourceNodeTaskIdx : sourceNodeResult.m_taskIdx;
        }

        context.m_branchState = previousBranchState;

        // Record source ctx and reset ctx for target state
        if ( context.m_layerContext.IsSet() )
        {
            sourceLayerCtx = context.m_layerContext;
            context.m_layerContext.BeginLayer();
        }

        // Update the target
        //-------------------------------------------------------------------------

        GraphPoseNodeResult const targetNodeResult = m_pTargetNode->Update( context );

        #if EE_DEVELOPMENT_TOOLS
        m_rootMotionActionIdxTarget = context.GetRootMotionDebugger()->GetLastActionIndex();
        #endif

        // Record target ctx and reset ctx back to parent
        if ( context.m_layerContext.IsSet() )
        {
            targetLayerCtx = context.m_layerContext;
            context.m_layerContext = parentLayerCtx;
        }

        // Register Blend tasks and update displacements
        //-------------------------------------------------------------------------

        RegisterPoseTasksAndUpdateRootMotion( context, sourceNodeResult, targetNodeResult, result );

        // Update internal time and events
        //-------------------------------------------------------------------------

        auto const PercentageTimeDelta = Percentage( context.m_deltaTime / m_duration );
        m_previousTime = m_currentTime;
        m_currentTime = m_currentTime + PercentageTimeDelta;
        result.m_sampledEventRange = CombineAndUpdateEvents( context.m_sampledEventsBuffer, sourceNodeResult.m_sampledEventRange, targetNodeResult.m_sampledEventRange, m_blendWeight );
        UpdateLayerContext( context, sourceLayerCtx, targetLayerCtx );

        // Caching
        //-------------------------------------------------------------------------

        if ( result.HasRegisteredTasks() )
        {
            // If we should cache, register the write task here
            if ( m_cachedPoseBufferID.IsValid() )
            {
                result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::CachedPoseWriteTask>( GetNodeIndex(), result.m_taskIdx, m_cachedPoseBufferID );
            }
        }

        return result;
    }

    GraphPoseNodeResult TransitionNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        EE_ASSERT( IsInitialized() && m_pSourceNode != nullptr && m_pSourceNode->IsInitialized() && !IsComplete() );
        EE_ASSERT( ( IsSourceATransition() ? static_cast<TransitionNode*>( m_pSourceNode )->m_pTargetNode : static_cast<StateNode*>( m_pSourceNode ) ) != nullptr );
        auto pSettings = GetSettings<TransitionNode>();

        UpdateCachedPoseBufferIDState( context );

        if ( !pSettings->IsSynchronized() )
        {
            return Update( context );
        }
        else
        {
            MarkNodeActive( context );

            // Update transition progress
            if ( pSettings->ShouldClampDuration() )
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
            m_duration = SyncTrack::CalculateDurationSynchronized( m_pSourceNode->GetDuration(), m_pTargetNode->GetDuration(), sourceSyncTrack.GetNumEvents(), targetSyncTrack.GetNumEvents(), m_syncTrack.GetNumEvents(), m_blendWeight );

            // Update source and target nodes and update internal state
            return UpdateSynchronized( context, updateRange );
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
            EE_LOG_ERROR( "Animation", "TODO", "Transition to state terminated due to synchronous update, this may indicate a bad graph setup!" );
            #endif
        }

        // Update source state in a synchronous manner
        //-------------------------------------------------------------------------

        SyncTrack const& sourceSyncTrack = m_pSourceNode->GetSyncTrack();
        SyncTrack const& targetSyncTrack = m_pTargetNode->GetSyncTrack();

        // Update range is for the target - so remove the transition sync event offset to calculate the source update range
        SyncTrackTimeRange sourceUpdateRange;
        sourceUpdateRange.m_startTime = SyncTrackTime( updateRange.m_startTime.m_eventIdx - m_syncEventOffset, updateRange.m_startTime.m_percentageThrough );
        sourceUpdateRange.m_endTime = SyncTrackTime( updateRange.m_endTime.m_eventIdx - m_syncEventOffset, updateRange.m_endTime.m_percentageThrough );

        // Ensure that we actually clamp the end time to the end of the source node
        if ( pSettings->ShouldClampDuration() && m_transitionProgress == 1.0f )
        {
            sourceUpdateRange.m_endTime.m_eventIdx = sourceUpdateRange.m_startTime.m_eventIdx;
            sourceUpdateRange.m_endTime.m_percentageThrough = 1.0f;
        }

        // Register the source cached task if it exists
        TaskIndex const cachedSourceNodeTaskIdx = ( m_sourceCachedPoseBufferID.IsValid() ) ? context.m_pTaskSystem->RegisterTask<Tasks::CachedPoseReadTask>( GetNodeIndex(), m_sourceCachedPoseBufferID ) : InvalidIndex;

        // Set the branch state and update the source node
        BranchState const previousBranchState = context.m_branchState;
        context.m_branchState = BranchState::Inactive;
        GraphPoseNodeResult sourceNodeResult = m_pSourceNode->Update( context, sourceUpdateRange );

        #if EE_DEVELOPMENT_TOOLS
        m_rootMotionActionIdxSource = context.GetRootMotionDebugger()->GetLastActionIndex();
        #endif

        // If we have a source cached pose and we have registered tasks, register a blend
        if ( sourceNodeResult.HasRegisteredTasks() && cachedSourceNodeTaskIdx != InvalidIndex )
        {
            sourceNodeResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::BlendTask>( GetNodeIndex(), cachedSourceNodeTaskIdx, sourceNodeResult.m_taskIdx, m_sourceCachedPoseBlendWeight );
        }
        else
        {
            sourceNodeResult.m_taskIdx = ( cachedSourceNodeTaskIdx != InvalidIndex ) ? cachedSourceNodeTaskIdx : sourceNodeResult.m_taskIdx;
        }

        context.m_branchState = previousBranchState;

        // Record source ctx and reset ctx for target state
        GraphLayerContext sourceLayerCtx;
        if ( context.m_layerContext.IsSet() )
        {
            sourceLayerCtx = context.m_layerContext;
            context.m_layerContext.BeginLayer();
        }

        // Update the target
        //-------------------------------------------------------------------------

        GraphPoseNodeResult const targetNodeResult = m_pTargetNode->Update( context, updateRange );

        #if EE_DEVELOPMENT_TOOLS
        m_rootMotionActionIdxTarget = context.GetRootMotionDebugger()->GetLastActionIndex();
        #endif

        // Record target ctx and reset ctx back to parent
        GraphLayerContext targetLayerCtx;
        if ( context.m_layerContext.IsSet() )
        {
            targetLayerCtx = context.m_layerContext;
            context.m_layerContext = sourceLayerCtx;
        }

        // Register Blend tasks and update displacements
        //-------------------------------------------------------------------------

        RegisterPoseTasksAndUpdateRootMotion( context, sourceNodeResult, targetNodeResult, result );

        // Update internal time and events
        //-------------------------------------------------------------------------

        m_previousTime = m_syncTrack.GetPercentageThrough( updateRange.m_startTime );
        m_currentTime = m_syncTrack.GetPercentageThrough( updateRange.m_endTime );
        result.m_sampledEventRange = CombineAndUpdateEvents( context.m_sampledEventsBuffer, sourceNodeResult.m_sampledEventRange, targetNodeResult.m_sampledEventRange, m_blendWeight );
        UpdateLayerContext( context, sourceLayerCtx, targetLayerCtx );

        // Cache the pose if we have any registered tasks
        //-------------------------------------------------------------------------

        if ( result.HasRegisteredTasks() )
        {
            if ( m_cachedPoseBufferID.IsValid() )
            {
                result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::CachedPoseWriteTask>( GetNodeIndex(), result.m_taskIdx, m_cachedPoseBufferID );
            }
        }

        return result;
    }

    void TransitionNode::UpdateLayerContext( GraphContext& context, GraphLayerContext const& sourceLayerContext, GraphLayerContext const& targetLayerContext )
    {
        EE_ASSERT( context.IsValid() );

        // Early out if we are not in a layer
        if ( !context.m_layerContext.IsSet() )
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
                if ( IsSourceAState() && static_cast<StateNode*>( m_pSourceNode )->IsOffState() )
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
}