#include "BulkImporter.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    bool BulkImporter::FileImportContext::LogError( char const* pFormat, ... ) const
    {
        va_list args;
        va_start( args, pFormat );
        m_log.LogErrorWithUserDataVarArgs( uintptr_t( &m_fileToImport ), pFormat, args );
        va_end( args );

        return false;
    }

    void BulkImporter::FileImportContext::LogWarning( char const* pFormat, ... ) const
    {
        va_list args;
        va_start( args, pFormat );
        m_log.LogWarningWithUserDataVarArgs( uintptr_t( &m_fileToImport ), pFormat, args );
        va_end( args );
    }

    void BulkImporter::FileImportContext::LogMessage( char const* pFormat, ... ) const
    {
        va_list args;
        va_start( args, pFormat );
        m_log.LogInfoWithUserDataVarArgs( uintptr_t( &m_fileToImport ), pFormat, args );
        va_end( args );
    }

    void BulkImporter::FileImportContext::AppendLog( Log const& log ) const
    {
        for ( auto const& entry : log.GetLogEntries() )
        {
            m_log.AddLogEntry( entry.m_severity, uintptr_t( &m_fileToImport ), "%s", entry.m_message.c_str() );
        }
    }

    //-------------------------------------------------------------------------

    BulkImporter::BulkImporter( ToolsContext const* pToolsContext )
        : m_pToolsContext( pToolsContext )
        , m_pPropertyGrid( EE::New<PropertyGrid>( pToolsContext ) )
    {
        EE_ASSERT( m_pToolsContext != nullptr );
        m_pPropertyGrid->SetControlBarVisible( false );
    }

    BulkImporter::~BulkImporter()
    {
        EE::Delete( m_pPropertyGrid );
    }

    void BulkImporter::SetRawFiles( TVector<RawFileInfo const*> const& rawFiles )
    {
        m_rawFiles.clear();

        TInlineVector<FileSystem::Extension, 8> supportedExtensions = GetSupportedFileExtensions();

        for ( auto pRF : rawFiles )
        {
            if ( eastl::find( supportedExtensions.begin(), supportedExtensions.end(), pRF->m_extension ) != supportedExtensions.end() )
            {
                m_rawFiles.emplace_back( pRF );
            }
        }
    }

    void BulkImporter::Draw()
    {
        ImGui::PushID( this );

        ImGui::SetNextItemOpen( !m_isCollapsed );
        m_isCollapsed = !ImGui::CollapsingHeader( "##Header", ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_OpenOnArrow );

        // Do not allow opening if the settings are disabled
        if ( !m_isEnabled && !m_isCollapsed )
        {
            m_isCollapsed = true;
        }

        //-------------------------------------------------------------------------

        ImGui::SameLine( 0, 0 ); // Adjust position to the right side of the header
        ImGui::SetNextItemAllowOverlap();
        if ( ImGuiX::Checkbox( "##enable", &m_isEnabled ) )
        {
            m_isCollapsed = !m_isEnabled;
        }

        ImGui::SameLine();
        ImGui::Text( GetTitle() );

        if ( !m_isCollapsed )
        {
            InlineString const infoMsg = GetInfoMessage();
            if ( !infoMsg.empty() )
            {
                ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 4.0f );
                ImGui::PushStyleColor( ImGuiCol_ChildBg, Colors::SlateGray );
                if ( ImGui::BeginChild( "info", ImVec2( -1, 0 ), ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY ) )
                {
                    ImGui::Text( EE_ICON_INFORMATION );
                    ImGui::SameLine();
                    ImGui::Text( infoMsg.c_str() );
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();
            }

            m_pPropertyGrid->UpdateAndDraw( ImVec2( -1, 0) );
        }

        ImGui::PopID();
    }

    void BulkImporter::Initialize()
    {
        m_pPropertyGrid->SetTypeToEdit( this );
    }

    void BulkImporter::Shutdown()
    {
        // Do Nothing
    }

    void BulkImporter::Import( Log& importLog, std::atomic<float>& progress, std::atomic<bool> const& cancelSignal )
    {
        EE_ASSERT( HasFilesToImport() );

        progress = 0.0f;

        ResourceTypeID const resourceTypeID = GetImportedType();
        FileSystem::Extension const resourceTypeExtension( resourceTypeID.ToString() );

        int32_t numFilesToImport = (int32_t) m_rawFiles.size();
        for ( int32_t i = 0; i < numFilesToImport; i++ )
        {
            if ( cancelSignal )
            {
                return;
            }

            // Update progress
            progress = ( float( i + 1 ) / numFilesToImport );

            // Create and validate target path
            FileSystem::Path targetPath = m_rawFiles[i]->m_filePath;
            targetPath.ReplaceExtension( resourceTypeExtension );

            if ( targetPath.Exists() )
            {
                importLog.LogWarningWithUserData( m_rawFiles[i], "Target filename '%s' already exists, skipping import", targetPath.c_str() );
                continue;
            }

            // Import file
            FileImportContext ctx = { *m_pToolsContext->m_pTypeRegistry, *m_rawFiles[i], importLog, m_pToolsContext->GetSourceDataDirectory(), targetPath };
            ImportFile( ctx );
        }
    }
}