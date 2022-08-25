#pragma once

#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Definition.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Core/Workspace.h"
#include "EngineTools/Core/VisualGraph/VisualGraph_View.h"
#include "EngineTools/Core/Helpers/CategoryTree.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "System/Time/Timers.h"

//-------------------------------------------------------------------------

namespace EE::Render { class SkeletalMeshComponent; }
namespace EE::Physics { class PhysicsSystem; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class AnimationGraphComponent;
    class GraphUndoableAction;
    class ControlParameterPreviewState;
    class VariationHierarchy;

    namespace GraphNodes { class VirtualParameterToolsNode; class ControlParameterToolsNode; }

    //-------------------------------------------------------------------------

    class AnimationGraphWorkspace final : public TWorkspace<GraphDefinition>
    {
        friend GraphUndoableAction;

        enum class DebugMode
        {
            None,
            Preview,
            LiveDebug,
        };

        enum class DebugTargetType
        {
            None,
            MainGraph,
            ChildGraph,
            ExternalGraph
        };

        struct DebugTarget
        {
            bool IsValid() const;

            DebugTargetType                 m_type = DebugTargetType::None;
            AnimationGraphComponent*        m_pComponentToDebug = nullptr;
            PointerID                       m_childGraphID;
            StringID                        m_externalSlotID;
        };

        enum class GraphOperationType
        {
            None,
            RenameParameter,
            DeleteParameter,
            CreateVariation,
            RenameVariation,
            DeleteVariation
        };

    public:

        AnimationGraphWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );
        ~AnimationGraphWorkspace();

        virtual bool IsEditingResource( ResourceID const& resourceID ) const override;

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void PreUpdateWorld( EntityWorldUpdateContext const& updateContext ) override;

        virtual bool HasViewportToolbarTimeControls() const override { return true; }
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void DrawWorkspaceToolbarItems( UpdateContext const& context ) override;
        virtual void Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;
        virtual void PreUndoRedo( UndoStack::Operation operation ) override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;
        virtual bool IsDirty() const override;
        virtual bool AlwaysAllowSaving() const override { return true; }
        virtual bool Save() override;

        virtual void DrawDialogs( UpdateContext const& context );

        // Graph Operations
        //-------------------------------------------------------------------------

        void OnBeginGraphModification( VisualGraph::BaseGraph* pRootGraph );
        void OnEndGraphModification( VisualGraph::BaseGraph* pRootGraph );

        // Variations
        //-------------------------------------------------------------------------

        inline bool IsDefaultVariationSelected() const { return m_selectedVariationID == Variation::s_defaultVariationID; }

        inline StringID GetSelectedVariationID() const { return m_selectedVariationID; }

        // Sets the current selected variation. Assumes a valid variation ID!
        void SetSelectedVariation( StringID variationID );

        // Tries to case-insensitively match a supplied variation name to the various variations we have
        void TrySetSelectedVariation( String const& variationName );

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

        void NavigateTo( VisualGraph::BaseNode* pNode, bool focusViewOnNode = true );
        void NavigateTo( VisualGraph::BaseGraph* pGraph );
        void DrawGraphView( UpdateContext const& context, ImGuiWindowClass* pWindowClass );
        void UpdateSecondaryViewState();

        // Property Grid
        //-------------------------------------------------------------------------

        void DrawPropertyGrid( UpdateContext const& context, ImGuiWindowClass* pWindowClass );

        void InitializePropertyGrid();
        void ShutdownPropertyGrid();

        // Compilation Log
        //-------------------------------------------------------------------------

        void DrawCompilationLog( UpdateContext const& context, ImGuiWindowClass* pWindowClass );

        // Control Parameter Editor
        //-------------------------------------------------------------------------

        bool DrawControlParameterEditor( UpdateContext const& context, ImGuiWindowClass* pWindowClass );

        void InitializeControlParameterEditor();
        void ShutdownControlParameterEditor();

        void CreateControlParameterPreviewStates();
        void DestroyControlParameterPreviewStates();

        void RefreshControlParameterCache();
        void DrawParameterList();
        void DrawParameterPreviewControls();
        void DrawAddParameterCombo();

        void DrawDeleteParameterDialogWindow();
        void DrawRenameParameterDialogWindow();

        void StartParameterRename( UUID const& parameterID );
        void StartParameterDelete( UUID const& parameterID );

        GraphNodes::ControlParameterToolsNode* FindControlParameter( UUID parameterID ) const;
        GraphNodes::VirtualParameterToolsNode* FindVirtualParameter( UUID parameterID ) const;

        void CreateControlParameter( GraphValueType type );
        void CreateVirtualParameter( GraphValueType type );

        void RenameControlParameter( UUID parameterID, String const& newName, String const& category );
        void RenameVirtualParameter( UUID parameterID, String const& newName, String const& category );

        void DestroyControlParameter( UUID parameterID );
        void DestroyVirtualParameter( UUID parameterID );

        void EnsureUniqueParameterName( String& parameterName ) const;

        // Variation Editor
        //-------------------------------------------------------------------------

        void DrawVariationEditor( UpdateContext const& context, ImGuiWindowClass* pWindowClass );

        void StartCreate( StringID variationID );
        void StartRename( StringID variationID );
        void StartDelete( StringID variationID );

        void CreateVariation( StringID newVariationID, StringID parentVariationID );
        void RenameVariation( StringID oldVariationID, StringID newVariationID );
        void DeleteVariation( StringID variationID );

