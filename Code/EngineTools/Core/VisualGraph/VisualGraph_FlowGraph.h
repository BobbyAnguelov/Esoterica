#pragma once

#include "VisualGraph_BaseGraph.h"
#include "Base/Types/String.h"
#include "Base/Types/UUID.h"

//-------------------------------------------------------------------------
// Flow Graph
//-------------------------------------------------------------------------
// A basic node graph, allowing you to edit a set a of nodes visually
// While this uses the engine's reflection system, the serialization is custom since its serialized to a specific json format

namespace EE::VisualGraph
{
    class FlowGraph;
    class GraphView;

    //-------------------------------------------------------------------------

    namespace Flow
    {
        //-------------------------------------------------------------------------
        // Flow Graph Pin
        //-------------------------------------------------------------------------
        // A pin (connection point) for a node's input/outputs

        struct Pin
        {
            enum class Direction
            {
                In,
                Out
            };

            constexpr static char const* const s_IDKey = "ID";
            constexpr static char const* const s_nameKey = "Name";
            constexpr static char const* const s_typeKey = "Type";
            constexpr static char const* const s_dynamicKey = "IsDynamic";
            constexpr static char const* const s_userDataKey = "UserData";
            constexpr static char const* const s_allowMultipleConnectionsKey = "AllowMultipleConnections";

        public:

            inline bool IsInputPin() const { return m_direction == Direction::In; }
            inline bool IsOutputPin() const { return m_direction == Direction::Out; }
            inline bool IsDynamicPin() const { return m_isDynamic; }

            inline float GetWidth() const { return m_size.m_x; }
            inline float GetHeight() const { return m_size.m_y; }

            void ResetCalculatedSizes()
            {
                m_position = Float2::Zero;
                m_size = Float2( -1, -1 );
            }

        public:

            UUID                    m_ID = UUID::GenerateID();
            String                  m_name;
            StringID                m_type;
            Direction               m_direction;
            Float2                  m_position = Float2( 0, 0 ); // Updated each frame ( rendered window space )
            Float2                  m_size = Float2( -1, -1 ); // Updated each frame ( rendered window space ) - used to render offset correctly;
            uint64_t                m_userData = 0; // Optional user data that can be useful in various tools scenarios
            bool                    m_isDynamic = false; // Only relevant for input pins
            bool                    m_allowMultipleOutConnections = false; // Only relevant for output pins
        };

        //-------------------------------------------------------------------------
        // Flow Graph Node
        //-------------------------------------------------------------------------
        // Base class for a node inside of a flow graph, provides helpers for all common operations
        // Users are expected to derive their own custom nodes from this node 

        class EE_ENGINETOOLS_API Node : public BaseNode
        {
            friend FlowGraph;
            friend GraphView;

            EE_REFLECT_TYPE( Node );

        private:

            constexpr static char const* const  s_inputPinsKey = "InputPins";
            constexpr static char const* const  s_outputPinsKey = "OutputPins";
            constexpr static float const        s_pinSelectionExtraRadius = 10.0f;

        public:

            using BaseNode::BaseNode;

            // Node 
            //-------------------------------------------------------------------------

            // Get node category name (separated via '/')
            virtual char const* GetCategory() const { return nullptr; }

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

            // Does an output pin exist with this index
            inline bool HasInputPin( int32_t pinIdx ) const { return pinIdx >= 0 && pinIdx < m_inputPins.size(); }

            // Get an input pin for this node
            inline Pin* GetInputPin( int32_t pinIdx ) { EE_ASSERT( pinIdx >= 0 && pinIdx < m_inputPins.size() ); return &m_inputPins[pinIdx]; }

            // Get an input pin for this node
            inline Pin const* GetInputPin( int32_t pinIdx ) const { EE_ASSERT( pinIdx >= 0 && pinIdx < m_inputPins.size() ); return &m_inputPins[pinIdx]; }

            // Get a specific input pin via ID
            inline Pin const* GetInputPin( UUID const& pinID ) const;

            // Get a specific input pin via ID
            inline Pin* GetInputPin( UUID const& pinID ) { return const_cast<Pin*>( const_cast<Node const*>( this )->GetInputPin( pinID ) ); }

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

