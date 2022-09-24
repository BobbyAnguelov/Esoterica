#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{;
    class GraphInstance;
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API ExternalGraphNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Settings final : public PoseNode::Settings
        {
            EE_REGISTER_TYPE( Settings );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    public:

        virtual ~ExternalGraphNode();

        void AttachGraphInstance( GraphContext& context, GraphInstance* pExternalGraphInstance );
        void DetachExternalGraphInstance( GraphContext& context );

    private:

        virtual SyncTrack const& GetSyncTrack() const override;
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const& updateRange ) override;

        void TransferGraphInstanceData( GraphContext& context, GraphPoseNodeResult& result );

    private:

        GraphInstance*          m_pGraphInstance = nullptr;
        bool                    m_isFirstUpdate = false;
    };
}