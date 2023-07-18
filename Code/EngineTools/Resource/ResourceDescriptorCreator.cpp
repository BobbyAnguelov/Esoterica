#include "ResourceDescriptorCreator.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "EngineTools/Core/CommonDialogs.h"
#include "Base/TypeSystem/TypeRegistry.h"
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

        m_pDescriptor = Cast<ResourceDescriptor>( pTypeInfo->CreateType() );
        EE_ASSERT( m_pDescriptor != nullptr );

        m_propertyGrid.SetTypeToEdit( m_pDescriptor );

        //-------------------------------------------------------------------------

        TInlineString<10> const filenameStr( TInlineString < 10>::CtorSprintf(), "NewResource.%s", m_pDescriptor->GetCompiledResourceTypeID().ToString().c_str() );
        m_startingPath += filenameStr.c_str();
    }

    ResourceDescriptorCreator::~ResourceDescriptorCreator()
    {
        EE::Delete( m_pDescriptor );
    }

    bool ResourceDescriptorCreator::Draw()
    {
        if ( !ImGui::IsPopupOpen( s_title ) )
        {
            ImGui::OpenPopup( s_title );
        }

        //-------------------------------------------------------------------------

        bool isOpen = true;
        ImGui::SetNextWindowSize( ImVec2( 600, 800 ), ImGuiCond_FirstUseEver );
        if ( ImGui::BeginPopupModal( s_title, &isOpen ) )
        {
            if ( ImGui::BeginChild( "#descEditor", ImGui::GetContentRegionAvail() - ImVec2( 0, 40 ) ) )
            {
                m_propertyGrid.DrawGrid();
            }
            ImGui::EndChild();

            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( !m_pDescriptor->IsValid() );
            if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, "Save", ImVec2( 120, 0 ) ) )
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

        return isOpen && ImGui::IsPopupOpen( s_title );
    }

    void ResourceDescriptorCreator::SaveDescriptor()
    {
        ResourceTypeID const resourceTypeID = m_pDescriptor->GetCompiledResourceTypeID();
        TInlineString<5> const resourceTypeIDString = resourceTypeID.ToString();
        TypeSystem::ResourceInfo const* pResourceInfo = m_pToolsContext->m_pTypeRegistry->GetResourceInfoForResourceType( resourceTypeID );

        FileSystem::Path outPath = SaveDialog( resourceTypeID, m_startingPath, pResourceInfo->m_friendlyName );
        if( !outPath.IsValid() )
        {
            return;
        }

        // Ensure that the extension matches the expected type
        auto const extension = outPath.GetExtensionAsString();
        if ( extension != resourceTypeIDString.c_str() )
        {
            outPath.ReplaceExtension( resourceTypeIDString.c_str() );
        }

        EE_ASSERT( m_pDescriptor != nullptr );
        if ( !ResourceDescriptor::TryWriteToFile( *m_pToolsContext->m_pTypeRegistry, outPath, m_pDescriptor ) )
        {
            InlineString const str( InlineString::CtorSprintf(), "Failed to write descriptor file (%s) to disk!", outPath.c_str() );
            pfd::message( "Error Saving Descriptor!", str.c_str(), pfd::choice::ok, pfd::icon::error ).result();
        }
    }
}