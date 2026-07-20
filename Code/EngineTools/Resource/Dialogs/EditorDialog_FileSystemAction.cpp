#include "EditorDialog_FileSystemAction.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Base/TypeSystem/ResourceInfo.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    FileSystemActionDialog::FileSystemActionDialog( Action action, ToolsContext const* pToolsContext, DataPath const& path )
        : m_action( action )
        , m_pToolsContext( pToolsContext )
        , m_path( path )
        , m_newPath( path )
    {}

    FileSystemActionDialog::FileSystemActionDialog( ToolsContext const* pToolsContext, DataPath const& oldPath, DataPath const& newPath )
        : m_action( Action::Rename )
        , m_pToolsContext( pToolsContext )
        , m_path( oldPath )
        , m_newPath( newPath )
        , m_allowSelectionOfNewName( false )
    {}

    ImVec2 FileSystemActionDialog::GetSize() const
    {
        switch ( m_action )
        {
            case Action::Rename:
            {
                return ImVec2( 800, 220 );
            }
            break;

            case Action::Delete:
            {
                return ImVec2( 600, 180 );
            }
            break;

            default:
            {
                return ImVec2( 800, 400 );
            }
            break;
        }
    }

    char const* FileSystemActionDialog::GetTitle() const
    {
        switch ( m_action )
        {
            case Action::Rename:
            {
                return "Rename";
            }
            break;

            case Action::Delete:
            {
                return "Delete";
            }
            break;

            default:
            {
                return "INVALID DIALOG!";
            }
            break;
        }
    }

    bool FileSystemActionDialog::Draw()
    {
        bool shouldCloseDialog = false;

        switch ( m_action )
        {
            case Action::Rename:
            {
                if ( m_stage == Stage::Confirmation )
                {
                    shouldCloseDialog = DrawRenameConfirmation();
                }
                else
                {
                    shouldCloseDialog = DrawRenameExecution();
                }
            }
            break;

            case Action::Delete:
            {
                if ( m_stage == Stage::Confirmation )
                {
                    shouldCloseDialog = DrawDeleteConfirmation();
                }
                else
                {
                    shouldCloseDialog = DrawDeleteExecution();
                }
            }
            break;

            default:
            EE_UNREACHABLE_CODE();
            break;
        }

        return !shouldCloseDialog;
    }

    //-------------------------------------------------------------------------

    void FileSystemActionDialog::StartDelete()
    {
        
        m_stage = Stage::Execution;
    }

    bool FileSystemActionDialog::DrawDeleteConfirmation()
    {
        bool shouldClose = false;

        ImGuiX::PushFont( ImGuiX::FontType::Regular, 60 );
        ImGui::TextColored( Colors::Red, EE_ICON_DELETE_ALERT );
        ImGuiX::PopFont();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Text( "Deleting File: %s", m_path.c_str() );
            ImGui::Text( "Are you sure you want to delete this file?" );
            ImGui::Text( "This cannot be undone!" );
        }
        ImGui::EndGroup();

        ImGui::Dummy( ImVec2( 0, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() ) );

        constexpr static float const buttonWidth = 143;
        ImGui::Dummy( ImVec2( ImGui::GetContentRegionAvail().x - ( buttonWidth * 2 ) - ( ImGui::GetStyle().ItemSpacing.x * 2 ), 0 ) );
        ImGui::SameLine();

        if ( ImGuiX::IconButton( EE_ICON_DELETE, "Ok", Colors::Lime, ImVec2( buttonWidth, 0 ), true ) )
        {
            StartDelete();
        }

        ImGui::SameLine( 0, 6 );

        if ( ImGuiX::IconButton( EE_ICON_CLOSE, "Cancel", Colors::Red, ImVec2( buttonWidth, 0 ), true ) )
        {
            shouldClose = true;
        }

        return shouldClose;
    }

    bool FileSystemActionDialog::DrawDeleteExecution()
    {
        ImGui::Text( "Fixing references!" );
        ImGui::ProgressBar( m_actionProgress );

        return false;
    }

    //-------------------------------------------------------------------------

    void FileSystemActionDialog::StartRename()
    {
        m_stage = Stage::Execution;
    }

    bool FileSystemActionDialog::DrawRenameConfirmation()
    {
        bool shouldClose = false;

        ImGuiX::PushFont( ImGuiX::FontType::Regular, 80 );
        ImGui::TextColored( Colors::Red, EE_ICON_RENAME );
        ImGuiX::PopFont();

        ImGui::SameLine();

        if ( m_allowSelectionOfNewName )
        {
            ImGui::BeginGroup();
            {
                if ( m_path.IsFilePath() )
                {
                    FileSystem::Extension const ext = m_path.GetExtension();

                    ImGui::Text( "Renaming File: %s", m_path.c_str() );

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( "New Path: %s", m_path.GetParentDirectory().c_str() );
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize( ext.c_str() ).x );
                    if ( ImGui::InputText( "##newname", m_newName.Data(), m_newName.Size(), ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CallbackCharFilter, ImGuiX::FilterFilenameChars ) )
                    {
                        if ( m_newName.Empty() )
                        {
                            m_newPath.Clear();
                        }
                        else
                        {
                            String const newFileName = m_newName.GetString() + "." + ext.c_str();
                            m_newPath = m_path;
                            m_newPath.ReplaceFilename( newFileName );
                        }
                    }
                    ImGui::SameLine();
                    ImGui::Text( ".%s", ext.c_str() );

                    ImGui::Text( "Are you sure you want to rename this file?" );
                    ImGui::TextColored( Colors::Red, "This cannot be undone!" );
                }
                else
                {
                    ImGui::Text( "Renaming Directory: %s", m_path.c_str() );

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( "New Path: %s", m_path.GetParentDirectory().c_str() );
                    ImGui::SameLine();
                    if ( ImGui::InputText( "##newname", m_newName.Data(), m_newName.Size(), ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CallbackCharFilter, ImGuiX::FilterFilenameChars ) )
                    {
                        if ( m_newName.Empty() )
                        {
                            m_newPath.Clear();
                        }
                        else
                        {
                            String const newDirName = m_newName.GetString() + DataPath::s_pathDelimiter;
                            m_newPath = m_path;
                            m_newPath.ReplaceFilename( newDirName );
                        }
                    }

                    ImGui::Text( "Are you sure you want to rename this directory?" );
                    ImGui::TextColored( Colors::Red, "This cannot be undone!" );
                }
            }
            ImGui::EndGroup();
        }
        else
        {
            ImGui::BeginGroup();
            {
                ImGui::Text( "Renaming: '%s'", m_path.c_str() );
                ImGui::Text( "New Path: '%s'", m_newPath.c_str() );
                ImGui::Text( "Are you sure you want to rename this file/directory?" );
                ImGui::TextColored( Colors::Red, "This cannot be undone!" );
            }
            ImGui::EndGroup();
        }

        //-------------------------------------------------------------------------

        ImGui::Dummy( ImVec2( 0, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() ) );

        constexpr static float const buttonWidth = 143;
        ImGui::Dummy( ImVec2( ImGui::GetContentRegionAvail().x - ( buttonWidth * 2 ) - ( ImGui::GetStyle().ItemSpacing.x * 2 ), 0 ) );
        ImGui::SameLine();

        bool const allowRename = m_newPath.IsValid() && ( m_path != m_newPath ) && !m_newPath.GetFileSystemPath( m_pToolsContext->GetSourceDataDirectory() ).Exists();

        ImGui::BeginDisabled( !allowRename );
        if ( ImGuiX::IconButton( EE_ICON_RENAME, "Ok", Colors::Lime, ImVec2( buttonWidth, 0 ), true ) )
        {
            StartRename();
        }
        ImGui::EndDisabled();

        ImGui::SameLine( 0, 6 );

        if ( ImGuiX::IconButton( EE_ICON_CLOSE, "Cancel", Colors::Red, ImVec2( buttonWidth, 0 ), true ) )
        {
            shouldClose = true;
        }

        return shouldClose;
    }

    bool FileSystemActionDialog::DrawRenameExecution()
    {
        ImGui::Text( "Fixing references!" );
        ImGui::ProgressBar( m_actionProgress );

        return false;
    }
}