#include "Gizmo_Scale.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Math/MathUtils.h"
#include "Base/Math/Line.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------

namespace EE
{
    void ScaleGizmo::Style::SetScale( float scale )
    {
        GizmoBase::Style::SetScale( scale );

        m_axisEndCapRadius *= scale;
        m_axisOffsetFromOrigin *= scale;
    }

    //-------------------------------------------------------------------------

    void ScaleGizmo::SetupManipulators( Context const& ctx )
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
            axis.m_axisStartInfSS = ctx.m_positionSS - ( axis.m_axisDirSS * 100000 );
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
    }

    void ScaleGizmo::UpdateHoverState( Context const& ctx )
    {
        auto const& pickingData = ctx.m_viewport.GetPickingData();

        if ( ctx.m_isMouseInViewport && !pickingData.empty() )
        {
            for ( AxisManipulator& axis : m_axes )
            {
                if ( axis.m_hitTestID == pickingData[0].m_primaryID )
                {
                    axis.m_isHovered = true;
                }
            }
        }

        m_isAnyManipulatorHovered = m_axes[0].m_isHovered || m_axes[1].m_isHovered || m_axes[2].m_isHovered;
    }

    void ScaleGizmo::DrawManipulators( Context const& ctx )
    {
        auto drawingCtx = ctx.m_viewport.GetDebugDrawContext();

        float const endCapRadiusWS = ctx.PixelsToWorld( m_style.m_axisEndCapRadius );

        for ( AxisManipulator& axis : m_axes )
        {
            // Update color
            bool const isHighlighted = IsManipulating() ? ( m_activeManipulator == axis.m_manipulator ) : axis.m_isHovered;
            if ( !isHighlighted )
            {
                axis.m_color = axis.m_color.GetScaledColor( Style::s_dimColorScale );
            }

            drawingCtx.SetHitTestID( axis.m_hitTestID );

            // Draw the full manipulation axis
            if ( m_activeManipulator == axis.m_manipulator )
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
            Vector const endCapCenterWS = axis.m_axisEndWS + ( axis.m_axisDirWS * ( endCapRadiusWS / 2 ) );
            drawingCtx.DrawSphere( endCapCenterWS.ToFloat3(), endCapRadiusWS, axis.m_color );

            drawingCtx.ClearHitTestID();
        }

        // Draw origin sphere
        //-------------------------------------------------------------------------

        Color originColor = Style::s_defaultColor;

        if ( m_activeManipulator != Manipulator::None )
        {
            originColor = Style::s_axisColors[(int32_t) m_activeManipulator - 1];
        }

        DrawOrigin( ctx, originColor );
    }

    bool ScaleGizmo::TryStartManipulating( Context const& ctx )
    {
        m_previousScaleFactor = FLT_MAX;

        if ( ctx.m_isMouseInViewport && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
        {
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

            if ( !ctx.m_mouseDeltaSS.IsNearZero2() )
            {
                // Calculate Scaling Factor
                //-------------------------------------------------------------------------

                uint32_t const axisIndex = uint8_t( m_activeManipulator ) - uint8_t( Manipulator::ScaleX );
                Line const manipulationAxisSS = Line( Line::CtorStartEnd(), m_axes[axisIndex].m_axisStartInfSS, m_axes[axisIndex].m_axisEndInfSS );

                Vector directionToPointSS;
                float distanceFromOrigin = 0.0f;
                Vector const projectedPointSS = manipulationAxisSS.GetClosestPoint( ctx.m_mousePositionSS );
                ( projectedPointSS - ctx.m_positionSS ).ToDirectionAndLength2( directionToPointSS, distanceFromOrigin );

                float const referenceLength = ( m_axes[axisIndex].m_axisEndSS - ctx.m_positionSS ).GetLength2();
                float scaleFactor = distanceFromOrigin / referenceLength;

                if ( m_debugEnabled )
                {
                    Vector const projectedPointWS = ScreenSpaceToWorldSpaceAtGizmoDepth( ctx, projectedPointSS );

                    auto drawingCtx = ctx.m_viewport.GetDebugDrawContext();
                    drawingCtx.DrawPoint( projectedPointWS, Colors::HotPink, 6.0f );
                    drawingCtx.DrawLine( ctx.m_positionWS, projectedPointWS, Colors::HotPink, 1.0f );
                    drawingCtx.DrawText3D( projectedPointWS, "Projected Point", Colors::HotPink );
                }

                // Did we cross the separating plane, if so negate the scale
                float const dotManipulationAxis = manipulationAxisSS.GetDirection().GetDot2( directionToPointSS );
                if ( dotManipulationAxis < 0 )
                {
                    scaleFactor = -scaleFactor;
                }

                // Calculate Delta
                //-------------------------------------------------------------------------

                float scaleDelta = 0.0f;
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

        static char const* const manipulatorNames[] = { "Not Manipulating", "Scale X", "Scale Y", "Scale Z" };

        InlineString str;
        str.sprintf( "%s, D: %.2f, %.2f, %.2f", manipulatorNames[(int32_t) m_activeManipulator], m_delta.GetX(), m_delta.GetY(), m_delta.GetZ() );

        auto drawingCtx = ctx.m_viewport.GetDebugDrawContext();
        drawingCtx.DrawText3D( ctx.m_positionWS, str.c_str(), Colors::HotPink );
    }
}
