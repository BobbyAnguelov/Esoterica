#include "Gizmo_Rotate.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Imgui/ImguiStyle.h"
#include "Base/Math/MathUtils.h"
#include "Base/Math/Intersection.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------

namespace EE
{
    void RotationGizmo::Style::SetScale( float scale )
    {
        GizmoBase::Style::SetScale( scale );

        m_axisManipulatorDeadZone *= scale;
        m_trackballDeadZone *= scale;
    }

    //-------------------------------------------------------------------------

    float RotationGizmo::GenerateArcPoints( Context const& ctx, Vector const& rotationAxisWS, Vector const& startOffsetWS, float arcAngleRadians, PointList& outPointsWS ) const
    {
        outPointsWS.clear();

        float maxDistanceFromOriginSS = 0.0f;

        int32_t const numSegments = Math::CeilingToInt32( arcAngleRadians / Style::s_angleRadiansBetweenPoints );
        if ( numSegments <= 0 )
        {
            return maxDistanceFromOriginSS;
        }

        float const deltaAngleRadians = arcAngleRadians / numSegments;
        for ( int32_t p = 0; p <= numSegments; p++ )
        {
            Quaternion const rot( rotationAxisWS, Radians( deltaAngleRadians * p ) );
            Vector const pointWS = ctx.m_positionWS + rot.RotateVector( startOffsetWS );
            Vector const pointSS = ctx.WorldSpaceToScreenSpace( pointWS );
            outPointsWS.emplace_back( pointWS );
            maxDistanceFromOriginSS = Math::Max( maxDistanceFromOriginSS, ctx.m_positionSS.GetDistance2( pointSS ) );
        }

        return maxDistanceFromOriginSS;
    }

