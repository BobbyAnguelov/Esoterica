#pragma once

#include "EngineTools/Entity/EntityEditor/EntityEditor_BaseWorkspace.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class NavmeshGeneratorDialog;
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EE_ENGINETOOLS_API EntityMapEditor final : public EntityEditorBaseWorkspace
    {
    public:

        EntityMapEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld );
        ~EntityMapEditor();

        inline bool HasLoadedMap() const { return m_loadedMap.IsValid(); }
        inline ResourceID GetLoadedMap() const { return m_loadedMap; }

        void CreateNewMap();
        void SelectAndLoadMap();
        void LoadMap( TResourcePtr<EntityModel::EntityMapDescriptor> const& mapToLoad );
        void SaveMap();
        void SaveMapAs();

        // Game Preview
        //-------------------------------------------------------------------------

        inline TEventHandle<UpdateContext const&> OnGamePreviewStartRequested() { return m_gamePreviewStartRequested; }
        inline TEventHandle<UpdateContext const&> OnGamePreviewStopRequested() { return m_gamePreviewStopRequested; }

        void NotifyGamePreviewStarted();
        void NotifyGamePreviewEnded();

    private:

        virtual uint32_t GetID() const override { return 0xFFFFFFFF; }
        virtual bool IsDirty() const override{ return false; } // TODO
        virtual bool Save() override;
        virtual void DrawWorkspaceToolbarItems( UpdateContext const& context ) override;
        virtual void UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;

        // Navmesh
        //-------------------------------------------------------------------------

        void CreateNavmeshComponent();
        void BeginNavmeshGeneration( UpdateContext const& context );
        void UpdateNavmeshGeneration( UpdateContext const& context );
        void EndNavmeshGeneration( UpdateContext const& context );

    private:

        ResourceID                                      m_loadedMap;
        bool                                            m_isGamePreviewRunning = false;
        TEvent<UpdateContext const&>                    m_gamePreviewStartRequested;
        TEvent<UpdateContext const&>                    m_gamePreviewStopRequested;

        Navmesh::NavmeshGeneratorDialog*                m_pNavmeshGeneratorDialog = nullptr;
    };
}