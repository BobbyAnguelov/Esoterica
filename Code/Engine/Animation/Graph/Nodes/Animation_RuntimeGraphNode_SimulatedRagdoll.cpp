#include "Animation_RuntimeGraphNode_SimulatedRagdoll.h"
#include "Animation_RuntimeGraphNode_AnimationClip.h"
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
    void SimulatedRagdollNode::Settings::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<SimulatedRagdollNode>( context, options );

        // Set entry node
        context.SetNodePtrFromIndex( m_entryNodeIdx, pNode->m_pEntryNode );

        // Get exit node options
        pNode->m_exitNodeOptions.reserve( m_exitOptionNodeIndices.size() );
        for ( auto exitOptionIdx : m_exitOptionNodeIndices )
        {
            auto& pExitOptionNode = pNode->m_exitNodeOptions.emplace_back( nullptr );
            context.SetNodePtrFromIndex( exitOptionIdx, pExitOptionNode );
        }

        // Get ragdoll definition
        pNode->m_pRagdollDefinition = context.GetResource<Physics::RagdollDefinition>( m_dataSlotIdx );
    }

    bool SimulatedRagdollNode::IsValid() const
    {
        return PoseNode::IsValid() && m_pEntryNode->IsValid() && m_pRagdollDefinition != nullptr && m_pRagdoll != nullptr;
    }

    SyncTrack const& SimulatedRagdollNode::GetSyncTrack() const
    {
        return m_pEntryNode->GetSyncTrack();
    }

    void SimulatedRagdollNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );

        PoseNode::InitializeInternal( context, initialTime );

        // Initialize entry node
        m_pEntryNode->Initialize( context, initialTime );

        // Initialize exit options
        for ( auto pExitOptionNode : m_exitNodeOptions )
        {
            pExitOptionNode->Initialize( context, SyncTrackTime() );
        }

        // Create ragdoll
        if ( m_pRagdollDefinition != nullptr && context.m_pPhysicsScene != nullptr )
        {
            auto pNodeSettings = GetSettings<SimulatedRagdollNode>();
            m_pRagdoll = context.m_pPhysicsScene->CreateRagdoll( m_pRagdollDefinition, pNodeSettings->m_profileID, context.m_graphUserID );
            m_pRagdoll->SetPoseFollowingEnabled( true );
            m_pRagdoll->SetGravityEnabled( true );
        }

        //-------------------------------------------------------------------------

        m_isFirstUpdate = true;
    }

    void SimulatedRagdollNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        // Destroy the ragdoll
        if ( m_pRagdoll != nullptr )
        {
            m_pRagdoll->RemoveFromScene();
            EE::Delete( m_pRagdoll );
        }

        // Shutdown exit options
        for ( auto pExitOptionNode : m_exitNodeOptions )
        {
            pExitOptionNode->Shutdown( context );
        }

        // Shutdown entry node
        m_pEntryNode->Shutdown( context );

        PoseNode::ShutdownInternal( context );
    }

    GraphPoseNodeResult SimulatedRagdollNode::Update( GraphContext& context )
    {
        EE_ASSERT( IsInitialized() );

        GraphPoseNodeResult result;

        if ( IsValid() )
        {
            // Forward child node results
            result = m_pEntryNode->Update( context );
            m_duration = m_pEntryNode->GetDuration();
            m_previousTime = m_pEntryNode->GetPreviousTime();
            m_currentTime = m_pEntryNode->GetCurrentTime();

            result = RegisterRagdollTasks( context, result );
        }
        else
        {
            result.m_sampledEventRange = SampledEventRange( context.m_sampledEventsBuffer.GetNumEvents() );
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
        }

        return result;
    }

    GraphPoseNodeResult SimulatedRagdollNode::Update( GraphContext& context, SyncTrackTimeRange const& updateRange )
    {
        EE_ASSERT( IsInitialized() );

        GraphPoseNodeResult result;

        if ( IsValid() )
        {
            // Forward child node results
            result = m_pEntryNode->Update( context, updateRange );
            m_duration = m_pEntryNode->GetDuration();
            m_previousTime = m_pEntryNode->GetPreviousTime();
            m_currentTime = m_pEntryNode->GetCurrentTime();

            result = RegisterRagdollTasks( context, result );
        }
        else
        {
            result.m_sampledEventRange = SampledEventRange( context.m_sampledEventsBuffer.GetNumEvents() );
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
        }

        return result;
    }

    GraphPoseNodeResult SimulatedRagdollNode::RegisterRagdollTasks( GraphContext& context, GraphPoseNodeResult const& childResult )
    {
        EE_ASSERT( IsValid() );
        GraphPoseNodeResult result = childResult;

        //-------------------------------------------------------------------------

        Tasks::RagdollSetPoseTask::InitOption const initOptions = m_isFirstUpdate ? Tasks::RagdollSetPoseTask::InitializeBodies : Tasks::RagdollSetPoseTask::DoNothing;
        TaskIndex const setPoseTaskIdx = context.m_pTaskSystem->RegisterTask<Tasks::RagdollSetPoseTask>( m_pRagdoll, GetNodeIndex(), childResult.m_taskIdx, initOptions );
        m_isFirstUpdate = false;

        //-------------------------------------------------------------------------

        result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::RagdollGetPoseTask>( m_pRagdoll, GetNodeIndex(), setPoseTaskIdx, 1.0f );
        return result;
    }
}