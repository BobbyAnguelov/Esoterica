#include "ResourceServerUI.h"
#include "ResourceServer.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include "Base/Imgui/ImguiSystem.h"
#include "EngineTools/Core/ToolsEmbeddedResources.inl"
#include <eastl/sort.h>

//-------------------------------------------------------------------------

namespace EE::Resource
{
    static char const* const g_completedRequestsWindowName = "Completed Requests";
    static char const* const g_serverControlsWindowName = "Server";
    static char const* const g_connectionInfoWindowName = "Connected Clients";
    static char const* const g_packagingControlsWindowName = "Packaging";

    //-------------------------------------------------------------------------

    ResourceServerUI::ResourceServerUI( ResourceServer& resourceServer, ImGuiX::ImageCache* pImageCache )
        : m_resourceServer( resourceServer )
        , m_pImageCache( pImageCache )
    {
        EE_ASSERT( m_pImageCache != nullptr );
    }

    ResourceServerUI::~ResourceServerUI()
    {
        EE_ASSERT( !m_resourceServerIcon.IsValid() );
    }

    //-------------------------------------------------------------------------

    void ResourceServerUI::Initialize()
    {
        EE_ASSERT( m_pImageCache != nullptr && m_pImageCache->IsInitialized() );
        m_resourceServerIcon = m_pImageCache->LoadImageFromMemoryBase64( g_iconDataPurple, 3332 );
    }

    void ResourceServerUI::Shutdown()
    {
        m_pImageCache->UnloadImage( m_resourceServerIcon );
    }

    void ResourceServerUI::Draw()
    {
        // Window title bar
        //-------------------------------------------------------------------------

        auto DrawTitleBarContents = [this] ()
        {
            ImGui::SetCursorPos( ImGui::GetCursorPos() + ImVec2( 0, 4 ) );
            ImGuiX::Image( m_resourceServerIcon.m_ID, m_resourceServerIcon.m_size );
            ImGui::SameLine();
            ImGui::SetCursorPos( ImGui::GetCursorPos() + ImVec2( -8, -2 ) );
            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Resource Server" );
        };

        m_titleBar.Draw( DrawTitleBarContents, 200 );

        // Create dock space
        //-------------------------------------------------------------------------

        ImGuiViewport const* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos( viewport->WorkPos );
        ImGui::SetNextWindowSize( viewport->WorkSize );
        ImGui::SetNextWindowViewport( viewport->ID );

        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );

        ImGuiWindowFlags const windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::Begin( "DockSpace", nullptr, windowFlags );
        {
            ImGuiID const dockspaceID = ImGui::GetID( "ResourceServerDockSpace" );

            // Initial Layout
            if ( !ImGui::DockBuilderGetNode( dockspaceID ) )
            {
                ImGui::DockBuilderAddNode( dockspaceID, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton );
                ImGui::DockBuilderSetNodeSize( dockspaceID, viewport->Size );

                ImGuiID topDockID = 0, topRightDockID = 0;
                ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.7f, nullptr, &topDockID );
                ImGuiID topLeftDockID = ImGui::DockBuilderSplitNode( topDockID, ImGuiDir_Left, 0.5f, nullptr, &topRightDockID );

                ImGui::DockBuilderDockWindow( g_connectionInfoWindowName, topLeftDockID );
                ImGui::DockBuilderDockWindow( g_serverControlsWindowName, topRightDockID );
                ImGui::DockBuilderDockWindow( g_packagingControlsWindowName, topLeftDockID );
                ImGui::DockBuilderDockWindow( g_completedRequestsWindowName, bottomDockID );

                ImGui::DockBuilderFinish( dockspaceID );
            }

