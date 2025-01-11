#pragma once

#include "NodeGraph_BaseGraph.h"
#include "NodeGraph_Comment.h"
#include "Base/Types/String.h"
#include "Base/Types/UUID.h"

//-------------------------------------------------------------------------
// Flow Graph
//-------------------------------------------------------------------------
// A basic node graph, allowing you to edit a set a of nodes visually
// While this uses the engine's reflection system, the serialization is custom since its serialized to a specific json format

namespace EE::NodeGraph
{
    class GraphView;

    //-------------------------------------------------------------------------
    // Flow Graph Pin
    //-------------------------------------------------------------------------
    // A pin (connection point) for a node's input/outputs

    struct EE_ENGINETOOLS_API Pin : public IReflectedType
    {
        EE_REFLECT_TYPE( Pin );

    public:

        enum class Direction
        {
            Input,
            Output,
            None
        };

    public:

        inline bool IsDynamicPin() const { return m_isDynamic; }

        inline float GetWidth() const { return m_size.m_x; }
        inline float GetHeight() const { return m_size.m_y; }

        void ResetCalculatedSizes()
        {
            m_position = Float2::Zero;
            m_size = Float2( -1, -1 );
        }

    public:

        EE_REFLECT( Hidden );
        UUID                    m_ID = UUID::GenerateID();

        EE_REFLECT( Hidden );
        String                  m_name;

        EE_REFLECT( Hidden );
        StringID                m_type;

        EE_REFLECT( Hidden );
        bool                    m_isDynamic = false; // Only relevant for input pins

        EE_REFLECT( Hidden );
        bool                    m_allowMultipleOutConnections = false; // Only relevant for output pins

        EE_REFLECT( Hidden );
        uint64_t                m_userData = 0; // Optional user data that can be useful in various tools scenarios

        Float2                  m_position = Float2( 0, 0 ); // Updated each frame ( rendered window space )
        Float2                  m_size = Float2( -1, -1 ); // Updated each frame ( rendered window space ) - used to render offset correctly;
    };

    //-------------------------------------------------------------------------
    // Flow Graph Node
    //-------------------------------------------------------------------------
    // Base class for a node inside of a flow graph, provides helpers for all common operations
    // Users are expected to derive their own custom nodes from this node 

    class EE_ENGINETOOLS_API FlowNode : public BaseNode
    {
        friend class FlowGraph;
        friend GraphView;

        EE_REFLECT_TYPE( FlowNode );

    private:

        constexpr static float const        s_pinSelectionExtraRadius = 10.0f;

        constexpr static uint32_t const     s_defaultPinColor = IM_COL32( 88, 88, 88, 255 );

    public:

        using BaseNode::BaseNode;

        // Pins
        //-------------------------------------------------------------------------

        // Does the node contain a pin with the specified ID
        inline bool HasPin( UUID const& pinID ) const { return HasInputPin( pinID ) || HasOutputPin( pinID ); }

        // Does the node contain this pin
        inline bool HasPin( Pin const& pin ) const { return HasPin( pin.m_ID ); }

        // Does the node contain this pin
        inline bool HasPin( Pin const* pPin ) const { EE_ASSERT( pPin != nullptr ); return HasPin( pPin->m_ID ); }

        // Get the direction for the specified pin ID (returns none if the pin doesnt exist)
        Pin::Direction GetPinDirection( UUID const& pinID ) const;

        // Get the direction for the specified pin ID (returns none if the pin doesnt exist)
        inline Pin::Direction GetPinDirection( Pin const& pin ) const { return GetPinDirection( pin.m_ID ); }

        // Get the direction for the specified pin ID (returns none if the pin doesnt exist)
        inline Pin::Direction GetPinDirection( Pin const* pPin ) const {EE_ASSERT( pPin != nullptr ); return GetPinDirection( pPin->m_ID ); }

        // Is the specified pin an input pin
        inline bool IsInputPin( UUID const& pinID ) const { return HasInputPin( pinID ); }

        // Is the specified pin an input pin
        inline bool IsInputPin( Pin const& pin ) const { return HasInputPin( pin.m_ID ); }

        // Is the specified pin an input pin
        inline bool IsInputPin( Pin const* pPin ) const { EE_ASSERT( pPin != nullptr ); return HasInputPin( pPin->m_ID ); }

        // Is the specified pin an output pin
        inline bool IsOutputPin( UUID const& pinID ) const { return HasOutputPin( pinID ); }

