#include "Animation_ToolsGraphNode_GlobalTransitions.h"
#include "Animation_ToolsGraphNode_State.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    GlobalTransitionConduitToolsNode::GlobalTransitionConduitToolsNode()
        : NodeGraph::StateMachineNode()
    {
        CreateSecondaryGraph<FlowGraph>( GraphType::ValueTree );
        UpdateTransitionNodes();
    }

    void GlobalTransitionConduitToolsNode::OnShowNode()
    {
        NodeGraph::StateMachineNode::OnShowNode();
        UpdateTransitionNodes();
    }

    void GlobalTransitionConduitToolsNode::UpdateTransitionNodes()
    {
        // Do not run the below code for the default instance
        if ( GetParentGraph() == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto stateNodes = GetParentGraph()->FindAllNodesOfType<StateToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
        auto globalTransitions = GetSecondaryGraph()->FindAllNodesOfType<GlobalTransitionToolsNode>();

        //-------------------------------------------------------------------------

        TInlineVector<StateToolsNode*, 20> transitionsToCreate;
        TInlineVector<GlobalTransitionToolsNode*, 20> transitionsToRemove;

        for ( auto const& pTransition : globalTransitions )
        {
            transitionsToRemove.emplace_back( pTransition );
        }

        ImVec2 const initialPosition = globalTransitions.empty() ? ImVec2( 0, 0 ) : ImVec2( globalTransitions.front()->GetPosition() );

        // Check transition states
        //-------------------------------------------------------------------------

        auto SearchPredicate = [] ( GlobalTransitionToolsNode* pNode, UUID const& ID )
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

        for ( StateToolsNode* pState : stateNodes )
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

        for ( GlobalTransitionToolsNode* pTransition : transitionsToRemove )
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
            auto pNewTransition = pSecondaryGraph->CreateNode<GlobalTransitionToolsNode>();
            pNewTransition->m_name = pState->GetName();
            pNewTransition->m_stateID = pState->GetID();

            pNewTransition->SetPosition( initialPosition + offset );
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