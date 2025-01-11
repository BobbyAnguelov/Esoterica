#include "Animation_RuntimeGraphNode_AimIK.h"
#include "Game/Animation/Tasks/Animation_Task_AimIK.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void AimIKNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<AimIKNode>( context, options );
        PassthroughNode::Definition::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
        context.SetNodePtrFromIndex( m_targetNodeIdx, pNode->m_pTargetValueNode );
    }

    void AimIKNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        PassthroughNode::InitializeInternal( context, initialTime );
        m_pTargetValueNode->Initialize( context );
    }

    void AimIKNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        m_pTargetValueNode->Shutdown( context );
        PassthroughNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult AimIKNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        GraphPoseNodeResult result = PassthroughNode::Update( context, pUpdateRange );

        if ( result.HasRegisteredTasks() )
        {
            Vector const worldSpaceTarget = m_pTargetValueNode->GetValue<Float3>( context );
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::AimIKTask>( GetNodeIndex(), result.m_taskIdx, worldSpaceTarget );
        }

        return result;
    }
}