    void RotationGizmo::SetupManipulators( Context const& ctx ) 
    {
        float const axesScale = ctx.PixelsToWorld( m_style.m_axisLength );

        Quaternion const manipulatorOrientationWS = IsManipulating() ? m_manipulationStartRotationWS : ctx.m_rotationWS;

        // Calculate axes
        //-------------------------------------------------------------------------

        Vector const unitAxes[3] = { Vector::UnitX, Vector::UnitY, Vector::UnitZ };

        for ( int32_t i = 0; i < 3; i++ )
        {
            m_axes[i].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? unitAxes[i] : manipulatorOrientationWS.RotateVector( unitAxes[i] );
            m_axes[i].m_axisDirWS.Normalize3();
            m_axes[i].m_color = ImGuiX::Style::s_axisColors[i];
            m_axes[i].m_manipulator = (Manipulator) ( i + 1 );
            m_axes[i].m_isHovered = ( m_activeManipulator == m_axes[i].m_manipulator );
        }

        m_axes[0].m_vectorInRotationPlane = m_axes[2].m_axisDirWS;
        m_axes[0].m_hitTestID = CalculateUniqueHitTestID( s_hitTestID_axisX );

        m_axes[1].m_vectorInRotationPlane = m_axes[2].m_axisDirWS;
        m_axes[1].m_hitTestID = CalculateUniqueHitTestID( s_hitTestID_axisY );

        m_axes[2].m_vectorInRotationPlane = m_axes[0].m_axisDirWS;
        m_axes[2].m_hitTestID = CalculateUniqueHitTestID( s_hitTestID_axisZ );

        m_screenManipulator.m_hitTestID = CalculateUniqueHitTestID( s_hitTestID_screen );
        m_trackballManipulator.m_hitTestID = CalculateUniqueHitTestID( s_hitTestID_trackball );

        // Generate points
        //-------------------------------------------------------------------------

        float maxDistanceFromOriginSS = 0.0f;

        for ( int32_t i = 0; i < 3; i++ )
        {
            Vector startVector = Vector::Zero;
            float angularDistanceToDraw = 0.0f;

            // Check if the rotation axis is nearly aligned to the view direction, if so we can see the whole circle
            Radians const angleBetween = Math::CalculateAngleBetweenUnitVectors( ctx.m_viewDirectionWS, m_axes[i].m_axisDirWS );
            if ( Math::IsNearZero( angleBetween.ToFloat(), Style::s_minAngleRadiansBetweenViewAndAxis ) || Math::IsNearEqual( angleBetween.ToFloat(), Math::Pi, Style::s_minAngleRadiansBetweenViewAndAxis ) )
            {
                startVector = m_axes[i].m_vectorInRotationPlane;
                angularDistanceToDraw = Math::TwoPi;
            }
            else // Only draw the camera facing half of the circle
            {
                Plane const viewPlane = Plane::FromNormalAndPoint( -ctx.m_viewDirectionWS, ctx.m_positionWS );
                Plane const axisPlane = Plane::FromNormalAndPoint( m_axes[i].m_axisDirWS, ctx.m_positionWS );

                Math::PlanePlaneResult const result = Math::IntersectPlanePlane( viewPlane, axisPlane );
                if ( result )
                {
                    startVector = ( result.m_intersectionLineStart - result.m_intersectionLineEnd ).GetNormalized3();
                    angularDistanceToDraw = Math::Pi;
                }
            }

            // Draw full circle if manipulating
            if ( m_activeManipulator == m_axes[i].m_manipulator && angularDistanceToDraw > 0.0f )
            {
                angularDistanceToDraw = Math::TwoPi;
            }

            float const maxDistanceSS = GenerateArcPoints( ctx, m_axes[i].m_axisDirWS, startVector * axesScale, angularDistanceToDraw, m_axes[i].m_pointsWS );
            maxDistanceFromOriginSS = Math::Max( maxDistanceFromOriginSS, maxDistanceSS );
        }

        // Screen/Trackball rotation
        //-------------------------------------------------------------------------
        // These are circles that face the camera

        m_screenManipulator.m_color = Style::s_defaultColor;
        m_screenManipulator.m_isHovered = ( m_activeManipulator == Manipulator::RotateScreen );

        m_trackballManipulator.m_color = Style::s_trackballColor;
        m_trackballManipulator.m_isHovered = ( m_activeManipulator == Manipulator::RotateTrackball );

        Vector const drawRotationAxis = ( ctx.m_viewPositionWS - ctx.m_positionWS ).GetNormalized3();
        Vector const startVector = drawRotationAxis.Cross3( ctx.m_viewport.GetViewRightDirection() ).GetNormalized3();

        float const trackballMaxDistanceSS = GenerateArcPoints( ctx, drawRotationAxis, startVector * ( axesScale * m_style.m_trackBallSizeMultiplier ), Math::TwoPi, m_trackballManipulator.m_pointsWS );
        float const screenMaxDistanceSS = GenerateArcPoints( ctx, drawRotationAxis, startVector * ( axesScale * m_style.m_screenManipulatorSizeMultiplier ), Math::TwoPi, m_screenManipulator.m_pointsWS );

        m_trackballManipulator.m_manipulationRadius = Math::Max( maxDistanceFromOriginSS, Math::Max( trackballMaxDistanceSS, screenMaxDistanceSS ) );
    }

    void RotationGizmo::UpdateHoverState( Context const& ctx )
    {
        auto const& pickingData = ctx.m_viewport.GetPickingData();

        m_isAnyManipulatorHovered = false;

        if ( ctx.m_isMouseInViewport && !pickingData.empty() )
        {
            for ( AxisManipulator& axis : m_axes )
            {
                if ( axis.m_hitTestID == pickingData[0].m_primaryID )
                {
                    axis.m_isHovered = true;
                    m_isAnyManipulatorHovered = true;
                }
            }

            // Screen Manipulator
            //-------------------------------------------------------------------------

            if ( !m_isAnyManipulatorHovered && m_screenManipulator.m_hitTestID == pickingData[0].m_primaryID )
            {
                m_screenManipulator.m_isHovered = true;
                m_isAnyManipulatorHovered = true;
            }

            // Trackball Manipulator
            //-------------------------------------------------------------------------

            if ( !m_isAnyManipulatorHovered && m_trackballManipulator.m_hitTestID == pickingData[0].m_primaryID )
            {
                m_trackballManipulator.m_isHovered = true;
                m_isAnyManipulatorHovered = true;
            }
        }
    }

