#pragma once

#include "EngineTools/Resource/ResourceBrowser/ResourceBrowser.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Engine/ToolsUI/IDevelopmentToolsUI.h"
#include "Engine/DebugViews/DebugView_System.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Workspace;
    class EntityWorldManager;
    class GamePreviewer;
    namespace EntityModel { class EntityMapEditor; }
    namespace Render{ class RenderingSystem; }

    //-------------------------------------------------------------------------

    class EditorUI final : public ImGuiX::IDevelopmentToolsUI, public ToolsContext
    {
    public:

        ~EditorUI();

        void SetStartupMap( ResourceID const& mapID );

        void Initialize( UpdateContext const& context, ImGuiX::ImageCache* pImageCache ) override;
        void Shutdown( UpdateContext const& context ) override;

        TInlineVector<Math::ScreenSpaceRectangle, 4> const& GetTitleBarDraggableRegions() const { return m_titleBar.GetTitleBarDraggableRegions(); }

    private:

        virtual void StartFrame( UpdateContext const& context ) override final;
        virtual void EndFrame( UpdateContext const& context ) override final;
        virtual void Update( UpdateContext const& context ) override final;
        virtual EntityWorldManager* GetWorldManager() const override final { return m_pWorldManager; }
        virtual void TryOpenResource( ResourceID const& resourceID ) const override;

        // Title bar
        //-------------------------------------------------------------------------

        void DrawTitleBarMenu( UpdateContext const& context );
        void DrawTitleBarGamePreviewControls( UpdateContext const& context );
        void DrawTitleBarPerformanceStats( UpdateContext const& context );

        // Hot Reload
        //-------------------------------------------------------------------------

        virtual void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded ) override;
        virtual void EndHotReload() override;

        // Resource Management
        //-------------------------------------------------------------------------

        void OnResourceDeleted( ResourceID const& resourceID );

        // Workspace Management
        //-------------------------------------------------------------------------

        // Immediately destroy a workspace
        void DestroyWorkspace( UpdateContext const& context, Workspace* pWorkspace, bool isEditorShutdown = false );

        // Queues a workspace destruction request till the next update
        void QueueDestroyWorkspace( Workspace* pWorkspace );

        // Tries to immediately create a workspace
        bool TryCreateWorkspace( UpdateContext const& context, ResourceID const& resourceID );

        // Queues a workspace creation request till the next update
        void QueueCreateWorkspace( ResourceID const& resourceID );

        // Draw a workspace
        bool DrawWorkspaceWindow( UpdateContext const& context, Workspace* pWorkspace );

        // Create a game preview workspace
        void CreateGamePreviewWorkspace( UpdateContext const& context );

        // Queues the preview workspace for destruction
        void DestroyGamePreviewWorkspace( UpdateContext const& context );

        // Misc
        //-------------------------------------------------------------------------

        void DrawUITestWindow();

    private:

        ResourceID                          m_startupMapResourceID;
        ImGuiX::ApplicationTitleBar         m_titleBar;
        ImGuiX::ImageInfo                   m_editorIcon;

        // Systems
        Render::RenderingSystem*            m_pRenderingSystem = nullptr;
        EntityWorldManager*                 m_pWorldManager = nullptr;

        // Window Management
        ImGuiWindowClass                    m_editorWindowClass;
        bool                                m_isResourceBrowserWindowOpen = true;
        bool                                m_isResourceLogWindowOpen = false;
        bool                                m_isResourceOverviewWindowOpen = false;
        bool                                m_isPhysicsMaterialDatabaseWindowOpen = false;
        bool                                m_isImguiDemoWindowOpen = false;
        bool                                m_isUITestWindowOpen = false;

        // Resource Browser
        Resource::ResourceDatabase          m_resourceDB;
        ResourceBrowser*                    m_pResourceBrowser = nullptr;
        EventBindingID                      m_resourceDeletedEventID;
        float                               m_resourceBrowserViewWidth = 150;

        // System Log
        SystemLogView                       m_systemLogView;
        bool                                m_isSystemLogWindowOpen = false;

        // Workspaces
        TVector<Workspace*>                 m_workspaces;
        TVector<ResourceID>                 m_workspaceCreationRequests;
        TVector<Workspace*>                 m_workspaceDestructionRequests;

        // Map Editor
        EntityModel::EntityMapEditor*       m_pMapEditor = nullptr;
        GamePreviewer*                      m_pGamePreviewer = nullptr;
        EventBindingID                      m_gamePreviewStartedEventBindingID;
        EventBindingID                      m_gamePreviewStoppedEventBindingID;
    };
}