#include "Animation_ToolsGraphNode_GlobalTransitions.h"
#include "Animation_ToolsGraphNode_State.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void GlobalTransitionConduitToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        VisualGraph::SM::Node::Initialize( pParent );
        auto pFlowGraph = EE::New<FlowGraph>( GraphType::ValueTree );
        SetSecondaryGraph( pFlowGraph );
        UpdateTransitionNodes();
    }

    void GlobalTransitionConduitToolsNode::OnShowNode()
    {
        VisualGraph::SM::Node::OnShowNode();
        UpdateTransitionNodes();
    }

    void GlobalTransitionConduitToolsNode::UpdateTransitionNodes()
    {
        auto stateNodes = GetParentGraph()->FindAllNodesOfType<GraphNodes::StateToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
        auto globalTransitions = GetSecondaryGraph()->FindAllNodesOfType<GraphNodes::GlobalTransitionToolsNode>();

        //-------------------------------------------------------------------------

        TInlineVector<GraphNodes::StateToolsNode*, 20> transitionsToCreate;
        TInlineVector<GraphNodes::GlobalTransitionToolsNode*, 20> transitionsToRemove;

        for ( auto const& pTransition : globalTransitions )
        {
            transitionsToRemove.emplace_back( pTransition );
        }

        ImVec2 const initialPosition = globalTransitions.empty() ? ImVec2( 0, 0 ) : globalTransitions.front()->GetCanvasPosition();

        // Check transition states
        //-------------------------------------------------------------------------

        auto SearchPredicate = [] ( GraphNodes::GlobalTransitionToolsNode* pNode, UUID const& ID )
        {
            if ( pNode != nullptr )
            {
                return pNode->GetEndStateID() == ID;
            }
            else
            {
                return false;
            }
        };

        for ( auto pState : stateNodes )
        {
            int32_t const stateIdx = VectorFindIndex( transitionsToRemove, pState->GetID(), SearchPredicate );
            if ( stateIdx == InvalidIndex )
            {
                transitionsToCreate.emplace_back( pState );
            }
            else // Found the transition, so update the name
            {
                transitionsToRemove[stateIdx]->m_name = pState->GetName();
                transitionsToRemove[stateIdx] = nullptr;
            }
        }

        // Remove invalid transitions
        //-------------------------------------------------------------------------

        for ( auto const& pTransition : transitionsToRemove )
        {
            if ( pTransition != nullptr )
            {
                pTransition->Destroy();
            }
        }

        // Add new transitions
        //-------------------------------------------------------------------------

        ImVec2 offsetStep( 20, 20 );
        ImVec2 offset = offsetStep;

        for ( auto pState : transitionsToCreate )
        {
            auto pSecondaryGraph = Cast<FlowGraph>( GetSecondaryGraph() );
            auto pNewTransition = pSecondaryGraph->CreateNode<GraphNodes::GlobalTransitionToolsNode>();
            pNewTransition->m_name = pState->GetName();
            pNewTransition->m_stateID = pState->GetID();

            pNewTransition->SetCanvasPosition( initialPosition + offset );
            offset += offsetStep;
        }
    }

    void GlobalTransitionConduitToolsNode::UpdateStateMapping( THashMap<UUID, UUID> const& IDMapping )
    {
        auto globalTransitions = GetSecondaryGraph()->FindAllNodesOfType<GlobalTransitionToolsNode>();
        for ( auto pNode : globalTransitions )
        {
            pNode->m_stateID = IDMapping.at( pNode->m_stateID );
        }
    }

    bool GlobalTransitionConduitToolsNode::HasGlobalTransitionForState( UUID const& stateID ) const
    {
        auto globalTransitions = GetSecondaryGraph()->FindAllNodesOfType<GlobalTransitionToolsNode>();
        for ( auto pGlobalTransition : globalTransitions )
        {
            if ( pGlobalTransition->m_stateID == stateID )
            {
                return pGlobalTransition->GetConnectedInputNode( 0 ) != nullptr;
            }
        }

        // Dont call this function with an invalid state ID
        EE_UNREACHABLE_CODE();
        return false;
    }
}