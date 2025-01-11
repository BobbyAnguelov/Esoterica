#pragma once

#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Definition.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationGraph.h"
#include "EngineTools/Animation/Shared/AnimationClipBrowser.h"
#include "EngineTools/Widgets/Pickers/ResourcePickers.h"
#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Widgets/TreeListView.h"
#include "EngineTools/NodeGraph/NodeGraph_View.h"
#include "EngineTools/Core/CategoryTree.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Base/Time/Timers.h"
#include "Base/Imgui/ImguiFilteredCombo.h"
#include "Base/Imgui/ImguiGizmo.h"

//-------------------------------------------------------------------------

namespace EE::Render { class SkeletalMeshComponent; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphComponent;
    class VariationHierarchy;
    struct GraphRecorder;

    class ParameterBaseToolsNode;
    class VirtualParameterToolsNode;
    class ControlParameterToolsNode;
    class ParameterReferenceToolsNode;
    class IDControlParameterToolsNode;
    class TargetControlParameterToolsNode;
    class ReferencedGraphToolsNode;
    class VariationDataToolsNode;

    //-------------------------------------------------------------------------

    class AnimationGraphEditor final : public DataFileEditor
    {
        EE_EDITOR_TOOL( AnimationGraphEditor );

        friend class BoneMaskIDEditor;
        friend class IDComboWidget;
        friend class IDEditor;

    private:

        struct LoadedGraphData
        {
            inline FlowGraph* GetRootGraph()
            {
                return m_pGraphDefinition->GetRootGraph();
            }

            inline ResourceID GetActiveVariationSkeleton() const
            {
                EE_ASSERT( m_activeVariationID.IsValid() );
                auto pVariation = m_pGraphDefinition->GetVariation( m_activeVariationID );
                EE_ASSERT( pVariation != nullptr );
                return pVariation->m_skeleton.GetResourceID();
            }

        public:

            ToolsGraphDefinition*                   m_pGraphDefinition = nullptr;
            GraphResourceDescriptor                 m_descriptor; // Needed for referenced graphs - this will be invalid for main graph
            StringID                                m_activeVariationID = Variation::s_defaultVariationID;
            NodeGraph::BaseNode*                    m_pParentNode = nullptr;

            //-------------------------------------------------------------------------

            GraphInstance*                          m_pGraphInstance = nullptr;
            THashMap<UUID, int16_t>                 m_nodeIDtoIndexMap;
            THashMap<int16_t, UUID>                 m_nodeIndexToIDMap;
        };

        //-------------------------------------------------------------------------

        struct RecordedGraphViewAndSelectionState
        {
            void Clear() { m_primaryViewSelectedNode = NodeGraph::SelectedNode(); m_selectedNodes.clear(); m_isSecondaryViewSelection = false; }

        public:

            TVector<UUID>                           m_viewedGraphPath;
            TVector<NodeGraph::SelectedNode>        m_selectedNodes;
            NodeGraph::SelectedNode                 m_primaryViewSelectedNode;
            bool                                    m_isSecondaryViewSelection = false;
        };

        //-------------------------------------------------------------------------

        class IDComboWidget : public ImGuiX::ComboWithFilterWidget<StringID>
        {

        public:

            IDComboWidget( AnimationGraphEditor* pGraphEditor );
            IDComboWidget( AnimationGraphEditor* pGraphEditor, IDControlParameterToolsNode* pControlParameter );

        private:

            virtual void PopulateOptionsList() override;

        private:

            AnimationGraphEditor*                       m_pGraphEditor = nullptr;
            IDControlParameterToolsNode*    m_pControlParameter = nullptr;
        };

        //-------------------------------------------------------------------------
        // Debug
        //-------------------------------------------------------------------------

        enum class DebugMode
        {
            None,
            Preview,
            ReviewRecording,
            LiveDebug,
        };

        enum class DebugTargetType
        {
            None,
            DirectPreview,
            MainGraph,
            ReferencedGraph,
            ExternalGraph,
            Recording
        };

        enum class ParameterType
        {
            Control,
            Virtual
        };

        struct DebugTarget
        {
            bool IsValid() const;
            void Clear() { m_type = DebugTargetType::None; m_pComponentToDebug = nullptr; m_referencedGraphID.Clear(); m_externalSlotID.Clear(); }

