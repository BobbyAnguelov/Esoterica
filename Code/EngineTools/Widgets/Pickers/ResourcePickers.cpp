#include "ResourcePickers.h"
#include "EngineTools/FileSystem/FileRegistry.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Core/CommonToolTypes.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include "Base/TypeSystem/DataFileInfo.h"
#include "Base/TypeSystem/ResourceInfo.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE
{
    DataPickerBase::DataPickerBase( ToolsContext const& toolsContext )
        : m_toolsContext( toolsContext )
    {
        float const controlsHeight = ImGuiX::CalculateButtonHeight( EE_ICON_ABACUS );
        ImGuiX::ScopedFont const sfx( ImGuiX::Font::Small );
        m_height = ( ImGui::GetFrameHeight() + s_controlsRowGapY + controlsHeight );
    }

    bool DataPickerBase::UpdateAndDraw()
    {
        bool valueUpdated = false;

        //-------------------------------------------------------------------------
        // Validation
        //-------------------------------------------------------------------------

        bool const isValidAndExistingPath = ValidateCurrentlySetPath();
        DataPath const currentPath = GetDataPath();

        //-------------------------------------------------------------------------
        // Draw Picker
        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        if ( ImGui::BeginChild( "RP", ImVec2( -1, 0 ), ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
        {
            auto const& style = ImGui::GetStyle();
            float const itemSpacingX = style.ItemSpacing.x;

            ImGuiX::ScopedFont const sfx( ImGuiX::Font::Small );

            // Preview
            //-------------------------------------------------------------------------

            {
                TInlineString<7> const previewStr = GetPreviewLabel();
                Color const previewColor = GetPreviewColor();

                ImGui::PushStyleColor( ImGuiCol_Button, ImGuiX::Style::s_colorGray0 );
                ImGui::BeginDisabled( !isValidAndExistingPath );
                {
                    ImGuiX::ScopedFont const sf( ImGuiX::Font::TinyBold, previewColor );
                    ImGui::Button( previewStr.c_str(), ImVec2( m_height, m_height ) );
                }
                ImGui::EndDisabled();
                ImGui::PopStyleColor();

                if ( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                {
                    if ( isValidAndExistingPath && currentPath.IsValid() )
                    {
                        m_toolsContext.TryOpenDataFile( currentPath );
                    }
                }

                ImGui::SameLine();
            }

            // Combo Selector
            //-------------------------------------------------------------------------

            ImGui::BeginGroup();
            ImGui::BeginDisabled( !m_toolsContext.m_pFileRegistry->IsDataFileCacheBuilt() );
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( style.ItemSpacing.x, s_controlsRowGapY ) );

            {
                // Type and path fields
                //-------------------------------------------------------------------------

                float const comboArrowWidth = ImGui::GetFrameHeight();
                float const totalPathWidgetWidth = ImGui::GetContentRegionAvail().x;
                float const textWidgetWidth = totalPathWidgetWidth - comboArrowWidth;

                ImVec4 const pathColor = ( isValidAndExistingPath ? ImGuiX::Style::s_colorText : Colors::Red ).ToFloat4();
                ImGui::PushStyleColor( ImGuiCol_Text, pathColor );
                ImGui::SetNextItemWidth( textWidgetWidth );

                if ( currentPath.IsValid() )
                {
                    size_t const size = currentPath.GetString().length();
                    m_tempBuffer.resize( size + 1 );
                    memcpy( m_tempBuffer.data(), currentPath.c_str(), size );
                    m_tempBuffer.back() = 0;
                }
                else
                {
                    m_tempBuffer.resize( 512 );
                    Memory::MemsetZero( m_tempBuffer.data(), 512 );
                }

                bool openDropDownDueToInput = false;
                if ( ImGui::InputText( "##DataPathText", m_tempBuffer.data(), m_tempBuffer.size(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ElideLeft | ImGuiInputTextFlags_ReadOnly ) )
                {
                    //m_shouldOpenDropDown = true; // This is disabled as there is no way to get imgui to behave properly with switching focus to input text boxes
                    openDropDownDueToInput = true;
                }
                ImGui::PopStyleColor();

                // Drag and drop
                if ( ImGui::BeginDragDropTarget() )
                {
                    valueUpdated = TryUpdatePathFromDragAndDrop();
                    ImGui::EndDragDropTarget();
                }

                // Tooltip
                if ( currentPath.IsValid() )
                {
                    ImGuiX::ItemTooltip( currentPath.c_str() );
                }

                // Allow pasting valid paths
                bool bSwitchFocus = false; // We need to do this to display the pasted value immediately!
                if ( ImGui::IsItemFocused() )
                {
                    if ( ImGui::IsKeyDown( ImGuiMod_Ctrl ) && ImGui::IsKeyPressed( ImGuiKey_V ) )
                    {
                        valueUpdated |= TryUpdatePathFromClipboard();
                        m_shouldOpenDropDown = false;
                        bSwitchFocus = true;
                    }
                    else if ( ImGui::IsKeyDown( ImGuiMod_Ctrl ) && ImGui::IsKeyPressed( ImGuiKey_X ) )
                    {
                        ImGui::SetClipboardText( currentPath.c_str() );
                        SetDataPath( DataPath() );
                        m_shouldOpenDropDown = false;
                        valueUpdated = true;
                    }
                    else if ( ImGui::IsKeyPressed( ImGuiKey_Delete ) || ImGui::IsKeyPressed( ImGuiKey_Backspace ) )
                    {
                        SetDataPath( DataPath() );
                        m_shouldOpenDropDown = false;
                        valueUpdated = true;
                    }
                }

                // Combo
                //-------------------------------------------------------------------------

                ImVec2 const comboDropDownSize( Math::Max( totalPathWidgetWidth, 500.0f ), ImGui::GetFrameHeight() * 20 );
                ImGui::SetNextWindowSizeConstraints( ImVec2( comboDropDownSize.x, 0 ), comboDropDownSize );

                ImGui::SameLine( 0, 0 );
                bool const wasPopupOpen = m_isPopupOpen;

                ImGui::SetNextItemOpen( true );
                bool const didComboBeginReturnTrue = ImGui::BeginCombo( "##DataPath", "", ImGuiComboFlags_HeightLarge | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_NoPreview );
                m_isPopupOpen = didComboBeginReturnTrue;

                if ( m_shouldOpenDropDown && !m_isPopupOpen )
                {
                    // Make sure to open the actual combo popup
                    ImGuiID const popupID = ImHashStr( "##ComboPopup", 0, ImGui::GetID( "##DataPath" ) );
                    ImGui::OpenPopupEx( popupID, ImGuiPopupFlags_None );
                    m_isPopupOpen = ImGui::IsPopupOpen( "##DataPath" );
                }
                m_shouldOpenDropDown = false;

                if( bSwitchFocus )
                {
                    ImGui::FocusItem();
                }

                // Regenerate options and clear filter each time we open the combo
                if ( m_isPopupOpen && !wasPopupOpen )
                {
                    if ( openDropDownDueToInput )
                    {
                        m_filterWidget.SetFilter( m_tempBuffer.data() );
                    }
                    else
                    {
                        m_filterWidget.Clear();
                    }
                    GenerateOptionsList();
                    GenerateFilteredOptionList();
                }

                // Draw combo if open
                bool shouldUpdateNavID = false;
                if ( m_isPopupOpen )
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

                    if ( ImGui::BeginChild( "##OptList", childSize, false, ImGuiChildFlags_NavFlattened ) )
                    {
                        ImGuiX::ScopedFont const sf( ImGuiX::Font::Medium );

                        ImGuiListClipper clipper;
                        clipper.Begin( (int32_t) m_filteredOptions.size() );
                        while ( clipper.Step() )
                        {
                            for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                            {
                                if ( ImGui::Selectable( m_filteredOptions[i].c_str() + 7 ) )
                                {
                                    if ( m_filteredOptions[i] != GetDataPath() )
                                    {
                                        SetDataPath( m_filteredOptions[i] );
                                        valueUpdated = true;
                                    }
                                    ImGui::CloseCurrentPopup();
                                }
                                ImGuiX::ItemTooltip( m_filteredOptions[i].c_str() );
                            }
                        }
                        clipper.End();
                    }
                    ImGui::EndChild();

                    if ( didComboBeginReturnTrue )
                    {
                        ImGui::EndCombo();
                    }
                }
            }

            ImGui::PopStyleVar();
            ImGui::EndDisabled();

            // Buttons
            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );
                static ImVec2 const buttonSize( ImGuiX::Style::s_iconButtonWidthSmall, 0 );

                // Open Resource
                ImGui::BeginDisabled( !currentPath.IsValid() );
                if ( ImGui::Button( EE_ICON_CONTENT_COPY "##Copy", buttonSize ) )
                {
                    ImGui::SetClipboardText( currentPath.c_str() );
                }
                ImGuiX::ItemTooltip( "Copy Data Path" );

                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_FOLDER_SYNC "##ShowInRB", buttonSize ) )
                {
                    m_toolsContext.TryFindInResourceBrowser( currentPath );
                }
                ImGuiX::ItemTooltip( "Show In Resource Browser" );

                if ( m_showDependenciesButton )
                {
                    ImGui::SameLine();
                    if ( ImGui::Button( EE_ICON_GRAPH "##ShowDeps", buttonSize ) )
                    {
                        m_toolsContext.ShowResourceDependencies( currentPath );
                    }
                    ImGuiX::ItemTooltip( "Show Dependencies" );
                }

                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_COG "##Options", buttonSize ) )
                {
                    ImGui::OpenPopup( "##ResourcePickerOptions" );
                }
                ImGuiX::ItemTooltip( "Options" );

                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_CLOSE_CIRCLE "##Clear", buttonSize ) )
                {
                    if ( GetDataPath().IsValid() )
                    {
                        Clear();
                        valueUpdated = true;
                    }
                }
                ImGuiX::ItemTooltip( "Clear" );

                ImGui::EndDisabled();
            }
            ImGui::EndGroup();

            // Options Context Menu
            //-------------------------------------------------------------------------

            if ( ImGui::BeginPopup( "##DataFilePathPickerOptions" ) )
            {
                if ( ImGui::MenuItem( EE_ICON_FILE_OUTLINE " Copy Data Path" ) )
                {
                    ImGui::SetClipboardText( currentPath.c_str() );
                }

                if ( ImGui::MenuItem( EE_ICON_FOLDER_OPEN_OUTLINE " Show In Resource Browser" ) )
                {
                    m_toolsContext.TryFindInResourceBrowser( currentPath );
                }

                ImGui::Separator();

                if ( ImGui::MenuItem( EE_ICON_FILE " Copy File Path" ) )
                {
                    FileSystem::Path const fileSystemPath = currentPath.GetFileSystemPath( m_toolsContext.GetSourceDataDirectory() );
                    ImGui::SetClipboardText( fileSystemPath.c_str() );
                }

                if ( ImGui::MenuItem( EE_ICON_FOLDER_OPEN " Open In Explorer" ) )
                {
                    FileSystem::Path const fileSystemPath = currentPath.GetFileSystemPath( m_toolsContext.GetSourceDataDirectory() );
                    Platform::Win32::OpenInExplorer( fileSystemPath );
                }

                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();

        //-------------------------------------------------------------------------

        ImGui::PopID();

        return valueUpdated;
    }

    TInlineString<7> DataPickerBase::GetPreviewLabel() const
    {
        TInlineString<7> previewStr;

        DataPath const& currentPath = GetDataPath();
        if ( currentPath.IsValid() )
        {
            previewStr = TInlineString<7>( TInlineString<7>::CtorSprintf(), "%s##Preview", currentPath.GetExtension().c_str() );
        }
        else
        {
            previewStr = "##Preview";
        }

        return previewStr;
    }

    void DataPickerBase::GenerateFilteredOptionList()
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

    bool DataPickerBase::TryUpdatePathFromClipboard()
    {
        String clipboardText = ImGui::GetClipboardText();

        if ( clipboardText.length() > 256 )
        {
            EE_LOG_WARNING( LogCategory::Resource, "DataFilePathPicker", "Pasting invalid length string" );
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
                pastedPath = DataPath( pastedFilePath, m_toolsContext.GetSourceDataDirectory().c_str() );
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

        if ( ValidateDataPath( pastedPath ) )
        {
            SetDataPath( pastedPath );
            return true;
        }

        return false;
    }

    bool DataPickerBase::TryUpdatePathFromDragAndDrop()
    {
        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( DragAndDrop::s_filePayloadID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( payload->IsDelivery() )
            {
                DataPath droppedPath( (char*) payload->Data );
                if ( ValidateDataPath( droppedPath ) )
                {
                    SetDataPath( droppedPath );
                    return true;
                }
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    DataFilePathPicker::DataFilePathPicker( ToolsContext const& toolsContext, TypeSystem::TypeID dataFileTypeID, DataPath const& datafilePath )
        : DataPickerBase( toolsContext )
    {
        SetRequiredDataFileType( dataFileTypeID );
        SetDataPath( datafilePath );
    }

    bool DataFilePathPicker::ValidateDataPath( DataPath const& path )
    {
        bool isValidPath = path.IsValid();
        if ( isValidPath )
        {
            // Check extension
            //-------------------------------------------------------------------------

            isValidPath = ( m_requiredExtension.comparei( path.GetExtension() ) == 0 );

            // Check if file exist
            //-------------------------------------------------------------------------

            if ( isValidPath )
            {
                isValidPath = m_toolsContext.m_pFileRegistry->DoesFileExist( path );
            }
        }

        return isValidPath;
    }

    void DataFilePathPicker::SetDataPath( DataPath const& path )
    {
        if ( ValidateDataPath( path ) )
        {
            m_path = path;
        }
        else
        {
            m_path.Clear();
        }
    }

    void DataFilePathPicker::GenerateOptionsList()
    {
        m_generatedOptions.clear();

        if ( m_fileTypeID.IsValid() )
        {
            EE_ASSERT( m_pDataFileInfo != nullptr );

            DataFileExtension const dataFileExt = DataFileExtension( m_requiredExtension.c_str() );
            for ( auto const& pFileInfo : m_toolsContext.m_pFileRegistry->GetAllDataFileEntries( dataFileExt ) )
            {
                m_generatedOptions.emplace_back( pFileInfo->m_dataPath );
            }
        }
        else // Show all data files
        {
            for ( auto const& pFileInfo : m_toolsContext.m_pFileRegistry->GetAllDataFileEntries() )
            {
                m_generatedOptions.emplace_back( pFileInfo->m_dataPath );
            }
        }
    }

    void DataFilePathPicker::SetRequiredDataFileType( TypeSystem::TypeID typeID )
    {
        m_fileTypeID = typeID;
        m_pDataFileInfo = m_toolsContext.m_pTypeRegistry->GetDataFileInfo( typeID );
        EE_ASSERT( m_pDataFileInfo != nullptr ); // Type ID is not a datafile type!

        m_requiredExtension = m_pDataFileInfo->GetFileSystemExtension();

        if ( m_isPopupOpen )
        {
            GenerateOptionsList();
            GenerateFilteredOptionList();
        }
    }

    //-------------------------------------------------------------------------

    ResourcePicker::ResourcePicker( ToolsContext const& toolsContext, ResourceTypeID resourceTypeID, ResourceID const& resourceID )
        : DataPickerBase( toolsContext )
    {
        SetRequiredResourceType( resourceTypeID );
        SetResourceID( resourceID );
        m_showDependenciesButton = true;
    }

    TInlineString<7> ResourcePicker::GetPreviewLabel() const
    {
        TInlineString<7> previewStr;

        if ( m_resourceTypeID.IsValid() )
        {
            previewStr = TInlineString<7>( TInlineString<7>::CtorSprintf(), "%s##Preview", m_resourceTypeID.ToString().c_str() );
        }
        else
        {
            previewStr = DataPickerBase::GetPreviewLabel();
        }

        return previewStr;
    }

    Color ResourcePicker::GetPreviewColor() const
    {
        Color previewColor = ImGuiX::Style::s_colorText;

        if ( m_pResourceTypeInfo != nullptr )
        {
            previewColor = m_pResourceTypeInfo->m_color;
        }

        return previewColor;
    }

    bool ResourcePicker::ValidateDataPath( DataPath const& path )
    {
        bool isValidPath = true;

        if ( path.IsValid() )
        {
            // Check resource TypeID
            //-------------------------------------------------------------------------

            // Get actual resource typeID
            ResourceTypeID actualResourceTypeID;
            FileSystem::Extension const extension = path.GetExtension();
            if ( !extension.empty() )
            {
                if ( ResourceTypeID::IsValidResourceTypeIdentifierString( extension ) )
                {
                    actualResourceTypeID = ResourceTypeID( extension );
                }
            }

            if ( !actualResourceTypeID.IsValid() )
            {
                return false;
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

            // Custom validation
            //-------------------------------------------------------------------------

            if ( isValidPath && m_pCustomOptionProvider != nullptr )
            {
                isValidPath = m_pCustomOptionProvider->ValidatePath( m_toolsContext, path );
            }

            // Check if file exist
            //-------------------------------------------------------------------------

            if ( isValidPath )
            {
                isValidPath = m_toolsContext.m_pFileRegistry->DoesFileExist( path );
            }
        }
        else
        {
            isValidPath = false;
        }

        return isValidPath;
    }

    void ResourcePicker::GenerateOptionsList()
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
                        m_generatedOptions.emplace_back( resourceID.GetDataPath() );
                    }
                }
                else // Apply custom filter
                {
                    for ( auto const& resourceID : m_toolsContext.m_pFileRegistry->GetAllResourcesOfTypeFiltered( m_resourceTypeID, m_customResourceFilter ) )
                    {
                        m_generatedOptions.emplace_back( resourceID.GetDataPath() );
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
                            if ( pResourceFileInfo->HasLoadedDescriptor() && m_customResourceFilter( TryCast<Resource::ResourceDescriptor>( pResourceFileInfo->m_pDataFile ) ) )
                            {
                                m_generatedOptions.emplace_back( pResourceFileInfo->m_dataPath );
                            }
                        }
                    }
                }
            }
        }

        //-------------------------------------------------------------------------

        eastl::sort( m_generatedOptions.begin(), m_generatedOptions.end(), SortComparison_DataPath );
    }

    void ResourcePicker::SetRequiredResourceType( ResourceTypeID resourceTypeID )
    {
        m_resourceTypeID = resourceTypeID;
        m_pResourceTypeInfo = nullptr;

        // Get the resource type info
        //-------------------------------------------------------------------------

        if ( m_resourceTypeID.IsValid() )
        {
            m_pResourceTypeInfo = m_toolsContext.m_pTypeRegistry->GetResourceInfo( m_resourceTypeID );
            EE_ASSERT( m_pResourceTypeInfo != nullptr );
        }

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
    }

    void ResourcePicker::SetCustomResourceFilter( TFunction<bool( Resource::ResourceDescriptor const* )>&& filter )
    {
        m_customResourceFilter = filter;

        if ( m_isPopupOpen )
        {
            GenerateOptionsList();
            GenerateFilteredOptionList();
        }
    }

    void ResourcePicker::ClearCustomResourceFilter()
    {
        m_customResourceFilter = nullptr;

        if ( m_isPopupOpen )
        {
            GenerateOptionsList();
            GenerateFilteredOptionList();
        }
    }

    void ResourcePicker::SetResourceID( ResourceID const& resourceID )
    {
        if ( resourceID.IsValid() && m_resourceTypeID.IsValid() )
        {
            EE_ASSERT( resourceID.GetResourceTypeID() == m_resourceTypeID );
        }

        if ( m_resourceID != resourceID )
        {
            m_resourceID = resourceID;
        }
    }

    void ResourcePicker::SetDataPath( DataPath const& path )
    {
        if ( ValidateDataPath( path ) )
        {
            m_resourceID = ResourceID( path );
        }
        else
        {
            m_resourceID.Clear();
        }
    }
}