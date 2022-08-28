#pragma once

#include "EngineTools/_Module/API.h"
#include "VisualGraph_DrawingContext.h"
#include "System/Serialization/JsonSerialization.h"
#include "System/TypeSystem/RegisteredType.h"
#include "System/Esoterica.h"
#include "System/Types/Event.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    class BaseGraph;
    struct UserContext;

    //-------------------------------------------------------------------------
    // Node Base
    //-------------------------------------------------------------------------

    enum class NodeVisualState
    {
        None,
        Active,
        Selected,
        Hovered,
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API BaseNode : public IRegisteredType
    {
        friend BaseGraph;
        friend class GraphView;

        constexpr static char const* const s_typeDataKey = "TypeData";
        constexpr static char const* const s_childGraphKey = "ChildGraph";
        constexpr static char const* const s_secondaryChildGraphKey = "SecondaryGraph";

    public:

        EE_REGISTER_TYPE( BaseNode );

        static BaseNode* CreateNodeFromSerializedData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& nodeObjectValue, BaseGraph* pParentGraph );

    public:

        BaseNode() = default;
        virtual ~BaseNode();

        // Lifetime
        //-------------------------------------------------------------------------

        // Called whenever we created a node for the first time (not called via serialization)
        virtual void Initialize( BaseGraph* pParentGraph );

        // Called just before we destroy a node
        virtual void Shutdown();

        // Requests that the parent graph destroys this node
        void Destroy();

        // Undo / Redo
        //-------------------------------------------------------------------------

        // Called whenever an operation starts that will modify the graph state - allowing clients to serialize the before state
        void BeginModification();

        // Called whenever an operation ends that has modified the graph state - allowing clients to serialize the after state
        void EndModification();

        // Node
        //-------------------------------------------------------------------------

        // Get the node unique ID
        inline UUID const& GetID() const { return m_ID; }

        // Get friendly type name
        virtual char const* GetTypeName() const = 0;
        
        // Get node name (not guaranteed to be unique within the same graph)
        virtual char const* GetName() const { return GetTypeName(); }

        // Get optional icon
        virtual char const* GetIcon() const { return nullptr; }

        // Can this node be renamed via user input
        virtual bool IsRenameable() const { return false; }

        // Override this function to set a node's name
        virtual void SetName( String const& newName ) { EE_ASSERT( IsRenameable() ); }

        // Should this node be drawn?
        virtual bool IsVisible() const { return true; }

        // Can this node be created and added to a graph by a user? i.e. Does it show up in the context menu
        virtual bool IsUserCreatable() const { return true; }

        // Can this node be destroyed via user input - this is generally tied to whether the user can create a node of this type
        virtual bool IsDestroyable() const { return IsUserCreatable(); }

        // Returns the string path from the root graph
        virtual String GetPathFromRoot() const;

        // Regenerate UUIDs for this node and its sub-graphs, returns the original ID for the node.
        // The ID mapping will contain all the IDs changed: key = original ID, value = new ID
        virtual UUID RegenerateIDs( THashMap<UUID, UUID>& IDMapping );

        // This gets called immediately after the node is pasted, before the undo action is closed. Allows you to adjust node state as required.
        virtual void PostPaste() {}

        // Node Visuals
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE Float2 const& GetCanvasPosition() const { return m_canvasPosition; }

        EE_FORCE_INLINE Float2 const& GetSize() const { return m_size; }

        EE_FORCE_INLINE float GetWidth() const { return m_size.m_x; }

        EE_FORCE_INLINE float GetHeight() const { return m_size.m_y; }

        // Get the rect (in canvas coords) that this node takes
        ImRect GetCanvasRect() const;

        // Get the rect (in window coords) that this node takes.
        ImRect GetWindowRect( Float2 const& canvasToWindowOffset ) const;

        // Set the node's canvas position
        void SetCanvasPosition( Float2 const& newPosition );

        // Get node title bar color
        virtual ImColor GetTitleBarColor() const { return VisualSettings::s_genericNodeTitleColor; }

        // Optional function that can be overridden in derived classes to draw a border around the node to signify an active state
        virtual bool IsActive( UserContext* pUserContext ) const { return false; }

        // Get node highlight color
        virtual ImColor GetNodeBorderColor( DrawContext const& ctx, UserContext* pUserContext, NodeVisualState visualState ) const;

        // Get the margin between the node contents and the outer border
        virtual Float2 GetNodeMargin() const { return Float2( 8, 4 ); }

        // Graphs
        //-------------------------------------------------------------------------

        // Get the root graph
        BaseGraph* GetRootGraph();

        // Get the root graph
        inline BaseGraph const* GetRootGraph() const { return const_cast<BaseNode*>( this )->GetRootGraph(); }

        // Get our parent graph (all valid nodes should have a parent graph!)
        inline bool HasParentGraph() const { return m_pParentGraph != nullptr; }

        // Get the graph that we belong to
        inline BaseGraph* GetParentGraph() { return m_pParentGraph; }

        // Get the graph that we belong to
        inline BaseGraph const* GetParentGraph() const { return m_pParentGraph; }

        // Does this node represent a graph? e.g. a state machine node
        inline bool HasChildGraph() const { return m_pChildGraph != nullptr; }

        // Get the child graph for this node
        inline BaseGraph const* GetChildGraph() const { return m_pChildGraph; }

        // Get the child graph for this node
        inline BaseGraph* GetChildGraph() { return m_pChildGraph; }

        // Does this node have a graph that needs to be displayed in the secondary view? e.g. a condition graph for a selector node
        inline bool HasSecondaryGraph() const { return m_pSecondaryGraph != nullptr; }

        // Get the secondary graph for this node
        inline BaseGraph* GetSecondaryGraph() { return m_pSecondaryGraph; }

        // Get the secondary graph for this node
        inline BaseGraph const* GetSecondaryGraph() const { return m_pSecondaryGraph; }

        // Serialization
        //-------------------------------------------------------------------------

        void Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& nodeObjectValue );
        void Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const;

    protected:

        // Override this if you want to add extra controls to this node (the derived nodes will determine where this content is placed)
        virtual void DrawExtraControls( DrawContext const& ctx, UserContext* pUserContext ) {}

        // Set and initialize the secondary graph
        void SetSecondaryGraph( BaseGraph* pGraph );

        // Set and initialize the child graph
        void SetChildGraph( BaseGraph* pGraph );

        // Called whenever we switch the view to this node
        virtual void OnShowNode( UserContext* pUserContext ) {}

        // Called whenever the user double clicks the node - By default this will request a navigate action to its child when double clicked
        virtual void OnDoubleClick( UserContext* pUserContext );

        // Allow for custom serialization in derived types (called after secondary/child graphs have been serialized)
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue ) {};

        // Allow for custom serialization in derived types (called before secondary/child graphs have been serialized)
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const {};

    protected:

        EE_REGISTER UUID            m_ID;
        EE_REGISTER Float2          m_canvasPosition = Float2( 0, 0 ); // Updated each frame
        Float2                      m_size = Float2( 0, 0 ); // Updated each frame
        Float2                      m_titleRectSize = Float2( 0, 0 ); // Updated each frame
        bool                        m_isHovered = false;

    private:

        BaseGraph*                  m_pParentGraph = nullptr; // Private so that we can enforce how we add nodes to the graphs
        BaseGraph*                  m_pChildGraph = nullptr;
        BaseGraph*                  m_pSecondaryGraph = nullptr;
    };

    //-------------------------------------------------------------------------
    // Graph Base
    //-------------------------------------------------------------------------

    enum class SearchMode { Localized, Recursive };
    enum class SearchTypeMatch { Exact, Derived };

    class EE_ENGINETOOLS_API BaseGraph : public IRegisteredType
    {
        friend class GraphView;

        constexpr static char const* const s_nodesKey = "Nodes";

    public:

        EE_REGISTER_TYPE( BaseGraph );

        static BaseGraph* CreateGraphFromSerializedData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue, BaseNode* pParentNode  );

        static inline TEventHandle<BaseGraph*> OnBeginModification() { return s_onBeginModification; }
        static inline TEventHandle<BaseGraph*> OnEndModification() { return s_onEndModification; }

    private:

        static TEvent<BaseGraph*>                 s_onBeginModification;
        static TEvent<BaseGraph*>                 s_onEndModification;

    public:

        BaseGraph() = default;
        virtual ~BaseGraph();

        // Lifetime
        //-------------------------------------------------------------------------

        // Called whenever we created a graph for the first time (not called via serialization)
        virtual void Initialize( BaseNode* pParentNode );

        // Called just before we destroy a graph
        virtual void Shutdown();

        // Called whenever we change the view to this graph
        void OnShowGraph( UserContext* pUserContext );

        // Undo/Redo
        //-------------------------------------------------------------------------

        // Called whenever an operation starts that will modify the graph state - allowing clients to serialize the before state
        void BeginModification();

        // Called whenever an operation ends that has modified the graph state - allowing clients to serialize the after state
        void EndModification();

        // Graph
        //-------------------------------------------------------------------------

        inline UUID const& GetID() const { return m_ID; }

        // Get the display title for this graph
        virtual char const* GetTitle() const { return HasParentNode() ? m_pParentNode->GetName() : "Root Graph"; }

        // Serialization
        void Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue );
        void Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const;

        // Root Graph
        inline bool IsRootGraph() const { return !HasParentNode(); }
        BaseGraph* GetRootGraph();

        // Parent Node
        inline bool HasParentNode() const { return m_pParentNode != nullptr; }
        inline BaseNode* GetParentNode() { return m_pParentNode; }
        inline BaseNode const* GetParentNode() const { return m_pParentNode; }

        // Regenerate UUIDs for this graph and children, returns the original ID for the graph
        // The ID mapping will contain all the IDs changed: key = original ID, value = new ID
        virtual UUID RegenerateIDs( THashMap<UUID, UUID>& IDMapping );

        // Node Operations
        //-------------------------------------------------------------------------

        // Returns the most significant node for a given graph, could be the final result node or the default state node, etc...
        virtual BaseNode const* GetMostSignificantNode() const { return nullptr; }

        // Destroys and deletes the specified node (UUID copy is intentional)
        void DestroyNode( UUID nodeID );

        // Override this for additional control of what nodes can be placed
        virtual bool CanCreateNode( TypeSystem::TypeInfo const* pNodeTypeInfo ) const { return true; }

        // Override this for additional control of what nodes can be deleted
        virtual bool CanDeleteNode( BaseNode const* pNode )const { return true; }

        // Helpers
        //-------------------------------------------------------------------------

        // Find a specific node in the graph
        inline BaseNode* FindNode( UUID const& nodeID, bool checkRecursively = false ) const
        {
            for ( auto pNode : m_nodes )
            {
                if ( pNode->GetID() == nodeID )
                {
                    return pNode;
                }

                if ( checkRecursively )
                {
                    if ( pNode->HasChildGraph() )
                    {
                        auto pFoundNode = pNode->GetChildGraph()->FindNode( nodeID, checkRecursively );
                        if ( pFoundNode != nullptr )
                        {
                            return pFoundNode;
                        }
                    }

                    if ( pNode->HasSecondaryGraph() )
                    {
                        auto pFoundNode = pNode->GetSecondaryGraph()->FindNode( nodeID, checkRecursively );
                        if ( pFoundNode != nullptr )
                        {
                            return pFoundNode;
                        }
                    }
                }
            }

            return nullptr;
        }

        // Find a specific primary graph
        BaseGraph* FindPrimaryGraph( UUID const& graphID )
        {
            if ( m_ID == graphID )
            {
                return this;
            }

            for ( auto pNode : m_nodes )
            {
                if ( pNode->HasChildGraph() )
                {
                    auto pFoundGraph = pNode->GetChildGraph()->FindPrimaryGraph( graphID );
                    if ( pFoundGraph != nullptr )
                    {
                        return pFoundGraph;
                    }
                }
            }

            return nullptr;
        }

        // Find all nodes of a specific type in this graph. Note: doesnt clear the results array so ensure you feed in an empty array
        void FindAllNodesOfType( TypeSystem::TypeID typeID, TInlineVector<BaseNode*, 20>& results, SearchMode mode = SearchMode::Localized, SearchTypeMatch typeMatch = SearchTypeMatch::Exact ) const;

        template<typename T>
        TInlineVector<T*, 20> FindAllNodesOfType( SearchMode depth = SearchMode::Localized, SearchTypeMatch typeMatch = SearchTypeMatch::Exact )
        {
            static_assert( std::is_base_of<BaseNode, T>::value );
            TInlineVector<BaseNode*, 20> intermediateResults;
            FindAllNodesOfType( T::GetStaticTypeID(), intermediateResults, depth, typeMatch );

            // Transfer results to typed array
            TInlineVector<T*, 20> results;
            for ( auto const& pFoundNode : intermediateResults )
            {
                results.emplace_back( Cast<T>( pFoundNode ) );
            }
            return results;
        }

        template<typename T>
        TInlineVector<T const*, 20> FindAllNodesOfType( SearchMode depth = SearchMode::Localized, SearchTypeMatch typeMatch = SearchTypeMatch::Exact ) const
        {
            static_assert( std::is_base_of<BaseNode, T>::value );
            TInlineVector<BaseNode*, 20> intermediateResults;
            FindAllNodesOfType( T::GetStaticTypeID(), intermediateResults, depth, typeMatch );

            // Transfer results to typed array
            TInlineVector<T const*, 20> results;
            for ( auto const& pFoundNode : intermediateResults )
            {
                results.emplace_back( Cast<T>( pFoundNode ) );
            }
            return results;
        }

        // Get a unique name for a renameable node within this graph
        String GetUniqueNameForRenameableNode( String const& desiredName, BaseNode const* m_pNodeToIgnore = nullptr ) const;

    protected:

        // Adds a node to this graph - Note: this transfers ownership of the node memory to this graph!
        // Adding of a node must go through this code path as we need to set the parent node ptr
        void AddNode( BaseNode* pNode )
        {
            EE_ASSERT( pNode != nullptr && pNode->m_ID.IsValid() );
            EE_ASSERT( FindNode( pNode->m_ID ) == nullptr );
            EE_ASSERT( pNode->m_pParentGraph == this );
            BeginModification();
            m_nodes.emplace_back( pNode );
            EndModification();
        }

        // Called whenever the user double clicks the graph - By default this will request a navigate action to its parent graph if it has one
        virtual void OnDoubleClick( UserContext* pUserContext );

        // User this to draw any extra contextual information on the graph canvas
        virtual void DrawExtraInformation( DrawContext const& ctx, UserContext* pUserContext ) {}

        // Draw the graph context menu option
        virtual void DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos ) {}

        // Called whenever a drag and drop operation occurs
        virtual void HandleDragAndDrop( UserContext* pUserContext, ImVec2 const& mouseCanvasPos ) {}

        // Called after we finish pasting nodes (allows for child graphs to run further processes)
        virtual void PostPasteNodes( TInlineVector<BaseNode*, 20> const& pastedNodes ) {}

        // Called just before we destroy a node, allows derived graphs to handle the event
        virtual void PreDestroyNode( BaseNode* pNodeAboutToBeDestroyed ) {};

        // Called after we destroy a node, allows derived graphs to handle the event.
        virtual void PostDestroyNode( UUID const& nodeID ) {};

        // Allow for custom serialization in derived types
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue ) {}
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const {}

    protected:

        EE_REGISTER UUID                        m_ID;
        TVector<BaseNode*>                      m_nodes;

    private:
        BaseNode*                               m_pParentNode = nullptr; // Private so that we can enforce usage
        int32_t                                 m_beginModificationCallCount = 0;
        EE_REGISTER Float2                      m_viewOffset = Float2( 0, 0 ); // Updated each frame
    };

    //-------------------------------------------------------------------------

    class [[nodiscard]] ScopedNodeModification
    {
    public:

        ScopedNodeModification( BaseNode* pNode )
            : m_pNode( pNode )
        {
            EE_ASSERT( pNode != nullptr );
            m_pNode->BeginModification();
        }

        ~ScopedNodeModification()
        {
            m_pNode->EndModification();
        }

    private:

        BaseNode*  m_pNode = nullptr;
    };

    class [[nodiscard]] ScopedGraphModification
    {
    public:

        ScopedGraphModification( BaseGraph* pGraph )
            : m_pGraph( pGraph )
        {
            EE_ASSERT( pGraph != nullptr );
            m_pGraph->BeginModification();
        }

        ~ScopedGraphModification()
        {
            m_pGraph->EndModification();
        }

    private:

        BaseGraph*  m_pGraph = nullptr;
    };
}