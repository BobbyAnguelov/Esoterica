#include "ResourceFilePicker.h"
#include "ResourceDatabase.h"
#include "System/Imgui/ImguiX.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "System/Imgui/ImguiStyle.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    constexpr static float const g_resourcePickerButtonWidth = 24;

    //-------------------------------------------------------------------------

    ResourceFilePicker::PickerOption::PickerOption( ResourceID const& resourceID )
        : m_resourceID( resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );

        ResourcePath const& resourcePath = resourceID.GetResourcePath();
        m_filename = resourcePath.GetFileNameWithoutExtension();
        m_parentFolder = resourcePath.GetParentDirectory();
    }

    //-------------------------------------------------------------------------

    ResourceFilePicker::ResourceFilePicker( ToolsContext const& toolsContext )
        : m_toolsContext( toolsContext )
    {
        EE_ASSERT( toolsContext.IsValid() );
        Memory::MemsetZero( m_filterBuffer, 256 * sizeof( char ) );
    }

    FileSystem::Path const& ResourceFilePicker::GetRawResourceDirectoryPath() const
    {
        return m_toolsContext.m_pResourceDatabase->GetRawResourceDirectoryPath();
    }

    void ResourceFilePicker::RefreshResourceList( ResourceTypeID resourceTypeID )
    {
        m_knownResourceIDs.clear();

        if ( resourceTypeID.IsValid() )
        {
            for ( auto const& resourceRecord : m_toolsContext.m_pResourceDatabase->GetAllResourcesOfType( resourceTypeID ) )
            {
                m_knownResourceIDs.emplace_back( resourceRecord->m_resourceID.GetResourcePath() );
            }
        }
        else
        {
            for ( auto const& resourceListPair : m_toolsContext.m_pResourceDatabase->GetAllResources() )
            {
                for ( auto const& resourceRecord : resourceListPair.second )
                {
                    m_knownResourceIDs.emplace_back( resourceRecord->m_resourceID.GetResourcePath() );
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( m_filterBuffer[0] == 0 )
        {
            m_filteredResourceIDs = m_knownResourceIDs;
        }
        else
        {
            m_filteredResourceIDs.clear();
            for ( auto const& resourceID : m_knownResourceIDs )
            {
                String lowercasePath = resourceID.m_resourceID.GetResourcePath().GetString();
                lowercasePath.make_lower();

                bool passesFilter = true;
                char* token = strtok( m_filterBuffer, " " );
                while ( token )
                {
                    if ( lowercasePath.find( token ) == String::npos )
                    {
                        passesFilter = false;
                        break;
                    }

                    token = strtok( NULL, " " );
                }

                if ( passesFilter )
                {
                    m_filteredResourceIDs.emplace_back( resourceID );
                }
            }
        }
    }

    bool ResourceFilePicker::DrawResourcePicker( ResourceTypeID resourceTypeID, ResourceID const* pResourceID )
    {
        EE_ASSERT( pResourceID != nullptr );
        if ( resourceTypeID.IsValid() && pResourceID->IsValid() )
        {
            EE_ASSERT( pResourceID->GetResourceTypeID() == resourceTypeID );
        }
        bool valueUpdated = false;

        //-------------------------------------------------------------------------

        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const contentRegionAvailableX = ImGui::GetContentRegionAvail().x;
        constexpr float const typeLabelWidth = 40;

        ImGui::PushID( pResourceID );

        // Type and path
        //-------------------------------------------------------------------------

        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::TinyBold );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4, 6 ) );

            TInlineString<5> const resourceTypeStr = pResourceID->IsValid() ? pResourceID->GetResourceTypeID().ToString() : "-";
            ImVec2 const textSize = ImGui::CalcTextSize( resourceTypeStr.c_str() );
            float const deltaWidth = typeLabelWidth - textSize.x;
            if ( deltaWidth > 0 )
            {
                ImGui::SameLine( ( deltaWidth / 2 ) + itemSpacing );
            }
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored( Colors::LightPink.ToFloat4(), resourceTypeStr.c_str() );

            ImGui::SameLine( typeLabelWidth, itemSpacing );
            ImGui::SetNextItemWidth( contentRegionAvailableX - ( itemSpacing * 3 ) - ( g_resourcePickerButtonWidth * 3 ) - typeLabelWidth );

            ImVec4 const pathColor = VectorContains( m_knownResourceIDs, *pResourceID ) ? ImGuiX::Style::s_colorText.Value : Colors::Red.ToFloat4_ABGR();
            ImGui::PushStyleColor( ImGuiCol_Text, pathColor );
            ImGui::InputText( "##DataPath", const_cast<char*>( pResourceID->GetResourcePath().c_str() ), pResourceID->GetResourcePath().GetString().length(), ImGuiInputTextFlags_ReadOnly );
            ImGui::PopStyleColor();

            if ( pResourceID->IsValid() )
            {
                ImGuiX::ItemTooltip( pResourceID->GetResourcePath().c_str() );
            }

            ImGui::PopStyleVar();
        }

        // Buttons
        //-------------------------------------------------------------------------

        ImGui::SameLine( 0, itemSpacing );
        ImGui::BeginDisabled( !pResourceID->IsValid() );
        if ( ImGui::Button( EE_ICON_EYE_OUTLINE "##Open", ImVec2( g_resourcePickerButtonWidth, 0 ) ) )
        {
            m_toolsContext.TryOpenResource( *pResourceID );
        }
        ImGuiX::ItemTooltip( "Open Resource" );
        ImGui::EndDisabled();

        ImGui::SameLine( 0, itemSpacing );
        if ( ImGui::Button( EE_ICON_EYEDROPPER "##Pick", ImVec2( g_resourcePickerButtonWidth, 0 ) ) )
        {
            ImGui::OpenPopup( "Resource Picker" );
            m_filterBuffer[0] = 0;
            m_initializeFocus = true;
            RefreshResourceList( resourceTypeID );
            m_selectedID = *pResourceID;
        }
        ImGuiX::ItemTooltip( "Pick Resource" );

        ImGui::SameLine( 0, itemSpacing );
        ImGui::BeginDisabled( !pResourceID->IsValid() );
        if ( ImGui::Button( EE_ICON_DOTS_HORIZONTAL "##Options", ImVec2( g_resourcePickerButtonWidth, 0 ) ) )
        {
            ImGui::OpenPopup( "##ResourcePickerOptions" );
        }
        ImGuiX::ItemTooltip( "Options" );
        ImGui::EndDisabled();

        //-------------------------------------------------------------------------

        if( ImGui::BeginPopup( "##ResourcePickerOptions" ) )
        {
            if ( ImGui::MenuItem( EE_ICON_CONTENT_COPY "Copy Resource Path" ) )
            {
                ImGui::SetClipboardText( pResourceID->GetResourcePath().ToFileSystemPath( m_toolsContext.m_pResourceDatabase->GetRawResourceDirectoryPath() ).c_str() );
            }

            if ( ImGui::MenuItem( EE_ICON_ERASER "Clear" ) )
            {
                m_selectedID.Clear();
                valueUpdated = true;
            }

            ImGui::EndPopup();
        }

        ImGui::PopID();

        //-------------------------------------------------------------------------

        valueUpdated |= DrawDialog( resourceTypeID, pResourceID );

        return valueUpdated;
    }

    bool ResourceFilePicker::DrawPathPicker( ResourcePath* pResourcePath )
    {
        float const contentRegionAvailableX = ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const textAreaWidth = contentRegionAvailableX - ( g_resourcePickerButtonWidth * 2 ) - ( itemSpacing * 2 );

        bool valueUpdated = false;

        ImGui::SetNextItemWidth( textAreaWidth );
        ImGui::InputText( "##pathstring", const_cast<char*>( pResourcePath->GetString().data() ), pResourcePath->GetString().length(), ImGuiInputTextFlags_ReadOnly );

        ImGui::SameLine( 0, itemSpacing );
        if ( ImGui::Button( EE_ICON_EYEDROPPER "##Pick", ImVec2( g_resourcePickerButtonWidth, 0 ) ) )
        {
            auto const selectedFiles = pfd::open_file( "Choose Data File", GetRawResourceDirectoryPath().c_str(), { "All Files", "*" }, pfd::opt::none ).result();
            if ( !selectedFiles.empty() )
            {
                FileSystem::Path const selectedPath( selectedFiles[0].c_str() );

                if ( selectedPath.IsUnderDirectory( GetRawResourceDirectoryPath() ) )
                {
                    *pResourcePath = ResourcePath::FromFileSystemPath( GetRawResourceDirectoryPath().c_str(), selectedPath );
                    valueUpdated = true;
                }
                else
                {
                    pfd::message( "Error", "Selected file is not with the resource folder!", pfd::choice::ok, pfd::icon::error ).result();
                }
            }
        }

        ImGui::SameLine( 0, itemSpacing );
        ImGui::BeginDisabled( !pResourcePath->IsValid() );
        if ( ImGui::Button( EE_ICON_DOTS_HORIZONTAL "##Options", ImVec2( g_resourcePickerButtonWidth, 0 ) ) )
        {
            ImGui::OpenPopup( "##ResourcePickerOptions" );
        }
        ImGuiX::ItemTooltip( "Options" );
        ImGui::EndDisabled();

        //-------------------------------------------------------------------------

        if ( ImGui::BeginPopup( "##ResourcePickerOptions" ) )
        {
            if ( ImGui::MenuItem( EE_ICON_CONTENT_COPY "Copy Resource Path" ) )
            {
                ImGui::SetClipboardText( pResourcePath->ToFileSystemPath( m_toolsContext.m_pResourceDatabase->GetRawResourceDirectoryPath() ).c_str() );
            }

            if ( ImGui::MenuItem( EE_ICON_ERASER "Clear" ) )
            {
                pResourcePath->Clear();
                valueUpdated = true;
            }

            ImGui::EndPopup();
        }

        return valueUpdated;
    }

    bool ResourceFilePicker::DrawDialog( ResourceTypeID resourceTypeID, ResourceID const* pResourceID )
    {
        ImGui::PushID( pResourceID );

        bool selectionMade = false;
        bool isDialogOpen = true;
        ImGui::SetNextWindowSize( ImVec2( 1000, 400 ), ImGuiCond_FirstUseEver );
        ImGui::SetNextWindowSizeConstraints( ImVec2( 400, 400 ), ImVec2( FLT_MAX, FLT_MAX ) );
        if ( ImGui::BeginPopupModal( "Resource Picker", &isDialogOpen, ImGuiWindowFlags_NoSavedSettings ) )
        {
            ImVec2 const contentRegionAvailable = ImGui::GetContentRegionAvail();

            // Draw Filter
            //-------------------------------------------------------------------------

            bool filterUpdated = false;

            ImGui::SetNextItemWidth( contentRegionAvailable.x - ImGui::GetStyle().WindowPadding.x - 26 );
            InlineString filterCopy( m_filterBuffer );
            
            if ( m_initializeFocus )
            {
                ImGui::SetKeyboardFocusHere();
                m_initializeFocus = false;
            }

            if ( ImGui::InputText( "##Filter", filterCopy.data(), 256) )
            {
                if ( strcmp( filterCopy.data(), m_filterBuffer) != 0 )
                {
                    strcpy_s( m_filterBuffer, 256, filterCopy.data() );

                    // Convert buffer to lower case
                    int32_t i = 0;
                    while ( i < 256 && m_filterBuffer[i] != 0 )
                    {
                        m_filterBuffer[i] = eastl::CharToLower( m_filterBuffer[i] );
                        i++;
                    }

                    filterUpdated = true;
                }
            }

            ImGui::SameLine();
            if ( ImGui::Button( EE_ICON_CLOSE_CIRCLE"##Clear Filter", ImVec2( 26, 0 ) ) )
            {
                m_filterBuffer[0] = 0;
                filterUpdated = true;
            }

            // Update filter options
            //-------------------------------------------------------------------------

            if ( filterUpdated )
            {
                RefreshResourceList( resourceTypeID );
            }

            // Draw results
            //-------------------------------------------------------------------------

            float const tableHeight = contentRegionAvailable.y - ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.y;
            if ( ImGui::BeginTable( "Resource List", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2( contentRegionAvailable.x, tableHeight ) ) )
            {
                ImGui::TableSetupColumn( "File", ImGuiTableColumnFlags_WidthStretch, 0.65f );
                ImGui::TableSetupColumn( "Path", ImGuiTableColumnFlags_WidthStretch );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                ImGuiListClipper clipper;
                clipper.Begin( (int32_t) m_filteredResourceIDs.size() );
                while ( clipper.Step() )
                {
                    for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                    {
                        ImGui::TableNextRow();

                        ImGui::TableNextColumn();
                        bool isSelected = ( m_selectedID == m_filteredResourceIDs[i].m_resourceID );
                        if ( ImGui::Selectable( m_filteredResourceIDs[i].m_filename.c_str(), &isSelected, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns ) )
                        {
                            m_selectedID = m_filteredResourceIDs[i].m_resourceID;

                            if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                            {
                                selectionMade = true;
                                ImGui::CloseCurrentPopup();
                            }
                        }

                        ImGui::TableNextColumn();
                        ImGui::Text( m_filteredResourceIDs[i].m_parentFolder.c_str() );
                    }
                }

                ImGui::EndTable();
            }

            ImGui::EndPopup();
        }
        ImGui::PopID();

        //-------------------------------------------------------------------------

        if ( !isDialogOpen )
        {
            m_selectedID = *pResourceID;
        }

        //-------------------------------------------------------------------------

        return selectionMade;
    }
}