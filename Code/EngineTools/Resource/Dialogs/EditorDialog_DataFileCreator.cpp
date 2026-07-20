#include "EditorDialog_DataFileCreator.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "EngineTools/Core/SystemDialogs.h"
#include "Base/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceDataFileCreatorDialog::ResourceDataFileCreatorDialog( ToolsContext const* pToolsContext, TypeSystem::TypeID const descriptorTypeID, FileSystem::Path const& startingDir )
        : m_pToolsContext( pToolsContext )
        , m_propertyGrid( pToolsContext )
        , m_startingPath( startingDir )
    {
        EE_ASSERT( m_pToolsContext != nullptr );
        EE_ASSERT( m_pToolsContext->m_pTypeRegistry->IsTypeDerivedFrom( descriptorTypeID, IDataFile::GetStaticTypeID() ) );
        EE_ASSERT( startingDir.IsValid() && startingDir.IsDirectoryPath() );

        auto pTypeInfo = m_pToolsContext->m_pTypeRegistry->GetTypeInfo( descriptorTypeID );
        EE_ASSERT( pTypeInfo != nullptr );

        bool isResource = pTypeInfo->IsDerivedFrom( ResourceDescriptor::GetStaticTypeID() );
        if ( isResource )
        {
            auto pRD = pTypeInfo->GetDefaultInstance<ResourceDescriptor>();
            EE_ASSERT( pRD->IsUserCreateableDescriptor() );

            if ( !pRD->RequiresInitialSetup() )
            {
                m_showEditor = false;
            }
        }

        //-------------------------------------------------------------------------

        m_pDataFile = Cast<IDataFile>( pTypeInfo->CreateType() );
        EE_ASSERT( m_pDataFile != nullptr );
        m_propertyGrid.SetTypeToEdit( m_pDataFile );

        //-------------------------------------------------------------------------

        m_extension = m_pDataFile->GetExtension();
        TInlineString<10> const filenameStr( TInlineString < 10>::CtorSprintf(), "%s.%s", m_pDataFile->GetFriendlyName(), m_extension.c_str() );
        m_startingPath += filenameStr.c_str();
    }

    ResourceDataFileCreatorDialog::~ResourceDataFileCreatorDialog()
    {
        EE::Delete( m_pDataFile );
    }

    bool ResourceDataFileCreatorDialog::Draw()
    {
        bool isOpen = true;

        if ( m_showEditor )
        {
            ImVec2 const buttonSize( 120, 0 );

            if ( ImGui::BeginChild( "#descEditor", ImGui::GetContentRegionAvail() - ImVec2( 0, ImGui::GetFrameHeightWithSpacing() ) ) )
            {
                m_propertyGrid.UpdateAndDraw();
            }
            ImGui::EndChild();

            //-------------------------------------------------------------------------

            auto pResourceDescriptor = TryCast<ResourceDescriptor>( m_pDataFile );
            bool const disabled = pResourceDescriptor ? pResourceDescriptor->IsValid() : false;

            ImGui::BeginDisabled( !disabled );
            if ( ImGuiX::ButtonColored( "Save", Colors::Green, Colors::White, buttonSize ) )
            {
                SaveDescriptor();
                isOpen = false;
            }
            ImGui::EndDisabled();

            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();

            if ( ImGui::Button( "Cancel", buttonSize ) )
            {
                isOpen = false;
            }
        }
        else // Directly save
        {
            SaveDescriptor();
            isOpen = false;
        }

        //-------------------------------------------------------------------------

        return isOpen;
    }

    void ResourceDataFileCreatorDialog::SaveDescriptor()
    {
        FileDialog::Result result = FileDialog::SaveResourceOrDataFile( m_pToolsContext, m_extension, m_startingPath );
        if ( !result )
        {
            return;
        }

        EE_ASSERT( m_pDataFile != nullptr );
        Log log;
        if ( !IDataFile::TryWriteToFile( *m_pToolsContext->m_pTypeRegistry, log, result.m_filePaths[0], m_pDataFile ) )
        {
            InlineString const str( InlineString::CtorSprintf(), "Failed to write descriptor file (%s) to disk!", result.m_filePaths[0].c_str() );
            MessageDialog::Error( "Error Saving Descriptor!", str.c_str() );
            return;
        }

        m_pToolsContext->TryOpenDataFile( DataPath( result.m_filePaths[0], m_pToolsContext->GetSourceDataDirectory() ) );
    }
}