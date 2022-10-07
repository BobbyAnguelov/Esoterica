#pragma once

#include "Animation_RuntimeGraphNode_Passthrough.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class Ragdoll;
    struct RagdollDefinition;
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
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

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_entryNodeIdx, m_dataSlotIdx, m_entryProfileID, m_simulatedProfileID, m_exitProfileID, m_exitOptionNodeIndices );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                                         m_entryNodeIdx = InvalidIndex;
            int16_t                                         m_dataSlotIdx = InvalidIndex;
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

        virtual GraphPoseNodeResult Update( GraphContext& context ) override;

        // We dont support synchronization for this node!
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override { return Update( context ); }

        //-------------------------------------------------------------------------

        GraphPoseNodeResult UpdateEntry( GraphContext& context );
        GraphPoseNodeResult UpdateSimulated( GraphContext& context );
        GraphPoseNodeResult UpdateExit( GraphContext& context );

    private:

        Physics::RagdollDefinition const*                   m_pRagdollDefinition = nullptr;
        Physics::Ragdoll*                                   m_pRagdoll = nullptr;
        AnimationClipReferenceNode*                         m_pEntryNode = nullptr;
        TInlineVector<AnimationClipReferenceNode*, 3>       m_exitNodeOptions;
        Stage                                               m_stage = Stage::Invalid;
        bool                                                m_isFirstRagdollUpdate = false;
    };
}