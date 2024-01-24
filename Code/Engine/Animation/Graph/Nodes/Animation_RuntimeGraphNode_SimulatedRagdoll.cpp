#include "Animation_RuntimeGraphNode_SimulatedRagdoll.h"
#include "Animation_RuntimeGraphNode_AnimationClip.h"
#include "Engine/Animation/Events/AnimationEvent_Ragdoll.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_DataSet.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Ragdoll.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Physics/PhysicsRagdoll.h"
#include "Engine/Physics/PhysicsWorld.h"


//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void SimulatedRagdollNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
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
        pNode->m_pRagdollDefinition = context.m_pDataSet->GetResource<Physics::RagdollDefinition>( m_dataSlotIdx );
    }

    bool SimulatedRagdollNode::IsValid() const
    {
        if ( m_isFirstUpdate )
        {
            return PoseNode::IsValid() && m_pEntryNode->IsValid();
        }

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
        m_stage = Stage::Invalid;
        m_isFirstUpdate = true;

        // Initialize entry node
        m_pEntryNode->Initialize( context, initialTime );

        //-------------------------------------------------------------------------

        // Initialize exit options
        for ( auto pExitOptionNode : m_exitNodeOptions )
        {
            pExitOptionNode->Initialize( context, SyncTrackTime() );
        }
    }

    void SimulatedRagdollNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        // Destroy the ragdoll
        if ( m_pRagdoll != nullptr )
        {
            context.m_pPhysicsWorld->DestroyRagdoll( m_pRagdoll );
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

    void SimulatedRagdollNode::CreateRagdoll( GraphContext& context )
    {
        if ( m_pEntryNode->IsValid() && m_pRagdollDefinition != nullptr && context.m_pPhysicsWorld != nullptr )
        {
            auto pNodeDefinition = GetDefinition<SimulatedRagdollNode>();

            // Validate profile IDs
            bool isValidSetup = m_pRagdollDefinition->HasProfile( pNodeDefinition->m_entryProfileID );
            isValidSetup &= m_pRagdollDefinition->HasProfile( pNodeDefinition->m_simulatedProfileID );
            if ( pNodeDefinition->m_exitOptionNodeIndices.size() > 0 )
            {
                isValidSetup &= m_pRagdollDefinition->HasProfile( pNodeDefinition->m_exitProfileID );
            }

            // Create ragdoll and set initial stage
            if ( isValidSetup )
            {
                m_pRagdoll = context.m_pPhysicsWorld->CreateRagdoll( m_pRagdollDefinition, pNodeDefinition->m_entryProfileID, context.m_graphUserID );
                m_pRagdoll->SetPoseFollowingEnabled( true );
                m_pRagdoll->SetGravityEnabled( true );

                //-------------------------------------------------------------------------

                // Set stage to start blending immediately, if we find an event below, we'll switch to fully in anim and wait
                m_stage = Stage::BlendToRagdoll;

                // Try to find a ragdoll event
                auto pEntryAnim = m_pEntryNode->GetAnimation();
                auto const& events = pEntryAnim->GetEvents();
                for ( auto pEvent : events )
                {
                    if ( IsOfType<RagdollEvent>( pEvent ) )
                    {
                        m_stage = Stage::FullyInEntryAnim;
                        m_pRagdoll->PutToSleep();
                        break;
                    }
                }
            }
        }
    }

    GraphPoseNodeResult SimulatedRagdollNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        EE_ASSERT( IsInitialized() );

        if ( m_isFirstUpdate )
        {
            CreateRagdoll( context );
        }

        //-------------------------------------------------------------------------

        GraphPoseNodeResult result;

        if ( IsValid() )
        {
            switch ( m_stage )
            {
                case Stage::FullyInEntryAnim:
                case Stage::BlendToRagdoll:
                {
                    result = UpdateEntry( context, pUpdateRange );
                }
                break;

                case Stage::FullyInRagdoll:
                {
                    result = UpdateSimulated( context );
                }
                break;

                case Stage::BlendOutOfRagdoll:
                case Stage::FullyInExitAnim:
                {
                    result = UpdateExit( context );
                }
                break;

                default:
                EE_UNREACHABLE_CODE();
                break;
            }
        }
        else
        {
            result.m_sampledEventRange = context.GetEmptySampledEventRange();
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::ReferencePoseTask>( GetNodeIndex() );
        }

        //-------------------------------------------------------------------------

        m_isFirstUpdate = false;

        return result;
    }

    GraphPoseNodeResult SimulatedRagdollNode::UpdateEntry( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        GraphPoseNodeResult result = m_pEntryNode->Update( context, pUpdateRange );
        m_duration = m_pEntryNode->GetDuration();
        m_previousTime = m_pEntryNode->GetPreviousTime();
        m_currentTime = m_pEntryNode->GetCurrentTime();

        // Wait for ragdoll event
        //-------------------------------------------------------------------------

        if ( m_stage == Stage::FullyInEntryAnim )
        {
            for ( auto i = result.m_sampledEventRange.m_startIdx; i < result.m_sampledEventRange.m_endIdx; i++ )
            {
                SampledEvent const& sampledEvent = context.m_pSampledEventsBuffer->GetEvent(i);
                if ( sampledEvent.IsAnimationEvent() && sampledEvent.IsEventOfType<RagdollEvent>() )
                {
                    m_stage = Stage::BlendToRagdoll;
                    break;
                }
            }
        }

        // Blend to ragdoll
        //-------------------------------------------------------------------------

        if ( m_stage == Stage::BlendToRagdoll )
        {
            float physicsBlendWeight = m_pEntryNode->GetCurrentTime();

            // Try get ragdoll event and the blend weight from it
            for ( auto i = result.m_sampledEventRange.m_startIdx; i < result.m_sampledEventRange.m_endIdx; i++ )
            {
                SampledEvent const& sampledEvent = context.m_pSampledEventsBuffer->GetEvent( i );
                if ( sampledEvent.IsAnimationEvent() && sampledEvent.IsEventOfType<RagdollEvent>() )
                {
                    auto pRagDollEvent = sampledEvent.GetEvent<RagdollEvent>();
                    if ( pRagDollEvent != nullptr )
                    {
                        physicsBlendWeight = pRagDollEvent->GetPhysicsBlendWeight( sampledEvent.GetPercentageThrough() );
                        break;
                    }
                }
            }

            // Register ragdoll task
            Tasks::RagdollSetPoseTask::InitOption const initOptions = m_isFirstUpdate ? Tasks::RagdollSetPoseTask::InitializeBodies : Tasks::RagdollSetPoseTask::DoNothing;
            TaskIndex const setPoseTaskIdx = context.m_pTaskSystem->RegisterTask<Tasks::RagdollSetPoseTask>( m_pRagdoll, GetNodeIndex(), result.m_taskIdx, initOptions );
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::RagdollGetPoseTask>( m_pRagdoll, GetNodeIndex(), setPoseTaskIdx, physicsBlendWeight );

            // Once we hit fully in physics, switch stage
            if ( physicsBlendWeight >= 1.0f )
            {
                auto pNodeDefinition = GetDefinition<SimulatedRagdollNode>();
                m_pRagdoll->SwitchProfile( pNodeDefinition->m_simulatedProfileID );
                m_stage = Stage::FullyInRagdoll;
            }
        }

        return result;
    }

    GraphPoseNodeResult SimulatedRagdollNode::UpdateSimulated( GraphContext& context )
    {
        GraphPoseNodeResult result;
        result.m_sampledEventRange = context.GetEmptySampledEventRange();
        result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::RagdollGetPoseTask>( m_pRagdoll, GetNodeIndex() );
        return result;
    }

    GraphPoseNodeResult SimulatedRagdollNode::UpdateExit( GraphContext& context )
    {
        GraphPoseNodeResult result = m_pEntryNode->Update( context );
        return result;
    }
}