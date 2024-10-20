#pragma once
#include "NodeGraph_StateMachineGraph.h"
#include "NodeGraph_FlowGraph.h"
#include "NodeGraph_UserContext.h"

//-------------------------------------------------------------------------

namespace EE::NodeGraph
{
    class CommentNode;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API GraphView final
    {
    protected:

        constexpr static char const* const s_graphCopiedDataNodeName = "AnimGraphCopiedData";
        constexpr static char const* const s_copiedNodesNodeName = "Nodes";
        constexpr static char const* const s_copiedConnectionsNodeName = "Connections";
        constexpr static char const* const s_dialogID_Rename = "Rename ";
        constexpr static char const* const s_dialogID_Comment = "Comment";

    protected:

        enum class DragMode
        {
            None,
            View,
            Selection,
            Node,
            Connection,
            ResizeComment,
        };

        // Drag state
        //-------------------------------------------------------------------------

        struct DragState
        {
            inline FlowNode* GetAsFlowNode() const{ return Cast<FlowNode> ( m_pNode ); }
            inline FlowNode* GetAsStateMachineNode() const{ return Cast<FlowNode> ( m_pNode ); }
            inline void Reset() { *this = DragState(); }

        public:

            DragMode                        m_mode = DragMode::None;
            ImVec2                          m_startValue = ImVec2( 0, 0 );
            ImVec2                          m_lastFrameDragDelta = ImVec2( 0, 0 );
            BaseNode*                       m_pNode = nullptr;
            Pin*                            m_pPin = nullptr;
            TInlineVector<BaseNode*, 10>    m_draggedNodes;
            ResizeHandle                    m_resizeHandle = ResizeHandle::None;
            bool                            m_dragReadyToStart = false;
        };

        // Context menu state
        //-------------------------------------------------------------------------

        struct ContextMenuState
        {
            inline bool IsNodeContextMenu() const { return m_pNode != nullptr; }
            inline FlowNode* GetAsFlowNode() const{ return Cast<FlowNode>( m_pNode ); }
            inline StateMachineNode* GetAsStateMachineNode() const{ return Cast<StateMachineNode>( m_pNode ); }

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
            Pin*                    m_pPin = nullptr;
            bool                    m_requestOpenMenu = false;
            bool                    m_menuOpened = false;
            bool                    m_isAutoConnectMenu = false;
            bool                    m_isDragReady = false;
            ImGuiX::FilterWidget    m_filterWidget;
        };

        // Drag and Drop State
        //-------------------------------------------------------------------------

        struct DragAndDropState : public NodeGraph::DragAndDropState
        {
            void Reset()
            {
                m_mouseCanvasPos = m_mouseScreenPos = ImVec2( 0, 0 );
                m_payloadID.clear();
                m_payloadData.clear();
                m_isActiveDragAndDrop = false;
            }

            bool m_isActiveDragAndDrop = false;
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

        void SelectNode( BaseNode const* pNode );
        void SelectNodes( TVector<BaseNode const*> pNodes );
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
        void CreateCommentAroundSelectedNodes();

        // Visual
        //-------------------------------------------------------------------------

        bool BeginDrawCanvas( float childHeightOverride );
        void EndDrawCanvas( DrawContext const& ctx );
    
        // Dragging
        //-------------------------------------------------------------------------

        inline DragMode GetDragMode() const { return m_dragState.m_mode; }

        inline bool IsNotDragging() const { return GetDragMode() == DragMode::None; }

        inline bool IsDraggingView() const { return GetDragMode() == DragMode::View; }
        virtual void StartDraggingView( DrawContext const& ctx );
        virtual void OnDragView( DrawContext const& ctx );
        virtual void StopDraggingView( DrawContext const& ctx );

        inline bool IsDraggingSelection() const { return GetDragMode() == DragMode::Selection; }
        virtual void StartDraggingSelection( DrawContext const& ctx );
        virtual void OnDragSelection( DrawContext const& ctx );
        virtual void StopDraggingSelection( DrawContext const& ctx );

        inline bool IsDraggingNode() const { return GetDragMode() == DragMode::Node; }
        virtual void StartDraggingNode( DrawContext const& ctx );
        virtual void OnDragNode( DrawContext const& ctx );
        virtual void StopDraggingNode( DrawContext const& ctx );

