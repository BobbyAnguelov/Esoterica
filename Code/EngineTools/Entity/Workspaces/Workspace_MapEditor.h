#pragma once

#include "Workspace_EntityEditor.h"
#include "Engine/Entity/EntityDescriptors.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class NavmeshGeneratorDialog;
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EE_ENGINETOOLS_API EntityMapEditor final : public EntityEditorWorkspace
    {
    public:

        EntityMapEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld );
        ~EntityMapEditor();

        inline bool HasLoadedMap() const { return m_loadedMap.IsValid(); }
        inline ResourceID GetLoadedMap() const { return m_loadedMap; }

        void CreateNewMap();
        void SelectAndLoadMap();
        void LoadMap( TResourcePtr<EntityModel::SerializedEntityMap> const& mapToLoad );
        void SaveMap();
        void SaveMapAs();

        // Game Preview
        //-------------------------------------------------------------------------

        TEventHandle<UpdateContext const&> OnGamePreviewStartRequested() { return m_requestStartGamePreview; }
        TEventHandle<UpdateContext const&> OnGamePreviewStopRequested() { return m_requestStopGamePreview; }

        void NotifyGamePreviewStarted();
        void NotifyGamePreviewEnded();

    private:

        EntityMap* GetEditedMap() const;

        virtual bool Save() override;
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_EARTH; }
        virtual void DrawWorkspaceToolbar( UpdateContext const& context ) override;
        virtual void DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;

        // Navmesh
        //-------------------------------------------------------------------------

        void CreateNavmeshComponent();
        void BeginNavmeshGeneration( UpdateContext const& context );
        void UpdateNavmeshGeneration( UpdateContext const& context );
        void EndNavmeshGeneration( UpdateContext const& context );

    private:

        ResourceID                                      m_loadedMap;
        EntityMapID                                     m_editedMapID;
        bool                                            m_isGamePreviewRunning = false;

        TEvent<UpdateContext const&>                    m_requestStartGamePreview;
        TEvent<UpdateContext const&>                    m_requestStopGamePreview;

        Navmesh::NavmeshGeneratorDialog*                m_pNavmeshGeneratorDialog = nullptr;
    };
}