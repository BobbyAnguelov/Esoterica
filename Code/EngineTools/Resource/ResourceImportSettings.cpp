#include "ResourceImportSettings.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Core/CommonDialogs.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ImportSettings::ImportSettings( ToolsContext const* pToolsContext )
        : m_pToolsContext( pToolsContext )
        , m_propertyGrid( pToolsContext )
    {}

    bool ImportSettings::UpdateAndDraw( ResourcePath& outCreatedDescriptorPath )
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
            m_propertyGrid.DrawGrid();
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

        ResourceDescriptor const* pDescriptor = GetDescriptor();

        FileSystem::Path const defaultDescriptorFilePath = m_defaultDescriptorResourcePath.IsValid() ? m_defaultDescriptorResourcePath.ToFileSystemPath( m_pToolsContext->GetRawResourceDirectory() ) : FileSystem::Path();

        ImGui::BeginDisabled( !pDescriptor->IsValid() || !defaultDescriptorFilePath.IsValid() || defaultDescriptorFilePath.Exists() );
        if ( ImGuiX::ColoredIconButton( Colors::Green, Colors::White, Colors::White, EE_ICON_FILE_IMPORT, "Import", ImVec2( 100, 0 ) ) )
        {
            if ( TrySaveDescriptorToDisk( defaultDescriptorFilePath ) )
            {
                outCreatedDescriptorPath = ResourcePath::FromFileSystemPath( m_pToolsContext->GetRawResourceDirectory(), defaultDescriptorFilePath );
                wasDescriptorSuccessfullyCreated = true;
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled( !pDescriptor->IsValid() || !m_defaultDescriptorResourcePath.IsValid() );
        if ( ImGuiX::ColoredIconButton( Colors::Green, Colors::White, Colors::White, EE_ICON_FILE_IMPORT, "Import As", ImVec2( 100, 0 ) ) )
        {
            FileSystem::Path userSpecifiedDescriptorFilePath;
            if ( TryGetUserSpecifiedDescriptorPath( userSpecifiedDescriptorFilePath ) )
            {
                if ( TrySaveDescriptorToDisk( userSpecifiedDescriptorFilePath ) )
                {
                    outCreatedDescriptorPath = ResourcePath::FromFileSystemPath( m_pToolsContext->GetRawResourceDirectory(), userSpecifiedDescriptorFilePath );
                    wasDescriptorSuccessfullyCreated = true;
                }
            }
        }
        ImGui::EndDisabled();

        //-------------------------------------------------------------------------

        return wasDescriptorSuccessfullyCreated;
    }

    void ImportSettings::UpdateDescriptor( ResourcePath sourceFileResourcePath, TVector<Import::ImportableItem*> const& selectedItems )
    {
        EE_ASSERT( sourceFileResourcePath.IsValid() && sourceFileResourcePath.IsFile() );
        m_sourceResourcePath = sourceFileResourcePath;
        GenerateDefaultDescriptorPath();
        UpdateDescriptorInternal( sourceFileResourcePath, selectedItems );
    }

    void ImportSettings::GenerateDefaultDescriptorPath()
    {
        if ( m_sourceResourcePath.IsValid() )
        {
            ResourceDescriptor const* pDesc = GetDescriptor();
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
        ResourceDescriptor const* pDesc = GetDescriptor();
        ResourceTypeID const resourceTypeID = pDesc->GetCompiledResourceTypeID();
        EE_ASSERT( resourceTypeID.IsValid() );

        EE_ASSERT( m_defaultDescriptorResourcePath.IsValid() );
        FileSystem::Path const newDescriptorPath = m_defaultDescriptorResourcePath.ToFileSystemPath( m_pToolsContext->GetRawResourceDirectory() );

        TypeSystem::ResourceInfo const* pResourceInfo = m_pToolsContext->m_pTypeRegistry->GetResourceInfoForResourceType( resourceTypeID );
        return SaveDialog( resourceTypeID, outPath, newDescriptorPath, pResourceInfo->m_friendlyName );
    }

    bool ImportSettings::TrySaveDescriptorToDisk( FileSystem::Path const filePath )
    {
        EE_ASSERT( filePath.IsValid() );

        ResourceDescriptor const* pDescriptor = GetDescriptor();
        EE_ASSERT( pDescriptor != nullptr );

        ResourceID resourceID = ResourcePath::FromFileSystemPath( m_pToolsContext->GetRawResourceDirectory(), filePath );
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
            Serialization::TypeArchiveWriter typeWriter( *m_pToolsContext->m_pTypeRegistry );
            typeWriter << pDescriptor;
            if ( !typeWriter.WriteToFile( filePath ) )
            {
                pfd::message( "Failed to save descriptor!", "Failed to write descriptor to disk!", pfd::choice::ok, pfd::icon::error ).result();
                result = false;
            }

        }

        // Always run post save to undo any changes we might have done in the pre-save
        PostSaveDescriptor( filePath );

        return result;
    }
}