            ImGui::DockSpace( dockspaceID, ImVec2( 0.0f, 0.0f ), ImGuiDockNodeFlags_None );
        }
        ImGui::PopStyleVar( 3 );
        ImGui::End();

        // Draw windows
        //-------------------------------------------------------------------------

        DrawServerControls();
        DrawConnectionInfo();
        DrawRequests();
        DrawPackagingControls();
    }

    //-------------------------------------------------------------------------

    void ResourceServerUI::GetBorderlessTitleBarInfo( Math::ScreenSpaceRectangle& outTitlebarRect, bool& isInteractibleWidgetHovered ) const
    {
        outTitlebarRect = m_titleBar.GetScreenRectangle();
        isInteractibleWidgetHovered = ImGui::IsAnyItemHovered();
    }

    //-------------------------------------------------------------------------

    void ResourceServerUI::DrawRequests()
    {
        if ( ImGui::Begin( g_completedRequestsWindowName ) )
        {
            constexpr static float const compilationLogFieldHeight = 125;
            constexpr static float const buttonWidth = 140;
            float const tableHeight = ImGui::GetContentRegionAvail().y - compilationLogFieldHeight - ImGui::GetFrameHeight() - 16;
            float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
            float const textfieldWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x - ( ( buttonWidth + itemSpacing ) * 5 );

            //-------------------------------------------------------------------------
            // Table
            //-------------------------------------------------------------------------

            ImGuiX::ScopedFont const BigScopedFont( ImGuiX::Font::Small );

            if ( ImGui::BeginTable( "Requests", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY, ImVec2( 0, tableHeight ) ) )
            {
                auto const& requests = m_resourceServer.GetRequests();

                //-------------------------------------------------------------------------

                ImGui::TableSetupColumn( "##Status", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 18 );
                ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30 );
                ImGui::TableSetupColumn( "ID", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "Compile", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 60 );
                ImGui::TableSetupColumn( "Origin", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "Source", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "Destination", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupScrollFreeze( 0, 1 );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                ImGuiListClipper clipper;
                clipper.Begin( (int32_t) requests.size() );
                while ( clipper.Step() )
                {
                    for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                    {
                        CompilationRequest const* pRequest = requests[i];
                        ImGui::PushID( pRequest );

                        ImVec4 itemColor;
                        ImGui::TableNextRow();

                        //-------------------------------------------------------------------------

                        auto HandleContextMenuOpening = [&] ()
                        {
                            if ( ImGui::IsItemHovered() && ImGui::IsMouseReleased( ImGuiMouseButton_Right ) )
                            {
                                ImGui::OpenPopup( "RequestContextMenu" );
                                m_pContextMenuRequest = pRequest;
                            }
                        };

                        //-------------------------------------------------------------------------

                        ImGui::TableNextColumn();
                        switch ( pRequest->GetStatus() )
                        {
                            case CompilationRequest::Status::Pending:
                            {
                                itemColor = Colors::LightGray.ToFloat4();
                                ImGui::TextColored( itemColor, EE_ICON_CLOCK );
                                HandleContextMenuOpening();
                                ImGuiX::TextTooltip( "Pending" );
                            }
                            break;

                            case CompilationRequest::Status::Compiling:
                            {
                                ImGuiX::DrawSpinner( "##Compiling", Colors::Cyan, ImVec2( 16, 16 ), 3.0f, 0.0f );
                                HandleContextMenuOpening();
                                ImGuiX::TextTooltip( "Compiling" );
                            }
                            break;

                            case CompilationRequest::Status::Succeeded:
                            {
                                itemColor = Colors::Lime.ToFloat4();
                                ImGui::TextColored( itemColor, EE_ICON_CHECK );
                                HandleContextMenuOpening();
                                ImGuiX::TextTooltip( "Succeeded" );
                            }
                            break;

                            case CompilationRequest::Status::SucceededWithWarnings:
                            {
                                itemColor = Colors::Yellow.ToFloat4();
                                ImGui::TextColored( itemColor, EE_ICON_ALERT );
                                HandleContextMenuOpening();
                                ImGuiX::TextTooltip( "Succeeded with Warnings" );
                            }
                            break;

                            case CompilationRequest::Status::SucceededUpToDate:
                            {
                                itemColor = Colors::Lime.ToFloat4();
                                ImGui::TextColored( itemColor, EE_ICON_CLOCK_CHECK );
                                HandleContextMenuOpening();
                                ImGuiX::TextTooltip( "Up To Date" );
                            }
                            break;

                            case CompilationRequest::Status::Failed:
                            {
                                itemColor = Colors::Red.ToFloat4();
                                ImGui::TextColored( itemColor, EE_ICON_ALERT_OCTAGON );
                                HandleContextMenuOpening();
                                ImGuiX::TextTooltip( "Failed" );
                            }
                            break;

                            default:
                            break;
                        }

                        //-------------------------------------------------------------------------

                        auto const resourceTypeStr = pRequest->GetResourceID().GetResourceTypeID().ToString();
                        ImGui::TableNextColumn();
                        ImGui::Text( "%s", resourceTypeStr.c_str() );
                        HandleContextMenuOpening();

                        //-------------------------------------------------------------------------

                        ImGui::TableNextColumn();
                        ImGui::Text( pRequest->m_resourceID.c_str() );
                        HandleContextMenuOpening();

                        //-------------------------------------------------------------------------

                        ImGui::TableNextColumn();
                        ImGui::Text( "%.3fms", pRequest->GetCompilationElapsedTime().ToFloat() );
                        HandleContextMenuOpening();

                        //-------------------------------------------------------------------------

                        ImGui::TableNextColumn();
                        switch ( pRequest->m_origin )
                        {
                            case CompilationRequest::Origin::External :
                            {
                                ImGui::Text( "Client: %llu", pRequest->GetClientID() );
                            }
                            break;

                            case CompilationRequest::Origin::FileWatcher:
                            {
                                ImGui::Text( "File System Watcher" );
                            }
                            break;

                            case CompilationRequest::Origin::ManualCompile:
                            case CompilationRequest::Origin::ManualCompileForced:
                            {
                                ImGui::Text( "Manual" );
                            }
                            break;

                            case CompilationRequest::Origin::Package:
                            {
                                ImGui::Text( "Package" );
                            }
                            break;

                        }
                        HandleContextMenuOpening();

                        //-------------------------------------------------------------------------

                        ImGui::TableNextColumn();
                        ImGui::TextColored( itemColor, pRequest->GetSourceFilePath().c_str() );
                        HandleContextMenuOpening();

                        //-------------------------------------------------------------------------

                        ImGui::TableNextColumn();
                        bool const isItemSelected = ( pRequest == m_pSelectedRequest );
                        if ( ImGui::Selectable( pRequest->GetDestinationFilePath().c_str(), isItemSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, ImVec2( 0, 0 ) ) )
                        {
                            if ( pRequest->IsComplete() )
                            {
                                m_pSelectedRequest = pRequest;
                            }
                        }
                        HandleContextMenuOpening();

                        ImGui::PopID();
                    }
                }

                //-------------------------------------------------------------------------

                // Auto scroll the table
                if ( ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
                {
                    ImGui::SetScrollHereY( 1.0f );
                }

                //-------------------------------------------------------------------------

                // Context Menu
                ImGui::PushID( m_pContextMenuRequest );
                if ( ImGui::BeginPopup( "RequestContextMenu" ) )
                {
                    if ( ImGui::MenuItem( EE_ICON_CONTENT_COPY" Copy Compiler Args") )
                    {
                        String path( "-compile " );
                        path += m_pContextMenuRequest->GetCompilerArgs();
                        ImGui::SetClipboardText( path.c_str() );
                    }

                    if ( ImGui::MenuItem( EE_ICON_COG" Recompile" ) )
                    {
                        m_resourceServer.CompileResource( m_pContextMenuRequest->GetResourceID() );
                    }

                    if ( ImGui::MenuItem( EE_ICON_FILE" Go to Source File" ) )
                    {
                        Platform::Win32::OpenInExplorer( m_pContextMenuRequest->GetSourceFilePath() );
                    }

                    if ( ImGui::MenuItem( EE_ICON_FILE_CHECK" Go to Compiled File" ) )
                    {
                        Platform::Win32::OpenInExplorer( m_pContextMenuRequest->GetDestinationFilePath() );
                    }
                    ImGui::EndPopup();
                }

                if ( !ImGui::IsPopupOpen( "RequestContextMenu" ) )
                {
                    m_pContextMenuRequest = nullptr;
                }
                ImGui::PopID();

                //-------------------------------------------------------------------------

                ImGui::EndTable();
            }

            ImGui::Separator();

            //-------------------------------------------------------------------------
            // Info Panel
            //-------------------------------------------------------------------------

            char emptyBuffer[2] = { 0 };
            bool const hasSelectedItem = m_pSelectedRequest != nullptr;

            {
                ImGuiX::ScopedFont const scopedFont( ImGuiX::Font::Medium );

                if ( m_pSelectedRequest != nullptr )
                {
                    ImGui::SetNextItemWidth( textfieldWidth );
                    ImGui::InputText( "##Name", const_cast<char*>( m_pSelectedRequest->GetCompilerArgs() ), strlen( m_pSelectedRequest->GetCompilerArgs() ), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );
                }
                else
                {
                    ImGui::SetNextItemWidth( textfieldWidth );
                    ImGui::InputText( "##Name", emptyBuffer, 2, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );
                }

                ImGui::BeginDisabled( !hasSelectedItem );
                ImGui::SameLine();
                if( ImGui::Button( EE_ICON_CONTENT_COPY " Copy Args", ImVec2( buttonWidth, 0 ) ) )
                {
                    String path( "-compile " );
                    path += m_pSelectedRequest->GetCompilerArgs();
                    ImGui::SetClipboardText( path.c_str() );
                }

                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_COG " Recompile", ImVec2( buttonWidth, 0 ) ) )
                {
                    m_resourceServer.CompileResource( m_pSelectedRequest->GetResourceID() );
                }

                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_FILE " Source File", ImVec2( buttonWidth, 0 ) ) )
                {
                    Platform::Win32::OpenInExplorer( m_pSelectedRequest->GetSourceFilePath() );
                }

                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_FILE_CHECK " Compiled File", ImVec2( buttonWidth, 0 ) ) )
                {
                    Platform::Win32::OpenInExplorer( m_pSelectedRequest->GetDestinationFilePath() );
                }

                ImGui::EndDisabled();

                ImGui::SameLine();
                ImGui::BeginDisabled( m_resourceServer.IsBusy() );
                if ( ImGui::Button( EE_ICON_DELETE " Clear History", ImVec2( buttonWidth, 0 ) ) )
                {
                    m_resourceServer.CleanHistory();
                }
                ImGui::EndDisabled();
            }

            //-------------------------------------------------------------------------

            if ( m_pSelectedRequest != nullptr )
            {
                ImGui::InputTextMultiline( "##Output", const_cast<char*>( m_pSelectedRequest->GetLog() ), strlen( m_pSelectedRequest->GetLog() ), ImVec2( ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x, compilationLogFieldHeight ), ImGuiInputTextFlags_ReadOnly );
            }
            else
            {
                ImGui::InputTextMultiline( "##Output", emptyBuffer, 255, ImVec2( ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x, compilationLogFieldHeight ), ImGuiInputTextFlags_ReadOnly );
            }
        }
        ImGui::End();
    }

    void ResourceServerUI::DrawServerControls()
    {
        if ( ImGui::Begin( g_serverControlsWindowName ) )
        {
            ImGui::SeparatorText( "Info" );

            ImGui::Text( "Raw Resource Path: %s", m_resourceServer.GetRawResourceDir().c_str() );
            ImGui::Text( "Compiled Resource Path: %s", m_resourceServer.GetCompiledResourceDir().c_str() );
            ImGui::Text( "IP Address: %s:%d", m_resourceServer.GetNetworkAddress().c_str(), m_resourceServer.GetNetworkPort() );

            //-------------------------------------------------------------------------

            ImGui::SeparatorText( "Tools" );

            ImVec2 buttonSize( 155, 0 );
            float const fieldWidth = ImGui::GetContentRegionAvail().x - buttonSize.x - ImGui::GetStyle().ItemSpacing.x - 28;

            ImGui::SetNextItemWidth( fieldWidth );
            ImGui::InputText( "##Path", m_resourcePathbuffer, 255 );

            ImGui::SameLine();
            ImGui::Checkbox( "##ForceRecompile", &m_forceRecompilation );
            ImGuiX::TextTooltip( "Force Recompilation" );

            ImGui::BeginDisabled( !ResourcePath::IsValidPath( m_resourcePathbuffer ) );
            ImGui::SameLine();
            if ( ImGui::Button( "Request Compilation", buttonSize ) )
            {
                m_resourceServer.CompileResource( ResourceID( m_resourcePathbuffer ), m_forceRecompilation );
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            ImGui::SeparatorText( "Registered Compilers" );

            if ( ImGui::BeginTable( "Registered Compilers Table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg ) )
            {
                ImGui::SeparatorText( "Info" );
                ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 150 );
                ImGui::TableSetupColumn( "Ver", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 25 );
                ImGui::TableSetupColumn( "Output Types", ImGuiTableColumnFlags_WidthStretch );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                auto const pCompilerRegistry = m_resourceServer.GetCompilerRegistry();

                for ( auto const& pCompiler : pCompilerRegistry->GetRegisteredCompilers() )
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::Text( pCompiler->GetName().c_str() );

                    ImGui::TableSetColumnIndex( 1 );
                    ImGui::Text( "%d", pCompiler->GetVersion() );

                    //-------------------------------------------------------------------------

                    char str[32] = { 0 };

                    ImGui::TableSetColumnIndex( 2 );
                    for ( auto const& type : pCompiler->GetOutputTypes() )
                    {
                        type.GetString( str );
                        ImGui::Text( str );
                        ImGui::SameLine();
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void ResourceServerUI::DrawConnectionInfo()
    {
        if ( ImGui::Begin( g_connectionInfoWindowName ) )
        {
            if ( ImGui::BeginTable( "ConnectionInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg ) )
            {
                ImGui::TableSetupColumn( "Client ID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 150 );
                ImGui::TableSetupColumn( "IP", ImGuiTableColumnFlags_WidthStretch );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                int32_t const numConnectedClients = m_resourceServer.GetNumConnectedClients();
                for ( int32_t i = 0; i < numConnectedClients; i++ )
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::Text( "%u", m_resourceServer.GetClientID( i ) );

                    ImGui::TableSetColumnIndex( 1 );
                    ImGui::Text( m_resourceServer.GetConnectedClientAddress( i ).c_str() );
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void ResourceServerUI::DrawPackagingControls()
    {
        if ( ImGui::Begin( g_packagingControlsWindowName ) )
        {
            auto const packagingStage = m_resourceServer.GetPackagingStage();

            //-------------------------------------------------------------------------
            // Packaging UI
            //-------------------------------------------------------------------------

            bool const disablePackagingUI = ( packagingStage == ResourceServer::PackagingStage::Preparing ) || ( packagingStage == ResourceServer::PackagingStage::Packaging );
            ImGui::BeginDisabled( disablePackagingUI );
            {
                InlineString previewStr;
                auto const& mapsToBePackaged = m_resourceServer.GetMapsQueuedForPackaging();
                for ( auto const& mapID : mapsToBePackaged )
                {
                    if ( !previewStr.empty() )
                    {
                        previewStr.append( ", " );
                    }
                    previewStr.append( mapID.GetFileNameWithoutExtension().c_str() );
                }

                if ( previewStr.empty() )
                {
                    previewStr = "Nothing To Package";
                }

                //-------------------------------------------------------------------------

                ImVec2 const dimensions = ImGui::GetContentRegionAvail();

                auto const& allMaps = m_resourceServer.GetAllFoundMaps();
                constexpr static float const buttonSize = 28;
                ImGui::SetNextItemWidth( dimensions.x  - 50 - buttonSize - ( ImGui::GetStyle().ItemSpacing.x * 2 ) );
                {
                    if ( ImGui::BeginCombo( "##SelectedMaps", previewStr.c_str(), ImGuiComboFlags_HeightLargest ) )
                    {
                        for ( auto const& mapID : allMaps )
                        {
                            ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );
                            bool isSelected = VectorContains( mapsToBePackaged, mapID );
                            if ( ImGui::Checkbox( mapID.c_str(), &isSelected ) )
                            {
                                if ( isSelected )
                                {
                                    m_resourceServer.AddMapToPackagingList( mapID );
                                }
                                else
                                {
                                    m_resourceServer.RemoveMapFromPackagingList( mapID );
                                }
                            }
                        }

                        ImGui::EndCombo();
                    }
                }
                ImGuiX::ItemTooltip( "Select maps to package..." );

                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_REFRESH"##RefreshMaps", ImVec2( buttonSize, 0 ) ) )
                {
                    m_resourceServer.RefreshAvailableMapList();
                }
                ImGuiX::ItemTooltip( "Refresh Map List" );

                //-------------------------------------------------------------------------

                ImGui::SameLine();
                ImGui::BeginDisabled( !m_resourceServer.CanStartPackaging() );
                if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, "Start", ImVec2( 50, 0 ) ) )
                {
                    m_resourceServer.StartPackaging();
                }
                ImGui::EndDisabled();
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------
            // Packaging Progress
            //-------------------------------------------------------------------------

            if ( packagingStage != ResourceServer::PackagingStage::None )
            {
                ImGui::SeparatorText( "Selected Maps");

                auto const& mapsToBePackaged = m_resourceServer.GetMapsQueuedForPackaging();
                for ( auto const& mapID : mapsToBePackaged )
                {
                    ImGui::BulletText( mapID.c_str() );
                }

                ImGui::SeparatorText( "Progress" );

                if ( packagingStage != ResourceServer::PackagingStage::Complete )
                {
                    ImGui::Indent( 4.0f );
                    ImGuiX::DrawSpinner( "##Packaging" );
                    ImGui::Unindent( 4.0f );
                }
                else
                {
                    ImGuiX::ScopedFont const sf( ImGuiX::Font::Medium, Colors::Lime );
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( EE_ICON_CHECK_BOLD );
                }

                ImGui::SameLine( 26 );

                float const progress = m_resourceServer.GetPackagingProgress();
                TInlineString<32> overlay( TInlineString<32>::CtorSprintf(), "%.2f%%", progress * 100 );
                ImGui::ProgressBar( progress, ImVec2( -1, 0 ), overlay.c_str() );
            }
        }
        ImGui::End();
    }
}