            // Does an output pin exist with this index
            inline bool HasOutputPin( int32_t pinIdx ) const { return pinIdx >= 0 && pinIdx < m_outputPins.size(); }

            // Get an output pin for this node
            inline Pin* GetOutputPin( int32_t pinIdx = 0 ) { EE_ASSERT( pinIdx >= 0 && pinIdx < m_outputPins.size() ); return &m_outputPins[pinIdx]; }

            // Get an output pin for this node
            inline Pin const* GetOutputPin( int32_t pinIdx = 0 ) const { EE_ASSERT( pinIdx >= 0 && pinIdx < m_outputPins.size() ); return &m_outputPins[pinIdx]; }

            // Get a specific output pin via ID
            inline Pin const* GetOutputPin( UUID const& pinID ) const;

            // Get a specific output pin via ID
            inline Pin* GetOutputPin( UUID const& pinID ) { return const_cast<Pin*>( const_cast<Node const*>( this )->GetOutputPin( pinID ) ); }

            // Does the specified pin ID exist on this node
            inline bool HasPin( UUID const& pinID ) const { return HasInputPin( pinID ) || HasOutputPin( pinID ); }

            // Get the index for a specific output pin via ID
            int32_t GetOutputPinIndex( UUID const& pinID ) const;

            // Try to reorder the output pins based on the supplied order
            // This is a complex reorder, in that only the specified pins will be reordered and the remaining pins will remain untouched
            void ReorderOutputPins( TVector<UUID> const& newOrder );

            // Connections
            //-------------------------------------------------------------------------

            // Get the node connected to the specified input pin
            Flow::Node* GetConnectedInputNode( int32_t inputPinIdx ) const;

            template<typename T>
            T* GetConnectedInputNode( int32_t inputPinIdx ) const { return TryCast<T>( GetConnectedInputNode( inputPinIdx ) ); }

            virtual bool IsValidConnection( UUID const& inputPinID, Node const* pOutputPinNode, UUID const& outputPinID ) const
            {
                auto pInputPin = GetInputPin( inputPinID );
                EE_ASSERT( pInputPin != nullptr );

                auto pOutputPin = pOutputPinNode->GetOutputPin( outputPinID );
                EE_ASSERT( pOutputPin != nullptr );

                return pInputPin->m_type == pOutputPin->m_type;
            }

            // Visual
            //-------------------------------------------------------------------------

            // What color should this pin and the connection for it be?
            virtual Color GetPinColor( Pin const& pin ) const { return 0x888888FF; }

        protected:

            virtual void ResetCalculatedNodeSizes() override;

            virtual UUID RegenerateIDs( THashMap<UUID, UUID>& IDMapping ) override;

            // Override this if you want custom UI after/before the pin. Returns true if something was drawn, false otherwise
            virtual bool DrawPinControls( DrawContext const& ctx, UserContext* pUserContext, Pin const& pin ) { return false; }

            // Override this if you want to provide additional context menu options
            virtual void DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos, Pin* pHoveredPin ) {}

            // Create a new dynamic pin
            void CreateDynamicInputPin();

            // Create a new dynamic pin
            void CreateDynamicInputPin( char const* pPinName, StringID pinType );

            // Called just after creating a dynamic pin
            virtual void OnDynamicPinCreation( UUID pinID ) {}

            // Destroy a dynamic pin
            void DestroyDynamicInputPin( UUID const& pinID );

            // Called just before actually destroying a dynamic pin
            virtual void OnDynamicPinDestruction( UUID pinID ) {}

            void CreateInputPin( char const* pPinName, StringID pinType );
            void CreateOutputPin( char const* pPinName, StringID pinType, bool allowMultipleOutputConnections = false );
            void DestroyInputPin( int32_t pinIdx );
            void DestroyOutputPin( int32_t pinIdx );
            
            void DestroyPin( UUID const& pinID );

            // Serialization
            //-------------------------------------------------------------------------

            virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& nodeObjectValue ) override;
            virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const override;

        private:

