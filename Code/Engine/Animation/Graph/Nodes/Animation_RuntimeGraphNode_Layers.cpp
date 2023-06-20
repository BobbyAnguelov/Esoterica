#include "Animation_RuntimeGraphNode_Layers.h"
#include "Animation_RuntimeGraphNode_StateMachine.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_RootMotionDebugger.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Blend.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void LayerBlendNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<LayerBlendNode>( context, options );
        context.SetNodePtrFromIndex( m_baseNodeIdx, pNode->m_pBaseLayerNode );

        pNode->m_layers.reserve( m_layerSettings.size() );
        for ( auto const& layerSettings : m_layerSettings )
        {
            Layer& pLayer = pNode->m_layers.emplace_back();
            context.SetNodePtrFromIndex( layerSettings.m_inputNodeIdx, pLayer.m_pInputNode );
            context.SetOptionalNodePtrFromIndex( layerSettings.m_weightValueNodeIdx, pLayer.m_pWeightValueNode );
            context.SetOptionalNodePtrFromIndex( layerSettings.m_rootMotionWeightValueNodeIdx, pLayer.m_pRootMotionWeightValueNode );
            context.SetOptionalNodePtrFromIndex( layerSettings.m_boneMaskValueNodeIdx, pLayer.m_pBoneMaskValueNode );
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
            m_duration = s_oneFrameDuration;
        }

        //-------------------------------------------------------------------------

        auto pSettings = GetSettings<LayerBlendNode>();
        int32_t const numLayers = (int32_t) m_layers.size();
        for ( auto i = 0; i < numLayers; i++ )
        {
            EE_ASSERT( m_layers[i].m_pInputNode != nullptr );

            // Initialize input node
            // Only initialize the start time for synchronized layers
            if ( pSettings->m_layerSettings[i].m_isSynchronized )
            {
                m_layers[i].m_pInputNode->Initialize( context, initialTime );
            }
            else
            {
                m_layers[i].m_pInputNode->Initialize( context, SyncTrackTime() );
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

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_debugLayerWeights.resize( numLayers );
        #endif
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
    GraphPoseNodeResult LayerBlendNode::Update( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        GraphPoseNodeResult result;

        if ( IsValid() )
        {
            // Update the base
            //-------------------------------------------------------------------------

            MarkNodeActive( context );
            m_previousTime = m_pBaseLayerNode->GetCurrentTime();
            result = m_pBaseLayerNode->Update( context );
            m_currentTime = m_pBaseLayerNode->GetCurrentTime();
            m_duration = m_pBaseLayerNode->GetDuration();

            #if EE_DEVELOPMENT_TOOLS
            m_rootMotionActionIdxBase = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Update the layers
            //-------------------------------------------------------------------------

            // We need to register a task at the base layer in all cases - since we blend the layers tasks on top of it
            if ( !result.HasRegisteredTasks() )
            {
                result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
            }

            UpdateLayers( context, result );
        }

        return result;
    }

    // NB: Layered nodes always update the base according to the specified update time delta or time range. The layers are then updated relative to the base.
    GraphPoseNodeResult LayerBlendNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        EE_ASSERT( context.IsValid() );

        GraphPoseNodeResult result;

        if ( IsValid() )
        {
            // Update the base
            //-------------------------------------------------------------------------

            MarkNodeActive( context );
            result = m_pBaseLayerNode->Update( context, updateRange );
            m_previousTime = GetSyncTrack().GetPercentageThrough( updateRange.m_startTime );
            m_currentTime = m_pBaseLayerNode->GetCurrentTime();
            m_duration = m_pBaseLayerNode->GetDuration();

            #if EE_DEVELOPMENT_TOOLS
            m_rootMotionActionIdxBase = context.GetRootMotionDebugger()->GetLastActionIndex();
            #endif

            // Update the layers
            //-------------------------------------------------------------------------

            // We need to register a task at the base layer in all cases - since we blend the layers tasks on top of it
            if ( !result.HasRegisteredTasks() )
            {
                result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
            }

            UpdateLayers( context, result );
        }

        return result;
    }

    //-------------------------------------------------------------------------

    void LayerBlendNode::UpdateLayers( GraphContext& context, GraphPoseNodeResult& nodeResult )
    {
        EE_ASSERT( context.IsValid() && IsValid() );
        auto pSettings = GetSettings<LayerBlendNode>();

        SyncTrackTime const baseStartTime = m_pBaseLayerNode->GetSyncTrack().GetTime( m_pBaseLayerNode->GetPreviousTime() );
        SyncTrackTime const baseEndTime = m_pBaseLayerNode->GetSyncTrack().GetTime( m_pBaseLayerNode->GetCurrentTime() );
        SyncTrackTimeRange layerUpdateRange( baseStartTime, baseEndTime );

        #if EE_DEVELOPMENT_TOOLS
        int16_t rootMotionActionIdxCurrentBase = m_rootMotionActionIdxBase;
        int16_t rootMotionActionIdxLayer = rootMotionActionIdxCurrentBase;
        #endif

        int32_t const numLayers = (int32_t) m_layers.size();
        for ( auto i = 0; i < numLayers; i++ )
        {
            // Start a new layer
            //-------------------------------------------------------------------------

            // If we are currently in a higher-level layer then cache it so that we can safely overwrite it
            if ( context.IsInLayer() )
            {
                m_previousContext = context.m_layerContext;
            }

            context.m_layerContext.BeginLayer();

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
            if ( !pSettings->m_layerSettings[i].m_isStateMachineLayer )
            {
                // Layer Weight
                if ( m_layers[i].m_pWeightValueNode != nullptr )
                {
                    context.m_layerContext.m_layerWeight = m_layers[i].m_pWeightValueNode->GetValue<float>( context );
                }

                // Root Motion
                if ( m_layers[i].m_pRootMotionWeightValueNode != nullptr )
                {
                    context.m_layerContext.m_rootMotionLayerWeight = m_layers[i].m_pRootMotionWeightValueNode->GetValue<float>( context );
                }

                // Bone Mask
                if ( m_layers[i].m_pBoneMaskValueNode != nullptr )
                {
                    auto pBoneMaskTaskList = m_layers[i].m_pBoneMaskValueNode->GetValue<BoneMaskTaskList const*>( context );
                    if ( pBoneMaskTaskList != nullptr )
                    {
                        context.m_layerContext.m_layerMaskTaskList.CopyFrom( *pBoneMaskTaskList );
                    }
                }
            }

            // Update input node
            // Always update SM layers as the transitions need to be evaluated
            GraphPoseNodeResult layerResult;
            if ( pSettings->m_layerSettings[i].m_isStateMachineLayer || context.m_layerContext.m_layerWeight > 0.0f )
            {
                if ( pSettings->m_layerSettings[i].m_isSynchronized )
                {
                    layerResult = m_layers[i].m_pInputNode->Update( context, layerUpdateRange );
                }
                else
                {
                    layerResult = m_layers[i].m_pInputNode->Update( context );
                }
            }

            // Register the blend tasks
            // If we registered a task for this layer, then we need to blend
            if ( layerResult.HasRegisteredTasks() )
            {
                // Create a blend task if the layer is enabled
                if ( context.m_layerContext.m_layerWeight > 0 )
                {
                    PoseBlendMode poseBlendMode = pSettings->m_layerSettings[i].m_blendMode;

                    // We cannot perform a global blend without a bone mask
                    if ( poseBlendMode == PoseBlendMode::InterpolativeGlobalSpace && !context.m_layerContext.m_layerMaskTaskList.HasTasks() )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        context.LogWarning( GetNodeIndex(), "Attempting to perform a global blend without a bone mask! This is not supported so falling back to a local blend!" );
                        #endif

                        poseBlendMode = PoseBlendMode::Interpolative;
                    }

                    nodeResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::BlendTask>( GetNodeIndex(), nodeResult.m_taskIdx, layerResult.m_taskIdx, context.m_layerContext.m_layerWeight, poseBlendMode, &context.m_layerContext.m_layerMaskTaskList );

                    // Blend root motion
                    if ( !pSettings->m_onlySampleBaseRootMotion )
                    {
                        RootMotionBlendMode const rootMotionBlendMode = ( poseBlendMode == PoseBlendMode::Additive ) ? RootMotionBlendMode::Additive : RootMotionBlendMode::Blend;
                        float const rootMotionWeight = context.m_layerContext.m_layerWeight * context.m_layerContext.m_rootMotionLayerWeight;
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
                    context.m_sampledEventsBuffer.MarkEventsAsIgnored( layerResult.m_sampledEventRange );

                    // Remove any registered pose tasks
                    EE_ASSERT( pSettings->m_layerSettings[i].m_isStateMachineLayer ); // Cannot occur for local layers
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
                context.m_sampledEventsBuffer.UpdateWeights( layerEventRange, context.m_layerContext.m_layerWeight );

                // Mark events as ignored if requested
                if ( pSettings->m_layerSettings[i].m_ignoreEvents )
                {
                    context.m_sampledEventsBuffer.MarkEventsAsIgnored( layerEventRange );
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

            #if EE_DEVELOPMENT_TOOLS
            EE_ASSERT( i < m_debugLayerWeights.size() );
            m_debugLayerWeights[0] = context.m_layerContext.m_layerWeight;
            #endif

            context.m_layerContext.EndLayer();

            // Restore previous context if it existed;
            if ( m_previousContext.IsSet() )
            {
                context.m_layerContext = m_previousContext;
            }
        }
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void LayerBlendNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        PoseNode::RestoreGraphState( inState );
        m_debugLayerWeights.resize( m_layers.size() );
    }
    #endif
}