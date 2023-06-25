#pragma once

#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Definition.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Resource/ResourcePicker.h"
#include "EngineTools/Core/Workspace.h"
#include "EngineTools/Core/VisualGraph/VisualGraph_View.h"
#include "EngineTools/Core/CategoryTree.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "System/Time/Timers.h"
#include "System/Imgui/ImguiFilteredCombo.h"

//-------------------------------------------------------------------------

namespace EE::Render { class SkeletalMeshComponent; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphComponent;
    class VariationHierarchy;
    struct GraphRecorder;

    namespace GraphNodes { class VirtualParameterToolsNode; class ControlParameterToolsNode; }

    //-------------------------------------------------------------------------

    class AnimationGraphWorkspace final : public TWorkspace<GraphDefinition>
    {
        friend class GraphUndoableAction;
        friend class BoneMaskIDEditor;
        friend class IDComboWidget;
        friend class IDEditor;
    private:

        struct LoadedGraphData
        {
            inline FlowGraph* GetRootGraph()
            {
                return m_graphDefinition.GetRootGraph();
            }

            inline ResourceID const& GetSelectedVariationSkeleton() const
            {
                return m_graphDefinition.GetVariation( m_selectedVariationID )->m_skeleton.GetResourceID();
            }

        public:

            ToolsGraphDefinition                    m_graphDefinition;
            StringID                                m_selectedVariationID = Variation::s_defaultVariationID;
            VisualGraph::BaseNode*                  m_pParentNode = nullptr;

            //-------------------------------------------------------------------------

            GraphInstance*                          m_pGraphInstance = nullptr;
            THashMap<UUID, int16_t>                 m_nodeIDtoIndexMap;
            THashMap<int16_t, UUID>                 m_nodeIndexToIDMap;
        };

        //-------------------------------------------------------------------------

        class IDComboWidget : public ImGuiX::ComboWithFilterWidget<StringID>
        {

        public:

            IDComboWidget( AnimationGraphWorkspace* pGraphWorkspace );

        private:

            virtual void PopulateOptionsList() override;

        private:

            AnimationGraphWorkspace* m_pGraphWorkspace = nullptr;
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
            MainGraph,
            ChildGraph,
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

            DebugTargetType                     m_type = DebugTargetType::None;
            GraphComponent*                     m_pComponentToDebug = nullptr;
            PointerID                           m_childGraphID;
            StringID                            m_externalSlotID;
        };

        //-------------------------------------------------------------------------
        // Control Parameter Preview
        //-------------------------------------------------------------------------

        class ControlParameterPreviewState
        {
        public:

            ControlParameterPreviewState( AnimationGraphWorkspace* pGraphWorkspace, GraphNodes::ControlParameterToolsNode* pParameter );
            virtual ~ControlParameterPreviewState() = default;

            inline GraphNodes::ControlParameterToolsNode* GetParameter() const { return m_pParameter; }

            virtual void DrawPreviewEditor( UpdateContext const& context, Transform const& characterWorldTransform, bool isLiveDebug ) = 0;

        protected:

            EE_FORCE_INLINE GraphInstance* GetGraphInstance() const { return m_pGraphWorkspace->m_pDebugGraphInstance; }
            int16_t GetRuntimeGraphNodeIndex( UUID const& nodeID ) const;

        protected:

            AnimationGraphWorkspace*                    m_pGraphWorkspace = nullptr;
            GraphNodes::ControlParameterToolsNode*      m_pParameter = nullptr;
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

            static void CreateNavigationTargets( VisualGraph::BaseNode const* pNode, TVector<NavigationTarget>& outTargets );

        public:

            inline bool operator==( NavigationTarget const& rhs ) const
            {
                return m_compKey == rhs.m_compKey;
            }

        private:

            NavigationTarget() = default;

        public:

            VisualGraph::BaseNode const*        m_pNode = nullptr;
            String                              m_label;
            String                              m_path;
            String                              m_sortKey;
            StringID                            m_compKey;
            Type                                m_type = Type::Unknown;
        };

