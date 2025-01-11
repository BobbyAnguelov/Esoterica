#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphInstance;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API ReferencedGraphNode final : public PoseNode
    {
    public:

        struct EE_ENGINE_API Definition final : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_referencedGraphIdx, m_fallbackNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                             m_referencedGraphIdx = InvalidIndex;
            int16_t                             m_fallbackNodeIdx = InvalidIndex;
        };

    private:

        virtual SyncTrack const& GetSyncTrack() const override;

        virtual bool IsValid() const override;
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        void ReflectControlParameters( GraphContext& context );

        #if EE_DEVELOPMENT_TOOLS
        virtual void DrawDebug( GraphContext& graphContext, Drawing::DrawContext& drawCtx ) override;
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        GraphInstance*                          m_pGraphInstance = nullptr;
        PoseNode*                               m_pFallbackNode = nullptr;
        TInlineVector<ValueNode*, 10>           m_parameterMapping;
    };
}