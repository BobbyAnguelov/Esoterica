#include "Animation_RuntimeGraphNode_TwoBoneIK.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_ChainSolver.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void TwoBoneIKNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<TwoBoneIKNode>( context, options );
        PassthroughNode::Definition::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );
        context.SetNodePtrFromIndex( m_effectorTargetNodeIdx, pNode->m_pEffectorTarget );
    }

    void TwoBoneIKNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );
        PassthroughNode::InitializeInternal( context, initialTime );
        m_pEffectorTarget->Initialize( context );

        auto pDefinition = GetDefinition<TwoBoneIKNode>();
        EE_ASSERT( pDefinition->m_effectorBoneID.IsValid() );

        // Get and validate effector chain
        //-------------------------------------------------------------------------

        m_effectorBoneIdx = context.m_pSkeleton->GetBoneIndex( pDefinition->m_effectorBoneID );
        if ( m_effectorBoneIdx == InvalidIndex )
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogError( GetNodeIndex(), "Cant find specified bone ID ('%s') for two bone effector node", pDefinition->m_effectorBoneID.c_str() );
            #endif
        }

        // Validate that there are at least two bones in the chain
        int32_t parentCount = 0;
        int32_t parentIdx = context.m_pSkeleton->GetParentBoneIndex( m_effectorBoneIdx );
        while ( parentIdx != InvalidIndex && parentCount < 2 )
        {
            parentCount++;
            parentIdx = context.m_pSkeleton->GetParentBoneIndex( parentIdx );
        }

        if ( parentCount != 2 )
        {
            #if EE_DEVELOPMENT_TOOLS
            context.LogError( GetNodeIndex(), "Invalid effector bone ID ('%s') specified, there are not enough bones in the chain to solve the IK request", pDefinition->m_effectorBoneID.c_str() );
            #endif

            m_effectorBoneIdx = InvalidIndex;
        }
    }

    void TwoBoneIKNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        m_effectorBoneIdx = InvalidIndex;
        m_pEffectorTarget->Shutdown( context );
        PassthroughNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult TwoBoneIKNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        GraphPoseNodeResult result;

        result = PassthroughNode::Update( context, pUpdateRange );

        if ( result.HasRegisteredTasks() && m_effectorBoneIdx != InvalidIndex )
        {
            Target const effectorTarget = m_pEffectorTarget->GetValue<Target>( context );
            if ( effectorTarget.IsTargetSet() )
            {
                bool shouldRegisterTask = true;

                // Validate input target
                if ( effectorTarget.IsBoneTarget() )
                {
                    StringID const effectorTargetBoneID = effectorTarget.GetBoneID();
                    if ( !effectorTargetBoneID.IsValid() )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        context.LogError( GetNodeIndex(), "No effector bone ID specified in input target" );
                        #endif
                        shouldRegisterTask = false;
                    }

                    if( context.m_pSkeleton->GetBoneIndex( effectorTargetBoneID ) == InvalidIndex )
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        context.LogError( GetNodeIndex(), "Invalid effector bone ID ('%s') specified in input target", effectorTarget.GetBoneID().c_str() );
                        #endif
                        shouldRegisterTask = false;
                    }
                }

                if ( shouldRegisterTask )
                {
                    auto pDefinition = GetDefinition<TwoBoneIKNode>();
                    result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::ChainSolverTask>( GetNodeIndex(), result.m_taskIdx, m_effectorBoneIdx, 3, pDefinition->m_isTargetInWorldSpace, effectorTarget );
                }
            }
        }

        return result;
    }
}