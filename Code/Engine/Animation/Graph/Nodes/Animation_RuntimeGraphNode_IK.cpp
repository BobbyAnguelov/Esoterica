#include "Animation_RuntimeGraphNode_IK.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_IK.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void IKRigNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<IKRigNode>( context, options );
        PassthroughNode::Definition::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );

        // Get rig definition
        auto pRigDefinition = context.GetResource<IKRigDefinition>( m_dataSlotIdx );
        if ( pRigDefinition )
        {
            pNode->m_pRig = EE::New<IKRig>( pRigDefinition );
        }

        //-------------------------------------------------------------------------

        int32_t const numTargets = (int32_t) m_effectorTargetIndices.size();
        pNode->m_effectorTargetNodes.resize( numTargets, nullptr );

        for ( int32_t i = 0; i < numTargets; i++ )
        {
            if ( m_effectorTargetIndices[i] != InvalidIndex )
            {
                context.SetNodePtrFromIndex( m_effectorTargetIndices[i], pNode->m_effectorTargetNodes[i] );
            }
        }
    }

    IKRigNode::~IKRigNode()
    {
        EE::Delete( m_pRig );
    }

    void IKRigNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        PassthroughNode::InitializeInternal( context, initialTime );

        for ( auto pTarget : m_effectorTargetNodes )
        {
            if ( pTarget != nullptr )
            {
                pTarget->Initialize( context );
            }
        }
    }

    void IKRigNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        PassthroughNode::ShutdownInternal( context );

        for ( auto pTarget : m_effectorTargetNodes )
        {
            if ( pTarget != nullptr )
            {
                pTarget->Shutdown( context );
            }
        }
    }

    GraphPoseNodeResult IKRigNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        GraphPoseNodeResult result;

        if ( m_pRig == nullptr )
        {
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::ReferencePoseTask>( GetNodeIndex() );
        }
        else
        {
            result = PassthroughNode::Update( context, pUpdateRange );

            if ( result.HasRegisteredTasks() )
            {
                int32_t const numTargets = (int32_t) m_effectorTargetNodes.size();
                TInlineVector<Target, 6> targets;
                targets.resize( numTargets );

                for ( int32_t i = 0; i < numTargets; i++ )
                {
                    if ( m_effectorTargetNodes[i] != nullptr )
                    {
                        targets[i] = m_effectorTargetNodes[i]->GetValue<Target>( context );
                    }
                }

                result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::IKRigTask>( GetNodeIndex(), result.m_taskIdx, m_pRig, targets );
            }
        }

        return result;
    }
}