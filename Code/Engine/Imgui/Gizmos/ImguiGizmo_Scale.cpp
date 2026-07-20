#include "ImguiGizmo_Scale.h"
#include "Base/Imgui/ImguiStyle.h"
#include "Base/Math/MathUtils.h"
#include "EASTL/sort.h"
#include "Base/Math/Intersection.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    void ScaleGizmo::Style::SetScale( float scale )
    {
        *this = ScaleGizmo::Style();

        m_originCircleRadius *= scale;
        m_axisLength *= scale;
        m_axisThickness *= scale;
        m_axisEndCapRadius *= scale;
        m_axisAdditionalHoverBorder *= scale;
        m_axisOffsetFromOrigin *= scale;
        m_hoverDetectionDistance *= scale;
    }

    void ScaleGizmo::SetupManipulators( Context const& ctx )
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
    }

    void ScaleGizmo::UpdateHoverState( Context const& ctx )
    {
        if ( ctx.m_isMouseInViewport )
        {
            // Individual axes take priority
            //-------------------------------------------------------------------------

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
        }

        m_isAnyManipulatorHovered = m_axes[0].m_isHovered || m_axes[1].m_isHovered || m_axes[2].m_isHovered;
    }

    void ScaleGizmo::DrawManipulators( Context const& ctx )
    {
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();

        // Sort manipulators back to front
        //-------------------------------------------------------------------------

        TInlineVector<int32_t, 6> drawOrder = { 0, 1, 2 };

        auto Comparator = [&] ( int32_t const& a, int32_t const& b )
        {
            float const va = m_axes[a].m_distanceToCamera;
            float const vb = m_axes[b].m_distanceToCamera;
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

        UpdateAxisColor( m_axes[0] );
        UpdateAxisColor( m_axes[1] );
        UpdateAxisColor( m_axes[2] );

        // Manipulators
        //-------------------------------------------------------------------------

        for ( auto d : drawOrder )
        {
            bool shouldDrawManipulationAxis = false;
            if ( IsManipulating() )
            {
                if ( d == 0 )
                {
                    shouldDrawManipulationAxis = m_activeManipulator == Manipulator::ScaleX;
                }
                else if ( d == 1 )
                {
                    shouldDrawManipulationAxis = m_activeManipulator == Manipulator::ScaleY;
                }
                else if ( d == 2 )
                {
                    shouldDrawManipulationAxis = m_activeManipulator == Manipulator::ScaleZ;
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

            Vector const C = Vector::MultiplyAdd( m_axes[d].m_axisDirSS, Vector( m_style.m_axisEndCapRadius / 2 ), m_axes[d].m_axisEndSS );
            pDrawList->AddCircleFilled( C, m_style.m_axisEndCapRadius, m_axes[d].m_color );
        }

        // Draw origin sphere
        //-------------------------------------------------------------------------

        Color originColor = Style::s_defaultColor;

        if ( m_activeManipulator != Manipulator::None )
        {
            originColor = Style::s_axisColors[ (int32_t) m_activeManipulator - 1];
        }

        pDrawList->AddCircleFilled( ctx.m_positionSS.ToFloat2(), m_style.m_originCircleRadius, originColor );
    }

    bool ScaleGizmo::TryStartManipulating( Context const& ctx )
    {
        m_previousScaleFactor = FLT_MAX;

        if ( ctx.m_isMouseInViewport && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
        {
            // Check Axes
            //-------------------------------------------------------------------------

            for ( int32_t i = 0; i < 3; i++ )
            {
                if ( m_axes[i].m_isHovered )
                {
                    m_manipulationStartMousePositionSS = ctx.m_mousePositionSS;
                    m_activeManipulator = m_axes[i].m_manipulator;
                    return true;
                }
            }
        }

        return false;
    }

    void ScaleGizmo::Manipulate( Context const& ctx )
    {
        m_delta = Vector::Zero;

        //-------------------------------------------------------------------------

        if ( ctx.m_isMouseInViewport )
        {
            // Set the hovered ID so that we disable any click through in the UI
            ImGui::SetHoveredID( ImGui::GetID( "Gizmo" ) );

            LineSegment const mouseWorldRay = ctx.m_viewport.ScreenSpaceToWorldSpace( ctx.m_mousePositionSS );

            if ( !ctx.m_mouseDeltaSS.IsNearZero2() )
            {
                // Calculate Scaling Factor
                //-------------------------------------------------------------------------

                float scaleDelta = 0.0f;
                float scaleFactor = 1.0f;
                Vector manipulationNormal;
                Vector directionToPointSS;

                uint32_t const axisIndex = uint8_t( m_activeManipulator ) - uint8_t( Manipulator::ScaleX );
                Line const manipulationAxisSS = Line( Line::CtorStartEnd(), m_axes[axisIndex].m_axisStartInfSS, m_axes[axisIndex].m_axisEndInfSS );

                float distanceFromOrigin = 0.0f;
                Vector projectedPointSS = manipulationAxisSS.GetClosestPoint( ctx.m_mousePositionSS, &distanceFromOrigin );
                ( projectedPointSS - ctx.m_positionSS ).ToDirectionAndLength2( directionToPointSS, distanceFromOrigin );

                float const referenceLength = ( m_axes[axisIndex].m_axisEndSS - ctx.m_positionSS ).GetLength2();
                scaleFactor = distanceFromOrigin / referenceLength;
                manipulationNormal = manipulationAxisSS.GetDirection();

                if ( m_debugEnabled )
                {
                    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
                    pDrawList->AddCircleFilled( projectedPointSS, 3, Colors::HotPink );
                    pDrawList->AddLine( ctx.m_positionSS, projectedPointSS, Colors::HotPink, 1.0f );
                    pDrawList->AddText( projectedPointSS, Colors::HotPink, "Projected Point" );
                }

                // Did we cross the separating plane, if so negate the scale
                float const dotManipulationAxis = manipulationNormal.GetDot2( directionToPointSS );
                if ( dotManipulationAxis < 0 )
                {
                    scaleFactor = -scaleFactor;
                }

                // Calculate Delta
                //-------------------------------------------------------------------------

                if ( m_previousScaleFactor != FLT_MAX )
                {
                    scaleDelta = scaleFactor - m_previousScaleFactor;
                }

                m_previousScaleFactor = scaleFactor;

                if ( !m_allowNonUniformScale )
                {
                    m_delta = Vector( scaleDelta );
                }
                else
                {
                    if ( m_activeManipulator == Manipulator::ScaleX )
                    {
                        m_delta = Vector( scaleDelta, 0, 0 );
                    }
                    else if ( m_activeManipulator == Manipulator::ScaleY )
                    {
                        m_delta = Vector( 0, scaleDelta, 0 );
                    }
                    else if ( m_activeManipulator == Manipulator::ScaleZ )
                    {
                        m_delta = Vector( 0, 0, scaleDelta );
                    }
                }

                if ( m_debugEnabled )
                {
                    EE_TRACE_MSG( "Previous Scale: %f", m_previousScaleFactor );
                    EE_TRACE_MSG( "Delta: %f, %f, %f\n\n", m_delta.GetX(), m_delta.GetY(), m_delta.GetZ() );
                }
            }
        }
    }

    void ScaleGizmo::StopManipulating( Context const& ctx )
    {
        m_previousScaleFactor = FLT_MAX;
        m_delta = Vector::Zero;
        m_activeManipulator = Manipulator::None;
        m_isAnyManipulatorHovered = false;
    }

    void ScaleGizmo::DrawDebug( Context const& ctx )
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
            str = "Not Manipulating";
            break;

            case Manipulator::ScaleX:
            str.sprintf( "%s, D: %.2f, %.2f, %.2f", "Scale X", m_delta.GetX(), m_delta.GetY(), m_delta.GetZ() );
            break;

            case Manipulator::ScaleY:
            str.sprintf( "%s, D: %.2f, %.2f, %.2f", "Scale Y", m_delta.GetX(), m_delta.GetY(), m_delta.GetZ() );
            break;

            case Manipulator::ScaleZ:
            str.sprintf( "%s, D: %.2f, %.2f, %.2f", "Scale Z", m_delta.GetX(), m_delta.GetY(), m_delta.GetZ() );
            break;
        }

        ImVec2 const textPos = ctx.m_positionSS + Vector( 4, 4, 0 );
        pDrawList->AddText( textPos, Colors::HotPink, str.c_str() );
    }
}
#endif