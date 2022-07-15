#include "Animation_RuntimeGraph_Contexts.h"
#include "Animation_RuntimeGraph_RootMotionRecorder.h"
#include "System/Animation/AnimationPose.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    GraphContext::GraphContext()
    {
        #if EE_DEVELOPMENT_TOOLS
        m_activeNodes.reserve( 50 );
        #endif
    }

    void GraphContext::Initialize( uint64_t graphUserID, TaskSystem* pTaskSystem, Pose const* pPreviousPose, RootMotionRecorder* pRootMotionRecorder )
    {
        EE_ASSERT( graphUserID != 0 );
        m_graphUserID = graphUserID;

        EE_ASSERT( m_pPreviousPose == nullptr && m_pTaskSystem == nullptr && m_pSkeleton == nullptr );
        EE_ASSERT( pPreviousPose != nullptr && pTaskSystem != nullptr );

        const_cast<TaskSystem*&>( m_pTaskSystem ) = pTaskSystem;
        const_cast<Pose const*&>( m_pPreviousPose ) = pPreviousPose;
        const_cast<Skeleton const*&>( m_pSkeleton ) = pPreviousPose->GetSkeleton();

        EE_ASSERT( m_pPreviousPose->GetSkeleton() == m_pSkeleton );

        m_boneMaskPool.Initialize( m_pSkeleton );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        EE_ASSERT( pRootMotionRecorder != nullptr );
        const_cast<RootMotionRecorder const*&>( m_pRootMotionActionRecorder ) = pRootMotionRecorder;
        #endif

        //-------------------------------------------------------------------------

        m_deltaTime = 0;
        m_updateID = 0;
        m_worldTransform = Transform::Identity;
        m_worldTransformInverse = Transform::Identity;
        m_sampledEvents.Reset();
        m_branchState = BranchState::Active;
    }

    void GraphContext::Shutdown()
    {
        #if EE_DEVELOPMENT_TOOLS
        const_cast<RootMotionRecorder const*&>( m_pRootMotionActionRecorder ) = nullptr;
        #endif

        m_sampledEvents.Reset();
        m_boneMaskPool.Shutdown();

        const_cast<Pose const*&>( m_pPreviousPose ) = nullptr;
        const_cast<Skeleton const*&>( m_pSkeleton ) = nullptr;
        const_cast<TaskSystem*&>( m_pTaskSystem ) = nullptr;
    }

    void GraphContext::Update( Seconds const deltaTime, Transform const& currentWorldTransform, Physics::Scene* pPhysicsScene )
    {
        #if EE_DEVELOPMENT_TOOLS
        m_activeNodes.clear();
        #endif

        //-------------------------------------------------------------------------

        m_deltaTime = deltaTime;
        m_updateID++;
        m_sampledEvents.Reset();
        m_worldTransform = currentWorldTransform;
        m_worldTransformInverse = m_worldTransform.GetInverse();
        m_pPhysicsScene = pPhysicsScene;

        m_branchState = BranchState::Active;
        m_boneMaskPool.Reset();

        EE_ASSERT( m_pPreviousPose->HasGlobalTransforms() );
    }
}