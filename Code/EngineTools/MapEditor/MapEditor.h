#pragma once

#include "Engine/Entity/EntityDescriptors.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_Context.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_Outliner.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_EntityInspector.h"
#include "EngineTools/Core/EditorTool.h"
#include "Engine/Imgui/ImguiGizmo.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    struct ViewportResourceDropHandler;
    class MapEditorMode;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API MapEditor final : public EditorTool
    {
        EE_SINGLETON_EDITOR_TOOL( MapEditor );

    public:

        MapEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld );
        ~MapEditor();

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;

        // Map
        //-------------------------------------------------------------------------

        inline bool HasLoadedMap() const { return m_loadedMap.IsValid(); }
        inline ResourceID GetLoadedMap() const { return m_loadedMap; }
        void LoadMap( ResourceID const& mapToLoad );

        // Game Preview
        //-------------------------------------------------------------------------

        TEventHandle<UpdateContext const&> OnGamePreviewStartRequested() { return m_requestStartGamePreview; }
        TEventHandle<UpdateContext const&> OnGamePreviewStopRequested() { return m_requestStopGamePreview; }

        void NotifyGamePreviewStarted();
        void NotifyGamePreviewEnded();

    private:

        virtual void SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;

        virtual bool SupportsNewFileCreation() const override { return true; }
        virtual bool IsEditingFile( DataPath const& dataPath ) const override { return m_loadedMap.GetDataPath() == dataPath; }
        virtual void CreateNewFile() const override;
        virtual String GetFilenameForSave() const override;

        virtual bool SaveData() override;
        virtual bool SupportsSaving() const override { return true; }
        virtual bool AlwaysAllowSaving() const override { return true; }

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_EARTH; }
        virtual bool ShouldLoadDefaultEditorMap() const override { return false; }
        virtual bool SupportsViewport() const override { return true; }
        virtual bool SupportsToolbar() const override { return true; }
        virtual void DrawToolbar( UpdateContext const& context ) override;
        virtual void DrawMenu( UpdateContext const& context ) override;
        virtual void DrawHelpMenu() const override;
        virtual void ExtendViewportToolBar( UpdateContext const& context, Viewport* pViewport ) override;
        virtual bool ExtendViewportToolBar_VisualizationControls( UpdateContext const& context, Viewport* pViewport ) override;
        virtual void DrawViewportUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused ) override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;

        virtual void HandleViewportInteractions() override;
        virtual void DropResourceInViewport( ResourceID const& resourceID, Vector const& worldPosition ) override;

        virtual void PreUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;

        // Map
        //-------------------------------------------------------------------------

        void SelectAndLoadMap();
        void SaveMap();
        void SaveMapAs();

        // Edit Mode
        //-------------------------------------------------------------------------

        void DrawEditModeWindow( UpdateContext const& context, bool isFocused );
        void SwitchEditMode( TypeSystem::TypeInfo const* pNewModeTypeInfo );
        void ClearEditMode() { SwitchEditMode( nullptr ); }

    private:

        EditorContext                                   m_editorContext;
        TVector<ViewportResourceDropHandler const*>     m_pViewportDropHandlers;

        TVector<TypeSystem::TypeInfo const*>            m_editModeTypeInfos;
        MapEditorMode*                                  m_pActiveEditMode = nullptr;

        ResourceID                                      m_loadedMap;
        EntityMapID                                     m_editedMapID;
        bool                                            m_isGamePreviewRunning = false;

        TEvent<UpdateContext const&>                    m_requestStartGamePreview;
        TEvent<UpdateContext const&>                    m_requestStopGamePreview;

        ImGuiX::Gizmo                                   m_gizmo;

        EntityOutliner                                  m_outliner;
        EntityInspector                                 m_inspector;
    };
}