#include "Animation_RuntimeGraph_Context.h"
#include "Animation_RuntimeGraph_SampledEvents.h"
#include "Animation_RuntimeGraph_RootMotionDebugger.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    static std::atomic<int32_t> g_instanceID = 1;

    static int32_t GenerateInstanceID()
    {
        int32_t ID = g_instanceID++;
        EE_ASSERT( ID != INT32_MAX );
        return ID;
    }

    //-------------------------------------------------------------------------

    GraphContext::GraphContext( Skeleton const* pSkeleton )
        : m_instanceID( GenerateInstanceID() )
        , m_pSkeleton( pSkeleton )
    {
        EE_ASSERT( m_instanceID != 0 );
        EE_ASSERT( m_pSkeleton != nullptr );
    }

    GraphContext::~GraphContext()
    {
        EE_ASSERT( m_pTaskSystem == nullptr );
        EE_ASSERT( m_pSampledEventsBuffer == nullptr );
        EE_ASSERT( !m_isInitialized );
    }

    void GraphContext::Initialize( GraphContext *pParentContext )
    {
        EE_ASSERT( !m_isInitialized );
        EE_ASSERT( m_pTaskSystem == nullptr && m_pSampledEventsBuffer == nullptr );

        if ( pParentContext != nullptr )
        {
            m_pTaskSystem = pParentContext->m_pTaskSystem;
            m_pSampledEventsBuffer = pParentContext->m_pSampledEventsBuffer;

            #if EE_DEVELOPMENT_TOOLS
            m_basePath = pParentContext->m_basePath;
            m_pLog = pParentContext->m_pLog;
            m_pRootMotionDebugger = pParentContext->m_pRootMotionDebugger;
            #endif
        }
        else
        {
            m_pTaskSystem = EE::New<TaskSystem>( m_pSkeleton );
            m_pSampledEventsBuffer = EE::New<SampledEventsBuffer>();

            #if EE_DEVELOPMENT_TOOLS
            m_pLog = EE::New<TVector<GraphLogEntry>>();
            m_pRootMotionDebugger = EE::New<RootMotionDebugger>();
            #endif
        }

        m_activeNodes.reserve( 50 );
        m_activeNodes.clear();

        m_isStandaloneGraphContext = ( pParentContext == nullptr );
        m_isInitialized = true;
    }

    void GraphContext::Shutdown()
    {
        EE_ASSERT( m_isInitialized );

        if ( m_isStandaloneGraphContext )
        {
            EE::Delete( m_pTaskSystem );
            EE::Delete( m_pSampledEventsBuffer );

            #if EE_DEVELOPMENT_TOOLS
            EE::Delete( m_pLog );
            EE::Delete( m_pRootMotionDebugger );
            #endif
        }
        else
        {
            m_pTaskSystem = nullptr;
            m_pSampledEventsBuffer = nullptr;

            #if EE_DEVELOPMENT_TOOLS
            m_pLog = nullptr;
            m_pRootMotionDebugger = nullptr;
            #endif
        }

        m_activeNodes.clear();
        m_isStandaloneGraphContext = true;
        m_isInitialized = false;
    }

    void GraphContext::Update( Seconds const deltaTime, Transform const& currentWorldTransform, Physics::PhysicsWorld* pPhysicsWorld )
    {
        EE_ASSERT( IsValid() );

        m_deltaTime = deltaTime;
        m_updateID++;
        m_worldTransform = currentWorldTransform;
        m_worldTransformInverse = m_worldTransform.GetInverse();
        m_pPhysicsWorld = pPhysicsWorld;
        m_branchState = BranchState::Active;
    }

    PoseBuffer const* GraphContext::GetPreviousPoseBuffer() const
    {
        return m_pTaskSystem->GetPoseBufferForPreviousUpdate();
    }

    Pose const* GraphContext::GetPreviousPrimaryPose() const
    {
        return m_pTaskSystem->GetPrimaryPoseForPreviousUpdate();
    }

    void GraphContext::TransferContextDataFromParent( GraphContext const &parentContext )
    {
        m_pLayerContext = parentContext.m_pLayerContext;
        m_basePath = parentContext.m_basePath;
        m_deltaTime = parentContext.m_deltaTime;
        m_worldTransform = parentContext.m_worldTransform;
        m_branchState = parentContext.m_branchState;
        m_pInitializationTimeInfo = parentContext.m_pInitializationTimeInfo;
    }

    #if EE_DEVELOPMENT_TOOLS
    void GraphContext::LogWarning( int16_t nodeIdx, char const* pFormat, ... )
    {
        GraphLogEntry& entry = m_pLog->emplace_back();
        entry.m_updateID = m_updateID;
        entry.m_sourcePath = GetBasePath();
        entry.m_sourcePath.Push( nodeIdx );
        entry.m_severity = Severity::Warning;

        va_list args;
        va_start( args, pFormat );
        entry.m_message.append_sprintf_va_list( pFormat, args );
        va_end( args );
    }

    void GraphContext::LogError( int16_t nodeIdx, char const* pFormat, ... )
    {
        GraphLogEntry& entry = m_pLog->emplace_back();
        entry.m_updateID = m_updateID;
        entry.m_sourcePath = GetBasePath();
        entry.m_sourcePath.Push( nodeIdx );
        entry.m_severity = Severity::Error;
        
        va_list args;
        va_start( args, pFormat );
        entry.m_message.append_sprintf_va_list( pFormat, args );
        va_end( args );
    }
    #endif
}