    void RotationGizmo::DrawManipulators( Context const& ctx )
    {
        auto drawingCtx = ctx.m_viewport.GetDebugDrawContext();

        // Update colors
        //-------------------------------------------------------------------------

        for ( AxisManipulator& axis : m_axes )
        {
            if ( IsManipulating() )
            {
                if ( m_activeManipulator != axis.m_manipulator )
                {
                    axis.m_color = axis.m_color.GetScaledColor( ImGuiX::Style::s_dimColorScale );
                }
            }
            else // Not manipulating
            {
                if ( axis.m_isHovered )
                {
                    axis.m_color = axis.m_color.GetScaledColor( ImGuiX::Style::s_brightColorScale );
                }
            }
        }

        if ( !IsManipulating() )
        {
            if ( m_screenManipulator.m_isHovered )
            {
                m_screenManipulator.m_color = m_screenManipulator.m_color.GetScaledColor( ImGuiX::Style::s_brightColorScale );
            }

            if ( m_trackballManipulator.m_isHovered )
            {
                m_trackballManipulator.m_color = Style::s_trackballHoverColor;
            }
        }

        // Trackball Rotation
        //-------------------------------------------------------------------------

        if ( m_activeManipulator == Manipulator::None || m_activeManipulator == Manipulator::RotateTrackball )
        {
            drawingCtx.SetHitTestID( m_trackballManipulator.m_hitTestID );

            PointList const& pointsWS = m_trackballManipulator.m_pointsWS;
            for ( int32_t i = 1; i < (int32_t) pointsWS.size(); i++ )
            {
                DrawDoubleSidedTriangle( drawingCtx, ctx.m_positionWS, pointsWS[i - 1], pointsWS[i], m_trackballManipulator.m_color );
            }

            if ( IsManipulating() )
            {
                DrawTrackballRotationManipulationVisualization( ctx );
            }

            drawingCtx.ClearHitTestID();
        }

        // Axis Rotation
        //-------------------------------------------------------------------------

        for ( AxisManipulator const& axis : m_axes )
        {
            // Only draw the currently active manipulator
            if ( IsManipulating() && axis.m_manipulator != m_activeManipulator )
            {
                continue;
            }

            drawingCtx.SetHitTestID( axis.m_hitTestID );

            DrawPolyLine( drawingCtx, axis.m_pointsWS, axis.m_color, m_style.m_lineThickness );

            if ( IsManipulating() )
            {
                DrawAxisRotationManipulationVisualization( ctx, axis );
            }

            drawingCtx.ClearHitTestID();
        }

        // Screen Rotation
        //-------------------------------------------------------------------------

        if ( m_activeManipulator == Manipulator::None || m_activeManipulator == Manipulator::RotateScreen )
        {
            drawingCtx.SetHitTestID( m_screenManipulator.m_hitTestID );

            DrawPolyLine( drawingCtx, m_screenManipulator.m_pointsWS, m_screenManipulator.m_color, m_style.m_lineThickness );

            if ( IsManipulating() )
            {
                DrawScreenRotationManipulationVisualization( ctx );
            }

            drawingCtx.ClearHitTestID();
        }

        // Draw origin sphere
        //-------------------------------------------------------------------------

        Color originColor = Style::s_defaultColor;

        switch ( m_activeManipulator )
        {
            case Manipulator::RotateX:
            case Manipulator::RotateY:
            case Manipulator::RotateZ:
            {
                originColor = ImGuiX::Style::s_axisColors[(int32_t) m_activeManipulator - 1];
            }
            break;

            case Manipulator::RotateTrackball:
            case Manipulator::RotateScreen:
            {
                originColor = m_screenManipulator.m_color;
            }
            break;

            default:
            break;
        }

        DrawOrigin( ctx, originColor );
    }

