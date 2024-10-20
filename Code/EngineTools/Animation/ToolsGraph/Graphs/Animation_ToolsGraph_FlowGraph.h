#pragma once
#include "EngineTools/NodeGraph/NodeGraph_FlowGraph.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphCompilationContext;
    class ToolsGraphDefinition;

    //-------------------------------------------------------------------------
    // Base Animation Flow Graph
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API FlowGraph : public NodeGraph::FlowGraph
    {
        friend class ToolsGraphDefinition;
        EE_REFLECT_TYPE( FlowGraph );

    public:

        constexpr static char const* const s_graphParameterPayloadID = "AnimGraphParameterPayload";

    public:

        FlowGraph( GraphType type = GraphType::BlendTree );

        inline GraphType GetType() const { return m_type; }

        // Node factory methods
        //-------------------------------------------------------------------------

        virtual bool CanCreateNode( TypeSystem::TypeInfo const* pNodeTypeInfo ) const override;

    private:

        virtual bool SupportsNodeCreationFromConnection() const override { return true; }
        virtual bool DrawContextMenuOptions( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, TVector<String> const& filterTokens, NodeGraph::FlowNode* pSourceNode, NodeGraph::Pin* pOriginPin ) override;
        virtual void GetSupportedDragAndDropPayloadIDs( TInlineVector<char const*, 10>& outIDs ) const override;
        virtual bool HandleDragAndDrop( NodeGraph::UserContext* pUserContext, NodeGraph::DragAndDropState const& dragAndDropState ) override;

    private:

        EE_REFLECT( ReadOnly )
        GraphType       m_type;
    };
}