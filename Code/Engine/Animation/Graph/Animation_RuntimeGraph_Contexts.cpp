#include "Animation_RuntimeGraph_Contexts.h"
#include "Animation_RuntimeGraph_Events.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/AnimationBoneMask.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    GraphContext::GraphContext( uint64_t userID, Skeleton const* pSkeleton )
        : m_graphUserID( userID )
        , m_pSkeleton( pSkeleton )
        , m_boneMaskPool( pSkeleton )
    {
        EE_ASSERT( m_graphUserID != 0 );
        EE_ASSERT( m_pSkeleton != nullptr );
    }

    GraphContext::~GraphContext()
    {
        EE_ASSERT( m_pTaskSystem == nullptr );
        EE_ASSERT( m_pPreviousPose == nullptr );
    }

    void GraphContext::Initialize( TaskSystem* pTaskSystem )
    {

        EE_ASSERT( m_pTaskSystem == nullptr && m_pPreviousPose == nullptr );
        EE_ASSERT( pTaskSystem != nullptr );
        m_pTaskSystem = pTaskSystem;
        m_pPreviousPose = pTaskSystem->GetPose();
    }

    void GraphContext::Shutdown()
    {
         m_pTaskSystem = nullptr;
         m_pPreviousPose = nullptr;
    }

    void GraphContext::Update( Seconds const deltaTime, Transform const& currentWorldTransform, Physics::Scene* pPhysicsScene )
    {
        m_deltaTime = deltaTime;
        m_updateID++;
        m_worldTransform = currentWorldTransform;
        m_worldTransformInverse = m_worldTransform.GetInverse();
        m_pPhysicsScene = pPhysicsScene;
        m_branchState = BranchState::Active;
        m_boneMaskPool.Reset();
        m_sampledEventsBuffer.Reset();
    }

    #if EE_DEVELOPMENT_TOOLS
    void GraphContext::SetDebugSystems( RootMotionDebugger* pRootMotionRecorder, TVector<int16_t>* pActiveNodesList )
    {
        EE_ASSERT( m_pRootMotionActionRecorder == nullptr );
        EE_ASSERT( m_pActiveNodes == nullptr );

        m_pRootMotionActionRecorder = pRootMotionRecorder;
        m_pActiveNodes = pActiveNodesList;

        EE_ASSERT( m_pRootMotionActionRecorder != nullptr );
        EE_ASSERT( m_pActiveNodes != nullptr );
    }
    #endif
}