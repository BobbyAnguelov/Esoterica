#include "NodeGraph_FlowGraph.h"
#include "NodeGraph_UserContext.h"
#include "NodeGraph_Comment.h"

//-------------------------------------------------------------------------

namespace EE::NodeGraph
{
    Pin::Direction FlowNode::GetPinDirection( UUID const& pinID ) const
    {
        if ( HasInputPin( pinID ) )
        {
            return Pin::Direction::Input;
        }

        if ( HasOutputPin( pinID ) )
        {
            return Pin::Direction::Output;
        }

        return Pin::Direction::None;
    }

    Pin const* FlowNode::GetInputPin( UUID const& pinID ) const
    {
        for ( auto const& pin : m_inputPins )
        {
            if ( pin.m_ID == pinID )
            {
                return &pin;
            }
        }

        return nullptr;
    }

    Pin const* FlowNode::GetOutputPin( UUID const& pinID ) const
    {
        for ( auto const& pin : m_outputPins )
        {
            if ( pin.m_ID == pinID )
            {
                return &pin;
            }
        }

        return nullptr;
    }

    int32_t FlowNode::GetInputPinIndex( UUID const& pinID ) const
    {
        for ( auto i = 0; i < m_inputPins.size(); i++ )
        {
            if ( m_inputPins[i].m_ID == pinID )
            {
                return (int32_t) i;
            }
        }

        return InvalidIndex;
    }

    int32_t FlowNode::GetOutputPinIndex( UUID const& pinID ) const
    {
        for ( auto i = 0; i < m_outputPins.size(); i++ )
        {
            if ( m_outputPins[i].m_ID == pinID )
            {
                return (int32_t) i;
            }
        }

        return InvalidIndex;
    }

    //-------------------------------------------------------------------------

    void FlowNode::CreateInputPin( char const* pPinName, StringID pinType )
    {
        ScopedNodeModification snm( this );
        auto& newPin = m_inputPins.emplace_back( Pin() );
        newPin.m_name = pPinName;
        newPin.m_type = pinType;
    }

    void FlowNode::CreateOutputPin( char const* pPinName, StringID pinType, bool allowMultipleOutputConnections )
    {
        ScopedNodeModification snm( this );
        auto& newPin = m_outputPins.emplace_back( Pin() );
        newPin.m_name = pPinName;
        newPin.m_type = pinType;
        newPin.m_allowMultipleOutConnections = allowMultipleOutputConnections;
    }

    void FlowNode::DestroyInputPin( int32_t pinIdx )
    {
        EE_ASSERT( pinIdx >= 0 && pinIdx < m_inputPins.size() );
        ScopedNodeModification snm( this );
        reinterpret_cast<FlowGraph*>( GetParentGraph() )->BreakAnyConnectionsForPin( m_inputPins[pinIdx].m_ID );
        m_inputPins.erase( m_inputPins.begin() + pinIdx );
    }

    void FlowNode::DestroyOutputPin( int32_t pinIdx )
    {
        EE_ASSERT( pinIdx >= 0 && pinIdx < m_outputPins.size() );
        ScopedNodeModification snm( this );
        reinterpret_cast<FlowGraph*>( GetParentGraph() )->BreakAnyConnectionsForPin( m_outputPins[pinIdx].m_ID );
        m_outputPins.erase( m_outputPins.begin() + pinIdx );
    }

