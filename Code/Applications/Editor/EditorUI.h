#pragma once

#include "EngineTools/Resource/ResourceDatabase.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Core/EditorTool.h"
#include "Engine/ToolsUI/IDevelopmentToolsUI.h"
#include "Engine/DebugViews/DebugView_System.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Workspace;
    class EditorTool;
    class EntityWorldManager;
    class GamePreviewer;
    namespace EntityModel { class EntityMapEditor; }
    namespace Render{ class RenderingSystem; }

    //-------------------------------------------------------------------------

    class EditorUI final : public ImGuiX::IDevelopmentToolsUI, public ToolsContext
    {
        struct WorkspaceCreationRequest
        {
            enum Type
            {
                MapEditor,
                GamePreview,
                ResourceWorkspace,
            };

        public:

            ResourceID          m_resourceID;
            Type                m_type = ResourceWorkspace;
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
        virtual bool TryFindInResourceBrowser( ResourceID const& resourceID ) const override;

        // Title bar
        //-------------------------------------------------------------------------

        void DrawTitleBarMenu( UpdateContext const& context );
        void DrawTitleBarPerformanceStats( UpdateContext const& context );

        // Hot Reload
        //-------------------------------------------------------------------------

        virtual void HotReload_UnloadResources( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded ) override;
        virtual void HotReload_ReloadResources() override;
        virtual void HotReload_ReloadComplete() override;

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

        // Editor Tool Management
        //-------------------------------------------------------------------------

        // Immediately destroy an editor tool
        void DestroyEditorTool( UpdateContext const& context, EditorTool* pEditorTool, bool isEditorShutdown = false );

        // Queues an editor tool destruction request till the next update
        void QueueDestroyEditorTool( EditorTool* pEditorTool );

        // Submit an Editor Tool window so we can retrieve/update its docking location
        bool SubmitEditorToolWindow( UpdateContext const& context, EditorTool* pEditorTool, ImGuiID editorDockspaceID );

        // Draw Editor Tool child windows
        void DrawEditorToolContents( UpdateContext const& context, EditorTool* pEditorTool );

        // Copy the layout from one workspace to the other
        void EditorToolLayoutCopy( EditorTool* pEditorTool );

        // Get the first created editor tool of a specified type
        template<typename T>
        inline T* GetEditorTool() const
        {
            static_assert( std::is_base_of<EE::EditorTool, T>::value, "T is not derived from EditorTool" );
            for ( auto pEditorTool : m_editorTools )
            {
                if ( pEditorTool->GetToolTypeID() == T::s_toolTypeID )
                {
                    return static_cast<T*>( pEditorTool );
                }
            }

            return nullptr;
        }

        // Create a new editor tool
        template<typename T, typename... ConstructorParams>
        inline T* CreateEditorTool( ConstructorParams&&... params )
        {
            static_assert( std::is_base_of<EE::EditorTool, T>::value, "T is not derived from EditorTool" );

            if ( T::s_isSingleton )
            {
                auto pExistingTool = GetEditorTool<T>();
                if( pExistingTool != nullptr )
                {
                    return pExistingTool;
                }
            }

            //-------------------------------------------------------------------------

            T* pEditorTool = EE::New<T>( eastl::forward<ConstructorParams>( params )... );
            m_editorToolsPendingInitialization.emplace_back( pEditorTool );
            return pEditorTool;
        }

        // Misc
        //-------------------------------------------------------------------------

        void DrawUITestWindow();

    private:

        ResourceID                                      m_startupMapResourceID;
        ImGuiX::ApplicationTitleBar                     m_titleBar;
        ImGuiX::ImageInfo                               m_editorIcon;

        // Systems
        Render::RenderingSystem*                        m_pRenderingSystem = nullptr;
        EntityWorldManager*                             m_pWorldManager = nullptr;

        // Window Management
        ImGuiWindowClass                                m_editorWindowClass;
        bool                                            m_isImguiDemoWindowOpen = false;
        bool                                            m_isImguiPlotDemoWindowOpen = false;
        bool                                            m_isUITestWindowOpen = false;

        // Resource Browser
        Resource::ResourceDatabase                      m_resourceDB;
        EventBindingID                                  m_resourceDeletedEventID;
        float                                           m_resourceBrowserViewWidth = 150;

        // Editor Tools and Workspaces
        TVector<EditorTool*>                            m_editorTools;
        TVector<EditorTool*>                            m_editorToolsPendingInitialization;
        TVector<EditorTool*>                            m_editorToolDestructionRequests;

        TVector<Workspace*>                             m_workspaces;
        TVector<WorkspaceCreationRequest>               m_workspaceCreationRequests;
        TVector<Workspace*>                             m_workspaceDestructionRequests;
        void*                                           m_pLastActiveWorkspaceOrEditorTool = nullptr;
        bool                                            m_hasOpenModalDialog = false;

        // Map Editor and Game Preview
        EntityModel::EntityMapEditor*                   m_pMapEditor = nullptr;
        GamePreviewer*                                  m_pGamePreviewer = nullptr;
        EventBindingID                                  m_gamePreviewStartRequestEventBindingID;
        EventBindingID                                  m_gamePreviewStopRequestEventBindingID;
    };
}