            Node( Node const& ) = delete;
            Node& operator=( Node const& ) = delete;

        private:

            TInlineVector<Pin, 4>       m_inputPins;
            TInlineVector<Pin, 1>       m_outputPins;
            Pin*                        m_pHoveredPin = nullptr;
        };

        //-------------------------------------------------------------------------
        // Flow Graph Connection
        //-------------------------------------------------------------------------
        // A connection is made between an output pin (start) and an input pin (end) on two different nodes

        struct Connection
        {
            constexpr static char const* const s_startNodeKey = "StartNode";
            constexpr static char const* const s_endNodeKey = "EndNode";
            constexpr static char const* const s_startPinKey = "StartPin";
            constexpr static char const* const s_endPinKey = "EndPin";

        public:

            inline bool IsValid() const { return m_pStartNode != nullptr && m_pEndNode != nullptr && m_startPinID.IsValid() && m_endPinID.IsValid(); }

        public:

            UUID                        m_ID = UUID::GenerateID(); // Transient ID that is only needed for identification in the UI
            Node*                       m_pStartNode = nullptr;
            UUID                        m_startPinID;
            Node*                       m_pEndNode = nullptr;
            UUID                        m_endPinID;
        };
    }

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

        // Nodes
        //-------------------------------------------------------------------------
        // Note: There are no default addition methods exposed, it is up to the derived graphs to create them

        virtual bool CanCreateNode( TypeSystem::TypeInfo const* pNodeTypeInfo ) const override { return pNodeTypeInfo->IsDerivedFrom( Flow::Node::GetStaticTypeID() ); }

        // Destroys and deletes the specified node
        virtual void PreDestroyNode( BaseNode* pNode ) override;

        Flow::Node* GetNode( UUID const& nodeID ) const { return reinterpret_cast<Flow::Node*>( FindNode( nodeID ) ); }
        Flow::Node* GetNodeForPinID( UUID const& pinID ) const;

        void CreateDynamicPin( UUID const& nodeID );
        void DestroyDynamicPin( UUID const& nodeID, UUID const& pinID );

        // Connections
        //-------------------------------------------------------------------------

        bool IsValidConnection( Flow::Node* pOutputPinNode, Flow::Pin* pOutputPin, Flow::Node* pInputPinNode, Flow::Pin* pInputPin ) const;
        bool TryMakeConnection( Flow::Node* pOutputPinNode, Flow::Pin* pOutputPin, Flow::Node* pInputPinNode, Flow::Pin* pInputPin );
        void BreakConnection( UUID const& connectionID );

        void BreakAnyConnectionsForPin( UUID const& pinID );

        void BreakAllConnectionsForNode( Flow::Node* pNode );
        void BreakAllConnectionsForNode( UUID const& nodeID );

        Flow::Node* GetConnectedNodeForInputPin( UUID const& pinID ) const;

        Flow::Connection const* GetConnectionForInputPin( UUID const& pinID ) const;

        void GetConnectedOutputNodes( Flow::Node const* pOriginalStartNode, Flow::Node const* pNode, TInlineVector<Flow::Node const*, 10>& connectedNodes ) const;

        // Context Menu
        //-------------------------------------------------------------------------

        // Do we support creating new nodes directly from a pin
        virtual bool SupportsAutoConnection() const { return false; }

        // Draw the graph context menu options - returns true if the menu should be closed i.e. a custom selection or action has been made
        virtual bool DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos, TVector<String> const& filterTokens, Flow::Node* pSourceNode, Flow::Pin* pSourcePin ) { return false; }

        // Should we show the context menu filter?
        virtual bool HasContextMenuFilter() const { return true; }

        // Serialization
        //-------------------------------------------------------------------------

        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue ) override;
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const override;
        void SerializeConnection( Serialization::JsonWriter& writer, Flow::Connection const& connection ) const;

    protected:

        virtual UUID RegenerateIDs( THashMap<UUID, UUID>& IDMapping ) override;
        bool CheckForCyclicConnection( Flow::Node const* pOutputNode, Flow::Node const* pInputNode ) const;

    public:

        TVector<Flow::Connection>       m_connections;
    };
}