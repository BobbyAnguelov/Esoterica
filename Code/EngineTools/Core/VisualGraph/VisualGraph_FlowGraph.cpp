#include "VisualGraph_FlowGraph.h"
#include "VisualGraph_UserContext.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph::Flow
{
    Pin const* Node::GetInputPin( UUID const& pinID ) const
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

    Pin const* Node::GetOutputPin( UUID const& pinID ) const
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

    int32_t Node::GetInputPinIndex( UUID const& pinID ) const
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

    int32_t Node::GetOutputPinIndex( UUID const& pinID ) const
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

    void Node::CreateInputPin( char const* pPinName, uint32_t valueType )
    {
        ScopedNodeModification snm( this );
        auto& newPin = m_inputPins.emplace_back( Pin() );
        newPin.m_name = pPinName;
        newPin.m_type = valueType;
        newPin.m_direction = Pin::Direction::In;
    }

    void Node::CreateOutputPin( char const* pPinName, uint32_t valueType, bool allowMultipleOutputConnections )
    {
        ScopedNodeModification snm( this );
        auto& newPin = m_outputPins.emplace_back( Pin() );
        newPin.m_name = pPinName;
        newPin.m_type = valueType;
        newPin.m_direction = Pin::Direction::Out;
        newPin.m_allowMultipleOutConnections = allowMultipleOutputConnections;
    }

    void Node::DestroyInputPin( int32_t pinIdx )
    {
        EE_ASSERT( pinIdx >= 0 && pinIdx < m_inputPins.size() );
        ScopedNodeModification snm( this );
        reinterpret_cast<FlowGraph*>( GetParentGraph() )->BreakAnyConnectionsForPin( m_inputPins[pinIdx].m_ID );
        m_inputPins.erase( m_inputPins.begin() + pinIdx );
    }

    void Node::DestroyOutputPin( int32_t pinIdx )
    {
        EE_ASSERT( pinIdx >= 0 && pinIdx < m_outputPins.size() );
        ScopedNodeModification snm( this );
        reinterpret_cast<FlowGraph*>( GetParentGraph() )->BreakAnyConnectionsForPin( m_outputPins[pinIdx].m_ID );
        m_outputPins.erase( m_outputPins.begin() + pinIdx );
    }

    void Node::DestroyPin( UUID const& pinID )
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

    void Node::CreateDynamicInputPin()
    {
        ScopedNodeModification snm( this );
        auto& newPin = m_inputPins.emplace_back( Pin() );
        newPin.m_name = GetNewDynamicInputPinName();
        newPin.m_type = GetDynamicInputPinValueType();
        newPin.m_direction = Pin::Direction::In;
        newPin.m_isDynamic = true;
        OnDynamicPinCreation( newPin.m_ID );
    }

    void Node::DestroyDynamicInputPin( UUID const& pinID )
    {
        ScopedNodeModification snm( this );
        auto pPin = GetInputPin( pinID );
        EE_ASSERT( pPin != nullptr && pPin->IsDynamicPin() );
        OnDynamicPinDestruction( pinID );
        DestroyPin( pinID );
    }

    //-------------------------------------------------------------------------

    Flow::Node* Node::GetConnectedInputNode( int32_t inputPinIdx ) const
    {
        EE_ASSERT( inputPinIdx >= 0 && inputPinIdx < m_inputPins.size() );
        EE_ASSERT( HasParentGraph() );
        auto const pParentGraph = (FlowGraph const*) GetParentGraph();
        return pParentGraph->GetConnectedNodeForInputPin( m_inputPins[inputPinIdx].m_ID );
    }

    //-------------------------------------------------------------------------

    void Node::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& nodeObjectValue )
    {
        EE_ASSERT( nodeObjectValue.IsObject() );

        //-------------------------------------------------------------------------

        m_inputPins.clear();
        for ( auto& pinObjectValue : nodeObjectValue[s_inputPinsKey].GetArray() )
        {
            Pin& pin = m_inputPins.emplace_back();
            pin.m_direction = Pin::Direction::In;
            pin.m_ID = UUID( pinObjectValue[Pin::s_IDKey].GetString() );
            pin.m_name = pinObjectValue[Pin::s_nameKey].GetString();
            pin.m_type = pinObjectValue[Pin::s_typeKey].GetUint();
            pin.m_isDynamic = pinObjectValue[Pin::s_dynamicKey].GetBool();
        }

        //-------------------------------------------------------------------------

        m_outputPins.clear();
        for ( auto& pinObjectValue : nodeObjectValue[s_outputPinsKey].GetArray() )
        {
            Pin& pin = m_outputPins.emplace_back();
            pin.m_direction = Pin::Direction::Out;
            pin.m_ID = UUID( pinObjectValue[Pin::s_IDKey].GetString() );
            pin.m_name = pinObjectValue[Pin::s_nameKey].GetString();
            pin.m_type = pinObjectValue[Pin::s_typeKey].GetUint();
            pin.m_allowMultipleOutConnections = pinObjectValue[Pin::s_allowMultipleConnectionsKey].GetBool();
        }
    }

    void Node::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const
    {
        writer.Key( s_inputPinsKey );
        writer.StartArray();
        for ( auto const& pin : m_inputPins )
        {
            EE_ASSERT( pin.IsInputPin() );

            writer.StartObject();

            writer.Key( Pin::s_IDKey );
            writer.String( pin.m_ID.ToString().c_str() );

            writer.Key( Pin::s_nameKey );
            writer.String( pin.m_name.c_str() );

            writer.Key( Pin::s_typeKey );
            writer.Uint( pin.m_type );

            writer.Key( Pin::s_dynamicKey );
            writer.Bool( pin.m_isDynamic );

            writer.EndObject();
        }
        writer.EndArray();

        //-------------------------------------------------------------------------

        writer.Key( s_outputPinsKey );
        writer.StartArray();
        for ( auto const& pin : m_outputPins )
        {
            EE_ASSERT( pin.IsOutputPin() );

            writer.StartObject();

            writer.Key( Pin::s_IDKey );
            writer.String( pin.m_ID.ToString().c_str() );

            writer.Key( Pin::s_nameKey );
            writer.String( pin.m_name.c_str() );

            writer.Key( Pin::s_typeKey );
            writer.Uint( pin.m_type );

            writer.Key( Pin::s_allowMultipleConnectionsKey );
            writer.Bool( pin.m_allowMultipleOutConnections );

            writer.EndObject();
        }
        writer.EndArray();
    }

    UUID Node::RegenerateIDs( THashMap<UUID, UUID>& IDMapping )
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
}

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    using namespace Flow;

    //-------------------------------------------------------------------------

    void FlowGraph::PreDestroyNode( BaseNode* pNode )
    {
        BreakAllConnectionsForNode( pNode->GetID() );
        BaseGraph::PreDestroyNode( pNode );
    }

    Flow::Node* FlowGraph::GetNodeForPinID( UUID const& pinID ) const
    {
        for ( auto pNode : m_nodes )
        {
            auto pFlowNode = Cast<Flow::Node>( pNode );
            if ( pFlowNode->HasPin( pinID ) )
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
        EE_ASSERT( pPin->IsDynamicPin() && pPin->IsInputPin() );

        ScopedGraphModification sgm( this );
        BreakAnyConnectionsForPin( pPin->m_ID );
        pNode->DestroyDynamicInputPin( pPin->m_ID );
    }

    //-------------------------------------------------------------------------

    bool FlowGraph::IsValidConnection( Node* pOutputPinNode, Pin* pOutputPin, Node* pInputPinNode, Pin* pInputPin ) const
    {
        EE_ASSERT( pOutputPinNode != nullptr && pOutputPin != nullptr );
        EE_ASSERT( pOutputPinNode->HasOutputPin( pOutputPin->m_ID ) );
        EE_ASSERT( pInputPinNode != nullptr && pInputPin != nullptr );
        EE_ASSERT( pInputPinNode->HasInputPin( pInputPin->m_ID ) );

        //-------------------------------------------------------------------------

        if ( CheckForCyclicConnection( pOutputPinNode, pInputPinNode ) )
        {
            return false;
        }

        if ( !pInputPinNode->IsValidConnection( pInputPin->m_ID, pOutputPinNode, pOutputPin->m_ID ) )
        {
            return false;
        }

        return true;
    }

    bool FlowGraph::TryMakeConnection( Node* pOutputPinNode, Pin* pOutputPin, Node* pInputPinNode, Pin* pInputPin )
    {
        if ( !IsValidConnection( pOutputPinNode, pOutputPin, pInputPinNode, pInputPin ) )
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
        newConnection.m_pStartNode = pOutputPinNode;
        newConnection.m_startPinID = pOutputPin->m_ID;
        newConnection.m_pEndNode = pInputPinNode;
        newConnection.m_endPinID = pInputPin->m_ID;

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
            if ( m_connections[i].m_startPinID == pinID || m_connections[i].m_endPinID == pinID )
            {
                m_connections.erase_unsorted( m_connections.begin() + i );
            }
        }
    }

    void FlowGraph::BreakAllConnectionsForNode( Flow::Node* pNode )
    {
        EE_ASSERT( pNode != nullptr );

        ScopedGraphModification sgm( this );
        for ( int32_t i = int32_t( m_connections.size() ) - 1; i >= 0; i-- )
        {
            if ( m_connections[i].m_pStartNode == pNode || m_connections[i].m_pEndNode == pNode )
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
            if ( m_connections[i].m_pStartNode->GetID() == nodeID || m_connections[i].m_pEndNode->GetID() == nodeID )
            {
                m_connections.erase_unsorted( m_connections.begin() + i );
            }
        }
    }

    Flow::Node* FlowGraph::GetConnectedNodeForInputPin( UUID const& pinID ) const
    {
        for ( auto const& connection : m_connections )
        {
            if ( connection.m_endPinID == pinID )
            {
                EE_ASSERT( connection.m_pEndNode == GetNodeForPinID( pinID ) );
                return connection.m_pStartNode;
            }
        }

        return nullptr;
    }

    void FlowGraph::GetConnectedOutputNodes( Flow::Node const* pOriginalStartNode, Flow::Node const* pNode, TInlineVector<Flow::Node const*, 10>& connectedNodes ) const
    {
        EE_ASSERT( pNode != nullptr );

        for ( auto const& connection : m_connections )
        {
            if ( connection.m_pStartNode == pNode )
            {
                connectedNodes.emplace_back( connection.m_pEndNode );
                if ( pNode != pOriginalStartNode )
                {
                    GetConnectedOutputNodes( pOriginalStartNode, connection.m_pEndNode, connectedNodes );
                }
            }
        }
    }

    bool FlowGraph::CheckForCyclicConnection( Flow::Node const* pOutputNode, Flow::Node const* pInputNode ) const
    {
        EE_ASSERT( pOutputNode != nullptr && pInputNode != nullptr );

        if ( pInputNode == pOutputNode )
        {
            return true;
        }

        TInlineVector<Flow::Node const*, 10> connectedNodes;
        GetConnectedOutputNodes( pInputNode, pInputNode, connectedNodes );
        return VectorContains( connectedNodes, pOutputNode );
    }

    //-------------------------------------------------------------------------

    void FlowGraph::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue )
    {
        EE_ASSERT( graphObjectValue.IsObject() );

        m_connections.clear();
        for ( auto& connectionObjectValue : graphObjectValue[s_connectionsKey].GetArray() )
        {
            EE_ASSERT( connectionObjectValue.IsObject() );
            Connection connection;

            UUID const startNodeID = UUID( connectionObjectValue[Connection::s_startNodeKey].GetString() );
            UUID const endNodeID = UUID( connectionObjectValue[Connection::s_endNodeKey].GetString() );

            connection.m_pStartNode = GetNode( startNodeID );
            connection.m_pEndNode = GetNode( endNodeID );
            connection.m_startPinID = UUID( connectionObjectValue[Connection::s_startPinKey].GetString() );
            connection.m_endPinID = UUID( connectionObjectValue[Connection::s_endPinKey].GetString() );

            if ( connection.m_pStartNode != nullptr && connection.m_pEndNode != nullptr )
            {
                m_connections.emplace_back( connection );
            }
        }
    }

    void FlowGraph::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const
    {
        writer.Key( s_connectionsKey );
        writer.StartArray();

        for ( auto const& connection : m_connections )
        {
            SerializeConnection( writer, connection );
        }

        writer.EndArray();
    }

    void FlowGraph::SerializeConnection( Serialization::JsonWriter& writer, Flow::Connection const& connection ) const
    {
        writer.StartObject();

        writer.Key( Connection::s_startNodeKey );
        writer.String( connection.m_pStartNode->GetID().ToString().c_str() );

        writer.Key( Connection::s_startPinKey );
        writer.String( connection.m_startPinID.ToString().c_str() );

        writer.Key( Connection::s_endNodeKey );
        writer.String( connection.m_pEndNode->GetID().ToString().c_str() );

        writer.Key( Connection::s_endPinKey );
        writer.String( connection.m_endPinID.ToString().c_str() );

        writer.EndObject();
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

            connection.m_startPinID = IDMapping.at( connection.m_startPinID );
            connection.m_endPinID = IDMapping.at( connection.m_endPinID );
        }

        return originalID;
    }
}