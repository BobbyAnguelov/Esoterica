#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/CommonToolTypes.h"
#include "VisualGraph_DrawingContext.h"
#include "Base/Serialization/JsonSerialization.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Esoterica.h"
#include "Base/Types/Event.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    class BaseGraph;
    struct UserContext;

    //-------------------------------------------------------------------------
    // Node Base
    //-------------------------------------------------------------------------
    // NOTE: All node sizes, positions, etc are in canvas coordinates where 1 canvas unit is 1 pixel with no view scaling

    enum class NodeVisualState
    {
        None,
        Active,
        Selected,
        Hovered,
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API BaseNode : public IReflectedType
    {
        friend BaseGraph;
        friend class GraphView;

        // Serialization Keys
        //-------------------------------------------------------------------------

        constexpr static char const* const  s_typeDataKey = "TypeData";
        constexpr static char const* const  s_childGraphKey = "ChildGraph";
        constexpr static char const* const  s_secondaryChildGraphKey = "SecondaryGraph";

    public:

        // Colors
        //-------------------------------------------------------------------------

        constexpr static uint32_t const     s_defaultTitleColor = IM_COL32( 28, 28, 28, 255 );
        constexpr static uint32_t const     s_defaultBackgroundColor = IM_COL32( 64, 64, 64, 255 );
        constexpr static uint32_t const     s_defaultActiveColor = IM_COL32( 50, 205, 50, 255 );
        constexpr static uint32_t const     s_defaultHoveredColor = IM_COL32( 150, 150, 150, 255 );
        constexpr static uint32_t const     s_defaultSelectedColor = IM_COL32( 200, 200, 200, 255 );

        constexpr static uint32_t const     s_connectionColor = IM_COL32( 185, 185, 185, 255 );
        constexpr static uint32_t const     s_connectionColorValid = IM_COL32( 0, 255, 0, 255 );
        constexpr static uint32_t const     s_connectionColorInvalid = IM_COL32( 255, 0, 0, 255 );
        constexpr static uint32_t const     s_connectionColorHovered = IM_COL32( 255, 255, 255, 255 );
        constexpr static uint32_t const     s_connectionColorSelected = IM_COL32( 255, 255, 0, 255 );

        static Color const                  s_genericNodeSeparatorColor;
        static Color const                  s_genericNodeInternalRegionDefaultColor;
        

    public:

        EE_REFLECT_TYPE( BaseNode );

        static BaseNode* TryCreateNodeFromSerializedData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& nodeObjectValue, BaseGraph* pParentGraph );

    public:

        BaseNode() = default;
        BaseNode( BaseNode const& ) = default;
        virtual ~BaseNode();

        BaseNode& operator=( BaseNode const& rhs ) = default;

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

        // Optional function that can be overridden in derived classes to draw a border around the node to signify an active state
        virtual bool IsActive( UserContext* pUserContext ) const { return false; }

        // Should this node be drawn?
        virtual bool IsVisible() const { return true; }

        // Is this node currently hovered
        EE_FORCE_INLINE bool IsHovered() const { return m_isHovered; }

        // Can this node be created and added to a graph by a user? i.e. Does it show up in the context menu
        virtual bool IsUserCreatable() const { return true; }

        // Can this node be destroyed via user input - this is generally tied to whether the user can create a node of this type
        virtual bool IsDestroyable() const { return IsUserCreatable(); }

        // Returns the string path from the root graph (including the current node)
        virtual String GetStringPathFromRoot() const;

        // Returns the ID path from the root graph (including the current node)
        virtual TVector<UUID> GetIDPathFromRoot() const;

        // Returns the node path from the root graph (including the current node)
        virtual TVector<BaseNode*> GetNodePathFromRoot() const;

        // Regenerate UUIDs for this node and its sub-graphs, returns the original ID for the node.
        // The ID mapping will contain all the IDs changed: key = original ID, value = new ID
        virtual UUID RegenerateIDs( THashMap<UUID, UUID>& IDMapping );

        // This gets called immediately after the node is pasted, before the undo action is closed. Allows you to adjust node state as required.
        virtual void PostPaste() {}

        // Node Visuals
        //-------------------------------------------------------------------------

        // Resets the node's calculated size and results in a recalculation
        virtual void ResetCalculatedNodeSizes();

        // Get the margin between the node contents (canvas units) and the outer border.
        virtual Float2 GetNodeMargin() const { return Float2( 8, 4 ); }

        // Get the canvas position for this node (one canvas unit is one pixel when there is no scale)
        EE_FORCE_INLINE Float2 GetPosition() const { return Float2( m_canvasPosition.m_x, m_canvasPosition.m_y ); }

        // Get the rendered node size in canvas units (one canvas unit is one pixel when there is no scale) - this is calculated each frame
        EE_FORCE_INLINE Float2 const& GetSize() const { return m_size; }

        // Get the calculated rendered node width
        EE_FORCE_INLINE float GetWidth() const { return m_size.m_x; }

        // Get the calculated rendered node width
        EE_FORCE_INLINE float GetHeight() const { return m_size.m_y; }

        // Get the rect (in canvas coords) that this node takes (one canvas unit is one pixel when there is no scale)
        ImRect GetRect() const;

        // Set the node's canvas position (one canvas unit is one pixel when there is no scale)
        void SetPosition( Float2 const& newPosition );

        // Get node title bar color
        virtual Color GetTitleBarColor() const { return s_defaultTitleColor; }

        // Get node highlight color
        virtual Color GetNodeBorderColor( DrawContext const& ctx, UserContext* pUserContext, NodeVisualState visualState ) const;

        // Draw an internal separator - negative margin values = use ImGui defaults
        void DrawInternalSeparator( DrawContext const& ctx, Color color = s_genericNodeSeparatorColor, float unscaledPreMarginY = -1, float unscaledPostMarginY = -1 ) const;

        // Start an internal box region - negative margin values = use ImGui defaults
        void BeginDrawInternalRegion( DrawContext const& ctx, Color color = s_genericNodeInternalRegionDefaultColor, float unscaledPreMarginY = -1, float unscaledPostMarginY = -1 ) const;

        // End an internal region
        void EndDrawInternalRegion( DrawContext const& ctx ) const;

        // Graphs
        //-------------------------------------------------------------------------

        // Get the root graph
        BaseGraph* GetRootGraph();

        // Get the root graph
        inline BaseGraph const* GetRootGraph() const { return const_cast<BaseNode*>( this )->GetRootGraph(); }

        // Get our parent node (the parent of our parent graph!)
        bool HasParentNode() const;

        // Get our parent node if we have one
        BaseNode* GetParentNode();

        // Get our parent node if we have one
        BaseNode const* GetParentNode() const;

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

        // Override this if you need to do some logic each frame before the node is drawn - use sparingly
        virtual void PreDrawUpdate( UserContext* pUserContext ) {}

        // Override this if you want to add extra controls to this node (the derived nodes will determine where this content is placed)
        virtual void DrawExtraControls( DrawContext const& ctx, UserContext* pUserContext ) {}

        // Set and initialize the secondary graph
        void SetSecondaryGraph( BaseGraph* pGraph );

        // Set and initialize the child graph
        void SetChildGraph( BaseGraph* pGraph );

        // Called whenever we switch the view to this node
        virtual void OnShowNode() {}

        // Called whenever the user double clicks the node - By default this will request a navigate action to its child when double clicked
        virtual void OnDoubleClick( UserContext* pUserContext );

        // Called before we copy this node
        virtual void PrepareForCopy() {}

        // Allow for custom serialization in derived types (called after secondary/child graphs have been serialized)
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue ) {};

        // Allow for custom serialization in derived types (called before secondary/child graphs have been serialized)
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const {};

    protected:

        EE_REFLECT( "IsToolsReadOnly" : true );
        UUID                        m_ID;
        
        EE_REFLECT( "IsToolsReadOnly" : true );
        Float2                      m_canvasPosition = Float2( 0, 0 ); // Updated each frame

        Float2                      m_size = Float2( 0, 0 ); // Updated each frame (its the actual rendered size in canvas units)
        Float2                      m_titleRectSize = Float2( 0, 0 ); // Updated each frame (is the actual rendered size in canvas units)

    private:

        BaseGraph*                  m_pParentGraph = nullptr; // Private so that we can enforce how we add nodes to the graphs
        BaseGraph*                  m_pChildGraph = nullptr;
        BaseGraph*                  m_pSecondaryGraph = nullptr;
        mutable float               m_internalRegionStartY = -1.0f;
        mutable Color               m_internalRegionColor;
        mutable float               m_internalRegionMargins[2] = {0, 0};
        mutable bool                m_regionStarted = false;
        bool                        m_isHovered = false;
    };

    //-------------------------------------------------------------------------
    // Comment Node
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API CommentNode final : public BaseNode
    {
        friend GraphView;

        EE_REFLECT_TYPE( CommentNode );

    public:

        using BaseNode::BaseNode;

        constexpr static float const s_resizeSelectionRadius = 10.0f;
        constexpr static float const s_minBoxDimensions = ( s_resizeSelectionRadius * 2 ) + 20.0f;

    protected:

        virtual char const* GetTypeName() const override { return "Comment"; }

        virtual bool IsRenameable() const override { return true; }
        virtual char const* GetName() const override { return m_comment.c_str(); }
        virtual void SetName( String const& newName ) override { m_comment = newName; }

        Color GetCommentBoxColor( DrawContext const& ctx, UserContext* pUserContext, NodeVisualState visualState ) const;

        // Draw the context menu options - returns true if the menu should be closed i.e. a custom selection or action has been made
        bool DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos );

        // Get the currently hovered resize handle
        ResizeHandle GetHoveredResizeHandle( DrawContext const& ctx ) const;

        // Get the comment box size (this is the actual full node size in canvas units - including title)
        Float2 const& GetCommentBoxSize() const { return m_commentBoxSize; }

    private:

        CommentNode( CommentNode const& ) = delete;
        CommentNode& operator=( CommentNode const& ) = delete;

        void AdjustSizeBasedOnMousePosition( DrawContext const& ctx, ResizeHandle handle );

    private:

        EE_REFLECT( "IsToolsReadOnly" : true );
        Float2                      m_commentBoxSize = Float2( 0, 0 );

        EE_REFLECT();
        String                      m_comment;

        EE_REFLECT( "IsToolsReadOnly" : true );
        Color                       m_nodeColor = Color( 0xFF4C4C4C );
    };

    //-------------------------------------------------------------------------
    // Graph Base
    //-------------------------------------------------------------------------

    enum class SearchMode { Localized, Recursive };
    enum class SearchTypeMatch { Exact, Derived };

    class EE_ENGINETOOLS_API BaseGraph : public IReflectedType
    {
        friend class GraphView;
        friend class ScopedNodeModification;

        constexpr static char const* const s_nodesKey = "Nodes";

    public:

        EE_REFLECT_TYPE( BaseGraph );

        static BaseGraph* CreateGraphFromSerializedData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue, BaseNode* pParentNode  );

        // Fired whenever a root graph is about to be modified
        static inline TEventHandle<BaseGraph*> OnBeginRootGraphModification() { return s_onBeginRootGraphModification; }

        // Fired whenever a root graph modification has been completed
        static inline TEventHandle<BaseGraph*> OnEndRootGraphModification() { return s_onEndRootGraphModification; }

    private:

        static TEvent<BaseGraph*>                 s_onBeginRootGraphModification;
        static TEvent<BaseGraph*>                 s_onEndRootGraphModification;

    public:

        BaseGraph() = default;
        BaseGraph( BaseGraph const& ) = default;
        virtual ~BaseGraph();

        BaseGraph& operator=( BaseGraph const& rhs ) = default;

        // Lifetime
        //-------------------------------------------------------------------------

        // Called whenever we created a graph for the first time (not called via serialization)
        virtual void Initialize( BaseNode* pParentNode );

        // Called just before we destroy a graph
        virtual void Shutdown();

        // Called whenever we change the view to this graph
        void OnShowGraph();

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
        virtual char const* GetName() const { return HasParentNode() ? m_pParentNode->GetName() : "Root Graph"; }

        // Does this graph support adding comment nodes
        virtual bool SupportsComments() const { return true; }

        // Serialization
        void Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue );
        void Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const;

        // Root Graph
        inline bool IsRootGraph() const { return !HasParentNode(); }

        // Get the root graph for this graph
        BaseGraph* GetRootGraph();

        // Returns the string path from the root graph
        String GetStringPathFromRoot() const;

        // Returns the string path from the root graph
        TVector<UUID> GetIDPathFromRoot() const;

        // Parent Node
        inline bool HasParentNode() const { return m_pParentNode != nullptr; }
        inline BaseNode* GetParentNode() { return m_pParentNode; }
        inline BaseNode const* GetParentNode() const { return m_pParentNode; }

        // Regenerate UUIDs for this graph and children, returns the original ID for the graph
        // The ID mapping will contain all the IDs changed: key = original ID, value = new ID
        virtual UUID RegenerateIDs( THashMap<UUID, UUID>& IDMapping );

        // Node Operations
        //-------------------------------------------------------------------------

        // Get read-only nodes
        TVector<BaseNode*> const& GetNodes() const { return m_nodes; }

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

        // Find all nodes of a specific type in this graph. Note: doesnt clear the results array so ensure you feed in an empty array
        void FindAllNodesOfTypeAdvanced( TypeSystem::TypeID typeID, TFunction<bool( BaseNode const* )> const& matchFunction, TInlineVector<BaseNode*, 20>& results, SearchMode mode = SearchMode::Localized, SearchTypeMatch typeMatch = SearchTypeMatch::Exact ) const;

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
        TInlineVector<T*, 20> FindAllNodesOfTypeAdvanced( TFunction<bool( BaseNode const* )> const& matchFunction, SearchMode depth = SearchMode::Localized, SearchTypeMatch typeMatch = SearchTypeMatch::Exact )
        {
            static_assert( std::is_base_of<BaseNode, T>::value );
            TInlineVector<BaseNode*, 20> intermediateResults;
            FindAllNodesOfTypeAdvanced( T::GetStaticTypeID(), matchFunction, intermediateResults, depth, typeMatch );

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

        template<typename T>
        TInlineVector<T const*, 20> FindAllNodesOfTypeAdvanced( TFunction<bool( BaseNode const* )> const& matchFunction, SearchMode depth = SearchMode::Localized, SearchTypeMatch typeMatch = SearchTypeMatch::Exact ) const
        {
            static_assert( std::is_base_of<BaseNode, T>::value );
            TInlineVector<BaseNode*, 20> intermediateResults;
            FindAllNodesOfTypeAdvanced( T::GetStaticTypeID(), matchFunction, intermediateResults, depth, typeMatch );

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
            {
                // Ensure unique name
                if ( pNode->IsRenameable() )
                {
                    String nodeName = pNode->GetName();
                    if ( nodeName.empty() )
                    {
                        nodeName = pNode->GetTypeName();
                    }

                    EE_ASSERT( !nodeName.empty() );

                    pNode->SetName( GetUniqueNameForRenameableNode( nodeName ) );
                }

                // Add to the graph
                m_nodes.emplace_back( pNode );
            }
            EndModification();
        }

        // Called whenever the user double clicks the graph - By default this will request a navigate action to its parent graph if it has one
        virtual void OnDoubleClick( UserContext* pUserContext );

        // Called whenever we modify a direct child node of this graph
        virtual void OnNodeModified( BaseNode* pModifiedNode ) {}

        // User this to draw any extra contextual information on the graph canvas
        virtual void DrawExtraInformation( DrawContext const& ctx, UserContext* pUserContext ) {}

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

        EE_REFLECT( "IsToolsReadOnly" : true );
        UUID                                    m_ID;

        TVector<BaseNode*>                      m_nodes;

    private:

        BaseNode*                               m_pParentNode = nullptr; // Private so that we can enforce usage
        int32_t                                 m_beginModificationCallCount = 0;

        EE_REFLECT( "IsToolsReadOnly" : true );
        Float2                                  m_viewOffset = Float2( 0, 0 ); // Updated each frame

        EE_REFLECT( "IsToolsReadOnly" : true );
        float                                   m_viewScaleFactor = 1.0f; // View zoom level
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
            auto pParentGraph = m_pNode->GetParentGraph();
            if ( pParentGraph != nullptr )
            {
                pParentGraph->OnNodeModified( m_pNode );
            }
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