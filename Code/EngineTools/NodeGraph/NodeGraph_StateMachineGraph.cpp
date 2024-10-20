#include "NodeGraph_StateMachineGraph.h"
#include "NodeGraph_UserContext.h"
#include "NodeGraph_Style.h"

//-------------------------------------------------------------------------

namespace EE::NodeGraph
{
    TransitionConduitNode::TransitionConduitNode( NodeGraph::StateNode const* pStartState, NodeGraph::StateNode const* pEndState )
        : StateMachineNode()
        , m_startStateID( pStartState->GetID() )
        , m_endStateID( pEndState->GetID() )
    {}

    Color TransitionConduitNode::GetConduitColor( NodeGraph::DrawContext const& ctx, UserContext* pUserContext, TBitFlags<NodeGraph::NodeVisualState> visualState ) const
    {
        if ( visualState.IsFlagSet( NodeVisualState::Active ) )
        {
            return NodeGraph::Style::s_connectionColorActive;
        }
        else if ( visualState.IsFlagSet( NodeVisualState::Hovered ) )
        {
            return NodeGraph::Style::s_connectionColorHovered;
        }
        else if ( visualState.IsFlagSet( NodeVisualState::Selected ) )
        {
            return NodeGraph::Style::s_connectionColorSelected;
        }
        else
        {
            return NodeGraph::Style::s_connectionColor;
        }
    }

    //-------------------------------------------------------------------------

    bool StateMachineGraph::CanCreateNode( TypeSystem::TypeInfo const* pNodeTypeInfo ) const
    {
        return pNodeTypeInfo->IsDerivedFrom( CommentNode::GetStaticTypeID() ) || pNodeTypeInfo->IsDerivedFrom( StateMachineNode::GetStaticTypeID() );
    }

    void StateMachineGraph::SetDefaultEntryState( UUID const& newDefaultEntryStateID )
    {
        EE_ASSERT( newDefaultEntryStateID.IsValid() && FindNode( newDefaultEntryStateID ) != nullptr );
        ScopedGraphModification sgm( this );
        m_entryStateID = newDefaultEntryStateID;
    }

    bool StateMachineGraph::DoesTransitionConduitExist( StateNode const* pStartState, StateNode const* pEndState ) const
    {
        EE_ASSERT( pStartState != nullptr && pEndState != nullptr );
        EE_ASSERT( pStartState != pEndState );

        auto const conduits = FindAllNodesOfType<TransitionConduitNode>( SearchMode::Localized, SearchTypeMatch::Derived );
        for ( auto pConduit : conduits )
        {
            if ( pConduit->m_startStateID == pStartState->m_ID && pConduit->m_endStateID == pEndState->m_ID )
            {
                return true;
            }
        }

        return false;
    }

    bool StateMachineGraph::CanCreateTransitionConduit( StateNode const* pStartState, StateNode const* pEndState ) const
    {
        EE_ASSERT( pStartState != nullptr && pEndState != nullptr );

        if ( pStartState == pEndState )
        {
            return false;
        }

        if ( DoesTransitionConduitExist( pStartState, pEndState ) )
        {
            return false;
        }

        return true;
    }

    void StateMachineGraph::PreDestroyNode( BaseNode* pNodeAboutToBeDestroyed )
    {
        auto pStateNode = TryCast<StateNode>( pNodeAboutToBeDestroyed );
        if ( pStateNode != nullptr )
        {
            auto const transitions = FindAllNodesOfType<TransitionConduitNode>( SearchMode::Localized, SearchTypeMatch::Derived );
            for ( auto pTransition : transitions )
            {
                if ( pTransition->m_startStateID == pStateNode->m_ID || pTransition->m_endStateID == pStateNode->m_ID )
                {
                    DestroyNode( pTransition->GetID() );
                }
            }
        }

        BaseGraph::PreDestroyNode( pNodeAboutToBeDestroyed );
    }

    void StateMachineGraph::PostDestroyNode( UUID const& nodeID )
    {
        if ( m_entryStateID == nodeID )
        {
            UpdateEntryState();
        }

        BaseGraph::PostDestroyNode( nodeID );
    }

    void StateMachineGraph::PostDeserialize()
    {
        BaseGraph::PostDeserialize();

        // Ensure that the entry state is valid
        if ( FindNode( m_entryStateID ) == nullptr )
        {
            UpdateEntryState();
        }
    }

    void StateMachineGraph::UpdateEntryState()
    {
        m_entryStateID.Clear();

        // Set to the first state we find
        for ( TTypeInstance<BaseNode>& nodeInstance : m_nodes )
        {
            if ( IsOfType<StateNode>( nodeInstance.Get() ) )
            {
                m_entryStateID = nodeInstance->GetID();
                break;
            }
        }
    }

    UUID StateMachineGraph::RegenerateIDs( THashMap<UUID, UUID>& IDMapping )
    {
        UUID const originalID = BaseGraph::RegenerateIDs( IDMapping );

        // Update conduit UUIDs
        //-------------------------------------------------------------------------

        for ( TTypeInstance<BaseNode>& nodeInstance : m_nodes )
        {
            if ( auto pConduitNode = nodeInstance.TryGetAs<TransitionConduitNode>() )
            {
                pConduitNode->m_startStateID = IDMapping.at( pConduitNode->m_startStateID );
                pConduitNode->m_endStateID = IDMapping.at( pConduitNode->m_endStateID );
            }
        }

        // Update entry state ID
        //-------------------------------------------------------------------------

        m_entryStateID = IDMapping.at( m_entryStateID );

        return originalID;
    }
}