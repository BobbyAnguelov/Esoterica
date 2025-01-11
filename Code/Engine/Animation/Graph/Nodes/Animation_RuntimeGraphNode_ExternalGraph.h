#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{;
    class GraphInstance;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API ExternalGraphNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Definition final : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;
        };

    public:

        virtual ~ExternalGraphNode();

        void AttachExternalGraphInstance( GraphContext& context, GraphInstance* pExternalGraphInstance );
        void DetachExternalGraphInstance( GraphContext& context );

    private:

        virtual SyncTrack const& GetSyncTrack() const override;
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void DrawDebug( GraphContext& graphContext, Drawing::DrawContext& drawCtx ) override;
        #endif

    private:

        GraphInstance*          m_pGraphInstance = nullptr;
        bool                    m_shouldResetGraphInstance = false;
    };
}