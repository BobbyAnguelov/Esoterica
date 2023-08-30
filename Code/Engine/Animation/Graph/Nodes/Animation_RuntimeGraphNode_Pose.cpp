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

        m_previousTime = m_currentTime = 1.0f;
        m_duration = 0;
    }

    GraphPoseNodeResult ZeroPoseNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
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

        m_previousTime = m_currentTime = 1.0f;
        m_duration = 0;
    }

    GraphPoseNodeResult ReferencePoseNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
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
        context.SetOptionalNodePtrFromIndex( m_poseTimeValueNodeIdx, pNode->m_pPoseTimeValue );
        pNode->m_pAnimation = context.m_pDataSet->GetResource<AnimationClip>( m_dataSlotIndex );

        //-------------------------------------------------------------------------

        if ( pNode->m_pAnimation != nullptr )
        {
            if ( pNode->m_pAnimation->GetSkeleton() != context.m_pDataSet->GetSkeleton() )
            {
                pNode->m_pAnimation = nullptr;
            }
        }
    }

    bool AnimationPoseNode::IsValid() const
    {
        return PoseNode::IsValid() && m_pAnimation != nullptr;
    }

    void AnimationPoseNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        PoseNode::InitializeInternal( context, initialTime );

        if ( m_pPoseTimeValue != nullptr )
        {
            m_pPoseTimeValue->Initialize( context );
        }

        m_previousTime = m_currentTime = 1.0f;
        m_duration = 0;
    }

    void AnimationPoseNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        if ( m_pPoseTimeValue != nullptr )
        {
            m_pPoseTimeValue->Shutdown( context );
        }

        PoseNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult AnimationPoseNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
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

        if ( m_pAnimation->IsSingleFrameAnimation() )
        {
            m_currentTime = 0.0f;
            m_previousTime = 0.0f;
        }
        else
        {
            auto pSettings = GetSettings<AnimationPoseNode>();
            float timeValue = ( m_pPoseTimeValue != nullptr ) ? m_pPoseTimeValue->GetValue<float>( context ) : pSettings->m_userSpecifiedTime;

            // Optional Remap
            if ( pSettings->m_inputTimeRemapRange.IsSet() )
            {
                timeValue = pSettings->m_inputTimeRemapRange.GetPercentageThroughClamped( timeValue );
            }

            // Convert to percentage
            if ( pSettings->m_useFramesAsInput )
            {
                timeValue = timeValue / ( m_pAnimation->GetNumFrames() - 1 );
            }

            // Ensure valid time value
            m_currentTime = Math::Clamp( timeValue, 0.0f, 1.0f );
            m_previousTime = m_currentTime;
        }

        result.m_sampledEventRange = context.GetEmptySampledEventRange();
        result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::SampleTask>( GetNodeIndex(), m_pAnimation, Percentage( m_currentTime ) );
        return result;
    }
}