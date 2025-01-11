#include "Animation_RuntimeGraphNode_ExternalGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Instance.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void ExternalGraphNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ExternalGraphNode>( context, options );
    }

    ExternalGraphNode::~ExternalGraphNode()
    {
        // Check for external instance leaks
        EE_ASSERT( m_pGraphInstance == nullptr );
    }

    void ExternalGraphNode::AttachExternalGraphInstance( GraphContext& context, GraphInstance* pExternalGraphInstance )
    {
        EE_ASSERT( pExternalGraphInstance != nullptr );
        EE_ASSERT( m_pGraphInstance == nullptr );
        m_pGraphInstance = pExternalGraphInstance;
        m_shouldResetGraphInstance = true;
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
            m_shouldResetGraphInstance = true;
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
        if ( m_pGraphInstance != nullptr )
        {
            auto pRootNode = m_pGraphInstance->GetRootNode();
            return pRootNode->GetSyncTrack();
        }

        return SyncTrack::s_defaultTrack;
    }

    //-------------------------------------------------------------------------

    GraphPoseNodeResult ExternalGraphNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        EE_ASSERT( context.IsValid() );
        MarkNodeActive( context );

        //-------------------------------------------------------------------------

        GraphPoseNodeResult result;
        result.m_sampledEventRange = context.GetEmptySampledEventRange();
        result.m_taskIdx = InvalidIndex;

        //-------------------------------------------------------------------------

        if ( m_pGraphInstance != nullptr )
        {
            // Push the current node idx into the event debug path
            #if EE_DEVELOPMENT_TOOLS
            context.PushDebugPath( GetNodeIndex() );
            #endif

            // Evaluate external graph
            result = m_pGraphInstance->EvaluateGraph( context.m_deltaTime, context.m_worldTransform, context.m_pPhysicsWorld, pUpdateRange, m_shouldResetGraphInstance );
            m_shouldResetGraphInstance = false;

            // Transfer graph state
            auto pRootNode = m_pGraphInstance->GetRootNode();
            m_previousTime = pRootNode->GetCurrentTime();
            m_currentTime = pRootNode->GetCurrentTime();
            m_duration = pRootNode->GetDuration();

            // Update sampled event range
            result.m_sampledEventRange.m_endIdx = context.m_pSampledEventsBuffer->GetNumSampledEvents();

            // Pop debug path element
            #if EE_DEVELOPMENT_TOOLS
            context.PopDebugPath();
            #endif
        }

        return result;
    }

    #if EE_DEVELOPMENT_TOOLS
    void ExternalGraphNode::DrawDebug( GraphContext& graphContext, Drawing::DrawContext& drawCtx )
    {
        if ( m_pGraphInstance != nullptr )
        {
            m_pGraphInstance->DrawNodeDebug( graphContext, drawCtx );
        }
    }
    #endif
}