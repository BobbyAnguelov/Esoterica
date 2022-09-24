#include "Animation_RuntimeGraphNode_PoweredRagdoll.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_DataSet.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Ragdoll.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Physics/PhysicsRagdoll.h"
#include "Engine/Physics/PhysicsScene.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void PoweredRagdollNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<PoweredRagdollNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_physicsBlendWeightNodeIdx, pNode->m_pBlendWeightValueNode );
        PassthroughNode::Settings::InstantiateNode( context, InstantiationOptions::NodeAlreadyCreated );

        pNode->m_pRagdollDefinition = context.GetResource<Physics::RagdollDefinition>( m_dataSlotIdx );
    }

    bool PoweredRagdollNode::IsValid() const
    {
        return PassthroughNode::IsValid() && m_pRagdollDefinition != nullptr && m_pRagdoll != nullptr;
    }

    void PoweredRagdollNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );

        PassthroughNode::InitializeInternal( context, initialTime );

        // Create ragdoll
        if ( m_pRagdollDefinition != nullptr && context.m_pPhysicsScene != nullptr )
        {
            auto pNodeSettings = GetSettings<PoweredRagdollNode>();
            m_pRagdoll = context.m_pPhysicsScene->CreateRagdoll( m_pRagdollDefinition, pNodeSettings->m_profileID, context.m_graphUserID );
            m_pRagdoll->SetPoseFollowingEnabled( true );
            m_pRagdoll->SetGravityEnabled( pNodeSettings->m_isGravityEnabled );
        }

        //-------------------------------------------------------------------------

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

        //-------------------------------------------------------------------------

        // Destroy the ragdoll
        if ( m_pRagdoll != nullptr )
        {
            m_pRagdoll->RemoveFromScene();
            EE::Delete( m_pRagdoll );
        }

        PassthroughNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult PoweredRagdollNode::Update( GraphContext& context )
    {
        GraphPoseNodeResult result;

        if ( IsValid() )
        {
            result = PassthroughNode::Update( context );
            result = RegisterRagdollTasks( context, result );
        }
        else
        {
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
        }

        return result;
    }

    GraphPoseNodeResult PoweredRagdollNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        GraphPoseNodeResult result;

        if ( IsValid() )
        {
            result = PassthroughNode::Update( context, updateRange );
            result = RegisterRagdollTasks( context, result );
        }
        else
        {
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
        }

        return result;
    }

    GraphPoseNodeResult PoweredRagdollNode::RegisterRagdollTasks( GraphContext& context, GraphPoseNodeResult const& childResult )
    {
        EE_ASSERT( IsValid() );
        GraphPoseNodeResult result = childResult;

        //-------------------------------------------------------------------------

        Tasks::RagdollSetPoseTask::InitOption const initOptions = m_isFirstUpdate ? Tasks::RagdollSetPoseTask::InitializeBodies : Tasks::RagdollSetPoseTask::DoNothing;
        TaskIndex const setPoseTaskIdx = context.m_pTaskSystem->RegisterTask<Tasks::RagdollSetPoseTask>( m_pRagdoll, GetNodeIndex(), childResult.m_taskIdx, initOptions );
        m_isFirstUpdate = false;

        //-------------------------------------------------------------------------

        float const physicsWeight = ( m_pBlendWeightValueNode != nullptr ) ? m_pBlendWeightValueNode->GetValue<float>( context ) : GetSettings<PoweredRagdollNode>()->m_physicsBlendWeight;
        result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::RagdollGetPoseTask>( m_pRagdoll, GetNodeIndex(), setPoseTaskIdx, physicsWeight );
        return result;
    }
}