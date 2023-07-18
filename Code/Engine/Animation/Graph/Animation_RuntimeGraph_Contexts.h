#pragma once

#include "Animation_RuntimeGraph_Events.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "Base/Math/Transform.h"
#include "Base/Time/Time.h"
#include "Base/Types/Arrays.h"

// HACK
#include "Engine/Animation/AnimationSyncTrack.h"

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
    struct BoneMaskTaskList;

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    struct GraphLogEntry
    {
        uint32_t        m_updateID;
        Log::Severity   m_severity;
        uint16_t        m_nodeIdx;
        String          m_message;
    };
    #endif

    //-------------------------------------------------------------------------
    // Instantiation Context
    //-------------------------------------------------------------------------

    enum class InstantiationOptions
    {
        CreateNode,                 // Instruct the instantiate function to actually create the node
        NodeAlreadyCreated          // Informs the instantiate function that the node has been created (via derived class) and so it should only get it's ptrs set
    };

    struct InstantiationContext final
    {
        template<typename T>
        EE_FORCE_INLINE void SetNodePtrFromIndex( int16_t nodeIdx, T*& pTargetPtr ) const
        {
            EE_ASSERT( nodeIdx >= 0 && nodeIdx < m_nodePtrs.size() );
            pTargetPtr = reinterpret_cast<T*>( m_nodePtrs[nodeIdx] );
        }

        template<typename T>
        EE_FORCE_INLINE void SetNodePtrFromIndex( int16_t nodeIdx, T const*& pTargetPtr ) const
        {
            EE_ASSERT( nodeIdx >= 0 && nodeIdx < m_nodePtrs.size() );
            pTargetPtr = reinterpret_cast<T const*>( m_nodePtrs[nodeIdx] );
        }

        template<typename T>
        EE_FORCE_INLINE void SetOptionalNodePtrFromIndex( int16_t nodeIdx, T*& pTargetPtr ) const
        {
            if ( nodeIdx == InvalidIndex )
            {
                pTargetPtr = nullptr;
            }
            else
            {
                EE_ASSERT( nodeIdx >= 0 && nodeIdx < m_nodePtrs.size() );
                pTargetPtr = reinterpret_cast<T*>( m_nodePtrs[nodeIdx] );
            }
        }

        template<typename T>
        EE_FORCE_INLINE void SetOptionalNodePtrFromIndex( int16_t nodeIdx, T const*& pTargetPtr ) const
        {
            if ( nodeIdx == InvalidIndex )
            {
                pTargetPtr = nullptr;
            }
            else
            {
                EE_ASSERT( nodeIdx >= 0 && nodeIdx < m_nodePtrs.size() );
                pTargetPtr = reinterpret_cast<T const*>( m_nodePtrs[nodeIdx] );
            }
        }

        //-------------------------------------------------------------------------

        template<typename T>
        EE_FORCE_INLINE T const* GetResource( int16_t const& ID ) const
        {
            return m_pDataSet->GetResource<T>( ID );
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void LogWarning( char const* pFormat, ... ) const;
        #endif

    public:

        int16_t                                     m_currentNodeIdx;
        TVector<GraphNode*> const&                  m_nodePtrs;
        TInlineVector<GraphInstance*, 20> const&    m_childGraphInstances;
        THashMap<StringID, int16_t> const&          m_parameterLookupMap;
        GraphDataSet const*                         m_pDataSet;
        uint64_t                                    m_userID;

        #if EE_DEVELOPMENT_TOOLS
        TVector<GraphLogEntry>*                     m_pLog;
        #endif
    };

    //-------------------------------------------------------------------------
    // Layer Context
    //-------------------------------------------------------------------------

    struct GraphLayerInitInfo
    {
        int16_t                                                     m_layerNodeIdx;
        TInlineVector<TPair<int8_t, SyncTrackTimeRange>, 5>         m_layerInitTimes;
    };

    struct GraphLayerContext final
    {
        EE_FORCE_INLINE bool IsSet() const { return m_isCurrentlyInLayer; }

        EE_FORCE_INLINE void BeginLayer()
        {
            m_isCurrentlyInLayer = true;
            m_layerWeight = 1.0f;
            m_rootMotionLayerWeight = 1.0f;
            m_layerMaskTaskList.Reset();
        }

        EE_FORCE_INLINE void ResetLayer()
        {
            EE_ASSERT( m_isCurrentlyInLayer );
            m_layerWeight = 1.0f;
            m_rootMotionLayerWeight = 1.0f;
            m_layerMaskTaskList.Reset();
        }

        EE_FORCE_INLINE void EndLayer()
        {
            m_isCurrentlyInLayer = false;
            m_layerWeight = 0.0f;
            m_rootMotionLayerWeight = 0.0f;
            m_layerMaskTaskList.Reset();
        }

    public:

        BoneMaskTaskList                        m_layerMaskTaskList;
        float                                   m_layerWeight = 0.0f;
        float                                   m_rootMotionLayerWeight = 0.0f;
        bool                                    m_isCurrentlyInLayer = false;
    };

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

        GraphContext( uint64_t userID, Skeleton const* pSkeleton );
        ~GraphContext();

        void Initialize( TaskSystem* pTaskSystem );
        void Shutdown();

        inline bool IsValid() const { return m_pSkeleton != nullptr && m_pTaskSystem != nullptr; }
        void Update( Seconds const deltaTime, Transform const& currentWorldTransform, Physics::PhysicsWorld* pPhysicsWorld );

        inline bool IsInLayer() const { return m_layerContext.m_isCurrentlyInLayer; }

        // Get an valid but empty range given the current state of the sampled event buffer
        EE_FORCE_INLINE SampledEventRange GetEmptySampledEventRange() const { return SampledEventRange( m_sampledEventsBuffer.GetNumSampledEvents() ); }

        // Debugging
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Flag a node as active
        inline void TrackActiveNode( int16_t nodeIdx ) { EE_ASSERT( nodeIdx != InvalidIndex ); m_pActiveNodes->emplace_back( nodeIdx ); }

        // Root Motion
        inline RootMotionDebugger* GetRootMotionDebugger() { return m_pRootMotionDebugger; }

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
        uint64_t                                m_graphUserID = 0; // The entity ID that owns this graph.
        Skeleton const*                         m_pSkeleton = nullptr;
        SampledEventsBuffer                     m_sampledEventsBuffer;

        // Set at initialization time
        TaskSystem*                             m_pTaskSystem = nullptr;
        Pose const*                             m_pPreviousPose = nullptr;

        // Initialization data
        TInlineVector<GraphLayerInitInfo, 10>   m_layerInitInfo;

        // Runtime Values
        Transform                               m_worldTransform = Transform::Identity;
        Transform                               m_worldTransformInverse = Transform::Identity;
        uint32_t                                m_updateID = 0;
        BranchState                             m_branchState = BranchState::Active;
        Physics::PhysicsWorld*                  m_pPhysicsWorld = nullptr;
        GraphLayerContext                       m_layerContext;
        Seconds                                 m_deltaTime = 0.0f;

    private:

        #if EE_DEVELOPMENT_TOOLS
        RootMotionDebugger*                     m_pRootMotionDebugger = nullptr; // Allows nodes to record root motion operations
        TVector<int16_t>*                       m_pActiveNodes = nullptr;
        TVector<GraphLogEntry>*                 m_pLog = nullptr;
        #endif
    };
}