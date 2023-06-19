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
    namespace Resource { class RawFileInspector; }

    //-------------------------------------------------------------------------

    class EditorUI final : public ImGuiX::IDevelopmentToolsUI, public ToolsContext
    {
        struct WorkspaceCreationRequest
        {
            enum Type
            {
                MapEditor,
                GamePreview,
                ResourceWorkspace
            };

        public:

            ResourceID  m_resourceID;
            Type        m_type = ResourceWorkspace;
        };

    public:

        ~EditorUI();

        void SetStartupMap( ResourceID const& mapID );

        void Initialize( UpdateContext const& context, ImGuiX::ImageCache* pImageCache ) override;
        void Shutdown( UpdateContext const& context ) override;

        void GetBorderlessTitleBarInfo( Math::ScreenSpaceRectangle& outTitlebarRect, bool& isInteractibleWidgetHovered ) const;

    private:

        virtual void StartFrame( UpdateContext const& context ) override final;
        virtual void EndFrame( UpdateContext const& context ) override final;
        virtual void Update( UpdateContext const& context ) override final;
        virtual EntityWorldManager* GetWorldManager() const override final { return m_pWorldManager; }
        virtual bool TryOpenResource( ResourceID const& resourceID ) const override;
        virtual bool TryOpenRawResource( FileSystem::Path const& resourcePath ) const override;

        // Title bar
        //-------------------------------------------------------------------------

        void DrawTitleBarMenu( UpdateContext const& context );
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
        bool TryCreateWorkspace( UpdateContext const& context, WorkspaceCreationRequest const& request );

        // Queues a workspace creation request till the next update
        void QueueCreateWorkspace( ResourceID const& resourceID );

        // Submit a workspace so we can retrieve/update its docking location
        bool SubmitWorkspaceWindow( UpdateContext const& context, Workspace* pWorkspace, ImGuiID editorDockspaceID );

        // Draw workspace child windows
        void DrawWorkspaceContents( UpdateContext const& context, Workspace* pWorkspace );

        // Create a game preview workspace
        void CreateGamePreviewWorkspace( UpdateContext const& context );

        // Queues the preview workspace for destruction
        void DestroyGamePreviewWorkspace( UpdateContext const& context );

        // Copy the layout from one workspace to the other
        void WorkspaceLayoutCopy( Workspace* pSourceWorkspace );

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
        bool                                m_isImguiDemoWindowOpen = false;
        bool                                m_isUITestWindowOpen = false;

        // Resource Browser
        Resource::ResourceDatabase          m_resourceDB;
        ResourceBrowser*                    m_pResourceBrowser = nullptr;
        EventBindingID                      m_resourceDeletedEventID;
        float                               m_resourceBrowserViewWidth = 150;
        Resource::RawFileInspector*         m_pRawResourceInspector = nullptr;

        // System Log
        SystemLogView                       m_systemLogView;
        bool                                m_isSystemLogWindowOpen = false;

        // Workspaces
        TVector<Workspace*>                 m_workspaces;
        TVector<WorkspaceCreationRequest>   m_workspaceCreationRequests;
        TVector<Workspace*>                 m_workspaceDestructionRequests;
        Workspace*                          m_pLastActiveWorkspace = nullptr;

        // Map Editor and Game Preview
        EntityModel::EntityMapEditor*       m_pMapEditor = nullptr;
        GamePreviewer*                      m_pGamePreviewer = nullptr;
        EventBindingID                      m_gamePreviewStartRequestEventBindingID;
        EventBindingID                      m_gamePreviewStopRequestEventBindingID;
    };
}