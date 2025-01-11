#pragma once

#include "Animation_RuntimeGraphNode_Passthrough.h"
#include "Engine/Physics/PhysicsRagdoll.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class Ragdoll;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class AnimationClipReferenceNode;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SimulatedRagdollNode final : public PoseNode
    {
        enum class Stage
        {
            Invalid,
            FullyInEntryAnim,
            BlendToRagdoll,
            FullyInRagdoll,
            BlendOutOfRagdoll,
            FullyInExitAnim,
        };

    public:

        struct EE_ENGINE_API Definition final : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_entryNodeIdx, m_dataSlotIdx, m_entryProfileID, m_simulatedProfileID, m_exitProfileID, m_exitOptionNodeIndices );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                                         m_dataSlotIdx = InvalidIndex;
            int16_t                                         m_entryNodeIdx = InvalidIndex;
            StringID                                        m_entryProfileID;
            StringID                                        m_simulatedProfileID;
            StringID                                        m_exitProfileID;
            TInlineVector<int16_t, 3>                       m_exitOptionNodeIndices;
        };

    private:

        virtual bool IsValid() const override;
        virtual SyncTrack const& GetSyncTrack() const override;
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        //-------------------------------------------------------------------------

        void CreateRagdoll( GraphContext& context );

        GraphPoseNodeResult UpdateEntry( GraphContext& context, SyncTrackTimeRange const* pUpdateRange );
        GraphPoseNodeResult UpdateSimulated( GraphContext& context );
        GraphPoseNodeResult UpdateExit( GraphContext& context );

    private:

        Physics::RagdollDefinition const*                   m_pRagdollDefinition = nullptr;
        Physics::Ragdoll*                                   m_pRagdoll = nullptr;
        AnimationClipReferenceNode*                         m_pEntryNode = nullptr;
        TInlineVector<AnimationClipReferenceNode*, 3>       m_exitNodeOptions;
        Stage                                               m_stage = Stage::Invalid;
        bool                                                m_isFirstUpdate = false;
    };
}