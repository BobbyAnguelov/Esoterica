#include "RawFileInspector.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    RawFileInspector::RawFileInspector( ToolsContext const* pToolsContext, FileSystem::Path const& filePath )
        : m_pToolsContext( pToolsContext )
        , m_rawResourceDirectory( pToolsContext->m_pResourceDatabase->GetRawResourceDirectoryPath() )
        , m_filePath( filePath )
        , m_propertyGrid( pToolsContext )
    {
        EE_ASSERT( m_rawResourceDirectory.IsDirectoryPath() && FileSystem::Exists( m_rawResourceDirectory ) );
        EE_ASSERT( filePath.IsFilePath() && FileSystem::Exists( filePath ) );
    }

    RawFileInspector::~RawFileInspector()
    {
        EE::Delete( m_pDescriptor );
    }

    bool RawFileInspector::DrawDialog()
    {
        if ( !ImGui::IsPopupOpen( GetInspectorTitle() ) )
        {
            ImGui::OpenPopup( GetInspectorTitle() );
        }

        //-------------------------------------------------------------------------

        bool isOpen = true;
        ImGui::SetNextWindowSize( ImVec2( 600, 800 ), ImGuiCond_FirstUseEver );
        if ( ImGui::BeginPopupModal( GetInspectorTitle(), &isOpen ) )
        {
            DrawFileInfo();

            //-------------------------------------------------------------------------

            auto availableSpace = ImGui::GetContentRegionAvail();
            if ( ImGui::BeginTable( "DialogTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_NoSavedSettings, availableSpace ) )
            {
                ImGui::TableSetupColumn( "Info", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "Creator", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex( 0 );
                DrawFileContents();

                ImGui::TableSetColumnIndex( 1 );
                DrawResourceDescriptorCreator();

                ImGui::EndTable();
            }

            ImGui::EndPopup();
        }

        return isOpen && ImGui::IsPopupOpen( GetInspectorTitle() );
    }

    bool RawFileInspector::CreateNewDescriptor( ResourceTypeID resourceTypeID, Resource::ResourceDescriptor const* pDescriptor ) const
    {
        EE_ASSERT( resourceTypeID.IsValid() );

        //-------------------------------------------------------------------------

        TInlineString<5> extension = resourceTypeID.ToString();
        extension.make_lower();

        FileSystem::Path newDescriptorPath = m_filePath;
        newDescriptorPath.ReplaceExtension( extension );

        //-------------------------------------------------------------------------

        TInlineString<10> const filter( TInlineString<10>::CtorSprintf(), "*.%s", extension.c_str() );
        pfd::save_file saveDialog( "Save Resource Descriptor", newDescriptorPath.c_str(), { "Descriptor", filter.c_str() } );
        newDescriptorPath = saveDialog.result().c_str();

        if ( !newDescriptorPath.IsValid() || !newDescriptorPath.IsFilePath() )
        {
            return false;
        }

        // Ensure correct extension
        if ( !newDescriptorPath.MatchesExtension( extension.c_str() ) )
        {
            newDescriptorPath.Append( "." );
            newDescriptorPath.Append( extension.c_str() );
        }

        //-------------------------------------------------------------------------

        Serialization::TypeArchiveWriter typeWriter( *m_pToolsContext->m_pTypeRegistry );
        typeWriter << pDescriptor;
        return typeWriter.WriteToFile( newDescriptorPath );
    }

    //-------------------------------------------------------------------------

    EE_GLOBAL_REGISTRY( RawFileInspectorFactory );

    //-------------------------------------------------------------------------

    bool RawFileInspectorFactory::CanCreateInspector( FileSystem::Path const& filePath )
    {
        EE_ASSERT( filePath.IsValid() );

        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( pCurrentFactory->IsSupportedFile( filePath ) )
            {
                return true;
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        return false;
    }

    RawFileInspector* RawFileInspectorFactory::TryCreateInspector( ToolsContext const* pToolsContext, FileSystem::Path const& filePath )
    {
        EE_ASSERT( filePath.IsValid() );

        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( pCurrentFactory->IsSupportedFile( filePath ) )
            {
                return pCurrentFactory->CreateInspector( pToolsContext, filePath );
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        return nullptr;
    }
}