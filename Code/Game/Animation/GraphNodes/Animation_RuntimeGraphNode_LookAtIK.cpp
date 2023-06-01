#include "Animation_RuntimeGraphNode_LookAtIK.h"
#include "Game/Animation/Tasks/Animation_Task_LookAtIK.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void LookAtIKNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<LookAtIKNode>( context, options );
        PassthroughNode::Settings::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
        context.SetNodePtrFromIndex( m_targetNodeIdx, pNode->m_pTargetValueNode );
    }

    void LookAtIKNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        PassthroughNode::InitializeInternal( context, initialTime );
        m_pTargetValueNode->Initialize( context );
    }

    void LookAtIKNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        m_pTargetValueNode->Shutdown( context );
        PassthroughNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult LookAtIKNode::Update( GraphContext& context )
    {
        GraphPoseNodeResult result = PassthroughNode::Update( context );
        result = RegisterIKTask( context, result );
        return result;
    }

    GraphPoseNodeResult LookAtIKNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        GraphPoseNodeResult result = PassthroughNode::Update( context, updateRange );
        result = RegisterIKTask( context, result );
        return result;
    }

    GraphPoseNodeResult LookAtIKNode::RegisterIKTask( GraphContext& context, GraphPoseNodeResult const& childNodeResult )
    {
        GraphPoseNodeResult updatedResult = childNodeResult;
        Vector const worldSpaceTarget = m_pTargetValueNode->GetValue<Vector>( context );
        updatedResult.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::LookAtIKTask>( GetNodeIndex(), updatedResult.m_taskIdx, worldSpaceTarget );
        return updatedResult;
    }
}