        public:

            DebugTargetType                     m_type = DebugTargetType::None;
            GraphComponent*                     m_pComponentToDebug = nullptr;
            PointerID                           m_referencedGraphID;
            StringID                            m_externalSlotID;
        };

        //-------------------------------------------------------------------------
        // Control Parameter Preview
        //-------------------------------------------------------------------------

        class ControlParameterPreviewState
        {
        public:

            ControlParameterPreviewState( AnimationGraphEditor* pGraphEditor, ControlParameterToolsNode* pParameter );
            virtual ~ControlParameterPreviewState() = default;

            inline ControlParameterToolsNode* GetParameter() const { return m_pParameter; }

            virtual void DrawPreviewEditor( UpdateContext const& context, Transform const& characterWorldTransform, bool isLiveDebug ) = 0;

        protected:

            EE_FORCE_INLINE GraphInstance* GetGraphInstance() const { return m_pGraphEditor->m_pDebugGraphInstance; }
            int16_t GetRuntimeGraphNodeIndex( UUID const& nodeID ) const;

        protected:

            AnimationGraphEditor*                       m_pGraphEditor = nullptr;
            ControlParameterToolsNode*      m_pParameter = nullptr;
        };

        class BoolParameterState;
        class IntParameterState;
        class FloatParameterState;
        class VectorParameterState;
        class IDParameterState;
        class TargetParameterState;

        //-------------------------------------------------------------------------
        // Navigation
        //-------------------------------------------------------------------------

        // Defines a navigation target for the navigation dialog
        struct NavigationTarget
        {
            enum class Type
            {
                Unknown = -1,
                Parameter = 0,
                Pose,
                Value,
                ID
            };

            static void CreateNavigationTargets( NodeGraph::BaseNode const* pNode, TVector<NavigationTarget>& outTargets );

        public:

            inline bool operator==( NavigationTarget const& rhs ) const
            {
                return m_compKey == rhs.m_compKey;
            }

        private:

            NavigationTarget() = default;

        public:

            NodeGraph::BaseNode const*        m_pNode = nullptr;
            String                              m_label;
            String                              m_path;
            String                              m_sortKey;
            StringID                            m_compKey;
            Type                                m_type = Type::Unknown;
        };

        //-------------------------------------------------------------------------

        // Event fired whenever we perform a graph edit
        static TEvent<ResourceID const&> s_graphModifiedEvent;

    public:

        AnimationGraphEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld );
        ~AnimationGraphEditor();

        virtual bool IsEditingFile( DataPath const& dataPath ) const override;

        inline StringID GetActiveVariation() const { return GetEditedGraphData()->m_activeVariationID; }

        // Navigation
        //-------------------------------------------------------------------------

