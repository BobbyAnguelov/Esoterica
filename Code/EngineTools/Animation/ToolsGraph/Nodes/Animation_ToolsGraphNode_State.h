#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ResultToolsNode;

    //-------------------------------------------------------------------------

    // The result node for a state's layer settings
    class StateLayerDataToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( StateLayerDataToolsNode );

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetTypeName() const override { return "State Layer Data"; }
        virtual char const* GetCategory() const override { return "State Machine"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree ); }
    };

    //-------------------------------------------------------------------------

    // A basic state
    class BlendTreeStateToolsNode final : public ToolsState
    {
        friend class StateMachineToolsNode;
        EE_REGISTER_TYPE( BlendTreeStateToolsNode );

    public:

        ResultToolsNode const* GetBlendTreeRootNode() const;
        StateLayerDataToolsNode const* GetLayerDataNode() const;
        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

    private:

        virtual char const* GetTypeName() const override { return "State"; }
        virtual void DrawExtraContextMenuOptions( VisualGraph::DrawContext const& ctx, Float2 const& mouseCanvasPos ) override;
    };

    //-------------------------------------------------------------------------

    // An off-state
    class OffStateToolsNode final : public ToolsState
    {
        friend class StateMachineToolsNode;
        EE_REGISTER_TYPE( OffStateToolsNode );

    private:

        virtual char const* GetTypeName() const override { return "Off State"; }
        virtual ImColor GetTitleBarColor() const override { return ImGuiX::ConvertColor( Colors::PaleVioletRed ); }
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx ) override;
    };
}