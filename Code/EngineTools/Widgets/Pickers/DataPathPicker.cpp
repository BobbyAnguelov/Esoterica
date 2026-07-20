#include "DataPathPicker.h"
#include "EngineTools/Core/CommonToolTypes.h"
#include "EngineTools/Core/SystemDialogs.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include "Base/Resource/ResourceID.h"

//-------------------------------------------------------------------------

namespace EE
{
    DataPathPicker::DataPathPicker( FileSystem::Path const& sourceDataDirectoryPath, DataPath const& path )
        : m_sourceDataDirectoryPath( sourceDataDirectoryPath )
    {
        EE_ASSERT( m_sourceDataDirectoryPath.IsValid() && m_sourceDataDirectoryPath.Exists() );
        SetPath( path );
    }

    bool DataPathPicker::UpdateAndDraw( float width )
    {
        bool valueUpdated = false;

        FileSystem::Path filePath;
        bool isValidPath = false;
        if ( m_dataPath.IsValid() )
        {
            filePath = m_dataPath.GetFileSystemPath( m_sourceDataDirectoryPath );
            isValidPath = filePath.Exists();
        }

        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        if ( ImGui::BeginChild( "RP", ImVec2( width, ImGui::GetFrameHeight() ), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
        {
            static ImVec2 const pickButtonSize( 70, 0 );

            // Type and path
            //-------------------------------------------------------------------------

            {
                // Calculate size of resource path field
                float const contentRegionAvailableX = ImGui::GetContentRegionAvail().x;
                float usedWidth = pickButtonSize.x + ImGui::GetStyle().ItemSpacing.x;

                // Resource path
                ImGui::SetNextItemWidth( contentRegionAvailableX - usedWidth );
                ImGui::PushStyleColor( ImGuiCol_Text, isValidPath ? ImGuiX::Style::s_colorText : Colors::Red );
                ImGui::InputText( "##DataPath", m_dataPathBuffer.Data(), m_dataPathBuffer.Size(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );
                ImGui::PopStyleColor();

                // Drag and drop
                if ( ImGui::BeginDragDropTarget() )
                {
                    if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( DragAndDrop::s_filePayloadID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
                    {
                        if ( payload->IsDelivery() )
                        {
                            ResourceID const droppedResourceID( (char*) payload->Data );
                            if ( droppedResourceID.IsValid() )
                            {
                                valueUpdated = TryUpdateDataPath( droppedResourceID.GetDataPath() );
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // Allow pasting valid paths
                if ( ImGui::IsItemFocused() )
                {
                    if ( ImGui::IsKeyDown( ImGuiMod_Ctrl ) && ImGui::IsKeyPressed( ImGuiKey_V ) )
                    {
                        String clipboardText = ImGui::GetClipboardText();
                        if ( clipboardText.length() > 256 )
                        {
                            EE_LOG_WARNING( LogCategory::Resource, "DataFilePathPicker", "Pasting invalid length string" );
                            clipboardText.clear();
                        }

                        if ( DataPath::IsValidPath( clipboardText ) )
                        {
                            DataPath const pastedDataPath( clipboardText );
                            valueUpdated = TryUpdateDataPath( pastedDataPath );
                        }
                        else
                        {
                            FileSystem::Path pastedFilePath( clipboardText );
                            if ( pastedFilePath.IsValid() && pastedFilePath.IsUnderDirectory( m_sourceDataDirectoryPath ) )
                            {
                                DataPath const pastedDataPath( pastedFilePath, m_sourceDataDirectoryPath.c_str() );
                                valueUpdated = TryUpdateDataPath( pastedDataPath );
                            }
                        }
                    }
                }

                // Tooltip
                if ( m_dataPath.IsValid() )
                {
                    ImGuiX::ItemTooltip( m_dataPath.c_str() );
                }

                // Options
                auto OptionsMenu = [this, &valueUpdated]
                {
                    if ( m_mode != Mode::PickFiles )
                    {
                        if ( ImGui::MenuItem( EE_ICON_FOLDER_STAR_OUTLINE" Select Folder" ) )
                        {
                            PickDirectory( valueUpdated );
                        }
                    }

                    if ( ImGui::MenuItem( EE_ICON_ERASER" Clear" ) )
                    {
                        if ( m_dataPath.IsValid() )
                        {
                            SetPath( DataPath() );
                            valueUpdated = true;
                        }
                    }

                    ImGui::Separator();

                    ImGui::BeginDisabled( !m_dataPath.IsValid() );
                    {
                        if ( ImGui::MenuItem( EE_ICON_FILE_OUTLINE" Copy Data Path" ) )
                        {
                            ImGui::SetClipboardText( m_dataPath.c_str() );
                        }

                        FileSystem::Path filePath = m_dataPath.IsValid() ? m_dataPath.GetFileSystemPath( m_sourceDataDirectoryPath ) : FileSystem::Path();

                        if ( ImGui::MenuItem( EE_ICON_FILE" Copy File Path" ) )
                        {
                            ImGui::SetClipboardText( filePath.c_str() );
                        }

                        if ( ImGui::MenuItem( EE_ICON_FOLDER_OPEN " Open In Explorer" ) )
                        {
                            Platform::Win32::OpenInExplorer( filePath );
                        }
                    }
                    ImGui::EndDisabled();
                };

                ImGui::SameLine();
                if ( ImGuiX::ComboIconButton( EE_ICON_FILE_IMPORT, "##Pick", OptionsMenu, ImGuiX::Style::s_colorText, pickButtonSize ) )
                {
                    if ( m_mode == Mode::PickDirectories )
                    {
                        PickDirectory( valueUpdated );
                    }
                    else
                    {
                        PickFile( valueUpdated );
                    }
                }
                ImGuiX::ItemTooltip( "Select Path" );
            }
        }
        ImGui::EndChild();
        ImGui::PopID();

        //-------------------------------------------------------------------------

        return valueUpdated;
    }

    void DataPathPicker::PickDirectory( bool& outPathUpdated )
    {
        EE_ASSERT( m_mode != Mode::PickFiles );

        FileSystem::Path const startingPath = m_dataPath.IsValid() ? m_dataPath.GetFileSystemPath( m_sourceDataDirectoryPath.GetDirectoryPath() ) : m_sourceDataDirectoryPath;

        FileDialog::Result const result = FileDialog::SelectFolder( startingPath );
        if ( result )
        {
            FileSystem::Path const selectedPath( result.m_filePaths[0].c_str() );

            if ( selectedPath.IsUnderDirectory( m_sourceDataDirectoryPath ) )
            {
                DataPath const selectedDataPath( selectedPath, m_sourceDataDirectoryPath.c_str() );
                outPathUpdated = TryUpdateDataPath( selectedDataPath );
            }
            else
            {
                MessageDialog::Error( "Error", "Selected file is not with the source data folder!" );
            }
        }
    }

    void DataPathPicker::PickFile( bool& outPathUpdated )
    {
        EE_ASSERT( m_mode != Mode::PickDirectories );

        FileSystem::Path const startingPath = m_dataPath.IsValid() ? m_dataPath.GetFileSystemPath( m_sourceDataDirectoryPath.GetDirectoryPath() ) : m_sourceDataDirectoryPath;

        FileDialog::Result const result = FileDialog::Load( {}, "Choose Data File", true, startingPath );
        if ( result )
        {
            FileSystem::Path const selectedPath( result.m_filePaths[0].c_str() );

            if ( selectedPath.IsUnderDirectory( m_sourceDataDirectoryPath ) )
            {
                outPathUpdated = TryUpdateDataPath( DataPath( selectedPath, m_sourceDataDirectoryPath.c_str() ) );
            }
            else
            {
                MessageDialog::Error( "Error", "Selected file is not with the source data folder!" );
            }
        }
    }

    void DataPathPicker::SetPath( DataPath const& path )
    {
        m_dataPath = path;
        if ( m_dataPath.IsValid() )
        {
            m_dataPathBuffer.Fill( path.c_str() );
        }
        else
        {
            m_dataPathBuffer.Clear();
        }
    }

    bool DataPathPicker::TryUpdateDataPath( DataPath const& newPath )
    {
        if ( newPath != m_dataPath )
        {
            if ( m_mode == Mode::PickFiles && newPath.IsDirectoryPath() )
            {
                return false;
            }
            else if ( m_mode == Mode::PickDirectories && newPath.IsFilePath() )
            {
                return false;
            }

            SetPath( newPath );
            return true;
        }

        return false;
    }
}