        void NavigateTo( NodeGraph::BaseNode* pNode, bool focusViewOnNode = true );
        void NavigateTo( NodeGraph::BaseGraph* pGraph );

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual void WorldUpdate( EntityWorldUpdateContext const& updateContext ) override;

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_STATE_MACHINE; }
        virtual void DrawMenu( UpdateContext const& context ) override;
        virtual bool SupportsViewport() const override { return true; }
        virtual bool IsDataFileManualEditingAllowed() const override { return false; }
        virtual bool HasViewportToolbarTimeControls() const override { return true; }
        virtual void DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;
        virtual bool AlwaysAllowSaving() const override { return true; }

        // Dialogs
        //-------------------------------------------------------------------------

        void ShowNotifyDialog( String const& title, String const& message );
        void ShowNotifyDialog( String const& title, char const* pMessageFormat, ... );

        // Graph Operations
        //-------------------------------------------------------------------------

        inline LoadedGraphData* GetEditedGraphData() { return m_loadedGraphStack[0]; }
        inline LoadedGraphData const* GetEditedGraphData() const { return m_loadedGraphStack[0]; }
        inline FlowGraph* GetEditedRootGraph() { return ( m_loadedGraphStack[0]->m_pGraphDefinition != nullptr ) ? m_loadedGraphStack[0]->m_pGraphDefinition->GetRootGraph() : nullptr; }
        inline FlowGraph const* GetEditedRootGraph() const { return ( m_loadedGraphStack[0]->m_pGraphDefinition != nullptr ) ? m_loadedGraphStack[0]->m_pGraphDefinition->GetRootGraph() : nullptr; }

        void NodeDoubleClicked( NodeGraph::BaseNode* pNode );
        void GraphDoubleClicked( NodeGraph::BaseGraph* pGraph );
        void PostPasteNodes( TInlineVector<NodeGraph::BaseNode*, 20> const& pastedNodes );

        // Call this whenever any graph state is modified
        void OnGraphStateModified();

        // Commands
        //-------------------------------------------------------------------------

        void ProcessCustomCommandRequest( NodeGraph::CustomCommand const* pCommand );

        // Start a graph rename operation
        void StartRenameIDs();

        // Draw the rename dialog
        bool DrawRenameIDsDialog( UpdateContext const& context );

        // Copy all the found parameter names to the clipboard
        void CopyAllParameterNamesToClipboard();

        // Copy all the found graph IDs to the clipboard
        void CopyAllGraphIDsToClipboard();

        // Variations
        //-------------------------------------------------------------------------

        inline bool IsDefaultVariationSelected() const { return GetEditedGraphData()->m_activeVariationID == Variation::s_defaultVariationID; }

        inline StringID GetSelectedVariationID() const { return GetEditedGraphData()->m_activeVariationID; }

        // Sets the current selected variation. Assumes a valid variation ID!
        void SetActiveVariation( StringID variationID );

        // Tries to case-insensitively match a supplied variation name to the various variations we have
        bool TrySetActiveVariation( String const& variationName );

        // Selection
        //-------------------------------------------------------------------------

        void SetSelectedNodes( TVector<NodeGraph::SelectedNode> const& selectedNodes ) { m_selectedNodes = selectedNodes; }
        void ClearSelection() { m_selectedNodes.clear(); }

        // User Context
        //-------------------------------------------------------------------------

        void UpdateUserContext();

        // Graph View
        //-------------------------------------------------------------------------

        void DrawGraphView( UpdateContext const& context, bool isFocused );
        void DrawGraphViewNavigationBar();
        void UpdateSecondaryViewState();

        inline bool IsInReadOnlyState() const;

        inline bool IsViewingMainGraph() const { return m_loadedGraphStack.empty(); }
        
        inline ToolsGraphDefinition* GetCurrentlyViewedGraphDefinition() { return m_loadedGraphStack.back()->m_pGraphDefinition; }
        
        inline ToolsGraphDefinition const* GetCurrentlyViewedGraphDefinition() const { return m_loadedGraphStack.back()->m_pGraphDefinition; }

        // Get the stack index for the specified node!
        int32_t GetStackIndexForNode( NodeGraph::BaseNode* pNode ) const;

        // Get the stack index for the specified primary graph!
        int32_t GetStackIndexForGraph( NodeGraph::BaseGraph* pGraph ) const;

        // Create a new view stack based on a provided referenced-graph
        void PushOnGraphStack( NodeGraph::BaseNode* pSourceNode, ResourceID const& graphID );

        // Pop referenced graph view from stack
        void PopGraphStack();

        // Clear the entire view stack - except for the edited graph!
        void ClearGraphStack();

        // Generate the necessary debug data (node ID to idx mappings, graph instances, etc...) for the graph stack
        bool GenerateGraphStackDebugData();

        // Clear all graph stack debug data
        void ClearGraphStackDebugData();

        // Navigation
        //-------------------------------------------------------------------------

        void StartNavigationOperation();
        bool DrawNavigationDialogWindow( UpdateContext const& context );
        void GenerateNavigationTargetList();
        void GenerateActiveTargetList();

        // Graph Outliner
        //-------------------------------------------------------------------------

        void DrawOutliner( UpdateContext const& context, bool isFocused );
        void RefreshOutliner();
        void RebuildOutlinerTree( TreeListViewItem* pRootItem );

        // Property Grid
        //-------------------------------------------------------------------------

        void DrawNodeEditor( UpdateContext const& context, bool isFocused );

        // Clip Browser
        //-------------------------------------------------------------------------

        void DrawClipBrowser( UpdateContext const& context, bool isFocused );

        // Compilation Log
        //-------------------------------------------------------------------------

        void DrawGraphLog( UpdateContext const& context, bool isFocused );

        // Control Parameter Editor
        //-------------------------------------------------------------------------

        void DrawControlParameterEditor( UpdateContext const& context, bool isFocused );

        void CreateControlParameterPreviewStates();
        void DestroyControlParameterPreviewStates();

        void RefreshControlParameterCache();
        void RefreshParameterGroupTree();
        void DrawParameterList();
        void DrawParameterListRow( FlowToolsNode* pParameter );
        void DrawPreviewParameterList( UpdateContext const& context );

        bool DrawCreateOrRenameParameterDialogWindow( UpdateContext const& context, bool isRename );
        bool DrawDeleteParameterDialogWindow( UpdateContext const& context );
        bool DrawDeleteUnusedParameterDialogWindow( UpdateContext const& context );

        void ControlParameterGroupDragAndDropHandler( Category<ParameterBaseToolsNode*>& group );

        void StartParameterCreate( GraphValueType type, ParameterType parameterType );
        void StartParameterRename( UUID const& parameterID );
        void StartParameterDelete( UUID const& parameterID );

        ParameterBaseToolsNode* FindParameter( UUID const& parameterID ) const;

        void CreateParameter( ParameterType parameterType, GraphValueType valueType, String const& desiredParameterName, String const& desiredGroupName );
        void RenameParameter( UUID const& parameterID, String const& desiredParameterName, String const& desiredGroupName );
        void DestroyParameter( UUID const& parameterID );

        void EnsureUniqueParameterName( String& desiredParameterName ) const;
        void ResolveParameterGroupName( String& desiredGroupName ) const;

        // Variation Editor
        //-------------------------------------------------------------------------

        void DrawVariationEditor( UpdateContext const& context, bool isFocused );

        void StartCreateVariation( StringID variationID );
        void StartRenameVariation( StringID variationID );
        void StartDeleteVariation( StringID variationID );

        void CreateVariation( StringID newVariationID, StringID parentVariationID );
        void RenameVariation( StringID oldVariationID, StringID newVariationID );
        void DeleteVariation( StringID variationID );

        void DrawVariationSelector( float width = -1 );
        void DrawVariationTreeNode( VariationHierarchy const& variationHierarchy, StringID variationID );

        void RefreshVariationEditor();
        void SetupVariationEditorItemExtraColumnHeaders();
        void DrawVariationEditorItemExtraColumns( TreeListViewItem* pRootItem, int32_t columnIdx );
        void RebuildVariationDataTree( TreeListViewItem* pRootItem );

        bool DrawVariationNameEditor( bool& isDialogOpen, bool isRename );
        bool DrawCreateVariationDialogWindow( UpdateContext const& context );
        bool DrawRenameVariationDialogWindow( UpdateContext const& context );
        bool DrawDeleteVariationDialogWindow( UpdateContext const& context );

        // Debugging
        //-------------------------------------------------------------------------

        void DrawDebuggerWindow( UpdateContext const& context, bool isFocused );

        inline bool IsDebugging() const { return m_debugMode != DebugMode::None; }
        inline bool IsPreviewing() const { return m_debugMode == DebugMode::Preview; }
        inline bool IsLiveDebugging() const { return m_debugMode == DebugMode::LiveDebug; }
        inline bool IsReviewingRecording() const { return m_debugMode == DebugMode::ReviewRecording; }
        inline bool IsPreviewOrReview() const { return m_debugMode == DebugMode::Preview || m_debugMode == DebugMode::ReviewRecording; }

        virtual void OnResourceUnload( Resource::ResourcePtr* pResourcePtr ) override;
        virtual void OnDataFileLoadCompleted() override;
        virtual void OnDataFileUnload() override;

        // Request that we start a debug session, this may take several frames as we cannot start a session while hot-reloading, etc...
        void RequestDebugSession( UpdateContext const& context, DebugTarget target );

        // Starts a debugging session. If a target component is provided we assume we are attaching to a live game
        void StartDebugging( UpdateContext const& context );

        // Ends the current debug session
        void StopDebugging();

        // Set's the preview graph parameters to their default preview values
        void ReflectInitialPreviewParameterValues( UpdateContext const& context );

        // Draw all the debug options for this graph
        void DrawLiveDebugTargetsMenu( UpdateContext const& context );

        // Calculate the offset at which to place the camera when tracking
        void CalculateCameraOffset();

        // Recording
        //-------------------------------------------------------------------------

        // Draw all the recorder controls
        void DrawRecorderUI( UpdateContext const& context );

        // Clear recorded data
        void ClearRecordedData();

        // Start recorder the live preview state
        void StartRecording();

        // Stop recording the live preview state
        void StopRecording();

        // Set the currently reviewed frame, this is set on all clients in the review
        void SetFrameToReview( int32_t newFrameIdx );

        // Step the review forwards one frame
        void StepReviewForward();

        // Step the review backwards one frame
        void StepReviewBackward();

        // Undo/Redo/Reload
        //-------------------------------------------------------------------------

        void OnBeginGraphModification( NodeGraph::BaseGraph* pRootGraph );
        void OnEndGraphModification( NodeGraph::BaseGraph* pRootGraph );

        void PropertyGridPreEdit( PropertyEditInfo const& info );
        void PropertyGridPostEdit( PropertyEditInfo const& info );

        virtual void PreUndoRedo( UndoStack::Operation operation ) override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;

        void RecordViewAndSelectionState();
        void RestoreViewAndSelectionState();

    private:

        ResourceID const                                                    m_openedResourceID;

        AnimationClipBrowser                                                m_clipBrowser;
        ImGuiX::Gizmo                                                       m_gizmo;

        EventBindingID                                                      m_globalGraphEditEventBindingID;
        EventBindingID                                                      m_rootGraphBeginModificationBindingID;
        EventBindingID                                                      m_rootGraphEndModificationBindingID;

        // Node editing
        PropertyGrid                                                        m_nodeEditorPropertyGrid;
        EventBindingID                                                      m_nodeEditorPropertyGridPreEditEventBindingID;
        EventBindingID                                                      m_nodeEditorPropertyGridPostEditEventBindingID;
        PropertyGrid                                                        m_nodeVariationDataPropertyGrid;
        EventBindingID                                                      m_nodeVariationDataPropertyGridPreEditEventBindingID;
        EventBindingID                                                      m_nodeVariationDataPropertyGridPostEditEventBindingID;

        // Graph Type Data
        TVector<TypeSystem::TypeInfo const*>                                m_registeredNodeTypes;
        CategoryTree<TypeSystem::TypeInfo const*>                           m_categorizedNodeTypes;

        // Loaded Graph
        TVector<LoadedGraphData*>                                           m_loadedGraphStack;
        TVector<NodeGraph::SelectedNode>                                    m_selectedNodes;

        // View and selection state tracking
        RecordedGraphViewAndSelectionState                                  m_recordedViewAndSelectionState;

        // User Context
        ToolsGraphUserContext                                               m_userContext;
        EventBindingID                                                      m_navigateToNodeEventBindingID;
        EventBindingID                                                      m_navigateToGraphEventBindingID;
        EventBindingID                                                      m_resourceOpenRequestEventBindingID;
        EventBindingID                                                      m_nodeDoubleClickedEventBindingID;
        EventBindingID                                                      m_graphDoubleClickedEventBindingID;
        EventBindingID                                                      m_postPasteNodesEventBindingID;
        EventBindingID                                                      m_customCommandEventBindingID;

        // Graph view
        float                                                               m_primaryGraphViewProportionalHeight = 0.6f;
        NodeGraph::GraphView                                                m_primaryGraphView;
        NodeGraph::GraphView                                                m_secondaryGraphView;
        NodeGraph::GraphView*                                               m_pFocusedGraphView = nullptr;
        NodeGraph::BaseNode*                                                m_pBreadcrumbPopupContext = nullptr;

        // Navigation
        TVector<NavigationTarget>                                           m_navigationTargetNodes;
        TVector<NavigationTarget>                                           m_navigationActiveTargetNodes;
        ImGuiX::FilterWidget                                                m_navigationFilter;
        bool                                                                m_navigationDialogSearchesPaths = false;

        // Rename Dialog State
        IDComboWidget                                                       m_oldIDWidget;
        char                                                                m_oldIDBuffer[256] = { 0 };
        IDComboWidget                                                       m_newIDWidget;
        char                                                                m_newIDBuffer[256] = { 0 };

        // Compilation Log
        TVector<NodeCompilationLogEntry>                                    m_compilationLog;

        // Control Parameter Editor
        TInlineVector<ParameterBaseToolsNode*, 20>                          m_parameters;
        UUID                                                                m_currentOperationParameterID;
        ParameterType                                                       m_currentOperationParameterType;
        GraphValueType                                                      m_currentOperationParameterValueType = GraphValueType::Unknown;
        char                                                                m_parameterNameBuffer[255];
        char                                                                m_parameterGroupBuffer[255];
        THashMap<UUID, TVector<UUID>>                                       m_cachedUses;
        CategoryTree<ParameterBaseToolsNode*>                               m_parameterGroupTree;
        TVector<ControlParameterPreviewState*>                              m_previewParameterStates;
        CategoryTree<ControlParameterPreviewState*>                         m_previewParameterGroupTree;
        TargetControlParameterToolsNode*                                    m_pSelectedTargetControlParameter = nullptr;

        // Outliner
        ImGuiX::FilterWidget                                                m_outlinerFilterWidget;
        TreeListView                                                        m_outlinerTreeView;
        TreeListViewContext                                                 m_outlinerTreeContext;

        // Variation Editor
        StringID                                                            m_activeOperationVariationID;
        char                                                                m_nameBuffer[255] = { 0 };
        ResourcePicker                                                      m_variationSkeletonPicker;
        PropertyGrid                                                        m_variationDataPropertyGrid;
        EventBindingID                                                      m_variationDataPropertyGridPreEditEventBindingID;
        EventBindingID                                                      m_variationDataPropertyGridPostEditEventBindingID;
        UUID                                                                m_selectedVariationDataNode;
        ImGuiX::FilterWidget                                                m_variationEditorFilter;
        TreeListView                                                        m_variationTreeView;
        TreeListViewContext                                                 m_variationTreeContext;
        TVector<TPair<VariationDataToolsNode*, bool>>                       m_variationRequests;
        TVector<TypeSystem::TypeInfo const*>                                m_variationDataNodeTypes;
        TVector<TypeSystem::TypeInfo const*>                                m_selectedVariationDataNodeTypeFilters;

        // Preview/Debug
        DebugTarget                                                         m_requestedDebugTarget;
        DebugMode                                                           m_debugMode = DebugMode::None;
        EntityID                                                            m_debuggedEntityID; // This is needed to ensure that we dont try to debug a destroyed entity
        ComponentID                                                         m_debuggedComponentID;
        GraphComponent*                                                     m_pDebugGraphComponent = nullptr;
        Render::SkeletalMeshComponent*                                      m_pDebugMeshComponent = nullptr;
        GraphInstance*                                                      m_pDebugGraphInstance = nullptr;
        GraphInstance*                                                      m_pHostGraphInstance = nullptr;
        StringID                                                            m_debugExternalGraphSlotID = StringID();
        GraphDebugMode                                                      m_graphDebugMode = GraphDebugMode::On;
        RootMotionDebugMode                                                 m_rootMotionDebugMode = RootMotionDebugMode::Off;
        TaskSystemDebugMode                                                 m_taskSystemDebugMode = TaskSystemDebugMode::Off;
        bool                                                                m_showPreviewCapsule = false;
        float                                                               m_previewCapsuleHalfHeight = 0.65f;
        float                                                               m_previewCapsuleRadius = 0.35f;
        TResourcePtr<GraphDefinition>                                       m_previewGraphDefinitionPtr;
        Entity*                                                             m_pPreviewEntity = nullptr;
        Transform                                                           m_previewStartTransform = Transform::Identity;
        Transform                                                           m_characterTransform = Transform::Identity;
        Transform                                                           m_cameraOffsetTransform = Transform::Identity;
        Transform                                                           m_previousCameraTransform = Transform::Identity;
        SyncTrackTime                                                       m_previewStartSyncTime;
        bool                                                                m_startPaused = false;
        bool                                                                m_isFirstPreviewFrame = false;
        bool                                                                m_isCameraTrackingEnabled = true;
        bool                                                                m_initializeGraphToSpecifiedSyncTime = false;
        Skeleton::LOD                                                       m_skeletonLOD = Skeleton::LOD::High;

        // Recording
        GraphRecorder                                                       m_graphRecorder;
        int32_t                                                             m_currentReviewFrameIdx = InvalidIndex;
        bool                                                                m_isRecording = false;
        bool                                                                m_reviewStarted = false;
        bool                                                                m_drawExtraRecordingInfo = false;
    };
}