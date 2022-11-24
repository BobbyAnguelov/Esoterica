#include "ResourceServerUI.h"
#include "ResourceServer.h"
#include "System/Imgui/ImguiX.h"
#include "System/Imgui/ImguiStyle.h"
#include "System/Types/Color.h"
#include "System/Platform/PlatformHelpers_Win32.h"
#include <eastl/sort.h>

//-------------------------------------------------------------------------

namespace EE::Resource
{
    static char const* const g_completedRequestsWindowName = "Completed Requests";
    static char const* const g_serverControlsWindowName = "Server";
    static char const* const g_connectionInfoWindowName = "Connected Clients";
    static char const* const g_packagingControlsWindowName = "Packaging";

    //-------------------------------------------------------------------------

    ResourceServerUI::ResourceServerUI( ResourceServer& resourceServer )
        : m_resourceServer( resourceServer )
    {}

    void ResourceServerUI::Draw()
    {
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
                ImGui::TableSetupColumn( "Client", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 110 );
                ImGui::TableSetupColumn( "Origin", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "Destination", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30 );
                ImGui::TableSetupColumn( "Up To Date", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 60 );
                ImGui::TableSetupColumn( "Compile", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 60 );
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

                        ImGui::TableSetColumnIndex( 0 );
                        switch ( pRequest->GetStatus() )
                        {
                            case CompilationRequest::Status::Pending:
                            {
                                itemColor = Colors::LightGray.ToFloat4();
                                ImGui::TextColored( itemColor, EE_ICON_CLOCK );
                                ImGuiX::TextTooltip( "Pending" );
                            }
                            break;

                            case CompilationRequest::Status::Compiling:
                            {
                                itemColor = Colors::Cyan.ToFloat4();
                                ImGui::TextColored( itemColor, EE_ICON_COG );
                                ImGuiX::TextTooltip( "Compiling" );
                            }
                            break;

                            case CompilationRequest::Status::Succeeded:
                            {
                                itemColor = Colors::Lime.ToFloat4();
                                ImGui::TextColored( itemColor, EE_ICON_CHECK );
                                ImGuiX::TextTooltip( "Succeeded" );
                            }
                            break;

                            case CompilationRequest::Status::SucceededWithWarnings:
                            {
                                itemColor = Colors::Yellow.ToFloat4();
                                ImGui::TextColored( itemColor, EE_ICON_ALERT );
                                ImGuiX::TextTooltip( "Succeeded with Warnings" );
                            }
                            break;

                            case CompilationRequest::Status::SucceededUpToDate:
                            {
                                itemColor = Colors::Lime.ToFloat4();
                                ImGui::TextColored( itemColor, EE_ICON_CLOCK_CHECK );
                                ImGuiX::TextTooltip( "Up To Date" );
                            }
                            break;

                            case CompilationRequest::Status::Failed:
                            {
                                itemColor = Colors::Red.ToFloat4();
                                ImGui::TextColored( itemColor, EE_ICON_ALERT_OCTAGON );
                                ImGuiX::TextTooltip( "Failed" );
                            }
                            break;

                            default:
                            break;
                        }

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 1 );
                        switch ( pRequest->m_origin )
                        {
                            case CompilationRequest::Origin::External :
                            {
                                ImGui::Text( "%llu", pRequest->GetClientID() );
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
                                ImGui::Text( "Packaging" );
                            }
                            break;
                        }

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 2 );
                        ImGui::TextColored( itemColor, pRequest->GetSourceFilePath().c_str() );

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 3 );
                        bool const isItemSelected = ( pRequest == m_pSelectedCompletedRequest );
                        if ( ImGui::Selectable( pRequest->GetDestinationFilePath().c_str(), isItemSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap, ImVec2( 0, 0 ) ) )
                        {
                            if ( pRequest->IsComplete() )
                            {
                                m_pSelectedCompletedRequest = pRequest;
                            }
                        }

                        //-------------------------------------------------------------------------

                        auto const resourceTypeStr = pRequest->GetResourceID().GetResourceTypeID().ToString();
                        ImGui::TableSetColumnIndex( 4 );
                        ImGui::Text( "%s", resourceTypeStr.c_str() );

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 5 );
                        ImGui::Text( "%.3fms", pRequest->GetUptoDateCheckElapsedTime().ToFloat() );

                        ImGui::TableSetColumnIndex( 6 );
                        ImGui::Text( "%.3fms", pRequest->GetCompilationElapsedTime().ToFloat() );

                        ImGui::PopID();
                    }
                }

                // Auto scroll the table
                if ( ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
                {
                    ImGui::SetScrollHereY( 1.0f );
                }

                ImGui::EndTable();
            }

            ImGui::Separator();

            //-------------------------------------------------------------------------
            // Info Panel
            //-------------------------------------------------------------------------

            char emptyBuffer[2] = { 0 };
            bool const hasSelectedItem = m_pSelectedCompletedRequest != nullptr;

            {
                ImGuiX::ScopedFont const scopedFont( ImGuiX::Font::Medium );

                if ( m_pSelectedCompletedRequest != nullptr )
                {
                    ImGui::SetNextItemWidth( textfieldWidth );
                    ImGui::InputText( "##Name", const_cast<char*>( m_pSelectedCompletedRequest->GetCompilerArgs() ), strlen( m_pSelectedCompletedRequest->GetCompilerArgs() ), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );
                }
                else
                {
                    ImGui::SetNextItemWidth( textfieldWidth );
                    ImGui::InputText( "##Name", emptyBuffer, 2, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );
                }

                ImGui::BeginDisabled( !hasSelectedItem );
                ImGui::SameLine();
                if( ImGui::Button( EE_ICON_CONTENT_COPY "Copy Args", ImVec2( buttonWidth, 0 ) ) )
                {
                    String path( "-compile " );
                    path += m_pSelectedCompletedRequest->GetCompilerArgs();
                    ImGui::SetClipboardText( path.c_str() );
                }

                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_COG "Recompile", ImVec2( buttonWidth, 0 ) ) )
                {
                    m_resourceServer.CompileResource( m_pSelectedCompletedRequest->GetResourceID() );
                }

                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_FILE "Source File", ImVec2( buttonWidth, 0 ) ) )
                {
                    Platform::Win32::OpenInExplorer( m_pSelectedCompletedRequest->GetSourceFilePath() );
                }

                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_FILE_CHECK "Compiled File", ImVec2( buttonWidth, 0 ) ) )
                {
                    Platform::Win32::OpenInExplorer( m_pSelectedCompletedRequest->GetDestinationFilePath() );
                }

                ImGui::EndDisabled();

                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_DELETE "Clear History", ImVec2( buttonWidth, 0 ) ) )
                {
                    m_resourceServer.CleanHistory();
                }
            }

            //-------------------------------------------------------------------------

            if ( m_pSelectedCompletedRequest != nullptr )
            {
                ImGui::InputTextMultiline( "##Output", const_cast<char*>( m_pSelectedCompletedRequest->GetLog() ), strlen( m_pSelectedCompletedRequest->GetLog() ), ImVec2( ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x, compilationLogFieldHeight ), ImGuiInputTextFlags_ReadOnly );
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
            ImGuiX::TextSeparator( "Info" );

            ImGui::Text( "Raw Resource Path: %s", m_resourceServer.GetRawResourceDir().c_str() );
            ImGui::Text( "Compiled Resource Path: %s", m_resourceServer.GetCompiledResourceDir().c_str() );
            ImGui::Text( "IP Address: %s:%d", m_resourceServer.GetNetworkAddress().c_str(), m_resourceServer.GetNetworkPort() );

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Tools" );

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

            ImGuiX::TextSeparator( "Registered Compilers" );

            if ( ImGui::BeginTable( "Registered Compilers Table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg ) )
            {
                ImGuiX::TextSeparator( "Info" );
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
            if ( m_resourceServer.IsPackaging() )
            {
                ImGui::Text( "Packaging in Progress:" );

                float const progress = m_resourceServer.GetPackagingProgress();
                ImGui::ProgressBar( progress, ImVec2( -1, 0 ) );

                ImGui::NewLine();
                ImGui::Text( "Maps being packaged:" );

                auto const& mapsToBePackaged = m_resourceServer.GetMapsQueuedForPackaging();
                for ( auto const& mapID : mapsToBePackaged )
                {
                    ImGui::BulletText( mapID.c_str() );
                }
            }
            else
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
                ImGui::SetNextItemWidth( dimensions.x - buttonSize - ImGui::GetStyle().ItemSpacing.x );
                {
                    ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );
                    if ( ImGui::BeginCombo( "##SelectedMaps", previewStr.c_str(), ImGuiComboFlags_HeightLargest) )
                    {
                        for ( auto const& mapID : allMaps )
                        {
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

                ImGui::BeginDisabled( !m_resourceServer.CanStartPackaging() );
                if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, "Package Selected Maps", ImVec2( -1, 0 ) ) )
                {
                    m_resourceServer.StartPackaging();
                }
                ImGui::EndDisabled();
            }
        }
        ImGui::End();
    }
}