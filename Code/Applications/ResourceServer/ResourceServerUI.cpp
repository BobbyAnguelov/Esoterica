#include "ResourceServerUI.h"
#include "ResourceServer.h"
#include "ResourceServerRequest.h"
#include "EngineTools/Widgets/Pickers/DataPathPicker.h"
#include "EngineTools/Core/ToolsEmbeddedResources.inl"
#include "EngineTools/Core/SystemDialogs.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include "Base/TypeSystem/ResourceInfo.h"
#include "Base/Profiling.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    static char const* const g_requestsWindowName = "Requests";
    static char const* const g_serverInfoWindowName = "Server";
    static char const* const g_clientInfoWindowName = "Clients";
    static char const* const g_compilersWindowName = "Compilers";
    static char const* const g_packagingControlsWindowName = "Packaging";
    static char const* const g_recompilationBlockersWindowName = "Recompilation Blockers";

    //-------------------------------------------------------------------------

    ResourceServerUI::ResourceServerUI( ResourceServer& resourceServer )
        : m_resourceServer( resourceServer )
        , m_resourcePathBuffer( 512 )
        , m_compilationLogBuffer( 1024 )
    {
        m_requestsUpdatedEventBindingID = m_resourceServer.OnRequestsUpdated().Bind( [this] () { m_requestsUpdated = true; } );
    }

    ResourceServerUI::~ResourceServerUI()
    {
        EE_ASSERT( !m_resourceServerIcon.IsValid() );
        EE_ASSERT( m_pRecompilationBlockerPathPicker == nullptr );

        m_resourceServer.OnRequestsUpdated().Unbind( m_requestsUpdatedEventBindingID );
    }

    //-------------------------------------------------------------------------

    void ResourceServerUI::Initialize( ImGuiX::ImageCache* pImageCache )
    {
        EE_ASSERT( m_pImageCache == nullptr && pImageCache->WasInitialized() );
        m_pImageCache = pImageCache;
        m_resourceServerIcon = m_pImageCache->LoadImageFromMemoryBase64( g_iconDataPurple, 3332 );

        m_pCompileResourcePathPicker = EE::New<DataPathPicker>( m_resourceServer.GetSourceDataDirectory() );
        m_pCompileResourcePathPicker->SetPickingMode( DataPathPicker::Mode::PickFiles );

        m_pRecompilationBlockerPathPicker = EE::New<DataPathPicker>( m_resourceServer.GetSourceDataDirectory() );
        m_pRecompilationBlockerPathPicker->SetPickingMode( DataPathPicker::Mode::PickDirectories );
    }

    void ResourceServerUI::Shutdown()
    {
        m_pImageCache->UnloadImage( m_resourceServerIcon );

        EE::Delete( m_pRecompilationBlockerPathPicker );
        EE::Delete( m_pCompileResourcePathPicker );
    }

    void ResourceServerUI::SetupDockingLayout( ImGuiID dockspaceID )
    {
        ImGuiID leftDockID = 0;
        ImGuiID rightDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Right, 0.7f, nullptr, &leftDockID );
        ImGuiID bottomLeftDockID = ImGui::DockBuilderSplitNode( leftDockID, ImGuiDir_Down, 0.33f, nullptr, &leftDockID );

        ImGui::DockBuilderDockWindow( g_requestsWindowName, rightDockID );

        ImGui::DockBuilderDockWindow( g_serverInfoWindowName, leftDockID );
        ImGui::DockBuilderDockWindow( g_clientInfoWindowName, leftDockID );
        ImGui::DockBuilderDockWindow( g_compilersWindowName, leftDockID );

        ImGui::DockBuilderDockWindow( g_packagingControlsWindowName, bottomLeftDockID );
        ImGui::DockBuilderDockWindow( g_recompilationBlockersWindowName, bottomLeftDockID );
    }

    void ResourceServerUI::Draw()
    {
        // Window title bar
        //-------------------------------------------------------------------------

        auto DrawTitleBarContents = [this] ()
        {
            DrawTitleBarMenu();
        };

        m_titleBar.Draw( DrawTitleBarContents, 400 );

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
        ImGui::Begin( "ResourceServer", nullptr, windowFlags );
        {
            ImGuiID const dockspaceID = ImGui::GetID( "ResourceServerDock" );

            // Initial Layout
            if ( m_requiresLayoutReset || ImGui::DockBuilderGetNode( dockspaceID ) == nullptr )
            {
                ImGui::DockBuilderAddNode( dockspaceID, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton );
                ImGui::DockBuilderSetNodeSize( dockspaceID, viewport->Size );
                SetupDockingLayout( dockspaceID );
                ImGui::DockBuilderFinish( dockspaceID );

                m_requiresLayoutReset = false;
            }

            ImGui::DockSpace( dockspaceID, ImVec2( 0.0f, 0.0f ), ImGuiDockNodeFlags_None );
        }
        ImGui::PopStyleVar( 3 );
        ImGui::End();

        // Draw windows
        //-------------------------------------------------------------------------

        DrawServerInfoWindow();
        DrawClientInfoWindow();
        DrawCompilerInfoWindow();
        DrawCompilationRequestsWindow();
        DrawPackagingWindow();
        DrawRecompilationBlockersWindow();

        // Draw any open dialogs
        //-------------------------------------------------------------------------

        m_dialogManager.DrawDialog();
    }

    //-------------------------------------------------------------------------

    void ResourceServerUI::SetSelectedRequest( Request const* pRequest )
    {
        if ( !m_allowSelectionChange )
        {
            return;
        }

        if ( m_pSelectedRequest == pRequest )
        {
            return;
        }

        m_pSelectedRequest = pRequest;
        m_allowSelectionChange = false;
        m_compilationLogBufferFilled = false;

        // Set log
        //-------------------------------------------------------------------------

        if ( m_pSelectedRequest != nullptr )
        {
            if ( m_pSelectedRequest->IsComplete() )
            {
                m_compilationLogBuffer.Fill( m_pSelectedRequest->m_log );
                m_compilationLogBufferFilled = true;
            }
            else
            {
                m_compilationLogBuffer.Fill( "In Progress..." );
            }
        }
        else
        {
            m_compilationLogBuffer.Clear();
        }
    }

    void ResourceServerUI::UpdateRequestsList( int32_t sortColumnIdx, bool sortAscending )
    {
        m_requests = m_resourceServer.GetRequests();
        if ( m_requestsFilter.HasFilterSet() )
        {
            for ( int32_t i = 0; i < (int32_t) m_requests.size(); i++ )
            {
                if ( !m_requestsFilter.MatchesFilter( m_requests[i]->m_resourceID.c_str() ) )
                {
                    m_requests.erase( m_requests.begin() + i );
                    i--;
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( sortColumnIdx == 0 )
        {
            auto SortPredicate = [sortAscending] ( Request const* pA, Request const* pB )
            {
                if ( sortAscending )
                {
                    return pA->m_timeRequested < pB->m_timeRequested;
                }
                else
                {
                    return pA->m_timeRequested > pB->m_timeRequested;
                }
            };

            eastl::sort( m_requests.begin(), m_requests.end(), SortPredicate );
        }
        else if ( sortColumnIdx == 1 )
        {
            auto SortPredicate = [sortAscending] ( Request const* pA, Request const* pB )
            {
                if ( sortAscending )
                {
                    return pA->m_status < pB->m_status;
                }
                else
                {
                    return pA->m_status > pB->m_status;
                }
            };

            eastl::sort( m_requests.begin(), m_requests.end(), SortPredicate );
        }
        else if ( sortColumnIdx == 2 )
        {
            auto SortPredicate = [sortAscending] ( Request const* pA, Request const* pB )
            {
                if ( sortAscending )
                {
                    return pA->m_origin < pB->m_origin;
                }
                else
                {
                    return pA->m_origin > pB->m_origin;
                }
            };

            eastl::sort( m_requests.begin(), m_requests.end(), SortPredicate );
        }
        else if ( sortColumnIdx == 3 )
        {
            EE_UNREACHABLE_CODE();
        }
        else if ( sortColumnIdx == 4 )
        {
            auto SortPredicate = [sortAscending] ( Request const* pA, Request const* pB )
            {
                if ( sortAscending )
                {
                    return pA->m_resourceID.GetResourceTypeID().ToString() < pB->m_resourceID.GetResourceTypeID().ToString();
                }
                else
                {
                    return pA->m_resourceID.GetResourceTypeID().ToString() > pB->m_resourceID.GetResourceTypeID().ToString();
                }
            };

            eastl::sort( m_requests.begin(), m_requests.end(), SortPredicate );
        }
        else if ( sortColumnIdx == 5 )
        {
            auto SortPredicate = [sortAscending] ( Request const* pA, Request const* pB )
            {
                if ( sortAscending )
                {
                    return pA->m_resourceID.GetString() < pB->m_resourceID.GetString();
                }
                else
                {
                    return pA->m_resourceID.GetString() > pB->m_resourceID.GetString();
                }
            };

            eastl::sort( m_requests.begin(), m_requests.end(), SortPredicate );
        }
        else if ( sortColumnIdx == 6 )
        {
            auto SortPredicate = [sortAscending] ( Request const* pA, Request const* pB )
            {
                if ( sortAscending )
                {
                    return pA->m_destinationFile.GetString() < pB->m_destinationFile.GetString();
                }
                else
                {
                    return pA->m_destinationFile.GetString() > pB->m_destinationFile.GetString();
                }
            };

            eastl::sort( m_requests.begin(), m_requests.end(), SortPredicate );
        }
        else if ( sortColumnIdx == 7 )
        {
            EE_UNREACHABLE_CODE();
        }
        else if ( sortColumnIdx == 8 )
        {
            auto SortPredicate = [sortAscending] ( Request const* pA, Request const* pB )
            {
                if ( sortAscending )
                {
                    return pA->m_upToDateCheckTime < pB->m_upToDateCheckTime;
                }
                else
                {
                    return pA->m_upToDateCheckTime > pB->m_upToDateCheckTime;
                }
            };

            eastl::sort( m_requests.begin(), m_requests.end(), SortPredicate );
        }
        else if ( sortColumnIdx == 9 )
        {
            auto SortPredicate = [sortAscending] ( Request const* pA, Request const* pB )
            {
                if ( sortAscending )
                {
                    return pA->m_compileTime < pB->m_compileTime;
                }
                else
                {
                    return pA->m_compileTime > pB->m_compileTime;
                }
            };

            eastl::sort( m_requests.begin(), m_requests.end(), SortPredicate );
        }
        else if ( sortColumnIdx == 10 )
        {
            auto SortPredicate = [sortAscending] ( Request const* pA, Request const* pB )
            {
                if ( sortAscending )
                {
                    return pA->GetCompletionTime() < pB->GetCompletionTime();
                }
                else
                {
                    return pA->GetCompletionTime() > pB->GetCompletionTime();
                }
            };

            eastl::sort( m_requests.begin(), m_requests.end(), SortPredicate );
        }
    }

    //-------------------------------------------------------------------------

    void ResourceServerUI::GetBorderlessTitleBarInfo( Math::ScreenSpaceRectangle& outTitlebarRect, bool& isInteractibleWidgetHovered ) const
    {
        outTitlebarRect = m_titleBar.GetScreenRectangle();
        isInteractibleWidgetHovered = ImGui::IsAnyItemHovered();
    }

    void ResourceServerUI::DrawTitleBarMenu()
    {
        ImGui::Image( m_resourceServerIcon.m_ID, m_resourceServerIcon.m_size );
        ImGuiX::TextTooltip( "Esoterica Resource Server" );

        //-------------------------------------------------------------------------

        constexpr float const verticalOffset = -4;

        ImGui::SameLine();
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + verticalOffset );

        if ( ImGui::BeginMenu( "Tools" ) )
        {
            if ( ImGui::MenuItem( EE_ICON_DOCK_WINDOW" Reset Layout" ) )
            {
                m_requiresLayoutReset = true;
            }

            ImGui::SeparatorText( "Data" );

            if ( ImGui::MenuItem( EE_ICON_CONTENT_SAVE_ALL" Resave All Data Files", nullptr, false, !m_resourceServer.IsResavingDataFiles() ) )
            {
                m_resourceServer.RequestResaveOfDataFiles();
                m_dialogManager.StartModalDialog( "Resave Progress", [this] () { return DrawResaveProgressDialog(); }, ImVec2( 600, -1 ), false, false );
            }

            ImGui::BeginDisabled( m_resourceServer.IsBusy() );
            if ( ImGui::MenuItem( EE_ICON_TRASH_CAN" Delete Compiled Resource Database" ) )
            {
                if ( MessageDialog::Confirmation( Severity::Warning, "Confirm Deletion", "Are you sure you want to delete the compiled resource DB. This will cause all resources to recompile!" ) )
                {
                    m_resourceServer.DeleteCompiledResourceDatabase();
                }
            }
            ImGui::EndDisabled();

            ImGui::SeparatorText( "Profiling" );

            if ( m_isProfiling )
            {
                if ( ImGui::MenuItem( EE_ICON_STOP" Stop Profiling" ) )
                {
                    Profiling::StopCapture( "D:\\res.opt" );
                    m_isProfiling = false;
                }
            }
            else
            {
                if ( ImGui::MenuItem( EE_ICON_RECORD" Start Profiling" ) )
                {
                    Profiling::StartCapture();
                    m_isProfiling = true;
                }
            }

            ImGui::EndMenu();
        }
    }

    void ResourceServerUI::DrawCompilationRequestsWindow()
    {
        m_allowSelectionChange = true;

        if ( ImGui::Begin( g_requestsWindowName ) )
        {
            ImVec2 const itemSpacing = ImGui::GetStyle().ItemSpacing;

            //-------------------------------------------------------------------------
            // Filter
            //-------------------------------------------------------------------------

            constexpr static float const clearButtonWidth = 250;
            float const filterWidth = ImGui::GetContentRegionAvail().x - ( clearButtonWidth + itemSpacing.x );

            if ( m_requestsFilter.UpdateAndDraw( filterWidth ) )
            {
                m_requestsUpdated = true;
            }

            ImGui::SameLine();
            ImGui::BeginDisabled( m_resourceServer.IsBusy() );
            if ( ImGuiX::IconButton( EE_ICON_DELETE, "Clear Completed Requests", Colors::Red, ImVec2( clearButtonWidth, 0 ) ) )
            {
                m_resourceServer.CleanHistory();
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------
            // Table
            //-------------------------------------------------------------------------

            ImGui::SetNextWindowSize( ImGui::GetContentRegionAvail() - ImVec2( 0, 125 ), ImGuiCond_Once );
            ImGui::SetNextWindowSizeConstraints( ImVec2( 100, 100 ), ImVec2( FLT_MAX, ImGui::GetContentRegionAvail().y - 125 ) );
            if ( ImGui::BeginChild( "RequestsTable", ImVec2( 0, 0 ), ImGuiChildFlags_ResizeY, 0 ) )
            {
                ImGuiX::ScopedFont tsf( ImGuiX::Font::Small );

                ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 4 ) );
                if ( ImGui::BeginTable( "Requests", 11, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable, ImVec2( 0, ImGui::GetContentRegionAvail().y ) ) )
                {
                    ImGui::TableSetupColumn( "##Timestamp", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 60 );
                    ImGui::TableSetupColumn( "##Status", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 24 );
                    ImGui::TableSetupColumn( "##Origin", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 24 );
                    ImGui::TableSetupColumn( "##Info", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 24 );
                    ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 85 );
                    ImGui::TableSetupColumn( "ID", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupColumn( "Destination", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupColumn( EE_ICON_HEART, ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40 );
                    ImGui::TableSetupColumn( "UTD", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 60 );
                    ImGui::TableSetupColumn( "Compile", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 80 );
                    ImGui::TableSetupColumn( "Time", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 60 );
                    ImGui::TableSetupScrollFreeze( 0, 1 );

                    // Update Requests List
                    //-------------------------------------------------------------------------

                    int32_t sortedColumnIdx = InvalidIndex;
                    bool sortAscending = false;

                    if ( ImGuiTableSortSpecs* pSortSpecs = ImGui::TableGetSortSpecs() )
                    {
                        if ( pSortSpecs->SpecsDirty )
                        {
                            sortedColumnIdx = pSortSpecs->Specs[0].ColumnIndex;
                            sortAscending = pSortSpecs->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
                            m_requestsUpdated = true;

                            pSortSpecs->SpecsDirty = false;
                        }
                    }

                    if ( m_requestsUpdated )
                    {
                        UpdateRequestsList( sortedColumnIdx, sortAscending );
                        m_requestsUpdated = false;
                    }

                    // Draw Request rows
                    //-------------------------------------------------------------------------

                    Milliseconds const currentTime = PlatformClock::GetTimeInMilliseconds();

                    ImGui::TableHeadersRow();

                    ImGuiListClipper clipper;
                    clipper.Begin( (int32_t) m_requests.size() );
                    while ( clipper.Step() )
                    {
                        for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                        {
                            Request const* pRequest = m_requests[i];
                            ImGui::PushID( pRequest );

                            ImVec4 itemColor;
                            ImGui::TableNextRow();

                            //-------------------------------------------------------------------------

                            bool isItemSelected = ( pRequest == m_pSelectedRequest );

                            // Time stamp
                            //-------------------------------------------------------------------------

                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();

                            ImGui::Text( pRequest->m_timestamp.c_str() );

                            // Status
                            //-------------------------------------------------------------------------

                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();

                            switch ( pRequest->GetStatus() )
                            {
                                case RequestStatus::Pending:
                                {
                                    {
                                        ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                                        ImGui::TextColored( Colors::LightGray, EE_ICON_CLOCK );
                                    }
                                    ImGuiX::TextTooltip( "Pending" );
                                }
                                break;

                                case RequestStatus::Compiling:
                                {
                                    ImGui::SetCursorPosX( ImGui::GetCursorPosX() - ImGui::GetStyle().CellPadding.x );
                                    ImGuiX::DrawSpinner( "##Compiling", Colors::Cyan );
                                    ImGuiX::TextTooltip( "Compiling" );
                                }
                                break;

                                case RequestStatus::Succeeded:
                                {
                                    {
                                        ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                                        ImGui::TextColored( Colors::Lime, EE_ICON_CHECK );
                                    }
                                    ImGuiX::TextTooltip( "Succeeded" );
                                }
                                break;

                                case RequestStatus::SucceededWithWarnings:
                                {
                                    {
                                        ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                                        ImGui::TextColored( Colors::Yellow, EE_ICON_ALERT );
                                    }
                                    ImGuiX::TextTooltip( "Succeeded with Warnings" );
                                }
                                break;

                                case RequestStatus::SucceededUpToDate:
                                {
                                    {
                                        ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                                        ImGui::TextColored( Colors::Lime, EE_ICON_CLOCK_CHECK );
                                    }
                                    ImGuiX::TextTooltip( "Up To Date" );
                                }
                                break;

                                case RequestStatus::Failed:
                                {
                                    {
                                        ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                                        ImGui::TextColored( Colors::Red, EE_ICON_CLOSE );
                                    }
                                    ImGuiX::TextTooltip( "Failed" );
                                }
                                break;

                                default:
                                break;
                            }

                            // Origin
                            //-------------------------------------------------------------------------

                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();

                            switch ( pRequest->m_origin )
                            {
                                case RequestOrigin::Network:
                                {
                                    {
                                        ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                                        ImGui::Text( EE_ICON_NETWORK );
                                    }
                                    ImGui::SetItemTooltip( "Client: %llu", pRequest->m_clientID );
                                }
                                break;

                                case RequestOrigin::FileWatcher:
                                {
                                    {
                                        ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                                        ImGui::Text( EE_ICON_FILE_ALERT );
                                    }
                                    ImGui::SetItemTooltip( "File System Watcher" );
                                }
                                break;

                                case RequestOrigin::UnblockedRecompilationRequest:
                                {
                                    {
                                        ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                                        ImGui::Text( EE_ICON_LOCK_OPEN_VARIANT );
                                    }
                                    ImGui::SetItemTooltip( "Unblocked Recompilation" );
                                }
                                break;

                                case RequestOrigin::ManualCompile:
                                case RequestOrigin::ManualCompileForced:
                                {
                                    {
                                        ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                                        ImGui::Text( EE_ICON_ACCOUNT );
                                    }
                                    ImGui::SetItemTooltip( "Manual" );
                                }
                                break;

                                case RequestOrigin::Package:
                                {
                                    {
                                        ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                                        ImGui::Text( EE_ICON_PACKAGE_VARIANT );
                                    }
                                    ImGui::SetItemTooltip( "Package" );
                                }
                                break;
                            }

                            // Info
                            //-------------------------------------------------------------------------

                            ImGui::TableNextColumn();
                            if ( !pRequest->m_extraInfo.empty() )
                            {
                                {
                                    ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
                                    ImGui::Text( EE_ICON_INFORMATION_OUTLINE );
                                }
                                ImGui::SetItemTooltip( pRequest->m_extraInfo.c_str() );
                            }
                            else
                            {
                                {
                                    ImGuiX::ScopedFont sf( ImGuiX::Font::Large, ImGuiX::Style::s_colorTextDisabled );
                                    ImGui::Text( EE_ICON_MINUS_CIRCLE_OUTLINE );
                                }
                                ImGui::SetItemTooltip( "No extra info" );
                            }

                            // Type
                            //-------------------------------------------------------------------------

                            auto const resourceTypeStr = pRequest->m_resourceID.GetResourceTypeID().ToString();
                            ImGui::TableNextColumn();

                            if ( pRequest->m_pResourceInfo != nullptr )
                            {
                                ImGui::TextColored( pRequest->m_pResourceInfo->m_color.ToFloat4(), "%s", resourceTypeStr.c_str() );
                                ImGuiX::ItemTooltip( pRequest->m_pResourceInfo->m_friendlyName.c_str() );
                            }
                            else
                            {
                                ImGui::Text( "%s", resourceTypeStr.c_str() );
                            }

                            // Resource ID
                            //-------------------------------------------------------------------------

                            ImGui::TableNextColumn();

                            bool changeSelectionToThisRequest = false;

                            if ( ImGui::Selectable( pRequest->GetSourceFilePath().c_str(), isItemSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, ImVec2( 0, 0 ) ) )
                            {
                                changeSelectionToThisRequest = true;
                            }

                            if ( ImGui::IsItemHovered() && ImGui::IsMouseReleased( ImGuiMouseButton_Right ) )
                            {
                                changeSelectionToThisRequest = true;
                                ImGui::OpenPopup( "RequestContextMenu" );
                                m_pContextMenuRequest = pRequest;
                            }

                            if ( changeSelectionToThisRequest )
                            {
                                SetSelectedRequest( pRequest );
                                ImGui::SetKeyboardFocusHere( -1 );
                            }

                            // Destination
                            //-------------------------------------------------------------------------

                            ImGui::TableNextColumn();
                            {
                                ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );
                                ImGui::Text( pRequest->GetDestinationFilePath().c_str() );
                            }

                            // Timing
                            //-------------------------------------------------------------------------

                            ImGui::TableNextColumn();
                            if ( pRequest->IsComplete() )
                            {
                                ImGui::Text( "" );
                            }
                            else
                            {
                                ImGui::Text( "%.2fs", Seconds( currentTime - pRequest->m_lastSentHeartbeatTime ).ToFloat() );
                            }

                            ImGui::TableNextColumn();
                            ImGui::Text( "%.1fms", pRequest->m_upToDateCheckTime.ToFloat() );

                            ImGui::TableNextColumn();
                            ImGui::Text( "%.1fms", pRequest->m_compileTime.ToFloat() );

                            ImGui::TableNextColumn();
                            ImGui::Text( "%.2fs", pRequest->GetCompletionTime().ToSeconds().ToFloat() );

                            // Origin
                            //-------------------------------------------------------------------------

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
                        ImGui::SeparatorText( m_pContextMenuRequest->m_resourceID.GetFilenameWithoutExtension().c_str() );

                        if ( ImGui::MenuItem( EE_ICON_CONTENT_COPY" Copy Resource ID" ) )
                        {
                            ImGui::SetClipboardText( m_pContextMenuRequest->m_resourceID.c_str() );
                        }

                        if ( ImGui::MenuItem( EE_ICON_COG" Copy Compilation Args" ) )
                        {
                            InlineString str( InlineString::CtorSprintf(), "-compile %s", m_pContextMenuRequest->m_resourceID.c_str() );
                            ImGui::SetClipboardText( str.c_str() );
                        }

                        if ( ImGui::MenuItem( EE_ICON_COG" Recompile" ) )
                        {
                            m_resourceServer.CompileResource( m_pContextMenuRequest->m_resourceID );
                        }

                        ImGui::SeparatorText( "Source" );

                        if ( ImGui::MenuItem( EE_ICON_FILE" Copy Source Path" ) )
                        {
                            ImGui::SetClipboardText( m_pContextMenuRequest->GetSourceFilePath().c_str() );
                        }

                        if ( ImGui::MenuItem( EE_ICON_FILE" Go to Source File" ) )
                        {
                            Platform::Win32::OpenInExplorer( m_pContextMenuRequest->GetSourceFilePath() );
                        }

                        ImGui::SeparatorText( "Compiled" );

                        if ( ImGui::MenuItem( EE_ICON_FILE" Copy Compiled Path" ) )
                        {
                            ImGui::SetClipboardText( m_pContextMenuRequest->GetDestinationFilePath().c_str() );
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
                ImGui::PopStyleVar();
            }
            ImGui::EndChild();

            //-------------------------------------------------------------------------
            // Info Panel
            //-------------------------------------------------------------------------

            if ( ImGui::BeginChild( "LogWindow", ImGui::GetContentRegionAvail(), 0, 0 ) )
            {
                // Update buffer when complete
                char emptyBuffer[2] = { 0 };
                if ( m_pSelectedRequest != nullptr && m_pSelectedRequest->IsComplete() && !m_compilationLogBufferFilled )
                {
                    m_compilationLogBuffer.Fill( m_pSelectedRequest->m_log );
                    m_compilationLogBufferFilled = true;
                }

                ImGui::InputTextMultiline( "##Output", m_compilationLogBuffer.Data(), m_compilationLogBuffer.Size(), ImVec2( -1, ImGui::GetContentRegionAvail().y ), ImGuiInputTextFlags_ReadOnly );
            }
            ImGui::EndChild();

        }
        ImGui::End();
    }

    void ResourceServerUI::DrawServerInfoWindow()
    {
        if ( ImGui::Begin( g_serverInfoWindowName ) )
        {
            ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
            if ( ImGui::CollapsingHeader( "Server" ) )
            {
                {
                    ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                    ImGui::Text( EE_ICON_FILE_IMPORT" %s", m_resourceServer.GetSourceDataDirectory().c_str() );
                    ImGui::Text( EE_ICON_FILE_EXPORT_OUTLINE" %s", m_resourceServer.GetCompiledResourceDirectory().c_str() );
                    ImGui::Text( EE_ICON_IP_NETWORK" %s:%d", m_resourceServer.GetNetworkAddress().c_str(), m_resourceServer.GetNetworkPort() );
                }
            }

            //-------------------------------------------------------------------------

            ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
            if ( ImGui::CollapsingHeader( "Test Compile" ) )
            {
                m_pCompileResourcePathPicker->UpdateAndDraw();

                ImGui::Checkbox( "Force Recompile", &m_forceRecompilation );
                ImGuiX::TextTooltip( "Force Recompilation" );

                ImGui::BeginDisabled( !m_pCompileResourcePathPicker->GetPath().IsValid() );
                ImGui::SameLine( ImGui::GetContentRegionAvail().x - 200 );
                if ( ImGuiX::IconButtonColored( EE_ICON_COG, "Request Compilation", Colors::Green, Colors::White, Colors::White, ImVec2( -1, 0 ) ) )
                {
                    m_resourceServer.CompileResource( ResourceID( m_pCompileResourcePathPicker->GetPath() ), m_forceRecompilation );
                }
                ImGui::EndDisabled();
            }

            //-------------------------------------------------------------------------

            ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
            if ( ImGui::CollapsingHeader( "Connected Workers" ) )
            {
                if ( ImGui::BeginTable( "WorkerInfo", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg ) )
                {
                    ImGui::TableSetupColumn( "ID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30 );
                    ImGui::TableSetupColumn( "Client ID", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupColumn( "Tasks", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 60 );

                    //-------------------------------------------------------------------------

                    ImGui::TableHeadersRow();

                    auto const workers = m_resourceServer.GetWorkers();
                    for ( ResourceServerWorker const* pWorker : workers )
                    {
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex( 0 );
                        ImGui::Text( "%u", pWorker->m_workerID );

                        ImGui::TableSetColumnIndex( 1 );
                        ImGui::Text( "%u", pWorker->m_networkClientID );

                        ImGui::TableSetColumnIndex( 2 );
                        ImGui::Text( "%u", pWorker->m_tasks.size() );
                    }

                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    }

    void ResourceServerUI::DrawClientInfoWindow()
    {
        if ( ImGui::Begin( g_clientInfoWindowName ) )
        {
            if ( ImGui::BeginTable( "ConnectionInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg ) )
            {
                ImGui::TableSetupColumn( "Client ID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 150 );
                ImGui::TableSetupColumn( "IP", ImGuiTableColumnFlags_WidthStretch );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                auto const clients = m_resourceServer.GetConnectedClients();
                for ( Network::Server::ClientInfo const* pClient : clients )
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::Text( "%u", pClient->m_ID );

                    ImGui::TableSetColumnIndex( 1 );
                    ImGui::Text( pClient->m_address.c_str() );
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void ResourceServerUI::DrawCompilerInfoWindow()
    {
        if ( ImGui::Begin( g_compilersWindowName ) )
        {
            if ( ImGui::BeginTable( "Registered Compilers Table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg, ImGui::GetContentRegionAvail() ) )
            {
                ImGui::SeparatorText( "Info" );
                ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthFixed, 200 );
                ImGui::TableSetupColumn( "Output Types", ImGuiTableColumnFlags_WidthStretch );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                auto const pCompilerRegistry = m_resourceServer.GetCompilerRegistry();

                for ( auto const& pCompiler : pCompilerRegistry->GetRegisteredCompilers() )
                {
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text( pCompiler->GetName().c_str() );

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();

                    InlineString str;
                    for ( Resource::Compiler::Output const& output : pCompiler->GetCompilableResourceTypes() )
                    {
                        str.append_sprintf( "%s (Ver: %d), ", output.m_typeID.ToString().c_str(), output.m_version );
                    }
                    str = str.substr( 0, str.length() - 2 );

                    ImGui::Text( str.c_str() );
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void ResourceServerUI::DrawPackagingWindow()
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
                    previewStr.append( mapID.GetFilenameWithoutExtension().c_str() );
                }

                if ( previewStr.empty() )
                {
                    previewStr = "Nothing To Package";
                }

                //-------------------------------------------------------------------------

                ImVec2 const dimensions = ImGui::GetContentRegionAvail();

                auto const& allMaps = m_resourceServer.GetAllFoundMaps();
                ImGui::SetNextItemWidth( dimensions.x  - 50 - ImGuiX::Style::s_iconButtonWidth - ( ImGui::GetStyle().ItemSpacing.x * 2 ) );
                {
                    if ( ImGui::BeginCombo( "##SelectedMaps", previewStr.c_str(), ImGuiComboFlags_HeightLargest ) )
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
                if ( ImGui::Button( EE_ICON_REFRESH"##RefreshMaps", ImVec2( ImGuiX::Style::s_iconButtonWidth, 0 ) ) )
                {
                    m_resourceServer.RefreshAvailableMapList();
                }
                ImGuiX::ItemTooltip( "Refresh Map List" );

                //-------------------------------------------------------------------------

                ImGui::SameLine();
                ImGui::BeginDisabled( !m_resourceServer.CanStartPackaging() );
                if ( ImGuiX::ButtonColored( "Start", Colors::Green, Colors::White, ImVec2( 50, 0 ) ) )
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
                float const progress = m_resourceServer.GetPackagingProgress();
                TInlineString<32> overlay( TInlineString<32>::CtorSprintf(), "%.2f%%", progress * 100 );
                ImGui::ProgressBar( progress, ImVec2( -1, 0 ), overlay.c_str() );

                ImGui::SeparatorText( "Selected Maps" );

                if ( ImGui::BeginChild( "SMaps", ImGui::GetContentRegionAvail(), 0, 0 ) )
                {
                    auto const& mapsToBePackaged = m_resourceServer.GetMapsQueuedForPackaging();
                    for ( auto const& mapID : mapsToBePackaged )
                    {
                        ImGui::BulletText( mapID.c_str() );
                    }
                }
                ImGui::EndChild();
            }
        }
        ImGui::End();
    }

    void ResourceServerUI::DrawRecompilationBlockersWindow()
    {
        if ( ImGui::Begin( g_recompilationBlockersWindowName ) )
        {
            float const buttonWidth = ImGuiX::CalculateButtonWidth( "Create Block" );
            m_pRecompilationBlockerPathPicker->UpdateAndDraw( ImGui::GetContentRegionAvail().x - buttonWidth - ImGui::GetStyle().ItemSpacing.x );

            ImGui::SameLine();
            ImGui::BeginDisabled( !m_pRecompilationBlockerPathPicker->GetPath().IsValid() );
            if ( ImGuiX::ButtonColored( "Create Block", Colors::Red, Colors::White, ImVec2( buttonWidth, 0 ) ) )
            {
                m_resourceServer.AddRecompilationBlocker( UUID::GenerateID(), m_pRecompilationBlockerPathPicker->GetPath() );
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            ImGui::SeparatorText( "Recompilation Blockers" );

            if ( ImGui::BeginTable( "BlockerInfo", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg ) )
            {
                ImGui::TableSetupColumn( "ID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 320 );
                ImGui::TableSetupColumn( "Time", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 50 );
                ImGui::TableSetupColumn( "Source", ImGuiTableColumnFlags_WidthFixed, 150 );
                ImGui::TableSetupColumn( "Path", ImGuiTableColumnFlags_WidthStretch );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                TVector<ResourceServer::RecompilationBlocker> const& blockers = m_resourceServer.GetRecompilationBlockers();
                for ( auto const& blocker : blockers )
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::Text( blocker.m_ID.ToString().c_str() );

                    ImGui::TableSetColumnIndex( 1 );
                    ImGui::Text( "%.2fs", blocker.m_timer.GetRemainingTime().ToFloat() );

                    ImGui::TableSetColumnIndex( 2 );
                    ImGui::Text( blocker.m_sourceID.empty() ? "" : blocker.m_sourceID.c_str() );

                    ImGui::TableSetColumnIndex( 3 );
                    ImGui::Text( blocker.m_path.c_str() );
                }

                ImGui::EndTable();
            }

            //-------------------------------------------------------------------------

            ImGui::SeparatorText( "Blocked Recompilation Requests" );

            if ( ImGui::BeginTable( "BlockedRequests", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg ) )
            {
                ImGui::TableSetupColumn( "Resource ID", ImGuiTableColumnFlags_WidthStretch );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                TVector<ResourceID> const& blockedList = m_resourceServer.GetBlockedRecompilationRequests();
                for ( auto const& blockID : blockedList )
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::Text( blockID.c_str() );
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    bool ResourceServerUI::DrawResaveProgressDialog()
    {
        if ( m_resourceServer.IsResavingDataFiles() )
        {
            float const progress = m_resourceServer.GetDataFileResaveProgress();
            TInlineString<32> overlay( TInlineString<32>::CtorSprintf(), "%.2f%%", progress * 100 );
            ImGui::ProgressBar( progress, ImVec2( -1, 0 ), overlay.c_str() );
            return true;
        }

        return false;
    }
}