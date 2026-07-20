#include "EditorTool_ResourceSystem.h"
#include "Engine/Resource/Debug/DebugView_Resource.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/Resource/ResourceRequest.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/TypeSystem/ResourceInfo.h"
#include "Base/Resource/Settings/Settings_Resource.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceSystemEditorTool::ResourceSystemEditorTool( ToolsContext const* pToolsContext )
        : EditorTool( pToolsContext, "Resource System" )
    {}

    void ResourceSystemEditorTool::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );
        CreateToolWindow( "Overview", [this] ( UpdateContext const& context, bool isFocused ) { DrawOverviewWindow( context, isFocused ); } );
        CreateToolWindow( "Details", [this] ( UpdateContext const& context, bool isFocused ) { DrawDetailsWindow( context, isFocused ); } );
        CreateToolWindow( "ResourceRecord History", [this] ( UpdateContext const& context, bool isFocused ) { DrawRequestHistoryWindow( context, isFocused ); } );
        CreateToolWindow( "Resource Provider", [this] ( UpdateContext const& context, bool isFocused ) { DrawResourceProviderWindow( context, isFocused ); } );
    }

    void ResourceSystemEditorTool::SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID leftDockID = 0, bottomLeftDockID = 0, rightDockID = 0, bottomRightDockID = 0;
        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.5f, &leftDockID, &rightDockID );
        ImGui::DockBuilderSplitNode( rightDockID, ImGuiDir_Down, 0.5f, &bottomRightDockID, &rightDockID );
        ImGui::DockBuilderSplitNode( leftDockID, ImGuiDir_Down, 0.5f, &bottomLeftDockID, &leftDockID );

        // Dock windows
        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetToolWindowName( "Overview" ).c_str(), leftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "ResourceRecord History" ).c_str(), rightDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Details" ).c_str(), bottomLeftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Resource Provider" ).c_str(), bottomRightDockID );
    }

    void ResourceSystemEditorTool::DrawOverviewWindow( UpdateContext const& context, bool isFocused )
    {
        bool isSelectedResourceIDValid = false;

        // Header
        //-------------------------------------------------------------------------

        ImGui::Text( "Num Resources Loaded: %d", m_pResourceSystem->m_resourceRecords.size() );

        ImGui::Separator();

        // Filter
        //-------------------------------------------------------------------------

        m_filter.UpdateAndDraw();

        // Draw Resource List
        //-------------------------------------------------------------------------

        InlineString tooltipStr;

        ImGui::TableNextColumn();
        if ( ImGui::BeginTable( "Resource Reference Tracker Table", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable, ImGui::GetContentRegionAvail() ) )
        {
            ImGui::TableSetupColumn( "ID", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Status", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 80 );
            ImGui::TableSetupColumn( "Stage", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 200 );
            ImGui::TableSetupColumn( "References", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40 );
            ImGui::TableSetupColumn( "Load Time", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 100 );
            ImGui::TableSetupColumn( "Wait Time", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 100 );
            ImGui::TableSetupColumn( "Install Time", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 100 );

            ImGui::TableSetupScrollFreeze( 0, 1 );

            // Update Requests List
            //-------------------------------------------------------------------------

            if ( ImGuiTableSortSpecs* pSortSpecs = ImGui::TableGetSortSpecs() )
            {
                if ( pSortSpecs->SpecsDirty )
                {
                    m_sortedColumnIdx = pSortSpecs->Specs[0].ColumnIndex;
                    m_sortAscending = pSortSpecs->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
                    pSortSpecs->SpecsDirty = false;
                }
            }

            GenerateSortedAndFilteredRecordList();

            //-------------------------------------------------------------------------

            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 8, 0 ) );

            ImGuiListClipper clipper;
            clipper.Begin( (int32_t) m_cachedRecords.size() );
            while ( clipper.Step() )
            {
                for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                {
                    ResourceRecord const* pRecord = m_cachedRecords[i];
                    auto pResourceInfo = m_pToolsContext->m_pTypeRegistry->GetResourceInfo( pRecord->GetResourceTypeID() );
                    EE_ASSERT( pResourceInfo != nullptr );

                    ImGui::PushID( pRecord );
                    ImGui::TableNextRow();

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();

                    bool isSelected = ( m_selectedResourceID == pRecord->GetResourceID() );
                    ImGui::TextColored( pResourceInfo->m_color.ToFloat4(), EE_ICON_FILE );
                    ImGui::SameLine();
                    if ( ImGui::Selectable( pRecord->GetResourceID().c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns ) )
                    {
                        m_selectedResourceID = pRecord->GetResourceID();
                        isSelected = true;
                    }

                    if ( isSelected )
                    {
                        isSelectedResourceIDValid = true;
                    }

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();
                    ImGui::Text( pResourceInfo->m_friendlyName.c_str() );

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();
                    switch ( pRecord->GetLoadingStatus() )
                    {
                        case LoadingStatus::Unloaded:
                        {
                            ImGui::Text( "Unloaded" );
                        }
                        break;

                        case LoadingStatus::Loading:
                        {
                            ImGui::TextColored( Colors::Yellow.ToFloat4(), "Loading" );
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
                            ImGui::TextColored( Colors::Red.ToFloat4(), "Failed" );
                            ImGui::SameLine();
                            InlineString str;
                            if ( !pRecord->GetCompilationLog().empty() )
                            {
                                str.append( pRecord->GetCompilationLog().c_str() );
                            }

                            if ( !str.empty() )
                            {
                                str.append( "\n" );
                            }

                            if ( !pRecord->GetErrorLog().empty() )
                            {
                                str.append( pRecord->GetErrorLog().c_str() );
                            }

                            ImGuiX::HelpMarker( str.c_str() );
                        }
                        break;
                    }

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();
                    if ( pRecord->GetLoadStage() != Resource::ResourceLoadStage::None )
                    {
                        ImGui::Text( Resource::GetResourceLoadStageName( pRecord->GetLoadStage() ) );
                    }

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();
                    ImGui::Text( "%d", pRecord->GetReferences().size() );

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();
                    ImGui::Text( "%.1fms", pRecord->GetLoadTime().ToFloat() );

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();
                    ImGui::Text( "%.1fms", pRecord->GetLoadStageTime( ResourceLoadStage::WaitForInstallDependencies ).ToFloat() );

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();
                    ImGui::Text( "%.1fms", pRecord->GetLoadStageTime( ResourceLoadStage::InstallResource ).ToFloat() );

                    ImGui::PopID();
                }
            }

            ImGui::PopStyleVar();
            ImGui::EndTable();
        }

        if ( !isSelectedResourceIDValid )
        {
            m_selectedResourceID.Clear();
        }
    }

    void ResourceSystemEditorTool::DrawRequestHistoryWindow( UpdateContext const& context, bool isFocused )
    {
        ResourceDebugView::DrawRequestHistory( m_pResourceSystem );
    }

    void ResourceSystemEditorTool::DrawDetailsWindow( UpdateContext const& context, bool isFocused )
    {
        ResourceRecord const* pSelectedRecord = nullptr;
        if ( m_selectedResourceID.IsValid() )
        {
            auto iter = m_pResourceSystem->m_resourceRecords.find( m_selectedResourceID );
            if ( iter != m_pResourceSystem->m_resourceRecords.end() )
            {
                pSelectedRecord = iter->second;
            }
        }

        //-------------------------------------------------------------------------

        if ( pSelectedRecord != nullptr )
        {
            auto pResourceInfo = m_pToolsContext->m_pTypeRegistry->GetResourceInfo( pSelectedRecord->GetResourceTypeID() );

            ImGuiX::DrawHeader( EE_ICON_FILE, pSelectedRecord->GetResourceID().GetDataPath().GetFilename().c_str(), pResourceInfo->m_color );

            //-------------------------------------------------------------------------

            if ( ImGui::CollapsingHeader( EE_ICON_FILE" File Details", ImGuiTreeNodeFlags_DefaultOpen ) )
            {
                ImGui::Text( "Data Path: %s", pSelectedRecord->GetResourceID().c_str() );

                ImGui::Text( "Resource Type: %s", pResourceInfo->m_friendlyName.c_str() );
            }

            //-------------------------------------------------------------------------

            if ( ImGui::CollapsingHeader( EE_ICON_UPLOAD" Load Details", ImGuiTreeNodeFlags_DefaultOpen ) )
            {
                ImGui::Text( "File Read Time: %.1fms", pSelectedRecord->GetFileReadTime().ToFloat() );
                ImGui::Text( "Load Time: %.1fms", pSelectedRecord->GetLoadTime().ToFloat() );
                ImGui::Text( "Install Time: %.1fms", pSelectedRecord->GetInstallTime().ToFloat() );
            }

            if ( pSelectedRecord->GetLoadingStatus() == LoadingStatus::Failed )
            {
                if ( ImGui::CollapsingHeader( EE_ICON_ALERT_CIRCLE_OUTLINE" Load Failure Log", ImGuiTreeNodeFlags_DefaultOpen ) )
                {
                    ImGui::PushTextWrapPos( ImGui::GetCursorPos().x + ImGui::GetContentRegionAvail().x );
                    ImGui::TextColored( Colors::Red.ToFloat4(), pSelectedRecord->GetCompilationLog().c_str() );
                    ImGui::PopTextWrapPos();
                }
            }

            //-------------------------------------------------------------------------

            InlineString str;
            str.sprintf( EE_ICON_GRAPH" Install Dependencies (%d)##Dependencies", pSelectedRecord->GetInstallDependencies().size() );
            if ( ImGui::CollapsingHeader( str.c_str(), ImGuiTreeNodeFlags_DefaultOpen ) )
            {
                if ( pSelectedRecord->GetInstallDependencies().empty() )
                {
                    ImGui::Text( "No Install Dependencies" );
                }

                for ( ResourceID const& dependencyID : pSelectedRecord->GetInstallDependencies() )
                {
                    if ( ImGui::TextLink( dependencyID.c_str() ) )
                    {
                        m_selectedResourceID = dependencyID;
                    }
                }
            }

            //-------------------------------------------------------------------------

            str.sprintf( EE_ICON_LINK_VARIANT" Referenced By (%d)##References", pSelectedRecord->GetReferences().size() );
            if ( ImGui::CollapsingHeader( str.c_str(), ImGuiTreeNodeFlags_DefaultOpen ) )
            {
                if ( ImGui::BeginTable( "References Table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImGui::GetContentRegionAvail() ) )
                {
                    ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 150 );
                    ImGui::TableSetupColumn( "Source", ImGuiTableColumnFlags_WidthStretch );

                    ImGui::TableSetupScrollFreeze( 0, 1 );
                    ImGui::TableHeadersRow();

                    for ( ResourceRequesterID const& requesterID : pSelectedRecord->GetReferences() )
                    {
                        ImGui::PushID( (void*) requesterID.GetID() );
                        ImGui::TableNextRow();

                        if ( requesterID.IsManualRequest() )
                        {
                            ImGui::TableNextColumn();
                            ImGui::TextColored( Colors::Cyan.ToFloat4(), "Manual ResourceRecord" );
                        }

                        //-------------------------------------------------------------------------

                        else if ( requesterID.IsToolsRequest() )
                        {
                            ImGui::TableNextColumn();
                            ImGui::TextColored( Colors::Cyan.ToFloat4(), "Editor Tool" );

                            ImGui::TableNextColumn();
                            InlineString const editorToolName = m_pToolsContext->GetEditorToolName( requesterID.GetToolID() );
                            if ( editorToolName.empty() )
                            {
                                ImGui::TextColored( Colors::Red.ToFloat4(), "Unknown tool editor ID: %d", requesterID.GetToolID() );
                            }
                            else
                            {
                                ImGui::Text( "Loaded by a tool editor: %s", editorToolName.c_str() );
                            }
                        }

                        //-------------------------------------------------------------------------

                        else if ( requesterID.IsInstallDependencyRequest() )
                        {
                            ImGui::TableNextColumn();
                            ImGui::TextColored( Colors::Coral.ToFloat4(), "Install Dependency" );

                            ImGui::TableNextColumn();

                            // TODO: optimize this
                            ResourceID foundID;
                            for ( auto const& recordTuple : m_pResourceSystem->m_resourceRecords )
                            {
                                if ( recordTuple.second->GetResourceID().GetPathID() == requesterID.GetInstallDependencyResourcePathID() )
                                {
                                    foundID = recordTuple.second->GetResourceID();
                                    break;
                                }
                            }

                            if ( foundID.IsValid() )
                            {
                                if ( ImGui::TextLink( foundID.c_str() ) )
                                {
                                    m_selectedResourceID = foundID;
                                }
                            }
                            else
                            {
                                ImGui::TextColored( Colors::Red.ToFloat4(), "Unknown resource ID: %u", requesterID.GetInstallDependencyResourcePathID() );
                            }
                        }

                        //-------------------------------------------------------------------------

                        else if ( requesterID.IsEntitySystemRequest() )
                        {
                            ImGui::TableNextColumn();
                            ImGui::TextColored( Colors::GreenYellow.ToFloat4(), "Entity System" );

                            ImGui::TableNextColumn();
                            auto lookupResult = m_pToolsContext->TryFindEntityInAllWorlds( EntityID( requesterID.GetID() ) );
                            if ( lookupResult.IsValid() )
                            {
                                ImGui::Text( "Entity: '%s' in World: '%s'", lookupResult.m_pEntity->GetNameID().c_str(), lookupResult.m_pWorld->GetDebugName().c_str() );
                            }
                            else
                            {
                                ImGui::TextColored( Colors::Red.ToFloat4(), "Unknown Entity ID: %u", requesterID.GetID() );
                            }
                        }

                        //-------------------------------------------------------------------------

                        else
                        {
                            ImGui::TableNextColumn();
                            ImGui::TextColored( Colors::Red.ToFloat4(), "Invalid Requester ID" );
                        }

                        ImGui::PopID();
                    }

                    ImGui::EndTable();
                }
            }
        }
        else
        {
            ImGui::Text( "Nothing selected in the overview list" );
        }
    }

    void ResourceSystemEditorTool::DrawResourceProviderWindow( UpdateContext const& context, bool isFocused )
    {
        auto pSettings = context.GetSystemRegistry()->GetSystem<SettingsRegistry>()->GetSettings<ResourceSettings>();
        ResourceDebugView::DrawResourceProviderOverview( pSettings, m_pResourceSystem );
    }

    void ResourceSystemEditorTool::GenerateSortedAndFilteredRecordList()
    {
        m_cachedRecords.clear();

        auto const& resourceRecords = m_pResourceSystem->m_resourceRecords;
        for ( auto const& recordTuple : resourceRecords )
        {
            ResourceRecord const* pRecord = recordTuple.second;

            bool const matchesFilter = m_filter.HasFilterSet() ? m_filter.MatchesFilter( pRecord->GetResourceID().c_str() ) : true;
            if ( matchesFilter )
            {
                m_cachedRecords.emplace_back( pRecord );
            }
        }

        //-------------------------------------------------------------------------

        if ( m_sortedColumnIdx == 0 )
        {
            auto SortPredicate = [this] ( ResourceRecord const* pA, ResourceRecord const* pB )
            {
                if ( m_sortAscending )
                {
                    return pA->GetResourceID().GetString() < pB->GetResourceID().GetString();
                }
                else
                {
                    return pA->GetResourceID().GetString() > pB->GetResourceID().GetString();
                }
            };

            eastl::sort( m_cachedRecords.begin(), m_cachedRecords.end(), SortPredicate );
        }
        else if ( m_sortedColumnIdx == 1 )
        {
            auto SortPredicate = [this] ( ResourceRecord const* pA, ResourceRecord const* pB )
            {
                if ( m_sortAscending )
                {
                    return pA->GetResourceID().GetResourceTypeID().ToString() < pB->GetResourceID().GetResourceTypeID().ToString();
                }
                else
                {
                    return pA->GetResourceID().GetResourceTypeID().ToString() > pB->GetResourceID().GetResourceTypeID().ToString();
                }
            };

            eastl::sort( m_cachedRecords.begin(), m_cachedRecords.end(), SortPredicate );
        }
        else if ( m_sortedColumnIdx == 2 )
        {
            auto SortPredicate = [this] ( ResourceRecord const* pA, ResourceRecord const* pB )
            {
                if ( m_sortAscending )
                {
                    return pA->GetLoadingStatus() < pB->GetLoadingStatus();
                }
                else
                {
                    return pA->GetLoadingStatus() > pB->GetLoadingStatus();
                }
            };

            eastl::sort( m_cachedRecords.begin(), m_cachedRecords.end(), SortPredicate );
        }
        else if ( m_sortedColumnIdx == 3 )
        {
            auto SortPredicate = [this] ( ResourceRecord const* pA, ResourceRecord const* pB )
            {
                if ( m_sortAscending )
                {
                    return pA->GetLoadStage() < pB->GetLoadStage();
                }
                else
                {
                    return pA->GetLoadStage() > pB->GetLoadStage();
                }
            };

            eastl::sort( m_cachedRecords.begin(), m_cachedRecords.end(), SortPredicate );
        }
        else if ( m_sortedColumnIdx == 4 )
        {
            auto SortPredicate = [this] ( ResourceRecord const* pA, ResourceRecord const* pB )
            {
                if ( m_sortAscending )
                {
                    return pA->GetReferences().size() < pB->GetReferences().size();
                }
                else
                {
                    return pA->GetReferences().size() > pB->GetReferences().size();
                }
            };

            eastl::sort( m_cachedRecords.begin(), m_cachedRecords.end(), SortPredicate );
        }
        else if ( m_sortedColumnIdx == 5 )
        {
            auto SortPredicate = [this] ( ResourceRecord const* pA, ResourceRecord const* pB )
            {
                if ( m_sortAscending )
                {
                    return pA->GetLoadTime() < pB->GetLoadTime();
                }
                else
                {
                    return pA->GetLoadTime() > pB->GetLoadTime();
                }
            };

            eastl::sort( m_cachedRecords.begin(), m_cachedRecords.end(), SortPredicate );
        }
        else if ( m_sortedColumnIdx == 6 )
        {
            auto SortPredicate = [this] ( ResourceRecord const* pA, ResourceRecord const* pB )
            {
                if ( m_sortAscending )
                {
                    return pA->GetLoadStageTime( ResourceLoadStage::WaitForInstallDependencies ) < pB->GetLoadStageTime( ResourceLoadStage::WaitForInstallDependencies );
                }
                else
                {
                    return pA->GetLoadStageTime( ResourceLoadStage::WaitForInstallDependencies ) > pB->GetLoadStageTime( ResourceLoadStage::WaitForInstallDependencies );
                }
            };

            eastl::sort( m_cachedRecords.begin(), m_cachedRecords.end(), SortPredicate );
        }
        else if ( m_sortedColumnIdx == 7 )
        {
            auto SortPredicate = [this] ( ResourceRecord const* pA, ResourceRecord const* pB )
            {
                if ( m_sortAscending )
                {
                    return pA->GetInstallTime() < pB->GetInstallTime();
                }
                else
                {
                    return pA->GetInstallTime() > pB->GetInstallTime();
                }
            };

            eastl::sort( m_cachedRecords.begin(), m_cachedRecords.end(), SortPredicate );
        }
    }

    

}