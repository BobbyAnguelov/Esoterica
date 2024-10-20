#include "NodeGraph_Comment.h"

//-------------------------------------------------------------------------

namespace EE::NodeGraph
{
    ResizeHandle CommentNode::GetHoveredResizeHandle( DrawContext const& ctx ) const
    {
        float const selectionThreshold = ctx.WindowToCanvas( CommentNode::s_resizeSelectionRadius );
        float const cornerSelectionThreshold = selectionThreshold * 2;

        ImRect nodeRect = GetRect();
        nodeRect.Expand( selectionThreshold / 2 );

        // Corner tests first
        //-------------------------------------------------------------------------

        Vector const mousePos( ctx.m_mouseCanvasPos );
        float const distanceToCornerTL = mousePos.GetDistance2( nodeRect.GetTL() );
        float const distanceToCornerBL = mousePos.GetDistance2( nodeRect.GetBL() );
        float const distanceToCornerTR = mousePos.GetDistance2( nodeRect.GetTR() );
        float const distanceToCornerBR = mousePos.GetDistance2( nodeRect.GetBR() );

        if ( distanceToCornerBR < cornerSelectionThreshold )
        {
            return ResizeHandle::SE;
        }
        else if ( distanceToCornerBL < cornerSelectionThreshold )
        {
            return ResizeHandle::SW;
        }
        else if ( distanceToCornerTR < cornerSelectionThreshold )
        {
            return ResizeHandle::NE;
        }
        else if ( distanceToCornerTL < cornerSelectionThreshold )
        {
            return ResizeHandle::NW;
        }

        // Edge tests
        //-------------------------------------------------------------------------

        ImVec2 const points[4] =
        {
            ImLineClosestPoint( nodeRect.GetTL(), nodeRect.GetTR(), ctx.m_mouseCanvasPos ),
            ImLineClosestPoint( nodeRect.GetBL(), nodeRect.GetBR(), ctx.m_mouseCanvasPos ),
            ImLineClosestPoint( nodeRect.GetTL(), nodeRect.GetBL(), ctx.m_mouseCanvasPos ),
            ImLineClosestPoint( nodeRect.GetTR(), nodeRect.GetBR(), ctx.m_mouseCanvasPos )
        };

        float distancesSq[4] =
        {
            ImLengthSqr( points[0] - ctx.m_mouseCanvasPos ),
            ImLengthSqr( points[1] - ctx.m_mouseCanvasPos ),
            ImLengthSqr( points[2] - ctx.m_mouseCanvasPos ),
            ImLengthSqr( points[3] - ctx.m_mouseCanvasPos )
        };

        float const detectionDistanceSq = Math::Sqr( selectionThreshold );

        if ( distancesSq[0] < detectionDistanceSq )
        {
            return ResizeHandle::N;
        }
        else if ( distancesSq[1] < detectionDistanceSq )
        {
            return ResizeHandle::S;
        }
        else if ( distancesSq[2] < detectionDistanceSq )
        {
            return ResizeHandle::W;
        }
        else if ( distancesSq[3] < detectionDistanceSq )
        {
            return ResizeHandle::E;
        }

        return ResizeHandle::None;
    }

