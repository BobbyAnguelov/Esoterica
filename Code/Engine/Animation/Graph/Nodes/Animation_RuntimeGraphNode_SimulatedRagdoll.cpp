#include "Animation_RuntimeGraphNode_SimulatedRagdoll.h"
#include "Animation_RuntimeGraphNode_AnimationClip.h"
#include "Engine/Animation/Events/AnimationEvent_Ragdoll.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_Ragdoll.h"
#include "Engine/Animation/TaskSystem/Tasks/Animation_Task_DefaultPose.h"
#include "Engine/Physics/Ragdoll/PhysicsRagdoll_Instance.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void SimulatedRagdollNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<SimulatedRagdollNode>( context, options );

        // Set entry node
        context.SetNodePtrFromIndex( m_entryNodeIdx, pNode->m_pEntryNode );

        // Exit condition
        context.SetOptionalNodePtrFromIndex( m_exitConditionNodeIdx, pNode->m_pExitConditionNode );

        // Get exit node options
        pNode->m_exitNodeOptions.reserve( m_exitOptionNodeIndices.size() );
        for ( auto exitOptionIdx : m_exitOptionNodeIndices )
        {
            auto& pExitOptionNode = pNode->m_exitNodeOptions.emplace_back( nullptr );
            context.SetNodePtrFromIndex( exitOptionIdx, pExitOptionNode );
        }

        // Get ragdoll definition
        pNode->m_pRagdollDefinition = context.GetResourceForSlot<Physics::RagdollDefinition>( m_dataSlotIdx );
    }

    bool SimulatedRagdollNode::IsValid() const
    {
        if ( !PoseNode::IsValid() )
        {
            return false;
        }

        if ( m_pRagdollDefinition == nullptr )
        {
            return false;
        }

        if ( m_stage == Stage::Invalid )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        if ( m_stage == Stage::FullyInEntryAnim || m_stage == Stage::BlendToRagdoll )
        {
            if ( !m_isFirstUpdate && m_pRagdoll == nullptr )
            {
                return false;
            }

            if ( !m_pEntryNode->IsValid()  )
            {
                return false;
            }
        }
        else if ( m_stage == Stage::FullyInRagdoll )
        {
            if ( m_pRagdoll == nullptr )
            {
                return false;
            }
        }
        else if ( m_stage == Stage::BlendOutOfRagdoll )
        {
            if ( m_pRagdoll == nullptr || m_pExitNode == nullptr || !m_pExitNode->IsValid() )
            {
                return false;
            }
        }
        else if ( m_stage == Stage::FullyInExitAnim )
        {
            if ( m_pExitNode == nullptr || !m_pExitNode->IsValid() )
            {
                return false;
            }
        }

        return true;
    }

    SyncTrack const& SimulatedRagdollNode::GetSyncTrack() const
    {
        return m_pEntryNode->GetSyncTrack();
    }

    void SimulatedRagdollNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        EE_ASSERT( context.IsValid() );

        PoseNode::InitializeInternal( context, initialTime );
        SetStage( Stage::FullyInEntryAnim );

        // Initialize entry node
        m_pEntryNode->Initialize( context, initialTime );

        // Shutdown exit condition node
        if ( m_pExitConditionNode != nullptr )
        {
            m_pExitConditionNode->Initialize( context );
        }

        // Initialize exit options
        for ( auto pExitOptionNode : m_exitNodeOptions )
        {
            pExitOptionNode->Initialize( context, SyncTrackTime() );
        }

        m_pExitNode = nullptr;
    }

    void SimulatedRagdollNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );

        DestroyRagdoll( context );

        m_pExitNode = nullptr;

        // Shutdown exit options
        for ( auto pExitOptionNode : m_exitNodeOptions )
        {
            pExitOptionNode->Shutdown( context );
        }

        // Shutdown exit condition node
        if ( m_pExitConditionNode != nullptr )
        {
            m_pExitConditionNode->Shutdown( context );
        }

        // Shutdown entry node
        m_pEntryNode->Shutdown( context );

        PoseNode::ShutdownInternal( context );
    }

    void SimulatedRagdollNode::CreateRagdoll( GraphContext& context )
    {
        EE_ASSERT( m_pRagdoll == nullptr );
        EE_ASSERT( m_pRagdollDefinition != nullptr );

        #if EE_DEVELOPMENT_TOOLS
        if ( context.m_pPhysicsWorld == nullptr )
        {
            context.LogError( GetNodeIndex(), "Trying to create a ragdoll with no physics world set" );
            return;
        }
        #endif

        auto pNodeDefinition = GetDefinition<SimulatedRagdollNode>();
        m_pRagdoll = EE::New<Physics::Ragdoll>( context.m_pPhysicsWorld, m_pRagdollDefinition, context.m_instanceID );
        m_pRagdoll->SetPoseFollowing( true );
    }

    void SimulatedRagdollNode::DestroyRagdoll( GraphContext& context )
    {
        if ( m_pRagdoll != nullptr )
        {
            EE::Delete( m_pRagdoll );
        }
    }

    GraphPoseNodeResult SimulatedRagdollNode::Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
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
                    result = UpdateExit( context, pUpdateRange );
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
            result.m_taskIdx = context.GetTaskSystem()->RegisterTask<ReferencePoseTask>( GetNodePath( context ) );
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

        // Create the ragdoll
        //-------------------------------------------------------------------------

        if ( m_stage == Stage::FullyInEntryAnim && m_isFirstUpdate )
        {
            CreateRagdoll( context );

            if ( m_pRagdoll != nullptr )
            {
                // Check if we have any ragdoll events
                auto pEntryAnim = m_pEntryNode->GetAnimation();
                auto const& events = pEntryAnim->GetEvents();
                for ( auto pEvent : events )
                {
                    if ( IsOfType<RagdollEvent>( pEvent ) )
                    {
                        m_entryHasRagdollEvent = true;
                        break;
                    }
                }

                // If we have an event, go to fully in and when to start blending
                if ( m_entryHasRagdollEvent )
                {
                    SetStage( Stage::FullyInEntryAnim );
                    m_pRagdoll->PutToSleep();
                }
                else
                {
                    SetStage( Stage::BlendToRagdoll );
                    m_pRagdoll->SetPoseFollowing( true );
                }
            }
            else
            {
                SetStage( Stage::Invalid );
            }
        }

        // Wait for ragdoll event
        //-------------------------------------------------------------------------

        if ( m_stage == Stage::FullyInEntryAnim )
        {
            for ( auto i = result.m_sampledEventRange.m_startIdx; i < result.m_sampledEventRange.m_endIdx; i++ )
            {
                SampledEvent const& sampledEvent = context.GetSampledEventsBuffer()->GetEvent(i);
                if ( sampledEvent.IsAnimationEvent() && sampledEvent.IsEventOfType<RagdollEvent>() )
                {
                    SetStage( Stage::BlendToRagdoll );
                    m_pRagdoll->WakeUp();
                    break;
                }
            }
        }

        // Blend to ragdoll
        //-------------------------------------------------------------------------

        if ( m_stage == Stage::BlendToRagdoll )
        {
            float physicsBlendWeight = 0.0f;

            if ( m_entryHasRagdollEvent )
            {
                // Try get ragdoll event and the blend weight from it
                for ( auto i = result.m_sampledEventRange.m_startIdx; i < result.m_sampledEventRange.m_endIdx; i++ )
                {
                    SampledEvent const& sampledEvent = context.GetSampledEventsBuffer()->GetEvent( i );
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
            }
            else // Simple blend over time
            {
                auto pDefinition = GetDefinition<SimulatedRagdollNode>();
                m_currentBlendTime += context.m_deltaTime;
                physicsBlendWeight = Math::Min( ( m_currentBlendTime / pDefinition->m_blendTime ).ToFloat(), 1.0f );
            }

            // Register ragdoll task
            RagdollSetPoseTask::InitOption const initOptions = m_isFirstUpdate ? RagdollSetPoseTask::InitializeBodies : RagdollSetPoseTask::DoNothing;
            int8_t const setPoseTaskIdx = context.GetTaskSystem()->RegisterTask<RagdollSetPoseTask>( GetNodePath( context ), m_pRagdoll, result.m_taskIdx, initOptions );
            result.m_taskIdx = context.GetTaskSystem()->RegisterTask<RagdollGetPoseTask>( GetNodePath( context ), m_pRagdoll, setPoseTaskIdx, physicsBlendWeight );

            // Once we hit fully in physics, switch stage
            if ( physicsBlendWeight >= 1.0f )
            {
                auto pNodeDefinition = GetDefinition<SimulatedRagdollNode>();
                m_pRagdoll->SetPoseFollowing( false );
                SetStage( Stage::FullyInRagdoll );
            }
        }

        return result;
    }

    GraphPoseNodeResult SimulatedRagdollNode::UpdateSimulated( GraphContext& context )
    {
        EE_ASSERT( m_stage == Stage::FullyInRagdoll );

        GraphPoseNodeResult result;
        m_duration = 0.0f;
        m_previousTime = m_currentTime = 0.0f;

        result.m_sampledEventRange = context.GetEmptySampledEventRange();
        result.m_taskIdx = context.GetTaskSystem()->RegisterTask<RagdollGetPoseTask>( GetNodePath( context ), m_pRagdoll );

        // TODO: EMIT ROOT MOTION!!!

        if ( !m_exitNodeOptions.empty() && m_pExitConditionNode != nullptr && m_pExitConditionNode->GetValue<bool>( context ) )
        {
            SelectExitNode( context );
            SetStage( Stage::BlendOutOfRagdoll );
        }

        return result;
    }

    GraphPoseNodeResult SimulatedRagdollNode::UpdateExit( GraphContext& context, SyncTrackTimeRange const* pUpdateRange )
    {
        EE_ASSERT( m_pExitNode != nullptr );

        SyncTrackTimeRange const firstFrameUpdateRange;
        SyncTrackTimeRange const* pFinalUpdateRange = ( m_stage == Stage::BlendOutOfRagdoll ) ? &firstFrameUpdateRange : pUpdateRange;

        //-------------------------------------------------------------------------

        GraphPoseNodeResult result = m_pExitNode->Update( context, pFinalUpdateRange );
        m_duration = m_pExitNode->GetDuration();
        m_previousTime = m_pExitNode->GetPreviousTime();
        m_currentTime = m_pExitNode->GetCurrentTime();

        // Use ragdoll to blend to exit anim pose
        //-------------------------------------------------------------------------

        if ( m_stage == Stage::BlendOutOfRagdoll )
        {
            auto pDefinition = GetDefinition<SimulatedRagdollNode>();

            m_currentBlendTime += context.m_deltaTime;
            float const physicsBlendWeight = 1.0f - Math::Min( ( m_currentBlendTime / pDefinition->m_blendTime).ToFloat(), 1.0f );

            // TODO: FIX UP ROOT MOTION!!!

            // Register ragdoll task
            RagdollSetPoseTask::InitOption const initOptions = m_isFirstUpdate ? RagdollSetPoseTask::InitializeBodies : RagdollSetPoseTask::DoNothing;
            int8_t const setPoseTaskIdx = context.GetTaskSystem()->RegisterTask<RagdollSetPoseTask>( GetNodePath( context ), m_pRagdoll, result.m_taskIdx, initOptions );
            result.m_taskIdx = context.GetTaskSystem()->RegisterTask<RagdollGetPoseTask>( GetNodePath( context ), m_pRagdoll, setPoseTaskIdx, physicsBlendWeight );

            // Once we hit fully in physics, switch stage
            if ( physicsBlendWeight == 0.0f )
            {
                auto pNodeDefinition = GetDefinition<SimulatedRagdollNode>();
                m_pRagdoll->SetPoseFollowing( false );
                SetStage( Stage::FullyInExitAnim );
            }
        }

        return result;
    }

    void SimulatedRagdollNode::SelectExitNode( GraphContext& context )
    {
        EE_ASSERT( !m_exitNodeOptions.empty() && m_pExitNode == nullptr );

        // TODO: pose match
        m_pExitNode = m_exitNodeOptions[0];
    }
}