#include "ImguiGizmo_Base.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    Color const GizmoBase::Style::s_axisColors[3] = { Colors::Red, Colors::Lime, Colors::DodgerBlue };

    //-------------------------------------------------------------------------

    GizmoState GizmoBase::UpdateAndDraw( Vector const& positionWS, Quaternion const& rotationWS, Viewport const& viewport, char const* pOptionalLabel )
    {
        ImGuiIO& io = ImGui::GetIO();
        auto pWindow = ImGui::GetCurrentWindow();
        GetStyle()->SetScale( ImGuiX::Style::GetMaxDpiScale() );

        //-------------------------------------------------------------------------

        Context ctx( viewport );
        ctx.m_viewDirectionWS = viewport.GetViewForwardDirection();
        ctx.m_viewPositionWS = viewport.GetViewPosition();
        ctx.m_positionWS = positionWS;
        ctx.m_positionSS = ctx.m_viewport.WorldSpaceToScreenSpace( positionWS );
        ctx.m_rotationWS = rotationWS;
        ctx.m_mousePositionSS = Vector( io.MousePos.x, io.MousePos.y, 0, 1.0f );
        ctx.m_mouseDeltaSS = io.MouseDelta;
        ctx.m_isMouseInViewport = ctx.m_viewport.ContainsPointScreenSpace( io.MousePos );

        //-------------------------------------------------------------------------

        if ( pOptionalLabel != nullptr )
        {
            ImDrawList* pDrawList = ImGui::GetWindowDrawList();
            ImVec2 const textSize = ImGui::CalcTextSize( pOptionalLabel );
            ImVec2 const textPos = ImVec2( ctx.m_positionSS ) + ImVec2( -textSize.x / 2, textSize.y / 2 );
            pDrawList->AddText( textPos, Colors::White, pOptionalLabel );
        }

        //-------------------------------------------------------------------------

        bool const isVisible = viewport.GetViewVolume().Contains( positionWS );
        if ( isVisible )
        {
            SetupManipulators( ctx );
            if ( !IsManipulating() )
            {
                UpdateHoverState( ctx );
            }
            DrawManipulators( ctx );
        }

        //-------------------------------------------------------------------------

        GizmoState result = GizmoState::None;

        if ( IsManipulating() )
        {
            if ( ImGui::IsMouseReleased( ImGuiMouseButton_Left ) || !isVisible )
            {
                StopManipulating( ctx );
                result = GizmoState::StoppedManipulating;
            }
            else // Update
            {
                Manipulate( ctx );
                result = GizmoState::Manipulating;
            }
        }
        else if ( TryStartManipulating( ctx ) )
        {
            result = GizmoState::StartedManipulating;
        }

        DrawDebug( ctx );
        return result;
    }

    float GizmoBase::GetPixelLength( Context const& ctx, Vector const startWS, Vector const endWS )
    {
        Vector startPS = ctx.m_viewport.WorldSpaceToScreenSpace( startWS );
        Vector endPS = ctx.m_viewport.WorldSpaceToScreenSpace( endWS );
        return ( endPS - startPS ).GetLength2();
    }

    float GizmoBase::PixelHeightToWorldHeight( Context const& ctx, Vector const& position, float pixelHeight )
    {
        float const percentageOfScreenY = pixelHeight / ctx.m_viewport.GetDimensions().m_y;
        return percentageOfScreenY * ctx.m_viewport.GetViewVolume().GetViewForwardVector().GetDot3( position - ctx.m_viewport.GetViewPosition() );
    }

    float GizmoBase::PixelWidthToWorldHeight( Context const& ctx, Vector const& position, float pixelWidth )
    {
        float const percentageOfScreenX = pixelWidth / ctx.m_viewport.GetDimensions().m_x;
        return percentageOfScreenX * ctx.m_viewport.GetViewVolume().GetViewForwardVector().GetDot3( position - ctx.m_viewport.GetViewPosition() );
    }
}
#endif