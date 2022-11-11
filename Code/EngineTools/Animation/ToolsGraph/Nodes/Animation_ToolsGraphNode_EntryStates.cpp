#include "Animation_ToolsGraphNode_EntryStates.h"
#include "Animation_ToolsGraphNode_State.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void EntryStateOverrideConditionsToolsNode::UpdateInputPins()
    {
        auto pEntryStateOverridesNode = GetParentGraph()->GetParentNode();
        EE_ASSERT( pEntryStateOverridesNode != nullptr );

        auto pParentStateMachineGraph = pEntryStateOverridesNode->GetParentGraph();
        EE_ASSERT( pParentStateMachineGraph != nullptr );

        auto stateNodes = pParentStateMachineGraph->FindAllNodesOfType<StateToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );

        //-------------------------------------------------------------------------

        TInlineVector<StateToolsNode*, 20> pinsToCreate;
        TInlineVector<UUID, 20> pinsToRemove;

        for ( auto const& pin : GetInputPins() )
        {
            pinsToRemove.emplace_back( pin.m_ID );
        }

        // Check which pins are invalid
        //-------------------------------------------------------------------------

        for ( auto pState : stateNodes )
        {
            int32_t const pinIdx = VectorFindIndex( m_pinToStateMapping, pState->GetID() );
            if ( pinIdx == InvalidIndex )
            {
                pinsToCreate.emplace_back( pState );
            }
            else // Found the pin
            {
                pinsToRemove[pinIdx].Clear();
                auto pInputPin = GetInputPin( pinIdx );
                pInputPin->m_name = pState->GetName();
            }
        }

        // Remove invalid pins
        //-------------------------------------------------------------------------

        for ( auto const& pinID : pinsToRemove )
        {
            if ( pinID.IsValid() )
            {
                int32_t const pinIdx = GetInputPinIndex( pinID );
                DestroyInputPin( pinIdx );
                m_pinToStateMapping.erase( m_pinToStateMapping.begin() + pinIdx );
            }
        }

        // Add new pins
        //-------------------------------------------------------------------------

        for ( auto pState : pinsToCreate )
        {
            CreateInputPin( pState->GetName(), GraphValueType::Bool );
            m_pinToStateMapping.emplace_back( pState->GetID() );
        }
    }

    void EntryStateOverrideConditionsToolsNode::OnShowNode()
    {
        FlowToolsNode::OnShowNode();
        UpdateInputPins();
    }

    void EntryStateOverrideConditionsToolsNode::UpdatePinToStateMapping( THashMap<UUID, UUID> const& IDMapping )
    {
        for ( auto& stateID : m_pinToStateMapping )
        {
            stateID = IDMapping.at( stateID );
        }
    }

    //-------------------------------------------------------------------------

    void EntryStateOverrideConduitToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        VisualGraph::SM::Node::Initialize( pParent );
        auto pFlowGraph = EE::New<FlowGraph>( GraphType::ValueTree );
        SetSecondaryGraph( pFlowGraph );
        pFlowGraph->CreateNode<GraphNodes::EntryStateOverrideConditionsToolsNode>();
    }

    FlowToolsNode const* EntryStateOverrideConduitToolsNode::GetEntryConditionNodeForState( UUID const& stateID ) const
    {
        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<EntryStateOverrideConditionsToolsNode>();
        EE_ASSERT( conditionNodes.size() == 1 );
        auto pConditionNode = conditionNodes.back();

        //-------------------------------------------------------------------------

        int32_t const pinIdx = VectorFindIndex( pConditionNode->m_pinToStateMapping, stateID );
        EE_ASSERT( pinIdx != InvalidIndex );
        return TryCast<FlowToolsNode const>( pConditionNode->GetConnectedInputNode( pinIdx ) );
    }

    void EntryStateOverrideConduitToolsNode::UpdateConditionsNode()
    {
        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<EntryStateOverrideConditionsToolsNode>();
        EE_ASSERT( conditionNodes.size() == 1 );
        auto pConditionNode = conditionNodes.back();
        pConditionNode->UpdateInputPins();
    }

    void EntryStateOverrideConduitToolsNode::UpdateStateMapping( THashMap<UUID, UUID> const& IDMapping )
    {
        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<EntryStateOverrideConditionsToolsNode>();
        EE_ASSERT( conditionNodes.size() == 1 );
        auto pConditionNode = conditionNodes.back();
        pConditionNode->UpdatePinToStateMapping( IDMapping );
    }
}