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

    class StateToolsNode final : public ToolsState
    {
        friend class StateMachineToolsNode;
        EE_REGISTER_TYPE( StateToolsNode );

    public:

        enum class StateType
        {
            EE_REGISTER_ENUM

            BlendTreeState,
            StateMachineState
        };

    public:

        StateToolsNode() = default;
        StateToolsNode( StateType type ) : m_type( type ) {}

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        inline bool IsBlendTreeState() const { return m_type == StateType::BlendTreeState; }
        inline bool IsStateMachineState() const { return m_type == StateType::StateMachineState; }

    private:

        virtual char const* GetTypeName() const override { return "State"; }
        virtual ImColor GetTitleBarColor() const override;
        virtual void DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos ) override;
        virtual void OnDoubleClick( VisualGraph::UserContext* pUserContext ) override;
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;

    private:

        EE_REGISTER StateType m_type = StateType::BlendTreeState;
    };

    //-------------------------------------------------------------------------

    // An off-state
    class OffStateToolsNode final : public ToolsState
    {
        friend class StateMachineToolsNode;
        EE_REGISTER_TYPE( OffStateToolsNode );

    private:

        virtual char const* GetTypeName() const override { return "Off State"; }
        virtual ImColor GetTitleBarColor() const override { return ImGuiX::ConvertColor( Colors::DarkRed ); }
        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;
    };
}