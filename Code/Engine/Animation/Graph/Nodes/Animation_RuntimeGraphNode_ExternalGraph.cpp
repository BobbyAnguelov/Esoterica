#include "Animation_RuntimeGraphNode_ExternalGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Instance.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ExternalGraphNode::Settings::InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<ExternalGraphNode>( nodePtrs, options );
    }

    ExternalGraphNode::~ExternalGraphNode()
    {
        // Check for external instance leaks
        EE_ASSERT( m_pExternalInstance == nullptr );
    }

    void ExternalGraphNode::AttachGraphInstance( GraphContext& context, GraphInstance* pExternalGraphInstance )
    {
        EE_ASSERT( pExternalGraphInstance != nullptr );
        EE_ASSERT( m_pExternalInstance == nullptr );
        m_pExternalInstance = pExternalGraphInstance;
    }

    void ExternalGraphNode::DetachExternalGraphInstance( GraphContext& context )
    {
        EE_ASSERT( m_pExternalInstance != nullptr );
        m_pExternalInstance = nullptr;
    }

    void ExternalGraphNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        PoseNode::InitializeInternal( context, initialTime );

        if ( m_pExternalInstance != nullptr )
        {
            m_pExternalInstance->ResetGraphState();

            auto pRootNode = m_pExternalInstance->GetRootNode();
            m_previousTime = pRootNode->GetCurrentTime();
            m_currentTime = pRootNode->GetCurrentTime();
            m_duration = pRootNode->GetDuration();
        }
        else
        {
            m_previousTime = m_currentTime = 0.0f;
            m_duration = 0.0f;
        }
    }

    //-------------------------------------------------------------------------

    SyncTrack const& ExternalGraphNode::GetSyncTrack() const
    {
        if ( m_pExternalInstance != nullptr )
        {
            auto pRootNode = m_pExternalInstance->GetRootNode();
            return pRootNode->GetSyncTrack();
        }

        return SyncTrack::s_defaultTrack;
    }

    void ExternalGraphNode::TransferExternalData( GraphContext& context, GraphPoseNodeResult& result )
    {
        auto& localEventBuffer = context.m_sampledEventsBuffer;
        auto const& externalEventBuffer = m_pExternalInstance->GetSampledEvents();
        localEventBuffer.Append( externalEventBuffer );
        result.m_sampledEventRange = SampledEventRange( localEventBuffer.GetNumEvents(), localEventBuffer.GetNumEvents() + externalEventBuffer.GetNumEvents() );

        auto pRootNode = m_pExternalInstance->GetRootNode();
        m_previousTime = pRootNode->GetCurrentTime();
        m_currentTime = pRootNode->GetCurrentTime();
        m_duration = pRootNode->GetDuration();
    }

    //-------------------------------------------------------------------------

    GraphPoseNodeResult ExternalGraphNode::Update( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        MarkNodeActive( context );

        GraphPoseNodeResult result;
        if ( m_pExternalInstance == nullptr )
        {
            result.m_sampledEventRange = SampledEventRange( context.m_sampledEventsBuffer.GetNumEvents() );
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
        }
        else
        {
            result = m_pExternalInstance->UpdateGraph();
            TransferExternalData( context, result );
        }
        return result;
    }

    GraphPoseNodeResult ExternalGraphNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        EE_ASSERT( context.IsValid() );
        MarkNodeActive( context );

        GraphPoseNodeResult result;
        if ( m_pExternalInstance == nullptr )
        {
            result.m_sampledEventRange = SampledEventRange( context.m_sampledEventsBuffer.GetNumEvents() );
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
        }
        else
        {
            result = m_pExternalInstance->UpdateGraph( updateRange );
            TransferExternalData( context, result );
        }
        return result;
    }
}