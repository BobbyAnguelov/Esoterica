#include "ImguiGizmo_Translate.h"
#include "Base/Imgui/ImguiStyle.h"
#include "Base/Math/MathUtils.h"
#include "Base/Math/Intersection.h"
#include "EASTL/sort.h"
#include "Base/Math/Triangle.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    void TranslationGizmo::Style::SetScale( float scale )
    {
        *this = TranslationGizmo::Style();

        m_axisLength *= scale;
        m_axisThickness *= scale;
        m_axisArrowThickness *= scale;
        m_axisAdditionalHoverBorder *= scale;
        m_axisPlaneManipulatorBorderThickness *= scale;
        m_originCircleRadius *= scale;
        m_axisOffsetFromOrigin *= scale;
        m_hoverDetectionDistance *= scale;
        m_minimumRequiredAreaForPlaneManipulators *= scale;
    }

    //-------------------------------------------------------------------------

    void TranslationGizmo::SetupManipulators( Context const& ctx )
    {
        Vector const axesScale( PixelHeightToWorldHeight( ctx, ctx.m_positionWS, m_style.m_axisLength ) );

        m_closestAxisToCameraDistance = FLT_MAX;

        auto CalculateAxisValues = [this, &ctx, &axesScale] ( int32_t axisIdx )
        {
            m_axes[axisIdx].m_color = GizmoBase::Style::s_axisColors[axisIdx];
            m_axes[axisIdx].m_scaledAxisDirWS = m_axes[axisIdx].m_axisDirWS * axesScale;
            m_axes[axisIdx].m_axisEndSS = ctx.m_viewport.WorldSpaceToScreenSpace( ctx.m_positionWS + m_axes[axisIdx].m_scaledAxisDirWS );
            ( m_axes[axisIdx].m_axisEndSS - ctx.m_positionSS ).ToDirectionAndLength2( m_axes[axisIdx].m_axisDirSS, m_axes[axisIdx].m_axisLengthSS );
            m_axes[axisIdx].m_axisStartSS = ctx.m_positionSS + ( m_axes[axisIdx].m_axisDirSS * m_style.m_axisOffsetFromOrigin );
            m_axes[axisIdx].m_axisStartInfSS = ctx.m_positionSS - ( m_axes[axisIdx].m_axisDirSS * 100000 );
            m_axes[axisIdx].m_axisEndInfSS = ctx.m_positionSS + ( m_axes[axisIdx].m_axisDirSS * 100000 );
            m_axes[axisIdx].m_manipulator = (Manipulator) ( axisIdx + 1 );
            m_axes[axisIdx].m_isHovered = ( m_activeManipulator == m_axes[axisIdx].m_manipulator );
            m_axes[axisIdx].m_distanceToCamera = ctx.m_viewPositionWS.GetDistance3( ctx.m_positionWS + m_axes[axisIdx].m_scaledAxisDirWS );
        };

        // Calculate initial axis screen space points
        //-------------------------------------------------------------------------

        m_axes[0].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitX : ctx.m_rotationWS.RotateVector( Vector::UnitX );
        m_axes[1].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitY : ctx.m_rotationWS.RotateVector( Vector::UnitY );
        m_axes[2].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitZ : ctx.m_rotationWS.RotateVector( Vector::UnitZ );

        m_axes[0].m_axisDirWS.Normalize3();
        m_axes[1].m_axisDirWS.Normalize3();
        m_axes[2].m_axisDirWS.Normalize3();

        CalculateAxisValues( 0 );
        m_closestAxisToCameraDistance = Math::Min( m_closestAxisToCameraDistance, m_axes[0].m_distanceToCamera );

        CalculateAxisValues( 1 );
        m_closestAxisToCameraDistance = Math::Min( m_closestAxisToCameraDistance, m_axes[1].m_distanceToCamera );

        CalculateAxisValues( 2 );
        m_closestAxisToCameraDistance = Math::Min( m_closestAxisToCameraDistance, m_axes[2].m_distanceToCamera );

        // Calculate flipped axes
        //-------------------------------------------------------------------------

        m_axes[0].m_isFlipped = false;
        m_axes[1].m_isFlipped = false;
        m_axes[2].m_isFlipped = false;

        auto TryFlipAxes = [this, &ctx, &CalculateAxisValues] ( int32_t axisIdx0, int32_t axisIdx1 )
        {
            if ( Math::CalculateAngleBetweenUnitVectors( m_axes[axisIdx0].m_axisDirSS, m_axes[axisIdx1].m_axisDirSS ) < Degrees( 30 ) )
            {
                Radians const deltaViewAngle0 = Math::CalculateAngleBetweenUnitVectors( m_axes[axisIdx0].m_axisDirWS, ctx.m_viewDirectionWS );
                Radians const deltaViewAngle1 = Math::CalculateAngleBetweenUnitVectors( m_axes[axisIdx1].m_axisDirWS, ctx.m_viewDirectionWS );
                if ( deltaViewAngle0 <= deltaViewAngle1 )
                {
                    m_axes[axisIdx0].m_isFlipped = true;
                    m_axes[axisIdx0].m_axisDirWS.Negate();
                    CalculateAxisValues( axisIdx0 );
                }
                else
                {
                    m_axes[axisIdx1].m_isFlipped = true;
                    m_axes[axisIdx1].m_axisDirWS.Negate();
                    CalculateAxisValues( axisIdx1 );
                }

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

        Math::RaySphereResult r0 = Math::IntersectRaySphere( ctx.m_positionSS, m_axes[0].m_axisDirSS, ctx.m_positionSS, m_style.m_axisLength );
        Math::RaySphereResult r1 = Math::IntersectRaySphere( ctx.m_positionSS, m_axes[1].m_axisDirSS, ctx.m_positionSS, m_style.m_axisLength );
        Math::RaySphereResult r2 = Math::IntersectRaySphere( ctx.m_positionSS, m_axes[2].m_axisDirSS, ctx.m_positionSS, m_style.m_axisLength );

        float highestLength = Math::Max( m_axes[0].m_axisLengthSS, Math::Max( m_axes[1].m_axisLengthSS, m_axes[2].m_axisLengthSS ) );
        float ratios[3] = { m_axes[0].m_axisLengthSS / highestLength, m_axes[1].m_axisLengthSS / highestLength, m_axes[2].m_axisLengthSS / highestLength };

        ( r0.m_intersectionPoint0 - ctx.m_positionSS ).ToDirectionAndLength2( m_axes[0].m_axisDirSS, m_axes[0].m_axisLengthSS );
        ( r1.m_intersectionPoint0 - ctx.m_positionSS ).ToDirectionAndLength2( m_axes[1].m_axisDirSS, m_axes[1].m_axisLengthSS );
        ( r2.m_intersectionPoint0 - ctx.m_positionSS ).ToDirectionAndLength2( m_axes[2].m_axisDirSS, m_axes[2].m_axisLengthSS );

        m_axes[0].m_axisLengthSS *= ratios[0];
        m_axes[1].m_axisLengthSS *= ratios[1];
        m_axes[2].m_axisLengthSS *= ratios[2];

        m_axes[0].m_axisEndSS = Vector::MultiplyAdd( m_axes[0].m_axisDirSS, Vector( m_axes[0].m_axisLengthSS ), ctx.m_positionSS );
        m_axes[1].m_axisEndSS = Vector::MultiplyAdd( m_axes[1].m_axisDirSS, Vector( m_axes[1].m_axisLengthSS ), ctx.m_positionSS );
        m_axes[2].m_axisEndSS = Vector::MultiplyAdd( m_axes[2].m_axisDirSS, Vector( m_axes[2].m_axisLengthSS ), ctx.m_positionSS );

        // Plane manipulators
        //-------------------------------------------------------------------------

        static int32_t const planeManipulationAxis[3] = {2, 0, 1};

        m_planes[0].m_manipulationPlaneAxis = m_axes[planeManipulationAxis[0]].m_axisDirWS;
        m_planes[0].m_manipulator = Manipulator::TranslateXY;
        m_planes[0].m_color = GizmoBase::Style::s_axisColors[planeManipulationAxis[0]];
        m_planes[0].m_isHovered = ( m_activeManipulator == m_planes[0].m_manipulator );
        m_planes[0].m_planeDirSS0 = m_axes[0].m_axisDirSS;
        m_planes[0].m_planeDirSS1 = m_axes[1].m_axisDirSS;
        m_planes[0].m_offsetNear0 = m_planes[0].m_planeDirSS0 * m_style.m_axisPlaneManipulatorOffsetRatio * m_axes[0].m_axisLengthSS;
        m_planes[0].m_offsetFar0 = m_planes[0].m_planeDirSS0 * ( m_style.m_axisPlaneManipulatorOffsetRatio + m_style.m_axisPlaneManipulatorWidthRatio ) * m_axes[0].m_axisLengthSS;
        m_planes[0].m_offsetNear1 = m_planes[0].m_planeDirSS1 * m_style.m_axisPlaneManipulatorOffsetRatio * m_axes[1].m_axisLengthSS;
        m_planes[0].m_offsetFar1 = m_planes[0].m_planeDirSS1 * ( m_style.m_axisPlaneManipulatorOffsetRatio + m_style.m_axisPlaneManipulatorWidthRatio ) * m_axes[1].m_axisLengthSS;

        m_planes[1].m_manipulationPlaneAxis = m_axes[planeManipulationAxis[1]].m_axisDirWS;
        m_planes[1].m_manipulator = Manipulator::TranslateYZ;
        m_planes[1].m_color = GizmoBase::Style::s_axisColors[planeManipulationAxis[1]];
        m_planes[1].m_isHovered = ( m_activeManipulator == m_planes[1].m_manipulator );
        m_planes[1].m_planeDirSS0 = m_axes[1].m_axisDirSS;
        m_planes[1].m_planeDirSS1 = m_axes[2].m_axisDirSS;
        m_planes[1].m_offsetNear0 = m_planes[1].m_planeDirSS0 * m_style.m_axisPlaneManipulatorOffsetRatio * m_axes[1].m_axisLengthSS;
        m_planes[1].m_offsetFar0 = m_planes[1].m_planeDirSS0 * ( m_style.m_axisPlaneManipulatorOffsetRatio + m_style.m_axisPlaneManipulatorWidthRatio ) * m_axes[1].m_axisLengthSS;
        m_planes[1].m_offsetNear1 = m_planes[1].m_planeDirSS1 * m_style.m_axisPlaneManipulatorOffsetRatio * m_axes[2].m_axisLengthSS;
        m_planes[1].m_offsetFar1 = m_planes[1].m_planeDirSS1 * ( m_style.m_axisPlaneManipulatorOffsetRatio + m_style.m_axisPlaneManipulatorWidthRatio ) * m_axes[2].m_axisLengthSS;

        m_planes[2].m_manipulationPlaneAxis = m_axes[planeManipulationAxis[2]].m_axisDirWS;
        m_planes[2].m_manipulator = Manipulator::TranslateXZ;
        m_planes[2].m_color = GizmoBase::Style::s_axisColors[planeManipulationAxis[2]];
        m_planes[2].m_isHovered = ( m_activeManipulator == m_planes[2].m_manipulator );
        m_planes[2].m_planeDirSS0 = m_axes[0].m_axisDirSS;
        m_planes[2].m_planeDirSS1 = m_axes[2].m_axisDirSS;
        m_planes[2].m_offsetNear0 = m_planes[2].m_planeDirSS0 * m_style.m_axisPlaneManipulatorOffsetRatio * m_axes[0].m_axisLengthSS;
        m_planes[2].m_offsetFar0 = m_planes[2].m_planeDirSS0 * ( m_style.m_axisPlaneManipulatorOffsetRatio + m_style.m_axisPlaneManipulatorWidthRatio ) * m_axes[0].m_axisLengthSS;
        m_planes[2].m_offsetNear1 = m_planes[2].m_planeDirSS1 * m_style.m_axisPlaneManipulatorOffsetRatio * m_axes[2].m_axisLengthSS;
        m_planes[2].m_offsetFar1 = m_planes[2].m_planeDirSS1 * ( m_style.m_axisPlaneManipulatorOffsetRatio + m_style.m_axisPlaneManipulatorWidthRatio ) * m_axes[2].m_axisLengthSS;

        for ( auto p = 0; p < 3; p++ )
        {
            m_planes[p].m_pointsSS[0] = ctx.m_positionSS + m_planes[p].m_offsetNear0 + m_planes[p].m_offsetNear1;
            m_planes[p].m_pointsSS[1] = ctx.m_positionSS + m_planes[p].m_offsetFar0 + m_planes[p].m_offsetNear1;
            m_planes[p].m_pointsSS[2] = ctx.m_positionSS + m_planes[p].m_offsetFar0 + m_planes[p].m_offsetFar1;
            m_planes[p].m_pointsSS[3] = ctx.m_positionSS + m_planes[p].m_offsetNear0 + m_planes[p].m_offsetFar1;
            m_planes[p].m_pointsSS[4] = m_planes[p].m_pointsSS[0];

            Math::Triangle t0( m_planes[p].m_pointsSS[0], m_planes[p].m_pointsSS[1], m_planes[p].m_pointsSS[2] );
            Math::Triangle t1( m_planes[p].m_pointsSS[2], m_planes[p].m_pointsSS[3], m_planes[p].m_pointsSS[0] );

            float const totalArea = t0.GetArea() + t1.GetArea();
            m_planes[p].m_isHidden = totalArea <= m_style.m_minimumRequiredAreaForPlaneManipulators;

            // Check for intersection
            if ( !m_planes[p].m_isHidden )
            {
                int32_t const manipulationAxis = planeManipulationAxis[p];
                LineSegment const axisLine( m_axes[manipulationAxis].m_axisStartSS, m_axes[manipulationAxis].m_axisEndInfSS );

                LineSegment const planeLine0( m_planes[p].m_pointsSS[0], m_planes[p].m_pointsSS[1] );
                if ( Math::IntersectLineLine( axisLine, planeLine0 ) )
                {
                    m_planes[p].m_isHidden = true;
                    continue;
                }

                LineSegment const planeLine1( m_planes[p].m_pointsSS[1], m_planes[p].m_pointsSS[2] );
                if ( Math::IntersectLineLine( axisLine, planeLine1 ) )
                {
                    m_planes[p].m_isHidden = true;
                    continue;
                }

                LineSegment const planeLine2( m_planes[p].m_pointsSS[2], m_planes[p].m_pointsSS[3] );
                if ( Math::IntersectLineLine( axisLine, planeLine2 ) )
                {
                    m_planes[p].m_isHidden = true;
                    continue;
                }

                LineSegment const planeLine3( m_planes[p].m_pointsSS[3], m_planes[p].m_pointsSS[0] );
                if ( Math::IntersectLineLine( axisLine, planeLine3 ) )
                {
                    m_planes[p].m_isHidden = true;
                    continue;
                }
            }
        }
    }

    void TranslationGizmo::UpdateHoverState( Context const& ctx )
    {
        bool isAnyAxisHovered = false;
        bool isAnyPlaneHovered = false;

        if ( ctx.m_isMouseInViewport )
        {
            int32_t numHoveredAxes = 0;

            if ( !m_axes[0].m_axisStartSS.IsNearEqual3( m_axes[0].m_axisEndSS, m_style.m_hoverDetectionDistance ) )
            {
                LineSegment const axisSegment( m_axes[0].m_axisStartSS, m_axes[0].m_axisEndSS );
                float const mouseToLineDistance = axisSegment.GetDistanceToPoint( ctx.m_mousePositionSS );
                m_axes[0].m_isHovered = mouseToLineDistance < m_style.m_hoverDetectionDistance;
                numHoveredAxes += uint8_t( m_axes[0].m_isHovered );
            }

            if ( !m_axes[1].m_axisStartSS.IsNearEqual3( m_axes[1].m_axisEndSS, m_style.m_hoverDetectionDistance ) )
            {
                LineSegment const axisSegment( m_axes[1].m_axisStartSS, m_axes[1].m_axisEndSS );
                float const mouseToLineDistance = axisSegment.GetDistanceToPoint( ctx.m_mousePositionSS );
                m_axes[1].m_isHovered = mouseToLineDistance < m_style.m_hoverDetectionDistance;
                numHoveredAxes += uint8_t( m_axes[1].m_isHovered );
            }

            if ( !m_axes[2].m_axisStartSS.IsNearEqual3( m_axes[2].m_axisEndSS, m_style.m_hoverDetectionDistance ) )
            {
                LineSegment const axisSegment( m_axes[2].m_axisStartSS, m_axes[2].m_axisEndSS );
                float const mouseToLineDistance = axisSegment.GetDistanceToPoint( ctx.m_mousePositionSS );
                m_axes[2].m_isHovered = mouseToLineDistance < m_style.m_hoverDetectionDistance;
                numHoveredAxes += uint8_t( m_axes[2].m_isHovered );
            }

            // Ensure only the closest axis is selected
            //-------------------------------------------------------------------------

            if ( numHoveredAxes > 1 )
            {
                if ( m_axes[0].m_isHovered && m_axes[0].m_distanceToCamera > m_closestAxisToCameraDistance )
                {
                    m_axes[0].m_isHovered = false;
                }

                if ( m_axes[1].m_isHovered && m_axes[1].m_distanceToCamera > m_closestAxisToCameraDistance )
                {
                    m_axes[1].m_isHovered = false;
                }

                if ( m_axes[2].m_isHovered && m_axes[2].m_distanceToCamera > m_closestAxisToCameraDistance )
                {
                    m_axes[2].m_isHovered = false;
                }
            }

            isAnyAxisHovered = m_axes[0].m_isHovered || m_axes[1].m_isHovered || m_axes[2].m_isHovered;

            // Check planes for hover
            //-------------------------------------------------------------------------

            if ( !isAnyAxisHovered )
            {
                for ( auto p = 0; p < 3; p++ )
                {
                    auto CheckPointInPlaneManipulator = [&] ( PlaneManipulator& planeManipulator )
                    {
                        int i, j, c = 0;
                        for ( i = 0, j = 4 - 1; i < 4; j = i++ )
                        {
                            if (
                                    ( ( planeManipulator.m_pointsSS[i].y > ctx.m_mousePositionSS.GetY() ) != ( planeManipulator.m_pointsSS[j].y > ctx.m_mousePositionSS.GetY() ) ) &&
                                    ( ctx.m_mousePositionSS.GetX() < ( planeManipulator.m_pointsSS[j].x - planeManipulator.m_pointsSS[i].x ) * ( ctx.m_mousePositionSS.GetY() - planeManipulator.m_pointsSS[i].y ) / ( planeManipulator.m_pointsSS[j].y - planeManipulator.m_pointsSS[i].y ) + planeManipulator.m_pointsSS[i].x )
                                )
                            {
                                c = ~c;
                            }
                        }
                        return c;
                    };

                    if ( m_planes[p].m_isHidden )
                    {
                        continue;
                    }

                    if ( CheckPointInPlaneManipulator( m_planes[p] ) )
                    {
                        m_planes[p].m_isHovered = true;
                        isAnyPlaneHovered = true;
                    }
                }
            }
        }

        m_isAnyManipulatorHovered = isAnyAxisHovered || isAnyPlaneHovered;
    }

    void TranslationGizmo::DrawManipulators( Context const& ctx )
    {
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();

        // Sort manipulators back to front
        //-------------------------------------------------------------------------

        TInlineVector<int32_t, 6> drawOrder = { 0, 1, 2, 3, 4, 5 };

        auto Comparator = [&] ( int32_t const& a, int32_t const& b )
        {
            float const va = ( a < 3 ) ? m_axes[a].m_distanceToCamera : m_planes[a - 3].m_distanceToCamera;
            float const vb = ( b < 3 ) ? m_axes[b].m_distanceToCamera : m_planes[b - 3].m_distanceToCamera;
            return va > vb;
        };

        eastl::sort( drawOrder.begin(), drawOrder.end(), Comparator );

        // Update colors
        //-------------------------------------------------------------------------

        auto UpdateAxisColor = [this] ( AxisManipulator &axis )
        {
            if ( IsManipulating() )
            {
                if ( m_activeManipulator != axis.m_manipulator )
                {
                    axis.m_color = axis.m_color.GetScaledColor( Style::s_dimColorScale );
                }
            }
            else // Not manipulating
            {
                if ( !axis.m_isHovered )
                {
                    axis.m_color = axis.m_color.GetScaledColor( Style::s_dimColorScale );
                }
            }
        };

        auto UpdatePlaneColor = [this] ( PlaneManipulator &plane )
        {
            if ( IsManipulating() )
            {
                if ( m_activeManipulator != plane.m_manipulator )
                {
                    plane.m_color = plane.m_color.GetScaledColor( Style::s_dimColorScale );
                }
            }
            else // Not manipulating
            {
                if ( !plane.m_isHovered )
                {
                    plane.m_color = plane.m_color.GetScaledColor( Style::s_dimColorScale );
                }
            }
        };

        UpdateAxisColor( m_axes[0] );
        UpdateAxisColor( m_axes[1] );
        UpdateAxisColor( m_axes[2] );
        UpdatePlaneColor( m_planes[0] );
        UpdatePlaneColor( m_planes[1] );
        UpdatePlaneColor( m_planes[2] );

        // Manipulators
        //-------------------------------------------------------------------------

        for ( auto d : drawOrder )
        {
            // Axes
            if ( d < 3 )
            {
                bool shouldDrawManipulationAxis = false;
                if ( IsManipulating() )
                {
                    if ( d == 0 )
                    {
                        shouldDrawManipulationAxis =
                        (
                            m_activeManipulator == Manipulator::TranslateX ||
                            m_activeManipulator == Manipulator::TranslateXY ||
                            m_activeManipulator == Manipulator::TranslateXZ
                        );
                    }
                    else if ( d == 1 )
                    {
                        shouldDrawManipulationAxis =
                        (
                            m_activeManipulator == Manipulator::TranslateY ||
                            m_activeManipulator == Manipulator::TranslateYZ ||
                            m_activeManipulator == Manipulator::TranslateXY
                        );
                    }
                    else if ( d == 2 )
                    {
                        shouldDrawManipulationAxis =
                        (
                            m_activeManipulator == Manipulator::TranslateZ ||
                            m_activeManipulator == Manipulator::TranslateXZ ||
                            m_activeManipulator == Manipulator::TranslateYZ
                        );
                    }
                }

                if ( shouldDrawManipulationAxis )
                {
                    pDrawList->AddLine( m_axes[d].m_axisStartInfSS, m_axes[d].m_axisEndInfSS, m_axes[d].m_color, m_style.m_axisThickness / 2.0f );
                }

                if ( m_axes[d].m_isFlipped )
                {
                    ImGuiX::DrawDashedLine( pDrawList, m_axes[d].m_axisStartSS, m_axes[d].m_axisEndSS, m_axes[d].m_color, m_style.m_axisThickness );
                }
                else
                {
                    pDrawList->AddLine( m_axes[d].m_axisStartSS, m_axes[d].m_axisEndSS, m_axes[d].m_color, m_style.m_axisThickness );
                }

                // Draw axis caps
                Vector const arrowBaseDirSS = m_axes[d].m_axisDirSS.Orthogonal2D();

                float const arrowThickness = m_style.m_axisThickness * 2;
                Vector T0 = Vector::MultiplyAdd( m_axes[d].m_axisDirSS, Vector( m_style.m_axisArrowThickness * 3 ), m_axes[d].m_axisEndSS );
                Vector T1 = m_axes[d].m_axisEndSS - ( arrowBaseDirSS * m_style.m_axisArrowThickness );
                Vector T2 = m_axes[d].m_axisEndSS + ( arrowBaseDirSS * m_style.m_axisArrowThickness );

                pDrawList->AddTriangleFilled( T0, T1, T2, m_axes[d].m_color );
            }
            else // Plane Manipulator
            {
                int32_t const p = d - 3;
                if ( !m_planes[p].m_isHidden )
                {
                    pDrawList->AddConvexPolyFilled( m_planes[p].m_pointsSS, 5, m_planes[p].m_color.GetAlphaVersion( Style::s_planeManipulatorAlpha ) );
                    pDrawList->AddPolyline( m_planes[p].m_pointsSS, 5, m_planes[p].m_color, 0, m_style.m_axisPlaneManipulatorBorderThickness );
                }
            }
        }

        // Draw origin sphere
        //-------------------------------------------------------------------------

        Color originColor = Style::s_defaultColor;

        switch ( m_activeManipulator )
        {
            break;

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

        pDrawList->AddCircleFilled( ctx.m_positionSS.ToFloat2(), m_style.m_originCircleRadius, originColor );
    }

    //-------------------------------------------------------------------------

    bool TranslationGizmo::TryGetProjectedManipulationPoint( Context const& ctx, AxisManipulator const& axis, Vector const& axisOriginPosWS, Vector const& pointSS, Vector& projectManipulationPoint )
    {
        LineSegment const projectedPointRay = ctx.m_viewport.ScreenSpaceToWorldSpace( pointSS );
        Math::RayPlaneResult result = Math::IntersectLinePlane( projectedPointRay.GetStartPoint(), projectedPointRay.GetEndPoint(), m_translationPlane );
        if( result )
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
            LineSegment const mouseWorldRay = ctx.m_viewport.ScreenSpaceToWorldSpace( ctx.m_mousePositionSS );

            // Check Planes
            //-------------------------------------------------------------------------

            for ( int32_t i = 0; i < 3; i++ )
            {
                if ( m_planes[i].m_isHovered )
                {
                    m_translationPlane = Plane::FromNormalAndPoint( m_planes[i].m_manipulationPlaneAxis, ctx.m_positionWS );
                    m_activeManipulator = m_planes[i].m_manipulator;
                    Math::RayPlaneResult result = Math::IntersectLinePlane( mouseWorldRay.GetStartPoint(), mouseWorldRay.GetEndPoint(), m_translationPlane );
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
                    m_activeManipulator = m_axes[i].m_manipulator;

                    if ( TryGetProjectedManipulationPoint( ctx, m_axes[i], ctx.m_positionWS, ctx.m_mousePositionSS, m_previousPosWS ) )
                    {
                        m_manipulationStartPosWS = ctx.m_positionWS;
                        m_previousPosWS = m_previousPosWS;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        }

        return false;
    }

    void TranslationGizmo::Manipulate( Context const& ctx )
    {
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();

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
                    LineSegment const mouseWorldRay = ctx.m_viewport.ScreenSpaceToWorldSpace( ctx.m_mousePositionSS );
                    auto result = Math::IntersectLinePlane( mouseWorldRay.GetStartPoint(), mouseWorldRay.GetEndPoint(), m_translationPlane );
                    projectedPointWS = result ? result.m_intersectionPoint : m_previousPosWS;
                }

                if ( m_debugEnabled )
                {
                    EE_TRACE_MSG( "Point WS: %f, %f, %f", projectedPointWS.GetX(), projectedPointWS.GetY(), projectedPointWS.GetZ() );

                    ImVec2 lineProjectedPointSS = ctx.m_viewport.WorldSpaceToScreenSpace( projectedPointWS );
                    pDrawList->AddCircleFilled( lineProjectedPointSS, 3, Colors::Yellow );
                    pDrawList->AddLine( ctx.m_positionSS, lineProjectedPointSS, Colors::Yellow, 1.0f );
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

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        InlineString str;

        switch ( m_activeManipulator )
        {
            case Manipulator::None:
            str.sprintf( "%s, D: %.2f, %.2f, %.2f", "Not Manipulating", m_deltaWS.GetX(), m_deltaWS.GetY(), m_deltaWS.GetZ() );
            break;
           
            case Manipulator::TranslateX:
            str.sprintf( "%s, D: %.2f, %.2f, %.2f", "Translate X", m_deltaWS.GetX(), m_deltaWS.GetY(), m_deltaWS.GetZ() );
            break;

            case Manipulator::TranslateY:
            str.sprintf( "%s, D: %.2f, %.2f, %.2f", "Translate Y", m_deltaWS.GetX(), m_deltaWS.GetY(), m_deltaWS.GetZ() );
            break;

            case Manipulator::TranslateZ:
            str.sprintf( "%s, D: %.2f, %.2f, %.2f", "Translate Z", m_deltaWS.GetX(), m_deltaWS.GetY(), m_deltaWS.GetZ() );
            break;

            case Manipulator::TranslateXY:
            str.sprintf( "%s, D: %.2f, %.2f, %.2f", "Translate XY", m_deltaWS.GetX(), m_deltaWS.GetY(), m_deltaWS.GetZ() );
            break;

            case Manipulator::TranslateYZ:
            str.sprintf( "%s, D: %.2f, %.2f, %.2f", "Translate YZ", m_deltaWS.GetX(), m_deltaWS.GetY(), m_deltaWS.GetZ() );
            break;

            case Manipulator::TranslateXZ:
            str.sprintf( "%s, D: %.2f, %.2f, %.2f", "Translate XZ", m_deltaWS.GetX(), m_deltaWS.GetY(), m_deltaWS.GetZ() );
            break;
        }

        ImVec2 const textPos = ctx.m_positionSS + Vector( 4, 4, 0 );
        pDrawList->AddText( textPos, Colors::HotPink, str.c_str() );
    }
}
#endif