        inline bool IsDraggingConnection() const { return GetDragMode() == DragMode::Connection; }
        virtual void StartDraggingConnection( DrawContext const& ctx );
        virtual void OnDragConnection( DrawContext const& ctx );
        virtual void StopDraggingConnection( DrawContext const& ctx );

        inline bool IsResizingCommentBox() const { return GetDragMode() == DragMode::ResizeComment; }
        virtual void StartResizingCommentBox( DrawContext const& ctx );
        virtual void OnResizeCommentBox( DrawContext const& ctx );
        virtual void StopResizingCommentBox( DrawContext const& ctx );

        // Selection
        //-------------------------------------------------------------------------

        void UpdateSelection( BaseNode* pNewSelectedNode );
        void UpdateSelection( TVector<SelectedNode> const& newSelection );
        void UpdateSelection( TVector<SelectedNode>&& newSelection );
        void AddToSelection( BaseNode* pNodeToAdd );
        void RemoveFromSelection( BaseNode* pNodeToRemove );

    private:

        void OnGraphModified( NodeGraph::BaseGraph* pGraph );

        // View
        //-------------------------------------------------------------------------

        inline float GetViewScaleFactor() const { return ( m_pGraph == nullptr ) ? 1.0f : m_pGraph->m_viewScaleFactor; }
        void ChangeViewScale( DrawContext const& ctx, float newViewScale );

        // Node Drawing
        //-------------------------------------------------------------------------

        void DrawStateMachineNode( DrawContext const& ctx, StateMachineNode* pNode );
        void DrawStateMachineTransitionConduit( DrawContext const& ctx, TransitionConduitNode* pTransition );
        void DrawFlowNode( DrawContext const& ctx, FlowNode* pNode );
        void DrawCommentNode( DrawContext const& ctx, CommentNode* pNode );

        // Node Ops
        //-------------------------------------------------------------------------

        void BeginRenameNode( BaseNode* pNode );
        void EndRenameNode( bool shouldUpdateName );

        // Input, Context Menu and Dialogs
        //-------------------------------------------------------------------------

        inline bool IsContextMenuOpen() const { return m_contextMenuState.m_menuOpened; }

        void HandleInput( TypeSystem::TypeRegistry const& typeRegistry, DrawContext const& ctx );

        void HandleContextMenu( DrawContext const& ctx );

        void DrawSharedContextMenuOptions( DrawContext const& ctx );

        void DrawCommentContextMenu( DrawContext const& ctx );

        void DrawFlowGraphContextMenu( DrawContext const& ctx );

        void DrawStateMachineContextMenu( DrawContext const& ctx );

        void DrawDialogs();

        // Connection Helpers
        //-------------------------------------------------------------------------

        void TryGetAutoConnectionNodeAndPin( DrawContext const& ctx, FlowNode*& pOutNode, Pin*& pOutPin ) const;

    protected:

        UserContext*                    m_pUserContext = nullptr;
        BaseGraph*                      m_pGraph = nullptr;
        BaseNode*                       m_pHoveredNode = nullptr;

        Float2                          m_defaultViewOffset = Float2::Zero;
        Float2*                         m_pViewOffset = &m_defaultViewOffset; // This will be set to the view offset of any view graph
        ImVec2                          m_canvasSize = ImVec2( 0, 0 );
        TVector<SelectedNode>           m_selectedNodes;
        bool                            m_hasFocus = false;
        bool                            m_isViewHovered = false;
        bool                            m_selectionChanged = false;
        bool                            m_isReadOnly = false;
        bool                            m_requestFocus = false;

        DragState                       m_dragState;
        ContextMenuState                m_contextMenuState;
        DragAndDropState                m_dragAndDropState;

        // Flow graph state
        Pin*                            m_pHoveredPin = nullptr;
        UUID                            m_hoveredConnectionID;

        // Rename / Comments
        char                            m_textBuffer[255] = { 0 };
        BaseNode*                       m_pNodeBeingOperatedOn = nullptr;

        // Event bindings
        EventBindingID                  m_graphEndModificationBindingID;
    };
}