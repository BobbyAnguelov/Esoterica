#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    // The result node for the entry state overrides
    //-------------------------------------------------------------------------
    // It has one input pin per state in the state machine

    class EntryStateOverrideConditionsToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( EntryStateOverrideConditionsToolsNode );

        friend class EntryStateOverrideConduitToolsNode;

    public:

        void UpdateInputPins();

        void UpdatePinToStateMapping( THashMap<UUID, UUID> const& IDMapping );

        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree ); }

    private:

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetTypeName() const override { return "Entry State Conditions"; }
        virtual char const* GetCategory() const override { return "State Machine"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual void OnShowNode() override;

    private:

        // For each pin, what state does it represent
        EE_REFLECT( "IsToolsReadOnly" : true );
        TVector<UUID>  m_pinToStateMapping;
    };

    // State Machine Node
    //-------------------------------------------------------------------------

    class EntryStateOverrideConduitToolsNode final : public VisualGraph::SM::Node
    {
        EE_REFLECT_TYPE( EntryStateOverrideConduitToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        FlowToolsNode const* GetEntryConditionNodeForState( UUID const& stateID ) const;

        inline bool HasEntryOverrideForState( UUID const& stateID ) const { return GetEntryConditionNodeForState( stateID ) != nullptr; }

        void UpdateConditionsNode();

        void UpdateStateMapping( THashMap<UUID, UUID> const& IDMapping );

    private:

        virtual bool IsVisible() const override { return false; }
        virtual ImColor GetTitleBarColor() const override { return ImGuiX::ImColors::SlateBlue; }
        virtual char const* GetTypeName() const override { return "Entry Overrides"; }
        virtual bool IsUserCreatable() const override { return false; }
    };
}