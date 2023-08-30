#include "EditorTool.h"
#include "Engine/UpdateContext.h"
#include "EngineTools/ThirdParty/subprocess/subprocess.h"
#include "EngineTools/Resource/ResourceCompiler.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/Resource/ResourceSettings.h"
#include <EASTL/sort.h>

//-------------------------------------------------------------------------

namespace EE
{
    EditorTool::EditorTool( ToolsContext const* pToolsContext, String const& displayName )
        : m_pToolsContext( pToolsContext )
        , m_windowName( displayName ) // Temp storage for display name since we cant call virtuals from CTOR
    {
        EE_ASSERT( m_pToolsContext != nullptr && m_pToolsContext->IsValid() );

        m_ID = Hash::GetHash32( displayName );
    }

    EditorTool::~EditorTool()
    {
        EE_ASSERT( m_requestedResources.empty() );
        EE_ASSERT( m_reloadingResources.empty() );
        EE_ASSERT( !m_isInitialized );
        EE_ASSERT( !m_isHotReloading );
    }

    //-------------------------------------------------------------------------

    void EditorTool::Initialize( UpdateContext const& context )
    {
        m_pResourceSystem = context.GetSystem<Resource::ResourceSystem>();
        SetDisplayName( m_windowName );
        m_isInitialized = true;
    }

    void EditorTool::Shutdown( UpdateContext const& context )
    {
        EE_ASSERT( m_isInitialized );
        m_pResourceSystem = nullptr;
        m_isInitialized = false;
    }

    void EditorTool::SetDisplayName( String name )
    {
        EE_ASSERT( !name.empty() );

        // Add in title bar icon
        //-------------------------------------------------------------------------

        if ( HasTitlebarIcon() )
        {
            m_windowName.sprintf( "%s %s", GetTitlebarIcon(), name.c_str() );
        }
        else
        {
            m_windowName = name;
        }
    }

    void EditorTool::CreateToolWindow( String const& name, TFunction<void( UpdateContext const&, bool )> const& drawFunction, ImVec2 const& windowPadding, bool disableScrolling )
    {
        for ( auto const& toolWindow : m_toolWindows )
        {
            EE_ASSERT( toolWindow.m_name != name );
        }

        m_toolWindows.emplace_back( name, drawFunction, windowPadding, disableScrolling );

        eastl::sort( m_toolWindows.begin(), m_toolWindows.end(), [] ( ToolWindow const& lhs, ToolWindow const& rhs ) { return lhs.m_name < rhs.m_name; } );
    }

    //-------------------------------------------------------------------------

    void EditorTool::InitializeDockingLayout( ImGuiID const dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGui::DockBuilderRemoveNodeChildNodes( dockspaceID );
    }

    //-------------------------------------------------------------------------

    void EditorTool::DrawSharedMenus()
    {
        if ( !IsSingleWindowTool() && !m_toolWindows.empty() )
        {
            if ( ImGui::BeginMenu( EE_ICON_WINDOW_RESTORE" Window" ) )
            {
                for ( auto& toolWindow : m_toolWindows )
                {
                    ImGui::MenuItem( toolWindow.m_name.c_str(), nullptr, &toolWindow.m_isOpen );
                }

                ImGui::EndMenu();
            }
        }

        if ( ImGui::BeginMenu( EE_ICON_HELP_CIRCLE_OUTLINE" Help" ) )
        {
            if ( ImGui::BeginTable( "HelpTable", 2 ) )
            {
                DrawHelpMenu();
                ImGui::EndTable();
            }

            ImGui::EndMenu();
        }
    }

