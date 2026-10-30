#include "Gizmo_Base.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Imgui/ImguiStyle.h"
#include "Base/Math/Intersection.h"

//-------------------------------------------------------------------------

namespace EE
{
    Color const GizmoBase::Style::s_axisColors[3] = { Colors::Red, Colors::Lime, Colors::DodgerBlue };

    void GizmoBase::Style::SetScale( float scale )
    {
        Reset();

        m_originCircleRadius *= scale;
        m_axisLength *= scale;
        m_lineThickness *= scale;
        m_axisAdditionalHoverBorder *= scale;
    }

    //-------------------------------------------------------------------------

    GizmoState GizmoBase::UpdateAndDraw( Vector const& positionWS, Quaternion const& rotationWS, Viewport const& viewport, bool isFocused, char const* pOptionalLabel )
    {
        // Mouse input still comes from imgui, only the drawing goes through the debug draw system
        ImGuiIO const& io = ImGui::GetIO();
        GetStyle()->SetScale( ImGuiX::Style::GetMaxDpiScale() );

        //-------------------------------------------------------------------------

        Context ctx( viewport );
        ctx.m_viewDirectionWS = viewport.GetViewForwardDirection();
        ctx.m_viewPositionWS = viewport.GetViewPosition();
        ctx.m_positionWS = positionWS;
        ctx.m_positionSS = ctx.WorldSpaceToScreenSpace( positionWS );
        ctx.m_rotationWS = rotationWS;
        ctx.m_mousePositionSS = Vector( io.MousePos.x, io.MousePos.y, 0, 1.0f );
        ctx.m_mouseDeltaSS = Vector( io.MouseDelta.x, io.MouseDelta.y, 0, 0 );
        ctx.m_worldUnitsPerPixel = PixelHeightToWorldHeight( ctx, positionWS, 1.0f );
        ctx.m_isMouseInViewport = ctx.m_viewport.ContainsPointScreenSpace( Float2( io.MousePos.x, io.MousePos.y ) );

        //-------------------------------------------------------------------------

        bool const isVisible = viewport.GetViewVolume().Contains( positionWS );
        if ( isVisible )
        {
            SetupManipulators( ctx );
            if ( isFocused && !IsManipulating() )
            {
                UpdateHoverState( ctx );
            }
            DrawManipulators( ctx );

            if ( pOptionalLabel != nullptr )
            {
                auto drawingCtx = viewport.GetDebugDrawContext();
                Vector const labelPositionWS = positionWS - ( viewport.GetViewUpDirection() * ctx.PixelsToWorld( GetStyle()->m_originCircleRadius * 2 ) );
                drawingCtx.DrawText3D( labelPositionWS, pOptionalLabel, Colors::White, DebugFont::Normal, DebugTextAlign::TopCenter );
            }
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


    uint64_t GizmoBase::CalculateUniqueHitTestID( uint64_t baseHitTestID ) const
    {
        uint64_t const data[2] = { (uint64_t) this, baseHitTestID };
        return Hash::GetHash64( data, sizeof( uint64_t ) * 2 );
    }

    float GizmoBase::PixelHeightToWorldHeight( Context const& ctx, Vector const& position, float pixelHeight )
    {
        Math::ViewVolume const& viewVolume = ctx.m_viewport.GetViewVolume();
        float const percentageOfScreenY = pixelHeight / ctx.m_viewport.GetDimensions().m_y;
        float const depth = viewVolume.GetViewForwardVector().GetDot3( position - ctx.m_viewport.GetViewPosition() );
        return percentageOfScreenY * depth * ( 2.0f * Math::Tan( viewVolume.GetVerticalFOV().ToFloat() * 0.5f ) );
    }

    Vector GizmoBase::ScreenSpaceToWorldSpaceAtGizmoDepth( Context const& ctx, Vector const& pointSS )
    {
        Plane const gizmoPlane = Plane::FromNormalAndPoint( -ctx.m_viewDirectionWS, ctx.m_positionWS );
        LineSegment const worldRay = ctx.m_viewport.ScreenSpaceToWorldSpace( pointSS.ToFloat2() );
        Math::RayPlaneResult const result = Math::IntersectLinePlane( worldRay.GetStartPoint(), worldRay.GetEndPoint(), gizmoPlane );
        return result ? result.m_intersectionPoint : ctx.m_positionWS;
    }

    //-------------------------------------------------------------------------

    void GizmoBase::DrawOrigin( Context const& ctx, Color color )
    {
        auto drawingCtx = ctx.m_viewport.GetDebugDrawContext();
        drawingCtx.DrawSphere( ctx.m_positionWS.ToFloat3(), ctx.PixelsToWorld( GetStyle()->m_originCircleRadius ), color );
    }

    void GizmoBase::DrawPolyLine( DebugDrawContext& drawingCtx, PointList const& pointsWS, Color color, float thickness )
    {
        for ( int32_t i = 1; i < (int32_t) pointsWS.size(); i++ )
        {
            drawingCtx.DrawLine( pointsWS[i - 1], pointsWS[i], color, thickness );
        }
    }

    void GizmoBase::DrawInfiniteLine( Context const& ctx, Vector const& directionWS, Color color, float thickness )
    {
        FloatRange const depthRange = ctx.m_viewport.GetViewVolume().GetDepthRange();

        Vector startWS = ctx.m_positionWS - ( directionWS * depthRange.m_end );
        Vector endWS = ctx.m_positionWS + ( directionWS * depthRange.m_end );

        // Clip the line against the near plane so that we never try to draw anything behind the camera
        //-------------------------------------------------------------------------

        float const startDepth = ctx.m_viewDirectionWS.GetDot3( startWS - ctx.m_viewPositionWS );
        float const endDepth = ctx.m_viewDirectionWS.GetDot3( endWS - ctx.m_viewPositionWS );

        if ( startDepth < depthRange.m_begin && endDepth < depthRange.m_begin )
        {
            return;
        }

        if ( startDepth < depthRange.m_begin )
        {
            startWS = Vector::Lerp( startWS, endWS, ( depthRange.m_begin - startDepth ) / ( endDepth - startDepth ) );
        }
        else if ( endDepth < depthRange.m_begin )
        {
            endWS = Vector::Lerp( endWS, startWS, ( depthRange.m_begin - endDepth ) / ( startDepth - endDepth ) );
        }

        //-------------------------------------------------------------------------

        auto drawingCtx = ctx.m_viewport.GetDebugDrawContext();
        drawingCtx.DrawLine( startWS, endWS, color, thickness );
    }

    void GizmoBase::DrawDashedLine( Context const& ctx, Vector const& startWS, Vector const& endWS, Color color, float thickness, float dashLength, float gapLength )
    {
        Vector lineDirWS;
        float lineLengthWS = 0.0f;
        ( endWS - startWS ).ToDirectionAndLength3( lineDirWS, lineLengthWS );

        if ( Math::IsNearZero( lineLengthWS ) )
        {
            return;
        }

        float const dashLengthWS = ctx.PixelsToWorld( dashLength );
        float const stepLengthWS = ctx.PixelsToWorld( dashLength + gapLength );

        auto drawingCtx = ctx.m_viewport.GetDebugDrawContext();
        for ( float distance = 0.0f; distance < lineLengthWS; distance += stepLengthWS )
        {
            Vector const dashStartWS = startWS + ( lineDirWS * distance );
            Vector const dashEndWS = startWS + ( lineDirWS * Math::Min( distance + dashLengthWS, lineLengthWS ) );
            drawingCtx.DrawLine( dashStartWS, dashEndWS, color, thickness );
        }
    }

    void GizmoBase::DrawDoubleSidedTriangle( DebugDrawContext& drawingCtx, Vector const& v0, Vector const& v1, Vector const& v2, Color color )
    {
        drawingCtx.DrawTriangle( v0, v1, v2, color );
        drawingCtx.DrawTriangle( v0, v2, v1, color );
    }

    void GizmoBase::DrawSolidCone( DebugDrawContext& drawingCtx, Vector const& baseCenterWS, Vector const& directionWS, float length, float radius, Color color )
    {
        constexpr static int32_t const numSides = 12;

        Vector const referenceAxisWS = ( Math::Abs( directionWS.GetZ() ) < 0.9f ) ? Vector::UnitZ : Vector::UnitX;
        Vector const radiusVector = directionWS.Cross3( referenceAxisWS ).GetNormalized3() * radius;
        Vector const tipWS = baseCenterWS + ( directionWS * length );

        Vector previousPointWS = baseCenterWS + radiusVector;
        for ( int32_t i = 1; i <= numSides; i++ )
        {
            Quaternion const rot( directionWS, Radians( ( Math::TwoPi / numSides ) * i ) );
            Vector const pointWS = baseCenterWS + rot.RotateVector( radiusVector );
            DrawDoubleSidedTriangle( drawingCtx, tipWS, previousPointWS, pointWS, color );
            DrawDoubleSidedTriangle( drawingCtx, baseCenterWS, pointWS, previousPointWS, color );
            previousPointWS = pointWS;
        }
    }
}
