#include "ResourceDescriptorCreator.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "EngineTools/Core/Dialogs.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/TypeSystem/ResourceInfo.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceDescriptorCreator::ResourceDescriptorCreator( ToolsContext const* pToolsContext, TypeSystem::TypeID const descriptorTypeID, FileSystem::Path const& startingDir )
        : m_pToolsContext( pToolsContext )
        , m_propertyGrid( pToolsContext )
        , m_startingPath( startingDir )
    {
        EE_ASSERT( m_pToolsContext != nullptr );
        EE_ASSERT( m_pToolsContext->m_pTypeRegistry->IsTypeDerivedFrom( descriptorTypeID, ResourceDescriptor::GetStaticTypeID() ) );

        auto pTypeInfo = m_pToolsContext->m_pTypeRegistry->GetTypeInfo( descriptorTypeID );
        EE_ASSERT( pTypeInfo != nullptr );

        m_pDataFile = Cast<ResourceDescriptor>( pTypeInfo->CreateType() );
        EE_ASSERT( m_pDataFile != nullptr );

        m_propertyGrid.SetTypeToEdit( m_pDataFile );

        //-------------------------------------------------------------------------

        TInlineString<10> const filenameStr( TInlineString < 10>::CtorSprintf(), "NewResource.%s", m_pDataFile->GetCompiledResourceTypeID().ToString().c_str() );
        m_startingPath += filenameStr.c_str();
    }

    ResourceDescriptorCreator::~ResourceDescriptorCreator()
    {
        EE::Delete( m_pDataFile );
    }

    bool ResourceDescriptorCreator::Draw()
    {
        bool isOpen = true;

        // Just save empty descriptors immediately
        if ( m_pDataFile->GetTypeInfo()->m_properties.empty() )
        {
            SaveDescriptor();
            isOpen = false;
        }
        else // Draw property grid editor
        {
            if ( !ImGui::IsPopupOpen( s_title ) )
            {
                ImGui::OpenPopup( s_title );
            }

            //-------------------------------------------------------------------------

            ImGui::SetNextWindowSize( ImVec2( 600, 800 ), ImGuiCond_FirstUseEver );
            if ( ImGui::BeginPopupModal( s_title, &isOpen ) )
            {
                if ( ImGui::BeginChild( "#descEditor", ImGui::GetContentRegionAvail() - ImVec2( 0, 40 ) ) )
                {
                    m_propertyGrid.UpdateAndDraw();
                }
                ImGui::EndChild();

                //-------------------------------------------------------------------------

                ImGui::BeginDisabled( !m_pDataFile->IsValid() );
                if ( ImGuiX::ButtonColored( "Save", Colors::Green, Colors::White, ImVec2( 120, 0 ) ) )
                {
                    SaveDescriptor();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndDisabled();

                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();

                if ( ImGui::Button( "Cancel", ImVec2( 120, 0 ) ) )
                {
                    ImGui::CloseCurrentPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::EndPopup();
            }
        }

        //-------------------------------------------------------------------------

        return isOpen && ImGui::IsPopupOpen( s_title );
    }

    void ResourceDescriptorCreator::SaveDescriptor()
    {
        ResourceTypeID const resourceTypeID = m_pDataFile->GetCompiledResourceTypeID();
        TInlineString<5> const resourceTypeIDString = resourceTypeID.ToString();

        FileDialog::Result result = FileDialog::Save( m_pToolsContext, resourceTypeID, m_startingPath );
        if ( !result )
        {
            return;
        }

        EE_ASSERT( m_pDataFile != nullptr );
        if ( !ResourceDescriptor::TryWriteToFile( *m_pToolsContext->m_pTypeRegistry, result.m_filePaths[0], m_pDataFile ) )
        {
            InlineString const str( InlineString::CtorSprintf(), "Failed to write descriptor file (%s) to disk!", result.m_filePaths[0].c_str() );
            MessageDialog::Error( "Error Saving Descriptor!", str.c_str() );
        }
    }
}