#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphInstance;
    class GraphDefinition;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API ReferencedGraphNode final : public PoseNode
    {
        friend GraphInstance;

    public:

        struct EE_ENGINE_API Definition final : public PoseNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( PoseNode::Definition, m_referencedGraphIdx, m_fallbackNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                             m_referencedGraphIdx = InvalidIndex;
            int16_t                             m_fallbackNodeIdx = InvalidIndex;
        };

    public:

        virtual ~ReferencedGraphNode();
        virtual bool IsValid() const override;

        bool IsInternalGraphSlot() const { return GetDefinition<ReferencedGraphNode>()->m_referencedGraphIdx != InvalidIndex; }
        bool IsExternalGraphSlot() const { return GetDefinition<ReferencedGraphNode>()->m_referencedGraphIdx == InvalidIndex; }
        bool HasInstance() const { return m_pGraphInstance != nullptr; }
        GraphDefinition const* GetGraphDefinition() const;

    private:

        virtual SyncTrack const& GetSyncTrack() const override;

        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;

        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange ) override;

        void ReflectControlParametersFromParent( GraphContext& context );
        void ConnectExternalGraphInstance( GraphContext &context, GraphInstance *pInstance );
        void DisconnectExternalGraphInstance( GraphContext &context );

        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual bool RestoreGraphState( RecordedGraphState const& inState ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void DrawDebug( GraphContext& graphContext, DebugDrawContext& drawCtx ) override;
        #endif

    private:

        GraphInstance*                          m_pGraphInstance = nullptr;
        PoseNode*                               m_pFallbackNode = nullptr;
        TInlineVector<ValueNode*, 10>           m_parameterMapping;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API IsExternalGraphSlotFilledNode final : public BoolValueNode
    {

    public:

        struct EE_ENGINE_API Definition final : public BoolValueNode::Definition
        {
            EE_REFLECT_TYPE( Definition );
            EE_SERIALIZE_GRAPHNODEDEFINITION( BoolValueNode::Definition, m_externalGraphNodeIdx );

            virtual void InstantiateNode( InstantiationContext const &context, InstantiationOptions options ) const override;

            int16_t                             m_externalGraphNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext &context ) override;
        virtual void ShutdownInternal( GraphContext &context ) override;
        virtual void GetValueInternal( GraphContext &context, void *pOutValue ) override;

    private:

        ReferencedGraphNode                     *m_pExternalGraphNode = nullptr;
        bool                                    m_result = false;
    };
}