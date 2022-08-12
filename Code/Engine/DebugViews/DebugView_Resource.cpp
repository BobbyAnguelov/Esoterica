#include "DebugView_Resource.h"
#include "System/Resource/ResourceSystem.h"
#include "System/Systems.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Resource
{
    ResourceDebugView::ResourceDebugView()
    {
        m_menus.emplace_back( DebugMenu( "System/Resource", [this] ( EntityWorldUpdateContext const& context ) { DrawResourceMenu( context ); } ) );
    }

    void ResourceDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        EntityWorldDebugView::Initialize( systemRegistry, pWorld );
        m_pResourceSystem = systemRegistry.GetSystem<ResourceSystem>();
    }

    void ResourceDebugView::Shutdown()
    {
        m_pResourceSystem = nullptr;
        EntityWorldDebugView::Shutdown();
    }

    void ResourceDebugView::DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        if ( m_isHistoryWindowOpen )
        {
            ImGui::SetNextWindowBgAlpha( 0.75f );
            DrawLogWindow( m_pResourceSystem, &m_isHistoryWindowOpen );
        }

        if ( m_isOverviewWindowOpen )
        {
            ImGui::SetNextWindowBgAlpha( 0.75f );
            DrawOverviewWindow( m_pResourceSystem, &m_isOverviewWindowOpen );
        }
    }

    void ResourceDebugView::DrawResourceMenu( EntityWorldUpdateContext const& context )
    {
        if ( ImGui::MenuItem( "Show Resource System Overview" ) )
        {
            m_isOverviewWindowOpen = true;
        }

        if ( ImGui::MenuItem( "Show Request History" ) )
        {
            m_isHistoryWindowOpen = true;
        }
    }

    //-------------------------------------------------------------------------

    void ResourceDebugView::DrawOverviewWindow( ResourceSystem* pResourceSystem, bool* pIsOpen )
    {
        EE_ASSERT( pResourceSystem != nullptr );

        //-------------------------------------------------------------------------

        auto DrawRow = [] ( ResourceRecord const* pRecord )
        {
            ImGui::TableNextRow();

            //-------------------------------------------------------------------------

            ImGui::TableSetColumnIndex( 0 );
            ImGui::Text( pRecord->GetResourceTypeID().ToString().c_str() );

            //-------------------------------------------------------------------------

            ImGui::TableSetColumnIndex( 1 );
            ImGui::Text( "%d", pRecord->m_references.size() );

            //-------------------------------------------------------------------------

            ImGui::TableSetColumnIndex( 2 );

            switch ( pRecord->m_loadingStatus )
            {
                case LoadingStatus::Unloaded:
                {
                    ImGui::Text( "Unloaded" );
                }
                break;

                case LoadingStatus::Loading:
                {
                    ImGui::TextColored( Colors::Yellow.ToFloat4(), "Loaded" );
                }
                break;

                case LoadingStatus::Loaded:
                {
                    ImGui::TextColored( Colors::LimeGreen.ToFloat4(), "Loaded" );
                }
                break;

                case LoadingStatus::Unloading:
                {
                    ImGui::Text( "Unloading" );
                }
                break;

                case LoadingStatus::Failed:
                {
                    ImGui::TextColored( Colors::Red.ToFloat4(), "Loaded" );
                }
                break;
            }

            //-------------------------------------------------------------------------

            ImGui::TableSetColumnIndex( 3 );
            if ( ImGui::TreeNode( pRecord->GetResourceID().c_str() ) )
            {
                for ( auto const& requesterID : pRecord->m_references )
                {
                    if ( requesterID.IsManualRequest() )
                    {
                        ImGui::TextColored( Colors::Aqua.ToFloat4(), "Manual Request" );
                    }
                    else if ( requesterID.IsInstallDependencyRequest() )
                    {
                        ImGui::TextColored( Colors::Coral.ToFloat4(), "Install Dependency: %u", requesterID.GetInstallDependencyResourcePathID() );
                    }
                    else // Normal request
                    {
                        ImGui::TextColored( Colors::Lime.ToFloat4(), "Entity: %u", requesterID.GetID() );
                    }
                }

                ImGui::TreePop();
            }

            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );

                ImGui::TableSetColumnIndex( 4 );
                ImGui::Text( "%.3fms", pRecord->GetFileReadTime().ToFloat() );
                if ( ImGui::IsItemHovered() ) { ImGui::SetTooltip( "File Read Time" ); }

                ImGui::TableSetColumnIndex( 5 );
                ImGui::Text( "%.3fms", pRecord->GetLoadTime().ToFloat() );
                if ( ImGui::IsItemHovered() ) { ImGui::SetTooltip( "Load Time" ); }

                ImGui::TableSetColumnIndex( 6 );
                ImGui::Text( "%.3fms", pRecord->GetDependenciesWaitTime().ToFloat() );
                if ( ImGui::IsItemHovered() ) { ImGui::SetTooltip( "Wait for Dependencies Time" ); }

                ImGui::TableSetColumnIndex( 7 );
                ImGui::Text( "%.3fms", pRecord->GetInstallTime().ToFloat() );
                if ( ImGui::IsItemHovered() ) { ImGui::SetTooltip( "Install Time" ); }
            }
        };

        //-------------------------------------------------------------------------

        if ( ImGui::Begin( "Resource System Overview", pIsOpen ) )
        {
            ImGui::Text( "Num Resources Loaded: %d", pResourceSystem->m_resourceRecords.size() );

            ImGui::Separator();

            if ( ImGui::BeginTable( "Resource Reference Tracker Table", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable ) )
            {
                ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30 );
                ImGui::TableSetupColumn( "Refs", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 24 );
                ImGui::TableSetupColumn( "Status", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 60 );
                ImGui::TableSetupColumn( "ID", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "FRT", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0 );
                ImGui::TableSetupColumn( "LT", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0 );
                ImGui::TableSetupColumn( "DWT", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0 );
                ImGui::TableSetupColumn( "IT", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0 );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                //-------------------------------------------------------------------------

                auto const& resourceRecords = pResourceSystem->m_resourceRecords;
                for ( auto const& recordTuple : resourceRecords )
                {
                    ResourceRecord const* pRecord = recordTuple.second;
                    DrawRow( pRecord );
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void ResourceDebugView::DrawLogWindow( ResourceSystem* pResourceSystem, bool* pIsOpen )
    {
        if ( ImGui::Begin( "Resource Request History", pIsOpen ) )
        {
            if ( ImGui::BeginTable( "Resource Request History Table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable ) )
            {
                ImGui::TableSetupColumn( "Time", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 50 );
                ImGui::TableSetupColumn( "Request", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 45 );
                ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30 );
                ImGui::TableSetupColumn( "ID", ImGuiTableColumnFlags_WidthStretch );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                //-------------------------------------------------------------------------

                int32_t const numEntries = (int32_t) pResourceSystem->m_history.size();
                int32_t const lastEntryIdx = numEntries - 1;

                ImGuiListClipper clipper;
                clipper.Begin( numEntries );
                bool isFirstStep = true;
                while ( clipper.Step() )
                {
                    for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                    {
                        auto const& entry = pResourceSystem->m_history[lastEntryIdx - i];

                        ImGui::TableNextRow();

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 0 );
                        ImGui::Text( entry.m_time.GetTimeDetailed().c_str() );

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 1 );
                        switch ( entry.m_type )
                        {
                            case ResourceSystem::PendingRequest::Type::Load:
                            {
                                ImGui::TextColored( Colors::LimeGreen.ToFloat4(), "Load" );
                            }
                            break;

                            case ResourceSystem::PendingRequest::Type::Unload:
                            {
                                ImGui::TextColored( Colors::OrangeRed.ToFloat4(), "Unload" );
                            }
                            break;
                        }

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 2 );
                        ImGui::Text( entry.m_ID.GetResourceTypeID().ToString().c_str() );

                        //-------------------------------------------------------------------------

                        ImGui::TableSetColumnIndex( 3 );
                        ImGui::Text( entry.m_ID.c_str() );
                    }
                }

                // Auto scroll the table
                if ( ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
                {
                    ImGui::SetScrollHereY( 1.0f );
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }
}
#endif