    bool RotationGizmo::TryStartManipulating( Context const& ctx )
    {
        if ( ctx.m_isMouseInViewport && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
        {
            auto StartManipulating = [this, &ctx] ( Manipulator manipulator )
            {
                m_activeManipulator = manipulator;
                m_manipulationStartMousePositionSS = ctx.m_mousePositionSS;
                m_manipulationStartRotationWS = ctx.m_rotationWS;
                m_previousRotationAngle = 0.0f;
                m_previousRotation = ctx.m_rotationWS;
            };

            // Check Axes
            //-------------------------------------------------------------------------

            for ( int32_t i = 0; i < 3; i++ )
            {
                if ( m_axes[i].m_isHovered )
                {
                    m_rotationAxis = m_axes[i].m_axisDirWS;
                    m_rotationPlane = Plane::FromNormalAndPoint( m_rotationAxis, ctx.m_positionWS );
                    StartManipulating( m_axes[i].m_manipulator );

                    m_projectedStartMousePosWS = ProjectScreenSpacePositionOntoRotationPlane( ctx, m_manipulationStartMousePositionSS );
                    return true;
                }
            }

            // Check Screen Manipulator
            //-------------------------------------------------------------------------

            if ( m_screenManipulator.m_isHovered )
            {
                m_rotationAxis = ( ctx.m_positionWS - ctx.m_viewPositionWS ).GetNormalized3();
                m_rotationPlane = Plane::FromNormalAndPoint( m_rotationAxis, ctx.m_positionWS );
                StartManipulating( Manipulator::RotateScreen );
                return true;
            }

            // Check Trackball Manipulator
            //-------------------------------------------------------------------------

            if ( m_trackballManipulator.m_isHovered )
            {
                m_rotationPlane = Plane::FromNormalAndPoint( -ctx.m_viewDirectionWS, ctx.m_positionWS );
                StartManipulating( Manipulator::RotateTrackball );
                return true;
            }
        }

        return false;
    }

    void RotationGizmo::Manipulate( Context const& ctx )
    {
        m_delta = Quaternion::Identity;

        if ( ctx.m_isMouseInViewport )
        {
            // Set the hovered ID so that we disable any click through in the UI
            ImGui::SetHoveredID( ImGui::GetID( "Gizmo" ) );

            if ( m_activeManipulator == Manipulator::RotateScreen )
            {
                Vector manipulationStartPointDir;
                float distanceToManipulationStartPoint;
                ( m_manipulationStartMousePositionSS - ctx.m_positionSS ).ToDirectionAndLength2( manipulationStartPointDir, distanceToManipulationStartPoint );

                Vector currentMousePosDir;
                float distanceToMousePos;
                ( ctx.m_mousePositionSS - ctx.m_positionSS ).ToDirectionAndLength2( currentMousePosDir, distanceToMousePos );

                Radians rotationAngle = Math::CalculateAngleBetweenUnitVectors( manipulationStartPointDir, currentMousePosDir );

                // Check if this is a positive rotation
                float const dot = currentMousePosDir.GetDot2( manipulationStartPointDir.Orthogonal2D() );
                if ( dot < 0 )
                {
                    rotationAngle = -rotationAngle;
                }

                Radians const deltaAngle = rotationAngle - m_previousRotationAngle;
                m_previousRotationAngle = rotationAngle;
                m_delta = Quaternion( m_rotationAxis, deltaAngle );
                EE_ASSERT( m_delta.IsNormalized() );
            }
            else if ( m_activeManipulator == Manipulator::RotateTrackball )
            {
                float const deltaDistanceSS = ( ctx.m_mousePositionSS - m_manipulationStartMousePositionSS ).GetLength2();
                if ( deltaDistanceSS > m_style.m_trackballDeadZone )
                {
                    Vector const projectedStartMousePosWS = ProjectScreenSpacePositionOntoRotationPlane( ctx, m_manipulationStartMousePositionSS );
                    Vector const projectedMousePosWS = ProjectScreenSpacePositionOntoRotationPlane( ctx, ctx.m_mousePositionSS );
                    Vector const deltaMousePosDirWS = ( projectedMousePosWS - projectedStartMousePosWS ).GetNormalized3();

                    // Convert SS distance to angle, use the widget radius to imagine a real ball and convert the pixel distance to an angular distance on the ball
                    Radians const deltaAngle = deltaDistanceSS / m_trackballManipulator.m_manipulationRadius;

                    // Calculate axis of rotation and delta
                    m_rotationAxis = m_rotationPlane.GetNormal().Cross3( deltaMousePosDirWS ).GetNormalized3();
                    Quaternion const newRot = Quaternion( m_rotationAxis, deltaAngle ) * m_manipulationStartRotationWS;
                    m_delta = Quaternion::Delta( m_previousRotation, newRot );
                    m_previousRotation = newRot;
                    EE_ASSERT( m_delta.IsNormalized() );
                }
            }
            else // Axis rotation
            {
                float const mousePositionDeltaSS = ( ctx.m_mousePositionSS - m_manipulationStartMousePositionSS ).GetLength2();
                if ( mousePositionDeltaSS > m_style.m_axisManipulatorDeadZone )
                {
                    Vector const projectedMousePosWS = ProjectScreenSpacePositionOntoRotationPlane( ctx, ctx.m_mousePositionSS );

                    Vector const manipulationStart = ( m_projectedStartMousePosWS - ctx.m_positionWS ).GetNormalized3();
                    Vector const manipulationCurrent = ( projectedMousePosWS - ctx.m_positionWS ).GetNormalized3();
                    Radians rotationAngle = Math::CalculateAngleBetweenUnitVectors( manipulationStart, manipulationCurrent );

                    // Check if this is a positive rotation
                    Vector const manipulationNormal = Quaternion( m_rotationAxis, Math::PiDivTwo ).RotateVector( manipulationStart );
                    float const dot = manipulationNormal.GetDot3( manipulationCurrent );
                    if ( dot < 0 )
                    {
                        rotationAngle = -rotationAngle;
                    }

                    Radians const deltaAngle = rotationAngle - m_previousRotationAngle;
                    m_previousRotationAngle = rotationAngle;
                    m_delta = Quaternion( m_rotationAxis, deltaAngle );
                    EE_ASSERT( m_delta.IsNormalized() );
                }
            }
        }
    }

    void RotationGizmo::StopManipulating( Context const& ctx )
    {
        m_delta = Quaternion::Identity;
        m_rotationAxis = Vector::Zero;
        m_manipulationStartMousePositionSS = Vector::Zero;
        m_previousRotationAngle = 0.0f;
        m_manipulationStartRotationWS = m_previousRotation = Quaternion::Identity;
        m_activeManipulator = Manipulator::None;
        m_isAnyManipulatorHovered = false;
    }

    void RotationGizmo::DrawDebug( Context const& ctx )
    {
        if ( !m_debugEnabled )
        {
            return;
        }

        static char const* const manipulatorNames[] = { "Not Manipulating", "Rotate X", "Rotate Y", "Rotate Z", "Rotate Trackball", "Rotate Screen" };

        InlineString str;
        if ( IsManipulating() )
        {
            str.sprintf( "%s, Angle: %.2f deg", manipulatorNames[(int32_t) m_activeManipulator], m_previousRotationAngle.ToDegrees().ToFloat() );
        }
        else
        {
            str = manipulatorNames[0];
        }

        auto drawingCtx = ctx.m_viewport.GetDebugDrawContext();
        drawingCtx.DrawText3D( ctx.m_positionWS, str.c_str(), Colors::HotPink );
    }

    Vector RotationGizmo::ProjectScreenSpacePositionOntoRotationPlane( Context const& ctx, Vector const &positionSS ) const
    {
        EE_ASSERT( IsManipulating() );

        LineSegment const worldRay = ctx.m_viewport.ScreenSpaceToWorldSpace( positionSS.ToFloat2() );
        auto const result = Math::IntersectLinePlane( worldRay.GetStartPoint(), worldRay.GetEndPoint(), m_rotationPlane );
        EE_ASSERT( result.m_intersects );
        return result.m_intersectionPoint;
    }

    //-------------------------------------------------------------------------

    void RotationGizmo::DrawFilledArc( Context const& ctx, Vector const& rotationAxisWS, Vector const& startOffsetWS, float arcAngleRadians, Color color )
    {
        int32_t const numSegments = Math::CeilingToInt32( arcAngleRadians / Style::s_angleRadiansBetweenPoints );
        if ( numSegments <= 0 )
        {
            return;
        }

        auto drawingCtx = ctx.m_viewport.GetDebugDrawContext();

        float const stepAngleRadians = arcAngleRadians / numSegments;
        Vector previousPointWS = ctx.m_positionWS + startOffsetWS;
        for ( int32_t i = 1; i <= numSegments; i++ )
        {
            Quaternion const rot( rotationAxisWS, Radians( stepAngleRadians * i ) );
            Vector const pointWS = ctx.m_positionWS + rot.RotateVector( startOffsetWS );
            DrawDoubleSidedTriangle( drawingCtx, ctx.m_positionWS, previousPointWS, pointWS, color );
            previousPointWS = pointWS;
        }
    }

    void RotationGizmo::DrawAxisRotationManipulationVisualization( Context const& ctx, AxisManipulator const& axis )
    {
        if ( !ctx.m_isMouseInViewport )
        {
            return;
        }

        DrawInfiniteLine( ctx, axis.m_axisDirWS, axis.m_color, 1.0f );

        //-------------------------------------------------------------------------

        LineSegment const manipulationStartLine = ctx.m_viewport.ScreenSpaceToWorldSpace( m_manipulationStartMousePositionSS.ToFloat2() );
        LineSegment const manipulationEndLine = ctx.m_viewport.ScreenSpaceToWorldSpace( ctx.m_mousePositionSS.ToFloat2() );

        Math::RayPlaneResult const r0 = Math::IntersectLinePlane( manipulationStartLine, m_rotationPlane );
        Math::RayPlaneResult const r1 = Math::IntersectLinePlane( manipulationEndLine, m_rotationPlane );

        if ( !r0 || !r1 )
        {
            return;
        }

        Vector const startDir = ( r0.m_intersectionPoint - ctx.m_positionWS ).GetNormalized3();
        Vector const endDir = ( r1.m_intersectionPoint - ctx.m_positionWS ).GetNormalized3();
        Radians const deltaAngle = Math::CalculateAngleBetweenUnitVectors( startDir, endDir );

        // Check if this is a positive rotation
        Vector rotationAxis = axis.m_axisDirWS;
        Vector const manipulationNormal = Quaternion( m_rotationAxis, Math::PiDivTwo ).RotateVector( startDir );
        if ( manipulationNormal.GetDot3( endDir ) < 0 )
        {
            rotationAxis.Negate();
        }

        float const axesScale = ctx.PixelsToWorld( m_style.m_axisLength );
        DrawFilledArc( ctx, rotationAxis, startDir * axesScale, deltaAngle.ToFloat(), axis.m_color.GetAlphaVersion( Style::s_manipulationVisualizationAlpha ) );
    }

    void RotationGizmo::DrawScreenRotationManipulationVisualization( Context const& ctx )
    {
        if ( !ctx.m_isMouseInViewport )
        {
            return;
        }

        Vector const vRef = ( m_manipulationStartMousePositionSS - ctx.m_positionSS ).GetNormalized2();
        Vector const vEnd = ( ctx.m_mousePositionSS - ctx.m_positionSS ).GetNormalized2();

        Radians const deltaAngle = Math::CalculateAngleBetweenVectors( vRef, vEnd );
        Vector const rotationAxis = Math::IsVectorToTheLeft( vRef, vEnd ) ? -m_rotationAxis : m_rotationAxis;

        Vector const vRefWS = ( ProjectScreenSpacePositionOntoRotationPlane( ctx, m_manipulationStartMousePositionSS ) - ctx.m_positionWS ).GetNormalized3();
        float const axesScale = ctx.PixelsToWorld( m_style.m_axisLength );
        Vector const startVector = vRefWS * ( m_style.m_trackBallSizeMultiplier * axesScale );

        auto drawingCtx = ctx.m_viewport.GetDebugDrawContext();
        drawingCtx.DrawLine( ctx.m_positionWS, ctx.m_positionWS + startVector, m_screenManipulator.m_color, 1.0f );

        DrawFilledArc( ctx, rotationAxis, startVector, deltaAngle.ToFloat(), m_screenManipulator.m_color.GetAlphaVersion( Style::s_manipulationVisualizationAlpha ) );
    }

    void RotationGizmo::DrawTrackballRotationManipulationVisualization( Context const& ctx )
    {
        if ( !ctx.m_isMouseInViewport )
        {
            return;
        }

        auto drawingCtx = ctx.m_viewport.GetDebugDrawContext();
        DrawPolyLine( drawingCtx, m_trackballManipulator.m_pointsWS, Style::s_defaultColor, m_style.m_lineThickness );
    }
}
