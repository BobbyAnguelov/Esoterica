#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphInstance;
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API ChildGraphNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( PoseNode::Settings, m_childGraphIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                             m_childGraphIdx = InvalidIndex;
        };

    private:

        virtual SyncTrack const& GetSyncTrack() const override;
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;

        void TransferGraphInstanceData( GraphContext& context, GraphPoseNodeResult& result );

        void ReflectControlParameters( GraphContext& context );

    private:

        GraphInstance*                          m_pGraphInstance = nullptr;
        TInlineVector<ValueNode*, 10>           m_parameterMapping;
    };
}