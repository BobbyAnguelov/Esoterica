#pragma once

#include "EngineTools/Animation/GraphEditor/AnimationGraphEditor_Context.h"
#include "EngineTools/Animation/GraphEditor/AnimationGraphEditor_ControlParameterEditor.h"
#include "EngineTools/Animation/GraphEditor/AnimationGraphEditor_GraphEditor.h"
#include "EngineTools/Animation/GraphEditor/AnimationGraphEditor_VariationEditor.h"
#include "EngineTools/Animation/GraphEditor/AnimationGraphEditor_CompilationLog.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Common.h"
#include "EngineTools/Core/Workspaces/ResourceWorkspace.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::Render { class SkeletalMeshComponent; }
namespace EE::Physics { class PhysicsSystem; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class AnimationGraphComponent;
    class GraphUndoableAction;

    //-------------------------------------------------------------------------

    class AnimationGraphWorkspace final : public TResourceWorkspace<GraphDefinition>
    {
        friend GraphUndoableAction;

        enum class DebugMode
        {
            None,
            Preview,
            LiveDebug,
        };

    public:

        AnimationGraphWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );
        ~AnimationGraphWorkspace();

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void UpdateWorld( EntityWorldUpdateContext const& updateContext ) override;

        virtual bool HasViewportToolbarTimeControls() const override { return true; }
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void DrawWorkspaceToolbarItems( UpdateContext const& context ) override;
        virtual void UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;
        virtual void PreUndoRedo( UndoStack::Operation operation ) override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;
        virtual bool IsDirty() const override;
        virtual bool AlwaysAllowSaving() const override { return true; }
        virtual bool Save() override;

        // Debugging
        //-------------------------------------------------------------------------

        inline bool IsDebugging() const { return m_debugMode != DebugMode::None; }
        inline bool IsPreviewing() const { return m_debugMode == DebugMode::Preview; }
        inline bool IsLiveDebugging() const { return m_debugMode == DebugMode::LiveDebug; }

        // Starts a debugging session. If a target component is provided we assume we are attaching to a live game 
        void StartDebugging( UpdateContext const& context, AnimationGraphComponent* pTarget, StringID externalGraphSlotID = StringID() );

        // Ends the current debug session
        void StopDebugging();

        void ReflectInitialPreviewParameterValues( UpdateContext const& context );

        void DrawDebuggerWindow( UpdateContext const& context );
        void DrawLiveDebugTargetsMenu( UpdateContext const& context );

    private:

        String                              m_controlParametersWindowName;
        String                              m_graphViewWindowName;
        String                              m_propertyGridWindowName;
        String                              m_variationEditorWindowName;
        String                              m_compilationLogWindowName;
        String                              m_debuggerWindowName;

        GraphEditorContext                  m_editorContext;
        GraphControlParameterEditor         m_controlParameterEditor = GraphControlParameterEditor( m_editorContext );
        GraphVariationEditor                m_variationEditor = GraphVariationEditor( m_editorContext );
        GraphEditor                         m_graphEditor = GraphEditor( m_editorContext );
        GraphCompilationLog                 m_graphCompilationLog = GraphCompilationLog( m_editorContext );
        PropertyGrid                        m_propertyGrid;

        EventBindingID                      m_rootGraphBeginModificationBindingID;
        EventBindingID                      m_rootGraphEndModificationBindingID;
        EventBindingID                      m_preEditEventBindingID;
        EventBindingID                      m_postEditEventBindingID;
        EventBindingID                      m_navigateToNodeEventBindingID;
        EventBindingID                      m_navigateToGraphEventBindingID;

        GraphUndoableAction*                m_pActiveUndoableAction = nullptr;
        UUID                                m_selectedNodePreUndoRedo;

        FileSystem::Path                    m_graphFilePath;
        StringID                            m_selectedVariationID = GraphVariation::DefaultVariationID;

        Transform                           m_gizmoTransform;
        DebugMode                           m_debugMode = DebugMode::None;

        // Debug
        AnimationGraphComponent*            m_pDebugGraphComponent = nullptr;
        Render::SkeletalMeshComponent*      m_pDebugMeshComponent = nullptr;
        GraphInstance*                      m_pDebugGraphInstance = nullptr;
        StringID                            m_debugExternalGraphSlotID = StringID();
        GraphDebugMode                      m_graphDebugMode = GraphDebugMode::On;
        EditorGraphNodeContext              m_nodeContext;
        RootMotionDebugMode                 m_rootMotionDebugMode = RootMotionDebugMode::Off;
        TaskSystemDebugMode                 m_taskSystemDebugMode = TaskSystemDebugMode::Off;

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