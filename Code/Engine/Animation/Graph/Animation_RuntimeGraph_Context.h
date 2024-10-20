#pragma once

#include "Animation_RuntimeGraph_SampledEvents.h"
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
        Severity   m_severity;
        uint16_t        m_nodeIdx;
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

    class GraphContext final
    {
        friend class GraphInstance;

    public:

        GraphContext( uint64_t ownerID, Skeleton const* pSkeleton );
        ~GraphContext();

        void Initialize( TaskSystem* pTaskSystem, SampledEventsBuffer* pSampledEventsBuffer );
        void Shutdown();

        inline bool IsValid() const { return m_pSkeleton != nullptr && m_pTaskSystem != nullptr; }
        void Update( Seconds const deltaTime, Transform const& currentWorldTransform, Physics::PhysicsWorld* pPhysicsWorld );

        // Get the previous frame's pose buffer
        PoseBuffer const* GetPreviousPoseBuffer() const;

        // Get the previous frame's primary skeleton pose
        Pose const* GetPreviousPrimaryPose() const;

        // Are we currently in a layer
        EE_FORCE_INLINE bool IsInLayer() const { return m_pLayerContext != nullptr; }

        // Get an valid but empty range given the current state of the sampled event buffer
        EE_FORCE_INLINE SampledEventRange GetEmptySampledEventRange() const { return SampledEventRange( m_pSampledEventsBuffer->GetNumSampledEvents() ); }

        // Debugging
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Flag a node as active
        inline void TrackActiveNode( int16_t nodeIdx ) { EE_ASSERT( nodeIdx != InvalidIndex ); m_pActiveNodes->emplace_back( nodeIdx ); }

        // Root Motion
        inline RootMotionDebugger* GetRootMotionDebugger() { return m_pRootMotionDebugger; }

        // Push Debug Path
        void PushDebugPath( int16_t nodeIdx );

        // Push Debug Path
        void PopDebugPath();

        // Log a graph warning
        void LogWarning( int16_t nodeIdx, char const* pFormat, ... );

        // Log a graph error
        void LogError( int16_t nodeIdx, char const* pFormat, ... );
        #endif

    private:

        GraphContext( GraphContext const& ) = delete;
        GraphContext& operator=( GraphContext& ) = delete;

        #if EE_DEVELOPMENT_TOOLS
        void SetDebugSystems( RootMotionDebugger* pRootMotionRecorder, TVector<int16_t>* pActiveNodesList, TVector<GraphLogEntry>* pLog );
        #endif

    public:

        // Set at construction
        uint64_t                                    m_graphUserID = 0; // The entity ID that owns this graph.
        Skeleton const*                             m_pSkeleton = nullptr;

        // Set at initialization time
        SampledEventsBuffer*                        m_pSampledEventsBuffer = nullptr;
        TaskSystem*                                 m_pTaskSystem = nullptr;
        Pose const*                                 m_pPreviousPose = nullptr;

        // Initialization data
        TVector<GraphLayerUpdateState> const*       m_pLayerInitializationInfo = nullptr;

        // Runtime Values
        uint32_t                                    m_updateID = 0;
        BranchState                                 m_branchState = BranchState::Active;
        Transform                                   m_worldTransform = Transform::Identity;
        Transform                                   m_worldTransformInverse = Transform::Identity;
        Physics::PhysicsWorld*                      m_pPhysicsWorld = nullptr;
        GraphLayerContext*                          m_pLayerContext = nullptr;
        Seconds                                     m_deltaTime = 0.0f;

    private:

        #if EE_DEVELOPMENT_TOOLS
        RootMotionDebugger*                         m_pRootMotionDebugger = nullptr; // Allows nodes to record root motion operations
        TVector<int16_t>*                           m_pActiveNodes = nullptr;
        TVector<GraphLogEntry>*                     m_pLog = nullptr;
        #endif
    };
}