#pragma once
#include "VisualGraph_StateMachineGraph.h"
#include "VisualGraph_FlowGraph.h"
#include "VisualGraph_UserContext.h"
#include "System/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    class EE_ENGINETOOLS_API GraphView final
    {
    protected:

        constexpr static char const* const s_copiedNodesKey = "Copied Visual Graph Nodes";
        constexpr static char const* const s_copiedConnectionsKey = "Copied Visual Graph Connections";
        constexpr static char const* const s_dialogID_Rename = "Rename Dialog";

    protected:

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
            bool                    m_primaryMouseClickDetected = false;
            bool                    m_secondaryMouseClickDetected = false;
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
                m_requestOpenMenu = false;
                m_isAutoConnectMenu = false;
                m_filterWidget.Clear();
            }

        public:

            ImVec2                  m_mouseCanvasPos;
            BaseNode*               m_pNode = nullptr;
            Flow::Pin*              m_pPin = nullptr;
            bool                    m_requestOpenMenu = false;
            bool                    m_menuOpened = false;
            bool                    m_isAutoConnectMenu = false;
            ImGuiX::FilterWidget    m_filterWidget;
        };

    public:

        GraphView( UserContext* pUserContext );
        virtual ~GraphView();

        bool HasFocus() const { return m_hasFocus; }

        //-------------------------------------------------------------------------

        void SetGraphToView( BaseGraph* pGraph, bool tryMaintainSelection = false );

        inline BaseGraph* GetViewedGraph() { return m_pGraph; };
        inline BaseGraph const* GetViewedGraph() const { return m_pGraph; }

        inline bool IsViewingFlowGraph() const { return m_pGraph != nullptr && IsOfType<FlowGraph>( m_pGraph ); }
        inline bool IsViewingStateMachineGraph() const { return m_pGraph != nullptr && IsOfType<StateMachineGraph>( m_pGraph ); }

        inline FlowGraph* GetFlowGraph() const { return Cast<FlowGraph>( m_pGraph ); }
        inline StateMachineGraph* GetStateMachineGraph() const { return Cast<StateMachineGraph>( m_pGraph ); }

        inline bool IsReadOnly() const { return m_isReadOnly; }

        // Set the view to be read only - no graph modification allowed
        void SetReadOnly( bool isReadOnly ) { m_isReadOnly = isReadOnly; }

        // Drawing and view
        //-------------------------------------------------------------------------

        void UpdateAndDraw( TypeSystem::TypeRegistry const& typeRegistry, float childHeightOverride = 0.0f );

        void ResetView();

        void CenterView( BaseNode const* pNode );

        void RefreshNodeSizes();

        // Selection
        //-------------------------------------------------------------------------

        // This returns whether any selection changes occurred this update, will be cleared on each call to draw
        inline bool HasSelectionChangedThisFrame() const { return m_selectionChanged; }

        inline void SelectNode( BaseNode const* pNode );
        inline bool HasSelectedNodes() const { return !m_selectedNodes.empty(); }
        inline bool IsNodeSelected( BaseNode const* pNode ) const { return eastl::find( m_selectedNodes.begin(), m_selectedNodes.end(), pNode ) != m_selectedNodes.end(); }
        inline TVector<SelectedNode> const& GetSelectedNodes() const { return m_selectedNodes; }
        void ClearSelection();

        // Copy/Paste
        //-------------------------------------------------------------------------

        void CopySelectedNodes( TypeSystem::TypeRegistry const& typeRegistry );
        void PasteNodes( TypeSystem::TypeRegistry const& typeRegistry, ImVec2 const& canvasPastePosition );

    protected:

        void ResetInternalState();

        // Node
        //-------------------------------------------------------------------------

        void DestroySelectedNodes();

        // Visual
        //-------------------------------------------------------------------------

        bool BeginDrawCanvas( float childHeightOverride );
        void EndDrawCanvas();
    
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
        void UpdateSelection( TVector<SelectedNode> const& newSelection );
        void UpdateSelection( TVector<SelectedNode>&& newSelection );
        void AddToSelection( BaseNode* pNodeToAdd );
        void RemoveFromSelection( BaseNode* pNodeToRemove );

    private:

        void OnGraphModified( VisualGraph::BaseGraph* pGraph );

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

        // Node Ops
        //-------------------------------------------------------------------------

        void BeginRenameNode( BaseNode* pNode );
        void EndRenameNode( bool shouldUpdateName );

        // Input, Context Menu and Dialogs
        //-------------------------------------------------------------------------

        inline bool IsContextMenuOpen() const { return m_contextMenuState.m_menuOpened; }

        void HandleInput( TypeSystem::TypeRegistry const& typeRegistry, DrawContext const& ctx );

        void HandleContextMenu( DrawContext const& ctx );

        void DrawFlowGraphContextMenu( DrawContext const& ctx );

        void DrawStateMachineContextMenu( DrawContext const& ctx );

        void DrawDialogs();

    protected:

        UserContext*                    m_pUserContext = nullptr;
        BaseGraph*                      m_pGraph = nullptr;
        BaseNode*                       m_pHoveredNode = nullptr;

        Float2*                         m_pViewOffset = nullptr;
        ImVec2                          m_canvasSize = ImVec2( 0, 0 );
        TVector<SelectedNode>           m_selectedNodes;
        bool                            m_hasFocus = false;
        bool                            m_isViewHovered = false;
        bool                            m_selectionChanged = false;
        bool                            m_isReadOnly = false;

        DragState                       m_dragState;
        ContextMenuState                m_contextMenuState;

        // Flow graph state
        Flow::Pin*                      m_pHoveredPin = nullptr;
        UUID                            m_hoveredConnectionID;

        // Rename 
        char                            m_renameBuffer[255] = { 0 };
        BaseNode*                       m_pNodeBeingRenamed = nullptr;

        // Event bindings
        EventBindingID                  m_graphEndModificationBindingID;
    };
}