    void EditorTool::DrawHelpTextRow( char const* pLabel, char const* pText ) const
    {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );
            ImGui::Text( pLabel );
        }

        ImGui::TableNextColumn();
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );
            ImGui::Text( pText );
        }
    }

    void EditorTool::DrawHelpTextRowCustom( char const* pLabel, TFunction<void()> const& function ) const
    {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );
            ImGui::Text( pLabel );
        }

        ImGui::TableNextColumn();
        {
            function();
        }
    }

    //-------------------------------------------------------------------------

    void EditorTool::LoadResource( Resource::ResourcePtr* pResourcePtr )
    {
        EE_ASSERT( m_pResourceSystem != nullptr );
        EE_ASSERT( pResourcePtr != nullptr && pResourcePtr->IsUnloaded() );
        EE_ASSERT( !VectorContains( m_requestedResources, pResourcePtr ) );
        m_requestedResources.emplace_back( pResourcePtr );
        m_pResourceSystem->LoadResource( *pResourcePtr, Resource::ResourceRequesterID( Resource::ResourceRequesterID::s_toolsRequestID ) );
    }

    void EditorTool::UnloadResource( Resource::ResourcePtr* pResourcePtr )
    {
        EE_ASSERT( m_pResourceSystem != nullptr );
        EE_ASSERT( pResourcePtr->WasRequested() );
        EE_ASSERT( VectorContains( m_requestedResources, pResourcePtr ) );
        m_pResourceSystem->UnloadResource( *pResourcePtr, Resource::ResourceRequesterID( Resource::ResourceRequesterID::s_toolsRequestID ) );
        m_requestedResources.erase_first_unsorted( pResourcePtr );
    }

    bool EditorTool::RequestImmediateResourceCompilation( ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );

        // Trigger external recompile request and wait for it
        //-------------------------------------------------------------------------

        char const* processCommandLineArgs[5] = { m_pResourceSystem->GetSettings().m_resourceCompilerExecutablePath.c_str(), "-compile", resourceID.c_str(), nullptr, nullptr };

        subprocess_s subProcess;
        int32_t result = subprocess_create( processCommandLineArgs, subprocess_option_combined_stdout_stderr | subprocess_option_inherit_environment | subprocess_option_no_window, &subProcess );
        if ( result != 0 )
        {
            return false;
        }

        int32_t exitCode = -1;
        result = subprocess_join( &subProcess, &exitCode );
        if ( result != 0 )
        {
            subprocess_destroy( &subProcess );
            return false;
        }

        subprocess_destroy( &subProcess );

        // Notify resource server to reload the resource
        //-------------------------------------------------------------------------

        if ( exitCode >= (int32_t) Resource::CompilationResult::SuccessUpToDate )
        {
            m_pResourceSystem->RequestResourceHotReload( resourceID );
        }

        //-------------------------------------------------------------------------

        return true;
    }

    //-------------------------------------------------------------------------

    void EditorTool::HotReload_UnloadResources( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded )
    {
        EE_ASSERT( !m_isHotReloading );

        // Do we have any resources that need reloading
        TInlineVector<Resource::ResourcePtr*, 10> resourcesThatNeedReloading;
        for ( auto& pLoadedResource : m_requestedResources )
        {
            if ( pLoadedResource->IsUnloaded() )
            {
                continue;
            }

            // Check resource and install dependencies to see if we need to unload it
            bool shouldUnload = VectorContains( resourcesToBeReloaded, pLoadedResource->GetResourceID() );
            if ( !shouldUnload )
            {
                for ( ResourceID const& installDependency : pLoadedResource->GetInstallDependencies() )
                {
                    if ( VectorContains( resourcesToBeReloaded, installDependency ) )
                    {
                        shouldUnload = true;
                        break;
                    }
                }
            }

            // Request unload and track the resource we need to reload
            if ( shouldUnload )
            {
                resourcesThatNeedReloading.emplace_back( pLoadedResource );
            }
        }

        //-------------------------------------------------------------------------

        // Do we need to hot-reload anything?
        if ( !resourcesThatNeedReloading.empty() )
        {
            OnHotReloadStarted( resourcesThatNeedReloading );

            //-------------------------------------------------------------------------

            // Unload resources
            for ( auto& pResource : resourcesThatNeedReloading )
            {
                // The 'OnHotReloadStarted' function might have unloaded some resources as part of its execution, so ignore those here
                if ( pResource->IsUnloaded() )
                {
                    continue;
                }

                m_pResourceSystem->UnloadResource( *pResource, Resource::ResourceRequesterID( Resource::ResourceRequesterID::s_toolsRequestID ) );
                m_reloadingResources.emplace_back( pResource );
            }

            m_isHotReloading = true;
        }
    }

    void EditorTool::HotReload_ReloadResources()
    {
        bool const hasResourcesToReload = !m_reloadingResources.empty();
        if ( hasResourcesToReload )
        {
            // Load all unloaded resources
            for ( auto& pReloadedResource : m_reloadingResources )
            {
                m_pResourceSystem->LoadResource( *pReloadedResource, Resource::ResourceRequesterID( Resource::ResourceRequesterID::s_toolsRequestID ) );
            }
            m_reloadingResources.clear();
        }
    }

    void EditorTool::HotReload_ReloadComplete()
    {
        if ( m_isHotReloading )
        {
            OnHotReloadComplete();
            m_isHotReloading = false;
        }
    }
}