    void CommentNode::AdjustSizeBasedOnMousePosition( DrawContext const& ctx, ResizeHandle handle )
    {
        Float2 const unscaledBoxTL = GetPosition();
        Float2 const unscaledBoxBR = unscaledBoxTL + GetCommentBoxSize();

        switch ( handle )
        {
            case ResizeHandle::N:
            {
                m_canvasPosition.m_y = ctx.m_mouseCanvasPos.y;
                m_commentBoxSize.m_y = Math::Max( s_minBoxDimensions, unscaledBoxBR.m_y - m_canvasPosition.m_y );
                m_canvasPosition.m_y = unscaledBoxBR.m_y - m_commentBoxSize.m_y;
            }
            break;

            case ResizeHandle::NE:
            {
                m_canvasPosition.m_y = ctx.m_mouseCanvasPos.y;
                m_commentBoxSize.m_y = Math::Max( s_minBoxDimensions, unscaledBoxBR.m_y - m_canvasPosition.m_y );
                m_canvasPosition.m_y = unscaledBoxBR.m_y - m_commentBoxSize.m_y;
                m_commentBoxSize.m_x = Math::Max( s_minBoxDimensions, ctx.m_mouseCanvasPos.x - unscaledBoxTL.m_x );
            }
            break;

            case ResizeHandle::E:
            {
                m_commentBoxSize.m_x = Math::Max( s_minBoxDimensions, ctx.m_mouseCanvasPos.x - unscaledBoxTL.m_x );
            }
            break;

            case ResizeHandle::SE:
            {
                m_commentBoxSize.m_x = Math::Max( s_minBoxDimensions, ctx.m_mouseCanvasPos.x - unscaledBoxTL.m_x );
                m_commentBoxSize.m_y = Math::Max( s_minBoxDimensions, ctx.m_mouseCanvasPos.y - unscaledBoxTL.m_y );
            }
            break;

            case ResizeHandle::S:
            {
                m_commentBoxSize.m_y = Math::Max( s_minBoxDimensions, ctx.m_mouseCanvasPos.y - unscaledBoxTL.m_y );
            }
            break;

            case ResizeHandle::SW:
            {
                m_canvasPosition.m_x = ctx.m_mouseCanvasPos.x;
                m_commentBoxSize.m_x = Math::Max( s_minBoxDimensions, unscaledBoxBR.m_x - m_canvasPosition.m_x );
                m_canvasPosition.m_x = unscaledBoxBR.m_x - m_commentBoxSize.m_x;
                m_commentBoxSize.m_y = Math::Max( s_minBoxDimensions, ctx.m_mouseCanvasPos.y - unscaledBoxTL.m_y );
            }
            break;

            case ResizeHandle::W:
            {
                m_canvasPosition.m_x = ctx.m_mouseCanvasPos.x;
                m_commentBoxSize.m_x = Math::Max( s_minBoxDimensions, unscaledBoxBR.m_x - m_canvasPosition.m_x );
                m_canvasPosition.m_x = unscaledBoxBR.m_x - m_commentBoxSize.m_x;
            }
            break;

            case ResizeHandle::NW:
            {
                m_canvasPosition.m_x = ctx.m_mouseCanvasPos.x;
                m_commentBoxSize.m_x = Math::Max( s_minBoxDimensions, unscaledBoxBR.m_x - m_canvasPosition.m_x );
                m_canvasPosition.m_x = unscaledBoxBR.m_x - m_commentBoxSize.m_x;

                m_canvasPosition.m_y = ctx.m_mouseCanvasPos.y;
                m_commentBoxSize.m_y = Math::Max( s_minBoxDimensions, unscaledBoxBR.m_y - m_canvasPosition.m_y );
                m_canvasPosition.m_y = unscaledBoxBR.m_y - m_commentBoxSize.m_y;
            }
            break;

            default:
            EE_UNREACHABLE_CODE();
            break;
        }
    }

    bool CommentNode::DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos )
    {
        // Generate a default palette. The palette will persist and can be edited.
        constexpr static int32_t const paletteSize = 32;
        static bool wasPaletteInitialized = false;
        static ImVec4 palette[paletteSize] = {};
        if ( !wasPaletteInitialized )
        {
            palette[0] = Color( 0xFF4C4C4C ).ToFloat4();

            for ( int32_t n = 1; n < paletteSize; n++ )
            {
                ImGui::ColorConvertHSVtoRGB( n / 31.0f, 0.8f, 0.8f, palette[n].x, palette[n].y, palette[n].z );
            }
            wasPaletteInitialized = true;
        }

        //-------------------------------------------------------------------------

        bool result = false;

        if ( ImGui::BeginMenu( EE_ICON_PALETTE" Color" ) )
        {
            for ( int32_t n = 0; n < paletteSize; n++ )
            {
                ImGui::PushID( n );
                if ( ( n % 8 ) != 0 )
                {
                    ImGui::SameLine( 0.0f, ImGui::GetStyle().ItemSpacing.y );
                }

                ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
                if ( ImGui::ColorButton( "##paletteOption", palette[n], palette_button_flags, ImVec2( 20, 20 ) ) )
                {
                    m_nodeColor = Color( ImVec4( palette[n].x, palette[n].y, palette[n].z, 1.0f ) ); // Preserve alpha!
                    result = true;
                }

                ImGui::PopID();
            }

            ImGui::EndMenu();
        }

        return result;
    }

}