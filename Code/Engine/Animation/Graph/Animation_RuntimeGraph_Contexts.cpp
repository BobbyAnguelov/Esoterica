#include "Animation_RuntimeGraph_Contexts.h"
#include "Animation_RuntimeGraph_Events.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/AnimationBoneMask.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    #if EE_DEVELOPMENT_TOOLS
    void InstantiationContext::LogWarning( char const* pFormat, ... ) const
    {
        auto& entry = m_pLog->emplace_back();
        entry.m_updateID = 0;
        entry.m_nodeIdx = m_currentNodeIdx;
        entry.m_severity = Log::Severity::Warning;

        va_list args;
        va_start( args, pFormat );
        entry.m_message.append_sprintf_va_list( pFormat, args );
        va_end( args );
    }
    #endif

    //-------------------------------------------------------------------------

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

        #if EE_DEVELOPMENT_TOOLS
        m_pActiveNodes = nullptr;
        m_pRootMotionDebugger = nullptr;
        m_pLog = nullptr;
        #endif
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
        m_sampledEventsBuffer.Clear();
    }

    #if EE_DEVELOPMENT_TOOLS
    void GraphContext::SetDebugSystems( RootMotionDebugger* pRootMotionRecorder, TVector<int16_t>* pActiveNodesList, TVector<GraphLogEntry>* pLog )
    {
        EE_ASSERT( m_pRootMotionDebugger == nullptr );
        EE_ASSERT( m_pActiveNodes == nullptr );
        EE_ASSERT( m_pLog == nullptr );

        m_pRootMotionDebugger = pRootMotionRecorder;
        m_pActiveNodes = pActiveNodesList;
        m_pLog = pLog;

        EE_ASSERT( m_pRootMotionDebugger != nullptr );
        EE_ASSERT( m_pActiveNodes != nullptr );
        EE_ASSERT( m_pLog != nullptr );
    }

    void GraphContext::LogWarning( int16_t nodeIdx, char const* pFormat, ... )
    {
        auto& entry = m_pLog->emplace_back();
        entry.m_updateID = m_updateID;
        entry.m_nodeIdx = nodeIdx;
        entry.m_severity = Log::Severity::Warning;

        va_list args;
        va_start( args, pFormat );
        entry.m_message.append_sprintf_va_list( pFormat, args );
        va_end( args );
    }

    void GraphContext::LogError( int16_t nodeIdx, char const* pFormat, ... )
    {
        auto& entry = m_pLog->emplace_back();
        entry.m_updateID = m_updateID;
        entry.m_nodeIdx = nodeIdx;
        entry.m_severity = Log::Severity::Error;
        
        va_list args;
        va_start( args, pFormat );
        entry.m_message.append_sprintf_va_list( pFormat, args );
        va_end( args );
    }
    #endif
}