        //-------------------------------------------------------------------------
        // Graph Operations
        //-------------------------------------------------------------------------

        enum class GraphOperationType
        {
            None,
            NotifyUser,
            Navigate,
            RenameIDs,
            CreateParameter,
            RenameParameter,
            DeleteParameter,
            DeleteUnusedParameters,
            CreateVariation,
            RenameVariation,
            DeleteVariation
        };

        // Event fired whenever we perform a graph edit
        static TEvent<ResourceID const&> s_graphModifiedEvent;

    public:

        AnimationGraphWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );
        ~AnimationGraphWorkspace();

        virtual bool IsWorkingOnResource( ResourceID const& resourceID ) const override;

    private:

        virtual char const* GetWorkspaceUniqueTypeName() const override { return "Animation Graph"; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual void PreUpdateWorld( EntityWorldUpdateContext const& updateContext ) override;

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_STATE_MACHINE; }
        virtual void DrawMenu( UpdateContext const& context ) override;
        virtual bool HasViewportToolbarTimeControls() const override { return true; }
        virtual void DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void Update( UpdateContext const& context, bool isFocused ) override;
        virtual void PreUndoRedo( UndoStack::Operation operation ) override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;
        virtual bool AlwaysAllowSaving() const override { return true; }
        virtual bool Save() override;

        // Dialogs
        //-------------------------------------------------------------------------

        virtual void DrawDialogs( UpdateContext const& context ) override;
        void ShowNotifyDialog( String const& title, String const& message );
        void ShowNotifyDialog( String const& title, char const* pMessageFormat, ... );

        // Graph Operations
        //-------------------------------------------------------------------------

        inline LoadedGraphData* GetEditedGraphData() { return m_loadedGraphStack[0]; }
        inline LoadedGraphData const* GetEditedGraphData() const { return m_loadedGraphStack[0]; }
        inline FlowGraph* GetEditedRootGraph() { return m_loadedGraphStack[0]->m_graphDefinition.GetRootGraph(); }
        inline FlowGraph const* GetEditedRootGraph() const { return m_loadedGraphStack[0]->m_graphDefinition.GetRootGraph(); }

        void OnBeginGraphModification( VisualGraph::BaseGraph* pRootGraph );
        void OnEndGraphModification( VisualGraph::BaseGraph* pRootGraph );

        void GraphDoubleClicked( VisualGraph::BaseGraph* pGraph );
        void PostPasteNodes( TInlineVector<VisualGraph::BaseNode*, 20> const& pastedNodes );

        // Call this whenever any graph state is modified
        void OnGraphStateModified();

        // Commands
        //-------------------------------------------------------------------------

        void ProcessAdvancedCommandRequest( TSharedPtr<VisualGraph::AdvancedCommand> const& pCommand );

        // Start a graph rename operation
        void StartRenameIDs();

        // Draw the rename dialog
        void DrawRenameIDsDialog();

        // Copy all the found parameter names to the clipboard
        void CopyAllParameterNamesToClipboard();

        // Copy all the found graph IDs to the clipboard
        void CopyAllGraphIDsToClipboard();

        // Variations
        //-------------------------------------------------------------------------

        inline bool IsDefaultVariationSelected() const { return GetEditedGraphData()->m_selectedVariationID == Variation::s_defaultVariationID; }

        inline StringID GetSelectedVariationID() const { return GetEditedGraphData()->m_selectedVariationID; }

        // Sets the current selected variation. Assumes a valid variation ID!
        void SetSelectedVariation( StringID variationID );

        // Tries to case-insensitively match a supplied variation name to the various variations we have
        bool TrySetSelectedVariation( String const& variationName );

        // Selection
        //-------------------------------------------------------------------------

        void SetSelectedNodes( TVector<VisualGraph::SelectedNode> const& selectedNodes ) { m_selectedNodes = selectedNodes; }
        void ClearSelection() { m_selectedNodes.clear(); }

        // User Context
        //-------------------------------------------------------------------------

        void InitializeUserContext();
        void UpdateUserContext();
        void ShutdownUserContext();

