#include "Animation_RuntimeGraphNode_ExternalGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Instance.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ExternalGraphNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ExternalGraphNode>( context, options );
    }

    ExternalGraphNode::~ExternalGraphNode()
    {
        // Check for external instance leaks
        EE_ASSERT( m_pGraphInstance == nullptr );
    }

    void ExternalGraphNode::AttachGraphInstance( GraphContext& context, GraphInstance* pExternalGraphInstance )
    {
        EE_ASSERT( pExternalGraphInstance != nullptr );
        EE_ASSERT( m_pGraphInstance == nullptr );
        m_pGraphInstance = pExternalGraphInstance;
    }

    void ExternalGraphNode::DetachExternalGraphInstance( GraphContext& context )
    {
        EE_ASSERT( m_pGraphInstance != nullptr );
        m_pGraphInstance = nullptr;
    }

    void ExternalGraphNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        PoseNode::InitializeInternal( context, initialTime );

        if ( m_pGraphInstance != nullptr )
        {
            auto pRootNode = m_pGraphInstance->GetRootNode();
            m_previousTime = pRootNode->GetCurrentTime();
            m_currentTime = pRootNode->GetCurrentTime();
            m_duration = pRootNode->GetDuration();
        }
        else
        {
            m_previousTime = m_currentTime = 0.0f;
            m_duration = 0.0f;
        }

        m_isFirstUpdate = true;
    }

    //-------------------------------------------------------------------------

    SyncTrack const& ExternalGraphNode::GetSyncTrack() const
    {
        if ( m_pGraphInstance != nullptr )
        {
            auto pRootNode = m_pGraphInstance->GetRootNode();
            return pRootNode->GetSyncTrack();
        }

        return SyncTrack::s_defaultTrack;
    }

    void ExternalGraphNode::TransferGraphInstanceData( GraphContext& context, GraphPoseNodeResult& result )
    {
        auto& localEventBuffer = context.m_sampledEventsBuffer;
        auto const& externalEventBuffer = m_pGraphInstance->GetSampledEvents();
        result.m_sampledEventRange = localEventBuffer.AppendBuffer( externalEventBuffer );

        auto pRootNode = m_pGraphInstance->GetRootNode();
        m_previousTime = pRootNode->GetCurrentTime();
        m_currentTime = pRootNode->GetCurrentTime();
        m_duration = pRootNode->GetDuration();

        #if EE_DEVELOPMENT_TOOLS
        context.GetRootMotionDebugger()->RecordGraphSource( GetNodeIndex(), result.m_rootMotionDelta );
        #endif
    }

    //-------------------------------------------------------------------------

    GraphPoseNodeResult ExternalGraphNode::Update( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        MarkNodeActive( context );

        GraphPoseNodeResult result;
        if ( m_pGraphInstance == nullptr )
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
        }
        else
        {
            result = m_pGraphInstance->EvaluateGraph( context.m_deltaTime, context.m_worldTransform, context.m_pPhysicsWorld, m_isFirstUpdate );
            m_isFirstUpdate = false;
            TransferGraphInstanceData( context, result );
        }
        return result;
    }

    GraphPoseNodeResult ExternalGraphNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        EE_ASSERT( context.IsValid() );
        MarkNodeActive( context );

        GraphPoseNodeResult result;
        if ( m_pGraphInstance == nullptr )
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
        }
        else
        {
            result = m_pGraphInstance->EvaluateGraph( context.m_deltaTime, context.m_worldTransform, context.m_pPhysicsWorld, updateRange, m_isFirstUpdate );
            m_isFirstUpdate = false;
            TransferGraphInstanceData( context, result );
        }
        return result;
    }
}