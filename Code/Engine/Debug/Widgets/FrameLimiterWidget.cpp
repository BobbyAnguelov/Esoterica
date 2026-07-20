#include "FrameLimiterWidget.h"
#include "Engine/UpdateContext.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    FrameLimiterWidget::FrameLimiterWidget( float width )
    {
        if ( width > 0 )
        {
            m_width = width;
        }
        else
        {
            m_width = ImGuiX::CalculateButtonWidth( EE_ICON_CAR_SPEED_LIMITER );
        }
    }

    void FrameLimiterWidget::Draw( UpdateContext& context ) const
    {
        ImGui::PushStyleColor( ImGuiCol_FrameBg, 0x00000000 );
        ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImGui::GetColorU32( ImGuiCol_ButtonHovered ) );
        ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImGui::GetColorU32( ImGuiCol_ButtonActive ) );
        ImGui::SetNextItemWidth( m_width );
        if ( ImGui::BeginCombo( "##FLC", EE_ICON_CAR_SPEED_LIMITER, ImGuiComboFlags_NoArrowButton ) )
        {
            ImGui::PopStyleColor( 3 );

            bool noLimit = !context.HasFrameRateLimit();
            if ( ImGui::MenuItem( "None", nullptr, &noLimit ) )
            {
                context.SetFrameRateLimit( 0.0f );
            }

            bool is15FPS = context.HasFrameRateLimit() && context.GetFrameRateLimit() == 15.0f;
            if ( ImGui::MenuItem( "15 FPS", nullptr, &is15FPS ) )
            {
                context.SetFrameRateLimit( 15.0f );
            }

            bool is30FPS = context.HasFrameRateLimit() && context.GetFrameRateLimit() == 30.0f;
            if ( ImGui::MenuItem( "30 FPS", nullptr, &is30FPS ) )
            {
                context.SetFrameRateLimit( 30.0f );
            }

            bool is60FPS = context.HasFrameRateLimit() && context.GetFrameRateLimit() == 60.0f;
            if ( ImGui::MenuItem( "60 FPS", nullptr, &is60FPS ) )
            {
                context.SetFrameRateLimit( 60.0f );
            }

            bool is120FPS = context.HasFrameRateLimit() && context.GetFrameRateLimit() == 120.0f;
            if ( ImGui::MenuItem( "120 FPS", nullptr, &is120FPS ) )
            {
                context.SetFrameRateLimit( 120.0f );
            }

            bool is144FPS = context.HasFrameRateLimit() && context.GetFrameRateLimit() == 144.0f;
            if ( ImGui::MenuItem( "144 FPS", nullptr, &is144FPS ) )
            {
                context.SetFrameRateLimit( 144.0f );
            }

            ImGui::EndCombo();
        }
        else
        {
            ImGui::PopStyleColor( 3 );
        }
        ImGuiX::ItemTooltip( "Frame Limiter" );
    }
}
#endif