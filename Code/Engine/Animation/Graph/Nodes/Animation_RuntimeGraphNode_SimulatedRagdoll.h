#pragma once

#include "Animation_RuntimeGraphNode_Passthrough.h"
#include "Engine/Physics/Ragdoll/PhysicsRagdoll_Instance.h"

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
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_dataSlotIdx, m_entryNodeIdx, m_exitConditionNodeIdx, m_exitOptionNodeIndices, m_blendTime );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                                         m_dataSlotIdx = InvalidIndex;
            int16_t                                         m_entryNodeIdx = InvalidIndex;
            int16_t                                         m_exitConditionNodeIdx = InvalidIndex;
            TInlineVector<int16_t, 3>                       m_exitOptionNodeIndices;
            Seconds                                         m_blendTime = 0.3f;
        };

    private:

        virtual bool IsValid() const override;
        virtual SyncTrack const& GetSyncTrack() const override;
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        //-------------------------------------------------------------------------

        void CreateRagdoll( GraphContext& context );
        void DestroyRagdoll( GraphContext& context );

        GraphPoseNodeResult UpdateEntry( GraphContext& context, SyncTrackTimeRange const* pUpdateRange );
        GraphPoseNodeResult UpdateSimulated( GraphContext& context );
        GraphPoseNodeResult UpdateExit( GraphContext& context, SyncTrackTimeRange const* pUpdateRange );

        inline void SetStage( Stage stage )
        {
            m_stage = stage;
            m_isFirstUpdate = true;
            m_currentBlendTime = 0.0f;
        }

        void SelectExitNode( GraphContext& context );

    private:

        Physics::RagdollDefinition const*                   m_pRagdollDefinition = nullptr;
        Physics::Ragdoll*                                   m_pRagdoll = nullptr;
        AnimationClipReferenceNode*                         m_pEntryNode = nullptr;
        BoolValueNode*                                      m_pExitConditionNode = nullptr;
        AnimationClipReferenceNode*                         m_pExitNode = nullptr;
        TInlineVector<AnimationClipReferenceNode*, 3>       m_exitNodeOptions;
        Seconds                                             m_currentBlendTime = 0.0f;
        Stage                                               m_stage = Stage::Invalid;
        bool                                                m_entryHasRagdollEvent = false;
        bool                                                m_isFirstUpdate = false; // Is this the first update per stage
    };
}