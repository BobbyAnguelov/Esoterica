#include "Animation_Task_Ragdoll.h"
#include "Engine/Animation/AnimationBlender.h"
#include "Engine/Physics/PhysicsRagdoll.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    RagdollSetPoseTask::RagdollSetPoseTask( Physics::Ragdoll* pRagdoll, TaskSourceID sourceID, TaskIndex sourceTaskIdx, InitOption initOption )
        : Task( sourceID, TaskUpdateStage::PrePhysics, { sourceTaskIdx } )
        , m_pRagdoll( pRagdoll )
        , m_initOption( initOption )
    {
        EE_ASSERT( pRagdoll != nullptr );
        EE_ASSERT( sourceTaskIdx != InvalidIndex );
    }

    void RagdollSetPoseTask::Execute( TaskContext const& context )
    {
        EE_PROFILE_FUNCTION_ANIMATION();
        auto pSourceBuffer = TransferDependencyPoseBuffer( context, 0 );
        pSourceBuffer->m_pose.CalculateGlobalTransforms();
        m_pRagdoll->Update( context.m_deltaTime, context.m_worldTransform, &pSourceBuffer->m_pose, m_initOption == InitializeBodies );
        MarkTaskComplete( context );
    }

    //-------------------------------------------------------------------------

    RagdollGetPoseTask::RagdollGetPoseTask( Physics::Ragdoll* pRagdoll, TaskSourceID sourceID, TaskIndex sourceTaskIdx, float const physicsBlendWeight )
        : Task( sourceID, TaskUpdateStage::PostPhysics, { sourceTaskIdx } )
        , m_pRagdoll( pRagdoll )
        , m_physicsBlendWeight( physicsBlendWeight )
    {
        EE_ASSERT( pRagdoll != nullptr );
        EE_ASSERT( sourceTaskIdx != InvalidIndex );
    }

    RagdollGetPoseTask::RagdollGetPoseTask( Physics::Ragdoll* pRagdoll, TaskSourceID sourceID )
        : Task( sourceID, TaskUpdateStage::PostPhysics, {} )
        , m_pRagdoll( pRagdoll )
    {
        EE_ASSERT( pRagdoll != nullptr );
    }

    void RagdollGetPoseTask::Execute( TaskContext const& context )
    {
        EE_PROFILE_FUNCTION_ANIMATION();
        if ( context.m_dependencies.empty() )
        {
            auto pResultBuffer = GetNewPoseBuffer( context );
            pResultBuffer->m_pose.CalculateGlobalTransforms();
            m_pRagdoll->GetPose( context.m_worldTransform, &pResultBuffer->m_pose );
        }
        else // Potentially blend the poses
        {
            // Get the animation pose
            auto pResultBuffer = TransferDependencyPoseBuffer( context, 0 );

            // Overwrite it with the physics pose
            if ( Math::IsNearEqual( m_physicsBlendWeight, 1.0f, Math::LargeEpsilon ) )
            {
                pResultBuffer->m_pose.CalculateGlobalTransforms();
                m_pRagdoll->GetPose( context.m_worldTransform, &pResultBuffer->m_pose );
            }
            else if ( m_physicsBlendWeight > Math::LargeEpsilon )
            {
                PoseBuffer* pTempBuffer = nullptr;
                int8_t const tmpBufferIdx = GetTemporaryPoseBuffer( context, pTempBuffer );
                EE_ASSERT( pTempBuffer != nullptr );

                // Get the ragdoll pose and blend it with the animation pose
                pTempBuffer->m_pose.CalculateGlobalTransforms();
                m_pRagdoll->GetPose( context.m_worldTransform, &pTempBuffer->m_pose );
                Animation::Blender::LocalBlend( &pResultBuffer->m_pose, &pTempBuffer->m_pose, m_physicsBlendWeight, nullptr, &pResultBuffer->m_pose );

                ReleaseTemporaryPoseBuffer( context, tmpBufferIdx );
            }
        }

        MarkTaskComplete( context );
    }

    #if EE_DEVELOPMENT_TOOLS
    String RagdollGetPoseTask::GetDebugText() const
    {
        if ( m_physicsBlendWeight < 1.0f )
        {
            return String( String::CtorSprintf(), "Get Ragdoll Pose (%.2f)", m_physicsBlendWeight );
        }
        else
        {
            return "Get Ragdoll Pose";
        }
    }
    #endif
}