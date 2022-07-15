#include "Animation_RuntimeGraphNode_Layers.h"
#include "Animation_RuntimeGraphNode_StateMachine.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_RootMotionRecorder.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Blend.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void LayerBlendNode::Settings::InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<LayerBlendNode>( nodePtrs, options );
        SetNodePtrFromIndex( nodePtrs, m_baseNodeIdx, pNode->m_pBaseLayerNode );

        for ( auto const& layerSettings : m_layerSettings )
        {
            StateMachineNode*& pLayerNode = pNode->m_layers.emplace_back();
            SetNodePtrFromIndex( nodePtrs, layerSettings.m_layerNodeIdx, pLayerNode );
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
            m_duration = 1.0f;
        }

        //-------------------------------------------------------------------------

        auto pSettings = GetSettings<LayerBlendNode>();
        int32_t const numLayers = (int32_t) m_layers.size();
        for ( auto i = 0; i < numLayers; i++ )
        {
            auto pLayer = m_layers[i];
            EE_ASSERT( pLayer != nullptr );

            // Only initialize the start time for synchronized layers
            if ( pSettings->m_layerSettings[i].m_isSynchronized )
            {
                pLayer->Initialize( context, initialTime );
            }
            else
            {
                pLayer->Initialize( context, SyncTrackTime() );
            }
        }
    }

    void LayerBlendNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        for ( auto& Layer : m_layers )
        {
            EE_ASSERT( Layer != nullptr );
            Layer->Shutdown( context );
        }

        m_pBaseLayerNode->Shutdown( context );
        PoseNode::ShutdownInternal( context );
    }

    void LayerBlendNode::DeactivateBranch( GraphContext& context )
    {
        if ( IsValid() )
        {
            PoseNode::DeactivateBranch( context );
            m_pBaseLayerNode->DeactivateBranch( context );

            auto const numLayers = m_layers.size();
            for ( auto i = 0; i < numLayers; i++ )
            {
                static_cast<PoseNode*>( m_layers[i] )->DeactivateBranch( context );
            }
        }
    }

    // NB: Layered nodes always update the base according to the specified update time delta or time range. The layers are then updated relative to the base.
    GraphPoseNodeResult LayerBlendNode::Update( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        auto pSettings = GetSettings<LayerBlendNode>();

        GraphPoseNodeResult Result;

        if ( IsValid() )
        {
            // Update the base
            //-------------------------------------------------------------------------

            MarkNodeActive( context );
            m_previousTime = m_pBaseLayerNode->GetCurrentTime();
            Result = m_pBaseLayerNode->Update( context );
            m_currentTime = m_pBaseLayerNode->GetCurrentTime();
            m_duration = m_pBaseLayerNode->GetDuration();

            #if EE_DEVELOPMENT_TOOLS
            m_rootMotionActionIdxBase = context.GetRootMotionActionRecorder()->GetLastActionIndex();
            #endif

            // Update the layers
            //-------------------------------------------------------------------------

            // We need to register a task at the base layer in all cases - since we blend the layers tasks on top of it
            if ( !Result.HasRegisteredTasks() )
            {
                Result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
            }

            UpdateLayers( context, Result );
        }

        return Result;
    }

    // NB: Layered nodes always update the base according to the specified update time delta or time range. The layers are then updated relative to the base.
    GraphPoseNodeResult LayerBlendNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        EE_ASSERT( context.IsValid() );
        auto pSettings = GetSettings<LayerBlendNode>();

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
            m_rootMotionActionIdxBase = context.GetRootMotionActionRecorder()->GetLastActionIndex();
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

    void LayerBlendNode::UpdateLayers( GraphContext& context, GraphPoseNodeResult& nodeResult )
    {
        EE_ASSERT( context.IsValid() && IsValid() );
        auto pSettings = GetSettings<LayerBlendNode>();

        SyncTrackTime const baseStartTime = m_pBaseLayerNode->GetSyncTrack().GetTime( m_pBaseLayerNode->GetPreviousTime() );
        SyncTrackTime const baseEndTime = m_pBaseLayerNode->GetSyncTrack().GetTime( m_pBaseLayerNode->GetCurrentTime() );
        SyncTrackTimeRange layerUpdateRange( baseStartTime, baseEndTime );

        #if EE_DEVELOPMENT_TOOLS
        int16_t rootMotionActionIdxCurrentBase = m_rootMotionActionIdxBase;
        int16_t rootMotionActionIdxLayer = m_rootMotionActionIdxBase;
        #endif

        int32_t const numLayers = (int32_t) m_layers.size();
        for ( auto i = 0; i < numLayers; i++ )
        {
            // Create the layer context
            //-------------------------------------------------------------------------

            // If we are currently in a layer then cache it so that we can safely overwrite it
            if ( context.m_layerContext.IsSet() )
            {
                m_previousContext = context.m_layerContext;
            }

            context.m_layerContext.BeginLayer();

            // Record the current state of the registered tasks
            auto const taskMarker = context.m_pTaskSystem->GetCurrentTaskIndexMarker();

            // Update time and layer weight
            //-------------------------------------------------------------------------
            // Always update the layers as they are state machines and transitions need to be evaluated

            GraphPoseNodeResult layerResult;
            StateMachineNode* pLayerStateMachine = m_layers[i];

            if ( pSettings->m_layerSettings[i].m_isSynchronized )
            {
                layerResult = static_cast<PoseNode*>( pLayerStateMachine )->Update( context, layerUpdateRange );
            }
            else
            {
                layerResult = static_cast<PoseNode*>( pLayerStateMachine )->Update( context );
            }

            #if EE_DEVELOPMENT_TOOLS
            rootMotionActionIdxLayer = context.GetRootMotionActionRecorder()->GetLastActionIndex();
            #endif

            // Register the layer blend tasks
            //-------------------------------------------------------------------------

            // If we registered a task for this layer, then we need to blend
            if ( layerResult.HasRegisteredTasks() )
            {
                // Create a blend task if the layer is enabled
                if ( context.m_layerContext.m_layerWeight > 0 )
                {
                    RootMotionBlendMode const blendMode = pSettings->m_onlySampleBaseRootMotion ? RootMotionBlendMode::IgnoreTarget : RootMotionBlendMode::Blend;
                    nodeResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::BlendTask>( GetNodeIndex(), nodeResult.m_taskIdx, layerResult.m_taskIdx, context.m_layerContext.m_layerWeight, pSettings->m_layerSettings[i].m_blendOptions, context.m_layerContext.m_pLayerMask );
                    nodeResult.m_rootMotionDelta = Blender::BlendRootMotionDeltas( nodeResult.m_rootMotionDelta, layerResult.m_rootMotionDelta, context.m_layerContext.m_layerWeight, blendMode );

                    #if EE_DEVELOPMENT_TOOLS
                    context.GetRootMotionActionRecorder()->RecordBlend( GetNodeIndex(), rootMotionActionIdxCurrentBase, rootMotionActionIdxLayer, nodeResult.m_rootMotionDelta );
                    rootMotionActionIdxCurrentBase = context.GetRootMotionActionRecorder()->GetLastActionIndex();
                    #endif

                }
                else // Remove any layer registered tasks
                {
                    context.m_pTaskSystem->RollbackToTaskIndexMarker( taskMarker );
                }
            }

            // Update events for the Layer
            //-------------------------------------------------------------------------

            SampledEventRange const& layerEventRange = layerResult.m_sampledEventRange;
            int32_t const numLayerEvents = layerEventRange.GetLength();
            if ( numLayerEvents > 0 )
            {
                // Update events and mark them as ignored if requested
                context.m_sampledEvents.UpdateWeights( layerEventRange, context.m_layerContext.m_layerWeight );

                if ( pSettings->m_layerSettings[i].m_ignoreEvents )
                {
                    context.m_sampledEvents.SetFlag( layerEventRange, SampledEvent::Flags::Ignored );
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

            // Reset the layer context
            //-------------------------------------------------------------------------

            context.m_layerContext.EndLayer();

            // Restore previous context if it existed;
            if ( m_previousContext.IsSet() )
            {
                context.m_layerContext = m_previousContext;
            }
        }
    }
}