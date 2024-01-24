#include "Animation_RuntimeGraphNode_PoweredRagdoll.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_DataSet.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Ragdoll.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Physics/PhysicsRagdoll.h"
#include "Engine/Physics/PhysicsWorld.h"


//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void PoweredRagdollNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<PoweredRagdollNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_physicsBlendWeightNodeIdx, pNode->m_pBlendWeightValueNode );
        context.SetOptionalNodePtrFromIndex( m_inpulseOriginVectorNodeIdx, pNode->m_pImpulseOriginValueNode );
        context.SetOptionalNodePtrFromIndex( m_inpulseForceVectorNodeIdx, pNode->m_pImpulseForceValueNode );
        PassthroughNode::Definition::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );

        pNode->m_pRagdollDefinition = context.m_pDataSet->GetResource<Physics::RagdollDefinition>( m_dataSlotIdx );
    }

    bool PoweredRagdollNode::IsValid() const
    {
        if ( m_isFirstUpdate )
        {
            return PassthroughNode::IsValid();
        }

        return PassthroughNode::IsValid() && m_pRagdollDefinition != nullptr && m_pRagdoll != nullptr;
    }

    void PoweredRagdollNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );

        PassthroughNode::InitializeInternal( context, initialTime );

        if ( m_pImpulseOriginValueNode != nullptr )
        {
            m_pImpulseOriginValueNode->Initialize( context );
        }

        if ( m_pImpulseForceValueNode != nullptr )
        {
            m_pImpulseForceValueNode->Initialize( context );
        }

        if ( m_pBlendWeightValueNode != nullptr )
        {
            m_pBlendWeightValueNode->Initialize( context );
        }

        m_isFirstUpdate = true;
    }

    void PoweredRagdollNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        
        if ( m_pBlendWeightValueNode != nullptr )
        {
            m_pBlendWeightValueNode->Shutdown( context );
        }

        if ( m_pImpulseOriginValueNode != nullptr )
        {
            m_pImpulseOriginValueNode->Shutdown( context );
        }

        if ( m_pImpulseForceValueNode != nullptr )
        {
            m_pImpulseForceValueNode->Shutdown( context );
        }

        //-------------------------------------------------------------------------

        // Destroy the ragdoll
        if ( m_pRagdoll != nullptr )
        {
            context.m_pPhysicsWorld->DestroyRagdoll( m_pRagdoll );
        }

        PassthroughNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult PoweredRagdollNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        GraphPoseNodeResult result;

        // Create ragdoll
        //-------------------------------------------------------------------------

        if ( m_isFirstUpdate )
        {
            CreateRagdoll( context );
        }

        // Update source node
        //-------------------------------------------------------------------------

        if ( IsValid() )
        {
            result = PassthroughNode::Update( context, pUpdateRange );
            result = UpdateRagdoll( context, result );
        }
        else
        {
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::ReferencePoseTask>( GetNodeIndex() );
        }

        //-------------------------------------------------------------------------

        m_isFirstUpdate = false;

        return result;
    }

    void PoweredRagdollNode::CreateRagdoll( GraphContext& context )
    {
        EE_ASSERT( m_pRagdoll == nullptr && m_isFirstUpdate );

        if ( m_pRagdollDefinition != nullptr && context.m_pPhysicsWorld != nullptr )
        {
            auto pNodeDefinition = GetDefinition<PoweredRagdollNode>();
            m_pRagdoll = context.m_pPhysicsWorld->CreateRagdoll( m_pRagdollDefinition, pNodeDefinition->m_profileID, context.m_graphUserID );
            m_pRagdoll->SetPoseFollowingEnabled( true );
            m_pRagdoll->SetGravityEnabled( pNodeDefinition->m_isGravityEnabled );
        }
    }

    GraphPoseNodeResult PoweredRagdollNode::UpdateRagdoll( GraphContext& context, GraphPoseNodeResult const& childResult )
    {
        EE_ASSERT( IsValid() );
        GraphPoseNodeResult result = childResult;

        //-------------------------------------------------------------------------

        if ( m_pImpulseOriginValueNode != nullptr && m_pImpulseForceValueNode != nullptr )
        {
            Vector const impulseOrigin = m_pImpulseOriginValueNode->GetValue<Vector>( context );
            Vector const impulseForce = m_pImpulseForceValueNode->GetValue<Vector>( context );
            if ( !impulseForce.IsNearZero3() )
            {
                m_pRagdoll->ApplyImpulse( impulseOrigin, impulseForce );
            }
        }

        //-------------------------------------------------------------------------

        Tasks::RagdollSetPoseTask::InitOption const initOptions = m_isFirstUpdate ? Tasks::RagdollSetPoseTask::InitializeBodies : Tasks::RagdollSetPoseTask::DoNothing;
        TaskIndex const setPoseTaskIdx = context.m_pTaskSystem->RegisterTask<Tasks::RagdollSetPoseTask>( m_pRagdoll, GetNodeIndex(), childResult.m_taskIdx, initOptions );

        //-------------------------------------------------------------------------

        float const physicsWeight = ( m_pBlendWeightValueNode != nullptr ) ? m_pBlendWeightValueNode->GetValue<float>( context ) : GetDefinition<PoweredRagdollNode>()->m_physicsBlendWeight;
        result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::RagdollGetPoseTask>( m_pRagdoll, GetNodeIndex(), setPoseTaskIdx, physicsWeight );
        return result;
    }
}