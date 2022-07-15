#include "DebugView_RuntimeSettings.h"
#include "System/Imgui/ImguiX.h"
#include "System/Profiling.h"
#include "Engine/UpdateContext.h"
#include "Engine/RuntimeSettings/RuntimeSettings.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    static void DrawRuntimeSetting( RuntimeSetting* pRuntimeSetting )
    {
        EE_ASSERT( pRuntimeSetting != nullptr );

        //-------------------------------------------------------------------------

        switch ( pRuntimeSetting->GetType() )
        {
            case RuntimeSetting::Type::Bool:
            {
                auto pSetting = static_cast<RuntimeSettingBool*>( pRuntimeSetting );
                bool value = *pSetting;
                if ( ImGui::Checkbox( pRuntimeSetting->GetName(), &value ) )
                {
                    *pSetting = value;
                }
            }
            break;

            case RuntimeSetting::Type::Int:
            {
                auto pSetting = static_cast<RuntimeSettingInt*>( pRuntimeSetting );
                int32_t value = *pSetting;

                if ( pSetting->HasLimits() )
                {
                    if ( ImGui::SliderInt( pRuntimeSetting->GetName(), &value, pSetting->GetMin(), pSetting->GetMax() ) )
                    {
                        *pSetting = value;
                    }
                }
                else
                {
                    if ( ImGui::InputInt( pRuntimeSetting->GetName(), &value ) )
                    {
                        *pSetting = value;
                    }
                }
            }
            break;

            case RuntimeSetting::Type::Float:
            {
                auto pSetting = static_cast<RuntimeSettingFloat*>( pRuntimeSetting );
                float value = *pSetting;

                if ( pSetting->HasLimits() )
                {
                    if ( ImGui::SliderFloat( pRuntimeSetting->GetName(), &value, pSetting->GetMin(), pSetting->GetMax() ) )
                    {
                        *pSetting = value;
                    }
                }
                else
                {
                    if ( ImGui::InputFloat( pRuntimeSetting->GetName(), &value, 0.1f, 1.0f ) )
                    {
                        *pSetting = value;
                    }
                }
            }
            break;
        }
    }

    bool RuntimeSettingsDebugView::DrawRuntimeSettingsView( UpdateContext const& context )
    {
        auto pSettingsRegistry = context.GetSystem<SettingsRegistry>();
        auto const& RuntimeSettings = pSettingsRegistry->GetAllSettings();

        //-------------------------------------------------------------------------

        bool isRuntimeSettingsWindowOpen = true;

        if ( ImGui::Begin( "Debug Settings", &isRuntimeSettingsWindowOpen ) )
        {
            if ( ImGui::BeginTable( "Settings Table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable ) )
            {
                ImGui::TableSetupColumn( "Channel", ImGuiTableColumnFlags_WidthFixed, 200 );
                ImGui::TableSetupColumn( "Setting", ImGuiTableColumnFlags_WidthStretch );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                //-------------------------------------------------------------------------

                for ( auto const& settingPair : RuntimeSettings )
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::Text( settingPair.second->GetCategory() );

                    ImGui::TableSetColumnIndex( 1 );
                    DrawRuntimeSetting( settingPair.second );
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();

        //-------------------------------------------------------------------------

        return isRuntimeSettingsWindowOpen;
    }

    //-------------------------------------------------------------------------

    RuntimeSettingsDebugView::RuntimeSettingsDebugView()
    {
        m_menus.emplace_back( DebugMenu( "Runtime Settings", [this] ( EntityWorldUpdateContext const& context ) { DrawMenu( context ); } ) );
    }

    void RuntimeSettingsDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
    }
}
#endif