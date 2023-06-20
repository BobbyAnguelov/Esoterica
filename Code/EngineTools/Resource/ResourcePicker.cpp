#include "ResourcePicker.h"
#include "ResourceDatabase.h"
#include "System/Imgui/ImguiX.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Platform/PlatformUtils_Win32.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    static ImVec2 const g_buttonSize( 24, 24 );

    //-------------------------------------------------------------------------

    ResourcePicker::ResourcePicker( ToolsContext const& toolsContext, ResourceTypeID resourceTypeID, ResourceID const& resourceID )
        : m_toolsContext( toolsContext )
        , m_resourceTypeID( resourceTypeID )
        , m_resourceID( resourceID )
    {
        GenerateResourceOptionsList();
        GenerateFilteredOptionList();
    }

    bool ResourcePicker::UpdateAndDraw()
    {
        bool valueUpdated = false;

        //-------------------------------------------------------------------------
        // Validation
        //-------------------------------------------------------------------------

        ResourceTypeID actualResourceTypeID = m_resourceTypeID;
        bool validPath = true;

        if ( m_resourceID.IsValid() )
        {
            // Check resource TypeID
            //-------------------------------------------------------------------------

            // Get actual resource typeID
            auto const pExtension = m_resourceID.GetResourcePath().GetExtension();
            if ( pExtension != nullptr )
            {
                actualResourceTypeID = ResourceTypeID( pExtension );
            }

            //-------------------------------------------------------------------------

            if ( m_resourceTypeID.IsValid() )
            {
                if ( !m_toolsContext.m_pTypeRegistry->IsResourceTypeDerivedFrom( actualResourceTypeID, m_resourceTypeID ) )
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

            // Check if file exist
            //-------------------------------------------------------------------------

            if ( validPath )
            {
                validPath = m_toolsContext.m_pResourceDatabase->DoesResourceExist( m_resourceID );
            }
        }

        //-------------------------------------------------------------------------
        // Draw Picker
        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        if ( ImGui::BeginChild( "RP", ImVec2( -1, ImGui::GetFrameHeightWithSpacing() ), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
        {
            auto const& style = ImGui::GetStyle();
            float const itemSpacingX = style.ItemSpacing.x;

            // Combo Selector
            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::TinyBold );
                ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4, 6 ) );

                // Path Widget
                //-------------------------------------------------------------------------

                // Calculate size of resource path field
                float const contentRegionAvailableX = ImGui::GetContentRegionAvail().x;
                float usedWidth = ( itemSpacingX * 2 ) + ( g_buttonSize.x * 2 );
                float const pathWidgetWidth = contentRegionAvailableX - usedWidth;

                ImVec2 const comboDropDownSize( Math::Max( pathWidgetWidth, 500.0f ), ImGui::GetFrameHeightWithSpacing() * 20 );
                ImGui::SetNextItemWidth( pathWidgetWidth );
                ImGui::SetNextWindowSizeConstraints( ImVec2( comboDropDownSize.x, 0 ), comboDropDownSize );

                bool const wasComboOpen = m_isComboOpen;
                m_isComboOpen = ImGui::BeginCombo( "##DataPath", "", ImGuiComboFlags_HeightLarge | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_CustomPreview);

                // Regenerate options and clear filter each time we open the combo
                if ( m_isComboOpen && !wasComboOpen )
                {
                    m_filterWidget.Clear();
                    GenerateFilteredOptionList();
                }

                // Draw combo if open
                if ( m_isComboOpen )
                {
                    if ( m_filterWidget.DrawAndUpdate( -1, ImGuiX::FilterWidget::Flags::TakeInitialFocus ) )
                    {
                        GenerateFilteredOptionList();
                    }

                    //-------------------------------------------------------------------------

                    ImVec2 const childSize = comboDropDownSize - ImGui::GetStyle().WindowPadding - ImVec2( 0, ImGui::GetFrameHeightWithSpacing() + style.ItemSpacing.y );
                    if ( ImGui::BeginChild( "##ResList", childSize ) )
                    {
                        ImGuiListClipper clipper;
                        clipper.Begin( (int32_t) m_filteredResourceIDs.size() );
                        while ( clipper.Step() )
                        {
                            for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                            {
                                if ( ImGui::MenuItem( m_filteredResourceIDs[i].c_str() + 7 ) )
                                {
                                    m_resourceID = m_filteredResourceIDs[i];
                                    valueUpdated = true;
                                    ImGui::CloseCurrentPopup();
                                }
                                ImGuiX::ItemTooltip( m_filteredResourceIDs[i].c_str() );
                            }
                        }
                    }
                    ImGui::EndChild();

                    ImGui::EndCombo();
                }

                // Custom combo preview
                //-------------------------------------------------------------------------

                if ( ImGui::BeginComboPreview() )
                {
                    if ( actualResourceTypeID.IsValid() )
                    {
                        TInlineString<7> const resourceTypeStr( TInlineString<7>::CtorSprintf(), "[%s]", actualResourceTypeID.ToString().c_str() );
                        ImGui::TextColored( Colors::LightPink.ToFloat4(), resourceTypeStr.c_str() );
                        ImGui::SameLine();
                    }

                    ImVec4 const pathColor = validPath ? ImGuiX::Style::s_colorText.Value : Colors::Red.ToFloat4_ABGR();
                    ImGui::PushStyleColor( ImGuiCol_Text, pathColor );
                    ImGui::Text( m_resourceID.IsValid() ? m_resourceID.c_str() + 7 : "" );
                    ImGui::PopStyleColor();

                    ImGui::EndComboPreview();
                }

                // Tooltip
                if ( m_resourceID.IsValid() )
                {
                    ImGuiX::ItemTooltip( m_resourceID.c_str() );
                }

                ImGui::PopStyleVar();
            }

            // Buttons
            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
                ImGui::BeginDisabled( !validPath );

                // Open Resource
                ImGui::SameLine( 0, itemSpacingX );
                if ( ImGui::Button( EE_ICON_OPEN_IN_NEW "##Open", g_buttonSize ) )
                {
                    m_toolsContext.TryOpenResource( m_resourceID );
                }
                ImGuiX::ItemTooltip( "Open Resource" );

                // Options
                ImGui::SameLine( 0, itemSpacingX );
                if ( ImGui::Button( EE_ICON_COG "##Options", g_buttonSize ) )
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
                if ( ImGui::MenuItem( EE_ICON_FILE_OUTLINE " Copy Resource Path" ) )
                {
                    ImGui::SetClipboardText( m_resourceID.c_str() );
                }

                ImGui::BeginDisabled( true );
                if ( ImGui::MenuItem( EE_ICON_FOLDER_OPEN_OUTLINE " Show In Resource Browser" ) )
                {
                    // TODO
                }
                ImGui::EndDisabled();

                ImGui::Separator();

                if ( ImGui::MenuItem( EE_ICON_FILE " Copy File Path" ) )
                {
                    FileSystem::Path const fileSystemPath = m_resourceID.ToFileSystemPath( m_toolsContext.GetRawResourceDirectory() );
                    ImGui::SetClipboardText( fileSystemPath.c_str() );
                }

                if ( ImGui::MenuItem( EE_ICON_FOLDER_OPEN " Open In Explorer" ) )
                {
                    FileSystem::Path const fileSystemPath = m_resourceID.ToFileSystemPath( m_toolsContext.GetRawResourceDirectory() );
                    Platform::Win32::OpenInExplorer( fileSystemPath );
                }

                ImGui::Separator();

                if ( ImGui::MenuItem( EE_ICON_ERASER " Clear" ) )
                {
                    m_resourceID.Clear();
                    valueUpdated = true;
                }

                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();

        //-------------------------------------------------------------------------

        ImGui::PopID();

        return valueUpdated;
    }

    void ResourcePicker::GenerateResourceOptionsList()
    {
        m_allResourceIDs.clear();

        // Restrict options to specified resource type ID
        if ( m_resourceTypeID.IsValid() )
        {
            EE_ASSERT( m_toolsContext.m_pTypeRegistry->IsRegisteredResourceType( m_resourceTypeID ) );

            for ( auto const& resourceID : m_toolsContext.m_pResourceDatabase->GetAllResourcesOfType( m_resourceTypeID ) )
            {
                m_allResourceIDs.emplace_back( resourceID );
            }
        }
        else // All resource options are valid
        {
            for ( auto const& resourceListPair : m_toolsContext.m_pResourceDatabase->GetAllResources() )
            {
                for ( auto const& resourceRecord : resourceListPair.second )
                {
                    EE_ASSERT( resourceRecord->m_resourceID.IsValid() );
                    m_allResourceIDs.emplace_back( resourceRecord->m_resourceID );
                }
            }
        }
    }

    void ResourcePicker::GenerateFilteredOptionList()
    {
        if ( m_filterWidget.HasFilterSet() )
        {
            m_filteredResourceIDs.clear();

            for ( auto const& resourceID : m_allResourceIDs )
            {
                String lowercasePath = resourceID.GetResourcePath().GetString();
                lowercasePath.make_lower();

                if ( m_filterWidget.MatchesFilter( lowercasePath ) )
                {
                    m_filteredResourceIDs.emplace_back( resourceID );
                }
            }
        }
        else
        {
            m_filteredResourceIDs = m_allResourceIDs;
        }
    }

    void ResourcePicker::SetRequiredResourceType( ResourceTypeID resourceTypeID )
    {
        m_resourceTypeID = resourceTypeID;
        GenerateResourceOptionsList();
        GenerateFilteredOptionList();
    }

    void ResourcePicker::SetResourceID( ResourceID const& resourceID )
    {
        if ( resourceID.IsValid() && m_resourceTypeID.IsValid() )
        {
            EE_ASSERT( resourceID.GetResourceTypeID() == m_resourceTypeID );
        }

        m_resourceID = resourceID;
    }

    //-------------------------------------------------------------------------

    bool ResourcePathPicker::UpdateAndDraw()
    {
        bool valueUpdated = false;

        FileSystem::Path filePath;
        bool validPath = false;
        if ( m_resourcePath.IsValid() )
        {
            filePath = m_resourcePath.ToFileSystemPath( m_rawResourceDirectoryPath );
            validPath = filePath.Exists();
        }

        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        if ( ImGui::BeginChild( "RP", ImVec2( -1, ImGui::GetFrameHeightWithSpacing() ), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
        {
            float const itemSpacingX = ImGui::GetStyle().ItemSpacing.x;

            // Type and path
            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::TinyBold );

                // Calculate size of resource path field
                float const contentRegionAvailableX = ImGui::GetContentRegionAvail().x;
                float usedWidth = ( itemSpacingX * 2 ) + ( g_buttonSize.x * 2 ) + 1;

                // Resource path
                ImGui::SetNextItemWidth( contentRegionAvailableX - usedWidth );
                String const& resourcePathStr = m_resourcePath.GetString();
                ImGui::PushStyleColor( ImGuiCol_Text, validPath ? ImGuiX::Style::s_colorText.Value : ImGuiX::ImColors::Red );
                ImGui::InputText( "##DataPath", const_cast<char*>( resourcePathStr.c_str() ), resourcePathStr.length(), ImGuiInputTextFlags_ReadOnly );
                ImGui::PopStyleColor();

                // Tooltip
                if ( m_resourcePath.IsValid() )
                {
                    ImGuiX::ItemTooltip( m_resourcePath.c_str() );
                }

                // Pick Path
                ImGui::SameLine( 0, itemSpacingX );
                if ( ImGui::Button( EE_ICON_FILE_FIND "##Pick", g_buttonSize ) )
                {
                    auto const selectedFiles = pfd::open_file( "Choose Data File", m_rawResourceDirectoryPath.c_str(), { "All Files", "*" }, pfd::opt::none ).result();
                    if ( !selectedFiles.empty() )
                    {
                        FileSystem::Path const selectedPath( selectedFiles[0].c_str() );

                        if ( selectedPath.IsUnderDirectory( m_rawResourceDirectoryPath ) )
                        {
                            m_resourcePath = ResourcePath::FromFileSystemPath( m_rawResourceDirectoryPath.c_str(), selectedPath );
                            valueUpdated = true;
                        }
                        else
                        {
                            pfd::message( "Error", "Selected file is not with the raw resource folder!", pfd::choice::ok, pfd::icon::error ).result();
                        }
                    }
                }
                ImGuiX::ItemTooltip( "Pick Resource" );
            }

            // Options
            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );

                ImGui::SameLine( 0, itemSpacingX );
                ImGui::BeginDisabled( !m_resourcePath.IsValid() );
                if ( ImGui::Button( EE_ICON_COG "##Options", g_buttonSize ) )
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
                if ( ImGui::MenuItem( EE_ICON_FILE_OUTLINE" Copy Resource Path" ) )
                {
                    ImGui::SetClipboardText( m_resourcePath.c_str() );
                }

                if ( ImGui::MenuItem( EE_ICON_FILE" Copy File Path" ) )
                {
                    ImGui::SetClipboardText( filePath.c_str() );
                }

                if ( ImGui::MenuItem( EE_ICON_FOLDER_OPEN " Open In Explorer" ) )
                {
                    Platform::Win32::OpenInExplorer( filePath );
                }

                ImGui::Separator();

                if ( ImGui::MenuItem( EE_ICON_ERASER " Clear" ) )
                {
                    m_resourcePath.Clear();
                    valueUpdated = true;
                }

                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();
        ImGui::PopID();

        //-------------------------------------------------------------------------

        return valueUpdated;
    }
}