#include "Animation_ToolsGraph_StateMachineGraph.h"
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

    VisualGraph::SM::TransitionConduit* StateMachineGraph::CreateTransitionNode() const
    {
        return EE::New<TransitionConduitToolsNode>();
    }

    bool StateMachineGraph::CanDeleteNode( VisualGraph::BaseNode const* pNode ) const
    {
        auto pStateNode = TryCast<StateToolsNode>( pNode );
        if ( pStateNode != nullptr )
        {
            auto const stateNodes = FindAllNodesOfType<StateToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
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
        UpdateDependentNodes();
    }

    void StateMachineGraph::PostDestroyNode( UUID const& nodeID )
    {
        VisualGraph::StateMachineGraph::PostDestroyNode( nodeID );
        UpdateDependentNodes();
    }

    bool StateMachineGraph::DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, TVector<String> const& filterTokens )
    {
        if ( ctx.m_isReadOnly )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        auto const CreateState = [&, this] ( StateToolsNode::StateType type )
        {
            VisualGraph::ScopedGraphModification sgm( this );
            auto pStateNode = CreateNode<StateToolsNode>( type );
            pStateNode->SetPosition( mouseCanvasPos );
            UpdateDependentNodes();
            return true;
        };

        ImGuiX::TextSeparator( "States" );

        if ( ImGui::MenuItem( "Blend Tree State" ) )
        {
            return CreateState( StateToolsNode::StateType::BlendTreeState );
        }

        if ( ImGui::MenuItem( "State Machine State" ) )
        {
            return CreateState( StateToolsNode::StateType::StateMachineState );
        }

        if ( ImGui::MenuItem( "Off State" ) )
        {
            return CreateState( StateToolsNode::StateType::OffState );
        }

        return false;
    }

    void StateMachineGraph::OnDoubleClick( VisualGraph::UserContext* pUserContext )
    {
        auto pParentNode = GetParentNode();
        auto pGrandParentNode = pParentNode->GetParentGraph()->GetParentNode();
        if ( auto pStateNode = TryCast<GraphNodes::StateToolsNode>( pGrandParentNode ) )
        {
            if ( pStateNode->IsStateMachineState() )
            {
                pUserContext->NavigateTo( pStateNode->GetParentGraph() );
                return;
            }
        }

        VisualGraph::StateMachineGraph::OnDoubleClick( pUserContext );
    }

    void StateMachineGraph::DrawExtraInformation( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        float const scaledIconOffset = ctx.CanvasToWindow( 4.0f );

        auto const stateNodes = FindAllNodesOfType<StateToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
        for ( auto pStateNode : stateNodes )
        {
            ImRect const nodeRect = ctx.CanvasToWindow( pStateNode->GetRect() );
            ImVec2 const iconSize = ImGui::CalcTextSize( EE_ICON_ARROW_DOWN_CIRCLE );
            ImVec2 iconOffset( 0, iconSize.y + scaledIconOffset );

            // Draw entry override marker
            if ( GetDefaultEntryStateID() == pStateNode->GetID() )
            {
                ctx.m_pDrawList->AddText( nodeRect.Min + ctx.m_windowRect.Min - iconOffset, Colors::Gold, EE_ICON_STAR );
                iconOffset.x -= iconSize.x + scaledIconOffset;
            }

            // Draw entry override marker
            EntryStateOverrideConduitToolsNode const* pEntryOverrideConduit = GetEntryStateOverrideConduit();
            if ( pEntryOverrideConduit->HasEntryOverrideForState( pStateNode->GetID() ) )
            {
                ctx.m_pDrawList->AddText( nodeRect.Min + ctx.m_windowRect.Min - iconOffset, Colors::LimeGreen, EE_ICON_ARROW_DOWN_CIRCLE );
                iconOffset.x -= iconSize.x + scaledIconOffset;
            }

            // Draw global transition marker
            GlobalTransitionConduitToolsNode const* pGlobalTransitionConduit = GetGlobalTransitionConduit();
            if ( pGlobalTransitionConduit->HasGlobalTransitionForState( pStateNode->GetID() ) )
            {
                ctx.m_pDrawList->AddText( nodeRect.Min + ctx.m_windowRect.Min - iconOffset, Colors::OrangeRed, EE_ICON_LIGHTNING_BOLT_CIRCLE );
            }
        }
    }
}