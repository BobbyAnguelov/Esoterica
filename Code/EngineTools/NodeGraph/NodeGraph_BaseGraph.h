#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/CommonToolTypes.h"
#include "NodeGraph_DrawingContext.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Esoterica.h"
#include "Base/Types/Event.h"
#include "Base/Types/Function.h"
#include "Base/TypeSystem/TypeInstance.h"

//-------------------------------------------------------------------------

namespace EE::NodeGraph
{
    class BaseGraph;
    struct UserContext;

    //-------------------------------------------------------------------------
    // Node Base
    //-------------------------------------------------------------------------
    // NOTE: All node sizes, positions, etc are in canvas coordinates where 1 canvas unit is 1 pixel with no view scaling

    enum class NodeVisualState
    {
        Active = 0,
        Selected,
        Hovered,
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API BaseNode : public IReflectedType
    {
        friend BaseGraph;
        friend class GraphView;

    public:

        EE_REFLECT_TYPE( BaseNode );

    public:

        BaseNode() = default;
        BaseNode( BaseNode const& ) = default;
        virtual ~BaseNode();

        BaseNode& operator=( BaseNode const& rhs ) = default;

        // Lifetime
        //-------------------------------------------------------------------------

        // Requests that the parent graph destroys this node
        void Destroy();

        // Undo / Redo
        //-------------------------------------------------------------------------

        // Called whenever an operation starts that will modify the graph state - allowing clients to serialize the before state
        virtual void BeginModification();

        // Called whenever an operation ends that has modified the graph state - allowing clients to serialize the after state
        virtual void EndModification();

        // Node
        //-------------------------------------------------------------------------

        // Get the node unique ID
        inline UUID const& GetID() const { return m_ID; }

        // Get friendly type name
        virtual char const* GetTypeName() const = 0;
        
        // Get node name (not guaranteed to be unique within the same graph)
        virtual char const* GetName() const { return m_name.empty() ? GetTypeName() : m_name.c_str(); }

        // Get node category name (separated via '/')
        virtual char const* GetCategory() const { return nullptr; }

        // Get optional icon
        virtual char const* GetIcon() const { return nullptr; }

        // Can this node be renamed via user input
        virtual bool IsRenameable() const { return false; }

        // Does this renameable node require a unique name within the context of the parent graph (override the create unique name function to override the behavior)
        virtual bool RequiresUniqueName() const { return false; }

        // Call this function to rename a node
        void Rename( String const& newName );

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
        virtual Color GetTitleBarColor() const;

        // Draw an internal separator - negative margin values = use ImGui defaults
        void DrawInternalSeparatorColored( DrawContext const& ctx, Color color, float unscaledPreMarginY = -1, float unscaledPostMarginY = -1 ) const;

        // Start an internal box region - negative margin values = use ImGui defaults
        void DrawInternalSeparator( DrawContext const& ctx, float unscaledPreMarginY = -1, float unscaledPostMarginY = -1 ) const;

        // Start an internal box region - negative margin values = use ImGui defaults
        void BeginDrawInternalRegionColored( DrawContext const& ctx, Color color, float unscaledPreMarginY = -1, float unscaledPostMarginY = -1 ) const;

        // Start an internal box region - negative margin values = use ImGui defaults
        void BeginDrawInternalRegion( DrawContext const& ctx, float unscaledPreMarginY = -1, float unscaledPostMarginY = -1 ) const;

        // End an internal region
        void EndDrawInternalRegion( DrawContext const& ctx ) const;

        // Graphs
        //-------------------------------------------------------------------------

        // Get the root graph
        BaseGraph* GetRootGraph();

        // Get the root graph
        inline BaseGraph const* GetRootGraph() const { return const_cast<BaseNode*>( this )->GetRootGraph(); }

        // Do we have a parent node (the parent of our parent graph!)
        bool HasParentNode() const;

        // Get our parent node (the parent of our parent graph!) if we have one 
        BaseNode* GetParentNode();

        // Get our parent node if we have one
        BaseNode const* GetParentNode() const;

        // Do we have a parent graph (all valid nodes should have a parent graph!)
        inline bool HasParentGraph() const { return m_pParentGraph != nullptr; }

        // Get the graph that we belong to
        inline BaseGraph* GetParentGraph() { return m_pParentGraph; }

        // Get the graph that we belong to
        inline BaseGraph const* GetParentGraph() const { return m_pParentGraph; }

        // Does this node represent a graph? e.g. a state machine node
        inline bool HasChildGraph() const { return m_childGraph.IsSet(); }

        // Get the child graph for this node
        inline BaseGraph const* GetChildGraph() const { return m_childGraph.TryGetAs<BaseGraph>(); }

        // Get the child graph for this node
        inline BaseGraph* GetChildGraph() { return m_childGraph.TryGetAs<BaseGraph>(); }

        // Does this node have a graph that needs to be displayed in the secondary view? e.g. a condition graph for a selector node
        inline bool HasSecondaryGraph() const { return m_secondaryGraph.IsSet(); }

        // Get the secondary graph for this node
        inline BaseGraph* GetSecondaryGraph() { return m_secondaryGraph.TryGetAs<BaseGraph>(); }

        // Get the secondary graph for this node
        inline BaseGraph const* GetSecondaryGraph() const { return m_secondaryGraph.TryGetAs<BaseGraph>(); }

    protected:

        template<typename T, typename ... ConstructorParams>
        inline T* CreateChildGraph( ConstructorParams&&... params );

        template<typename T, typename ... ConstructorParams>
        inline T* CreateSecondaryGraph( ConstructorParams&&... params );

        // Post-deserialize fixup
        virtual void PostDeserialize() override;

        // Override this if you wish to provide a custom mechanism for determining unique node names
        virtual String CreateUniqueNodeName( String const& desiredName ) const;

        // Override this if you need to do some logic each frame before the node is drawn - use sparingly
        virtual void PreDrawUpdate( UserContext* pUserContext ) {}

        // Override this if you want to add extra controls to this node (the derived nodes will determine where this content is placed)
        virtual void DrawExtraControls( DrawContext const& ctx, UserContext* pUserContext ) {}

        // Called whenever we switch the view to this node
        virtual void OnShowNode() {}

        // Get the graph that we should navigate to when we go down a level (needed in some cases to skip a level)
        virtual BaseGraph* GetNavigationTarget();

        // Called before we copy this node
        virtual void PreCopy() {}

    protected:

        EE_REFLECT( ReadOnly );
        UUID                        m_ID;
        
        EE_REFLECT( ReadOnly );
        String                      m_name; // Optionally set name storage for renameable nodes

        EE_REFLECT( Hidden );
        Float2                      m_canvasPosition = Float2( 0, 0 ); // Updated each frame

        Float2                      m_size = Float2( 0, 0 ); // Updated each frame (its the actual rendered size in canvas units)
        Float2                      m_titleRectSize = Float2( 0, 0 ); // Updated each frame (is the actual rendered size in canvas units)

    private:

        EE_REFLECT( Hidden );
        TypeInstance                m_childGraph;

        EE_REFLECT( Hidden );
        TypeInstance                m_secondaryGraph;

        BaseGraph*                  m_pParentGraph = nullptr; // Not serialized - will be set post serialization

        mutable float               m_internalRegionStartY = -1.0f;
        mutable Color               m_internalRegionColor;
        mutable float               m_internalRegionMargins[2] = {0, 0};
        mutable bool                m_regionStarted = false;
        bool                        m_isHovered = false;
    };