        // Graph View
        //-------------------------------------------------------------------------

        void DrawGraphView( UpdateContext const& context, bool isFocused );
        void DrawGraphViewNavigationBar();
        void UpdateSecondaryViewState();

        inline bool IsInReadOnlyState() const;

        inline bool IsViewingMainGraph() const { return m_loadedGraphStack.empty(); }
        
        inline ToolsGraphDefinition* GetCurrentlyViewedGraphDefinition() { return m_loadedGraphStack.empty() ? &GetEditedGraphData()->m_graphDefinition : &m_loadedGraphStack.back()->m_graphDefinition; }
        inline ToolsGraphDefinition const* GetCurrentlyViewedGraphDefinition() const { return m_loadedGraphStack.empty() ? &GetEditedGraphData()->m_graphDefinition : &m_loadedGraphStack.back()->m_graphDefinition; }

        // Get the stack index for the specified node!
        int32_t GetStackIndexForNode( VisualGraph::BaseNode* pNode ) const;

        // Get the stack index for the specified primary graph!
        int32_t GetStackIndexForGraph( VisualGraph::BaseGraph* pGraph ) const;

        // Create a new view stack based on a provided child-graph
        void PushOnGraphStack( VisualGraph::BaseNode* pSourceNode, ResourceID const& graphID );

        // Pop child graph view from stack
        void PopGraphStack();

        // Clear the entire view stack - except for the edited graph!
        void ClearGraphStack();

        // Generate the necessary debug data for the graph stack
        bool GenerateGraphStackDebugData();

        // Clear all graph stack debug data
        void ClearGraphStackDebugData();

        // Navigation
        //-------------------------------------------------------------------------

        void NavigateTo( VisualGraph::BaseNode* pNode, bool focusViewOnNode = true );
        void NavigateTo( VisualGraph::BaseGraph* pGraph );
        void StartNavigationOperation();
        void DrawNavigationDialogWindow( UpdateContext const& context );
        void GenerateNavigationTargetList();
        void GenerateActiveTargetList();

        // Property Grid
        //-------------------------------------------------------------------------

        void DrawPropertyGrid( UpdateContext const& context, bool isFocused );

        void InitializePropertyGrid();
        void ShutdownPropertyGrid();

        // Compilation Log
        //-------------------------------------------------------------------------

        void DrawGraphLog( UpdateContext const& context, bool isFocused );

        // Control Parameter Editor
        //-------------------------------------------------------------------------

        void DrawControlParameterEditor( UpdateContext const& context, bool isFocused );

        void InitializeControlParameterEditor();
        void ShutdownControlParameterEditor();

        void CreateControlParameterPreviewStates();
        void DestroyControlParameterPreviewStates();

        void RefreshControlParameterCache();
        void RefreshParameterCategoryTree();
        void DrawParameterList();
        void DrawPreviewParameterList( UpdateContext const& context );

        void DrawCreateOrRenameParameterDialogWindow();
        void DrawDeleteParameterDialogWindow();
        void DrawDeleteUnusedParameterDialogWindow();

        void ControlParameterCategoryDragAndDropHandler( Category<GraphNodes::FlowToolsNode*>& category );

        void StartParameterCreate( GraphValueType type, ParameterType parameterType );
        void StartParameterRename( UUID const& parameterID );
        void StartParameterDelete( UUID const& parameterID );

        GraphNodes::ControlParameterToolsNode* FindControlParameter( UUID parameterID ) const;
        GraphNodes::VirtualParameterToolsNode* FindVirtualParameter( UUID parameterID ) const;

        void CreateParameter( ParameterType parameterType, GraphValueType valueType, String const& desiredParameterName, String const& desiredCategoryName );
        void RenameParameter( UUID parameterID, String const& desiredParameterName, String const& desiredCategoryName );
        void DestroyParameter( UUID parameterID );

        void EnsureUniqueParameterName( String& desiredParameterName ) const;
        void ResolveCategoryName( String& desiredCategoryName ) const;

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
        void DrawOverridesTable();

        void RefreshVariationEditor();
        void RefreshVariationSlotPickers();

