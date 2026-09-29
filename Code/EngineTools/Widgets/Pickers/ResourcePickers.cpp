#include "ResourcePickers.h"
#include "EngineTools/FileSystem/DataFileRegistry.h"
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
        m_optionData.SetOptionProvider( [this] ( TVector<ImGuiX::OptionData::Option>& outOptions ) { GenerateOptionsList( outOptions ); } );
    }

    bool DataPickerBase::UpdateAndDraw()
    {
        bool valueUpdated = false;

        //-------------------------------------------------------------------------
        // Validation
        //-------------------------------------------------------------------------

        bool const isValidAndExistingPath = ValidateCurrentlySetPath();

        //-------------------------------------------------------------------------
        // Draw Picker
        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        auto const& style = ImGui::GetStyle();
        float const itemSpacingX = style.ItemSpacing.x;

        ImGuiX::ScopedFont const sfx( ImGuiX::Font::Small );

        float const fullHeight = ( ImGui::GetFrameHeight() + s_controlsRowGapY + ImGuiX::CalculateButtonHeight( EE_ICON_ABACUS ) );
        m_height = m_isCompact ? ImGui::GetFrameHeight() : fullHeight;

        // Preview
        //-------------------------------------------------------------------------

        {
            TInlineString<7> const previewStr = GetPreviewLabel();
            Color const previewColor = GetPreviewColor();

            ImGui::PushStyleColor( ImGuiCol_Button, ImGuiX::Style::s_colorGray0 );
            ImGui::BeginDisabled( !isValidAndExistingPath );
            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::TinyBold, previewColor );
                ImGui::Button( previewStr.c_str(), ImVec2( fullHeight, m_height ) );

                if ( ImGui::BeginDragDropTarget() )
                {
                    valueUpdated = TryUpdatePathFromDragAndDrop();
                    ImGui::EndDragDropTarget();
                }
            }
            ImGui::EndDisabled();
            ImGui::PopStyleColor();

            if ( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
            {
                if ( isValidAndExistingPath && m_path.IsValid() )
                {
                    m_toolsContext.TryOpenDataFile( m_path );
                }
            }

            ImGui::SameLine();
        }

        // Combo Selector
        //-------------------------------------------------------------------------

        ImGui::BeginGroup();
        ImGui::BeginDisabled( !m_toolsContext.m_pDataFileRegistry->IsDataFileCacheBuilt() );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( style.ItemSpacing.x, s_controlsRowGapY ) );

        {
            // Text Input
            //-------------------------------------------------------------------------

            auto Callback = [] ( ImGuiInputTextCallbackData* pData ) -> int
            {
                auto* pCtx = (DataPickerBase*) pData->UserData;

                if ( pData->EventFlag == ImGuiInputTextFlags_CallbackResize )
                {
                    pCtx->m_buffer.Resize( pData->BufTextLen );
                    pData->Buf = pCtx->m_buffer.Data();
                }

                return 0;
            };

            float const optionsMenuWidth = m_isCompact ? ( ImGuiX::Style::s_iconButtonWidthSmall + style.ItemSpacing.x ) : 0;
            float const textWidgetWidth = ImGui::GetContentRegionAvail().x - optionsMenuWidth;

            m_optionData.m_preWidgetFunc = [this, isValidAndExistingPath] ()
            {
                ImVec4 pathColor = ImGuiX::Style::s_colorText;

                // Only change the color when there is a path set
                if ( m_path.IsValid() )
                {
                    pathColor = ( isValidAndExistingPath ? ImGuiX::Style::s_colorText : Colors::Red ).ToFloat4();
                }

                ImGui::PushStyleColor( ImGuiCol_Text, pathColor );
            };

            m_optionData.m_postWidgetFunc = [this, &valueUpdated] ()
            {
                ImGui::PopStyleColor();

                // Drag and drop
                if ( ImGui::BeginDragDropTarget() )
                {
                    valueUpdated = TryUpdatePathFromDragAndDrop();
                    ImGui::EndDragDropTarget();
                }
            };

            ImGui::SetNextItemWidth( textWidgetWidth );
            if ( ImGuiX::InputTextWithOptions( "IT", m_buffer.Data(), m_buffer.Size(), &m_optionData, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ElideLeft | ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_EnterReturnsTrue, Callback, this ) )
            {
                if ( UpdateDataPathFromString( m_buffer.GetString() ) )
                {
                    valueUpdated = true;
                }
            }

            // Options Button
            //-------------------------------------------------------------------------

            if ( m_isCompact )
            {
                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_COG "##Options", ImVec2( ImGuiX::Style::s_iconButtonWidthSmall, 0 ) ) )
                {
                    ImGui::OpenPopup( "##DataFilePathPickerOptions" );
                }
                ImGuiX::ItemTooltip( "Options" );
            }
        }

        ImGui::PopStyleVar();
        ImGui::EndDisabled();

        // Control Row
        //-------------------------------------------------------------------------

        if ( !m_isCompact )
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );
            static ImVec2 const buttonSize( ImGuiX::Style::s_iconButtonWidthSmall, 0 );

            // Open Resource
            ImGui::BeginDisabled( !m_path.IsValid() );
            if ( ImGui::Button( EE_ICON_CONTENT_COPY "##Copy", buttonSize ) )
            {
                ImGui::SetClipboardText( m_path.c_str() );
            }
            ImGuiX::ItemTooltip( "Copy Data Path" );

            ImGui::SameLine();
            if ( ImGui::Button( EE_ICON_FOLDER_SYNC "##ShowInRB", buttonSize ) )
            {
                m_toolsContext.TryFindInResourceBrowser( m_path );
            }
            ImGuiX::ItemTooltip( "Show In Resource Browser" );

            if ( m_showDependenciesButton )
            {
                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_GRAPH "##ShowDeps", buttonSize ) )
                {
                    m_toolsContext.ShowResourceDependencies( m_path );
                }
                ImGuiX::ItemTooltip( "Show Dependencies" );
            }

            ImGui::SameLine();
            if ( ImGui::Button( EE_ICON_COG "##Options", buttonSize ) )
            {
                ImGui::OpenPopup( "##DataFilePathPickerOptions" );
            }
            ImGuiX::ItemTooltip( "Options" );

            ImGui::EndDisabled();
        }
        ImGui::EndGroup();

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

            if ( ImGui::MenuItem( EE_ICON_GRAPH " Show Dependencies" ) )
            {
                m_toolsContext.ShowResourceDependencies( m_path );
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

            ImGui::EndPopup();
        }

        //-------------------------------------------------------------------------

        ImGui::PopID();

        return valueUpdated;
    }

    void DataPickerBase::SetDataPath( DataPath const& path )
    {
        if ( ValidateDataPath( path ) )
        {
            m_path = path;
        }
        else
        {
            m_path.Clear();
        }

        m_buffer.Fill( m_path.GetString() );
    }

    void DataPickerBase::Clear()
    {
        m_buffer.Clear();
        SetDataPath( DataPath() );
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

    bool DataPickerBase::ValidateCurrentlySetPath()
    {
        return ValidateDataPath( m_path ) && m_toolsContext.m_pDataFileRegistry->DoesFileExist( m_path );
    }

    void DataPickerBase::GenerateOptionsList( TVector<ImGuiX::OptionData::Option>& outOptions )
    {
        GenerateDataPathOptions();

        //-------------------------------------------------------------------------

        for ( DataPath const& dataPath : m_dataPathOptions )
        {
            outOptions.emplace_back( dataPath.GetString(), dataPath.GetID() );
        }
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

    bool DataPickerBase::UpdateDataPathFromString( String const& str )
    {
        if ( str.empty() )
        {
            if ( m_path.IsValid() )
            {
                Clear();
                return true;
            }

            return false;
        }

        //-------------------------------------------------------------------------

        String enteredPath = str;
        if ( !enteredPath.empty() && !StringUtils::StartsWith( str, DataPath::s_pathPrefix ) )
        {
            enteredPath.sprintf( "%s%s", DataPath::s_pathPrefix, str.c_str() );
        }

        DataPath dataPath( enteredPath, DataPath::AllowInvalidPath );
        if ( !dataPath.IsValid() || !ValidateDataPath( dataPath ) || dataPath == m_path )
        {
            m_buffer.Fill( GetDataPath().c_str() );
            return false;
        }

        SetDataPath( dataPath );
        return true;
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
        if ( !path.IsValid() )
        {
            return false;
        }

        if ( m_requiredExtension.comparei( path.GetExtension() ) != 0 )
        {
            return false;
        }

        return true;
    }

    void DataFilePathPicker::GenerateDataPathOptions()
    {
        m_dataPathOptions.clear();

        if ( m_fileTypeID.IsValid() )
        {
            EE_ASSERT( m_pDataFileInfo != nullptr );

            DataFileExtension const dataFileExt = DataFileExtension( m_requiredExtension.c_str() );
            for ( auto const& pFileInfo : m_toolsContext.m_pDataFileRegistry->GetAllDataFileEntries( dataFileExt ) )
            {
                m_dataPathOptions.emplace_back( pFileInfo->m_dataPath );
            }
        }
        else // Show all data files
        {
            for ( auto const& pFileInfo : m_toolsContext.m_pDataFileRegistry->GetAllDataFileEntries() )
            {
                m_dataPathOptions.emplace_back( pFileInfo->m_dataPath );
            }
        }
    }

    void DataFilePathPicker::SetRequiredDataFileType( TypeSystem::TypeID typeID )
    {
        m_fileTypeID = typeID;
        m_pDataFileInfo = m_toolsContext.m_pTypeRegistry->GetDataFileInfo( typeID );
        EE_ASSERT( m_pDataFileInfo != nullptr ); // Type ID is not a datafile type!

        m_requiredExtension = m_pDataFileInfo->GetFileSystemExtension();
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
        if ( !path.IsValid() )
        {
            return false;
        }

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
                return false;
            }
        }
        else
        {
            if ( !m_toolsContext.m_pTypeRegistry->IsRegisteredResourceType( actualResourceTypeID ) )
            {
                return false;
            }
        }

        // Custom validation
        //-------------------------------------------------------------------------

        if ( m_pCustomOptionProvider != nullptr )
        {
            if ( !m_pCustomOptionProvider->ValidatePath( m_toolsContext, path ) )
            {
                return false;
            }
        }

        return true;
    }

    void ResourcePicker::GenerateDataPathOptions()
    {
        m_dataPathOptions.clear();

        if ( m_pCustomOptionProvider != nullptr )
        {
            EE_ASSERT( m_resourceTypeID.IsValid() );
            EE_ASSERT( m_toolsContext.m_pTypeRegistry->IsRegisteredResourceType( m_resourceTypeID ) );
            m_pCustomOptionProvider->GenerateOptions( m_toolsContext, m_dataPathOptions );
        }
        else
        {
            // Restrict options to specified resource type ID
            if ( m_resourceTypeID.IsValid() )
            {
                EE_ASSERT( m_toolsContext.m_pTypeRegistry->IsRegisteredResourceType( m_resourceTypeID ) );

                if ( m_customResourceFilter == nullptr )
                {
                    for ( auto const& resourceID : m_toolsContext.m_pDataFileRegistry->GetAllResourcesOfType( m_resourceTypeID ) )
                    {
                        m_dataPathOptions.emplace_back( resourceID.GetDataPath() );
                    }
                }
                else // Apply custom filter
                {
                    for ( auto const& resourceID : m_toolsContext.m_pDataFileRegistry->GetAllResourcesOfTypeFiltered( m_resourceTypeID, m_customResourceFilter ) )
                    {
                        m_dataPathOptions.emplace_back( resourceID.GetDataPath() );
                    }
                }
            }
            else // All resource options are valid
            {
                for ( auto const& resourceListPair : m_toolsContext.m_pDataFileRegistry->GetAllResources() )
                {
                    for ( DataFileRegistry::FileInfo const* pResourceFileInfo : resourceListPair.second )
                    {
                        EE_ASSERT( pResourceFileInfo != nullptr );
                        EE_ASSERT( pResourceFileInfo->m_dataPath.IsValid() );

                        if ( m_customResourceFilter == nullptr )
                        {
                            m_dataPathOptions.emplace_back( pResourceFileInfo->m_dataPath );
                        }
                        else // Apply custom filter
                        {
                            if ( pResourceFileInfo->HasLoadedDescriptor() && m_customResourceFilter( TryCast<Resource::ResourceDescriptor>( pResourceFileInfo->m_pDataFile ) ) )
                            {
                                m_dataPathOptions.emplace_back( pResourceFileInfo->m_dataPath );
                            }
                        }
                    }
                }
            }
        }
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

    void ResourcePicker::SetResourceID( ResourceID const& resourceID )
    {
        if ( resourceID.IsValid() && m_resourceTypeID.IsValid() )
        {
            EE_ASSERT( resourceID.GetResourceTypeID() == m_resourceTypeID );
        }

        SetDataPath( resourceID.GetDataPath() );
    }
}