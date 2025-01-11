#pragma once
#include "EngineTools/NodeGraph/NodeGraph_StateMachineGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EntryStateOverrideConduitToolsNode;
    class GlobalTransitionConduitToolsNode;

    //-------------------------------------------------------------------------

    // The animation state machine graph
    class StateMachineGraph final : public NodeGraph::StateMachineGraph
    {
        EE_REFLECT_TYPE( StateMachineGraph );

    public:

        using NodeGraph::StateMachineGraph::CreateNode;

        EntryStateOverrideConduitToolsNode const* GetEntryStateOverrideConduit() const;
        EntryStateOverrideConduitToolsNode* GetEntryStateOverrideConduit() { return const_cast<EntryStateOverrideConduitToolsNode*>( const_cast<StateMachineGraph const*>( this )->GetEntryStateOverrideConduit() ); }
        GlobalTransitionConduitToolsNode const* GetGlobalTransitionConduit() const;
        GlobalTransitionConduitToolsNode* GetGlobalTransitionConduit() { return const_cast<GlobalTransitionConduitToolsNode*>( const_cast<StateMachineGraph const*>( this )->GetGlobalTransitionConduit() ); }
        
        void UpdateDependentNodes();

    private:

        virtual void DrawExtraInformation( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;
        virtual bool DrawContextMenuOptions( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, TVector<String> const& filterTokens ) override;
        virtual bool HasContextMenuFilter() const override { return false; }
        virtual bool CanDestroyNode( NodeGraph::BaseNode const* pNode ) const override;
        virtual UUID RegenerateIDs( THashMap<UUID, UUID>& IDMapping ) override;
        virtual NodeGraph::TransitionConduitNode* CreateTransitionConduit( NodeGraph::StateNode const* pStartState, NodeGraph::StateNode const* pEndState ) override;
        virtual void PostPasteNodes( TInlineVector<NodeGraph::BaseNode*, 20> const& pastedNodes ) override;
        virtual void PostDestroyNode( UUID const& nodeID ) override;
        virtual BaseGraph* GetNavigationTarget() override;
        virtual void OnNodeModified( NodeGraph::BaseNode* pModifiedNode ) override { UpdateDependentNodes(); }
    };
}