        // Is the specified pin an output pin
        inline bool IsOutputPin( Pin const& pin ) const { return HasOutputPin( pin.m_ID ); }

        // Is the specified pin an output pin
        inline bool IsOutputPin( Pin const* pPin ) const { EE_ASSERT( pPin != nullptr ); return HasOutputPin(pPin->m_ID); }

        // Input Pins
        //-------------------------------------------------------------------------

        // Does this node support user editing of dynamic input pins
        virtual bool SupportsUserEditableDynamicInputPins() const { return false; }

        // Whats the name of the dynamic inputs
        virtual TInlineString<100> GetNewDynamicInputPinName() const { return "Pin"; }

        // What's the value type of the dynamic inputs
        virtual StringID GetDynamicInputPinValueType() const { return StringID(); }

        // Get the number of input pins on this node
        inline int32_t GetNumInputPins() const { return (int32_t) m_inputPins.size(); }

        // Get all input pins
        inline TInlineVector<Pin, 4> const& GetInputPins() const { return m_inputPins; }

        // Does an input pin exist with this ID
        inline bool HasInputPin( UUID const& pinID ) const { return GetInputPin( pinID ) != nullptr; }

        // Get an input pin for this node
        inline Pin* GetInputPin( int32_t pinIdx ) { EE_ASSERT( pinIdx >= 0 && pinIdx < m_inputPins.size() ); return &m_inputPins[pinIdx]; }

        // Get an input pin for this node
        inline Pin const* GetInputPin( int32_t pinIdx ) const { EE_ASSERT( pinIdx >= 0 && pinIdx < m_inputPins.size() ); return &m_inputPins[pinIdx]; }

        // Get a specific input pin via ID
        inline Pin const* GetInputPin( UUID const& pinID ) const;

        // Get a specific input pin via ID
        inline Pin* GetInputPin( UUID const& pinID ) { return const_cast<Pin*>( const_cast<FlowNode const*>( this )->GetInputPin( pinID ) ); }

        // Get the index for a specific input pin via ID
        int32_t GetInputPinIndex( UUID const& pinID ) const;

        // Try to reorder the input pins based on the supplied order
        // This is a complex reorder, in that only the specified pins will be reordered and the remaining pins will remain untouched
        void ReorderInputPins( TVector<UUID> const& newOrder );

        // Output Pins
        //-------------------------------------------------------------------------

        // Get the number of output pins on this node
        inline int32_t GetNumOutputPins() const { return (int32_t) m_outputPins.size(); }

        // Get all output pins
        inline TInlineVector<Pin, 1> const& GetOutputPins() const { return m_outputPins; }

        // Does this node have an output pin
        inline bool HasOutputPin() const { return !m_outputPins.empty(); }

        // Does an output pin exist with this ID
        inline bool HasOutputPin( UUID const& pinID ) const { return GetOutputPin( pinID ) != nullptr; }

        // Get an output pin for this node
        inline Pin* GetOutputPin( int32_t pinIdx = 0 ) { EE_ASSERT( pinIdx >= 0 && pinIdx < m_outputPins.size() ); return &m_outputPins[pinIdx]; }

        // Get an output pin for this node
        inline Pin const* GetOutputPin( int32_t pinIdx = 0 ) const { EE_ASSERT( pinIdx >= 0 && pinIdx < m_outputPins.size() ); return &m_outputPins[pinIdx]; }

        // Get a specific output pin via ID
        inline Pin const* GetOutputPin( UUID const& pinID ) const;

        // Get a specific output pin via ID
        inline Pin* GetOutputPin( UUID const& pinID ) { return const_cast<Pin*>( const_cast<FlowNode const*>( this )->GetOutputPin( pinID ) ); }

        // Get the index for a specific output pin via ID
        int32_t GetOutputPinIndex( UUID const& pinID ) const;

        // Try to reorder the output pins based on the supplied order
        // This is a complex reorder, in that only the specified pins will be reordered and the remaining pins will remain untouched
        void ReorderOutputPins( TVector<UUID> const& newOrder );

        // Connections
        //-------------------------------------------------------------------------

        // Get the node connected to the specified input pin
        FlowNode* GetConnectedInputNode( int32_t inputPinIdx );

        // Get the node connected to the specified input pin
        template<typename T>
        T* GetConnectedInputNode( int32_t inputPinIdx ) { return TryCast<T>( GetConnectedInputNode( inputPinIdx ) ); }