        bool DrawVariationNameEditor();
        void DrawCreateVariationDialogWindow();
        void DrawRenameVariationDialogWindow();
        void DrawDeleteVariationDialogWindow();

        // Debugging
        //-------------------------------------------------------------------------

        void DrawDebuggerWindow( UpdateContext const& context, bool isFocused );

        inline bool IsDebugging() const { return m_debugMode != DebugMode::None; }
        inline bool IsPreviewing() const { return m_debugMode == DebugMode::Preview; }
        inline bool IsLiveDebugging() const { return m_debugMode == DebugMode::LiveDebug; }
        inline bool IsReviewingRecording() const { return m_debugMode == DebugMode::ReviewRecording; }
        inline bool IsPreviewOrReview() const { return m_debugMode == DebugMode::Preview || m_debugMode == DebugMode::ReviewRecording; }

        // Hot Reload
        virtual void OnHotReloadStarted( bool descriptorNeedsReload, TInlineVector<Resource::ResourcePtr*, 10> const& resourcesToBeReloaded ) override;

        // Starts a debugging session. If a target component is provided we assume we are attaching to a live game
        void StartDebugging( UpdateContext const& context, DebugTarget target );

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

    private:

        PropertyGrid                                                    m_propertyGrid;

        EventBindingID                                                  m_globalGraphEditEventBindingID;
        EventBindingID                                                  m_rootGraphBeginModificationBindingID;
        EventBindingID                                                  m_rootGraphEndModificationBindingID;
        EventBindingID                                                  m_preEditEventBindingID;
        EventBindingID                                                  m_postEditEventBindingID;

        // Graph Type Data
        TVector<TypeSystem::TypeInfo const*>                            m_registeredNodeTypes;
        CategoryTree<TypeSystem::TypeInfo const*>                       m_categorizedNodeTypes;

        // Loaded Graph
        FileSystem::Path                                                m_graphFilePath;
        TVector<LoadedGraphData*>                                       m_loadedGraphStack;
        TVector<VisualGraph::SelectedNode>                              m_selectedNodes;

        // Undo/Redo
        TVector<UUID>                                                   m_viewedGraphPathPreUndoRedo;
        TVector<VisualGraph::SelectedNode>                              m_selectedNodesPreUndoRedo;

        // User Context
        ToolsGraphUserContext                                           m_userContext;
        EventBindingID                                                  m_navigateToNodeEventBindingID;
        EventBindingID                                                  m_navigateToGraphEventBindingID;
        EventBindingID                                                  m_resourceOpenRequestEventBindingID;
        EventBindingID                                                  m_graphDoubleClickedEventBindingID;
        EventBindingID                                                  m_postPasteNodesEventBindingID;
        EventBindingID                                                  m_advancedCommandEventBindingID;

        // Operations/Dialogs
        GraphOperationType                                              m_activeOperation = GraphOperationType::None;
        String                                                          m_notifyDialogTitle;
        String                                                          m_notifyDialogText;

        // Graph view
        float                                                           m_primaryGraphViewProportionalHeight = 0.6f;
        VisualGraph::GraphView                                          m_primaryGraphView;
        VisualGraph::GraphView                                          m_secondaryGraphView;
        VisualGraph::GraphView*                                         m_pFocusedGraphView = nullptr;
        VisualGraph::BaseNode*                                          m_pBreadcrumbPopupContext = nullptr;

        // Navigation
        TVector<NavigationTarget>                                       m_navigationTargetNodes;
        TVector<NavigationTarget>                                       m_navigationActiveTargetNodes;
        ImGuiX::FilterWidget                                            m_navigationFilter;
        bool                                                            m_navigationDialogSearchesPaths = false;

        // Rename Dialog State
        IDComboWidget                                                   m_oldIDWidget;
        char                                                            m_oldIDBuffer[256] = { 0 };
        IDComboWidget                                                   m_newIDWidget;
        char                                                            m_newIDBuffer[256] = { 0 };

        // Compilation Log
        TVector<NodeCompilationLogEntry>                                m_compilationLog;

