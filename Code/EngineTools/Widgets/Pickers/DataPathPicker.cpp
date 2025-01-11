#include "DataPathPicker.h"
#include "EngineTools/Core/CommonToolTypes.h"
#include "EngineTools/Core/Dialogs.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include "Base/Resource/ResourceID.h"

//-------------------------------------------------------------------------

namespace EE
{
    static ImVec2 const g_buttonSize( 30, 0 );

    //-------------------------------------------------------------------------

    bool DataPathPicker::UpdateAndDraw()
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
        if ( ImGui::BeginChild( "RP", ImVec2( -1, ImGui::GetFrameHeight() ), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
        {
            float const itemSpacingX = ImGui::GetStyle().ItemSpacing.x;

            // Type and path
            //-------------------------------------------------------------------------

            {
                // Calculate size of resource path field
                float const contentRegionAvailableX = ImGui::GetContentRegionAvail().x;
                float usedWidth = ( itemSpacingX * 3 ) + ( g_buttonSize.x * 3 ) + 1;

                // Resource path
                ImGui::SetNextItemWidth( contentRegionAvailableX - usedWidth );
                String const& dataPathStr = m_dataPath.GetString();
                ImGui::PushStyleColor( ImGuiCol_Text, isValidPath ? ImGuiX::Style::s_colorText : Colors::Red );
                ImGui::InputText( "##DataPath", const_cast<char*>( dataPathStr.c_str() ), dataPathStr.length(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly );
                ImGui::PopStyleColor();

                // Drag and drop
                if ( ImGui::BeginDragDropTarget() )
                {
                    valueUpdated = TrySetPathFromDragAndDrop();
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
                            EE_LOG_WARNING( "Resource", "DataFilePathPicker", "Pasting invalid length string" );
                            clipboardText.clear();
                        }

                        if ( DataPath::IsValidPath( clipboardText ) )
                        {
                            m_dataPath = DataPath( clipboardText );
                            valueUpdated = true;
                        }
                        else
                        {
                            FileSystem::Path pastedFilePath( clipboardText );
                            if ( pastedFilePath.IsValid() && pastedFilePath.IsUnderDirectory( m_sourceDataDirectoryPath ) )
                            {
                                m_dataPath = DataPath::FromFileSystemPath( m_sourceDataDirectoryPath.c_str(), pastedFilePath );
                                valueUpdated = true;
                            }
                        }
                    }
                }

                // Tooltip
                if ( m_dataPath.IsValid() )
                {
                    ImGuiX::ItemTooltip( m_dataPath.c_str() );
                }

                // Pick Path
                ImGui::SameLine( 0, itemSpacingX );
                if ( ImGui::Button( EE_ICON_FILE_SEARCH_OUTLINE "##Pick", g_buttonSize ) )
                {
                    FileDialog::Result const result = FileDialog::Load( {}, "Choose Data File", true, m_sourceDataDirectoryPath.c_str() );
                    if ( result )
                    {
                        FileSystem::Path const selectedPath( result.m_filePaths[0].c_str() );

                        if ( selectedPath.IsUnderDirectory( m_sourceDataDirectoryPath ) )
                        {
                            m_dataPath = DataPath::FromFileSystemPath( m_sourceDataDirectoryPath.c_str(), selectedPath );
                            valueUpdated = true;
                        }
                        else
                        {
                            MessageDialog::Error( "Error", "Selected file is not with the raw resource folder!" );
                        }
                    }
                }
                ImGuiX::ItemTooltip( "Pick File" );
            }

            // Options
            //-------------------------------------------------------------------------

            {
                ImGui::SameLine();
                ImGui::BeginDisabled( !m_dataPath.IsValid() );
                if ( ImGui::Button( EE_ICON_COG "##Options", g_buttonSize ) )
                {
                    ImGui::OpenPopup( "##DataFilePathPickerOptions" );
                }
                ImGuiX::ItemTooltip( "Options" );

                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_ERASER "##Clear" ) )
                {
                    m_dataPath.Clear();
                    valueUpdated = true;
                }
                ImGuiX::ItemTooltip( "Clear" );
                ImGui::EndDisabled();
            }

            // Options Context Menu
            //-------------------------------------------------------------------------

            if ( ImGui::BeginPopup( "##DataFilePathPickerOptions" ) )
            {
                if ( ImGui::MenuItem( EE_ICON_FILE_OUTLINE" Copy Data Path" ) )
                {
                    ImGui::SetClipboardText( m_dataPath.c_str() );
                }

                if ( ImGui::MenuItem( EE_ICON_FILE" Copy File Path" ) )
                {
                    ImGui::SetClipboardText( filePath.c_str() );
                }

                if ( ImGui::MenuItem( EE_ICON_FOLDER_OPEN " Open In Explorer" ) )
                {
                    Platform::Win32::OpenInExplorer( filePath );
                }

                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();
        ImGui::PopID();

        //-------------------------------------------------------------------------

        return valueUpdated;
    }

    bool DataPathPicker::TrySetPathFromDragAndDrop()
    {
        if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( DragAndDrop::s_filePayloadID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
        {
            if ( payload->IsDelivery() )
            {
                ResourceID const droppedResourceID( (char*) payload->Data );
                if ( droppedResourceID.IsValid() )
                {
                    m_dataPath = droppedResourceID.GetDataPath();
                    return true;
                }
            }
        }

        return false;
    }
}