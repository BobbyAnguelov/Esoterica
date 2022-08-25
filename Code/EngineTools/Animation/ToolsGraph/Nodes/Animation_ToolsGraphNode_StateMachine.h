#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class StateMachineGraph;
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class TransitionToolsNode;
    class ToolsState;
    class EntryStateOverrideConduitToolsNode;
    class GlobalTransitionConduitToolsNode;

    //-------------------------------------------------------------------------

    // The state machine node shown in blend trees
    class StateMachineToolsNode final : public FlowToolsNode
    {
        EE_REGISTER_TYPE( StateMachineToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        virtual char const* GetName() const override { return m_name.c_str(); }
        virtual char const* GetTypeName() const override { return "State Machine"; }
        virtual char const* GetCategory() const override { return ""; }
        virtual ImColor GetTitleBarColor() const override { return ImGuiX::ConvertColor( Colors::CornflowerBlue ); }

        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        virtual void OnShowNode( VisualGraph::UserContext* pUserContext ) override;
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue ) override;

        int16_t CompileState( GraphCompilationContext& context, ToolsState const* pBaseStateNode ) const;
        int16_t CompileTransition( GraphCompilationContext& context, TransitionToolsNode const* pTransitionNode, int16_t targetStateNodeIdx ) const;

        EntryStateOverrideConduitToolsNode const* GetEntryStateOverrideConduit() const;
        inline GraphNodes::EntryStateOverrideConduitToolsNode* GetEntryStateOverrideConduit() { return const_cast<GraphNodes::EntryStateOverrideConduitToolsNode*>( const_cast<StateMachineToolsNode const*>( this )->GetEntryStateOverrideConduit() ); }
        GlobalTransitionConduitToolsNode const* GetGlobalTransitionConduit() const;
        inline GraphNodes::GlobalTransitionConduitToolsNode* GetGlobalTransitionConduit() { return const_cast<GraphNodes::GlobalTransitionConduitToolsNode*>( const_cast<StateMachineToolsNode const*>( this )->GetGlobalTransitionConduit() ); }

    private:

        EE_EXPOSE String m_name = "SM";
    };
}