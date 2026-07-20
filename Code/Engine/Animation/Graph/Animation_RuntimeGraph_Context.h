#pragma once

#include "Animation_RuntimeGraph_SampledEvents.h"
#include "Engine/Animation/TaskSystem/Animation_BoneMaskTask.h"
#include "Base/Math/Transform.h"
#include "Base/Time/Time.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::Physics { class PhysicsWorld; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Skeleton;
    class RootMotionDebugger;
    class TaskSystem;
    class Pose;
    class GraphNode;
    class GraphDataSet;
    class GraphTimeInfo;
    class GraphInstance;
    struct GraphLayerUpdateState;
    struct GraphLayerContext;
    struct BoneMaskTaskList;
    struct PoseBuffer;

    //-------------------------------------------------------------------------
    // Log entry for the graph
    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    struct GraphLogEntry
    {
        uint32_t        m_updateID;
        Severity        m_severity;
        SourcePath      m_sourcePath;
        String          m_message;
    };
    #endif

    //-------------------------------------------------------------------------
    // Graph Context
    //-------------------------------------------------------------------------

    // Used to signify if a node or node output is coming from an active state (i.e. a state we are not transitioning away from)
    enum class BranchState
    {
        Active,
        Inactive,
    };

    //-------------------------------------------------------------------------

    // Used to track layer data
    struct GraphLayerContext final
    {
        BoneMaskTaskList                            m_layerMaskTaskList;
        float                                       m_layerWeight = 1.0f;
        float                                       m_rootMotionLayerWeight = 1.0f;
        bool                                        m_isAdditive = false;
    };

    //-------------------------------------------------------------------------

    class GraphContext final
    {
        friend class GraphInstance;

    public:

        GraphContext( Skeleton const* pSkeleton );
        ~GraphContext();

        void Initialize( GraphContext *pParentContext = nullptr );
        void Shutdown();

        inline bool IsValid() const { return m_isInitialized && m_pSkeleton != nullptr && m_pTaskSystem != nullptr && m_pSampledEventsBuffer != nullptr; }
        inline bool IsInitialized() const { return m_isInitialized; }

        void Update( Seconds const deltaTime, Transform const& currentWorldTransform, Physics::PhysicsWorld* pPhysicsWorld );

        // Flag a node as active
        inline void TrackActiveNode( int16_t nodeIdx ) { EE_ASSERT( nodeIdx != InvalidIndex ); m_activeNodes.emplace_back( nodeIdx ); }

        // Get the previous frame's pose buffer
        PoseBuffer const* GetPreviousPoseBuffer() const;

        // Get the previous frame's primary skeleton pose
        Pose const* GetPreviousPrimaryPose() const;

        // Are we currently in a layer
        EE_FORCE_INLINE bool IsInLayer() const { return m_pLayerContext != nullptr; }

        // Get the task system
        EE_FORCE_INLINE TaskSystem* GetTaskSystem() { return m_pTaskSystem; }

        // Get the sampled events buffer
        EE_FORCE_INLINE SampledEventsBuffer* GetSampledEventsBuffer() { return m_pSampledEventsBuffer; }

        // Get an valid but empty range given the current state of the sampled event buffer
        EE_FORCE_INLINE SampledEventRange GetEmptySampledEventRange() const { return SampledEventRange( m_pSampledEventsBuffer->GetNumSampledEvents() ); }

        // Path Tracking
        //-------------------------------------------------------------------------
        // Needed for debugging and event disambiguation

        // Get the current base path
        SourcePath const& GetBasePath() const { return m_basePath; }

        // Push Path
        void PushBasePath( int16_t nodeIdx ) { m_basePath.Push( nodeIdx ); }

        // Push Path
        void PopBasePath() { m_basePath.Pop(); }

        // Debugging
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Root Motion
        inline RootMotionDebugger* GetRootMotionDebugger() { return m_pRootMotionDebugger; }

        // Log a graph warning
        void LogWarning( int16_t nodeIdx, char const* pFormat, ... );

        // Log a graph error
        void LogError( int16_t nodeIdx, char const* pFormat, ... );

        // Clear the graph log
        void ClearLog()
        {
            EE_ASSERT( m_isStandaloneGraphContext );
            m_pLog->clear();
            m_lastOutputtedLogItemIdx = 0;
        }
        #endif

    private:

        GraphContext( GraphContext const& ) = delete;
        GraphContext& operator=( GraphContext& ) = delete;

        void TransferContextDataFromParent( GraphContext const &parentContext );

    public:

        // Set at construction
        int32_t const                               m_instanceID = 0;
        Skeleton const*                             m_pSkeleton = nullptr;

        // Initialization data
        GraphTimeInfo const*                        m_pInitializationTimeInfo = nullptr;

        // Runtime Values
        uint32_t                                    m_updateID = 0;
        BranchState                                 m_branchState = BranchState::Active;
        Transform                                   m_worldTransform = Transform::Identity;
        Transform                                   m_worldTransformInverse = Transform::Identity;
        Physics::PhysicsWorld*                      m_pPhysicsWorld = nullptr;
        GraphLayerContext*                          m_pLayerContext = nullptr;
        Seconds                                     m_deltaTime = 0.0f;

        #if EE_DEVELOPMENT_TOOLS
        TVector<SourcePath> const*                  m_pNodesToDebug = nullptr;
        #endif

    private:

        TVector<int16_t>                            m_activeNodes;
        SourcePath                                  m_basePath; // The path to the current graph instance that we are in
        bool                                        m_isStandaloneGraphContext = true;
        bool                                        m_isInitialized = false;

        // This state can actually be that of a parent graph when the instance is used in a referenced fashion
        TaskSystem                                  *m_pTaskSystem = nullptr;
        SampledEventsBuffer                         *m_pSampledEventsBuffer = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        TVector<GraphLogEntry>*                     m_pLog = nullptr;
        int32_t                                     m_lastOutputtedLogItemIdx = 0;
        RootMotionDebugger*                         m_pRootMotionDebugger = nullptr; // Allows nodes to record root motion operations
        #endif
    };
}