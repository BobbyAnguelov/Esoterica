#include "Pickers.h"
#include "EngineTools/FileSystem/FileRegistry.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Core/CommonToolTypes.h"
#include "EngineTools/Core/Dialogs.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include "Base/TypeSystem/DataFileInfo.h"

//-------------------------------------------------------------------------

namespace EE
{
    static ImVec2 const g_buttonSize( 30, 0 );

    //-------------------------------------------------------------------------

    bool DataPathPicker::UpdateAndDraw()
    {
        bool valueUpdated = false;

        FileSystem::Path filePath;
        bool isValidPath = false;
        if ( m_dataPath.IsValid() )
        {
            filePath = m_dataPath.GetFileSystemPath( m_sourceDataDirectoryPath );
            isValidPath = filePath.Exists();
        }

        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        if ( ImGui::BeginChild( "RP", ImVec2( -1, ImGui::GetFrameHeight() ), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
        {
            float const itemSpacingX = ImGui::GetStyle().ItemSpacing.x;

            // Type and path
            //-------------------------------------------------------------------------

            {
                // Calculate size of resource path field
                float const contentRegionAvailableX = ImGui::GetContentRegionAvail().x;
                float usedWidth = ( itemSpacingX * 2 ) + ( g_buttonSize.x * 2 ) + 1;

                // Resource path
                ImGui::SetNextItemWidth( contentRegionAvailableX - usedWidth );
                String const& dataPathStr = m_dataPath.GetString();
                ImGui::PushStyleColor( ImGuiCol_Text, isValidPath ? ImGuiX::Style::s_colorText : Colors::Red );
                ImGui::InputText( "##DataPath", const_cast<char*>( dataPathStr.c_str() ), dataPathStr.length(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );
                ImGui::PopStyleColor();

                // Drag and drop
                if ( ImGui::BeginDragDropTarget() )
                {
                    valueUpdated = TrySetPathFromDragAndDrop();
                    ImGui::EndDragDropTarget();
                }

                // Allow pasting valid paths
                if ( ImGui::IsItemFocused() )
                {
                    if ( ImGui::IsKeyDown( ImGuiMod_Shortcut ) && ImGui::IsKeyPressed( ImGuiKey_V ) )
                    {
                        String clipboardText = ImGui::GetClipboardText();
                        if ( clipboardText.length() > 256 )
                        {
                            EE_LOG_WARNING( "Resource", "DataFilePathPicker", "Pasting invalid length string" );
                            clipboardText.clear();
                        }

                        if ( DataPath::IsValidPath( clipboardText ) )
                        {
                            m_dataPath = DataPath( clipboardText );
                            valueUpdated = true;
                        }
                        else
                        {
                            FileSystem::Path pastedFilePath( clipboardText );
                            if ( pastedFilePath.IsValid() && pastedFilePath.IsUnderDirectory( m_sourceDataDirectoryPath ) )
                            {
                                m_dataPath = DataPath::FromFileSystemPath( m_sourceDataDirectoryPath.c_str(), pastedFilePath );
                                valueUpdated = true;
                            }
                        }
                    }
                }

                // Tooltip
                if ( m_dataPath.IsValid() )
                {
                    ImGuiX::ItemTooltip( m_dataPath.c_str() );
                }

                // Pick Path
                ImGui::SameLine( 0, itemSpacingX );
                if ( ImGui::Button( EE_ICON_FILE_SEARCH_OUTLINE "##Pick", g_buttonSize ) )
                {
                    FileDialog::Result const result = FileDialog::Load( {}, "Choose Data File", true, m_sourceDataDirectoryPath.c_str() );
                    if ( result )
                    {
                        FileSystem::Path const selectedPath( result.m_filePaths[0].c_str() );

                        if ( selectedPath.IsUnderDirectory( m_sourceDataDirectoryPath ) )
                        {
                            m_dataPath = DataPath::FromFileSystemPath( m_sourceDataDirectoryPath.c_str(), selectedPath );
                            valueUpdated = true;
                        }
                        else
                        {
                            MessageDialog::Error( "Error", "Selected file is not with the raw resource folder!" );
                        }
                    }
                }
                ImGuiX::ItemTooltip( "Pick File" );
            }

            // Options
            //-------------------------------------------------------------------------

            {
                ImGui::SameLine( 0, itemSpacingX );
                ImGui::BeginDisabled( !m_dataPath.IsValid() );
                if ( ImGui::Button( EE_ICON_COG "##Options", g_buttonSize ) )
                {
                    ImGui::OpenPopup( "##DataFilePathPickerOptions" );
                }
                ImGuiX::ItemTooltip( "Options" );
                ImGui::EndDisabled();
            }

            // Options Context Menu
            //-------------------------------------------------------------------------

            if ( ImGui::BeginPopup( "##DataFilePathPickerOptions" ) )
            {
                if ( ImGui::MenuItem( EE_ICON_FILE_OUTLINE" Copy Data Path" ) )
                {
                    ImGui::SetClipboardText( m_dataPath.c_str() );
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
                    m_dataPath.Clear();
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

    bool DataPathPicker::TrySetPathFromDragAndDrop()
    {
        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( DragAndDrop::s_filePayloadID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( payload->IsDelivery() )
            {
                ResourceID const droppedResourceID( (char*) payload->Data );
                if ( droppedResourceID.IsValid() )
                {
                    m_dataPath = droppedResourceID.GetResourcePath();
                    return true;
                }
            }
        }

        return false;
    }
}

//-------------------------------------------------------------------------

namespace EE
{
    DataFilePathPicker::DataFilePathPicker( ToolsContext const& toolsContext, TypeSystem::TypeID dataFileTypeID, DataPath const& datafilePath )
        : m_toolsContext( toolsContext )
    {
        SetRequiredDataFileType( dataFileTypeID );
        SetPath( datafilePath );
    }

    bool DataFilePathPicker::UpdateAndDraw()
    {
        bool valueUpdated = false;

        //-------------------------------------------------------------------------
        // Validation
        //-------------------------------------------------------------------------

        bool isValidPath = m_path.IsValid();
        if ( isValidPath )
        {
            // Check extension
            //-------------------------------------------------------------------------

            isValidPath = IsValidExtension( m_path.GetExtension() );

            // Check if file exist
            //-------------------------------------------------------------------------

            if ( isValidPath )
            {
                isValidPath = m_toolsContext.m_pFileRegistry->DoesFileExist( m_path );
            }
        }

        //-------------------------------------------------------------------------
        // Draw Picker
        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        if ( ImGui::BeginChild( "RP", ImVec2( -1, ImGui::GetFrameHeight() ), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
        {
            auto const& style = ImGui::GetStyle();
            float const itemSpacingX = style.ItemSpacing.x;

            // Combo Selector
            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( !m_toolsContext.m_pFileRegistry->IsDataFileCacheBuilt() );
            {
                // Calculate size of resource path field
                float const contentRegionAvailableX = ImGui::GetContentRegionAvail().x;
                float usedWidth = ( itemSpacingX * 2 ) + ( g_buttonSize.x * 2 );

                // Type and path fields
                //-------------------------------------------------------------------------

                float const startCursorPosX = ImGui::GetCursorPosX();
                TInlineString<7> const resourceTypeStr( TInlineString<7>::CtorSprintf(), "[%s]", m_requiredExtension.c_str() );
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored( Colors::LightPink.ToFloat4(), resourceTypeStr.c_str() );
                ImGui::SameLine();

                float const typeStrWidth = ImGui::GetCursorPosX() - startCursorPosX;
                float const comboArrowWidth = ImGui::GetFrameHeight();
                float const totalPathWidgetWidth = contentRegionAvailableX - usedWidth - typeStrWidth;
                float const textWidgetWidth = totalPathWidgetWidth - comboArrowWidth - style.ItemSpacing.x;

                ImVec4 const pathColor = ( isValidPath ? ImGuiX::Style::s_colorText : Colors::Red ).ToFloat4();
                ImGui::PushStyleColor( ImGuiCol_Text, pathColor );
                ImGui::SetNextItemWidth( textWidgetWidth );
                String const& pathString = m_path.GetString();
                ImGui::InputText( "##DataPathText", const_cast<char*>( pathString.c_str() ), pathString.length(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );
                ImGui::PopStyleColor();

                // Drag and drop
                if ( ImGui::BeginDragDropTarget() )
                {
                    valueUpdated = TryUpdatePathFromDragAndDrop();
                    ImGui::EndDragDropTarget();
                }

                // Tooltip
                if ( m_path.IsValid() )
                {
                    ImGuiX::ItemTooltip( m_path.c_str() );
                }

                // Allow pasting valid paths
                if ( ImGui::IsItemFocused() )
                {
                    if ( ImGui::IsKeyDown( ImGuiMod_Shortcut ) && ImGui::IsKeyPressed( ImGuiKey_V ) )
                    {
                        valueUpdated |= TryUpdatePathFromClipboard();
                    }
                }

                // Combo
                //-------------------------------------------------------------------------

                ImVec2 const comboDropDownSize( Math::Max( totalPathWidgetWidth, 500.0f ), ImGui::GetFrameHeight() * 20 );
                ImGui::SetNextWindowSizeConstraints( ImVec2( comboDropDownSize.x, 0 ), comboDropDownSize );

                ImGui::SameLine();
                bool const wasComboOpen = m_isComboOpen;
                m_isComboOpen = ImGui::BeginCombo( "##DataPath", "", ImGuiComboFlags_HeightLarge | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_NoPreview );

                // Regenerate options and clear filter each time we open the combo
                if ( m_isComboOpen && !wasComboOpen )
                {
                    m_filterWidget.Clear();
                    GenerateResourceOptionsList();
                    GenerateFilteredOptionList();
                }

                // Draw combo if open
                bool shouldUpdateNavID = false;
                if ( m_isComboOpen )
                {
                    float const cursorPosYPreFilter = ImGui::GetCursorPosY();
                    if ( m_filterWidget.UpdateAndDraw( -1, ImGuiX::FilterWidget::Flags::TakeInitialFocus ) )
                    {
                        GenerateFilteredOptionList();
                    }
                    float const cursorPosYPostFilter = ImGui::GetCursorPosY();
                    float const filterHeight = cursorPosYPostFilter;

                    ImVec2 const previousCursorPos = ImGui::GetCursorPos();
                    ImVec2 const childSize( ImGui::GetContentRegionAvail().x, comboDropDownSize.y - filterHeight - style.ItemSpacing.y - style.WindowPadding.y );
                    ImGui::Dummy( childSize );
                    ImGui::SetCursorPos( previousCursorPos );

                    //-------------------------------------------------------------------------

                    if ( ImGui::BeginChild( "##OptList", childSize, false, ImGuiWindowFlags_NavFlattened ) )
                    {
                        ImGuiX::ScopedFont const sfo( ImGuiX::Font::Medium );

                        ImGuiListClipper clipper;
                        clipper.Begin( (int32_t) m_filteredOptions.size() );
                        while ( clipper.Step() )
                        {
                            for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                            {
                                if ( ImGui::Selectable( m_filteredOptions[i].c_str() + 7 ) )
                                {
                                    m_path = m_filteredOptions[i];
                                    valueUpdated = true;
                                    ImGui::CloseCurrentPopup();
                                }
                                ImGuiX::ItemTooltip( m_filteredOptions[i].c_str() );
                            }
                        }
                        clipper.End();
                    }
                    ImGui::EndChild();

                    ImGui::EndCombo();
                }
            }
            ImGui::EndDisabled();

            // Buttons
            //-------------------------------------------------------------------------

            {
                // Open Resource
                ImGui::SameLine( 0, itemSpacingX );
                ImGui::BeginDisabled( !m_path.IsValid() );
                if ( ImGui::Button( EE_ICON_OPEN_IN_NEW "##Open", g_buttonSize ) )
                {
                    m_toolsContext.TryOpenDataFile( m_path );
                }
                ImGuiX::ItemTooltip( "Open Data File" );
                ImGui::EndDisabled();

                // Options
                ImGui::SameLine( 0, itemSpacingX );
                if ( ImGui::Button( EE_ICON_COG "##Options", g_buttonSize ) )
                {
                    ImGui::OpenPopup( "##DataFilePathPickerOptions" );
                }
                ImGuiX::ItemTooltip( "Options" );
            }

            // Options Context Menu
            //-------------------------------------------------------------------------

            if ( ImGui::BeginPopup( "##DataFilePathPickerOptions" ) )
            {
                if ( ImGui::MenuItem( EE_ICON_FILE_OUTLINE " Copy Data Path" ) )
                {
                    ImGui::SetClipboardText( m_path.c_str() );
                }

                if ( ImGui::MenuItem( EE_ICON_FOLDER_OPEN_OUTLINE " Show In Resource Browser" ) )
                {
                    m_toolsContext.TryFindInResourceBrowser( m_path );
                }

                ImGui::Separator();

                if ( ImGui::MenuItem( EE_ICON_FILE " Copy File Path" ) )
                {
                    FileSystem::Path const fileSystemPath = m_path.GetFileSystemPath( m_toolsContext.GetSourceDataDirectory() );
                    ImGui::SetClipboardText( fileSystemPath.c_str() );
                }

                if ( ImGui::MenuItem( EE_ICON_FOLDER_OPEN " Open In Explorer" ) )
                {
                    FileSystem::Path const fileSystemPath = m_path.GetFileSystemPath( m_toolsContext.GetSourceDataDirectory() );
                    Platform::Win32::OpenInExplorer( fileSystemPath );
                }

                ImGui::Separator();

                if ( ImGui::MenuItem( EE_ICON_ERASER " Clear" ) )
                {
                    m_path.Clear();
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

    void DataFilePathPicker::GenerateResourceOptionsList()
    {
        m_generatedOptions.clear();

        EE_ASSERT( m_toolsContext.m_pTypeRegistry->IsRegisteredDataFileType( m_fileTypeID ) );
        uint32_t const extensionFourCC = FourCC::FromLowercaseString( m_requiredExtension.c_str() );
        for( auto const& pFileInfo : m_toolsContext.m_pFileRegistry->GetAllDataFileEntries( extensionFourCC ) )
        {
            m_generatedOptions.emplace_back( pFileInfo->m_dataPath );
        }
    }

    void DataFilePathPicker::GenerateFilteredOptionList()
    {
        if ( m_filterWidget.HasFilterSet() )
        {
            m_filteredOptions.clear();

            for ( DataPath const& dataPath : m_generatedOptions )
            {
                String lowercasePath = dataPath.GetString();
                lowercasePath.make_lower();

                if ( m_filterWidget.MatchesFilter( lowercasePath ) )
                {
                    m_filteredOptions.emplace_back( dataPath );
                }
            }
        }
        else
        {
            m_filteredOptions = m_generatedOptions;
        }
    }

    void DataFilePathPicker::SetRequiredDataFileType( TypeSystem::TypeID typeID )
    {
        m_fileTypeID = typeID;
        auto pDataFileInfo = m_toolsContext.m_pTypeRegistry->GetDataFileInfo( typeID );
        EE_ASSERT( pDataFileInfo != nullptr );
        m_requiredExtension = pDataFileInfo->GetExtension();

        GenerateResourceOptionsList();
        GenerateFilteredOptionList();
    }

    void DataFilePathPicker::SetPath( DataPath const& path )
    {
        if ( path.IsValid() )
        {
            EE_ASSERT( IsValidExtension( path.GetExtension() ) );
        }

        m_path = path;
    }

    bool DataFilePathPicker::TryUpdatePathFromClipboard()
    {
        String clipboardText = ImGui::GetClipboardText();

        if ( clipboardText.length() > 256 )
        {
            EE_LOG_WARNING( "Resource", "DataFilePathPicker", "Pasting invalid length string" );
            return false;
        }

        // Check for a valid file path if the resource path is bad
        //-------------------------------------------------------------------------

        DataPath pastedPath;

        if ( !DataPath::IsValidPath( clipboardText ) )
        {
            FileSystem::Path pastedFilePath( clipboardText );
            if ( pastedFilePath.IsValid() && pastedFilePath.IsUnderDirectory( m_toolsContext.GetSourceDataDirectory() ) )
            {
                pastedPath = DataPath::FromFileSystemPath( m_toolsContext.GetSourceDataDirectory().c_str(), pastedFilePath );
            }
        }
        else
        {
            pastedPath = DataPath( clipboardText );
        }

        if ( !pastedPath.IsValid() )
        {
            return false;
        }

        // Validate path extension
        //-------------------------------------------------------------------------

        if ( IsValidExtension( pastedPath.GetExtension() ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        m_path = pastedPath;
        return true;
    }

    bool DataFilePathPicker::TryUpdatePathFromDragAndDrop()
    {
        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( DragAndDrop::s_filePayloadID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( payload->IsDelivery() )
            {
                DataPath droppedPath( (char*) payload->Data );
                if ( droppedPath.IsValid() && IsValidExtension( droppedPath.GetExtension() ) )
                {
                    m_path = droppedPath;
                    return true;
                }
            }
        }

        return false;
    }
}

//-------------------------------------------------------------------------

namespace EE
{
    ResourcePicker::ResourcePicker( ToolsContext const& toolsContext, ResourceTypeID resourceTypeID, ResourceID const& resourceID )
        : m_toolsContext( toolsContext )
    {
        SetRequiredResourceType( resourceTypeID );
        SetResourceID( resourceID );
    }

    bool ResourcePicker::UpdateAndDraw()
    {
        bool valueUpdated = false;

        //-------------------------------------------------------------------------
        // Validation
        //-------------------------------------------------------------------------

        ResourceTypeID actualResourceTypeID = m_resourceTypeID;
        bool isValidPath = true;

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
                    isValidPath = false;
                }
            }
            else
            {
                if ( !m_toolsContext.m_pTypeRegistry->IsRegisteredResourceType( actualResourceTypeID ) )
                {
                    isValidPath = false;
                }
            }

            // Check if file exist
            //-------------------------------------------------------------------------

            if ( isValidPath )
            {
                if ( m_pCustomOptionProvider != nullptr )
                {
                    isValidPath = m_pCustomOptionProvider->ValidatePath( m_toolsContext, m_resourceID );
                }
                else
                {
                    isValidPath = m_toolsContext.m_pFileRegistry->DoesFileExist( m_resourceID );
                }
            }
        }

        //-------------------------------------------------------------------------
        // Draw Picker
        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        if ( ImGui::BeginChild( "RP", ImVec2( -1, ImGui::GetFrameHeight() ), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
        {
            auto const& style = ImGui::GetStyle();
            float const itemSpacingX = style.ItemSpacing.x;

            // Combo Selector
            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( !m_toolsContext.m_pFileRegistry->IsDataFileCacheBuilt() );
            {
                // Calculate size of resource path field
                float const contentRegionAvailableX = ImGui::GetContentRegionAvail().x;
                float usedWidth = ( itemSpacingX * 2 ) + ( g_buttonSize.x * 2 );

                // Type and path fields
                //-------------------------------------------------------------------------

                float typeStrWidth = 0;
                if ( actualResourceTypeID.IsValid() )
                {
                    float const startCursorPosX = ImGui::GetCursorPosX();
                    TInlineString<7> const resourceTypeStr( TInlineString<7>::CtorSprintf(), "[%s]", actualResourceTypeID.ToString().c_str() );
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextColored( Colors::LightPink.ToFloat4(), resourceTypeStr.c_str() );
                    ImGui::SameLine();

                    typeStrWidth = ImGui::GetCursorPosX() - startCursorPosX;
                }

                float const comboArrowWidth = ImGui::GetFrameHeight();
                float const totalPathWidgetWidth = contentRegionAvailableX - usedWidth - typeStrWidth;
                float const textWidgetWidth = totalPathWidgetWidth - comboArrowWidth - style.ItemSpacing.x;

                ImVec4 const pathColor = ( isValidPath ? ImGuiX::Style::s_colorText : Colors::Red ).ToFloat4();
                ImGui::PushStyleColor( ImGuiCol_Text, pathColor );
                ImGui::SetNextItemWidth( textWidgetWidth );
                String const& pathString = m_resourceID.ToString();
                ImGui::InputText( "##DataPathText", const_cast<char*>( pathString.c_str() ), pathString.length(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );
                ImGui::PopStyleColor();

                ImRect const inputRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

                // Drag and drop
                if ( ImGui::BeginDragDropTarget() )
                {
                    valueUpdated = TryUpdateResourceFromDragAndDrop();
                    ImGui::EndDragDropTarget();
                }

                // Tooltip
                if ( m_resourceID.IsValid() )
                {
                    ImGuiX::ItemTooltip( m_resourceID.c_str() );
                }

                // Allow pasting valid paths
                if ( ImGui::IsItemFocused() )
                {
                    if ( ImGui::IsKeyDown( ImGuiMod_Shortcut ) && ImGui::IsKeyPressed( ImGuiKey_V ) )
                    {
                        valueUpdated |= TryUpdateResourceFromClipboard();
                    }
                }

                // Fill gap between button and drop down button
                //-------------------------------------------------------------------------

                auto pDrawList = ImGui::GetWindowDrawList();
                ImVec2 const fillerMin = inputRect.GetTR() + ImVec2( -style.FrameRounding, 0 );
                ImVec2 const fillerMax = inputRect.GetBR() + ImVec2( style.ItemSpacing.x + style.FrameRounding, 0 );
                pDrawList->AddRectFilled( fillerMin, fillerMax, ImGui::ColorConvertFloat4ToU32( style.Colors[ImGuiCol_FrameBg] ) );

                // Combo
                //-------------------------------------------------------------------------

                ImVec2 const comboDropDownSize( Math::Max( totalPathWidgetWidth, 500.0f ), ImGui::GetFrameHeight() * 20 );
                ImGui::SetNextWindowSizeConstraints( ImVec2( comboDropDownSize.x, 0 ), comboDropDownSize );

                ImGui::SameLine();
                bool const wasComboOpen = m_isComboOpen;
                m_isComboOpen = ImGui::BeginCombo( "##DataPath", "", ImGuiComboFlags_HeightLarge | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_NoPreview );
                ImRect const comboButtonRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

                // Regenerate options and clear filter each time we open the combo
                if ( m_isComboOpen && !wasComboOpen )
                {
                    m_filterWidget.Clear();
                    GenerateResourceOptionsList();
                    GenerateFilteredOptionList();
                }

                // Draw combo if open
                bool shouldUpdateNavID = false;
                if ( m_isComboOpen )
                {
                    float const cursorPosYPreFilter = ImGui::GetCursorPosY();
                    if ( m_filterWidget.UpdateAndDraw( -1, ImGuiX::FilterWidget::Flags::TakeInitialFocus ) )
                    {
                        GenerateFilteredOptionList();
                    }
                    float const cursorPosYPostFilter = ImGui::GetCursorPosY();
                    float const filterHeight = cursorPosYPostFilter;

                    ImVec2 const previousCursorPos = ImGui::GetCursorPos();
                    ImVec2 const childSize( ImGui::GetContentRegionAvail().x, comboDropDownSize.y - filterHeight - style.ItemSpacing.y - style.WindowPadding.y );
                    ImGui::Dummy( childSize );
                    ImGui::SetCursorPos( previousCursorPos );

                    //-------------------------------------------------------------------------

                    if ( ImGui::BeginChild( "##OptList", childSize, false, ImGuiWindowFlags_NavFlattened ) )
                    {
                        ImGuiX::ScopedFont const sfo( ImGuiX::Font::Medium );

                        ImGuiListClipper clipper;
                        clipper.Begin( (int32_t) m_filteredOptions.size() );
                        while ( clipper.Step() )
                        {
                            for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                            {
                                if ( ImGui::Selectable( m_filteredOptions[i].c_str() + 7 ) )
                                {
                                    m_resourceID = m_filteredOptions[i];
                                    valueUpdated = true;
                                    ImGui::CloseCurrentPopup();
                                }
                                ImGuiX::ItemTooltip( m_filteredOptions[i].c_str() );
                            }
                        }
                        clipper.End();
                    }
                    ImGui::EndChild();

                    ImGui::EndCombo();
                }
            }
            ImGui::EndDisabled();

            // Buttons
            //-------------------------------------------------------------------------

            {
                // Open Resource
                ImGui::SameLine( 0, itemSpacingX );
                ImGui::BeginDisabled( !m_resourceID.IsValid() );
                if ( ImGui::Button( EE_ICON_OPEN_IN_NEW "##Open", g_buttonSize ) )
                {
                    m_toolsContext.TryOpenResource( m_resourceID );
                }
                ImGuiX::ItemTooltip( "Open Resource" );
                ImGui::EndDisabled();

                // Options
                ImGui::SameLine( 0, itemSpacingX );
                if ( ImGui::Button( EE_ICON_COG "##Options", g_buttonSize ) )
                {
                    ImGui::OpenPopup( "##ResourcePickerOptions" );
                }
                ImGuiX::ItemTooltip( "Options" );
            }

            // Options Context Menu
            //-------------------------------------------------------------------------

            if ( ImGui::BeginPopup( "##ResourcePickerOptions" ) )
            {
                if ( ImGui::MenuItem( EE_ICON_FILE_OUTLINE " Copy Resource Path" ) )
                {
                    ImGui::SetClipboardText( m_resourceID.c_str() );
                }

                if ( ImGui::MenuItem( EE_ICON_FOLDER_OPEN_OUTLINE " Show In Resource Browser" ) )
                {
                    m_toolsContext.TryFindInResourceBrowser( m_resourceID );
                }

                ImGui::Separator();

                if ( ImGui::MenuItem( EE_ICON_FILE " Copy File Path" ) )
                {
                    FileSystem::Path const fileSystemPath = m_resourceID.GetFileSystemPath( m_toolsContext.GetSourceDataDirectory() );
                    ImGui::SetClipboardText( fileSystemPath.c_str() );
                }

                if ( ImGui::MenuItem( EE_ICON_FOLDER_OPEN " Open In Explorer" ) )
                {
                    FileSystem::Path const fileSystemPath = m_resourceID.GetFileSystemPath( m_toolsContext.GetSourceDataDirectory() );
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
        m_generatedOptions.clear();

        if ( m_pCustomOptionProvider != nullptr )
        {
            EE_ASSERT( m_resourceTypeID.IsValid() );
            EE_ASSERT( m_toolsContext.m_pTypeRegistry->IsRegisteredResourceType( m_resourceTypeID ) );
            m_pCustomOptionProvider->GenerateOptions( m_toolsContext, m_generatedOptions );
        }
        else
        {
            // Restrict options to specified resource type ID
            if ( m_resourceTypeID.IsValid() )
            {
                EE_ASSERT( m_toolsContext.m_pTypeRegistry->IsRegisteredResourceType( m_resourceTypeID ) );

                if ( m_customResourceFilter == nullptr )
                {
                    for ( auto const& resourceID : m_toolsContext.m_pFileRegistry->GetAllResourcesOfType( m_resourceTypeID ) )
                    {
                        m_generatedOptions.emplace_back( resourceID );
                    }
                }
                else // Apply custom filter
                {
                    for ( auto const& resourceID : m_toolsContext.m_pFileRegistry->GetAllResourcesOfTypeFiltered( m_resourceTypeID, m_customResourceFilter ) )
                    {
                        m_generatedOptions.emplace_back( resourceID );
                    }
                }
            }
            else // All resource options are valid
            {
                for ( auto const& resourceListPair : m_toolsContext.m_pFileRegistry->GetAllResources() )
                {
                    for ( FileRegistry::FileInfo const* pResourceFileInfo : resourceListPair.second )
                    {
                        EE_ASSERT( pResourceFileInfo != nullptr );
                        EE_ASSERT( pResourceFileInfo->m_dataPath.IsValid() );

                        if ( m_customResourceFilter == nullptr )
                        {
                            m_generatedOptions.emplace_back( pResourceFileInfo->m_dataPath );
                        }
                        else // Apply custom filter
                        {
                            if ( pResourceFileInfo->HasLoadedDescriptor() && m_customResourceFilter( TryCast< Resource::ResourceDescriptor>( pResourceFileInfo->m_pDataFile ) ) )
                            {
                                m_generatedOptions.emplace_back( pResourceFileInfo->m_dataPath );
                            }
                        }
                    }
                }
            }
        }
    }

    void ResourcePicker::GenerateFilteredOptionList()
    {
        if ( m_filterWidget.HasFilterSet() )
        {
            m_filteredOptions.clear();

            for ( auto const& resourceID : m_generatedOptions )
            {
                String lowercasePath = resourceID.GetResourcePath().GetString();
                lowercasePath.make_lower();

                if ( m_filterWidget.MatchesFilter( lowercasePath ) )
                {
                    m_filteredOptions.emplace_back( resourceID );
                }
            }
        }
        else
        {
            m_filteredOptions = m_generatedOptions;
        }
    }

    void ResourcePicker::SetRequiredResourceType( ResourceTypeID resourceTypeID )
    {
        m_resourceTypeID = resourceTypeID;

        // Check if we have a custom option provider for this type
        //-------------------------------------------------------------------------

        OptionProvider* pCurrentProvider = OptionProvider::s_pHead;
        while ( pCurrentProvider != nullptr )
        {
            if ( m_resourceTypeID == pCurrentProvider->GetApplicableResourceTypeID() )
            {
                m_pCustomOptionProvider = pCurrentProvider;
                break;
            }

            pCurrentProvider = pCurrentProvider->GetNextItem();
        }

        // Generate options
        //-------------------------------------------------------------------------

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

    bool ResourcePicker::TryUpdateResourceFromClipboard()
    {
        String clipboardText = ImGui::GetClipboardText();

        if ( clipboardText.length() > 256 )
        {
            EE_LOG_WARNING( "Resource", "ResourcePicker", "Pasting invalid length string" );
            return false;
        }

        // Check for a valid file path if the resource path is bad
        //-------------------------------------------------------------------------

        DataPath resourcePath;

        if ( !DataPath::IsValidPath( clipboardText ) )
        {
            FileSystem::Path pastedFilePath( clipboardText );
            if ( pastedFilePath.IsValid() && pastedFilePath.IsUnderDirectory( m_toolsContext.GetSourceDataDirectory() ) )
            {
                resourcePath = DataPath::FromFileSystemPath( m_toolsContext.GetSourceDataDirectory().c_str(), pastedFilePath );
            }
        }
        else
        {
            resourcePath = DataPath( clipboardText );
        }

        if ( !resourcePath.IsValid() )
        {
            return false;
        }

        // Validate resource
        //-------------------------------------------------------------------------

        ResourceID potentialNewResourceID( resourcePath );

        // Validate resource type
        if ( m_resourceTypeID.IsValid() )
        {
            if ( potentialNewResourceID.GetResourceTypeID() != m_resourceTypeID )
            {
                return false;
            }
        }

        m_resourceID = potentialNewResourceID;
        return true;
    }

    bool ResourcePicker::TryUpdateResourceFromDragAndDrop()
    {
        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( DragAndDrop::s_filePayloadID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( payload->IsDelivery() )
            {
                ResourceID const droppedResourceID( (char*) payload->Data );
                if ( droppedResourceID.IsValid() && droppedResourceID.GetResourceTypeID() == m_resourceTypeID )
                {
                    m_resourceID = droppedResourceID;
                    return true;
                }
            }
        }

        return false;
    }
}

//-------------------------------------------------------------------------

namespace EE
{
    TypeInfoPicker::TypeInfoPicker( ToolsContext const& toolsContext, TypeSystem::TypeID const& baseClassTypeID, TypeSystem::TypeInfo const* pSelectedTypeInfo )
        : m_toolsContext( toolsContext )
        , m_baseClassTypeID( baseClassTypeID )
        , m_pSelectedTypeInfo( pSelectedTypeInfo )
    {
        if ( m_baseClassTypeID.IsValid() && pSelectedTypeInfo != nullptr )
        {
            EE_ASSERT( pSelectedTypeInfo->IsDerivedFrom( m_baseClassTypeID ) );
        }

        m_filterWidget.SetFilterHelpText( "Filter Types" );

        GenerateOptionsList();
        GenerateFilteredOptionList();
    }

    bool TypeInfoPicker::UpdateAndDraw()
    {
        bool valueUpdated = false;

        //-------------------------------------------------------------------------
        // Draw Picker
        //-------------------------------------------------------------------------

        if ( m_isPickerDisabled )
        {
            InlineString typeInfoLabel;
            if ( m_pSelectedTypeInfo != nullptr )
            {
                typeInfoLabel.sprintf( ConstructFriendlyTypeInfoName( m_pSelectedTypeInfo ).c_str() );
            }
            else
            {
                typeInfoLabel = "No Type selected";
            }

            ImGui::SetNextItemWidth( -1 );
            ImGui::InputText( "##typeInfoLabel", const_cast<char*>( typeInfoLabel.c_str() ), typeInfoLabel.length(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );
        }
        else
        {
            ImGui::PushID( this );
            auto const& style = ImGui::GetStyle();
            float const itemSpacingX = style.ItemSpacing.x;
            float const contentRegionAvailableX = ImGui::GetContentRegionAvail().x;
            float const buttonWidth = ImGui::GetFrameHeight() + style.ItemSpacing.x;

            // Type Label
            //-------------------------------------------------------------------------

            InlineString typeInfoLabel;
            if ( m_pSelectedTypeInfo != nullptr )
            {
                typeInfoLabel.sprintf( ConstructFriendlyTypeInfoName( m_pSelectedTypeInfo ).c_str() );
            }
            else
            {
                typeInfoLabel = "No Type selected";
            }

            ImGui::SetNextItemWidth( contentRegionAvailableX - ( buttonWidth * 2 ) - style.ItemSpacing.x );
            ImGui::InputText( "##typeInfoLabel", const_cast<char*>( typeInfoLabel.c_str() ), typeInfoLabel.length(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );

            // Combo
            //-------------------------------------------------------------------------

            ImVec2 const comboDropDownSize( Math::Max( contentRegionAvailableX, 500.0f ), ImGui::GetFrameHeight() * 20 );
            ImGui::SetNextWindowSizeConstraints( ImVec2( comboDropDownSize.x, 0 ), comboDropDownSize );

            ImGui::SameLine();

            bool const wasComboOpen = m_isComboOpen;
            m_isComboOpen = ImGui::BeginCombo( "##TypeInfoCombo", "", ImGuiComboFlags_HeightLarge | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_NoPreview );

            // Draw combo if open
            bool shouldUpdateNavID = false;
            if ( m_isComboOpen )
            {
                float const cursorPosYPreFilter = ImGui::GetCursorPosY();
                if ( m_filterWidget.UpdateAndDraw( -1, ImGuiX::FilterWidget::Flags::TakeInitialFocus ) )
                {
                    GenerateFilteredOptionList();
                }

                float const cursorPosYPostHeaderItems = ImGui::GetCursorPosY();
                ImVec2 const previousCursorPos = ImGui::GetCursorPos();
                ImVec2 const childSize( ImGui::GetContentRegionAvail().x, comboDropDownSize.y - cursorPosYPostHeaderItems - style.ItemSpacing.y - style.WindowPadding.y );
                ImGui::Dummy( childSize );
                ImGui::SetCursorPos( previousCursorPos );

                //-------------------------------------------------------------------------

                InlineString selectableLabel;
                {
                    ImGuiX::ScopedFont const sfo( ImGuiX::Font::Medium );

                    ImGuiListClipper clipper;
                    clipper.Begin( (int32_t) m_filteredOptions.size() );
                    while ( clipper.Step() )
                    {
                        for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                        {
                            EE_ASSERT( m_filteredOptions[i].m_pTypeInfo != nullptr );

                            selectableLabel.sprintf( "%s##%d", m_filteredOptions[i].m_friendlyName.c_str(), m_filteredOptions[i].m_pTypeInfo->m_ID.ToUint() );
                            if ( ImGui::Selectable( selectableLabel.c_str() ) )
                            {
                                m_pSelectedTypeInfo = m_filteredOptions[i].m_pTypeInfo;
                                valueUpdated = true;
                                ImGui::CloseCurrentPopup();
                            }
                            ImGuiX::ItemTooltip( m_filteredOptions[i].m_friendlyName.c_str() );
                        }
                    }
                    clipper.End();
                }

                ImGui::EndCombo();
            }

            // Clear Button
            //-------------------------------------------------------------------------

            ImGui::SameLine();

            ImGui::BeginDisabled( m_pSelectedTypeInfo == nullptr );
            if ( ImGui::Button( EE_ICON_TRASH_CAN"##Clear", ImVec2( buttonWidth, 0 ) ) )
            {
                m_pSelectedTypeInfo = nullptr;
                valueUpdated = true;
            }
            ImGuiX::ItemTooltip( "Clear Selected Type" );
            ImGui::EndDisabled();
            ImGui::PopID();
        }

        //-------------------------------------------------------------------------

        return valueUpdated;
    }

    void TypeInfoPicker::SetRequiredBaseClass( TypeSystem::TypeID baseClassTypeID )
    {
        m_baseClassTypeID = baseClassTypeID;
        GenerateOptionsList();
        GenerateFilteredOptionList();
    }

    void TypeInfoPicker::SetSelectedType( TypeSystem::TypeInfo const* pSelectedTypeInfo )
    {
        if ( m_baseClassTypeID.IsValid() && pSelectedTypeInfo != nullptr )
        {
            EE_ASSERT( pSelectedTypeInfo->IsDerivedFrom( m_baseClassTypeID ) );
            m_pSelectedTypeInfo = pSelectedTypeInfo;
        }
    }

    void TypeInfoPicker::SetSelectedType( TypeSystem::TypeID typeID )
    {
        if ( typeID.IsValid() )
        {
            m_pSelectedTypeInfo = m_toolsContext.m_pTypeRegistry->GetTypeInfo( typeID );
        }
        else
        {
            m_pSelectedTypeInfo = nullptr;
        }
    }

    void TypeInfoPicker::GenerateOptionsList()
    {
        m_generatedOptions.clear();

        TVector<TypeSystem::TypeInfo const*> validTypes;
        if ( m_baseClassTypeID.IsValid() )
        {
            validTypes = m_toolsContext.m_pTypeRegistry->GetAllDerivedTypes( m_baseClassTypeID, true, false, true );
        }
        else
        {
            validTypes = m_toolsContext.m_pTypeRegistry->GetAllTypes( false, true );
        }

        for ( auto pTypeInfo : validTypes )
        {
            auto& newOption = m_generatedOptions.emplace_back();
            newOption.m_pTypeInfo = pTypeInfo;
            newOption.m_friendlyName = ConstructFriendlyTypeInfoName( pTypeInfo );

            // Generate a string that will allow us to filter the list
            newOption.m_filterableData.sprintf( "%s %s", pTypeInfo->m_category.c_str(), newOption.m_friendlyName.c_str() );
            StringUtils::RemoveAllOccurrencesInPlace( newOption.m_filterableData, ":" );
            StringUtils::RemoveAllOccurrencesInPlace( newOption.m_filterableData, "/" );
            newOption.m_filterableData.make_lower();
        }
    }

    void TypeInfoPicker::GenerateFilteredOptionList()
    {
        if( m_filterWidget.HasFilterSet() )
        {
            m_filteredOptions.clear();

            for ( Option const& option : m_generatedOptions )
            {
                if ( m_filterWidget.MatchesFilter( option.m_filterableData ) )
                {
                    m_filteredOptions.emplace_back( option );
                }
            }
        }
        else
        {
            m_filteredOptions = m_generatedOptions;
        }
    }

    String TypeInfoPicker::ConstructFriendlyTypeInfoName( TypeSystem::TypeInfo const* pTypeInfo )
    {
        String friendlyName;

        if ( pTypeInfo->m_namespace.empty() )
        {
            friendlyName = pTypeInfo->m_friendlyName;
        }
        else
        {
            friendlyName.sprintf( "%s::%s", pTypeInfo->m_namespace.c_str(), pTypeInfo->m_friendlyName.c_str() );
        }

        return friendlyName;
    }
}