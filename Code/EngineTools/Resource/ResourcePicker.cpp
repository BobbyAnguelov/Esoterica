#include "ResourcePicker.h"
#include "ResourceDatabase.h"
#include "System/Imgui/ImguiX.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Imgui/ImguiStyle.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    static ImVec2 const g_buttonSize( 24, 24 );

    //-------------------------------------------------------------------------

    ResourcePicker::PickerOption::PickerOption( ResourceID const& resourceID )
        : m_resourceID( resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );

        ResourcePath const& resourcePath = resourceID.GetResourcePath();
        m_filename = resourcePath.GetFileNameWithoutExtension();
        m_parentFolder = resourcePath.GetParentDirectory();
    }

    //-------------------------------------------------------------------------

    ResourcePicker::ResourcePicker( ToolsContext const& toolsContext )
        : m_toolsContext( toolsContext )
    {
        EE_ASSERT( toolsContext.IsValid() );
        Memory::MemsetZero( m_filterBuffer, 256 * sizeof( char ) );
    }

    void ResourcePicker::GenerateResourceOptionsList( ResourceTypeID resourceTypeID )
    {
        m_allResourceIDs.clear();

        // Restrict options to specified resource type ID
        if ( resourceTypeID.IsValid() )
        {
            EE_ASSERT( m_toolsContext.m_pTypeRegistry->IsRegisteredResourceType( resourceTypeID ) );

            for ( auto const& resourceRecord : m_toolsContext.m_pResourceDatabase->GetAllResourcesOfType( resourceTypeID ) )
            {
                m_allResourceIDs.emplace_back( resourceRecord->m_resourceID.GetResourcePath() );
            }
        }
        else // All resource options are valid
        {
            for ( auto const& resourceListPair : m_toolsContext.m_pResourceDatabase->GetAllResources() )
            {
                for ( auto const& resourceRecord : resourceListPair.second )
                {
                    m_allResourceIDs.emplace_back( resourceRecord->m_resourceID.GetResourcePath() );
                }
            }
        }
    }

    void ResourcePicker::GeneratedFilteredOptionList()
    {
        if ( m_filterBuffer[0] == 0 )
        {
            m_filteredResourceIDs = m_allResourceIDs;
        }
        else
        {
            m_filteredResourceIDs.clear();
            for ( auto const& resourceID : m_allResourceIDs )
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

    //-------------------------------------------------------------------------

    bool ResourcePicker::DrawPicker( ResourcePath const& resourcePath, ResourcePath& outPath, bool restrictToResources, ResourceTypeID restrictedResourceTypeID )
    {
        bool valueUpdated = false;
        ResourceTypeID actualResourceTypeID;
        bool validPath = true;

        //-------------------------------------------------------------------------
        // Validation
        //-------------------------------------------------------------------------

        if ( resourcePath.IsValid() )
        {
            // Check if valid file path
            //-------------------------------------------------------------------------

            if ( resourcePath.IsDirectory() )
            {
                validPath = false;
            }

            // Check resource TypeID
            //-------------------------------------------------------------------------

            if ( validPath && restrictToResources )
            {
                // Get actual resource typeID
                auto const pExtension = resourcePath.GetExtension();
                if ( pExtension != nullptr )
                {
                    actualResourceTypeID = ResourceTypeID( pExtension );
                }

                //-------------------------------------------------------------------------

                if ( restrictedResourceTypeID.IsValid() )
                {
                    if ( !m_toolsContext.m_pTypeRegistry->IsResourceTypeDerivedFrom( actualResourceTypeID, restrictedResourceTypeID ) )
                    {
                        validPath = false;
                    }
                }
                else
                {
                    if ( !m_toolsContext.m_pTypeRegistry->IsRegisteredResourceType( actualResourceTypeID ) )
                    {
                        validPath = false;
                    }
                }
            }

            // Check if file exist
            //-------------------------------------------------------------------------

            if ( validPath )
            {
                validPath = m_toolsContext.m_pResourceDatabase->DoesResourceExist( resourcePath );
            }
        }

        //-------------------------------------------------------------------------
        // Draw Picker
        //-------------------------------------------------------------------------

        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const contentRegionAvailableX = ImGui::GetContentRegionAvail().x;
        constexpr float const typeLabelWidth = 30;

        ImGui::PushID( &resourcePath );

        // Type and path
        //-------------------------------------------------------------------------

        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::TinyBold );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4, 6 ) );

            // Type Label

            if ( restrictToResources )
            {
                TInlineString<5> const resourceTypeStr = actualResourceTypeID.IsValid() ? actualResourceTypeID.ToString() : "-";
                ImVec2 const textSize = ImGui::CalcTextSize( resourceTypeStr.c_str() );
                float const deltaWidth = typeLabelWidth - textSize.x;
                if ( deltaWidth > 0 )
                {
                    ImGui::SameLine( ( deltaWidth / 2 ) );
                }
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored( Colors::LightPink.ToFloat4(), resourceTypeStr.c_str() );
                ImGui::SameLine( typeLabelWidth, itemSpacing );
            }

            // Calculate size of resource path field
            float usedWidth = 0;
            if ( restrictToResources ) // Label and open resource button
            {
                usedWidth = ( itemSpacing * 3 ) + ( g_buttonSize.x * 3 ) + typeLabelWidth;
            }
            else // No label and no 'open resource button'
            {
                usedWidth = ( itemSpacing * 2 ) + ( g_buttonSize.x * 2 );
            }

            // Resource Path
            ImGui::SetNextItemWidth( contentRegionAvailableX - usedWidth );
            ImVec4 const pathColor = validPath ? ImGuiX::Style::s_colorText.Value : Colors::Red.ToFloat4_ABGR();
            ImGui::PushStyleColor( ImGuiCol_Text, pathColor );
            ImGui::InputText( "##DataPath", const_cast<char*>( resourcePath.c_str() ), resourcePath.GetString().length(), ImGuiInputTextFlags_ReadOnly );
            ImGui::PopStyleColor();

            // Tooltip
            if ( resourcePath.IsValid() )
            {
                ImGuiX::ItemTooltip( resourcePath.c_str() );
            }

            ImGui::PopStyleVar();
        }

        // Buttons
        //-------------------------------------------------------------------------

        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );

            // Open Resource
            if ( restrictToResources )
            {
                ImGui::SameLine( 0, itemSpacing );
                ImGui::BeginDisabled( !resourcePath.IsValid() || !validPath );
                if ( ImGui::Button( EE_ICON_EYE_OUTLINE "##Open", g_buttonSize ) )
                {
                    m_toolsContext.TryOpenResource( resourcePath );
                }
                ImGuiX::ItemTooltip( "Open Resource" );
                ImGui::EndDisabled();
            }

            // Pick Path
            ImGui::SameLine( 0, itemSpacing );
            if ( ImGui::Button( EE_ICON_EYEDROPPER "##Pick", g_buttonSize ) )
            {
                // Use resource picker
                if ( restrictToResources )
                {
                    m_filterBuffer[0] = 0;
                    m_initializeFocus = true;
                    GenerateResourceOptionsList( restrictedResourceTypeID );
                    GeneratedFilteredOptionList();
                    m_currentlySelectedID = resourcePath.IsValid() ? resourcePath : ResourceID();
                    ImGui::OpenPopup( "Resource Picker" );
                }
                else // Use file picker
                {
                    auto const selectedFiles = pfd::open_file( "Choose Data File", m_toolsContext.GetRawResourceDirectory().c_str(), { "All Files", "*" }, pfd::opt::none ).result();
                    if ( !selectedFiles.empty() )
                    {
                        FileSystem::Path const selectedPath( selectedFiles[0].c_str() );

                        if ( selectedPath.IsUnderDirectory( m_toolsContext.GetRawResourceDirectory() ) )
                        {
                            outPath = ResourcePath::FromFileSystemPath( m_toolsContext.GetRawResourceDirectory().c_str(), selectedPath );
                            valueUpdated = true;
                        }
                        else
                        {
                            pfd::message( "Error", "Selected file is not with the raw resource folder!", pfd::choice::ok, pfd::icon::error ).result();
                        }
                    }
                }
            }
            ImGuiX::ItemTooltip( "Pick Resource" );

            // Options
            ImGui::SameLine( 0, itemSpacing );
            ImGui::BeginDisabled( !resourcePath.IsValid() );
            if ( ImGui::Button( EE_ICON_DOTS_HORIZONTAL "##Options", g_buttonSize ) )
            {
                ImGui::OpenPopup( "##ResourcePickerOptions" );
            }
            ImGuiX::ItemTooltip( "Options" );
            ImGui::EndDisabled();
        }

        // Options Context Menu
        //-------------------------------------------------------------------------

        if ( ImGui::BeginPopup( "##ResourcePickerOptions" ) )
        {
            if ( ImGui::MenuItem( EE_ICON_CONTENT_COPY " Copy Resource Path" ) )
            {
                ImGui::SetClipboardText( resourcePath.ToFileSystemPath( m_toolsContext.m_pResourceDatabase->GetRawResourceDirectoryPath() ).c_str() );
            }

            if ( ImGui::MenuItem( EE_ICON_ERASER " Clear" ) )
            {
                outPath.Clear();
                valueUpdated = true;
            }

            ImGui::EndPopup();
        }

        //-------------------------------------------------------------------------

        if ( DrawDialog() )
        {
            outPath = m_currentlySelectedID.GetResourcePath();
            valueUpdated = true;
        }

        //-------------------------------------------------------------------------

        ImGui::PopID();

        return valueUpdated;
    }

    bool ResourcePicker::DrawDialog()
    {
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
                GeneratedFilteredOptionList();
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
                        bool isSelected = ( m_currentlySelectedID == m_filteredResourceIDs[i].m_resourceID );
                        if ( ImGui::Selectable( m_filteredResourceIDs[i].m_filename.c_str(), &isSelected, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns ) )
                        {
                            m_currentlySelectedID = m_filteredResourceIDs[i].m_resourceID;

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

        //-------------------------------------------------------------------------

        return selectionMade;
    }
}