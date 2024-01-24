#pragma once

#include "Game/_Module/API.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Passthrough.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_GAME_API LookAtIKNode final : public PassthroughNode
    {
    public:

        struct EE_GAME_API Definition final : public PassthroughNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PassthroughNode::Definition, m_targetNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

        public:

            int16_t                 m_targetNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        GraphPoseNodeResult RegisterIKTask( GraphContext& context, GraphPoseNodeResult const& childNodeResult );

    private:

        VectorValueNode*            m_pTargetValueNode; // The world space target point for the look-at
    };
}