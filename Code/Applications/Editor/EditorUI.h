#pragma once

#include "EngineTools/Resource/ResourceDatabase.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Engine/ToolsUI/IDevelopmentToolsUI.h"
#include "Engine/DebugViews/DebugView_System.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EditorTool;
    class EntityWorldManager;
    class GamePreviewer;
    namespace EntityModel { class EntityMapEditor; }
    namespace Render{ class RenderingSystem; }

    //-------------------------------------------------------------------------

    class EditorUI final : public ImGuiX::IDevelopmentToolsUI, public ToolsContext
    {
        struct ToolCreationRequest
        {
            enum Type
            {
                MapEditor,
                GamePreview,
                ResourceEditor,
                UninitializedTool,
            };

            ToolCreationRequest() = default;
            ToolCreationRequest( EditorTool* pEditorTool ) : m_pEditorTool( pEditorTool ), m_type( UninitializedTool ) { EE_ASSERT( m_pEditorTool != nullptr ); }

        public:

            ResourceID          m_resourceID;
            EditorTool*          m_pEditorTool = nullptr;
            Type                m_type = ResourceEditor;
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
        virtual bool TryFindInResourceBrowser( ResourceID const& resourceID ) const override;

        // Title bar
        //-------------------------------------------------------------------------

        void DrawTitleBarMenu( UpdateContext const& context );
        void DrawTitleBarInfoStats( UpdateContext const& context );

        // Hot Reload
        //-------------------------------------------------------------------------

        virtual void HotReload_UnloadResources( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded ) override;
        virtual void HotReload_ReloadResources() override;

        // Resource Management
        //-------------------------------------------------------------------------

        void OnResourceDeleted( ResourceID const& resourceID );

        // Editor Tool Management
        //-------------------------------------------------------------------------

        // Immediately destroy a editor tool
        void DestroyTool( UpdateContext const& context, EditorTool* pEditorTool, bool isEditorShutdown = false );

        // Queues a editor tool destruction request till the next update
        void QueueDestroyTool( EditorTool* pEditorTool );

        // Tries to immediately create a editor tool
        bool TryCreateTool( UpdateContext const& context, ToolCreationRequest const& request );

        // Queues a editor tool creation request till the next update
        void QueueCreateTool( ResourceID const& resourceID );

        // Submit a editor tool so we can retrieve/update its docking location
        bool SubmitToolMainWindow( UpdateContext const& context, EditorTool* pEditorTool, ImGuiID editorDockspaceID );

        // Draw editor tool child windows
        void DrawToolContents( UpdateContext const& context, EditorTool* pEditorTool );

        // Create a game preview editor tool
        void CreateGamePreviewTool( UpdateContext const& context );

        // Queues the preview editor tool for destruction
        void DestroyGamePreviewTool( UpdateContext const& context );

        // Copy the layout from one editor tool to the other
        void ToolLayoutCopy( EditorTool* pSourceTool );

        // Get the first created editor tool of a specified type
        template<typename T>
        inline T* GetTool() const
        {
            static_assert( std::is_base_of<EE::EditorTool, T>::value, "T is not derived from EditorTool" );
            for ( auto pEditorTool : m_editorTools )
            {
                if ( pEditorTool->GetUniqueTypeID() == T::s_toolTypeID )
                {
                    return static_cast<T*>( pEditorTool );
                }
            }

            return nullptr;
        }

        // Create a new editor tool
        template<typename T, typename... ConstructorParams>
        inline T* CreateTool( ConstructorParams&&... params )
        {
            static_assert( std::is_base_of<EE::EditorTool, T>::value, "T is not derived from EditorTool" );

            if ( T::s_isSingleton )
            {
                auto pExistingTool = GetTool<T>();
                if( pExistingTool != nullptr )
                {
                    return pExistingTool;
                }
            }

            //-------------------------------------------------------------------------

            T* pEditorTool = EE::New<T>( eastl::forward<ConstructorParams>( params )... );
            ToolCreationRequest& creationRequest = m_editorToolCreationRequests.emplace_back();
            creationRequest.m_type = ToolCreationRequest::UninitializedTool;
            creationRequest.m_pEditorTool = pEditorTool;

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

        // Tools
        TVector<EditorTool*>                            m_editorTools;
        TVector<ToolCreationRequest>                    m_editorToolCreationRequests;
        TVector<EditorTool*>                            m_editorToolDestructionRequests;
        void*                                           m_pLastActiveTool = nullptr;
        bool                                            m_hasOpenModalDialog = false;

        // Map Editor and Game Preview
        EntityModel::EntityMapEditor*                   m_pMapEditor = nullptr;
        GamePreviewer*                                  m_pGamePreviewer = nullptr;
        EventBindingID                                  m_gamePreviewStartRequestEventBindingID;
        EventBindingID                                  m_gamePreviewStopRequestEventBindingID;
    };
}