        // Get the node connected to the specified input pin
        FlowNode const* GetConnectedInputNode( int32_t inputPinIdx ) const { return const_cast<FlowNode*>( this )->GetConnectedInputNode( inputPinIdx ); }

        // Get the node connected to the specified input pin
        template<typename T>
        T const* GetConnectedInputNode( int32_t inputPinIdx ) const { return TryCast<T>( GetConnectedInputNode( inputPinIdx ) ); }

        virtual bool IsValidConnection( UUID const& inputPinID, FlowNode const* pFromNode, UUID const& outputPinID ) const
        {
            auto pInputPin = GetInputPin( inputPinID );
            EE_ASSERT( pInputPin != nullptr );

            auto pOutputPin = pFromNode->GetOutputPin( outputPinID );
            EE_ASSERT( pOutputPin != nullptr );

            return pInputPin->m_type == pOutputPin->m_type;
        }

        // Visual
        //-------------------------------------------------------------------------

        // What color should this pin and the connection for it be?
        virtual Color GetPinColor( Pin const& pin ) const { return s_defaultPinColor; }

    protected:

        virtual void ResetCalculatedNodeSizes() override;

        virtual UUID RegenerateIDs( THashMap<UUID, UUID>& IDMapping ) override;

        virtual void EndModification() override final;

        // Override this if you want custom UI after/before the pin. Returns true if something was drawn, false otherwise
        virtual bool DrawPinControls( DrawContext const& ctx, UserContext* pUserContext, Pin const& pin ) { return false; }

        // Override this if you want to provide additional context menu options
        virtual void DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos, Pin* pHoveredPin ) {}

        // Create a new dynamic pin
        void CreateDynamicInputPin();

        // Create a new dynamic pin
        void CreateDynamicInputPin( char const* pPinName, StringID pinType );

        // Called just after creating a dynamic pin
        virtual void OnDynamicPinCreation( UUID const& createdPinID ) {}

        // Destroy a dynamic pin
        void DestroyDynamicInputPin( UUID const& pinID );

        // Called just before actually destroying a dynamic pin
        virtual void PreDynamicPinDestruction( UUID const& pinID ) {}

        // Called just after actually destroying a dynamic pin
        virtual void PostDynamicPinDestruction() {}

        // Called after we create, destroy or modify node state
        virtual void RefreshDynamicPins() {}

        void CreateInputPin( char const* pPinName, StringID pinType );
        void CreateOutputPin( char const* pPinName, StringID pinType, bool allowMultipleOutputConnections = false );
        void DestroyInputPin( int32_t pinIdx );
        void DestroyOutputPin( int32_t pinIdx );

        void DestroyPin( UUID const& pinID );

        // Event when a node is double clicked and doesnt result in a navigation operation, by default if a node has a navigation target, a double-click performs a navigation operation
        // Return true if you handle the double-click, false otherwise so the event is sent to the listeners on the context
        virtual bool HandleDoubleClick( UserContext* pUserContext, UUID const& hoveredPinID ) { return false; }

    private:

        FlowNode( FlowNode const& ) = delete;
        FlowNode& operator=( FlowNode const& ) = delete;

        virtual bool HandleDoubleClick( UserContext* pUserContext ) override final { return HandleDoubleClick( pUserContext, UUID() ); }

    private:

        EE_REFLECT( Hidden );
        TInlineVector<Pin, 4>       m_inputPins;

        EE_REFLECT( Hidden );
        TInlineVector<Pin, 1>       m_outputPins;

