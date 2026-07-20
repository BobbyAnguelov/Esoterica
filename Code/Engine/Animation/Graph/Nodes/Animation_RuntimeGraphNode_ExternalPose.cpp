#include "Animation_RuntimeGraphNode_ExternalPose.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Context.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_RootMotionDebugger.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_CachedPose.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void ExternalPoseData::Reset()
    {
        m_rootMotion = Transform::Identity;
        m_hasRootMotion = false;
        m_isRootMotionDelta = true;
    }

    //-------------------------------------------------------------------------

    void ExternalPoseNode::Definition::InstantiateNode( InstantiationContext const &context, InstantiationOptions options ) const
    {
        CreateNode<ExternalPoseNode>( context, options );
    }

    bool ExternalPoseNode::IsValid() const
    {
        return PoseNode::IsValid() && m_isPoseSet;
    }

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

        GraphPoseNodeResult result;
        result.m_sampledEventRange = context.GetEmptySampledEventRange();

        if ( !IsValid() )
        {
            return result;
        }

        MarkNodeActive( context );

        // Root Motion
        //-------------------------------------------------------------------------

        if ( m_poseData.m_hasRootMotion )
        {
            EE_ASSERT( !m_poseData.m_rootMotion.HasScale() );

            if ( m_poseData.m_isRootMotionDelta )
            {
                result.m_rootMotionDelta = m_poseData.m_rootMotion;
            }
            else
            {
                result.m_rootMotionDelta = m_poseData.m_rootMotion * context.m_worldTransformInverse;
            }

            #if EE_DEVELOPMENT_TOOLS
            context.GetRootMotionDebugger()->RecordSampling( GetNodePath( context ), result.m_rootMotionDelta );
            #endif
        }

        // Register pose tasks
        //-------------------------------------------------------------------------

        //if ( hasdata )
        //{
        //    // Register task
        //    result.m_taskIdx = context.GetTaskSystem()->RegisterTask<ExternalPoseReadTask>( GetNodePath( context ), GetNodeIdx() );
        //}

        return result;
    }

    //-------------------------------------------------------------------------

    void IsExternalPoseSetNode::Definition::InstantiateNode( InstantiationContext const &context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<IsExternalPoseSetNode>( context, options );
        context.SetNodePtrFromIndex( m_externalPoseNodeIdx, pNode->m_pExternalPoseNode );
    }

    void IsExternalPoseSetNode::InitializeInternal( GraphContext &context )
    {
        EE_ASSERT( context.IsValid() );
        BoolValueNode::InitializeInternal( context );
        m_result = false;
    }

    void IsExternalPoseSetNode::ShutdownInternal( GraphContext &context )
    {
        EE_ASSERT( context.IsValid() );
        BoolValueNode::ShutdownInternal( context );
    }

    void IsExternalPoseSetNode::GetValueInternal( GraphContext &context, void *pOutValue )
    {
        EE_ASSERT( context.IsValid() );

        // Is the Result up to date?
        if ( !WasUpdated( context ) )
        {
            m_result = m_pExternalPoseNode->IsPoseSet();
            MarkNodeActive( context );
        }

        // Set Result
        *( (bool *) pOutValue ) = m_result;
    }
}