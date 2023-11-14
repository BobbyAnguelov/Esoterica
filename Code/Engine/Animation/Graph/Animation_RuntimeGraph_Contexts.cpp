#include "Animation_RuntimeGraph_Contexts.h"
#include "Animation_RuntimeGraph_Events.h"
#include "Animation_RuntimeGraph_LayerData.h"
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
    {
        EE_ASSERT( m_graphUserID != 0 );
        EE_ASSERT( m_pSkeleton != nullptr );
    }

    GraphContext::~GraphContext()
    {
        EE_ASSERT( m_pTaskSystem == nullptr );
    }

    void GraphContext::Initialize( TaskSystem* pTaskSystem, SampledEventsBuffer* pSampledEventsBuffer )
    {
        EE_ASSERT( m_pTaskSystem == nullptr && m_pSampledEventsBuffer == nullptr );
        EE_ASSERT( pTaskSystem != nullptr );
        EE_ASSERT( pSampledEventsBuffer != nullptr );
        m_pTaskSystem = pTaskSystem;
        m_pSampledEventsBuffer = pSampledEventsBuffer;
        m_pPreviousPose = pTaskSystem->GetPrimaryPose();
    }

    void GraphContext::Shutdown()
    {
        m_pTaskSystem = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        m_pActiveNodes = nullptr;
        m_pRootMotionDebugger = nullptr;
        m_pLog = nullptr;
        #endif
    }

    void GraphContext::Update( Seconds const deltaTime, Transform const& currentWorldTransform, Physics::PhysicsWorld* pPhysicsWorld )
    {
        m_deltaTime = deltaTime;
        m_updateID++;
        m_worldTransform = currentWorldTransform;
        m_worldTransformInverse = m_worldTransform.GetInverse();
        m_pPhysicsWorld = pPhysicsWorld;
        m_branchState = BranchState::Active;
    }

    PoseBuffer const* GraphContext::GetPreviousPoseBuffer() const
    {
        return m_pTaskSystem->GetPoseBuffer();
    }

    Pose const* GraphContext::GetPreviousPrimaryPose() const
    {
        return m_pTaskSystem->GetPrimaryPose();
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
