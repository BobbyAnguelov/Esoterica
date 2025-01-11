#include "Animation_RuntimeGraphNode_Layers.h"
#include "Animation_RuntimeGraphNode_StateMachine.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_LayerData.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_RootMotionDebugger.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Blend.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void LayerBlendNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<LayerBlendNode>( context, options );
        context.SetNodePtrFromIndex( m_baseNodeIdx, pNode->m_pBaseLayerNode );

        pNode->m_layers.reserve( m_layerDefinition.size() );
        for ( auto const& layerDefinition : m_layerDefinition )
        {
            Layer& pLayer = pNode->m_layers.emplace_back();
            context.SetNodePtrFromIndex( layerDefinition.m_inputNodeIdx, pLayer.m_pInputNode );
            context.SetOptionalNodePtrFromIndex( layerDefinition.m_weightValueNodeIdx, pLayer.m_pWeightValueNode );
            context.SetOptionalNodePtrFromIndex( layerDefinition.m_rootMotionWeightValueNodeIdx, pLayer.m_pRootMotionWeightValueNode );
            context.SetOptionalNodePtrFromIndex( layerDefinition.m_boneMaskValueNodeIdx, pLayer.m_pBoneMaskValueNode );
        }
    }

    void LayerBlendNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pBaseLayerNode != nullptr );
        PoseNode::InitializeInternal( context, initialTime );

        //-------------------------------------------------------------------------

        m_pBaseLayerNode->Initialize( context, initialTime );

        if ( m_pBaseLayerNode->IsValid() )
        {
            m_currentTime = m_pBaseLayerNode->GetCurrentTime();
            m_duration = m_pBaseLayerNode->GetDuration();
        }
        else
        {
            m_previousTime = m_currentTime = 0.0f;
            m_duration = 0;
        }

        //-------------------------------------------------------------------------

        // Check if we have layer initialization data for this node
        GraphLayerUpdateState const* pLayerInitializationData = nullptr;
        if ( context.m_pLayerInitializationInfo != nullptr )
        {
            for ( auto const& li : *context.m_pLayerInitializationInfo )
            {
                if ( li.m_nodeIdx == GetNodeIndex() )
                {
                    pLayerInitializationData = &li;
                    break;
                }
            }
        }

        //-------------------------------------------------------------------------

        auto pDefinition = GetDefinition<LayerBlendNode>();
        int32_t const numLayers = (int32_t) m_layers.size();
        for ( auto i = 0; i < numLayers; i++ )
        {
            EE_ASSERT( m_layers[i].m_pInputNode != nullptr );

            // Initialize input node
            // Only initialize the start time for synchronized layers
            if ( pDefinition->m_layerDefinition[i].m_isSynchronized )
            {
                m_layers[i].m_pInputNode->Initialize( context, initialTime );
            }
            else // If we have initialization data initialize the layer to the specified time, otherwise just initialize it to the start
            {
                SyncTrackTime layerInitTime;

                if( pLayerInitializationData != nullptr )
                {
                    for ( auto const& layerInfo : pLayerInitializationData->m_updateRanges )
                    {
                        if ( layerInfo.m_layerIdx == i )
                        {
                            layerInitTime = layerInfo.m_syncTimeRange.m_startTime;
                            break;
                        }
                    }
                }

                m_layers[i].m_pInputNode->Initialize( context, layerInitTime );
            }

            // Optional Nodes
            if ( m_layers[i].m_pWeightValueNode != nullptr )
            {
                m_layers[i].m_pWeightValueNode->Initialize( context );
            }

            if ( m_layers[i].m_pBoneMaskValueNode != nullptr )
            {
                m_layers[i].m_pBoneMaskValueNode->Initialize( context );
            }
        }
    }

    void LayerBlendNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        for ( auto& layer : m_layers )
        {
            // Optional Nodes
            if ( layer.m_pWeightValueNode != nullptr )
            {
                layer.m_pWeightValueNode->Shutdown( context );
            }

            if ( layer.m_pBoneMaskValueNode != nullptr )
            {
                layer.m_pBoneMaskValueNode->Shutdown( context );
            }

            // Input Node
            EE_ASSERT( layer.m_pInputNode != nullptr );
            layer.m_pInputNode->Shutdown( context );
        }

        m_pBaseLayerNode->Shutdown( context );
        PoseNode::ShutdownInternal( context );
    }

    // NB: Layered nodes always update the base according to the specified update time delta or time range. The layers are then updated relative to the base.
    GraphPoseNodeResult LayerBlendNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        EE_ASSERT( context.IsValid() );

        GraphPoseNodeResult result;

        if ( IsValid() )
        {
            // Update the base
            //-------------------------------------------------------------------------

            MarkNodeActive( context );

            if ( pUpdateRange != nullptr )
            {
                result = m_pBaseLayerNode->Update( context, pUpdateRange );
                m_previousTime = GetSyncTrack().GetPercentageThrough( pUpdateRange->m_startTime );
                m_currentTime = m_pBaseLayerNode->GetCurrentTime();
            }
            else // Unsynchronized
            {
                m_previousTime = m_pBaseLayerNode->GetCurrentTime();
                result = m_pBaseLayerNode->Update( context );
                m_currentTime = m_pBaseLayerNode->GetCurrentTime();
            }

            m_duration = m_pBaseLayerNode->GetDuration();

            #if EE_DEVELOPMENT_TOOLS
            m_rootMotionActionIdxBase = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Update the layers
            //-------------------------------------------------------------------------

            // We need to register a task at the base layer in all cases - since we blend the layers tasks on top of it
            if ( !result.HasRegisteredTasks() )
            {
                result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::ReferencePoseTask>( GetNodeIndex() );
            }

            UpdateLayers( context, result );
        }

        return result;
    }

    //-------------------------------------------------------------------------

    void LayerBlendNode::UpdateLayers( GraphContext& context, GraphPoseNodeResult& nodeResult )
    {
        EE_ASSERT( context.IsValid() && IsValid() );
        auto pDefinition = GetDefinition<LayerBlendNode>();

        SyncTrackTime const baseStartTime = m_pBaseLayerNode->GetSyncTrack().GetTime( m_pBaseLayerNode->GetPreviousTime() );
        SyncTrackTime const baseEndTime = m_pBaseLayerNode->GetSyncTrack().GetTime( m_pBaseLayerNode->GetCurrentTime() );
        SyncTrackTimeRange layerUpdateRange( baseStartTime, baseEndTime );

        #if EE_DEVELOPMENT_TOOLS
        int16_t rootMotionActionIdxCurrentBase = m_rootMotionActionIdxBase;
        int16_t rootMotionActionIdxLayer = rootMotionActionIdxCurrentBase;
        #endif

        // If we are currently in a higher-level layer then cache it so that we can safely overwrite it
        //-------------------------------------------------------------------------

        if ( context.IsInLayer() )
        {
            m_pPreviousContext = context.m_pLayerContext;
        }

        // Handle layers
        //-------------------------------------------------------------------------

        int32_t const numLayers = (int32_t) m_layers.size();
        for ( auto i = 0; i < numLayers; i++ )
        {
            // Start a new layer
            //-------------------------------------------------------------------------

            GraphLayerContext layerContext;
            context.m_pLayerContext = &layerContext;

            // Update layer
            //-------------------------------------------------------------------------

            // Record the current state of the registered tasks
            auto const taskMarker = context.m_pTaskSystem->GetCurrentTaskIndexMarker();

            // Record the current state of the root motion debugger
            #if EE_DEVELOPMENT_TOOLS
            auto pRootMotionDebugger = context.GetRootMotionDebugger();
            auto const rootMotionActionMarker = pRootMotionDebugger->GetCurrentActionIndexMarker();
            #endif

            // If we're not a state machine setup the layer context here
            if ( !pDefinition->m_layerDefinition[i].m_isStateMachineLayer )
            {
                // Layer Weight
                if ( m_layers[i].m_pWeightValueNode != nullptr )
                {
                    context.m_pLayerContext->m_layerWeight = m_layers[i].m_pWeightValueNode->GetValue<float>( context );
                }

                // Root Motion
                if ( m_layers[i].m_pRootMotionWeightValueNode != nullptr )
                {
                    context.m_pLayerContext->m_rootMotionLayerWeight = m_layers[i].m_pRootMotionWeightValueNode->GetValue<float>( context );
                }

                // Bone Mask
                if ( m_layers[i].m_pBoneMaskValueNode != nullptr )
                {
                    auto pBoneMaskTaskList = m_layers[i].m_pBoneMaskValueNode->GetValue<BoneMaskTaskList const*>( context );
                    if ( pBoneMaskTaskList != nullptr )
                    {
                        context.m_pLayerContext->m_layerMaskTaskList.CopyFrom( *pBoneMaskTaskList );
                    }
                }
            }

            // Update input node
            // Always update SM layers as the transitions need to be evaluated
            // SM layers will calculate the final layer weight and store it in the layer context
            GraphPoseNodeResult layerResult;
            if ( pDefinition->m_layerDefinition[i].m_isStateMachineLayer || context.m_pLayerContext->m_layerWeight > 0.0f )
            {
                if ( pDefinition->m_layerDefinition[i].m_isSynchronized )
                {
                    layerResult = m_layers[i].m_pInputNode->Update( context, &layerUpdateRange );
                }
                else
                {
                    layerResult = m_layers[i].m_pInputNode->Update( context );
                }
            }

            // We're done with the layer weight calculation at this stage
            m_layers[i].m_weight = context.m_pLayerContext->m_layerWeight;

            // Register the blend tasks
            // If we registered a task for this layer, then we need to blend
            if ( layerResult.HasRegisteredTasks() )
            {
                // Create a blend task if the layer is enabled
                if ( m_layers[i].m_weight > 0 )
                {
                    PoseBlendMode poseBlendMode = pDefinition->m_layerDefinition[i].m_blendMode;

                    // We cannot perform a global blend without a bone mask
                    if ( poseBlendMode == PoseBlendMode::ModelSpace && !context.m_pLayerContext->m_layerMaskTaskList.HasTasks() )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        context.LogWarning( GetNodeIndex(), "Attempting to perform a global blend without a bone mask! This is not supported so falling back to a local blend!" );
                        #endif

                        poseBlendMode = PoseBlendMode::Overlay;
                    }

                    // Register blend tasks
                    switch ( poseBlendMode )
                    {
                        case PoseBlendMode::Overlay:
                        {
                            nodeResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::OverlayBlendTask>( GetNodeIndex(), nodeResult.m_taskIdx, layerResult.m_taskIdx, m_layers[i].m_weight, &context.m_pLayerContext->m_layerMaskTaskList );
                        }
                        break;

                        case PoseBlendMode::Additive:
                        {
                            nodeResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::AdditiveBlendTask>( GetNodeIndex(), nodeResult.m_taskIdx, layerResult.m_taskIdx, m_layers[i].m_weight, &context.m_pLayerContext->m_layerMaskTaskList );
                        }
                        break;

                        case PoseBlendMode::ModelSpace:
                        {
                            nodeResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::GlobalBlendTask>( GetNodeIndex(), nodeResult.m_taskIdx, layerResult.m_taskIdx, m_layers[i].m_weight, context.m_pLayerContext->m_layerMaskTaskList );
                        }
                        break;
                    }

                    // Blend root motion
                    if ( !pDefinition->m_onlySampleBaseRootMotion )
                    {
                        RootMotionBlendMode const rootMotionBlendMode = ( poseBlendMode == PoseBlendMode::Additive ) ? RootMotionBlendMode::Additive : RootMotionBlendMode::Blend;
                        float const rootMotionWeight = m_layers[i].m_weight * context.m_pLayerContext->m_rootMotionLayerWeight;
                        nodeResult.m_rootMotionDelta = Blender::BlendRootMotionDeltas( nodeResult.m_rootMotionDelta, layerResult.m_rootMotionDelta, rootMotionWeight, rootMotionBlendMode );

                        #if EE_DEVELOPMENT_TOOLS
                        rootMotionActionIdxLayer = pRootMotionDebugger->GetLastActionIndex();
                        pRootMotionDebugger->RecordBlend( GetNodeIndex(), rootMotionActionIdxCurrentBase, rootMotionActionIdxLayer, nodeResult.m_rootMotionDelta );
                        rootMotionActionIdxCurrentBase = pRootMotionDebugger->GetLastActionIndex();
                        #endif
                    }
                    else
                    {
                        // Remove any root motion debug records
                        #if EE_DEVELOPMENT_TOOLS
                        pRootMotionDebugger->RollbackToActionIndexMarker( rootMotionActionMarker );
                        #endif
                    }
                }
                else // Layer is off
                {
                    // Flag all events as ignored
                    context.m_pSampledEventsBuffer->MarkEventsAsIgnored( layerResult.m_sampledEventRange );

                    // Remove any registered pose tasks
                    EE_ASSERT( pDefinition->m_layerDefinition[i].m_isStateMachineLayer ); // Cannot occur for local layers
                    context.m_pTaskSystem->RollbackToTaskIndexMarker( taskMarker );

                    // Remove any root motion debug records
                    #if EE_DEVELOPMENT_TOOLS
                    pRootMotionDebugger->RollbackToActionIndexMarker( rootMotionActionMarker );
                    #endif
                }
            }

            // Update events for the Layer
            //-------------------------------------------------------------------------

            SampledEventRange const& layerEventRange = layerResult.m_sampledEventRange;
            int32_t const numLayerEvents = layerEventRange.GetLength();
            if ( numLayerEvents > 0 )
            {
                // Update events
                context.m_pSampledEventsBuffer->UpdateWeights( layerEventRange, m_layers[i].m_weight );

                // Mark events as ignored if requested
                if ( pDefinition->m_layerDefinition[i].m_ignoreEvents )
                {
                    context.m_pSampledEventsBuffer->MarkEventsAsIgnored( layerEventRange );
                }

                // Merge layer sampled event into the layer's nodes range
                int32_t const numCurrentEvents = nodeResult.m_sampledEventRange.GetLength();
                if ( numCurrentEvents > 0 )
                {
                    // Combine sampled event range - the current range must always be before the layer's range
                    EE_ASSERT( nodeResult.m_sampledEventRange.m_endIdx <= layerEventRange.m_startIdx );
                    nodeResult.m_sampledEventRange.m_endIdx = layerEventRange.m_endIdx;
                }
                else
                {
                    nodeResult.m_sampledEventRange = layerEventRange;
                }
            }

            // End Layer
            //-------------------------------------------------------------------------

            context.m_pLayerContext = nullptr;
        }

        // Restore previous context if it existed
        //-------------------------------------------------------------------------

        if ( m_pPreviousContext )
        {
            context.m_pLayerContext = m_pPreviousContext;
        }
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void LayerBlendNode::RecordGraphState( RecordedGraphState& outState )
    {
        PoseNode::RecordGraphState( outState );
    }

    void LayerBlendNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );
    }

    void LayerBlendNode::GetSyncUpdateRangesForUnsynchronizedLayers( TInlineVector<GraphLayerSyncInfo, 5>& outLayerSyncInfos ) const
    {
        auto pDefinition = GetDefinition<LayerBlendNode>();
        int8_t const numLayers = (int8_t) m_layers.size();
        for ( int8_t i = 0; i < numLayers; i++ )
        {
            if ( m_layers[i].m_weight > 0.0f && !pDefinition->m_layerDefinition[i].m_isSynchronized )
            {
                auto const& layerSyncTrack = m_layers[i].m_pInputNode->GetSyncTrack();
                Percentage const prevTime = m_layers[i].m_pInputNode->GetPreviousTime();
                Percentage const currentTime = m_layers[i].m_pInputNode->GetCurrentTime();
                if ( prevTime != 0.0f && currentTime != 0.0f )
                {
                    outLayerSyncInfos.emplace_back( i, SyncTrackTimeRange( layerSyncTrack.GetTime( prevTime ), layerSyncTrack.GetTime( currentTime ) ) );
                }
            }
        }
    }
    #endif
}