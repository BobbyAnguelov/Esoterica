#pragma once

#include "EngineTools/FileSystem/FileRegistry.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Engine/ToolsUI/IDevelopmentToolsUI.h"
#include "Engine/DebugViews/DebugView_System.h"
#include "Base/Imgui/ImguiImageCache.h"

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
        struct ToolOperation
        {
            enum Type
            {
                CreateMapEditor,
                CreateGamePreview,
                OpenFile,
                InitializeTool,
                DestroyTool
            };

            ToolOperation() = default;
            
            ToolOperation( EditorTool* pEditorTool, Type type ) : m_pEditorTool( pEditorTool ), m_type( type ) 
            {
                EE_ASSERT( m_pEditorTool != nullptr ); 
                EE_ASSERT( m_type == InitializeTool || m_type == DestroyTool );
            }

            ToolOperation( DataPath const& path ) 
                : m_path( path )
            {
                EE_ASSERT( path.IsValid() );
            }

        public:

            EditorTool*         m_pEditorTool = nullptr;
            Type                m_type = OpenFile;
            DataPath            m_path;
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
        virtual bool TryOpenDataFile( DataPath const& path ) const override;
        virtual bool TryFindInResourceBrowser( DataPath const& path ) const override;

        // Title bar
        //-------------------------------------------------------------------------

        void DrawTitleBarMenu( UpdateContext const& context );
        void DrawTitleBarInfoStats( UpdateContext const& context );

        // Hot Reload
        //-------------------------------------------------------------------------

        virtual void HotReload_UnloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded ) override;
        virtual void HotReload_ReloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded ) override;

        // Resource Management
        //-------------------------------------------------------------------------

        void OnFileDeleted( DataPath const& dataPath );

        // Editor Tool Management
        //-------------------------------------------------------------------------

        // Immediately destroy a editor tool
        void DestroyTool( UpdateContext const& context, EditorTool* pEditorTool, bool isEditorShutdown = false );

        // Queues a editor tool destruction request till the next update
        void QueueDestroyTool( EditorTool* pEditorTool );

        // Tries to immediately create a editor tool
        bool ExecuteToolOperation( UpdateContext const& context, ToolOperation& request );

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

        // Check if the tool is currently open
        template<typename T>
        inline bool IsToolOpen() const
        {
            static_assert( std::is_base_of<EE::EditorTool, T>::value, "T is not derived from EditorTool" );
            for ( auto pEditorTool : m_editorTools )
            {
                if ( pEditorTool->GetUniqueTypeID() == T::s_toolTypeID )
                {
                    return true;
                }
            }

            return false;
        }

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
                // Check for already created tools
                auto pExistingTool = GetTool<T>();
                if( pExistingTool != nullptr )
                {
                    return pExistingTool;
                }

                // Check for already queued creation request
                for ( auto const& creationRequest : m_toolOperations )
                {
                    if ( creationRequest.m_pEditorTool == nullptr )
                    {
                        continue;
                    }

                    if ( creationRequest.m_pEditorTool->GetUniqueTypeID() == T::s_toolTypeID )
                    {
                        return static_cast<T*>( creationRequest.m_pEditorTool );
                    }
                }
            }

            //-------------------------------------------------------------------------

            T* pEditorTool = EE::New<T>( eastl::forward<ConstructorParams>( params )... );
            ToolOperation& creationRequest = m_toolOperations.emplace_back();
            creationRequest.m_type = ToolOperation::InitializeTool;
            creationRequest.m_pEditorTool = pEditorTool;

            return pEditorTool;
        }

        bool IsToolQueuedForDestruction( EditorTool* pEditorTool ) const;

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
        FileRegistry                                    m_fileRegistry;
        EventBindingID                                  m_resourceDeletedEventID;
        float                                           m_resourceBrowserViewWidth = 150;

        // Tools
        TVector<EditorTool*>                            m_editorTools;
        mutable TVector<ToolOperation>                  m_toolOperations;
        void*                                           m_pLastActiveTool = nullptr;
        bool                                            m_hasOpenModalDialog = false;
        String                                          m_focusTargetWindowName; // If this is set we need to switch focus to this window

        // Map Editor and Game Preview
        EntityModel::EntityMapEditor*                   m_pMapEditor = nullptr;
        GamePreviewer*                                  m_pGamePreviewer = nullptr;
        EventBindingID                                  m_gamePreviewStartRequestEventBindingID;
        EventBindingID                                  m_gamePreviewStopRequestEventBindingID;
    };
}