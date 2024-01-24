#include "Animation_RuntimeGraphNode_LookAtIK.h"
#include "Game/Animation/Tasks/Animation_Task_LookAtIK.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"


//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void LookAtIKNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<LookAtIKNode>( context, options );
        PassthroughNode::Definition::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
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

    GraphPoseNodeResult LookAtIKNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        GraphPoseNodeResult result = PassthroughNode::Update( context, pUpdateRange );
        if ( result.HasRegisteredTasks() )
        {
            result = RegisterIKTask( context, result );
        }
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