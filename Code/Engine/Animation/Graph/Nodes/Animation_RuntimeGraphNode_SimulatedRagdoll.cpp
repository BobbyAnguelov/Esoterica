#include "Animation_RuntimeGraphNode_SimulatedRagdoll.h"
#include "Animation_RuntimeGraphNode_AnimationClip.h"
#include "Engine/Animation/Events/AnimationEvent_Ragdoll.h"
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
        m_stage = Stage::Invalid;
        m_isFirstRagdollUpdate = true;

        // Initialize entry node
        m_pEntryNode->Initialize( context, initialTime );

        //-------------------------------------------------------------------------

        // Initialize exit options
        for ( auto pExitOptionNode : m_exitNodeOptions )
        {
            pExitOptionNode->Initialize( context, SyncTrackTime() );
        }

        //-------------------------------------------------------------------------

        // Create ragdoll
        if ( m_pEntryNode->IsValid() && m_pRagdollDefinition != nullptr && context.m_pPhysicsScene != nullptr )
        {
            auto pNodeSettings = GetSettings<SimulatedRagdollNode>();

            // Validate profile IDs
            bool isValidSetup = m_pRagdollDefinition->HasProfile( pNodeSettings->m_entryProfileID );
            isValidSetup &= m_pRagdollDefinition->HasProfile( pNodeSettings->m_simulatedProfileID );
            if ( pNodeSettings->m_exitOptionNodeIndices.size() > 0 )
            {
                isValidSetup &= m_pRagdollDefinition->HasProfile( pNodeSettings->m_exitProfileID );
            }

            // Create ragdoll and set initial stage
            if ( isValidSetup )
            {
                m_pRagdoll = context.m_pPhysicsScene->CreateRagdoll( m_pRagdollDefinition, pNodeSettings->m_entryProfileID, context.m_graphUserID );
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
            switch ( m_stage )
            {
                case Stage::FullyInEntryAnim:
                case Stage::BlendToRagdoll:
                {
                    result = UpdateEntry( context );
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
            result.m_sampledEventRange = SampledEventRange( context.m_sampledEventsBuffer.GetNumEvents() );
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::DefaultPoseTask>( GetNodeIndex(), Pose::Type::ReferencePose );
        }

        return result;
    }

    GraphPoseNodeResult SimulatedRagdollNode::UpdateEntry( GraphContext& context )
    {
        GraphPoseNodeResult result = m_pEntryNode->Update( context );
        m_duration = m_pEntryNode->GetDuration();
        m_previousTime = m_pEntryNode->GetPreviousTime();
        m_currentTime = m_pEntryNode->GetCurrentTime();

        // Wait for ragdoll event
        //-------------------------------------------------------------------------

        if ( m_stage == Stage::FullyInEntryAnim )
        {
            auto const& sampledEvents = context.m_sampledEventsBuffer.GetEvents();
            for ( auto i = result.m_sampledEventRange.m_startIdx; i < result.m_sampledEventRange.m_endIdx; i++ )
            {
                if ( sampledEvents[i].IsEventOfType<RagdollEvent>() )
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
            auto const& sampledEvents = context.m_sampledEventsBuffer.GetEvents();
            for ( auto i = result.m_sampledEventRange.m_startIdx; i < result.m_sampledEventRange.m_endIdx; i++ )
            {
                auto pRagDollEvent = sampledEvents[i].TryGetEvent<RagdollEvent>();
                if ( pRagDollEvent != nullptr )
                {
                    physicsBlendWeight = pRagDollEvent->GetPhysicsBlendWeight( sampledEvents[i].GetPercentageThrough() );
                    break;
                }
            }

            // Register ragdoll task
            Tasks::RagdollSetPoseTask::InitOption const initOptions = m_isFirstRagdollUpdate ? Tasks::RagdollSetPoseTask::InitializeBodies : Tasks::RagdollSetPoseTask::DoNothing;
            TaskIndex const setPoseTaskIdx = context.m_pTaskSystem->RegisterTask<Tasks::RagdollSetPoseTask>( m_pRagdoll, GetNodeIndex(), result.m_taskIdx, initOptions );
            result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::RagdollGetPoseTask>( m_pRagdoll, GetNodeIndex(), setPoseTaskIdx, physicsBlendWeight );
            m_isFirstRagdollUpdate = false;

            // Once we hit fully in physics, switch stage
            if ( physicsBlendWeight >= 1.0f )
            {
                auto pNodeSettings = GetSettings<SimulatedRagdollNode>();
                m_pRagdoll->SwitchProfile( pNodeSettings->m_simulatedProfileID );
                m_stage = Stage::FullyInRagdoll;
            }
        }

        return result;
    }

    GraphPoseNodeResult SimulatedRagdollNode::UpdateSimulated( GraphContext& context )
    {
        GraphPoseNodeResult result;
        result.m_sampledEventRange = SampledEventRange( context.m_sampledEventsBuffer.GetNumEvents() );
        result.m_taskIdx = context.m_pTaskSystem->RegisterTask<Tasks::RagdollGetPoseTask>( m_pRagdoll, GetNodeIndex() );
        return result;
    }

    GraphPoseNodeResult SimulatedRagdollNode::UpdateExit( GraphContext& context )
    {
        GraphPoseNodeResult result = m_pEntryNode->Update( context );
        return result;
    }
}