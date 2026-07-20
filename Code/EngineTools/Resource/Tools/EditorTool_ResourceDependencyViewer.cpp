#include "EditorTool_ResourceDependencyViewer.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    static void GetAllDependencies( ToolsContext const& toolsContext, ResourceDescriptor const* pResourceDesc, TVector<ResourceID>& installDependencies, TVector<Resource::CompileDependency>& compileDependencies )
    {
        EE_ASSERT( pResourceDesc != nullptr );

        TVector<String> subResources;
        pResourceDesc->GetAllSubResources( subResources );

        //-------------------------------------------------------------------------

        pResourceDesc->GetInstallDependencies( *toolsContext.m_pTypeRegistry, toolsContext.GetSourceDataDirectory(), "", installDependencies );
        pResourceDesc->GetCompileDependencies( *toolsContext.m_pTypeRegistry, toolsContext.GetSourceDataDirectory(), "", compileDependencies );

        for ( String const& subResourceID : subResources )
        {
            pResourceDesc->GetInstallDependencies( *toolsContext.m_pTypeRegistry, toolsContext.GetSourceDataDirectory(), subResourceID, installDependencies );
            pResourceDesc->GetCompileDependencies( *toolsContext.m_pTypeRegistry, toolsContext.GetSourceDataDirectory(), subResourceID, compileDependencies );
        }

        //-------------------------------------------------------------------------

        eastl::unique( installDependencies.begin(), installDependencies.end() );
        eastl::unique( compileDependencies.begin(), compileDependencies.end() );
        eastl::sort( installDependencies.begin(), installDependencies.end(), [] ( ResourceID const& lhs, ResourceID const& rhs ) { return lhs.GetDataPath() < rhs.GetDataPath(); } );
        eastl::sort( compileDependencies.begin(), compileDependencies.end(), [] ( Resource::CompileDependency const& lhs, Resource::CompileDependency const& rhs ) { return lhs.m_path < rhs.m_path; } );
    }

    //-------------------------------------------------------------------------

    void ResourceDependencyViewerEditorTool::DependencyTreeNode::Clear()
    {
        for ( auto pNode : m_installDependencies )
        {
            pNode->Clear();
            EE::Delete( pNode );
        }

        for ( auto pNode : m_compileDependencies )
        {
            pNode->Clear();
            EE::Delete( pNode );
        }

        m_path.Clear();
        m_installDependencies.clear();
        m_compileDependencies.clear();
    }

    void ResourceDependencyViewerEditorTool::DependencyTreeNode::AddInstallDependency( ToolsContext const& toolsContext, ResourceID const& installDependencyID )
    {
        EE_ASSERT( installDependencyID.IsValid() );

        DependencyTreeNode* pInstallDependency = m_installDependencies.emplace_back( EE::New<DependencyTreeNode>() );
        pInstallDependency->m_path = installDependencyID.GetDataPath();
        pInstallDependency->m_isResource = true;

        auto pFileInfo = toolsContext.m_pFileRegistry->GetFileEntry( installDependencyID.GetDataPath() );
        if ( pFileInfo == nullptr || !pFileInfo->HasLoadedDescriptor() )
        {
            pInstallDependency->m_isMissingOrInvalidFile = true;
            return;
        }

        //-------------------------------------------------------------------------

        ResourceDescriptor const* pResourceDesc = Cast<ResourceDescriptor>( pFileInfo->m_pDataFile );

        TVector<ResourceID> installDependencies;
        TVector<Resource::CompileDependency> compileDependencies; 
        GetAllDependencies( toolsContext, pResourceDesc, installDependencies, compileDependencies );

        for ( ResourceID const& idep : installDependencies )
        {
            pInstallDependency->AddInstallDependency( toolsContext, idep );
        }

        for ( Resource::CompileDependency const& cdep : compileDependencies )
        {
            pInstallDependency->AddCompileDependency( toolsContext, cdep );
        }
    }

    void ResourceDependencyViewerEditorTool::DependencyTreeNode::AddCompileDependency( ToolsContext const& toolsContext, Resource::CompileDependency const& compileDependency )
    {
        EE_ASSERT( compileDependency.m_path.IsValid() );

        DependencyTreeNode* pInstallDependency = m_compileDependencies.emplace_back( EE::New<DependencyTreeNode>() );
        pInstallDependency->m_path = compileDependency.m_path;
        pInstallDependency->m_isResource = compileDependency.m_isResource;

        auto pFileInfo = toolsContext.m_pFileRegistry->GetFileEntry( compileDependency.m_path );
        if ( pFileInfo == nullptr )
        {
            pInstallDependency->m_isMissingOrInvalidFile = true;
            return;
        }

        //-------------------------------------------------------------------------

        if ( pInstallDependency->m_isResource )
        {
            if ( !pFileInfo->HasLoadedDescriptor() )
            {
                pInstallDependency->m_isMissingOrInvalidFile = true;
                return;
            }

            ResourceDescriptor const* pResourceDesc = Cast<ResourceDescriptor>( pFileInfo->m_pDataFile );

            TVector<ResourceID> installDependencies;
            TVector<Resource::CompileDependency> compileDependencies;
            GetAllDependencies( toolsContext, pResourceDesc, installDependencies, compileDependencies );

            for ( ResourceID const& idep : installDependencies )
            {
                pInstallDependency->AddInstallDependency( toolsContext, idep );
            }

            for ( Resource::CompileDependency const& cdep : compileDependencies )
            {
                pInstallDependency->AddCompileDependency( toolsContext, cdep );
            }
        }
        else
        {
            if ( pFileInfo->IsDataFile() )
            {
                pInstallDependency->m_isMissingOrInvalidFile = !pFileInfo->HasLoadedDataFile();
            }
        }
    }

    //-------------------------------------------------------------------------

    ResourceDependencyViewerEditorTool::DependencyView::DependencyView( ResourceID const& ID )
        : m_ID( ID )
        , m_tabName(  )
    {
        EE_ASSERT( m_ID.IsValid() );

        m_tabName.sprintf( "%s##%d", ID.GetFilename().c_str(), ID.GetPathID() );
    }

    //-------------------------------------------------------------------------

    ResourceDependencyViewerEditorTool::ResourceDependencyViewerEditorTool( ToolsContext const* pToolsContext )
        : EditorTool( pToolsContext, "Resource Dependency Viewer" )
        , m_resourcePicker( *pToolsContext )
    {
        m_fileRegistryUpdateEventBindingID = m_pToolsContext->m_pFileRegistry->OnFileSystemCacheUpdated().Bind( [this] () { OnFileRegistryUpdated(); } );
    }

    ResourceDependencyViewerEditorTool::~ResourceDependencyViewerEditorTool()
    {
        m_pToolsContext->m_pFileRegistry->OnFileSystemCacheUpdated().Unbind( m_fileRegistryUpdateEventBindingID );

        // Free all dependency trees
        for ( auto& dependencyView : m_dependencyViews )
        {
            dependencyView.ClearDependencyTree();
        }
    }

    //-------------------------------------------------------------------------

    void ResourceDependencyViewerEditorTool::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );
        CreateToolWindow( "Dependency View", [this] ( UpdateContext const& context, bool isFocused ) { DrawWindow( context, isFocused ); } );
    }

    void ResourceDependencyViewerEditorTool::OnFileRegistryUpdated()
    {
        for ( DependencyView& view : m_dependencyViews )
        {
            RefreshView( view );
        }
    }

    void ResourceDependencyViewerEditorTool::DrawWindow( UpdateContext const& context, bool isFocused )
    {
        for ( ResourceID const& viewCloseRequest : m_viewCloseRequests )
        {
            int32_t const foundIdx = FindViewIndex( viewCloseRequest );
            if ( foundIdx != InvalidIndex )
            {
                m_dependencyViews[foundIdx].ClearDependencyTree();
                m_dependencyViews.erase( m_dependencyViews.begin() + foundIdx );
            }
        }
        m_viewCloseRequests.clear();

        // Custom Picker
        //-------------------------------------------------------------------------

        if ( m_resourcePicker.UpdateAndDraw() )
        {
            if ( m_resourcePicker.GetResourceID().IsValid() )
            {
                ShowDependenciesForResourceID( m_resourcePicker.GetResourceID() );
            }
            m_resourcePicker.Clear();
        }

        // View
        //-------------------------------------------------------------------------

        if ( ImGui::BeginChild( "ViewChild", ImGui::GetContentRegionAvail() ) )
        {
            int32_t focusViewIdx = InvalidIndex;
            if ( m_viewFocusRequest.IsValid() )
            {
                focusViewIdx = FindViewIndex( m_viewFocusRequest );
                m_viewFocusRequest.Clear();
            }

            if ( ImGui::BeginTabBar( "DependencyViews", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_DrawSelectedOverline | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll ) )
            {
                int32_t const numViews = (int32_t) m_dependencyViews.size();
                for ( int32_t i = 0; i < numViews; i++ )
                {
                    bool isOpen = true;
                    uint32_t const tabItemFlags = ( focusViewIdx == i ) ? ImGuiTabItemFlags_SetSelected : 0;
                    if ( ImGui::BeginTabItem( m_dependencyViews[i].m_tabName.c_str(), &isOpen, tabItemFlags ) )
                    {
                        ImGuiX::ItemTooltip( m_dependencyViews[i].m_ID.c_str() );

                        m_resourcePicker.SetResourceID( m_dependencyViews[i].m_ID );

                        DrawView( context, m_dependencyViews[i] );
                        ImGui::EndTabItem();
                    }

                    if ( !isOpen )
                    {
                        m_viewCloseRequests.emplace_back( m_dependencyViews[i].m_ID );
                    }
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::EndChild();
    }

    void ResourceDependencyViewerEditorTool::ShowDependenciesForResourceID( ResourceID const& resourceID )
    {
        int32_t const nFoundIdx = FindViewIndex( resourceID );
        if ( nFoundIdx != InvalidIndex )
        {
            RefreshView( m_dependencyViews[nFoundIdx] );
        }
        else
        {
            RefreshView( m_dependencyViews.emplace_back( resourceID ) );
        }
    }

    void ResourceDependencyViewerEditorTool::RefreshView( DependencyView &view )
    {
        m_viewFocusRequest = view.m_ID;

        FileRegistry::FileInfo const* pFileInfo = m_pToolsContext->m_pFileRegistry->GetFileEntry( view.m_ID.GetDataPath() );
        if ( pFileInfo == nullptr || !pFileInfo->IsResourceDescriptorFile() )
        {
            m_viewCloseRequests.emplace_back( view.m_ID );
            return;
        }

        view.m_pFileInfo = pFileInfo;
        auto pResourceDesc = Cast<ResourceDescriptor>( pFileInfo->m_pDataFile );

        // Create dependency Tree
        //-------------------------------------------------------------------------

        view.ClearDependencyTree();

        TVector<ResourceID> installDependencies;
        TVector<Resource::CompileDependency> compileDependencies;
        GetAllDependencies( *m_pToolsContext, pResourceDesc, installDependencies, compileDependencies );

        for ( ResourceID const& idep : installDependencies )
        {
            view.AddInstallDependency( *m_pToolsContext, idep );
        }

        for ( Resource::CompileDependency const& cdep : compileDependencies )
        {
            view.AddCompileDependency( *m_pToolsContext, cdep );
        }

        // Get everything that depends on this resource
        //-------------------------------------------------------------------------

        view.m_dependentResources = m_pToolsContext->m_pFileRegistry->GetAllDependentResources( view.m_ID.GetDataPath() );
    }

    void ResourceDependencyViewerEditorTool::DrawView( UpdateContext const& context, DependencyView &view )
    {
        ImGui::Text( view.m_ID.c_str() );

        ImGui::SeparatorText( "Depends On" );

        for ( auto pDep : view.m_installDependencies )
        {
            DrawInstallDependencyNode( pDep );
        }

        for ( auto pDep : view.m_compileDependencies )
        {
            DrawCompileDependencyNode( pDep );
        }

        ImGui::SeparatorText( "Is Dependency For" );

        for ( auto pDep : view.m_dependentResources )
        {
            DrawDependentResource( pDep );
        }
    }

    void ResourceDependencyViewerEditorTool::DrawInstallDependencyNode( DependencyTreeNode* pNode )
    {
        InlineString str( InlineString::CtorSprintf(), "Install Dep: %s", pNode->m_path.c_str() );
        if ( ImGui::TextLink( str.c_str() ) )
        {
            ShowDependenciesForResourceID( pNode->m_path );
        }

        ImGui::PushID( pNode->m_path.c_str() );
        ImGui::Indent();

        for ( auto pDep : pNode->m_installDependencies )
        {
            DrawInstallDependencyNode( pDep );
        }

        for ( auto pDep : pNode->m_compileDependencies )
        {
            DrawCompileDependencyNode( pDep );
        }

        ImGui::Unindent();
        ImGui::PopID();
    }

    void ResourceDependencyViewerEditorTool::DrawCompileDependencyNode( DependencyTreeNode* pNode )
    {
        InlineString str( InlineString::CtorSprintf(), "Compile Dep: %s", pNode->m_path.c_str() );
        if ( pNode->m_isResource )
        {
            if ( ImGui::TextLink( str.c_str() ) )
            {
                ShowDependenciesForResourceID( pNode->m_path );
            }
        }

        ImGui::PushID( pNode->m_path.c_str() );
        ImGui::Indent();

        for ( auto pDep : pNode->m_installDependencies )
        {
            DrawInstallDependencyNode( pDep );
        }

        for ( auto pDep : pNode->m_compileDependencies )
        {
            DrawCompileDependencyNode( pDep );
        }

        ImGui::Unindent();
        ImGui::PopID();
    }

    void ResourceDependencyViewerEditorTool::DrawDependentResource( DataPath const& path )
    {
        if ( ImGui::TextLink( path.c_str() ) )
        {
            ShowDependenciesForResourceID( path );
        }
    }
}