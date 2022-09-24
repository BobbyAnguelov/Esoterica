#include "Animation_RuntimeGraphNode_OrientationWarp.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void OrientationWarpNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<OrientationWarpNode>( context, options );
        context.SetNodePtrFromIndex( m_clipReferenceNodeIdx, pNode->m_pClipReferenceNode );
        context.SetNodePtrFromIndex( m_angleOffsetValueNodeIdx, pNode->m_pAngleOffsetValueNode );
    }

    void OrientationWarpNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pAngleOffsetValueNode != nullptr );
        PoseNode::InitializeInternal( context, initialTime );
        m_pClipReferenceNode->Initialize( context, initialTime );
        m_pAngleOffsetValueNode->Initialize( context );

        //-------------------------------------------------------------------------

        PerformWarp( context );
    }

    void OrientationWarpNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        EE_ASSERT( m_pAngleOffsetValueNode != nullptr );
        m_pAngleOffsetValueNode->Shutdown( context );
        m_pClipReferenceNode->Shutdown( context );
        PoseNode::ShutdownInternal( context );
    }

    void OrientationWarpNode::PerformWarp( GraphContext& context )
    {
        auto pAnimation = m_pClipReferenceNode->GetAnimation();
        EE_ASSERT( pAnimation != nullptr );
    }

    Transform OrientationWarpNode::SampleWarpedRootMotion( GraphContext& context ) const
    {
        return Transform();
    }

    GraphPoseNodeResult OrientationWarpNode::Update( GraphContext& context )
    {
        auto result = m_pClipReferenceNode->Update( context );
        m_duration = m_pClipReferenceNode->GetDuration();
        m_previousTime = m_pClipReferenceNode->GetPreviousTime();
        m_currentTime = m_pClipReferenceNode->GetCurrentTime();

        result.m_rootMotionDelta = SampleWarpedRootMotion( context );
        return result;
    }

    GraphPoseNodeResult OrientationWarpNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        auto result = m_pClipReferenceNode->Update( context, updateRange );
        m_duration = m_pClipReferenceNode->GetDuration();
        m_previousTime = m_pClipReferenceNode->GetPreviousTime();
        m_currentTime = m_pClipReferenceNode->GetCurrentTime();

        result.m_rootMotionDelta = SampleWarpedRootMotion( context );
        return result;
    }
}