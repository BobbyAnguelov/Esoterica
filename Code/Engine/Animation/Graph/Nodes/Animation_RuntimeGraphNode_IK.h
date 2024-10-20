#pragma once

#include "Game/_Module/API.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Passthrough.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class IKRigDefinition;
    class IKRig;
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API IKRigNode final : public PassthroughNode
    {
    public:

        struct EE_ENGINE_API Definition final : public PassthroughNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PassthroughNode::Definition, m_dataSlotIdx, m_effectorTargetIndices );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                         m_dataSlotIdx = InvalidIndex;
            TInlineVector<int16_t, 6>       m_effectorTargetIndices;
        };

    public:

        virtual ~IKRigNode();

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

    private:

        IKRig*                              m_pRig = nullptr;
        TInlineVector<TargetValueNode*, 6>  m_effectorTargetNodes;
    };
}