#pragma once

#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_Context.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_Outliner.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_EntityInspector.h"
#include "Engine/Imgui/ImguiGizmo.h"
#include "Engine/Entity/EntityDescriptors.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    struct ViewportResourceDropHandler;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API EntityCollectionEditor final : public EditorTool
    {
        EE_EDITOR_TOOL( EntityCollectionEditor );

    public:

        EntityCollectionEditor( ToolsContext const* pToolsContext, ResourceID const& collectionResourceID, EntityWorld* pWorld );

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;

    private:

        virtual void SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual bool ShouldLoadDefaultEditorMap() const override { return true; }
        virtual bool SupportsViewport() const override { return true; }
        virtual void ExtendViewportToolBar( UpdateContext const& context, Viewport* pViewport ) override;
        virtual void DrawMenu( UpdateContext const& context ) override;
        virtual void DrawViewportUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused ) override;
        virtual bool SupportsSaving() const override { return true; }
        virtual bool AlwaysAllowSaving() const override { return true; }
        virtual void HandleViewportInteractions() override;
        virtual void DropResourceInViewport( ResourceID const& resourceID, Vector const& worldPosition ) override;
        virtual bool IsEditingFile( DataPath const& dataPath ) const override { return m_collection.GetDataPath() == dataPath; }

        virtual String GetFilenameForSave() const override;
        virtual bool SaveData() override;

        virtual void PreUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;

    private:

        EditorContext                                   m_editorContext;
        TVector<ViewportResourceDropHandler const*>     m_pViewportDropHandlers;

        ImGuiX::Gizmo                                   m_gizmo;
        EntityOutliner                                  m_outliner;
        EntityInspector                                 m_inspector;
        TResourcePtr<EntityCollection>                  m_collection;
        bool                                            m_collectionInstantiated = false;
        bool                                            m_drawGrid = true;
    };
}