#include "Animation_RuntimeGraphNode_Pose.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_DataSet.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Sample.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ZeroPoseNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ZeroPoseNode>( context, options );
    }

    void ZeroPoseNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        PoseNode::InitializeInternal( context, initialTime );

        m_previousTime = m_currentTime;
        m_duration = s_oneFrameDuration;
    }

    GraphPoseNodeResult ZeroPoseNode::Update( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        MarkNodeActive( context );

        GraphPoseNodeResult result;
        result.m_sampledEventRange = context.GetEmptySampledEventRange();
        result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ZeroPose );
        return result;
    }

    //-------------------------------------------------------------------------

    void ReferencePoseNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        CreateNode<ReferencePoseNode>( context, options );
    }

    void ReferencePoseNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        PoseNode::InitializeInternal( context, initialTime );

        m_previousTime = m_currentTime;
        m_duration = s_oneFrameDuration;
    }

    GraphPoseNodeResult ReferencePoseNode::Update( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        MarkNodeActive( context );

        GraphPoseNodeResult result;
        result.m_sampledEventRange = context.GetEmptySampledEventRange();
        result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
        return result;
    }

    //-------------------------------------------------------------------------

    void AnimationPoseNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<AnimationPoseNode>( context, options );
        context.SetNodePtrFromIndex( m_poseTimeValueNodeIdx, pNode->m_pPoseTimeValue );
        pNode->m_pAnimation = context.GetResource<AnimationClip>( m_dataSlotIndex );
    }

    bool AnimationPoseNode::IsValid() const
    {
        return PoseNode::IsValid() && m_pAnimation != nullptr;
    }

    void AnimationPoseNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() && m_pPoseTimeValue != nullptr );
        PoseNode::InitializeInternal( context, initialTime );
        m_pPoseTimeValue->Initialize( context );

        m_previousTime = m_currentTime;
        m_duration = s_oneFrameDuration;
    }

    void AnimationPoseNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pPoseTimeValue != nullptr );
        m_pPoseTimeValue->Shutdown( context );
        PoseNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult AnimationPoseNode::Update( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        GraphPoseNodeResult result;
        if ( !IsValid() )
        {
            return result;
        }

        EE_ASSERT( m_pAnimation != nullptr );

        MarkNodeActive( context );

        // Register the sample task
        //-------------------------------------------------------------------------

        float timeValue = m_pPoseTimeValue->GetValue<float>( context );

        // Optional Remap
        auto pSettings = GetSettings<AnimationPoseNode>();
        if ( pSettings->m_inputTimeRemapRange.IsSet() )
        {
            timeValue = pSettings->m_inputTimeRemapRange.GetPercentageThroughClamped( timeValue );
        }

        // Ensure valid time value
        timeValue = Math::Clamp( timeValue, 0.0f, 1.0f );
        Percentage const sampleTime( timeValue );

        result.m_sampledEventRange = context.GetEmptySampledEventRange();
        result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::SampleTask>( GetNodeIndex(), m_pAnimation, sampleTime );
        return result;
    }
}