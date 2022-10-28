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
    class EE_ENGINE_API PoweredRagdollNode final : public PassthroughNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PassthroughNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PassthroughNode::Settings, m_physicsBlendWeightNodeIdx, m_inpulseOriginVectorNodeIdx, m_inpulseForceVectorNodeIdx, m_physicsBlendWeight, m_profileID, m_dataSlotIdx, m_isGravityEnabled );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                                         m_physicsBlendWeightNodeIdx = InvalidIndex;
            int16_t                                         m_inpulseOriginVectorNodeIdx = InvalidIndex;
            int16_t                                         m_inpulseForceVectorNodeIdx = InvalidIndex;
            int16_t                                         m_dataSlotIdx = InvalidIndex;
            StringID                                        m_profileID;
            float                                           m_physicsBlendWeight = 1.0f;
            bool                                            m_isGravityEnabled;
        };

    private:

        virtual bool IsValid() const override;
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;

        GraphPoseNodeResult UpdateRagdoll( GraphContext& context, GraphPoseNodeResult const& childResult );

    private:

        Physics::RagdollDefinition const*                   m_pRagdollDefinition = nullptr;
        FloatValueNode*                                     m_pBlendWeightValueNode = nullptr;
        VectorValueNode*                                    m_pImpulseOriginValueNode = nullptr;
        VectorValueNode*                                    m_pImpulseForceValueNode = nullptr;
        Physics::Ragdoll*                                   m_pRagdoll = nullptr;
        bool                                                m_isFirstUpdate = false;
    };
}