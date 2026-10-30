#include "Gizmo_Translate.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Math/MathUtils.h"
#include "Base/Math/Intersection.h"
#include "Base/Math/Triangle.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------

namespace EE
{
    void TranslationGizmo::Style::SetScale( float scale )
    {
        GizmoBase::Style::SetScale( scale );

        m_axisArrowThickness *= scale;
        m_axisPlaneManipulatorBorderThickness *= scale;
        m_axisOffsetFromOrigin *= scale;
        m_minimumRequiredAreaForPlaneManipulators *= scale;
    }

    //-------------------------------------------------------------------------

    void TranslationGizmo::SetupManipulators( Context const& ctx )
    {
        float const axesScale = ctx.PixelsToWorld( m_style.m_axisLength );

        auto CalculateAxisValues = [this, &ctx] ( int32_t axisIdx, float scale )
        {
            AxisManipulator& axis = m_axes[axisIdx];

            axis.m_color = GizmoBase::Style::s_axisColors[axisIdx];
            axis.m_scaledAxisDirWS = axis.m_axisDirWS * scale;
            axis.m_axisEndWS = ctx.m_positionWS + axis.m_scaledAxisDirWS;
            axis.m_axisEndSS = ctx.WorldSpaceToScreenSpace( axis.m_axisEndWS );
            ( axis.m_axisEndSS - ctx.m_positionSS ).ToDirectionAndLength2( axis.m_axisDirSS, axis.m_axisLengthSS );
            axis.m_axisEndInfSS = ctx.m_positionSS + ( axis.m_axisDirSS * 100000 );

            // Offset the start of the axis from the origin
            float const startOffsetRatio = ( axis.m_axisLengthSS > m_style.m_axisOffsetFromOrigin ) ? ( m_style.m_axisOffsetFromOrigin / axis.m_axisLengthSS ) : 1.0f;
            axis.m_axisStartWS = ctx.m_positionWS + ( axis.m_scaledAxisDirWS * startOffsetRatio );
            axis.m_axisStartSS = ctx.WorldSpaceToScreenSpace( axis.m_axisStartWS );

            axis.m_manipulator = (Manipulator) ( axisIdx + 1 );
            axis.m_isHovered = ( m_activeManipulator == axis.m_manipulator );
        };

        // Calculate initial axis screen space points
        //-------------------------------------------------------------------------

        m_axes[0].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitX : ctx.m_rotationWS.RotateVector( Vector::UnitX );
        m_axes[0].m_hitTestID = CalculateUniqueHitTestID( s_hitTestID_axisX );

        m_axes[1].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitY : ctx.m_rotationWS.RotateVector( Vector::UnitY );
        m_axes[1].m_hitTestID = CalculateUniqueHitTestID( s_hitTestID_axisY );

        m_axes[2].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitZ : ctx.m_rotationWS.RotateVector( Vector::UnitZ );
        m_axes[2].m_hitTestID = CalculateUniqueHitTestID( s_hitTestID_axisZ );

        for ( int32_t i = 0; i < 3; i++ )
        {
            m_axes[i].m_axisDirWS.Normalize3();
            m_axes[i].m_isFlipped = false;
            CalculateAxisValues( i, axesScale );
        }

        // Calculate flipped axes
        //-------------------------------------------------------------------------

        auto TryFlipAxes = [this, &ctx, &CalculateAxisValues, axesScale] ( int32_t axisIdx0, int32_t axisIdx1 )
        {
            if ( Math::CalculateAngleBetweenUnitVectors( m_axes[axisIdx0].m_axisDirSS, m_axes[axisIdx1].m_axisDirSS ) < Degrees( 30 ) )
            {
                Radians const deltaViewAngle0 = Math::CalculateAngleBetweenUnitVectors( m_axes[axisIdx0].m_axisDirWS, ctx.m_viewDirectionWS );
                Radians const deltaViewAngle1 = Math::CalculateAngleBetweenUnitVectors( m_axes[axisIdx1].m_axisDirWS, ctx.m_viewDirectionWS );
                int32_t const axisToFlipIdx = ( deltaViewAngle0 <= deltaViewAngle1 ) ? axisIdx0 : axisIdx1;

                m_axes[axisToFlipIdx].m_isFlipped = true;
                m_axes[axisToFlipIdx].m_axisDirWS.Negate();
                CalculateAxisValues( axisToFlipIdx, axesScale );
                return true;
            }

            return false;
        };

        if ( !TryFlipAxes( 0, 1 ) )
        {
            if ( !TryFlipAxes( 1, 2 ) )
            {
                TryFlipAxes( 0, 2 );
            }
        }

        // Scale axes
        //-------------------------------------------------------------------------
        // Rescale the gizmo so that the longest axis on screen is exactly the desired axis length

        float const highestLength = Math::Max( m_axes[0].m_axisLengthSS, Math::Max( m_axes[1].m_axisLengthSS, m_axes[2].m_axisLengthSS ) );
        if ( !Math::IsNearZero( highestLength ) )
        {
            float const rescaledAxesScale = axesScale * ( m_style.m_axisLength / highestLength );
            CalculateAxisValues( 0, rescaledAxesScale );
            CalculateAxisValues( 1, rescaledAxesScale );
            CalculateAxisValues( 2, rescaledAxesScale );
        }

        // Plane manipulators
        //-------------------------------------------------------------------------

        // The two axes that define the plane and then the plane normal axis
        static int32_t const planeAxes[3][3] = { { 0, 1, 2 }, { 1, 2, 0 }, { 0, 2, 1 } };
        static Manipulator const planeManipulators[3] = { Manipulator::TranslateXY, Manipulator::TranslateYZ, Manipulator::TranslateXZ };
        static uint64_t const planeHitTestIDs[3] = { s_hitTestID_axisXY, s_hitTestID_axisYZ, s_hitTestID_axisXZ };

        float const nearRatio = m_style.m_axisPlaneManipulatorOffsetRatio;
        float const farRatio = m_style.m_axisPlaneManipulatorOffsetRatio + m_style.m_axisPlaneManipulatorWidthRatio;

        for ( int32_t p = 0; p < 3; p++ )
        {
            PlaneManipulator& plane = m_planes[p];
            AxisManipulator const& axis0 = m_axes[planeAxes[p][0]];
            AxisManipulator const& axis1 = m_axes[planeAxes[p][1]];
            AxisManipulator const& normalAxis = m_axes[planeAxes[p][2]];

            plane.m_manipulationPlaneAxis = normalAxis.m_axisDirWS;
            plane.m_manipulator = planeManipulators[p];
            plane.m_hitTestID = CalculateUniqueHitTestID( planeHitTestIDs[p] );
            plane.m_color = GizmoBase::Style::s_axisColors[planeAxes[p][2]];
            plane.m_isHovered = ( m_activeManipulator == plane.m_manipulator );

            plane.m_pointsWS[0] = ctx.m_positionWS + ( axis0.m_scaledAxisDirWS * nearRatio ) + ( axis1.m_scaledAxisDirWS * nearRatio );
            plane.m_pointsWS[1] = ctx.m_positionWS + ( axis0.m_scaledAxisDirWS * farRatio ) + ( axis1.m_scaledAxisDirWS * nearRatio );
            plane.m_pointsWS[2] = ctx.m_positionWS + ( axis0.m_scaledAxisDirWS * farRatio ) + ( axis1.m_scaledAxisDirWS * farRatio );
            plane.m_pointsWS[3] = ctx.m_positionWS + ( axis0.m_scaledAxisDirWS * nearRatio ) + ( axis1.m_scaledAxisDirWS * farRatio );

            Vector pointsSS[4];
            for ( int32_t i = 0; i < 4; i++ )
            {
                pointsSS[i] = ctx.WorldSpaceToScreenSpace( plane.m_pointsWS[i] );
            }

            // Hide the manipulator if it is too small on screen
            Math::Triangle const t0( pointsSS[0], pointsSS[1], pointsSS[2] );
            Math::Triangle const t1( pointsSS[2], pointsSS[3], pointsSS[0] );
            plane.m_isHidden = ( t0.GetArea() + t1.GetArea() ) <= m_style.m_minimumRequiredAreaForPlaneManipulators;

            // Hide the manipulator if the plane normal axis overlaps it on screen
            if ( !plane.m_isHidden )
            {
                LineSegment const axisLine( normalAxis.m_axisStartSS, normalAxis.m_axisEndInfSS );
                for ( int32_t i = 0; i < 4; i++ )
                {
                    LineSegment const planeEdge( pointsSS[i], pointsSS[( i + 1 ) % 4] );
                    if ( Math::IntersectLineLine( axisLine, planeEdge ) )
                    {
                        plane.m_isHidden = true;
                        break;
                    }
                }
            }
        }
    }