    //-------------------------------------------------------------------------
    // Graph Base
    //-------------------------------------------------------------------------

    enum class SearchMode { Localized, Recursive };
    enum class SearchTypeMatch { Exact, Derived };

    struct DragAndDropState
    {
        bool TryAcceptDragAndDrop( char const* pPayloadID )
        {
            EE_ASSERT( pPayloadID != nullptr );
            if ( ImGuiPayload const* pPayload = ImGui::AcceptDragDropPayload( pPayloadID ) )
            {
                m_payloadID = pPayloadID;
                m_payloadData.resize( pPayload->DataSize );
                memcpy( m_payloadData.data(), pPayload->Data, pPayload->DataSize );
                return true;
            }

            return false;
        }

    public:

        ImVec2              m_mouseCanvasPos; // The mouse canvas position at which the drag and drop occurred
        ImVec2              m_mouseScreenPos; // The mouse screen position at which the drag and drop occurred
        String              m_payloadID;
        TVector<uint8_t>    m_payloadData;
    };

    class EE_ENGINETOOLS_API BaseGraph : public IReflectedType
    {
        friend BaseNode;
        friend class GraphView;
        friend class ScopedNodeModification;

    public:

        EE_REFLECT_TYPE( BaseGraph );

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
        BaseGraph( BaseNode* pParentNode );
        virtual ~BaseGraph();

