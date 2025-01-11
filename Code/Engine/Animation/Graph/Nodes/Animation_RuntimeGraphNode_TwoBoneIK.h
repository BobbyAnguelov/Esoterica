#pragma once

#include "Game/_Module/API.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Passthrough.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API TwoBoneIKNode final : public PassthroughNode
    {
    public:

        struct EE_ENGINE_API Definition final : public PassthroughNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PassthroughNode::Definition, m_effectorBoneID, m_effectorTargetNodeIdx, m_isTargetInWorldSpace );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            StringID        m_effectorBoneID;
            int16_t         m_effectorTargetNodeIdx = InvalidIndex;
            bool            m_isTargetInWorldSpace = false;
        };

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

    private:

        TargetValueNode*    m_pEffectorTarget = nullptr;
        int32_t             m_effectorBoneIdx = InvalidIndex;
    };
}