        Pin*                        m_pHoveredPin = nullptr;
    };

    //-------------------------------------------------------------------------
    // Flow Graph
    //-------------------------------------------------------------------------
    // This is a base class as it only provide the most basic drawing and operation functionality
    // Users are expected to derive from this and provide mechanism to create nodes and other operations
    // Note: graphs own the memory for their nodes so they are responsible for managing it

    class EE_ENGINETOOLS_API FlowGraph : public BaseGraph
    {
        EE_REFLECT_TYPE( FlowGraph );

        friend GraphView;

        constexpr static char const* const s_connectionsKey = "Connections";

    public:

        // A connection is made between an output pin (from) and an input pin (to) on two different nodes
        // The names here get confusing since the input node has the output pin and the output node has the input pin
        // This is why we use 'from' and 'to' to define the nodes
        struct Connection : public IReflectedType
        {
            EE_REFLECT_TYPE( Connection );

        public:

            inline bool IsValid() const { return m_fromNodeID.IsValid() && m_outputPinID.IsValid() && m_toNodeID.IsValid() && m_inputPinID.IsValid(); }

        public:

            EE_REFLECT( Hidden );
            UUID                        m_ID = UUID::GenerateID(); // Transient ID that is only needed for identification in the UI


            EE_REFLECT( Hidden );
            UUID                        m_fromNodeID; // The node that owns the output pin

            EE_REFLECT( Hidden );
            UUID                        m_outputPinID;

            EE_REFLECT( Hidden );
            UUID                        m_toNodeID; // The node that owns the input pin

            EE_REFLECT( Hidden );
            UUID                        m_inputPinID;
        };

    public:

        // Nodes
        //-------------------------------------------------------------------------
        // Note: There are no default addition methods exposed, it is up to the derived graphs to create them

        virtual bool CanCreateNode( TypeSystem::TypeInfo const* pNodeTypeInfo ) const override;

        // Destroys and deletes the specified node
        virtual void PreDestroyNode( BaseNode* pNode ) override;

        // Try to get the node with the specified ID
        FlowNode* GetNode( UUID const& nodeID ) { return reinterpret_cast<FlowNode*>( FindNode( nodeID ) ); }

        // Try to get the node with the specified ID
        inline FlowNode const* GetNode( UUID const& nodeID ) const { return reinterpret_cast<FlowNode*>( FindNode( nodeID ) ); }

        // Try to get the node for a given pin ID
        FlowNode* GetNodeForPinID( UUID const& pinID );

        // Try to get the node for a given pin ID
        inline FlowNode const* GetNodeForPinID( UUID const& pinID ) const { return const_cast<FlowGraph*>( this )->GetNodeForPinID( pinID ); }

        // Create a dynamic input pin
        void CreateDynamicPin( UUID const& nodeID );

        // Destroy a dynamic input pin
        void DestroyDynamicPin( UUID const& nodeID, UUID const& pinID );

        // Connections
        //-------------------------------------------------------------------------

        bool IsValidConnection( FlowNode const* pFromNode, Pin const* pOutputPin, FlowNode const* pToNode, Pin const* pInputPin ) const;
        bool IsValidConnection( UUID const& fromNodeID, UUID const& outputPinID, UUID const& toNodeID, UUID const& inputPinID ) const;
        bool TryMakeConnection( FlowNode* pFromNode, Pin* pOutputPin, FlowNode* pToNode, Pin* pInputPin );
        void BreakConnection( UUID const& connectionID );

        void BreakAnyConnectionsForPin( UUID const& pinID );

        void BreakAllConnectionsForNode( FlowNode* pNode );
        void BreakAllConnectionsForNode( UUID const& nodeID );

        FlowNode* GetConnectedNodeForInputPin( UUID const& inputPinID );

        FlowNode const* GetConnectedNodeForInputPin( UUID const& inputPinID ) const { return const_cast<FlowGraph*>( this )->GetConnectedNodeForInputPin( inputPinID ); }

        Connection* GetConnectionForInputPin( UUID const& inputPinID );

        Connection const* GetConnectionForInputPin( UUID const& inputPinID ) const { return const_cast<FlowGraph*>( this )->GetConnectionForInputPin( inputPinID ); }

        // Get the full chain of nodes connected to node - stop when there are no more nodes to find or if it detects a cycle
        // Returns false if a cycle was detected and we earlied out!
        bool GetConnectedInputNodes( FlowNode const* pNode, TInlineVector<FlowNode const*, 10>& connectedNodes ) const;

        // Context Menu
        //-------------------------------------------------------------------------

        // Do we support creating new nodes directly from a pin
        virtual bool SupportsNodeCreationFromConnection() const { return false; }

        // Draw the graph context menu options - returns true if the menu should be closed i.e. a custom selection or action has been made
        virtual bool DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos, TVector<String> const& filterTokens, FlowNode* pSourceNode, Pin* pSourcePin ) { return false; }

        // Should we show the context menu filter?
        virtual bool HasContextMenuFilter() const { return true; }

    protected:

        virtual void PostDeserialize() override;

        virtual UUID RegenerateIDs( THashMap<UUID, UUID>& IDMapping ) override;

    public:

        EE_REFLECT( Hidden );
        TVector<Connection>       m_connections;
    };
}