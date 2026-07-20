#include "PerformanceStatsWidget.h"
#include "Engine/UpdateContext.h"
#include "Engine/Render/RenderSystem.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    PerformanceStatsWidget::PerformanceStatsWidget( UpdateContext const& context )
    {
        m_fps = 1.0f / context.GetDeltaTime();
        m_fpsColor = Color::EvaluateRedGreenGradient( Math::RemapRange( m_fps, 0, 100 ), false );
        m_allocatedRAM = Memory::GetTotalAllocatedMemory() / ( 1024.0f * 1024.0f );

        auto pRenderSystem = context.GetSystem<Render::RenderSystem>();
        Render::RHI::Context* pContextRHI = pRenderSystem->GetContextRHI();
        m_allocatedVRAM = Render::RHI::GetTotalAllocatedDeviceMemory( pContextRHI ) / ( 1024.0F * 1024.0F );

        //-------------------------------------------------------------------------

        m_totalSize.x = 0;
        m_totalSize.y = ImGui::GetFrameHeight() - ( s_verticalMargin * 2 );

        {
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );

                ImVec2 const characterSize = ImGui::CalcTextSize( "0" );
                m_windowPadding = ImVec2( 0, Math::Floor( ( m_totalSize.y - characterSize.y ) / 2 ) );

                m_totalSize.x += s_startTextOffset;

                //-------------------------------------------------------------------------

                m_str_FPS.sprintf( "%4.0f", m_fps );
                m_fpsExtraSpacing = ( m_fps < 100 ) ? characterSize.x : 0;
                m_totalSize.x += ImGui::CalcTextSize( "FPS:" ).x;
                m_totalSize.x += ImGui::CalcTextSize( m_str_FPS.c_str() ).x;
                m_totalSize.x += m_fpsExtraSpacing;

                //-------------------------------------------------------------------------

                m_totalSize.x += s_individualStatSpacing;
                m_str_RAM.sprintf( "RAM:%6.1fMB", m_allocatedRAM );
                m_totalSize.x += ImGui::CalcTextSize( m_str_RAM.c_str() ).x;

                //-------------------------------------------------------------------------

                m_totalSize.x += s_individualStatSpacing;
                m_str_VRAM.sprintf( "VRAM:%6.1fMB", m_allocatedVRAM );
                m_totalSize.x += ImGui::CalcTextSize( m_str_VRAM.c_str() ).x;
            }

            //-------------------------------------------------------------------------

            m_totalSize.x += ( s_profilerButtonSpacing * 2 );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 0, 0 ) );
            m_profilerButtonSize.x = ImGuiX::CalculateButtonWidth( EE_ICON_FIRE );
            ImGui::PopStyleVar( 1 );
            m_totalSize.x += m_profilerButtonSize.x;
        }
    }

    void PerformanceStatsWidget::Draw() const
    {
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + s_verticalMargin );

        ImGui::PushStyleColor( ImGuiCol_ChildBg, ImGuiX::Style::s_colorGray8 );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 3.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, m_windowPadding );
        if ( ImGui::BeginChild( "SAD", m_totalSize, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse ) )
        {
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );

                ImGui::SameLine( 0, s_startTextOffset );
                ImGui::Text( "FPS:" );
                ImGui::SameLine( 0, m_fpsExtraSpacing );
                ImGui::TextColored( m_fpsColor, m_str_FPS.c_str() );

                ImGui::SameLine( 0, s_individualStatSpacing );
                ImGui::Text( m_str_RAM.c_str() );

                ImGui::SameLine( 0, s_individualStatSpacing );
                ImGui::Text( m_str_VRAM.c_str() );
            }

            ImGui::SameLine( 0, s_profilerButtonSpacing );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 0, 0 ) );
            if ( ImGuiX::FlatButton( EE_ICON_FIRE"##Profiler", m_profilerButtonSize, Colors::Orange ) )
            {
                Profiling::OpenProfiler();
            }
            ImGui::PopStyleVar( 1 );
            ImGuiX::ItemTooltip( "Open Profiler" );
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar( 2 );
    }
}
#endif