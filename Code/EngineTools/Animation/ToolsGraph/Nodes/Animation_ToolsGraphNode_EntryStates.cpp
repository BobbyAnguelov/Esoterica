#include "Animation_ToolsGraphNode_EntryStates.h"
#include "Animation_ToolsGraphNode_State.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EntryStateOverrideConditionsToolsNode::EntryStateOverrideConditionsToolsNode()
        : ResultToolsNode()
    {
        CreateInputPin( "Condition", GraphValueType::Bool );
    }

    //-------------------------------------------------------------------------

    EntryStateOverrideConduitToolsNode::EntryStateOverrideConduitToolsNode()
        : NodeGraph::StateMachineNode()
    {
        auto pFlowGraph = CreateSecondaryGraph<FlowGraph>( GraphType::EntryOverrideTree );
        pFlowGraph->CreateNode<EntryStateOverrideConditionsToolsNode>();
        UpdateConditionsNode();
    }

    void EntryStateOverrideConduitToolsNode::OnShowNode()
    {
        NodeGraph::StateMachineNode::OnShowNode();
        UpdateConditionsNode();
    }

    FlowToolsNode const* EntryStateOverrideConduitToolsNode::GetEntryConditionNodeForState( UUID const& stateID ) const
    {
        auto entryOverrides = GetSecondaryGraph()->FindAllNodesOfType<EntryStateOverrideConditionsToolsNode>();
        for ( EntryStateOverrideConditionsToolsNode const* pEntryOverrideNode : entryOverrides )
        {
            if ( pEntryOverrideNode->m_stateID == stateID )
            {
                return TryCast<FlowToolsNode>( pEntryOverrideNode->GetConnectedInputNode( 0 ) );
            }
        }

        // Dont call this function with an invalid state ID
        EE_UNREACHABLE_CODE();
        return nullptr;
    }

    void EntryStateOverrideConduitToolsNode::UpdateConditionsNode()
    {
        // Do not run the below code for the default instance
        if ( GetParentGraph() == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto stateNodes = GetParentGraph()->FindAllNodesOfType<StateToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
        auto entryOverrides = GetSecondaryGraph()->FindAllNodesOfType<EntryStateOverrideConditionsToolsNode>();

        //-------------------------------------------------------------------------

        TInlineVector<StateToolsNode*, 20> entryOverridesToCreate;
        TInlineVector<EntryStateOverrideConditionsToolsNode*, 20> entryOverridesToRemove;

        for ( auto const& pEntryOverride : entryOverrides )
        {
            entryOverridesToRemove.emplace_back( pEntryOverride );
        }

        ImVec2 const initialPosition = entryOverrides.empty() ? ImVec2( 0, 0 ) : ImVec2( entryOverrides.front()->GetPosition() );

        // Check entry overrides
        //-------------------------------------------------------------------------

        auto SearchPredicate = [] ( EntryStateOverrideConditionsToolsNode* pNode, UUID const& ID )
        {
            if ( pNode != nullptr )
            {
                return pNode->m_stateID == ID;
            }
            else
            {
                return false;
            }
        };

        for ( StateToolsNode* pState : stateNodes )
        {
            int32_t const stateIdx = VectorFindIndex( entryOverridesToRemove, pState->GetID(), SearchPredicate );
            if ( stateIdx == InvalidIndex )
            {
                entryOverridesToCreate.emplace_back( pState );
            }
            else // Found the transition, so update the name
            {
                entryOverridesToRemove[stateIdx]->m_name = pState->GetName();
                entryOverridesToRemove[stateIdx] = nullptr;
            }
        }

        // Remove invalid entry overrides
        //-------------------------------------------------------------------------

        for ( EntryStateOverrideConditionsToolsNode* pEntryOverride : entryOverridesToRemove )
        {
            if ( pEntryOverride != nullptr )
            {
                pEntryOverride->Destroy();
            }
        }

        // Add new entry overrides
        //-------------------------------------------------------------------------

        ImVec2 offsetStep( 0, 100 );
        ImVec2 offset = offsetStep;

        for ( auto pState : entryOverridesToCreate )
        {
            auto pSecondaryGraph = Cast<FlowGraph>( GetSecondaryGraph() );
            auto pNewTransition = pSecondaryGraph->CreateNode<EntryStateOverrideConditionsToolsNode>();
            pNewTransition->m_name = pState->GetName();
            pNewTransition->m_stateID = pState->GetID();

            pNewTransition->SetPosition( initialPosition + offset );
            offset += offsetStep;
        }
    }

    void EntryStateOverrideConduitToolsNode::UpdateStateMapping( THashMap<UUID, UUID> const& IDMapping )
    {
        auto entryOverrides = GetSecondaryGraph()->FindAllNodesOfType<EntryStateOverrideConditionsToolsNode>();
        for ( auto pNode : entryOverrides )
        {
            pNode->m_stateID = IDMapping.at( pNode->m_stateID );
        }
    }
}