    void TranslationGizmo::UpdateHoverState( Context const& ctx )
    {
        auto const& pickingData = ctx.m_viewport.GetPickingData();

        bool isAnyAxisHovered = false;
        bool isAnyPlaneHovered = false;

        if ( ctx.m_isMouseInViewport && !pickingData.empty() )
        {
            for ( AxisManipulator& axis : m_axes )
            {
                if ( axis.m_hitTestID == pickingData[0].m_primaryID )
                {
                    axis.m_isHovered = true;
                    isAnyAxisHovered = true;
                }
            }

            //-------------------------------------------------------------------------

            for ( PlaneManipulator& plane : m_planes )
            {
                if ( plane.m_hitTestID == pickingData[0].m_primaryID )
                {
                    plane.m_isHovered = true;
                    isAnyPlaneHovered = true;
                }
            }
        }

        m_isAnyManipulatorHovered = isAnyAxisHovered || isAnyPlaneHovered;
    }

    void TranslationGizmo::DrawManipulators( Context const& ctx )
    {
        auto drawingCtx = ctx.m_viewport.GetDebugDrawContext();

        // Update colors
        //-------------------------------------------------------------------------

        auto UpdateColor = [this] ( Color& color, Manipulator manipulator, bool isHovered )
        {
            bool const isHighlighted = IsManipulating() ? ( m_activeManipulator == manipulator ) : isHovered;
            if ( !isHighlighted )
            {
                color = color.GetScaledColor( Style::s_dimColorScale );
            }
        };

        for ( AxisManipulator& axis : m_axes )
        {
            UpdateColor( axis.m_color, axis.m_manipulator, axis.m_isHovered );
        }

        for ( PlaneManipulator& plane : m_planes )
        {
            UpdateColor( plane.m_color, plane.m_manipulator, plane.m_isHovered );
        }

        // Axes
        //-------------------------------------------------------------------------

        static Manipulator const manipulatorsPerAxis[3][3] =
        {
            { Manipulator::TranslateX, Manipulator::TranslateXY, Manipulator::TranslateXZ },
            { Manipulator::TranslateY, Manipulator::TranslateYZ, Manipulator::TranslateXY },
            { Manipulator::TranslateZ, Manipulator::TranslateXZ, Manipulator::TranslateYZ },
        };

        float const arrowLengthWS = ctx.PixelsToWorld( m_style.GetAxisArrowLength() );
        float const arrowRadiusWS = ctx.PixelsToWorld( m_style.m_axisArrowThickness );

        for ( int32_t i = 0; i < 3; i++ )
        {
            AxisManipulator const& axis = m_axes[i];

            drawingCtx.SetHitTestID( axis.m_hitTestID );

            // Draw the full manipulation axis for any active manipulator that uses this axis
            if ( m_activeManipulator == manipulatorsPerAxis[i][0] || m_activeManipulator == manipulatorsPerAxis[i][1] || m_activeManipulator == manipulatorsPerAxis[i][2] )
            {
                DrawInfiniteLine( ctx, axis.m_axisDirWS, axis.m_color, m_style.m_lineThickness / 2.0f );
            }

            if ( axis.m_isFlipped )
            {
                DrawDashedLine( ctx, axis.m_axisStartWS, axis.m_axisEndWS, axis.m_color, m_style.m_lineThickness );
            }
            else
            {
                drawingCtx.DrawLine( axis.m_axisStartWS, axis.m_axisEndWS, axis.m_color, m_style.m_lineThickness );
            }

            // Draw axis caps
            DrawSolidCone( drawingCtx, axis.m_axisEndWS, axis.m_axisDirWS, arrowLengthWS, arrowRadiusWS, axis.m_color );

            drawingCtx.ClearHitTestID();
        }

        // Plane Manipulators
        //-------------------------------------------------------------------------

        for ( PlaneManipulator const& plane : m_planes )
        {
            if ( plane.m_isHidden )
            {
                continue;
            }

            drawingCtx.SetHitTestID( plane.m_hitTestID );

            Color const fillColor = plane.m_color.GetAlphaVersion( Style::s_planeManipulatorAlpha );
            DrawDoubleSidedTriangle( drawingCtx, plane.m_pointsWS[0], plane.m_pointsWS[1], plane.m_pointsWS[2], fillColor );
            DrawDoubleSidedTriangle( drawingCtx, plane.m_pointsWS[2], plane.m_pointsWS[3], plane.m_pointsWS[0], fillColor );

            for ( int32_t i = 0; i < 4; i++ )
            {
                drawingCtx.DrawLine( plane.m_pointsWS[i], plane.m_pointsWS[( i + 1 ) % 4], plane.m_color, m_style.m_axisPlaneManipulatorBorderThickness );
            }

            drawingCtx.ClearHitTestID();
        }

        // Draw origin sphere
        //-------------------------------------------------------------------------

        Color originColor = Style::s_defaultColor;

        switch ( m_activeManipulator )
        {
            case Manipulator::TranslateX:
            case Manipulator::TranslateYZ:
            {
                originColor = Style::s_axisColors[0];
            }
            break;

            case Manipulator::TranslateY:
            case Manipulator::TranslateXZ:
            {
                originColor = Style::s_axisColors[1];
            }
            break;

            case Manipulator::TranslateZ:
            case Manipulator::TranslateXY:
            {
                originColor = Style::s_axisColors[2];
            }
            break;

            default:
            break;
        }

        DrawOrigin( ctx, originColor );
    }