        BaseGraph& operator=( BaseGraph const& rhs ) = default;

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

        // Get the special category ID for this graph, this is used to restrict nodes to specific types of graphs
        inline StringID GetCategory() const { return m_subType; }

        // Does this graph support adding comment nodes
        virtual bool SupportsComments() const { return true; }

        // Root Graph
        inline bool IsRootGraph() const { return !HasParentNode(); }

        // Get the root graph for this graph
        BaseGraph* GetRootGraph();

        // Returns the string path from the root graph
        String GetStringPathFromRoot() const;

        // Returns the path from the root graph in node IDs
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

        // Get read-only nodes - Note this will create a new array each time to prevent accidents with the type instances
        inline TInlineVector<BaseNode const*, 30> GetNodes() const 
        {
            TInlineVector<BaseNode const*, 30> nodes;
            nodes.reserve( m_nodes.size() );

            for ( TTypeInstance<BaseNode> const& nodeInstance : m_nodes )
            {
                nodes.emplace_back( nodeInstance.Get() );
            }

            return nodes;
        }

        // Returns the most significant node for a given graph, could be the final result node or the default state node, etc...
        virtual BaseNode const* GetMostSignificantNode() const { return nullptr; }

        // Destroys and deletes the specified node (UUID copy is intentional)
        void DestroyNode( UUID const& nodeID );

        // Can this graph contain a node of the specified type?
        virtual bool CanCreateNode( TypeSystem::TypeInfo const* pNodeTypeInfo ) const = 0;

        // Can this graph contain a node of the specified type?
        template<typename T>
        bool CanCreateNode() const
        {
            static_assert( std::is_base_of<BaseNode, T>::value );
            return CanCreateNode( T::s_pTypeInfo );
        }

        template<typename T, typename ... ConstructorParams>
        T* CreateNode( ConstructorParams&&... params )
        {
            EE_ASSERT( CanCreateNode( T::s_pTypeInfo ) );
            T* pNode = EE::New<T>( eastl::forward<ConstructorParams>( params )... );
            CreateNodeInternal( pNode, Float2::Zero );
            return pNode;
        }

        template<typename T, typename ... ConstructorParams>
        T* CreateNode( Float2 const& position, ConstructorParams&&... params )
        {
            EE_ASSERT( CanCreateNode( T::s_pTypeInfo ) );
            T* pNode = EE::New<T>( eastl::forward<ConstructorParams>( params )... );
            CreateNodeInternal( pNode, position );
            return pNode;
        }

        inline BaseNode* CreateNode( TypeSystem::TypeInfo const* pTypeInfo, Float2 const& position = Float2::Zero )
        {
            EE_ASSERT( CanCreateNode( pTypeInfo ) );
            BaseNode* pNode = Cast<BaseNode>( pTypeInfo->CreateType() );
            CreateNodeInternal( pNode, position );
            return pNode;
        }

        // Override this for additional control of what nodes can be deleted
        virtual bool CanDestroyNode( BaseNode const* pNode ) const { return true; }

        // Helpers
        //-------------------------------------------------------------------------

        // Called whenever we change the view to this graph
        void OnShowGraph();

