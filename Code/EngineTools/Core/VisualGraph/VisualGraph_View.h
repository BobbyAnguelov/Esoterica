#pragma once
#include "VisualGraph_StateMachineGraph.h"
#include "VisualGraph_FlowGraph.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    // Helper to unsure we can maintain selection after a undo/redo
    struct SelectedNode
    {
        SelectedNode( BaseNode* pNode ) : m_nodeID( pNode->GetID() ), m_pNode( pNode ) {}
        bool operator==( SelectedNode const& rhs ) const { return m_nodeID == rhs.m_nodeID; }
        bool operator==( BaseNode const* pNode ) const { return m_nodeID == pNode->GetID(); }

        UUID            m_nodeID;
        BaseNode*       m_pNode = nullptr;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API GraphView
    {
    protected:

        constexpr static char const* const s_copiedNodesKey = "Copied Visual Graph Nodes";
        constexpr static char const* const s_copiedConnectionsKey = "Copied Visual Graph Connections";
        constexpr static char const* const s_dialogID_Rename = "Rename Dialog";

    protected:

        enum class DrawChannel
        {
            Background = 0,
            NodeBackground = 1,
            NodeForeground = 2,
            Connections = 3
        };

        enum class DragMode
        {
            None,
            View,
            Selection,
            Node,
            Connection,
        };

        // Drag state
        //-------------------------------------------------------------------------

        struct DragState
        {
            inline Flow::Node* GetAsFlowNode() const{ return Cast<Flow::Node> ( m_pNode ); }
            inline SM::Node* GetAsStateMachineNode() const{ return Cast<SM::Node> ( m_pNode ); }

            void Reset()
            {
                m_mode = DragMode::None;
                m_startValue = m_lastFrameDragDelta = ImVec2( 0, 0 );
                m_pNode = nullptr;
                m_pPin = nullptr;
            }

        public:

            DragMode                m_mode = DragMode::None;
            ImVec2                  m_startValue = ImVec2( 0, 0 );
            ImVec2                  m_lastFrameDragDelta = ImVec2( 0, 0 );
            BaseNode*               m_pNode = nullptr;
            Flow::Pin*              m_pPin = nullptr;
            bool                    m_leftMouseClickDetected = false;
            bool                    m_middleMouseClickDetected = false;
        };

        // Context menu state
        //-------------------------------------------------------------------------

        struct ContextMenuState
        {
            inline bool IsNodeContextMenu() const { return m_pNode != nullptr; }
            inline Flow::Node* GetAsFlowNode() const{ return Cast<Flow::Node>( m_pNode ); }
            inline SM::Node* GetAsStateMachineNode() const{ return Cast<SM::Node>( m_pNode ); }

            void Reset()
            {
                m_mouseCanvasPos = ImVec2();
                m_pNode = nullptr;
                m_menuOpened = false;
                m_pPin = nullptr;
            }

        public:

            ImVec2                  m_mouseCanvasPos;
            BaseNode*               m_pNode = nullptr;
            Flow::Pin*              m_pPin = nullptr;
            bool                    m_menuOpened = false;
        };

    public:

        bool HasFocus() const { return m_hasFocus; }

        //-------------------------------------------------------------------------

        void SetGraphToView( BaseGraph* pGraph, bool tryMaintainSelection = false );

        inline BaseGraph* GetViewedGraph() { return m_pGraph; };
        inline BaseGraph const* GetViewedGraph() const { return m_pGraph; }

        inline bool IsViewingFlowGraph() const { return m_pGraph != nullptr && IsOfType<FlowGraph>( m_pGraph ); }
        inline bool IsViewingStateMachineGraph() const { return m_pGraph != nullptr && IsOfType<StateMachineGraph>( m_pGraph ); }

        inline FlowGraph* GetFlowGraph() const { return Cast<FlowGraph>( m_pGraph ); }
        inline StateMachineGraph* GetStateMachineGraph() const { return Cast<StateMachineGraph>( m_pGraph ); }

        // Drawing and view
        //-------------------------------------------------------------------------

        void UpdateAndDraw( TypeSystem::TypeRegistry const& typeRegistry, float childHeightOverride = 0.0f, void* pUserContext = nullptr );

        void ResetView();

        void CenterView( BaseNode const* pNode );

        // Selection
        //-------------------------------------------------------------------------

        // This returns whether any selection changes occurred this update, will be cleared on each call to draw
        inline bool HasSelectionChangedThisFrame() const { return m_selectionChanged; }

        inline void SelectNode( BaseNode const* pNode );
        inline bool HasSelectedNodes() const { return !m_selectedNodes.empty(); }
        inline bool IsNodeSelected( BaseNode const* pNode ) const { return eastl::find( m_selectedNodes.begin(), m_selectedNodes.end(), pNode ) != m_selectedNodes.end(); }
        inline TVector<SelectedNode> const& GetSelectedNodes() const { return m_selectedNodes; }
        void ClearSelection();

    protected:

        void ResetInternalState();

        // Node
        //-------------------------------------------------------------------------

        inline ImRect GetNodeCanvasRect( BaseNode* pNode ) const
        {
            ImVec2 const nodeMargin = pNode->GetNodeMargin();
            ImVec2 const rectMin = ImVec2( pNode->GetCanvasPosition() ) - nodeMargin;
            ImVec2 const rectMax = ImVec2( pNode->GetCanvasPosition() ) + pNode->GetSize() + nodeMargin;
            return ImRect( rectMin, rectMax );
        }

        inline ImRect GetNodeWindowRect( BaseNode* pNode ) const
        {
            EE_ASSERT( m_pViewOffset != nullptr );
            ImVec2 const nodeMargin = pNode->GetNodeMargin();
            ImVec2 const rectMin = ImVec2( pNode->GetCanvasPosition() ) - nodeMargin - *m_pViewOffset;
            ImVec2 const rectMax = ImVec2( pNode->GetCanvasPosition() ) + pNode->GetSize() + nodeMargin - *m_pViewOffset;
            return ImRect( rectMin, rectMax );
        }

        void DestroySelectedNodes();

        // Visual
        //-------------------------------------------------------------------------

        bool BeginDrawCanvas( float childHeightOverride );
        void EndDrawCanvas();

        // User implementable function to draw any additional information needed in the graph (called after everything is drawn)
        virtual void DrawExtraInformation( DrawContext const& ctx ) {}
    
        // Dragging
        //-------------------------------------------------------------------------

        inline DragMode GetDragMode() const { return m_dragState.m_mode; }

        inline bool IsNotDragging() const { return GetDragMode() == DragMode::None; }
        inline bool IsDraggingView() const { return GetDragMode() == DragMode::View; }
        inline bool IsDraggingSelection() const { return GetDragMode() == DragMode::Selection; }
        inline bool IsDraggingNode() const { return GetDragMode() == DragMode::Node; }
        inline bool IsDraggingConnection() const { return GetDragMode() == DragMode::Connection; }

        virtual void StartDraggingView( DrawContext const& ctx );
        virtual void OnDragView( DrawContext const& ctx );
        virtual void StopDraggingView( DrawContext const& ctx );

        virtual void StartDraggingSelection( DrawContext const& ctx );
        virtual void OnDragSelection( DrawContext const& ctx );
        virtual void StopDraggingSelection( DrawContext const& ctx );

        virtual void StartDraggingNode( DrawContext const& ctx );
        virtual void OnDragNode( DrawContext const& ctx );
        virtual void StopDraggingNode( DrawContext const& ctx );

        virtual void StartDraggingConnection( DrawContext const& ctx );
        virtual void OnDragConnection( DrawContext const& ctx );
        virtual void StopDraggingConnection( DrawContext const& ctx );

        // Selection
        //-------------------------------------------------------------------------

        void UpdateSelection( BaseNode* pNewSelectedNode );
        void UpdateSelection( TVector<SelectedNode>&& newSelection );
        void AddToSelection( BaseNode* pNodeToAdd );
        void RemoveFromSelection( BaseNode* pNodeToRemove );

        // User implementable custom selection change handler
        virtual void OnSelectionChanged( TVector<SelectedNode> const& oldSelection, TVector<SelectedNode> const& newSelection ) {}

        // Context Menu
        //-------------------------------------------------------------------------

        inline bool IsContextMenuOpen() const { return m_contextMenuState.m_menuOpened; }

        void HandleContextMenu( DrawContext const& ctx );

        // Called when we actually draw the context menu, is expected to call the specific draw functions for graphs and nodes
        void DrawContextMenu( DrawContext const& ctx );

        // Override this to provide global context menu options for all graphs 
        // Use this only if you need custom data stored in the derived graph editor, else prefer the context menu function on the graphs themselves
        virtual void DrawExtraGraphContextMenuOptions( DrawContext const& ctx ) {}

        // Override this to provide global context menu options for all nodes
        // Use this only if you need custom data stored in the derived graph editor, else prefer the context menu function on the nodes themselves
        virtual void DrawExtraNodeContextMenuOptions( DrawContext const& ctx ) {}

        // Input Handling
        //-------------------------------------------------------------------------

        void HandleInput( TypeSystem::TypeRegistry const& typeRegistry, DrawContext const& ctx );

        virtual void DrawDialogs();
        virtual void OnGraphDoubleClick( DrawContext const& ctx, BaseGraph* pGraph ) {}
        virtual void OnNodeDoubleClick( DrawContext const& ctx, BaseNode* pNode ) {}
        virtual void HandleDragAndDrop( ImVec2 const& mouseCanvasPos ) {}

    private:

        EE_FORCE_INLINE void OnSelectionChangedInternal( TVector<SelectedNode> const& oldSelection, TVector<SelectedNode> const& newSelection )
        {
            m_selectionChanged = true;
            OnSelectionChanged( oldSelection, newSelection );
        }

        // Node Drawing
        //-------------------------------------------------------------------------

        void DrawStateMachineNodeTitle( DrawContext const& ctx, SM::Node* pNode, ImVec2& newNodeSize );
        void DrawStateMachineNodeBackground( DrawContext const& ctx, SM::Node* pNode, ImVec2& newNodeSize );
        void DrawStateMachineNode( DrawContext const& ctx, SM::Node* pNode );
        void DrawStateMachineTransitionConduit( DrawContext const& ctx, SM::TransitionConduit* pTransition );
        void DrawFlowNodeTitle( DrawContext const& ctx, Flow::Node* pNode, ImVec2& newNodeSize );
        void DrawFlowNodePins( DrawContext const& ctx, Flow::Node* pNode, ImVec2& newNodeSize );
        void DrawFlowNodeBackground( DrawContext const& ctx, Flow::Node* pNode, ImVec2& newNodeSize );
        void DrawFlowNode( DrawContext const& ctx, Flow::Node* pNode );

        // Copy/Paste
        //-------------------------------------------------------------------------

        void CopySelectedNodes( TypeSystem::TypeRegistry const& typeRegistry );
        void PasteNodes( TypeSystem::TypeRegistry const& typeRegistry, ImVec2 const& canvasPastePosition );

        // Node Ops
        //-------------------------------------------------------------------------

        void BeginRenameNode( BaseNode* pNode );
        void EndRenameNode( bool shouldUpdateName );

    protected:

        BaseGraph*                      m_pGraph = nullptr;
        BaseNode*                       m_pHoveredNode = nullptr;

        Float2*                         m_pViewOffset = nullptr;
        ImVec2                          m_canvasSize = ImVec2( 0, 0 );
        TVector<SelectedNode>           m_selectedNodes;
        bool                            m_hasFocus = false;
        bool                            m_isViewHovered = false;
        bool                            m_selectionChanged = false;

        DragState                       m_dragState;
        ContextMenuState                m_contextMenuState;

        // Flow graph state
        Flow::Pin*                      m_pHoveredPin = nullptr;
        UUID                            m_hoveredConnectionID;

        // Rename 
        char                            m_renameBuffer[255] = { 0 };
        BaseNode*                       m_pNodeBeingRenamed = nullptr;
    };
}