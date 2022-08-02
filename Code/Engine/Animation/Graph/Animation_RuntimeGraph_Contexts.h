#pragma once

#include "Animation_RuntimeGraph_Events.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "System/Math/Transform.h"
#include "System/Time/Time.h"
#include "System/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::Physics { class Scene; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class RootMotionDebugger;
    class TaskSystem;
    class Pose;

    //-------------------------------------------------------------------------

    // Used to signify if a node or node output is coming from an active state (i.e. a state we are not transitioning away from)
    enum class BranchState
    {
        Active,
        Inactive,
    };

    //-------------------------------------------------------------------------
    // Layer Context
    //-------------------------------------------------------------------------

    struct GraphLayerContext
    {
        EE_FORCE_INLINE bool IsSet() const { return m_isCurrentlyInLayer; }

        EE_FORCE_INLINE void BeginLayer()
        {
            m_isCurrentlyInLayer = true;
            m_layerWeight = 1.0f;
            m_pLayerMask = nullptr;
        }

        EE_FORCE_INLINE void EndLayer()
        {
            m_isCurrentlyInLayer = false;
            m_layerWeight = 0.0f;
            m_pLayerMask = nullptr;
        }

    public:

        BoneMask*                               m_pLayerMask = nullptr;
        float                                   m_layerWeight = 0.0f;
        bool                                    m_isCurrentlyInLayer = false;
    };

    //-------------------------------------------------------------------------
    // Graph Context
    //-------------------------------------------------------------------------

    class GraphContext final
    {
        friend class GraphInstance;

    public:

        GraphContext( uint64_t userID, Skeleton const* pSkeleton );
        ~GraphContext();

        void Initialize( TaskSystem* pTaskSystem );
        void Shutdown();

        inline bool IsValid() const { return m_pSkeleton != nullptr && m_pTaskSystem != nullptr; }
        void Update( Seconds const deltaTime, Transform const& currentWorldTransform, Physics::Scene* pPhysicsScene );

        // Debugging
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void SetDebugSystems( RootMotionDebugger* pRootMotionRecorder, TVector<int16_t>* pActiveNodesList );

        // Flag a node as active
        inline void TrackActiveNode( int16_t nodeIdx ) { EE_ASSERT( nodeIdx != InvalidIndex ); m_pActiveNodes->emplace_back( nodeIdx ); }

        // Root Motion
        inline RootMotionDebugger* GetRootMotionActionRecorder() { return m_pRootMotionActionRecorder; }
        #endif

    private:

        GraphContext( GraphContext const& ) = delete;
        GraphContext& operator=( GraphContext& ) = delete;

    public:

        // Set at construction
        uint64_t                                m_graphUserID = 0; // The entity ID that owns this graph.
        Skeleton const*                         m_pSkeleton = nullptr;
        SampledEventsBuffer                     m_sampledEventsBuffer;
        BoneMaskPool                            m_boneMaskPool;

        // Set at initialization time
        TaskSystem*                             m_pTaskSystem = nullptr;
        Pose const*                             m_pPreviousPose = nullptr;

        // Runtime Values
        Transform                               m_worldTransform = Transform::Identity;
        Transform                               m_worldTransformInverse = Transform::Identity;
        uint32_t                                m_updateID = 0;
        BranchState                             m_branchState = BranchState::Active;
        Physics::Scene*                         m_pPhysicsScene = nullptr;
        GraphLayerContext                       m_layerContext;
        Seconds                                 m_deltaTime = 0.0f;

    private:

        #if EE_DEVELOPMENT_TOOLS
        RootMotionDebugger*                     m_pRootMotionActionRecorder = nullptr; // Allows nodes to record root motion operations
        TVector<int16_t>*                       m_pActiveNodes = nullptr;
        #endif
    };
}