        // Find a specific node in the graph
        inline BaseNode* FindNode( UUID const& nodeID, bool checkRecursively = false ) const
        {
            for ( TTypeInstance<BaseNode> const& nodeInstance : m_nodes )
            {
                EE_ASSERT( nodeInstance.IsSet() );

                BaseNode* pNode = const_cast<BaseNode*>( nodeInstance.Get() );

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

            for ( TTypeInstance<BaseNode>& nodeInstance : m_nodes )
            {
                if ( nodeInstance->HasChildGraph() )
                {
                    BaseGraph* pFoundGraph = nodeInstance->GetChildGraph()->FindPrimaryGraph( graphID );
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

        // Does a node with this name exist in this graph?
        bool IsUniqueNodeName( String const &name ) const;

        // Get a unique name for a renameable node within this graph
        String GetUniqueNameForRenameableNode( String const& desiredName, BaseNode const* m_pNodeToIgnore = nullptr ) const;

    protected:

        // Post-deserialize fixup
        virtual void PostDeserialize() override;

        // Get the graph that we should navigate to when we go up a level (needed in some cases to skip a level)
        virtual BaseGraph* GetNavigationTarget();

        // Called whenever we add a node to this graph
        virtual void OnNodeAdded( BaseNode* pAddedNode ) {}

        // Called whenever we modify a direct child node of this graph
        virtual void OnNodeModified( BaseNode* pModifiedNode ) {}

        // User this to draw any extra contextual information on the graph canvas
        virtual void DrawExtraInformation( DrawContext const& ctx, UserContext* pUserContext ) {}

        // Get the set of supported payloads for drag and drop
        virtual void GetSupportedDragAndDropPayloadIDs( TInlineVector<char const*, 10>& outIDs ) const { outIDs.clear(); }

        // Called whenever a drag and drop payload has been accepted - return true to complete the drag and drop action
        virtual bool HandleDragAndDrop( UserContext* pUserContext, DragAndDropState const& state ) { return true; }

        // Called after we finish pasting nodes (allows for child graphs to run further processes)
        virtual void PostPasteNodes( TInlineVector<BaseNode*, 20> const& pastedNodes ) {}

        // Called just before we destroy a node, allows derived graphs to handle the event
        virtual void PreDestroyNode( BaseNode* pNodeAboutToBeDestroyed ) {};

        // Called after we destroy a node, allows derived graphs to handle the event.
        virtual void PostDestroyNode( UUID const& nodeID ) {};

    private:

        void CreateNodeInternal( BaseNode* pCreatedNode, Float2 const& position );

    protected:

        EE_REFLECT( ReadOnly );
        UUID                                    m_ID;

        EE_REFLECT( Hidden );
        StringID                                m_subType;

        EE_REFLECT( Hidden );
        TVector<TTypeInstance<BaseNode>>        m_nodes;

    private:

        EE_REFLECT( Hidden );
        Float2                                  m_viewOffset = Float2( 0, 0 ); // Updated each frame

        EE_REFLECT( Hidden );
        float                                   m_viewScaleFactor = 1.0f; // View zoom level

        BaseNode*                               m_pParentNode = nullptr; // Not serialized, set after serialization

        int32_t                                 m_beginModificationCallCount = 0;
    };

    //-------------------------------------------------------------------------

    template<typename T, typename ... ConstructorParams>
    inline T* BaseNode::CreateChildGraph( ConstructorParams&&... params )
    {
        EE_ASSERT( !HasChildGraph() );
        T* pCreatedGraph = EE::New<T>( eastl::forward<ConstructorParams>( params )... );
        pCreatedGraph->m_ID = UUID::GenerateID();
        pCreatedGraph->m_pParentNode = this;
        m_childGraph.SetInstance( pCreatedGraph );
        return pCreatedGraph;
    }

    template<typename T, typename ... ConstructorParams>
    inline T* BaseNode::CreateSecondaryGraph( ConstructorParams&&... params )
    {
        EE_ASSERT( !HasSecondaryGraph() );
        T* pCreatedGraph = EE::New<T>( eastl::forward<ConstructorParams>( params )... );
        pCreatedGraph->m_ID = UUID::GenerateID();
        pCreatedGraph->m_pParentNode = this;
        m_secondaryGraph.SetInstance( pCreatedGraph );
        return pCreatedGraph;
    }

    //-------------------------------------------------------------------------
    // Undo/Redo
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