    //-------------------------------------------------------------------------

    bool TranslationGizmo::TryGetProjectedManipulationPoint( Context const& ctx, AxisManipulator const& axis, Vector const& axisOriginPosWS, Vector const& pointSS, Vector& projectManipulationPoint )
    {
        LineSegment const projectedPointRay = ctx.m_viewport.ScreenSpaceToWorldSpace( pointSS.ToFloat2() );
        Math::RayPlaneResult result = Math::IntersectLinePlane( projectedPointRay.GetStartPoint(), projectedPointRay.GetEndPoint(), m_translationPlane );
        if ( result )
        {
            projectManipulationPoint = result.m_intersectionPoint;
            Vector const projectedAxisPointWS = m_translationPlane.ProjectPoint( axisOriginPosWS + axis.m_axisDirWS );
            Line const projectedAxisWS = Line( Line::CtorStartEnd(), axisOriginPosWS, projectedAxisPointWS );
            projectManipulationPoint = projectedAxisWS.GetClosestPoint( projectManipulationPoint );
            return true;
        }

        return false;
    }

    bool TranslationGizmo::TryStartManipulating( Context const& ctx )
    {
        if ( ctx.m_isMouseInViewport && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
        {
            LineSegment const mouseWorldRay = ctx.m_viewport.ScreenSpaceToWorldSpace( ctx.m_mousePositionSS.ToFloat2() );

            // Check Planes
            //-------------------------------------------------------------------------

            for ( int32_t i = 0; i < 3; i++ )
            {
                if ( m_planes[i].m_isHovered )
                {
                    Plane const translationPlane = Plane::FromNormalAndPoint( m_planes[i].m_manipulationPlaneAxis, ctx.m_positionWS );
                    Math::RayPlaneResult const result = Math::IntersectLinePlane( mouseWorldRay.GetStartPoint(), mouseWorldRay.GetEndPoint(), translationPlane );
                    if ( !result )
                    {
                        return false;
                    }

                    m_translationPlane = translationPlane;
                    m_activeManipulator = m_planes[i].m_manipulator;
                    m_manipulationStartPosWS = ctx.m_positionWS;
                    m_previousPosWS = result.m_intersectionPoint;
                    return true;
                }
            }

            // Check Axes
            //-------------------------------------------------------------------------

            for ( int32_t i = 0; i < 3; i++ )
            {
                if ( m_axes[i].m_isHovered )
                {
                    EE_ASSERT( !Math::IsNearEqual( ctx.m_viewDirectionWS.GetDot3( m_axes[i].m_axisDirWS ), 1.0f ) );
                    Vector const orthogonalAxis = m_axes[i].m_axisDirWS.Cross3( ctx.m_viewDirectionWS.GetNegated() ).GetNormalized3();
                    Vector const normalAxis = orthogonalAxis.Cross3( m_axes[i].m_axisDirWS ).GetNormalized3();

                    m_translationPlane = Plane::FromNormalAndPoint( normalAxis, ctx.m_positionWS );

                    if ( TryGetProjectedManipulationPoint( ctx, m_axes[i], ctx.m_positionWS, ctx.m_mousePositionSS, m_previousPosWS ) )
                    {
                        m_activeManipulator = m_axes[i].m_manipulator;
                        m_manipulationStartPosWS = ctx.m_positionWS;
                        return true;
                    }

                    m_translationPlane = Plane();
                    return false;
                }
            }
        }

        return false;
    }

    void TranslationGizmo::Manipulate( Context const& ctx )
    {
        m_deltaWS = Vector::Zero;

        if ( ctx.m_isMouseInViewport )
        {
            // Set the hovered ID so that we disable any click through in the UI
            ImGui::SetHoveredID( ImGui::GetID( "Gizmo" ) );

            if ( !ctx.m_mouseDeltaSS.IsNearZero2() )
            {
                if ( m_debugEnabled )
                {
                    EE_TRACE_MSG( "Plane: %f, %f, %f, %f", m_translationPlane.a, m_translationPlane.b, m_translationPlane.c, m_translationPlane.d );
                }

                Vector projectedPointWS;

                // Calculate delta
                //-------------------------------------------------------------------------

                // Single axis manipulation
                if ( m_activeManipulator >= Manipulator::TranslateX && m_activeManipulator <= Manipulator::TranslateZ )
                {
                    uint32_t const axisIndex = uint8_t( m_activeManipulator ) - uint8_t( Manipulator::TranslateX );
                    if ( !TryGetProjectedManipulationPoint( ctx, m_axes[axisIndex], m_manipulationStartPosWS, ctx.m_mousePositionSS, projectedPointWS ) )
                    {
                        projectedPointWS = m_previousPosWS;
                    }
                }
                else // Plane manipulation
                {
                    LineSegment const mouseWorldRay = ctx.m_viewport.ScreenSpaceToWorldSpace( ctx.m_mousePositionSS.ToFloat2() );
                    auto result = Math::IntersectLinePlane( mouseWorldRay.GetStartPoint(), mouseWorldRay.GetEndPoint(), m_translationPlane );
                    projectedPointWS = result ? result.m_intersectionPoint : m_previousPosWS;
                }

                if ( m_debugEnabled )
                {
                    EE_TRACE_MSG( "Point WS: %f, %f, %f", projectedPointWS.GetX(), projectedPointWS.GetY(), projectedPointWS.GetZ() );

                    auto drawingCtx = ctx.m_viewport.GetDebugDrawContext();
                    drawingCtx.DrawPoint( projectedPointWS, Colors::Yellow, 6.0f );
                    drawingCtx.DrawLine( ctx.m_positionWS, projectedPointWS, Colors::Yellow, 1.0f );
                }

                m_deltaWS = projectedPointWS - m_previousPosWS;

                if ( m_debugEnabled )
                {
                    EE_TRACE_MSG( "Previous Pos WS: %f, %f, %f", m_previousPosWS.GetX(), m_previousPosWS.GetY(), m_previousPosWS.GetZ() );
                    EE_TRACE_MSG( "Delta WS: %f, %f, %f\n\n", m_deltaWS.GetX(), m_deltaWS.GetY(), m_deltaWS.GetZ() );
                }

                m_previousPosWS = projectedPointWS;
            }
        }
    }

    void TranslationGizmo::StopManipulating( Context const& ctx )
    {
        m_manipulationStartPosWS = Vector::Zero;
        m_previousPosWS = Vector::Zero;
        m_deltaWS = Vector::Zero;
        m_translationPlane = Plane();
        m_activeManipulator = Manipulator::None;
        m_isAnyManipulatorHovered = false;
    }

    void TranslationGizmo::DrawDebug( Context const& ctx )
    {
        if ( !m_debugEnabled )
        {
            return;
        }

        static char const* const manipulatorNames[] = { "Not Manipulating", "Translate X", "Translate Y", "Translate Z", "Translate XY", "Translate YZ", "Translate XZ" };

        InlineString str;
        str.sprintf( "%s, D: %.2f, %.2f, %.2f", manipulatorNames[(int32_t) m_activeManipulator], m_deltaWS.GetX(), m_deltaWS.GetY(), m_deltaWS.GetZ() );

        auto drawingCtx = ctx.m_viewport.GetDebugDrawContext();
        drawingCtx.DrawText3D( ctx.m_positionWS, str.c_str(), Colors::HotPink );
    }
}