        void DrawVariationSelector();
        void DrawVariationTreeNode( VariationHierarchy const& variationHierarchy, StringID variationID );
        void DrawOverridesTable();

        bool DrawVariationNameEditor();
        void DrawCreateVariationDialogWindow();
        void DrawRenameVariationDialogWindow();
        void DrawDeleteVariationDialogWindow();

        // Debugging
        //-------------------------------------------------------------------------

        void DrawDebuggerWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass );

        inline bool IsDebugging() const { return m_debugMode != DebugMode::None; }
        inline bool IsPreviewing() const { return m_debugMode == DebugMode::Preview; }
        inline bool IsLiveDebugging() const { return m_debugMode == DebugMode::LiveDebug; }

        // Starts a debugging session. If a target component is provided we assume we are attaching to a live game 
        void StartDebugging( UpdateContext const& context, DebugTarget target );

        // Ends the current debug session
        void StopDebugging();

        // 
        void ReflectInitialPreviewParameterValues( UpdateContext const& context );

        void DrawLiveDebugTargetsMenu( UpdateContext const& context );

        // Calculate the offset at which to place the camera when tracking
        void CalculateCameraOffset();

    private:

        String                              m_controlParametersWindowName;
        String                              m_graphViewWindowName;
        String                              m_propertyGridWindowName;
        String                              m_variationEditorWindowName;
        String                              m_compilationLogWindowName;
        String                              m_debuggerWindowName;
        PropertyGrid                        m_propertyGrid;
        Transform                           m_gizmoTransform;
        GraphUndoableAction*                m_pActiveUndoableAction = nullptr;
        GraphOperationType                  m_activeOperation = GraphOperationType::None;

        EventBindingID                      m_rootGraphBeginModificationBindingID;
        EventBindingID                      m_rootGraphEndModificationBindingID;
        EventBindingID                      m_preEditEventBindingID;
        EventBindingID                      m_postEditEventBindingID;
        

        // Graph Type Data
        TVector<TypeSystem::TypeInfo const*>                            m_registeredNodeTypes;
        CategoryTree<TypeSystem::TypeInfo const*>                       m_categorizedNodeTypes;

        // Graph Data
        FileSystem::Path                    m_graphFilePath;
        ToolsGraphDefinition                m_toolsGraph;
        TVector<VisualGraph::SelectedNode>  m_selectedNodes;
        TVector<VisualGraph::SelectedNode>  m_selectedNodesPreUndoRedo;
        StringID                            m_selectedVariationID = Variation::s_defaultVariationID; // NOTE: Do not set this directly!!! Use the provided functions

        // User Context
        ToolsGraphUserContext               m_userContext;
        EventBindingID                      m_navigateToNodeEventBindingID;
        EventBindingID                      m_navigateToGraphEventBindingID;
        EventBindingID                      m_resourceOpenRequestEventBindingID;

        // Graph view
        float                               m_primaryGraphViewHeight = 300;
        VisualGraph::GraphView              m_primaryGraphView;
        VisualGraph::GraphView              m_secondaryGraphView;
        VisualGraph::GraphView*             m_pFocusedGraphView = nullptr;
        UUID                                m_primaryViewGraphID;

        // Debug
        DebugMode                           m_debugMode = DebugMode::None;
        EntityID                            m_debuggedEntityID; // This is needed to ensure that we dont try to debug a destroyed entity
        ComponentID                         m_debuggedComponentID;
        AnimationGraphComponent*            m_pDebugGraphComponent = nullptr;
        Render::SkeletalMeshComponent*      m_pDebugMeshComponent = nullptr;
        GraphInstance*                      m_pDebugGraphInstance = nullptr;
        StringID                            m_debugExternalGraphSlotID = StringID();
        GraphDebugMode                      m_graphDebugMode = GraphDebugMode::On;
        RootMotionDebugMode                 m_rootMotionDebugMode = RootMotionDebugMode::Off;
        TaskSystemDebugMode                 m_taskSystemDebugMode = TaskSystemDebugMode::Off;

        // Compilation Log
        TVector<NodeCompilationLogEntry>    m_compilationLog;

        // Control Parameter Editor
        TInlineVector<GraphNodes::ControlParameterToolsNode*, 20>       m_controlParameters;
        TInlineVector<GraphNodes::VirtualParameterToolsNode*, 20>       m_virtualParameters;
        GraphNodes::VirtualParameterToolsNode*                          m_pVirtualParamaterToEdit = nullptr;
        UUID                                                            m_currentOperationParameterID;
        char                                                            m_parameterNameBuffer[255];
        char                                                            m_parameterCategoryBuffer[255];
        TVector<ControlParameterPreviewState*>                          m_parameterPreviewStates;
        THashMap<UUID, int32_t>                                         m_cachedNumUses;
        CountdownTimer<EngineClock>                                     m_updateCacheTimer;

        // Variation Editor
        StringID                            m_activeOperationVariationID;
        char                                m_buffer[255] = { 0 };
        Resource::ResourceFilePicker        m_resourcePicker;

        // Preview
        Physics::PhysicsSystem*             m_pPhysicsSystem = nullptr;
        Entity*                             m_pPreviewEntity = nullptr;
        Transform                           m_previewStartTransform = Transform::Identity;
        Transform                           m_characterTransform = Transform::Identity;
        Transform                           m_cameraOffsetTransform = Transform::Identity;
        Transform                           m_previousCameraTransform = Transform::Identity;
        bool                                m_startPaused = false;
        bool                                m_isFirstPreviewFrame = false;
        bool                                m_isCameraTrackingEnabled = true;
    };
}