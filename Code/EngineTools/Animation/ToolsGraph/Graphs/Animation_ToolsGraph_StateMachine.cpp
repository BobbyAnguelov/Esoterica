#include "Animation_ToolsGraph_StateMachine.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_State.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_EntryStates.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_GlobalTransitions.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_Parameters.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    using namespace EE::Animation::GraphNodes;

    //-------------------------------------------------------------------------

    void StateMachineGraph::Initialize( VisualGraph::BaseNode* pParentNode )
    {
        VisualGraph::StateMachineGraph::Initialize( pParentNode );
    }

    GraphNodes::EntryStateOverrideConduitToolsNode const* StateMachineGraph::GetEntryStateOverrideConduit() const
    {
        auto const foundNodes = FindAllNodesOfType<EntryStateOverrideConduitToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );
        EE_ASSERT( foundNodes.size() == 1 );
        return foundNodes[0];
    }

    GraphNodes::GlobalTransitionConduitToolsNode const* StateMachineGraph::GetGlobalTransitionConduit() const
    {
        auto const foundNodes = FindAllNodesOfType<GlobalTransitionConduitToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );
        EE_ASSERT( foundNodes.size() == 1 );
        return foundNodes[0];
    }

    void StateMachineGraph::CreateNewState( Float2 const& mouseCanvasPos )
    {
        VisualGraph::ScopedGraphModification sgm( this );

        auto pStateNode = EE::New<BlendTreeStateToolsNode>();
        pStateNode->Initialize( this );
        pStateNode->SetCanvasPosition( mouseCanvasPos );
        AddNode( pStateNode );
        UpdateDependentNodes();
    }

    void StateMachineGraph::CreateNewOffState( Float2 const& mouseCanvasPos )
    {
        VisualGraph::ScopedGraphModification sgm( this );

        auto pStateNode = EE::New<OffStateToolsNode>();
        pStateNode->Initialize( this );
        pStateNode->SetCanvasPosition( mouseCanvasPos );
        AddNode( pStateNode );
        UpdateDependentNodes();
    }

    VisualGraph::SM::TransitionConduit* StateMachineGraph::CreateTransitionNode() const
    {
        return EE::New<TransitionConduitToolsNode>();
    }

    bool StateMachineGraph::CanDeleteNode( VisualGraph::BaseNode const* pNode ) const
    {
        auto pStateNode = TryCast<ToolsState>( pNode );
        if ( pStateNode != nullptr )
        {
            auto const stateNodes = FindAllNodesOfType<ToolsState>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
            return stateNodes.size() > 1;
        }

        return VisualGraph::StateMachineGraph::CanDeleteNode( pNode );
    }

    void StateMachineGraph::UpdateDependentNodes()
    {
        GetEntryStateOverrideConduit()->UpdateConditionsNode();
        GetGlobalTransitionConduit()->UpdateTransitionNodes();
    }

    UUID StateMachineGraph::RegenerateIDs( THashMap<UUID, UUID>& IDMapping )
    {
        UUID const originalID = VisualGraph::StateMachineGraph::RegenerateIDs( IDMapping );
        GetEntryStateOverrideConduit()->UpdateStateMapping( IDMapping );
        GetGlobalTransitionConduit()->UpdateStateMapping( IDMapping );
        return originalID;
    }

    void StateMachineGraph::PostPasteNodes( TInlineVector<VisualGraph::BaseNode*, 20> const& pastedNodes )
    {
        VisualGraph::StateMachineGraph::PostPasteNodes( pastedNodes );
        GraphNodes::ParameterReferenceToolsNode::GloballyRefreshParameterReferences( GetRootGraph() );
        UpdateDependentNodes();
    }

    void StateMachineGraph::DrawExtraContextMenuOptions( VisualGraph::DrawContext const& ctx, Float2 const& mouseCanvasPos )
    {
        if ( ImGui::MenuItem( "Blend Tree State" ) )
        {
            CreateNewState( mouseCanvasPos );
        }

        if ( ImGui::MenuItem( "Off State" ) )
        {
            CreateNewOffState( mouseCanvasPos );
        }
    }

    void StateMachineGraph::PostDestroyNode( UUID const& nodeID )
    {
        VisualGraph::StateMachineGraph::PostDestroyNode( nodeID );
        UpdateDependentNodes();
    }
}