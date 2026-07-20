#include "ImguiGizmo_Rotate.h"
#include "Base/Imgui/ImguiStyle.h"
#include "Base/Math/MathUtils.h"
#include "Base/Math/Intersection.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    void RotationGizmo::Style::SetScale( float scale )
    {
        *this = RotationGizmo::Style();

        m_manipulatorRadius *= scale;
        m_manipulatorThickness *= scale;
        m_axisAdditionalHoverBorder *= scale;
        m_originCircleRadius *= scale;
        m_originCircleOffset *= scale;
        m_hoverDetectionDistance *= scale;
        m_screenManipulatorThickness *= scale;
        m_screenManipulatorOffset *= scale;
        m_screenManipulationLineOffset *= scale;
        m_trackballDeadZone *= scale;
        m_axisManipulatorDeadZone *= scale;
    }

    //-------------------------------------------------------------------------

    void RotationGizmo::SetupManipulators( Context const& ctx )
    {
        float const scaleX = PixelWidthToWorldHeight( ctx, ctx.m_positionWS, m_style.m_manipulatorRadius );
        float const scaleY = PixelHeightToWorldHeight( ctx, ctx.m_positionWS, m_style.m_manipulatorRadius );
        Vector const axesScale( Math::Min( scaleX, scaleY ) );

        Quaternion const manipulatorOrientationWS = IsManipulating() ? m_manipulationStartRotationWS : ctx.m_rotationWS;

        // Calculate initial axis screen space points
        //-------------------------------------------------------------------------

        m_axes[0].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitX : manipulatorOrientationWS.RotateVector( Vector::UnitX );
        m_axes[0].m_axisDirWS.Normalize3();
        m_axes[0].m_axisDirSS = Vector( ctx.m_viewport.WorldSpaceToScreenSpace( ctx.m_positionWS + m_axes[0].m_axisDirWS ) - ctx.m_positionSS ).GetNormalized2();
        m_axes[0].m_axisStartInfSS = ctx.m_positionSS - ( m_axes[0].m_axisDirSS * 100000 );
        m_axes[0].m_axisEndInfSS = ctx.m_positionSS + ( m_axes[0].m_axisDirSS * 100000 );
        m_axes[0].m_color = ImGuiX::Style::s_axisColorX;
        m_axes[0].m_manipulator = Manipulator::RotateX;
        m_axes[0].m_isHovered = ( m_activeManipulator == m_axes[0].m_manipulator );

        m_axes[1].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitY : manipulatorOrientationWS.RotateVector( Vector::UnitY );
        m_axes[1].m_axisDirWS.Normalize3();
        m_axes[1].m_axisDirSS = Vector( ctx.m_viewport.WorldSpaceToScreenSpace( ctx.m_positionWS + m_axes[1].m_axisDirWS ) - ctx.m_positionSS ).GetNormalized2();
        m_axes[1].m_axisStartInfSS = ctx.m_positionSS - ( m_axes[1].m_axisDirSS * 100000 );
        m_axes[1].m_axisEndInfSS = ctx.m_positionSS + ( m_axes[1].m_axisDirSS * 100000 );
        m_axes[1].m_color = ImGuiX::Style::s_axisColorY;
        m_axes[1].m_manipulator = Manipulator::RotateY;
        m_axes[1].m_isHovered = ( m_activeManipulator == m_axes[1].m_manipulator );

        m_axes[2].m_axisDirWS.Normalize3();
        m_axes[2].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitZ : manipulatorOrientationWS.RotateVector( Vector::UnitZ );
        m_axes[2].m_axisDirSS = Vector( ctx.m_viewport.WorldSpaceToScreenSpace( ctx.m_positionWS + m_axes[2].m_axisDirWS ) - ctx.m_positionSS ).GetNormalized2();
        m_axes[2].m_axisStartInfSS = ctx.m_positionSS - ( m_axes[2].m_axisDirSS * 100000 );
        m_axes[2].m_axisEndInfSS = ctx.m_positionSS + ( m_axes[2].m_axisDirSS * 100000 );
        m_axes[2].m_color = ImGuiX::Style::s_axisColorZ;
        m_axes[2].m_manipulator = Manipulator::RotateZ;
        m_axes[2].m_isHovered = ( m_activeManipulator == m_axes[2].m_manipulator );

        m_axes[0].m_vectorInRotationPlane = m_axes[2].m_axisDirWS;
        m_axes[1].m_vectorInRotationPlane = m_axes[2].m_axisDirWS;
        m_axes[2].m_vectorInRotationPlane = m_axes[0].m_axisDirWS;

        // Generate points
        //-------------------------------------------------------------------------

        float maxDistanceFromOriginSS = 0.0f;

        for ( auto i = 0; i < 3; i++ )
        {
            m_axes[i].m_pointsSS.clear();

            Vector startVector = Vector::Zero;
            Radians angularDistanceToDraw = 0;

            //-------------------------------------------------------------------------

            // Check if the rotation axis is nearly aligned to the view plane
            Radians angleBetween = Math::CalculateAngleBetweenUnitVectors( ctx.m_viewDirectionWS, m_axes[i].m_axisDirWS );
            if ( Math::IsNearZero( angleBetween.ToFloat(), Math::DegreesToRadians * Style::s_minAngleThresholdRadiansBetweenViewAndAxis ) || Math::IsNearEqual( angleBetween.ToFloat(), Math::Pi, Math::DegreesToRadians * Style::s_minAngleThresholdRadiansBetweenViewAndAxis ) )
            {
                startVector = m_axes[i].m_vectorInRotationPlane;
                angularDistanceToDraw = Math::TwoPi;
            }
            else // Calculate plane intersection
            {
                Plane const viewPlane = Plane::FromNormalAndPoint( -ctx.m_viewDirectionWS, ctx.m_positionWS );
                Plane const axisPlane = Plane::FromNormalAndPoint( m_axes[i].m_axisDirWS, ctx.m_positionWS );

                Math::PlanePlaneResult result = Math::IntersectPlanePlane( viewPlane, axisPlane );
                if ( result )
                {
                    startVector = ( result.m_intersectionLineStart - result.m_intersectionLineEnd ).GetNormalized3();
                    angularDistanceToDraw = Math::Pi;
                }
            }

            // Draw full circle if manipulating
            if ( m_activeManipulator == m_axes[i].m_manipulator )
            {
                angularDistanceToDraw = Math::TwoPi;
            }

            //-------------------------------------------------------------------------

            int32_t numPoints = Math::CeilingToInt32( angularDistanceToDraw.ToFloat() / Style::s_minAngleRadiansBetweenPoints );
            float const deltaAngleRadians = angularDistanceToDraw.ToFloat() / ( numPoints - 1 );
            for ( int32_t p = 0; p < numPoints; p++ )
            {
                Radians const angle( deltaAngleRadians * p );
                Quaternion const rot( m_axes[i].m_axisDirWS, angle );
                Vector const pointWS = rot.RotateVector( startVector * axesScale ) + ctx.m_positionWS;
                m_axes[i].m_pointsSS.emplace_back( ctx.m_viewport.WorldSpaceToScreenSpace( pointWS ) );
                maxDistanceFromOriginSS = Math::Max( maxDistanceFromOriginSS, ctx.m_positionSS.GetDistance2( m_axes[i].m_pointsSS.back() ) );
            }
        }


        // Screen rotation
        //-------------------------------------------------------------------------

        m_screenManipulator.m_color = Style::s_defaultColor;
        m_screenManipulator.m_isHovered = ( m_activeManipulator == Manipulator::RotateScreen );

        m_screenManipulator.m_pointsSS.clear();
        int32_t const numPoints = Math::CeilingToInt32( Math::TwoPi / Style::s_minAngleRadiansBetweenPoints );
        float const deltaAngleRadians = Math::TwoPi / ( numPoints - 1 );
        for ( int32_t p = 0; p < numPoints; p++ )
        {
            Radians const angle( deltaAngleRadians * p );
            Quaternion const rot( ( ctx.m_viewport.GetViewPosition() - ctx.m_positionWS ).GetNormalized3(), angle );
            Vector const pointWS = rot.RotateVector( Vector::WorldUp * axesScale * 1.1f ) + ctx.m_positionWS;
            m_screenManipulator.m_pointsSS.emplace_back( ctx.m_viewport.WorldSpaceToScreenSpace( pointWS ) );
            maxDistanceFromOriginSS = Math::Max( maxDistanceFromOriginSS, ctx.m_positionSS.GetDistance2( m_screenManipulator.m_pointsSS.back() ) );
        }

        // Trackball rotation
        //-------------------------------------------------------------------------

        m_trackballManipulator.m_color = Style::s_trackballColor;
        m_trackballManipulator.m_isHovered = ( m_activeManipulator == Manipulator::RotateTrackball );
        m_trackballManipulator.m_manipulationRadius = maxDistanceFromOriginSS;
    }

    void RotationGizmo::UpdateHoverState( Context const& ctx )
    {
        m_isAnyManipulatorHovered = false;

        if ( ctx.m_isMouseInViewport )
        {
            // Individual axes take priority
            //-------------------------------------------------------------------------

            int32_t numHoveredAxes = 0;

            auto UpdateHoverStateForAxis = [this, &ctx, &numHoveredAxes] ( AxisManipulator& manipulator )
            {
                manipulator.m_isHovered = false;

                for ( int32_t i = 1; i < (int32_t) manipulator.m_pointsSS.size(); i++ )
                {
                    LineSegment ls( manipulator.m_pointsSS[i - 1], manipulator.m_pointsSS[i] );
                    float const distance = ls.GetDistanceToPoint( ctx.m_mousePositionSS );
                    if ( Math::Abs( distance ) < m_style.m_hoverDetectionDistance )
                    {
                        manipulator.m_isHovered = true;
                        numHoveredAxes++;
                        return;
                    }
                }
            };

            UpdateHoverStateForAxis( m_axes[0] );
            UpdateHoverStateForAxis( m_axes[1] );
            UpdateHoverStateForAxis( m_axes[2] );

            // Fixed priority of axes hover
            //-------------------------------------------------------------------------

            if ( m_axes[0].m_isHovered )
            {
                m_axes[1].m_isHovered = m_axes[2].m_isHovered = false;
            }
            else if ( m_axes[1].m_isHovered )
            {
                m_axes[2].m_isHovered = false;
            }

            m_isAnyManipulatorHovered = m_axes[0].m_isHovered || m_axes[1].m_isHovered || m_axes[2].m_isHovered;

            // Screen Manipulator
            //-------------------------------------------------------------------------

            float const mouseToOriginDistance = ( ctx.m_mousePositionSS - ctx.m_positionSS ).GetLength2();
            if ( !m_isAnyManipulatorHovered )
            {
                for ( int32_t i = 1; i < (int32_t) m_screenManipulator.m_pointsSS.size(); i++ )
                {
                    LineSegment ls( m_screenManipulator.m_pointsSS[i - 1], m_screenManipulator.m_pointsSS[i] );
                    float const distance = ls.GetDistanceToPoint( ctx.m_mousePositionSS );
                    if ( Math::Abs( distance ) < m_style.m_hoverDetectionDistance )
                    {
                        m_screenManipulator.m_isHovered = true;
                        m_isAnyManipulatorHovered = true;
                        return;
                    }
                }
            }

            // Trackball Manipulator
            //-------------------------------------------------------------------------

            if ( !m_isAnyManipulatorHovered )
            {
                for ( int32_t i = 1; i < (int32_t) m_screenManipulator.m_pointsSS.size(); i++ )
                {
                    Float3 result;
                    if ( Math::CalculateBarycentricCoordinates( ctx.m_mousePositionSS, ctx.m_positionSS, m_screenManipulator.m_pointsSS[i - 1], m_screenManipulator.m_pointsSS[i], result ) )
                    {
                        m_trackballManipulator.m_isHovered = true;
                        m_isAnyManipulatorHovered = true;
                    }
                }
            }
        }
    }

    void RotationGizmo::DrawManipulators( Context const& ctx )
    {
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();

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

        auto UpdateScreenManipulatorColor = [this] ( ScreenManipulator &manipulator )
        {
            if ( !IsManipulating() )
            {
                if ( !manipulator.m_isHovered )
                {
                    manipulator.m_color = manipulator.m_color.GetScaledColor( Style::s_dimColorScale );
                }
            }
        };

        auto UpdateTrackballManipulatorColor = [this] ( TrackballManipulator &manipulator )
        {
            if ( !IsManipulating() )
            {
                if ( !manipulator.m_isHovered )
                {
                    manipulator.m_color = manipulator.m_color.GetScaledColor( Style::s_dimColorScale );
                }
            }
        };

        UpdateAxisColor( m_axes[0] );
        UpdateAxisColor( m_axes[1] );
        UpdateAxisColor( m_axes[2] );
        UpdateScreenManipulatorColor( m_screenManipulator );
        UpdateTrackballManipulatorColor( m_trackballManipulator );

        // Trackball Rotation
        //-------------------------------------------------------------------------

        if ( m_activeManipulator == Manipulator::None || m_activeManipulator == Manipulator::RotateTrackball )
        {
            pDrawList->AddConvexPolyFilled( m_screenManipulator.m_pointsSS.data(), (int32_t) m_screenManipulator.m_pointsSS.size(), m_trackballManipulator.m_color );

            if ( IsManipulating() )
            {
                DrawTrackballRotationManipulationVisualization( ctx );
            }
        }

        // Axis Rotation
        //-------------------------------------------------------------------------

        for ( int32_t d = 0; d < 3; d++ )
        {
            // Only draw the currently active manipulator
            if ( IsManipulating() && m_axes[d].m_manipulator != m_activeManipulator )
            {
                continue;
            }

            pDrawList->AddPolyline( m_axes[d].m_pointsSS.data(), (int32_t) m_axes[d].m_pointsSS.size(), m_axes[d].m_color, 0, m_style.m_manipulatorThickness );

            if ( IsManipulating() )
            {
                DrawAxisRotationManipulationVisualization( ctx, m_axes[d] );
            }
        }

        // Screen Rotation
        //-------------------------------------------------------------------------

        if ( m_activeManipulator == Manipulator::None || m_activeManipulator == Manipulator::RotateScreen )
        {
            pDrawList->AddPolyline( m_screenManipulator.m_pointsSS.data(), (int32_t) m_screenManipulator.m_pointsSS.size(), m_screenManipulator.m_color, 0, m_style.m_screenManipulatorThickness );

            if ( IsManipulating() )
            {
                DrawScreenRotationManipulationVisualization( ctx );
            }
        }

        // Draw origin sphere
        //-------------------------------------------------------------------------

        Color originColor = Style::s_defaultColor;

        switch ( m_activeManipulator )
        {
            break;

            case Manipulator::RotateX:
            {
                originColor = ImGuiX::Style::s_axisColorX;
            }
            break;

            case Manipulator::RotateY:
            {
                originColor = ImGuiX::Style::s_axisColorY;
            }
            break;

            case Manipulator::RotateZ:
            {
                originColor = ImGuiX::Style::s_axisColorZ;
            }
            break;

            case Manipulator::RotateTrackball:
            {
                originColor = m_screenManipulator.m_color;
            }
            break;

            case Manipulator::RotateScreen:
            {
                originColor = m_screenManipulator.m_color;
            }
            break;

            default:
            break;
        }

        pDrawList->AddCircleFilled( ctx.m_positionSS.ToFloat2(), m_style.m_originCircleRadius, originColor );
    }

    bool RotationGizmo::TryStartManipulating( Context const& ctx )
    {
        if ( ctx.m_isMouseInViewport && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
        {
            // Check Axes
            //-------------------------------------------------------------------------

            for ( int32_t i = 0; i < 3; i++ )
            {
                if ( m_axes[i].m_isHovered )
                {
                    m_activeManipulator = m_axes[i].m_manipulator;
                    m_rotationAxis = m_axes[i].m_axisDirWS;
                    m_rotationPlane = Plane::FromNormalAndPoint( m_rotationAxis, ctx.m_positionWS );
                    m_manipulationStartMousePositionSS = ctx.m_mousePositionSS;
                    m_manipulationStartRotationWS = ctx.m_rotationWS;
                    m_previousRotationAngle = 0.0f;
                    m_previousRotation = ctx.m_rotationWS;

                    m_projectedStartMousePosWS = ProjectScreenSpacePositionOntoRotationPlane( ctx, m_manipulationStartMousePositionSS );
                    return true;
                }
            }

            // Check Screen Manipulator
            //-------------------------------------------------------------------------

            if ( m_screenManipulator.m_isHovered )
            {
                m_activeManipulator = Manipulator::RotateScreen;
                m_rotationAxis = ctx.m_viewDirectionWS;
                m_manipulationStartMousePositionSS = ctx.m_mousePositionSS;
                m_manipulationStartRotationWS = ctx.m_rotationWS;
                m_previousRotationAngle = 0.0f;
                m_previousRotation = ctx.m_rotationWS;
                return true;
            }

            // Check Trackball Manipulator
            //-------------------------------------------------------------------------

            if ( m_trackballManipulator.m_isHovered )
            {
                m_activeManipulator = Manipulator::RotateTrackball;
                m_rotationPlane = Plane::FromNormalAndPoint( -ctx.m_viewDirectionWS, ctx.m_positionWS );
                m_manipulationStartMousePositionSS = ctx.m_mousePositionSS;
                m_manipulationStartRotationWS = ctx.m_rotationWS;
                m_previousRotationAngle = 0.0f;
                m_previousRotation = ctx.m_rotationWS;
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
            }
            else if ( m_activeManipulator == Manipulator::RotateTrackball )
            {
                float const deltaDistanceSS = ( ctx.m_mousePositionSS - m_manipulationStartMousePositionSS ).GetLength2();
                if ( deltaDistanceSS > m_style.m_trackballDeadZone )
                {
                    Vector const projectedStartMousePosWS = ProjectScreenSpacePositionOntoRotationPlane( ctx, m_manipulationStartMousePositionSS );
                    Vector const projectedMousePosWS = ProjectScreenSpacePositionOntoRotationPlane( ctx, ctx.m_mousePositionSS );
                    Vector const deltaMousePosDirWS = ( projectedMousePosWS - projectedStartMousePosWS ).GetNormalized2();

                    // Convert SS distance to angle, use the widget radius to imagine a real ball and convert the pixel distance to an angular distance on the ball
                    Radians const deltaAngle = deltaDistanceSS / m_trackballManipulator.m_manipulationRadius;

                    // Calculate axis of rotation and delta
                    m_rotationAxis = m_rotationPlane.GetNormal().Cross3( deltaMousePosDirWS ).GetNormalized3();
                    Quaternion const newRot = Quaternion( m_rotationAxis, deltaAngle ) * m_manipulationStartRotationWS;
                    m_delta = Quaternion::Delta( m_previousRotation, newRot );
                    m_previousRotation = newRot;
                }
            }
            else // Axis rotation
            {
                float const mousePositionDeltaSS = ( ctx.m_mousePositionSS - m_manipulationStartMousePositionSS ).GetLength2();
                if ( mousePositionDeltaSS > m_style.m_axisManipulatorDeadZone )
                {
                    Vector projectedMousePosWS = ProjectScreenSpacePositionOntoRotationPlane( ctx, ctx.m_mousePositionSS );

                    Vector const manipulationStart = ( m_projectedStartMousePosWS - ctx.m_positionWS ).GetNormalized3();
                    Vector const manipulationCurrent = ( projectedMousePosWS - ctx.m_positionWS ).GetNormalized3();
                    Radians rotationAngle = Math::CalculateAngleBetweenUnitVectors( manipulationStart, manipulationCurrent );

                    // Check if this is a positive rotation
                    Vector manipulationNormal = Quaternion( m_rotationAxis, Math::PiDivTwo ).RotateVector( manipulationStart );
                    float const dot = manipulationNormal.GetDot3( manipulationCurrent );
                    if ( dot < 0 )
                    {
                        rotationAngle = -rotationAngle;
                    }

                    Radians const deltaAngle = rotationAngle - m_previousRotationAngle;
                    m_previousRotationAngle = rotationAngle;
                    m_delta = Quaternion( m_rotationAxis, deltaAngle );
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

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        InlineString str;

        switch ( m_activeManipulator )
        {
            case Manipulator::None:
            str.sprintf( "Not Manipulating" );
            break;

            case Manipulator::RotateX:
            str.sprintf( "Rotate X, Angle: %.2f deg", m_previousRotationAngle.ToDegrees().ToFloat() );
            break;

            case Manipulator::RotateY:
            str.sprintf( "Rotate Y, Angle: %.2f deg", m_previousRotationAngle.ToDegrees().ToFloat() );
            break;

            case Manipulator::RotateZ:
            str.sprintf( "Rotate Z, Angle: %.2f deg", m_previousRotationAngle.ToDegrees().ToFloat() );
            break;

            case Manipulator::RotateTrackball:
            str.sprintf( "Rotate Trackball, Angle: %.2f deg", m_previousRotationAngle.ToDegrees().ToFloat() );
            break;

            case Manipulator::RotateScreen:
            str.sprintf( "Rotate Screen, Angle: %.2f deg", m_previousRotationAngle.ToDegrees().ToFloat() );
            break;
        }

        Float2 const textPos = ctx.m_positionSS + Float2( 4, 4 );
        pDrawList->AddText( textPos, Colors::HotPink, str.c_str() );
    }

    Vector RotationGizmo::ProjectScreenSpacePositionOntoRotationPlane( Context const& ctx, Vector const &positionSS )
    {
        EE_ASSERT( IsManipulating() );

        LineSegment const worldRay = ctx.m_viewport.ScreenSpaceToWorldSpace( positionSS );
        auto const result = Math::IntersectLinePlane( worldRay.GetStartPoint(), worldRay.GetEndPoint(), m_rotationPlane );
        EE_ASSERT( result.m_intersects );
        return result.m_intersectionPoint;
    }

    void RotationGizmo::DrawAxisRotationManipulationVisualization( Context const& ctx, AxisManipulator const& axis )
    {
        if ( !ctx.m_isMouseInViewport )
        {
            return;
        }

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();

        pDrawList->AddLine( axis.m_axisStartInfSS, axis.m_axisEndInfSS, axis.m_color, 1.0f );

        //-------------------------------------------------------------------------

        LineSegment const manipulationStartLine = ctx.m_viewport.ScreenSpaceToWorldSpace( m_manipulationStartMousePositionSS );
        LineSegment const manipulationEndLine = ctx.m_viewport.ScreenSpaceToWorldSpace( ctx.m_mousePositionSS );

        Math::RayPlaneResult const r0 = Math::IntersectLinePlane( manipulationStartLine, m_rotationPlane );
        Math::RayPlaneResult const r1 = Math::IntersectLinePlane( manipulationEndLine, m_rotationPlane );

        if ( !r0 || !r1 )
        {
            return;
        }

        Vector const startDir = ( r0.m_intersectionPoint - ctx.m_positionWS ).GetNormalized3();
        Vector const endDir = ( r1.m_intersectionPoint - ctx.m_positionWS ).GetNormalized3();

        Radians const deltaAngle = Math::CalculateAngleBetweenUnitVectors( startDir, endDir );
        int32_t const numPoints = Math::CeilingToInt32( deltaAngle.ToFloat() / Style::s_minAngleRadiansBetweenPoints );
        Radians const stepAngle = deltaAngle / numPoints;

        Vector const axesScale( PixelHeightToWorldHeight( ctx, ctx.m_positionWS, m_style.m_manipulatorRadius ) );

        if ( numPoints > 0 )
        {
            // Check if this is a positive rotation
            Vector rotationAxis = axis.m_axisDirWS;
            Vector const manipulationNormal = Quaternion( m_rotationAxis, Math::PiDivTwo ).RotateVector( startDir * axesScale );
            float const dot = manipulationNormal.GetDot3( endDir );
            if ( dot < 0 )
            {
                rotationAxis.Negate();
            }

            // Generate points
            TInlineVector<ImVec2, 190> points;
            points.emplace_back( ctx.m_positionSS );

            for ( int32_t i = 0; i < numPoints; i++ )
            {
                Quaternion rot( rotationAxis, stepAngle * i );
                Vector const vPoint = ctx.m_viewport.WorldSpaceToScreenSpace( ctx.m_positionWS + rot.RotateVector( startDir * axesScale ) );
                points.emplace_back( vPoint );
            }

            pDrawList->AddConvexPolyFilled( points.data(), (int32_t) points.size(), axis.m_color.GetAlphaVersion( 0.75f ) );
        }
    }

    void RotationGizmo::DrawScreenRotationManipulationVisualization( Context const& ctx )
    {
        if ( !ctx.m_isMouseInViewport )
        {
            return;
        }

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();

        Vector const vRef = ( m_manipulationStartMousePositionSS - ctx.m_positionSS ).GetNormalized2();
        Vector const vEnd = ( ctx.m_mousePositionSS - ctx.m_positionSS ).GetNormalized2();

        Radians const deltaAngle = Math::CalculateAngleBetweenVectors( vRef, vEnd );
        int32_t const numPoints = Math::CeilingToInt32( deltaAngle.ToFloat() / Style::s_minAngleRadiansBetweenPoints );
        Radians const stepAngle = deltaAngle / numPoints;
        Vector const rotationAxis = Math::IsVectorToTheLeft( vRef, vEnd ) ? -Vector::UnitZ : Vector::UnitZ;

        if ( numPoints > 0 )
        {
            pDrawList->AddConvexPolyFilled( m_screenManipulator.m_pointsSS.data(), (int32_t) m_screenManipulator.m_pointsSS.size(), m_screenManipulator.m_color.GetAlphaVersion( 0.75f ) );
        }
    }

    void RotationGizmo::DrawTrackballRotationManipulationVisualization( Context const& ctx )
    {
        if ( !ctx.m_isMouseInViewport )
        {
            return;
        }

        // Nothing to draw
    }
}
#endif