    void FlowNode::DestroyPin( UUID const& pinID )
    {
        for ( auto iter = m_inputPins.begin(); iter != m_inputPins.end(); ++iter )
        {
            if ( iter->m_ID == pinID )
            {
                ScopedNodeModification snm( this );
                reinterpret_cast<FlowGraph*>( GetParentGraph() )->BreakAnyConnectionsForPin( pinID );
                m_inputPins.erase( iter );
                return;
            }
        }

        for ( auto iter = m_outputPins.begin(); iter != m_outputPins.end(); ++iter )
        {
            if ( iter->m_ID == pinID )
            {
                ScopedNodeModification snm( this );
                reinterpret_cast<FlowGraph*>( GetParentGraph() )->BreakAnyConnectionsForPin( pinID );
                m_outputPins.erase( iter );
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    void FlowNode::CreateDynamicInputPin()
    {
        ScopedNodeModification snm( this );
        auto& newPin = m_inputPins.emplace_back( Pin() );
        newPin.m_name = GetNewDynamicInputPinName();
        newPin.m_type = GetDynamicInputPinValueType();
        newPin.m_isDynamic = true;
        OnDynamicPinCreation( newPin.m_ID );
        RefreshDynamicPins();
    }

    void FlowNode::CreateDynamicInputPin( char const* pPinName, StringID pinType )
    {
        ScopedNodeModification snm( this );
        auto& newPin = m_inputPins.emplace_back( Pin() );
        newPin.m_name = pPinName;
        newPin.m_type = pinType;
        newPin.m_isDynamic = true;
        OnDynamicPinCreation( newPin.m_ID );
        RefreshDynamicPins();
    }

    void FlowNode::DestroyDynamicInputPin( UUID const& pinID )
    {
        ScopedNodeModification snm( this );
        auto pPin = GetInputPin( pinID );
        EE_ASSERT( pPin != nullptr && pPin->IsDynamicPin() );
        PreDynamicPinDestruction( pinID );
        DestroyPin( pinID );
        PostDynamicPinDestruction();
        RefreshDynamicPins();
    }

    //-------------------------------------------------------------------------

    template<size_t N>
    static void ReorderPinArray( TVector<UUID> const& newOrder, TInlineVector<Pin, N>& pins )
    {
        // Create unique order list
        //-------------------------------------------------------------------------

        TInlineVector<UUID, N> uniqueOrder;
        for ( int32_t i = 0; i < newOrder.size(); i++ )
        {
            VectorEmplaceBackUnique( uniqueOrder, newOrder[i] );
        }

        // Find all pins that we need to re-order
        //-------------------------------------------------------------------------

        TInlineVector<int32_t, N> pinsToReorder;

        for ( int32_t pinIdx = 0; pinIdx < pins.size(); pinIdx++ )
        {
            if ( VectorContains( uniqueOrder, pins[pinIdx].m_ID ) )
            {
                pinsToReorder.emplace_back( pinIdx );
            }
        }

        //-------------------------------------------------------------------------

        TInlineVector<Pin, N> reorderedList;
        reorderedList.resize( pinsToReorder.size() );

        for ( int32_t i = 0; i < pinsToReorder.size(); i++ )
        {
            int32_t const pinIdx = pinsToReorder[i];
            UUID const& pinID = pins[pinIdx].m_ID;
            int32_t const newIdx = VectorFindIndex( uniqueOrder, pinID );
            reorderedList[newIdx] = pins[pinIdx];
        }

        // Update order
        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < pinsToReorder.size(); i++ )
        {
            int32_t const pinIdx = pinsToReorder[i];
            pins[pinIdx] = reorderedList[i];
        }
    }

    void FlowNode::ReorderInputPins( TVector<UUID> const& newOrder )
    {
        ReorderPinArray( newOrder, m_inputPins );
    }

    void FlowNode::ReorderOutputPins( TVector<UUID> const& newOrder )
    {
        ReorderPinArray( newOrder, m_outputPins );
    }

    //-------------------------------------------------------------------------

    FlowNode* FlowNode::GetConnectedInputNode( int32_t inputPinIdx )
    {
        EE_ASSERT( inputPinIdx >= 0 && inputPinIdx < m_inputPins.size() );
        EE_ASSERT( HasParentGraph() );
        auto pParentGraph = static_cast<FlowGraph*>( GetParentGraph() );
        return pParentGraph->GetConnectedNodeForInputPin( m_inputPins[inputPinIdx].m_ID );
    }

    UUID FlowNode::RegenerateIDs( THashMap<UUID, UUID>& IDMapping )
    {
        UUID const originalID = BaseNode::RegenerateIDs( IDMapping );

        for ( auto& pin : m_inputPins )
        {
            UUID const originalPinID = pin.m_ID;
            pin.m_ID = UUID::GenerateID();

            EE_ASSERT( IDMapping.find( originalPinID ) == IDMapping.end() );
            IDMapping.insert( TPair<UUID, UUID>( originalPinID, pin.m_ID ) );
        }

        for ( auto& pin : m_outputPins )
        {
            UUID const originalPinID = pin.m_ID;
            pin.m_ID = UUID::GenerateID();

            EE_ASSERT( IDMapping.find( originalPinID ) == IDMapping.end() );
            IDMapping.insert( TPair<UUID, UUID>( originalPinID, pin.m_ID ) );
        }

        return originalID;
    }

    void FlowNode::ResetCalculatedNodeSizes()
    {
        BaseNode::ResetCalculatedNodeSizes();
        for ( auto& pin : m_inputPins )
        {
            pin.ResetCalculatedSizes();
        }
        for ( auto& pin : m_outputPins )
        {
            pin.ResetCalculatedSizes();
        }
    }

    void FlowNode::EndModification()
    {
        // Only trigger refresh for nodes in a graph
        if ( HasParentGraph() )
        {
            RefreshDynamicPins();
        }

        BaseNode::EndModification();
    }

    //-------------------------------------------------------------------------

    void FlowGraph::PreDestroyNode( BaseNode* pNode )
    {
        BreakAllConnectionsForNode( pNode->GetID() );
        BaseGraph::PreDestroyNode( pNode );
    }

    FlowNode* FlowGraph::GetNodeForPinID( UUID const& pinID )
    {
        for ( TTypeInstance<BaseNode>& nodeInstance : m_nodes )
        {
            if ( auto pCommentNode = nodeInstance.TryGetAs<CommentNode>() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            auto pFlowNode = nodeInstance.TryGetAs<FlowNode>();
            if ( pFlowNode != nullptr && pFlowNode->HasPin( pinID ) )
            {
                return pFlowNode;
            }
        }

        return nullptr;
    }

    void FlowGraph::CreateDynamicPin( UUID const& nodeID )
    {
        auto pNode = GetNode( nodeID );
        pNode->CreateDynamicInputPin();
    }

    void FlowGraph::DestroyDynamicPin( UUID const& nodeID, UUID const& pinID )
    {
        auto pNode = GetNode( nodeID );
        auto pPin = pNode->GetInputPin( pinID );
        EE_ASSERT( pPin->IsDynamicPin() );

        ScopedGraphModification sgm( this );
        BreakAnyConnectionsForPin( pPin->m_ID );
        pNode->DestroyDynamicInputPin( pPin->m_ID );
    }

    //-------------------------------------------------------------------------

    bool FlowGraph::CanCreateNode( TypeSystem::TypeInfo const* pNodeTypeInfo ) const
    {
        return pNodeTypeInfo->IsDerivedFrom( CommentNode::GetStaticTypeID() ) || pNodeTypeInfo->IsDerivedFrom( FlowNode::GetStaticTypeID() );
    }

    bool FlowGraph::IsValidConnection( FlowNode const* pFromNode, Pin const* pOutputPin, FlowNode const* pToNode, Pin const* pInputPin ) const
    {
        EE_ASSERT( pFromNode != nullptr && pOutputPin != nullptr );
        EE_ASSERT( pFromNode->HasOutputPin( pOutputPin->m_ID ) );
        EE_ASSERT( pToNode != nullptr && pInputPin != nullptr );
        EE_ASSERT( pToNode->HasInputPin( pInputPin->m_ID ) );

        // Cant connect a node to itself
        //-------------------------------------------------------------------------

        if ( pToNode == pFromNode )
        {
            return false;
        }

        // Check if the node specific rules are valid
        //-------------------------------------------------------------------------

        if ( !pToNode->IsValidConnection( pInputPin->m_ID, pFromNode, pOutputPin->m_ID ) )
        {
            return false;
        }

        // Check for cyclic dependencies
        //-------------------------------------------------------------------------

        TInlineVector<FlowNode const*, 10> connectedNodes;
        GetConnectedInputNodes( pFromNode, connectedNodes );
        if ( VectorContains( connectedNodes, pToNode ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        return true;
    }

    bool FlowGraph::IsValidConnection( UUID const& fromNodeID, UUID const& outputPinID, UUID const& toNodeID, UUID const& inputPinID ) const
    {
        FlowNode const* pFromNode = TryCast<FlowNode>( GetNode( fromNodeID ) );
        FlowNode const* pToNode = TryCast<FlowNode>( GetNode( toNodeID ) );

        if ( pFromNode == nullptr || pToNode == nullptr )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        Pin const* pOutputPin = pFromNode->GetOutputPin( outputPinID );
        Pin const* pInputPin = pToNode->GetInputPin( inputPinID );

        if ( pOutputPin == nullptr || pInputPin == nullptr )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        return IsValidConnection( pFromNode, pOutputPin, pToNode, pInputPin );
    }

    bool FlowGraph::TryMakeConnection( FlowNode* pFromNode, Pin* pOutputPin, FlowNode* pToNode, Pin* pInputPin )
    {
        if ( !IsValidConnection( pFromNode, pOutputPin, pToNode, pInputPin ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        ScopedGraphModification sgm( this );

        // Should we break any existing connections
        if ( !pOutputPin->m_allowMultipleOutConnections )
        {
            BreakAnyConnectionsForPin( pOutputPin->m_ID );
        }

        // Break any existing connections for the end pin
        BreakAnyConnectionsForPin( pInputPin->m_ID );

        // Create connection
        auto& newConnection = m_connections.emplace_back();
        newConnection.m_fromNodeID = pFromNode->m_ID;
        newConnection.m_outputPinID = pOutputPin->m_ID;
        newConnection.m_toNodeID = pToNode->m_ID;
        newConnection.m_inputPinID = pInputPin->m_ID;

        return true;
    }

    void FlowGraph::BreakConnection( UUID const& connectionID )
    {
        for ( int32_t i = int32_t( m_connections.size() ) - 1; i >= 0; i-- )
        {
            if ( m_connections[i].m_ID == connectionID )
            {
                ScopedGraphModification sgm( this );
                m_connections.erase_unsorted( m_connections.begin() + i );
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    void FlowGraph::BreakAnyConnectionsForPin( UUID const& pinID )
    {
        ScopedGraphModification sgm( this );
        for ( int32_t i = int32_t( m_connections.size() ) - 1; i >= 0; i-- )
        {
            if ( m_connections[i].m_outputPinID == pinID || m_connections[i].m_inputPinID == pinID )
            {
                m_connections.erase_unsorted( m_connections.begin() + i );
            }
        }
    }

    void FlowGraph::BreakAllConnectionsForNode( FlowNode* pNode )
    {
        EE_ASSERT( pNode != nullptr );

        ScopedGraphModification sgm( this );
        for ( int32_t i = int32_t( m_connections.size() ) - 1; i >= 0; i-- )
        {
            if ( m_connections[i].m_fromNodeID == pNode->m_ID || m_connections[i].m_toNodeID == pNode->m_ID )
            {
                m_connections.erase_unsorted( m_connections.begin() + i );
            }
        }
    }

    void FlowGraph::BreakAllConnectionsForNode( UUID const& nodeID )
    {
        EE_ASSERT( nodeID.IsValid() );

        ScopedGraphModification sgm( this );
        for ( int32_t i = int32_t( m_connections.size() ) - 1; i >= 0; i-- )
        {
            if ( m_connections[i].m_fromNodeID == nodeID || m_connections[i].m_toNodeID == nodeID )
            {
                m_connections.erase_unsorted( m_connections.begin() + i );
            }
        }
    }

    FlowNode* FlowGraph::GetConnectedNodeForInputPin( UUID const& inputPinID )
    {
        for ( Connection& connection : m_connections )
        {
            // Find the connection for this input pin (on the 'To' node)
            if ( connection.m_inputPinID == inputPinID )
            {
                auto pFromNode = GetNode( connection.m_fromNodeID );
                EE_ASSERT( pFromNode != nullptr );
                return pFromNode;
            }
        }

        return nullptr;
    }

    FlowGraph::Connection* FlowGraph::GetConnectionForInputPin( UUID const& inputPinID )
    {
        for ( Connection& connection : m_connections )
        {
            // Find the connection for this input pin (on the 'To' node)
            if ( connection.m_inputPinID == inputPinID )
            {
                // Validate this is a valid connection
                EE_ASSERT( connection.m_toNodeID == GetNodeForPinID( inputPinID )->GetID() );
                return &connection;
            }
        }

        return nullptr;
    }

    bool FlowGraph::GetConnectedInputNodes( FlowNode const* pNode, TInlineVector<FlowNode const*, 10>& connectedNodes ) const
    {
        EE_ASSERT( pNode != nullptr );

        // Check all connection in the graph for the start node
        for ( Connection const& connection : m_connections )
        {
            // If this connection isn't for the node we are checking, then skip it
            if ( connection.m_toNodeID != pNode->GetID() )
            {
                continue;
            }

            // Add the 'from' node as a connected node
            FlowNode* pConnectedNode = Cast<FlowNode>( FindNode( connection.m_fromNodeID ) );

            if ( VectorContains( connectedNodes, pConnectedNode ) )
            {
                return false;
            }
            else
            {
                connectedNodes.emplace_back( pConnectedNode );

                // Recurse
                if ( !GetConnectedInputNodes( pConnectedNode, connectedNodes ) )
                {
                    return false;
                }
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    void FlowGraph::PostDeserialize()
    {
        BaseGraph::PostDeserialize();

        // Refresh all dynamic pins
        for ( TTypeInstance<BaseNode>& nodeInstance : m_nodes )
        {
            auto pFlowNode = nodeInstance.TryGetAs<FlowNode>();
            if ( pFlowNode != nullptr )
            {
                pFlowNode->RefreshDynamicPins();
            }
        }

        // Validate all connections
        for ( int32_t i = int32_t( m_connections.size() - 1 ); i >= 0; i-- )
        {
            Connection& connection = m_connections[i];
            FlowNode* pFromNode = TryCast<FlowNode>( FindNode( connection.m_fromNodeID ) );
            FlowNode* pToNode = TryCast<FlowNode>( FindNode( connection.m_toNodeID ) );

            if ( pFromNode == nullptr || pToNode == nullptr )
            {
                EE_LOG_WARNING( "Tools", "Node Graph", "Invalid connection detected and removed during deserialization" );
                m_connections.erase( m_connections.begin() + i );
                continue;
            }

            Pin* pOutputPin = pFromNode->GetOutputPin( connection.m_outputPinID );
            Pin* pInputPin = pToNode->GetInputPin( connection.m_inputPinID );

            if ( pOutputPin == nullptr || pInputPin == nullptr )
            {
                EE_LOG_WARNING( "Tools", "Node Graph", "Invalid connection detected and removed during deserialization" );
                m_connections.erase( m_connections.begin() + i );
                continue;
            }
        }
    }

    UUID FlowGraph::RegenerateIDs( THashMap<UUID, UUID>& IDMapping )
    {
        UUID const originalID = BaseGraph::RegenerateIDs( IDMapping );

        for ( auto& connection : m_connections )
        {
            UUID const originalConnectionID = connection.m_ID;
            connection.m_ID = UUID::GenerateID();

            EE_ASSERT( IDMapping.find( originalConnectionID ) == IDMapping.end() );
            IDMapping.insert( TPair<UUID, UUID>( originalConnectionID, connection.m_ID ) );

            //-------------------------------------------------------------------------

            connection.m_fromNodeID = IDMapping.at( connection.m_fromNodeID );
            connection.m_outputPinID = IDMapping.at( connection.m_outputPinID );
            connection.m_toNodeID = IDMapping.at( connection.m_toNodeID );
            connection.m_inputPinID = IDMapping.at( connection.m_inputPinID );
        }

        return originalID;
    }
}