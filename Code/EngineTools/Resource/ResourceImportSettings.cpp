#include "ResourceImportSettings.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Core/Dialogs.h"
#include "Base/TypeSystem/ResourceInfo.h"
#include "Base/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ImportSettings::ImportSettings( ToolsContext const* pToolsContext )
        : m_pToolsContext( pToolsContext )
        , m_propertyGrid( pToolsContext )
    {}

    bool ImportSettings::UpdateAndDraw( DataPath& outCreatedDescriptorPath )
    {
        EE_ASSERT( IsVisible() );

        outCreatedDescriptorPath.Clear();

        //-------------------------------------------------------------------------

        auto const& style = ImGui::GetStyle();
        ImVec2 const availableSpaceSize = ImGui::GetContentRegionAvail();
        float const requiredExtraOptionsHeight = HasExtraOptions() ? GetExtraOptionsRequiredHeight() : 0;
        constexpr static float const buttonRowHeight = 30;

        // Property Grid
        //-------------------------------------------------------------------------

        ImVec2 propertyGridWindowSize( availableSpaceSize.x, availableSpaceSize.y - requiredExtraOptionsHeight - buttonRowHeight - ( style.ItemSpacing.y * 2 ) );
        if ( ImGui::BeginChild( "grid", propertyGridWindowSize, 0, 0 ) )
        {
            m_propertyGrid.UpdateAndDraw();
        }
        ImGui::EndChild();

        // Extra Controls
        //-------------------------------------------------------------------------

        if ( HasExtraOptions() )
        {
            ImVec2 extraOptionsWindowSize( availableSpaceSize.x, requiredExtraOptionsHeight );
            if ( ImGui::BeginChild( "extraOptions", extraOptionsWindowSize, 0, 0 ) )
            {
                DrawExtraOptions();
            }
            ImGui::EndChild();
        }

        // Button Row
        //-------------------------------------------------------------------------

        bool wasDescriptorSuccessfullyCreated = false;

        ResourceDescriptor const* pDescriptor = GetDataFile();

        FileSystem::Path const defaultDescriptorFilePath = m_defaultDescriptorResourcePath.IsValid() ? m_defaultDescriptorResourcePath.GetFileSystemPath( m_pToolsContext->GetSourceDataDirectory() ) : FileSystem::Path();

        ImGui::BeginDisabled( !pDescriptor->IsValid() || !defaultDescriptorFilePath.IsValid() || defaultDescriptorFilePath.Exists() );
        if ( ImGuiX::IconButtonColored(  EE_ICON_FILE_IMPORT, "Import", Colors::Green, Colors::White, Colors::White, ImVec2( 100, 0 ) ) )
        {
            if ( TrySaveDescriptorToDisk( defaultDescriptorFilePath ) )
            {
                outCreatedDescriptorPath = DataPath::FromFileSystemPath( m_pToolsContext->GetSourceDataDirectory(), defaultDescriptorFilePath );
                wasDescriptorSuccessfullyCreated = true;
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled( !pDescriptor->IsValid() || !m_defaultDescriptorResourcePath.IsValid() );
        if ( ImGuiX::IconButtonColored( EE_ICON_FILE_IMPORT, "Import As", Colors::Green, Colors::White, Colors::White, ImVec2( 100, 0 ) ) )
        {
            FileSystem::Path userSpecifiedDescriptorFilePath;
            if ( TryGetUserSpecifiedDescriptorPath( userSpecifiedDescriptorFilePath ) )
            {
                if ( TrySaveDescriptorToDisk( userSpecifiedDescriptorFilePath ) )
                {
                    outCreatedDescriptorPath = DataPath::FromFileSystemPath( m_pToolsContext->GetSourceDataDirectory(), userSpecifiedDescriptorFilePath );
                    wasDescriptorSuccessfullyCreated = true;
                }
            }
        }
        ImGui::EndDisabled();

        //-------------------------------------------------------------------------

        return wasDescriptorSuccessfullyCreated;
    }

    void ImportSettings::UpdateDescriptor( DataPath sourceFileResourcePath, TVector<Import::ImportableItem*> const& selectedItems )
    {
        m_sourceResourcePath = sourceFileResourcePath;
        GenerateDefaultDescriptorPath();
        UpdateDescriptorInternal( sourceFileResourcePath, selectedItems );
    }

    void ImportSettings::GenerateDefaultDescriptorPath()
    {
        if ( m_sourceResourcePath.IsValid() && m_sourceResourcePath.IsFilePath() )
        {
            ResourceDescriptor const* pDesc = GetDataFile();
            ResourceTypeID const resourceTypeID = pDesc->GetCompiledResourceTypeID();
            EE_ASSERT( resourceTypeID.IsValid() );

            //-------------------------------------------------------------------------

            TInlineString<5> extension = resourceTypeID.ToString();
            extension.make_lower();

            m_defaultDescriptorResourcePath = m_sourceResourcePath;
            m_defaultDescriptorResourcePath.ReplaceExtension( extension );
        }
        else
        {
            m_defaultDescriptorResourcePath.Clear();
        }
    }

    bool ImportSettings::TryGetUserSpecifiedDescriptorPath( FileSystem::Path& outPath ) const
    {
        ResourceDescriptor const* pDesc = GetDataFile();
        ResourceTypeID const resourceTypeID = pDesc->GetCompiledResourceTypeID();
        EE_ASSERT( resourceTypeID.IsValid() );

        EE_ASSERT( m_defaultDescriptorResourcePath.IsValid() );
        FileSystem::Path const newDescriptorPath = m_defaultDescriptorResourcePath.GetFileSystemPath( m_pToolsContext->GetSourceDataDirectory() );

        FileDialog::Result result = FileDialog::Save( m_pToolsContext, resourceTypeID, newDescriptorPath );
        if ( result )
        {
            outPath = result;
            return true;
        }

        return false;
    }

    bool ImportSettings::TrySaveDescriptorToDisk( FileSystem::Path const filePath )
    {
        EE_ASSERT( filePath.IsValid() );

        ResourceDescriptor const* pDescriptor = GetDataFile();
        EE_ASSERT( pDescriptor != nullptr );

        ResourceID resourceID = DataPath::FromFileSystemPath( m_pToolsContext->GetSourceDataDirectory(), filePath );
        EE_ASSERT( resourceID.GetResourceTypeID() == pDescriptor->GetCompiledResourceTypeID() );

        //-------------------------------------------------------------------------

        bool result = true;

        // Run any require pre-save operations
        // This may further mutate the descriptor
        if ( !PreSaveDescriptor( filePath ) )
        {
            result = false;
        }

        // Save the descriptor
        if ( result )
        {
            if ( !IDataFile::TryWriteToFile( *m_pToolsContext->m_pTypeRegistry, filePath, pDescriptor ) )
            {
                MessageDialog::Error( "Failed to save descriptor!", "Failed to write descriptor to disk!" );
                result = false;
            }
        }

        // Always run post save to undo any changes we might have done in the pre-save
        PostSaveDescriptor( filePath );

        return result;
    }
}
