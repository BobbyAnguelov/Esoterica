#include "EditorDialog_OpenResource.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Base/TypeSystem/ResourceInfo.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    OpenResourceDialog::OpenResourceDialog( ToolsContext const* pToolsContext )
        : m_pToolsContext( pToolsContext )
    {
        m_buffer.reserve( 512 );
        m_files.reserve( 100000 );
        DataFileRegistry::DirectoryInfo const* pDirectoryInfo = m_pToolsContext->m_pDataFileRegistry->FindDirectoryEntry( DataPath( "Data://" ) );
        pDirectoryInfo->GetAllResourceOrDataFiles( m_files, true );
    }

    bool OpenResourceDialog::Draw()
    {
        ImGui::SetNextItemWidth( -1 );

        if ( ImGui::IsWindowAppearing() )
        {
            ImGui::SetKeyboardFocusHere();
        }

        if ( ImGuiX::InputFilterText( m_filter ) )
        {
            UpdateFilter();
        }

        //-------------------------------------------------------------------------

        bool shouldCloseDialog = false;

        TVector<DataFileRegistry::FileInfo const*> const& fileList = m_filter.HasFilterSet() ? m_filteredFiles : m_files;

        if ( ImGui::BeginChild( "List", ImVec2( 0, -1 ) ) )
        {
            ImGuiListClipper clipper;
            clipper.Begin( (int32_t) fileList.size() );
            while ( clipper.Step() )
            {
                for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                {
                    DataFileRegistry::FileInfo const* pFileInfo = fileList[i];

                    Color color = ImGuiX::Style::s_colorText;
                    if ( pFileInfo->IsResourceDescriptorFile() )
                    {
                        TypeSystem::ResourceInfo const* pResourceInfo = m_pToolsContext->m_pTypeRegistry->GetResourceInfo( pFileInfo->GetResourceTypeID() );
                        color = pResourceInfo != nullptr ? pResourceInfo->m_color : ImGuiX::Style::s_colorText;
                    }

                    m_buffer = pFileInfo->m_dataPath.GetString().substr( DataPath::s_pathPrefixLength ).c_str();

                    ImGui::PushStyleColor( ImGuiCol_Text, color );
                    if ( ImGui::Selectable( m_buffer.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick ) )
                    {
                        if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                        {
                            if ( pFileInfo->IsResourceDescriptorFile() )
                            {
                                m_pToolsContext->TryOpenResource( pFileInfo->GetResourceID() );
                            }
                            else if ( pFileInfo->IsDataFile() )
                            {
                                m_pToolsContext->TryOpenDataFile( pFileInfo->m_dataPath );
                            }

                            shouldCloseDialog = true;
                        }
                    }
                    ImGui::PopStyleColor();
                }
            }
            clipper.End();
        }
        ImGui::EndChild();

        return !shouldCloseDialog;
    }

    void OpenResourceDialog::UpdateFilter()
    {
        m_filteredFiles.clear();

        for ( auto pFile : m_files )
        {
            if ( m_filter.MatchesFilter( pFile->m_dataPath.c_str() ) )
            {
                m_filteredFiles.emplace_back( pFile );
            }
        }
    }
}