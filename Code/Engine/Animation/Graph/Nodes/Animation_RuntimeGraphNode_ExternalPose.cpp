#include "Animation_RuntimeGraphNode_ExternalPose.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Context.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_RootMotionDebugger.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_CachedPose.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void ExternalPoseNode::Definition::InstantiateNode( InstantiationContext const &context, InstantiationOptions options ) const
    {
        CreateNode<ExternalPoseNode>( context, options );
    }

    void ExternalPoseNode::Definition::PostInstantiateNode( InstantiationContext const &context ) const
    {
        auto pNode = static_cast<ExternalPoseNode*>( context.m_nodePtrs[context.m_currentNodeIdx] );
        pNode->m_externalPoseBufferID = context.m_pTaskSystem->CreatePersistentCachedPose();
        EE_ASSERT( pNode->m_externalPoseBufferID.IsValid() );
    }

    //-------------------------------------------------------------------------

    void ExternalPoseNode::InitializeInternal( GraphContext &context, SyncTrackTime const &initialTime )
    {
        EE_ASSERT( context.IsValid() );
        PoseNode::InitializeInternal( context, initialTime );

        m_previousTime = m_currentTime = 1.0f;
        m_duration = 0;
    }

    void ExternalPoseNode::ShutdownInternal( GraphContext &context )
    {
        EE_ASSERT( context.IsValid() );
        PoseNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult ExternalPoseNode::Update( GraphContext &context, SyncTrackTimeRange const *pUpdateRange )
    {
        EE_ASSERT( context.IsValid() );

        MarkNodeActive( context );

        GraphPoseNodeResult result;
        result.m_sampledEventRange = context.GetEmptySampledEventRange();

        // Set node time
        m_previousTime = m_currentTime = 1.0f;
        m_duration = 0;

        // Forward root-motion to the graph, and clear the delta
        result.m_rootMotionDelta = m_rootMotionDelta;

        #if EE_DEVELOPMENT_TOOLS
        context.GetRootMotionDebugger()->RecordSampling( GetNodePath( context ), result.m_rootMotionDelta );
        #endif

        m_rootMotionDelta = Transform::Identity;

        // Set the layer bone-mask
        // TODO: this needs to be tested in a variety of situations!
        if ( context.m_pLayerContext != nullptr && m_boneMask.HasTasks() )
        {
            // If we dont have a bone mask task list, use the task list in the code
            if ( !context.m_pLayerContext->m_layerMaskTaskList.HasTasks() )
            {
                context.m_pLayerContext->m_layerMaskTaskList.CopyFrom( m_boneMask );
            }
            else // If we already have a bone mask set, combine the bone masks
            {
                context.m_pLayerContext->m_layerMaskTaskList.CombineWith( m_boneMask );
            }
        }

        // Register the read
        result.m_taskIdx = context.GetTaskSystem()->RegisterTask<CachedPoseReadTask>( GetNodePath( context ), m_externalPoseBufferID );

        return result;
    }
}