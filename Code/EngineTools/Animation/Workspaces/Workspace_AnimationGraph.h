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

namespace EE::Physics
{
    class PhysicsSystem;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class AnimationGraphComponent;
    class GraphUndoableAction;

    //-------------------------------------------------------------------------

    class AnimationGraphWorkspace final : public TResourceWorkspace<GraphDefinition>
    {
        friend GraphUndoableAction;

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

        void GenerateAnimGraphVariationDescriptors();

        // Preview
        //-------------------------------------------------------------------------

        inline bool IsPreviewing() const { return m_isPreviewing; }
        void StartPreview( UpdateContext const& context );
        void StopPreview();

        void SetFirstFrameParameterValues( UpdateContext const& context );
        void DrawDebuggerWindow( UpdateContext const& context );

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

        // Preview
        Physics::PhysicsSystem*             m_pPhysicsSystem = nullptr;
        Entity*                             m_pPreviewEntity = nullptr;
        AnimationGraphComponent*            m_pGraphComponent = nullptr;
        GraphNodeContext                    m_nodeContext;
        Transform                           m_previewStartTransform = Transform::Identity;
        bool                                m_startPaused = false;
        bool                                m_isPreviewing = false;
        GraphDebugMode                      m_graphDebugMode = GraphDebugMode::On;
        RootMotionRecorderDebugMode         m_rootMotionDebugMode = RootMotionRecorderDebugMode::Off;
        TaskSystemDebugMode                 m_taskSystemDebugMode = TaskSystemDebugMode::Off;
        bool                                m_isFirstPreviewFrame = false;
    };
}