        // Control Parameter Editor
        TInlineVector<GraphNodes::ControlParameterToolsNode*, 20>       m_controlParameters;
        TInlineVector<GraphNodes::VirtualParameterToolsNode*, 20>       m_virtualParameters;
        UUID                                                            m_currentOperationParameterID;
        ParameterType                                                   m_currentOperationParameterType;
        GraphValueType                                                  m_currentOperationParameterValueType = GraphValueType::Unknown;
        char                                                            m_parameterNameBuffer[255];
        char                                                            m_parameterCategoryBuffer[255];
        THashMap<UUID, int32_t>                                         m_cachedNumUses;
        CategoryTree<GraphNodes::FlowToolsNode*>                        m_parameterCategoryTree;
        TVector<ControlParameterPreviewState*>                          m_previewParameterStates;
        CategoryTree<ControlParameterPreviewState*>                     m_previewParameterCategoryTree;

        // Variation Editor
        StringID                                                        m_activeOperationVariationID;
        char                                                            m_nameBuffer[255] = { 0 };
        ImGuiX::FilterWidget                                            m_variationFilter;
        Resource::ResourcePicker                                        m_variationSkeletonPicker;
        TVector<Resource::ResourcePicker>                               m_variationResourcePickers;
        bool                                                            m_refreshPickers = false;

        // Preview/Debug
        DebugMode                                                       m_debugMode = DebugMode::None;
        EntityID                                                        m_debuggedEntityID; // This is needed to ensure that we dont try to debug a destroyed entity
        ComponentID                                                     m_debuggedComponentID;
        GraphComponent*                                                 m_pDebugGraphComponent = nullptr;
        Render::SkeletalMeshComponent*                                  m_pDebugMeshComponent = nullptr;
        GraphInstance*                                                  m_pDebugGraphInstance = nullptr;
        StringID                                                        m_debugExternalGraphSlotID = StringID();
        GraphDebugMode                                                  m_graphDebugMode = GraphDebugMode::On;
        RootMotionDebugMode                                             m_rootMotionDebugMode = RootMotionDebugMode::Off;
        TaskSystemDebugMode                                             m_taskSystemDebugMode = TaskSystemDebugMode::Off;
        bool                                                            m_showPreviewCapsule = false;
        float                                                           m_previewCapsuleHalfHeight = 0.65f;
        float                                                           m_previewCapsuleRadius = 0.35f;
        TResourcePtr<GraphVariation>                                    m_previewGraphVariationPtr;
        Entity*                                                         m_pPreviewEntity = nullptr;
        Transform                                                       m_previewStartTransform = Transform::Identity;
        Transform                                                       m_characterTransform = Transform::Identity;
        Transform                                                       m_cameraOffsetTransform = Transform::Identity;
        Transform                                                       m_previousCameraTransform = Transform::Identity;
        SyncTrackTime                                                   m_previewStartSyncTime;
        bool                                                            m_startPaused = false;
        bool                                                            m_isFirstPreviewFrame = false;
        bool                                                            m_isCameraTrackingEnabled = true;
        bool                                                            m_initializeGraphToSpecifiedSyncTime = false;

        // Recording
        GraphRecorder                                                   m_graphRecorder;
        int32_t                                                         m_currentReviewFrameIdx = InvalidIndex;
        bool                                                            m_isRecording = false;
        bool                                                            m_reviewStarted = false;
        bool                                                            m_drawExtraRecordingInfo = false;
    };

    //-------------------------------------------------------------------------

    class GraphUndoableAction final : public IUndoableAction
    {
        EE_REFLECT_TYPE( IUndoableAction );

    public:

        GraphUndoableAction() = default;
        GraphUndoableAction( AnimationGraphWorkspace* pWorkspace );

        virtual void Undo() override;
        virtual void Redo() override;
        void SerializeBeforeState();
        void SerializeAfterState();

    private:

        AnimationGraphWorkspace*                                        m_pWorkspace = nullptr;
        String                                                          m_valueBefore;
        String                                                          m_valueAfter;
    };
}