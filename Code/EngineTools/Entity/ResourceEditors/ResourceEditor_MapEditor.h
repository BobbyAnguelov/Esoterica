#pragma once

#include "ResourceEditor_EntityEditor.h"
#include "Engine/Entity/EntityDescriptors.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class NavmeshGeneratorDialog;
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EE_ENGINETOOLS_API EntityMapEditor final : public EntityEditor
    {
        EE_SINGLETON_EDITOR_TOOL( EntityMapEditor );

    public:

        EntityMapEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld );
        ~EntityMapEditor();

        virtual void Initialize( UpdateContext const& context ) override;

        // Map
        //-------------------------------------------------------------------------

        inline bool HasLoadedMap() const { return m_loadedMap.IsValid(); }
        inline ResourceID GetLoadedMap() const { return m_loadedMap; }
        void LoadMap( TResourcePtr<EntityModel::EntityMapDescriptor> const& mapToLoad );

        // Game Preview
        //-------------------------------------------------------------------------

        TEventHandle<UpdateContext const&> OnGamePreviewStartRequested() { return m_requestStartGamePreview; }
        TEventHandle<UpdateContext const&> OnGamePreviewStopRequested() { return m_requestStopGamePreview; }

        void NotifyGamePreviewStarted();
        void NotifyGamePreviewEnded();

    private:

        EntityMap* GetEditedMap() const;

        virtual bool IsEditingFile( DataPath const& dataPath ) const override { return m_loadedMap.GetDataPath() == dataPath; }
        virtual bool SupportsNewFileCreation() const override { return true; }
        virtual void CreateNewFile() const override;
        virtual bool SaveData() override;
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_EARTH; }
        virtual void DrawMenu( UpdateContext const& context ) override;
        virtual void DrawHelpMenu() const override;
        virtual void DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;

        // Map
        //-------------------------------------------------------------------------

        void SelectAndLoadMap();
        void SaveMap();
        void SaveMapAs();

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