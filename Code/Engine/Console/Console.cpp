#include "Console.h"
#include "Base/Settings/SettingsRegistry.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    void Console::Initialize( Settings::SettingsRegistry& settingsRegistry )
    {
        EE_ASSERT( m_pSettingsRegistry == nullptr );
        m_pSettingsRegistry = &settingsRegistry;
    }

    void Console::Shutdown()
    {
        m_pSettingsRegistry = nullptr;
    }

    void Console::Update( UpdateContext const& context )
    {
        if ( !m_isVisible )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( !ImGui::Begin( "Console", &m_isVisible ) )
        {
            ImGui::End();
            return;
        }

        //-------------------------------------------------------------------------

        if ( ImGui::BeginTabBar( "SettingsTabs" ) )
        {
            if ( ImGui::BeginTabItem( "Global Settings" ) )
            {
                DrawGlobalSettingsEditor();
                ImGui::EndTabItem();
            }

            if ( ImGui::BeginTabItem( "World Settings" ) )
            {
                DrawWorldSettingsEditor();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        //-------------------------------------------------------------------------

        ImGui::End();
    }

    void Console::DrawGlobalSettingsEditor()
    {
        if( m_pSettingsRegistry == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( ImGui::BeginTable( "Layout", 2, ImGuiTableFlags_Resizable, ImGui::GetContentRegionAvail() ) )
        {
            ImGui::TableSetupColumn( "Objects", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Settings", ImGuiTableColumnFlags_WidthStretch );

            ImGui::TableNextRow();

            // Global Setting Objects
            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();

            ImGuiX::FlatButton( "Resource", ImVec2( -1, 0 ) );

            ImGuiX::FlatButton( "Render", ImVec2( -1, 0 ) );

            constexpr int32_t const buttonSize = 30;
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y - buttonSize );
            if ( ImGuiX::IconButtonColored( EE_ICON_FLOPPY, "Save To File", Colors::Green, Colors::White, Colors::White, ImVec2( ImGui::GetContentRegionAvail().x - 24, buttonSize ), true ) )
            {
                if ( m_pSettingsRegistry->SaveGlobalSettingsToIniFile() )
                {
                    ImGuiX::NotifySuccess( "Ini file saved!" );
                }
                else
                {
                    ImGuiX::NotifyError( "Failed to save ini file! Please check log for details!" );
                }
            }
            ImGui::SameLine();
            ImGuiX::HelpMarker( "Note: Some settings will only take affect on the next launch of the engine!" );

            // Settings
            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();

            m_globalSettingsFilterWidget.UpdateAndDraw();

            if ( ImGui::BeginTable( "GlobalSettings", 2 ) )
            {
                ImGui::TableSetupColumn( "Setting", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "Value", ImGuiTableColumnFlags_WidthStretch );

                ImGui::TableHeadersRow();

                ImGui::EndTable();
            }

            ImGui::EndTable();
        }
    }

    void Console::DrawWorldSettingsEditor()
    {
        m_globalSettingsFilterWidget.UpdateAndDraw();

        if ( ImGui::BeginTable( "WorldSettings", 3, ImGuiTableFlags_Resizable, ImGui::GetContentRegionAvail() ) )
        {
            ImGui::TableSetupColumn( "World", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Setting", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Value", ImGuiTableColumnFlags_WidthStretch );

            ImGui::TableHeadersRow();

            ImGui::EndTable();
        }
    }
}
#endif