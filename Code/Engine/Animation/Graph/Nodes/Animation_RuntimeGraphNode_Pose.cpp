#include "Animation_RuntimeGraphNode_Pose.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Contexts.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_DataSet.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Sample.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void ZeroPoseNode::Settings::InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<ZeroPoseNode>( nodePtrs, options );
    }

    void ZeroPoseNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        m_previousTime = m_currentTime;
        m_duration = 1 / 30.0f;
    }

    GraphPoseNodeResult ZeroPoseNode::Update( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        MarkNodeActive( context );
        auto pSettings = GetSettings<ZeroPoseNode>();

        GraphPoseNodeResult Result;
        Result.m_sampledEventRange = SampledEventRange( context.m_sampledEvents.GetNumEvents() );
        Result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ZeroPose );
        return Result;
    }

    //-------------------------------------------------------------------------

    void ReferencePoseNode::Settings::InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<ReferencePoseNode>( nodePtrs, options );
    }

    void ReferencePoseNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        m_previousTime = m_currentTime;
        m_duration = 1 / 30.0f;
    }

    GraphPoseNodeResult ReferencePoseNode::Update( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        MarkNodeActive( context );
        auto pSettings = GetSettings<ReferencePoseNode>();

        GraphPoseNodeResult Result;
        Result.m_sampledEventRange = SampledEventRange( context.m_sampledEvents.GetNumEvents() );
        Result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
        return Result;
    }

    //-------------------------------------------------------------------------

    void AnimationPoseNode::Settings::InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<AnimationPoseNode>( nodePtrs, options );
        SetNodePtrFromIndex( nodePtrs, m_poseTimeValueNodeIdx, pNode->m_pPoseTimeValue );
        pNode->m_pAnimation = pDataSet->GetResource<AnimationClip>( m_dataSlotIndex );
    }

    bool AnimationPoseNode::IsValid() const
    {
        return PoseNode::IsValid() && m_pAnimation != nullptr;
    }

    void AnimationPoseNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() && m_pPoseTimeValue != nullptr );
        auto pSettings = GetSettings<AnimationPoseNode>();

        PoseNode::InitializeInternal( context, initialTime );
        m_pPoseTimeValue->Initialize( context );
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
        auto Settings = GetSettings<PoseNode>();

        GraphPoseNodeResult Result;
        if ( !IsValid() )
        {
            return Result;
        }

        EE_ASSERT( m_pAnimation != nullptr );

        MarkNodeActive( context );
        m_currentTime = m_previousTime = GetTimeValue( context );

        //-------------------------------------------------------------------------

        Result.m_sampledEventRange = SampledEventRange( context.m_sampledEvents.GetNumEvents() );
        Result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::SampleTask>( GetNodeIndex(), m_pAnimation, m_currentTime );
        return Result;
    }

    float AnimationPoseNode::GetTimeValue( GraphContext& context )
    {
        EE_ASSERT( IsValid() );

        float timeValue = 0.0f;
        if ( m_pPoseTimeValue != nullptr )
        {
            auto pSettings = GetSettings<AnimationPoseNode>();
            timeValue = m_pPoseTimeValue->GetValue<float>( context );
            timeValue = pSettings->m_inputTimeRemapRange.GetPercentageThroughClamped( timeValue );
        }

        EE_ASSERT( timeValue >= 0.0f && timeValue <= 1.0f );
        return Percentage( timeValue );
    }
}