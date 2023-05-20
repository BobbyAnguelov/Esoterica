#pragma once
#include "Animation_ToolsGraphNode_Transition.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class GlobalTransitionConduitToolsNode;

    // The result node for a global transition
    //-------------------------------------------------------------------------
    // There is always one of these nodes created for every state in the parent state machine

    class GlobalTransitionToolsNode final : public TransitionToolsNode
    {
        friend GlobalTransitionConduitToolsNode;
        friend StateMachineGraph;
        EE_REFLECT_TYPE( GlobalTransitionToolsNode );

    public:

        inline UUID const& GetEndStateID() const { return m_stateID; }

        virtual GraphValueType GetValueType() const override { return GraphValueType::Bool; }
        virtual char const* GetName() const override { return m_name.c_str(); }
        virtual char const* GetTypeName() const override { return "Global Transition"; }
        virtual char const* GetCategory() const override { return "State Machine"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::ValueTree ); }

    private:

        EE_REFLECT( "IsToolsReadOnly" : true );
        UUID m_stateID;
    };

    // The global transition node present in state machine graphs
    //-------------------------------------------------------------------------

    class GlobalTransitionConduitToolsNode final : public VisualGraph::SM::Node
    {
        EE_REFLECT_TYPE( GlobalTransitionConduitToolsNode );

    public:

        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;

        bool HasGlobalTransitionForState( UUID const& stateID ) const;

        void UpdateTransitionNodes();

        void UpdateStateMapping( THashMap<UUID, UUID> const& IDMapping );

    private:

        virtual bool IsVisible() const override { return false; }
        virtual ImColor GetTitleBarColor() const override { return ImGuiX::ImColors::OrangeRed; }
        virtual char const* GetTypeName() const override { return "Global Transitions"; }
        virtual bool IsUserCreatable() const override { return false; }
        virtual void OnShowNode() override;
    };
}