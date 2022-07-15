#pragma once

#include "Animation_RuntimeGraph_Events.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "System/Math/Transform.h"
#include "System/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE::Physics { class Scene; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class RootMotionRecorder;
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

    public:

        GraphContext();

        void Initialize( uint64_t userID, TaskSystem* pTaskSystem, Pose const* pPreviousPose, RootMotionRecorder* pRootMotionRecorder );
        void Shutdown();

        inline bool IsValid() const { return m_pSkeleton != nullptr && m_pTaskSystem != nullptr && m_pPreviousPose != nullptr; }
        void Update( Seconds const deltaTime, Transform const& currentWorldTransform, Physics::Scene* pPhysicsScene );

        // Debugging
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS

        // Active nodes
        inline void TrackActiveNode( int16_t nodeIdx ) { EE_ASSERT( nodeIdx != InvalidIndex ); m_activeNodes.emplace_back( nodeIdx ); }
        inline TVector<int16_t> const& GetActiveNodes() const { return m_activeNodes; }

        // Root Motion
        inline RootMotionRecorder* GetRootMotionActionRecorder() { return m_pRootMotionActionRecorder; }
        inline RootMotionRecorder const* GetRootMotionActionRecorder() const { return m_pRootMotionActionRecorder; }

        #endif

    private:

        GraphContext( GraphContext const& ) = delete;
        GraphContext& operator=( GraphContext& ) = delete;

    public:

        uint64_t                                m_graphUserID = 0;
        TaskSystem* const                       m_pTaskSystem = nullptr;
        Skeleton const* const                   m_pSkeleton = nullptr;
        Pose const* const                       m_pPreviousPose = nullptr;
        Transform                               m_worldTransform = Transform::Identity;
        Transform                               m_worldTransformInverse = Transform::Identity;
        SampledEventsBuffer                     m_sampledEvents;
        uint32_t                                m_updateID = 0;
        BranchState                             m_branchState = BranchState::Active;
        Physics::Scene*                         m_pPhysicsScene = nullptr;

        BoneMaskPool                            m_boneMaskPool;
        GraphLayerContext                       m_layerContext;
        Seconds                                 m_deltaTime = 0.0f;

    private:

        #if EE_DEVELOPMENT_TOOLS
        RootMotionRecorder* const               m_pRootMotionActionRecorder = nullptr; // Allows nodes to record root motion operations
        TVector<int16_t>                        m_activeNodes;
        #endif
    };

}