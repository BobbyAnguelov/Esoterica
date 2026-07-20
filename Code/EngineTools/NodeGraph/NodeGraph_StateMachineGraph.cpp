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

    void TransitionConduitNode::SetStartStateID( UUID const &startStateID )
    {
        auto pParentGraph = Cast<StateMachineGraph>( GetParentGraph() );

        EE_ASSERT( startStateID.IsValid() );
        EE_ASSERT( pParentGraph->FindNode( startStateID ) != nullptr );
        EE_ASSERT( !pParentGraph->DoesTransitionConduitExist( startStateID, m_endStateID ) );

        ScopedNodeModification const snm( this );
        m_startStateID = startStateID;
    }

    void TransitionConduitNode::SetEndStateID( UUID const &endStateID )
    {
        auto pParentGraph = Cast<StateMachineGraph>( GetParentGraph() );

        EE_ASSERT( endStateID.IsValid() );
        EE_ASSERT( pParentGraph->FindNode( endStateID ) != nullptr );
        EE_ASSERT( !pParentGraph->DoesTransitionConduitExist( m_startStateID, endStateID ) );

        ScopedNodeModification const snm( this );
        m_endStateID = endStateID;
    }

    void TransitionConduitNode::InvertDirection()
    {
        auto pParentGraph = Cast<StateMachineGraph>( GetParentGraph() );
        EE_ASSERT( !pParentGraph->DoesTransitionConduitExist( m_endStateID, m_startStateID ) );

        ScopedNodeModification const snm( this );

        UUID tmp = m_startStateID;
        m_startStateID = m_endStateID;
        m_endStateID = tmp;
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

    bool StateMachineGraph::DoesTransitionConduitExist( UUID const& startStateID, UUID const &endStateID ) const
    {
        auto pStartState = Cast<StateNode>( FindNode( startStateID ) );
        auto pEndState = Cast<StateNode>( FindNode( endStateID ) );

        if ( pStartState == nullptr || pEndState == nullptr )
        {
            return false;
        }

        return DoesTransitionConduitExist( pStartState, pEndState );
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

    bool StateMachineGraph::CanCreateTransitionConduit( UUID const &startStateID, UUID const &endStateID ) const
    {
        auto pStartState = Cast<StateNode>( FindNode( startStateID ) );
        auto pEndState = Cast<StateNode>( FindNode( endStateID ) );

        if ( pStartState == nullptr || pEndState == nullptr )
        {
            return false;
        }

        return CanCreateTransitionConduit( pStartState, pEndState );
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

    void StateMachineGraph::PostDeserialize( TypeSystem::TypeRegistry const& typeRegistry )
    {
        BaseGraph::PostDeserialize( typeRegistry );

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