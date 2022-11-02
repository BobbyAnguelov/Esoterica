#include "Gizmo.h"
#include "imgui.h"
#include "System/Imgui/ImguiX.h"
#include "System/Math/MathHelpers.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    void Gizmo::Result::ApplyResult( Transform& transform ) const
    {
        switch ( m_deltaType )
        {
            case ResultDeltaType::Rotation:
            {
                transform.AddRotation( m_deltaOrientation );
            }
            break;

            case ResultDeltaType::Translation:
            {
                transform.AddTranslation( m_deltaPositionScale );
            }
            break;

            case ResultDeltaType::Scale:
            {
                float newScale = transform.GetScale() + m_deltaPositionScale.m_x;
                if ( Math::IsNearZero( newScale ) )
                {
                    newScale = 0.01f;
                }

                transform.SetScale( newScale );
            }
            break;

            case ResultDeltaType::NonUniformScale:
            {
                EE_UNREACHABLE_CODE(); // Not support for transforms
            }
            break;

            default:
            break;
        }
    }

    //-------------------------------------------------------------------------

    Gizmo::Gizmo()
    {
        m_options.SetAllFlags();
        m_options.ClearFlag( Options::AllowNonUniformScale );
    }

    Gizmo::Gizmo( TBitFlags<Options> options )
        : m_options( options )
    {}

    void Gizmo::SwitchToNextMode()
    {
        switch ( m_gizmoMode )
        {
            case GizmoMode::Translation:
            {
                SwitchMode( GizmoMode::Rotation );
            }
            break;

            case GizmoMode::Rotation:
            {
                if ( m_options.IsFlagSet( Options::AllowScale ) )
                {
                    SwitchMode( GizmoMode::Scale );
                }
                else
                {
                    SwitchMode( GizmoMode::Translation );
                }
            }
            break;

            case GizmoMode::Scale:
            {
                EE_ASSERT( m_options.IsFlagSet( Options::AllowScale ) );
                SwitchMode( GizmoMode::Translation );
            }
            break;
        }
    }

    void Gizmo::SwitchMode( GizmoMode newMode )
    {
        if ( newMode == m_gizmoMode )
        {
            return;
        }

        if ( newMode == GizmoMode::Scale )
        {
            if ( !m_options.IsFlagSet( Options::AllowScale ) )
            {
                EE_LOG_WARNING( "Tools", "Gizmo", "Trying to switch a gizmo to scale mode but it is disabled!" );
                return;
            }
        }

        m_gizmoMode = newMode;
        m_manipulationMode = ManipulationMode::None;
    }

    void Gizmo::SetCoordinateSystemSpace( CoordinateSpace space )
    {
        if ( !m_options.IsFlagSet( Options::AllowCoordinateSpaceSwitching ) )
        {
            EE_LOG_WARNING( "Tools", "Gizmo", "Trying to switch a gizmo's coordinate system, but this is disabled!" );
            return;
        }

        //-------------------------------------------------------------------------

        if ( m_coordinateSpace == space )
        {
            return;
        }

        //-------------------------------------------------------------------------

        m_coordinateSpace = space;
    }

    Gizmo::Result Gizmo::Draw( Vector const& positionWS, Quaternion const& orientationWS, Render::Viewport const& viewport )
    {
        Render::Viewport offsetViewport = viewport;
        offsetViewport.Resize( Float2( ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMin() ), viewport.GetDimensions() );

        //-------------------------------------------------------------------------

        Result result;

        if ( !viewport.IsWorldSpacePointVisible( positionWS ) )
        {
            if ( m_manipulationMode != ManipulationMode::None )
            {
                result.m_state = State::StoppedManipulating;
                m_manipulationMode = ManipulationMode::None;
            }

            return result;
        }

        //-------------------------------------------------------------------------

        switch ( m_gizmoMode )
        {
            case GizmoMode::Translation:
            {
                if ( m_manipulationMode == ManipulationMode::None || ( m_manipulationMode >= ManipulationMode::TranslateX && m_manipulationMode <= ManipulationMode::TranslateXZ ) )
                {
                    return DrawTranslationGizmo( positionWS, orientationWS, offsetViewport );
                }
            }
            break;

            case GizmoMode::Rotation:
            {
                if ( m_manipulationMode == ManipulationMode::None || ( m_manipulationMode >= ManipulationMode::RotateX && m_manipulationMode <= ManipulationMode::RotateZ ) )
                {
                    return DrawRotationGizmo( positionWS, orientationWS, offsetViewport );
                }
            }
            break;

            case GizmoMode::Scale:
            {
                if ( m_manipulationMode == ManipulationMode::None || m_manipulationMode >= ManipulationMode::ScaleX )
                {
                    return DrawScaleGizmo( positionWS, orientationWS, offsetViewport );
                }
            }
            break;
        }

        //-------------------------------------------------------------------------

        // Cancel previously active operation for a different mode
        if ( m_manipulationMode != ManipulationMode::None )
        {
            result.m_state = State::StoppedManipulating;
            m_manipulationMode = ManipulationMode::None;
        }

        return result;
    }

    //-------------------------------------------------------------------------

    static Color const                  g_hoveredAxisColorX = Colors::Red;
    static Color const                  g_hoveredAxisColorY = Colors::LimeGreen;
    static Color const                  g_hoveredAxisColorZ = Colors::DodgerBlue;
    static Color const                  g_axisColorX = g_hoveredAxisColorX.GetScaledColor( 0.7f );
    static Color const                  g_axisColorY = g_hoveredAxisColorY.GetScaledColor( 0.7f );
    static Color const                  g_axisColorZ = g_hoveredAxisColorZ.GetScaledColor( 0.7f );
    static Color const                  g_rotationPreviewColor = Colors::Orange;
    static Color const                  g_originColor = Colors::White;

    static constexpr float const        g_axisLength = 150.0f;
    static constexpr float const        g_axisThickness = 4.0f;
    static constexpr float const        g_axisAdditionalHoverBorder = 3.0f;
    static constexpr float const        g_axisPlaneManipulatorScale = 0.5f;
    static constexpr float const        g_originCircleWidth = 4.0f;
    static constexpr float const        g_originCircleOffset = g_originCircleWidth * 3;
    static constexpr float const        g_hoverDetectionDistance = g_axisThickness + g_axisAdditionalHoverBorder;

    Gizmo::Result Gizmo::DrawTranslationGizmo( Vector const& originWS, Quaternion const& orientationWS, Render::Viewport const& viewport )
    {
        EE_ASSERT( orientationWS.IsNormalized() );

        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        Vector const mousePos( io.MousePos.x, io.MousePos.y, 0, 1.0f );
        bool const isMouseInViewport = viewport.ContainsPointScreenSpace( io.MousePos );

        bool const isManipulating = m_manipulationMode != ManipulationMode::None;
        EE_ASSERT( !isManipulating || ( m_manipulationMode >= ManipulationMode::TranslateX && m_manipulationMode <= ManipulationMode::TranslateXZ ) );

        //-------------------------------------------------------------------------

        Vector const originSS = viewport.WorldSpaceToScreenSpace( originWS );
        Vector const axesScale( viewport.GetScalingFactorAtPosition( originWS, g_axisLength ) );

        //-------------------------------------------------------------------------

        struct AxisDrawInfo
        {
            Vector              m_axisDirWS = Vector::Zero;
            Vector              m_scaledAxisDirWS = Vector::Zero;
            Vector              m_axisStartSS = Vector::Zero;
            Vector              m_axisEndSS = Vector::Zero;
            Vector              m_axisDirSS = Vector::Zero;
            uint32_t            m_color = 0;
            ManipulationMode    m_manipulationMode = ManipulationMode::None;
            float               m_distanceToCamera = FLT_MAX;
            bool                m_isHovered = false;
            bool                m_isAxisFlipped = false;
        };

        float lowestAxesDistanceToCamera = FLT_MAX;
        TInlineVector<AxisDrawInfo, 3> axisInfo;
        axisInfo.resize( 3 );

        // Calculate initial axis screen space points
        //-------------------------------------------------------------------------

        axisInfo[0].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitX : orientationWS.RotateVector( Vector::UnitX );
        axisInfo[1].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitY : orientationWS.RotateVector( Vector::UnitY );
        axisInfo[2].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitZ : orientationWS.RotateVector( Vector::UnitZ );

        axisInfo[0].m_axisDirWS.Normalize3();
        axisInfo[1].m_axisDirWS.Normalize3();
        axisInfo[2].m_axisDirWS.Normalize3();

        if ( m_options.IsFlagSet( Options::AllowAxesFlipping ) )
        {
            Vector const viewDirWS = viewport.IsOrthographic() ? viewport.GetViewForwardDirection().GetNegated() : ( viewport.GetViewPosition() - originWS ).GetNormalized3();
            for ( int32_t i = 0; i < 3; i++ )
            {
                if ( axisInfo[i].m_axisDirWS.GetDot3( viewDirWS ) < 0.0f )
                {
                    axisInfo[i].m_axisDirWS.Negate();
                    axisInfo[i].m_isAxisFlipped = true;
                }
            }
        }

        axisInfo[0].m_scaledAxisDirWS = axisInfo[0].m_axisDirWS * axesScale;
        axisInfo[0].m_axisEndSS = viewport.WorldSpaceToScreenSpace( originWS + axisInfo[0].m_scaledAxisDirWS );
        axisInfo[0].m_axisDirSS = ( axisInfo[0].m_axisEndSS - originSS ).GetNormalized2();
        axisInfo[0].m_axisStartSS = originSS + ( axisInfo[0].m_axisDirSS * g_originCircleOffset );
        axisInfo[0].m_manipulationMode = ManipulationMode::TranslateX;
        axisInfo[0].m_isHovered = ( m_manipulationMode == axisInfo[0].m_manipulationMode );
        axisInfo[0].m_distanceToCamera = viewport.GetViewPosition().GetDistance3( originWS + axisInfo[0].m_scaledAxisDirWS );
        lowestAxesDistanceToCamera = Math::Min( lowestAxesDistanceToCamera, axisInfo[0].m_distanceToCamera );

        axisInfo[1].m_scaledAxisDirWS = axisInfo[1].m_axisDirWS * axesScale;
        axisInfo[1].m_axisEndSS = viewport.WorldSpaceToScreenSpace( originWS + axisInfo[1].m_scaledAxisDirWS );
        axisInfo[1].m_axisDirSS = ( axisInfo[1].m_axisEndSS - originSS ).GetNormalized2();
        axisInfo[1].m_axisStartSS = originSS + ( axisInfo[1].m_axisDirSS * g_originCircleOffset );
        axisInfo[1].m_manipulationMode = ManipulationMode::TranslateY;
        axisInfo[1].m_isHovered = ( m_manipulationMode == axisInfo[1].m_manipulationMode );
        axisInfo[1].m_distanceToCamera = viewport.GetViewPosition().GetDistance3( originWS + axisInfo[1].m_scaledAxisDirWS );
        lowestAxesDistanceToCamera = Math::Min( lowestAxesDistanceToCamera, axisInfo[1].m_distanceToCamera );

        axisInfo[2].m_scaledAxisDirWS = axisInfo[2].m_axisDirWS * axesScale;
        axisInfo[2].m_axisEndSS = viewport.WorldSpaceToScreenSpace( originWS + axisInfo[2].m_scaledAxisDirWS );
        axisInfo[2].m_axisDirSS = ( axisInfo[2].m_axisEndSS - originSS ).GetNormalized2();
        axisInfo[2].m_axisStartSS = originSS + ( axisInfo[2].m_axisDirSS * g_originCircleOffset );
        axisInfo[2].m_manipulationMode = ManipulationMode::TranslateZ;
        axisInfo[2].m_isHovered = ( m_manipulationMode == axisInfo[2].m_manipulationMode );
        axisInfo[2].m_distanceToCamera = viewport.GetViewPosition().GetDistance3( originWS + axisInfo[2].m_scaledAxisDirWS );
        lowestAxesDistanceToCamera = Math::Min( lowestAxesDistanceToCamera, axisInfo[2].m_distanceToCamera );

        // Plane manipulators
        //-------------------------------------------------------------------------

        constexpr uint32_t const numPlaneManipulationPoints = 20;
        constexpr float const deltaAngle = Math::PiDivTwo / ( numPlaneManipulationPoints - 1 );

        struct PlaneDrawInfo
        {
            Vector              m_manipulationPlaneAxis;
            ImVec2              m_pointsSS[ numPlaneManipulationPoints];
            float               m_distanceToCamera = FLT_MAX;
            uint32_t            m_color = 0;
            ManipulationMode    m_manipulationMode = ManipulationMode::None;
            bool                m_isHovered = false;
            bool                m_isRotationDirFlipped = false;
        };

        TInlineVector<PlaneDrawInfo, 3> planeInfo;
        planeInfo.resize( 3 );

        planeInfo[0].m_manipulationPlaneAxis = axisInfo[2].m_axisDirWS;
        planeInfo[0].m_manipulationMode = ManipulationMode::TranslateXY;
        planeInfo[0].m_color = ConvertColor( Colors::Yellow );
        planeInfo[0].m_isHovered = ( m_manipulationMode == planeInfo[0].m_manipulationMode );
        planeInfo[0].m_isRotationDirFlipped = Math::IsOdd( uint32_t( axisInfo[0].m_isAxisFlipped ) + uint32_t( axisInfo[1].m_isAxisFlipped ) );

        planeInfo[1].m_manipulationPlaneAxis = axisInfo[0].m_axisDirWS;
        planeInfo[1].m_manipulationMode = ManipulationMode::TranslateYZ;
        planeInfo[1].m_color = ConvertColor( Colors::DarkOrchid );
        planeInfo[1].m_isHovered = ( m_manipulationMode == planeInfo[1].m_manipulationMode );
        planeInfo[1].m_isRotationDirFlipped = Math::IsOdd( uint32_t( axisInfo[1].m_isAxisFlipped ) + uint32_t( axisInfo[2].m_isAxisFlipped ) );

        planeInfo[2].m_manipulationPlaneAxis = axisInfo[1].m_axisDirWS;
        planeInfo[2].m_manipulationMode = ManipulationMode::TranslateXZ;
        planeInfo[2].m_color = ConvertColor( Colors::LightSeaGreen );
        planeInfo[2].m_isHovered = ( m_manipulationMode == planeInfo[2].m_manipulationMode );
        planeInfo[2].m_isRotationDirFlipped = Math::IsOdd( uint32_t( axisInfo[0].m_isAxisFlipped ) + uint32_t( axisInfo[2].m_isAxisFlipped ) );

        float lowestPlaneDistanceToCamera = FLT_MAX;

        for ( auto p = 0; p < 3; p++ )
        {
            int32_t const axisToRotateIdx = p;
            int32_t const rotationAxisIdx = ( p + 2 ) % 3;

            float const angleDir = ( planeInfo[p].m_isRotationDirFlipped ? -1.0f : 1.0f );
            Vector const rotationAxis = axisInfo[rotationAxisIdx].m_isAxisFlipped ? axisInfo[rotationAxisIdx].m_axisDirWS.GetNegated() : axisInfo[rotationAxisIdx].m_axisDirWS;

            for ( auto i = 0; i < numPlaneManipulationPoints; i++ )
            {
                Radians const angle( deltaAngle * i * angleDir );
                Quaternion rot( rotationAxis, angle );
                Vector pointWS = rot.RotateVector( axisInfo[axisToRotateIdx].m_scaledAxisDirWS * g_axisPlaneManipulatorScale );
                pointWS += originWS;

                planeInfo[p].m_distanceToCamera = Math::Min( planeInfo[p].m_distanceToCamera, viewport.GetViewPosition().GetDistance3( pointWS ) );
                planeInfo[p].m_pointsSS[i] = viewport.WorldSpaceToScreenSpace( pointWS );

                // Check for hover
                if ( !planeInfo[p].m_isHovered && !isManipulating )
                {
                    float const distance = mousePos.GetDistance2( planeInfo[p].m_pointsSS[i] );
                    if ( distance < g_hoverDetectionDistance )
                    {
                        planeInfo[p].m_isHovered = true;
                    }
                }
            }

            lowestPlaneDistanceToCamera = Math::Min( lowestPlaneDistanceToCamera, planeInfo[p].m_distanceToCamera );
        }

        bool const isPlaneManipulatorHovered = planeInfo[0].m_isHovered || planeInfo[1].m_isHovered || planeInfo[2].m_isHovered;

        // Manage Hover State
        //-------------------------------------------------------------------------

        if ( isManipulating )
        {
            if ( isPlaneManipulatorHovered )
            {
                if ( planeInfo[0].m_isHovered )
                {
                    axisInfo[0].m_isHovered = true;
                    axisInfo[1].m_isHovered = true;
                    axisInfo[2].m_isHovered = false;
                }
                else if ( planeInfo[1].m_isHovered )
                {
                    axisInfo[0].m_isHovered = false;
                    axisInfo[1].m_isHovered = true;
                    axisInfo[2].m_isHovered = true;
                }
                else if ( planeInfo[2].m_isHovered )
                {
                    axisInfo[0].m_isHovered = true;
                    axisInfo[1].m_isHovered = false;
                    axisInfo[2].m_isHovered = true;
                }
            }
        }
        else if ( isMouseInViewport )
        {
            if ( isPlaneManipulatorHovered )
            {
                // Bias selection to closest plane
                int32_t const numHoveredPlanes = uint32_t( planeInfo[0].m_isHovered ) + uint32_t( planeInfo[1].m_isHovered ) + uint32_t( planeInfo[2].m_isHovered );
                if ( numHoveredPlanes > 1 )
                {
                    if ( planeInfo[0].m_isHovered && planeInfo[0].m_distanceToCamera > lowestPlaneDistanceToCamera )
                    {
                        planeInfo[0].m_isHovered = false;
                    }

                    if ( planeInfo[1].m_isHovered && planeInfo[1].m_distanceToCamera > lowestPlaneDistanceToCamera )
                    {
                        planeInfo[1].m_isHovered = false;
                    }

                    if ( planeInfo[2].m_isHovered && planeInfo[2].m_distanceToCamera > lowestPlaneDistanceToCamera )
                    {
                        planeInfo[2].m_isHovered = false;
                    }
                }

                // Set hover state for individual axes
                if ( planeInfo[0].m_isHovered )
                {
                    axisInfo[0].m_isHovered = true;
                    axisInfo[1].m_isHovered = true;
                    axisInfo[2].m_isHovered = false;
                }
                else if ( planeInfo[1].m_isHovered )
                {
                    axisInfo[0].m_isHovered = false;
                    axisInfo[1].m_isHovered = true;
                    axisInfo[2].m_isHovered = true;
                }
                else if ( planeInfo[2].m_isHovered )
                {
                    axisInfo[0].m_isHovered = true;
                    axisInfo[1].m_isHovered = false;
                    axisInfo[2].m_isHovered = true;
                }
            }
            else
            {
                int32_t numHoveredAxes = 0;

                if ( !axisInfo[0].m_axisStartSS.IsNearEqual3( axisInfo[0].m_axisEndSS, g_hoverDetectionDistance ) )
                {
                    LineSegment const axisSegment( axisInfo[0].m_axisStartSS, axisInfo[0].m_axisEndSS );
                    float const mouseToLineDistance = axisSegment.GetDistanceFromSegmentToPoint( mousePos );
                    axisInfo[0].m_isHovered = mouseToLineDistance < g_hoverDetectionDistance;
                    numHoveredAxes += uint8_t( axisInfo[0].m_isHovered );
                }

                if ( !axisInfo[1].m_axisStartSS.IsNearEqual3( axisInfo[1].m_axisEndSS, g_hoverDetectionDistance ) )
                {
                    LineSegment const axisSegment( axisInfo[1].m_axisStartSS, axisInfo[1].m_axisEndSS );
                    float const mouseToLineDistance = axisSegment.GetDistanceFromSegmentToPoint( mousePos );
                    axisInfo[1].m_isHovered = mouseToLineDistance < g_hoverDetectionDistance;
                    numHoveredAxes += uint8_t( axisInfo[1].m_isHovered );
                }

                if ( !axisInfo[2].m_axisStartSS.IsNearEqual3( axisInfo[2].m_axisEndSS, g_hoverDetectionDistance ) )
                {
                    LineSegment const axisSegment( axisInfo[2].m_axisStartSS, axisInfo[2].m_axisEndSS );
                    float const mouseToLineDistance = axisSegment.GetDistanceFromSegmentToPoint( mousePos );
                    axisInfo[2].m_isHovered = mouseToLineDistance < g_hoverDetectionDistance;
                    numHoveredAxes += uint8_t( axisInfo[2].m_isHovered );
                }

                // Ensure only the closest axis is selected
                //-------------------------------------------------------------------------

                if ( numHoveredAxes > 1 )
                {
                    if ( axisInfo[0].m_isHovered && axisInfo[0].m_distanceToCamera > lowestAxesDistanceToCamera )
                    {
                        axisInfo[0].m_isHovered = false;
                    }

                    if ( axisInfo[1].m_isHovered && axisInfo[1].m_distanceToCamera > lowestAxesDistanceToCamera )
                    {
                        axisInfo[1].m_isHovered = false;
                    }

                    if ( axisInfo[2].m_isHovered && axisInfo[2].m_distanceToCamera > lowestAxesDistanceToCamera )
                    {
                        axisInfo[2].m_isHovered = false;
                    }
                }
            }
        }

        bool const isAxisHovered = axisInfo[0].m_isHovered || axisInfo[1].m_isHovered || axisInfo[2].m_isHovered;

        // Draw Widget
        //-------------------------------------------------------------------------

        axisInfo[0].m_color = ( axisInfo[0].m_isHovered ) ? ConvertColor( g_hoveredAxisColorX ) : ConvertColor( g_axisColorX );
        axisInfo[1].m_color = ( axisInfo[1].m_isHovered ) ? ConvertColor( g_hoveredAxisColorY ) : ConvertColor( g_axisColorY );
        axisInfo[2].m_color = ( axisInfo[2].m_isHovered ) ? ConvertColor( g_hoveredAxisColorZ ) : ConvertColor( g_axisColorZ );

        // Sort axes back to front
        TInlineVector<int32_t, 6> drawOrder = { 0, 1, 2, 3, 4, 5 };

        auto Comparator = [&] ( int32_t const& a, int32_t const& b )
        {
            float const va = ( a < 3 ) ? axisInfo[a].m_distanceToCamera : planeInfo[a-3].m_distanceToCamera;
            float const vb = ( b < 3 ) ? axisInfo[b].m_distanceToCamera : planeInfo[b-3].m_distanceToCamera;
            return va > vb;
        };

        eastl::sort( drawOrder.begin(), drawOrder.end(), Comparator );

        // Draw origin sphere
        pDrawList->AddCircleFilled( originSS.ToFloat2(), g_originCircleWidth, ConvertColor( g_originColor ) );

        // Manipulators
        for ( auto d : drawOrder )
        {
            // Axes
            if ( d < 3 )
            {
                pDrawList->AddLine( axisInfo[d].m_axisStartSS, axisInfo[d].m_axisEndSS, axisInfo[d].m_color, g_axisThickness );

                // Draw axis caps
                if ( !isManipulating || axisInfo[d].m_isHovered )
                {
                    Vector const arrowBaseDirSS = axisInfo[d].m_axisDirSS.Orthogonal2D();

                    float const arrowThickness = g_axisThickness * 2;
                    Vector T0 = axisInfo[d].m_axisEndSS + ( axisInfo[d].m_axisDirSS * arrowThickness );
                    Vector T1 = axisInfo[d].m_axisEndSS - ( arrowBaseDirSS * arrowThickness );
                    Vector T2 = axisInfo[d].m_axisEndSS + ( arrowBaseDirSS * arrowThickness );

                    pDrawList->AddTriangleFilled( T0, T1, T2, axisInfo[d].m_color );
                }
            }
            else // Plane Manipulator
            {
                int32_t const p = d - 3;
                pDrawList->AddPolyline( planeInfo[p].m_pointsSS, numPlaneManipulationPoints, planeInfo[p].m_color, 0, g_axisThickness );
            }
        }

        // User Manipulation
        //-------------------------------------------------------------------------

        Result result;

        if ( isMouseInViewport )
        {
            // Check if we should start manipulating
            //-------------------------------------------------------------------------

            if ( !isManipulating && isAxisHovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
            {
                LineSegment const mouseWorldRay = viewport.ScreenSpaceToWorldSpace( mousePos );
                LineSegment manipulationAxis( Vector::Zero, Vector::One );
                Plane manipulationPlane = Plane::FromNormalAndPoint( viewport.GetViewForwardDirection(), originWS ); // View plane by default

                if ( isPlaneManipulatorHovered )
                {
                    for ( int32_t i = 0; i < 3; i++ )
                    {
                        if ( planeInfo[i].m_isHovered )
                        {
                            manipulationPlane = Plane::FromNormalAndPoint( planeInfo[i].m_manipulationPlaneAxis, originWS );
                            m_manipulationMode = planeInfo[i].m_manipulationMode;
                            break;
                        }
                    }
                }
                else // Individual axis hovered
                {
                    for ( int32_t i = 0; i < 3; i++ )
                    {
                        if ( axisInfo[i].m_isHovered )
                        {
                            manipulationAxis = LineSegment( originWS, originWS + axisInfo[i].m_scaledAxisDirWS );
                            m_manipulationMode = axisInfo[i].m_manipulationMode;
                            break;
                        }
                    }
                }

                // Project mouse onto manipulation plane
                Vector projectedPoint;
                if ( manipulationPlane.IntersectLine( mouseWorldRay.GetStartPoint(), mouseWorldRay.GetEndPoint(), projectedPoint ) )
                {
                    if ( !isPlaneManipulatorHovered )
                    {
                        projectedPoint = manipulationAxis.GetClosestPointOnLine( projectedPoint );
                    }

                    m_translationScaleDeltaOriginWS = projectedPoint;
                }

                result.m_state = State::StartedManipulating;
            }

            // Handle widget manipulation
            //-------------------------------------------------------------------------

            if ( m_manipulationMode != ManipulationMode::None )
            {
                // Set the hovered ID so that we disable any click through in the UI
                ImGui::SetHoveredID( ImGui::GetID( "Gizmo" ) );

                Vector const mouseDelta( io.MouseDelta );
                if ( !mouseDelta.IsNearZero2() )
                {
                    LineSegment const mouseWorldRay = viewport.ScreenSpaceToWorldSpace( mousePos );
                    LineSegment manipulationAxis( Vector::Zero, Vector::One );
                    Plane manipulationPlane = Plane::FromNormalAndPoint( viewport.GetViewForwardDirection(), originWS ); // View plane by default

                    // Single axis manipulation
                    if ( m_manipulationMode >= ManipulationMode::TranslateX && m_manipulationMode <= ManipulationMode::TranslateZ )
                    {
                        uint32_t const axisIndex = uint8_t( m_manipulationMode ) - uint8_t( ManipulationMode::TranslateX );
                        manipulationAxis = LineSegment( originWS, originWS + axisInfo[axisIndex].m_axisDirWS );
                    }
                    else // Plane manipulation
                    {
                        uint32_t const planeIndex = uint8_t( m_manipulationMode ) - uint8_t( ManipulationMode::TranslateXY );
                        manipulationPlane = Plane::FromNormalAndPoint( planeInfo[planeIndex].m_manipulationPlaneAxis, originWS );
                    }

                    // Calculate delta
                    Vector projectedPoint;
                    if ( manipulationPlane.IntersectLine( mouseWorldRay.GetStartPoint(), mouseWorldRay.GetEndPoint(), projectedPoint ) )
                    {
                        if ( !isPlaneManipulatorHovered )
                        {
                            projectedPoint = manipulationAxis.GetClosestPointOnLine( projectedPoint );
                        }

                        result.m_deltaPositionScale = projectedPoint - m_translationScaleDeltaOriginWS;
                        m_translationScaleDeltaOriginWS = projectedPoint;
                    }

                    //-------------------------------------------------------------------------

                    result.m_deltaType = ResultDeltaType::Translation;

                    if ( result.m_state == State::None ) // Don't override the start manipulating state
                    {
                        result.m_state = State::Manipulating;
                    }
                }
            }
        }

        // Should we stop manipulating?
        //-------------------------------------------------------------------------

        if ( isManipulating )
        {
            if ( ImGui::IsMouseReleased( ImGuiMouseButton_Left ) )
            {
                m_translationScaleDeltaOriginWS = Vector::Zero;
                m_manipulationMode = ManipulationMode::None;
                result.m_state = State::StoppedManipulating;
            }
        }

        return result;
    }
   
    Gizmo::Result Gizmo::DrawScaleGizmo( Vector const& originWS, Quaternion const& orientationWS, Render::Viewport const& viewport )
    {
        EE_ASSERT( orientationWS.IsNormalized() );

        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        Vector const mousePos( io.MousePos.x, io.MousePos.y, 0, 1.0f );
        bool const isMouseInViewport = viewport.ContainsPointScreenSpace( io.MousePos );

        //-------------------------------------------------------------------------

        bool const isManipulating = m_manipulationMode != ManipulationMode::None;
        EE_ASSERT( !isManipulating || ( m_manipulationMode >= ManipulationMode::ScaleX && m_manipulationMode <= ManipulationMode::ScaleZ ) );

        Vector const originSS = viewport.WorldSpaceToScreenSpace( originWS );

        //-------------------------------------------------------------------------

        struct AxisDrawInfo
        {
            Vector      m_axisDirWS = Vector::Zero;
            Vector      m_scaledAxisDirWS = Vector::Zero;
            Vector      m_axisStartSS = Vector::Zero;
            Vector      m_axisEndSS = Vector::Zero;
            Vector      m_axisDirSS = Vector::Zero;
            uint32_t    m_color = 0;
            float       m_distanceToCamera = FLT_MAX;
            bool        m_isHovered = false;
            bool        m_isAxisFlipped = false;
        };

        float lowestDistanceToCamera = FLT_MAX;
        TInlineVector<AxisDrawInfo, 3> axisInfo;
        axisInfo.resize( 3 );

        // Calculate initial axis screen space points
        //-------------------------------------------------------------------------

        Vector const axesScale( viewport.GetScalingFactorAtPosition( originWS, g_axisLength ) );

        axisInfo[0].m_axisDirWS = orientationWS.RotateVector( Vector::UnitX );
        axisInfo[1].m_axisDirWS = orientationWS.RotateVector( Vector::UnitY );
        axisInfo[2].m_axisDirWS = orientationWS.RotateVector( Vector::UnitZ );

        axisInfo[0].m_axisDirWS.Normalize3();
        axisInfo[1].m_axisDirWS.Normalize3();
        axisInfo[2].m_axisDirWS.Normalize3();

        if ( m_options.IsFlagSet( Options::AllowAxesFlipping ) )
        {
            Vector const viewDirWS = viewport.IsOrthographic() ? viewport.GetViewForwardDirection().GetNegated() : ( viewport.GetViewPosition() - originWS ).GetNormalized3();
            for ( int32_t i = 0; i < 3; i++ )
            {
                if ( axisInfo[i].m_axisDirWS.GetDot3( viewDirWS ) < 0.0f )
                {
                    axisInfo[i].m_axisDirWS.Negate();
                    axisInfo[i].m_isAxisFlipped = true;
                }
            }
        }

        axisInfo[0].m_scaledAxisDirWS = axisInfo[0].m_axisDirWS * axesScale;
        axisInfo[0].m_axisEndSS = viewport.WorldSpaceToScreenSpace( originWS + axisInfo[0].m_scaledAxisDirWS );
        axisInfo[0].m_axisDirSS = ( axisInfo[0].m_axisEndSS - originSS ).GetNormalized2();
        axisInfo[0].m_axisStartSS = originSS + ( axisInfo[0].m_axisDirSS * g_originCircleOffset );
        axisInfo[0].m_isHovered = ( m_manipulationMode == ManipulationMode::ScaleX );
        axisInfo[0].m_distanceToCamera = viewport.GetViewPosition().GetDistance3( originWS + axisInfo[0].m_scaledAxisDirWS );
        lowestDistanceToCamera = Math::Min( lowestDistanceToCamera, axisInfo[0].m_distanceToCamera );

        axisInfo[1].m_scaledAxisDirWS = axisInfo[1].m_axisDirWS * axesScale;
        axisInfo[1].m_axisEndSS = viewport.WorldSpaceToScreenSpace( originWS + axisInfo[1].m_scaledAxisDirWS );
        axisInfo[1].m_axisDirSS = ( axisInfo[1].m_axisEndSS - originSS ).GetNormalized2();
        axisInfo[1].m_axisStartSS = originSS + ( axisInfo[1].m_axisDirSS * g_originCircleOffset );
        axisInfo[1].m_isHovered = ( m_manipulationMode == ManipulationMode::ScaleY );
        axisInfo[1].m_distanceToCamera = viewport.GetViewPosition().GetDistance3( originWS + axisInfo[1].m_scaledAxisDirWS );
        lowestDistanceToCamera = Math::Min( lowestDistanceToCamera, axisInfo[1].m_distanceToCamera );

        axisInfo[2].m_scaledAxisDirWS = axisInfo[2].m_axisDirWS * axesScale;
        axisInfo[2].m_axisEndSS = viewport.WorldSpaceToScreenSpace( originWS + axisInfo[2].m_scaledAxisDirWS );
        axisInfo[2].m_axisDirSS = ( axisInfo[2].m_axisEndSS - originSS ).GetNormalized2();
        axisInfo[2].m_axisStartSS = originSS + ( axisInfo[2].m_axisDirSS * g_originCircleOffset );
        axisInfo[2].m_isHovered = ( m_manipulationMode == ManipulationMode::ScaleZ );
        axisInfo[2].m_distanceToCamera = viewport.GetViewPosition().GetDistance3( originWS + axisInfo[2].m_scaledAxisDirWS );
        lowestDistanceToCamera = Math::Min( lowestDistanceToCamera, axisInfo[2].m_distanceToCamera );

        // Check hover state
        //-------------------------------------------------------------------------

        if ( isMouseInViewport && !isManipulating )
        {
            int32_t numHoveredAxes = 0;

            if ( !axisInfo[0].m_axisStartSS.IsNearEqual3( axisInfo[0].m_axisEndSS, g_hoverDetectionDistance ) )
            {
                LineSegment const axisSegment( axisInfo[0].m_axisStartSS, axisInfo[0].m_axisEndSS );
                float const mouseToLineDistance = axisSegment.GetDistanceFromSegmentToPoint( mousePos );
                axisInfo[0].m_isHovered = mouseToLineDistance < g_hoverDetectionDistance;
                numHoveredAxes += uint8_t( axisInfo[0].m_isHovered );
            }

            if ( !axisInfo[1].m_axisStartSS.IsNearEqual3( axisInfo[1].m_axisEndSS, g_hoverDetectionDistance ) )
            {
                LineSegment const axisSegment( axisInfo[1].m_axisStartSS, axisInfo[1].m_axisEndSS );
                float const mouseToLineDistance = axisSegment.GetDistanceFromSegmentToPoint( mousePos );
                axisInfo[1].m_isHovered = mouseToLineDistance < g_hoverDetectionDistance;
                numHoveredAxes += uint8_t( axisInfo[1].m_isHovered );
            }

            if ( !axisInfo[2].m_axisStartSS.IsNearEqual3( axisInfo[2].m_axisEndSS, g_hoverDetectionDistance ) )
            {
                LineSegment const axisSegment( axisInfo[2].m_axisStartSS, axisInfo[2].m_axisEndSS );
                float const mouseToLineDistance = axisSegment.GetDistanceFromSegmentToPoint( mousePos );
                axisInfo[2].m_isHovered = mouseToLineDistance < g_hoverDetectionDistance;
                numHoveredAxes += uint8_t( axisInfo[2].m_isHovered );
            }

            // Ensure only the closest axis is selected
            //-------------------------------------------------------------------------

            if ( numHoveredAxes > 1 )
            {
                if ( axisInfo[0].m_isHovered && axisInfo[0].m_distanceToCamera > lowestDistanceToCamera )
                {
                    axisInfo[0].m_isHovered = false;
                }

                if ( axisInfo[1].m_isHovered && axisInfo[1].m_distanceToCamera > lowestDistanceToCamera )
                {
                    axisInfo[1].m_isHovered = false;
                }

                if ( axisInfo[2].m_isHovered && axisInfo[2].m_distanceToCamera > lowestDistanceToCamera )
                {
                    axisInfo[2].m_isHovered = false;
                }
            }
        }

        // Draw Widget
        //-------------------------------------------------------------------------

        axisInfo[0].m_color = ( axisInfo[0].m_isHovered ) ? ConvertColor( g_hoveredAxisColorX ) : ConvertColor( g_axisColorX );
        axisInfo[1].m_color = ( axisInfo[1].m_isHovered ) ? ConvertColor( g_hoveredAxisColorY ) : ConvertColor( g_axisColorY );
        axisInfo[2].m_color = ( axisInfo[2].m_isHovered ) ? ConvertColor( g_hoveredAxisColorZ ) : ConvertColor( g_axisColorZ );

        // Sort axes back to front
        TInlineVector<int32_t, 3> drawOrder = { 0, 1, 2 };

        auto Comparator = [&] ( int32_t const& a, int32_t const& b )
        {
            float const va = axisInfo[a].m_distanceToCamera;
            float const vb = axisInfo[b].m_distanceToCamera;
            return va > vb;
        };

        eastl::sort( drawOrder.begin(), drawOrder.end(), Comparator );

        // Draw origin sphere
        pDrawList->AddCircleFilled( originSS.ToFloat2(), g_originCircleWidth, ConvertColor( g_originColor ) );

        // Draw Axes
        for ( auto d : drawOrder )
        {
            pDrawList->AddLine( axisInfo[d].m_axisStartSS, axisInfo[d].m_axisEndSS, axisInfo[d].m_color, g_axisThickness );

            // Draw axis cap
            if ( !isManipulating || axisInfo[d].m_isHovered )
            {
                pDrawList->AddCircleFilled( axisInfo[d].m_axisEndSS, g_axisThickness, axisInfo[d].m_color );
            }
        }

        // User Manipulation
        //-------------------------------------------------------------------------

        Result result;

        if ( isMouseInViewport )
        {
            // Check if we should start manipulating
            //-------------------------------------------------------------------------

            bool const isAxisHovered = axisInfo[0].m_isHovered || axisInfo[1].m_isHovered || axisInfo[2].m_isHovered;
            if ( !isManipulating && isAxisHovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
            {
                LineSegment const mouseWorldRay = viewport.ScreenSpaceToWorldSpace( mousePos );
                LineSegment manipulationAxis( Vector::Zero, Vector::One );
                Plane manipulationPlane = Plane::FromNormalAndPoint( viewport.GetViewForwardDirection(), originWS ); // View plane by default

                if ( axisInfo[0].m_isHovered )
                {
                    manipulationAxis = LineSegment( originWS, originWS + axisInfo[0].m_axisDirWS );
                    m_manipulationMode = ManipulationMode::ScaleX;
                }
                else if ( axisInfo[1].m_isHovered )
                {
                    manipulationAxis = LineSegment( originWS, originWS + axisInfo[1].m_axisDirWS );
                    m_manipulationMode = ManipulationMode::ScaleY;
                }
                else if ( axisInfo[2].m_isHovered )
                {
                    manipulationAxis = LineSegment( originWS, originWS + axisInfo[2].m_axisDirWS );
                    m_manipulationMode = ManipulationMode::ScaleZ;
                }
                else
                {
                    EE_UNREACHABLE_CODE();
                }

                // Project mouse onto manipulation plane
                Vector projectedPoint;
                if ( manipulationPlane.IntersectLine( mouseWorldRay.GetStartPoint(), mouseWorldRay.GetEndPoint(), projectedPoint ) )
                {
                    m_translationScaleDeltaOriginWS = manipulationAxis.GetClosestPointOnLine( projectedPoint );
                }

                result.m_state = State::StartedManipulating;
            }

            // Handle widget manipulation
            //-------------------------------------------------------------------------

            if ( m_manipulationMode != ManipulationMode::None )
            {
                // Set the hovered ID so that we disable any click through in the UI
                ImGui::SetHoveredID( ImGui::GetID( "Gizmo" ) );

                Vector const mouseDelta( io.MouseDelta );
                if ( !mouseDelta.IsNearZero2() )
                {
                    LineSegment const mouseWorldRay = viewport.ScreenSpaceToWorldSpace( mousePos );
                    LineSegment manipulationAxis( Vector::Zero, Vector::One );
                    Plane manipulationPlane = Plane::FromNormalAndPoint( viewport.GetViewForwardDirection(), originWS ); // View plane by default
                    Vector selectMask;
                    bool const isNonUniformScaleAllowed = m_options.IsFlagSet( Options::AllowNonUniformScale );

                    if ( m_manipulationMode == ManipulationMode::ScaleX )
                    {
                        manipulationAxis = LineSegment( originWS, originWS + axisInfo[0].m_axisDirWS );
                        selectMask = isNonUniformScaleAllowed ? Vector::Select1000 : Vector::Select1110;
                    }
                    else if ( m_manipulationMode == ManipulationMode::ScaleY )
                    {
                        manipulationAxis = LineSegment( originWS, originWS + axisInfo[1].m_axisDirWS );
                        selectMask = isNonUniformScaleAllowed ? Vector::Select0100: Vector::Select1110;
                    }
                    else if ( m_manipulationMode == ManipulationMode::ScaleZ )
                    {
                        manipulationAxis = LineSegment( originWS, originWS + axisInfo[2].m_axisDirWS );
                        selectMask = isNonUniformScaleAllowed ? Vector::Select0010: Vector::Select1110;
                    }
                    else
                    {
                        EE_UNREACHABLE_CODE();
                    }

                    // Calculate delta
                    Vector projectedPoint;
                    if ( manipulationPlane.IntersectLine( mouseWorldRay.GetStartPoint(), mouseWorldRay.GetEndPoint(), projectedPoint ) )
                    {
                        projectedPoint = manipulationAxis.GetClosestPointOnLine( projectedPoint );
                        result.m_deltaPositionScale = Vector::Select( Vector::Zero, ( projectedPoint - m_translationScaleDeltaOriginWS ).Length3(), selectMask );

                        // Are we dragging in the positive direction or the negative one?
                        float const dot = ( projectedPoint - m_translationScaleDeltaOriginWS ).GetNormalized3().GetDot3( ( m_translationScaleDeltaOriginWS + manipulationAxis.GetDirection() ).GetNormalized3() );
                        if ( dot < 0 )
                        {
                            result.m_deltaPositionScale.Negate();
                        }

                        m_translationScaleDeltaOriginWS = projectedPoint;
                    }

                    //-------------------------------------------------------------------------

                    result.m_deltaType = isNonUniformScaleAllowed ? ResultDeltaType::NonUniformScale : ResultDeltaType::Scale;

                    if ( result.m_state == State::None ) // Don't override the start manipulating state
                    {
                        result.m_state = State::Manipulating;
                    }
                }
            }
        }

        // Should we stop manipulating?
        //-------------------------------------------------------------------------

        if ( isManipulating )
        {
            if ( ImGui::IsMouseReleased( ImGuiMouseButton_Left ) )
            {
                m_translationScaleDeltaOriginWS = Vector::Zero;
                m_manipulationMode = ManipulationMode::None;
                result.m_state = State::StoppedManipulating;
            }
        }

        return result;
    }

    Gizmo::Result Gizmo::DrawRotationGizmo( Vector const& originWS, Quaternion const& orientationWS, Render::Viewport const& viewport )
    {
        EE_ASSERT( orientationWS.IsNormalized() );

        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        Vector const mousePos( io.MousePos.x, io.MousePos.y, 0, 1.0f );
        bool const isMouseInViewport = viewport.ContainsPointScreenSpace( io.MousePos );

        bool const isManipulating = m_manipulationMode != ManipulationMode::None;
        EE_ASSERT( !isManipulating || ( m_manipulationMode >= ManipulationMode::RotateX && m_manipulationMode <= ManipulationMode::RotateZ ) );

        //-------------------------------------------------------------------------

        Vector const originSS = viewport.WorldSpaceToScreenSpace( originWS );
        Vector const axesScale( viewport.GetScalingFactorAtPosition( originWS, g_axisLength ) );

        // Axes Info
        //-------------------------------------------------------------------------

        constexpr uint32_t const numPlaneManipulationPoints = 20;

        struct AxisDrawInfo
        {
            Vector              m_axisDirWS;
            Vector              m_scaledOrthogonalAxisDirWS;
            int16_t             m_planeStartAxisIdx = InvalidIndex;
            int16_t             m_planeEndAxisIdx = InvalidIndex;
            float               m_distanceToCamera = FLT_MAX;
            uint32_t            m_color = 0;
            ManipulationMode    m_manipulationMode = ManipulationMode::None;
            ImVec2              m_pointsSS[numPlaneManipulationPoints];
            bool                m_isHovered = false;
            bool                m_isAxisFlipped = false;
        };

        TInlineVector<AxisDrawInfo, 3> axisInfo;
        axisInfo.resize( 3 );

        // Set up axes info
        //-------------------------------------------------------------------------

        axisInfo[0].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitX : orientationWS.RotateVector( Vector::UnitX );
        axisInfo[0].m_planeStartAxisIdx = 2;
        axisInfo[0].m_planeEndAxisIdx = 1;
        axisInfo[0].m_manipulationMode = ManipulationMode::RotateX;
        axisInfo[0].m_color = ConvertColor( g_axisColorX );
        axisInfo[0].m_isHovered = ( m_manipulationMode == axisInfo[0].m_manipulationMode );

        axisInfo[1].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitY : orientationWS.RotateVector( Vector::UnitY );
        axisInfo[1].m_planeStartAxisIdx = 2;
        axisInfo[1].m_planeEndAxisIdx = 0;
        axisInfo[1].m_manipulationMode = ManipulationMode::RotateY;
        axisInfo[1].m_color = ConvertColor( g_axisColorY );
        axisInfo[1].m_isHovered = ( m_manipulationMode == axisInfo[1].m_manipulationMode );

        axisInfo[2].m_axisDirWS = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitZ : orientationWS.RotateVector( Vector::UnitZ );
        axisInfo[2].m_planeStartAxisIdx = 1;
        axisInfo[2].m_planeEndAxisIdx = 0;
        axisInfo[2].m_manipulationMode = ManipulationMode::RotateZ;
        axisInfo[2].m_color = ConvertColor( g_axisColorZ );
        axisInfo[2].m_isHovered = ( m_manipulationMode == axisInfo[2].m_manipulationMode );

        if ( m_options.IsFlagSet( Options::AllowAxesFlipping ) )
        {
            Vector const viewDirWS = viewport.IsOrthographic() ? viewport.GetViewForwardDirection().GetNegated() : ( viewport.GetViewPosition() - originWS ).GetNormalized3();
            for ( int32_t i = 0; i < 3; i++ )
            {
                if ( axisInfo[i].m_axisDirWS.GetDot3( viewDirWS ) < 0.0f )
                {
                    axisInfo[i].m_axisDirWS.Negate();
                    axisInfo[i].m_isAxisFlipped = true;
                }
            }
        }

        axisInfo[0].m_axisDirWS.Normalize3();
        axisInfo[1].m_axisDirWS.Normalize3();
        axisInfo[2].m_axisDirWS.Normalize3();

        EE_ASSERT( axisInfo[0].m_axisDirWS.IsNormalized3() );
        EE_ASSERT( axisInfo[1].m_axisDirWS.IsNormalized3() );
        EE_ASSERT( axisInfo[2].m_axisDirWS.IsNormalized3() );

        // Generate points
        //-------------------------------------------------------------------------

        float lowestDistanceToCamera = FLT_MAX;

        if ( !isManipulating )
        {
            constexpr float const deltaAngle = Math::PiDivTwo / ( numPlaneManipulationPoints - 1 );

            for ( auto p = 0; p < 3; p++ )
            {
                Vector const& startAxisWS = axisInfo[axisInfo[p].m_planeStartAxisIdx].m_axisDirWS;
                Vector const& endAxisWS = axisInfo[axisInfo[p].m_planeEndAxisIdx].m_axisDirWS;
                float const rotationDir = ( axisInfo[p].m_axisDirWS.GetDot3( Vector::Cross3( startAxisWS , endAxisWS ) ) < 0 ) ? -1.0f : 1.0f;

                Vector const scaledStartAxis = startAxisWS * axesScale;

                for ( auto i = 0; i < numPlaneManipulationPoints; i++ )
                {
                    Radians const angle( rotationDir * ( deltaAngle * i ) );
                    Quaternion rot( axisInfo[p].m_axisDirWS, angle );
                    Vector pointWS = rot.RotateVector( scaledStartAxis );
                    pointWS += originWS;

                    axisInfo[p].m_distanceToCamera = Math::Min( axisInfo[p].m_distanceToCamera, viewport.GetViewPosition().GetDistance3( pointWS ) );
                    axisInfo[p].m_pointsSS[i] = viewport.WorldSpaceToScreenSpace( pointWS );

                    // Check for hover
                    if ( !axisInfo[p].m_isHovered )
                    {
                        float const distance = mousePos.GetDistance2( axisInfo[p].m_pointsSS[i] );
                        if ( distance < g_hoverDetectionDistance )
                        {
                            axisInfo[p].m_isHovered = true;
                        }
                    }
                }

                lowestDistanceToCamera = Math::Min( lowestDistanceToCamera, axisInfo[p].m_distanceToCamera );
            }
        }

        // Manage Hover State
        //-------------------------------------------------------------------------

        AxisDrawInfo* pHoveredAxisInfo = nullptr;

        if ( isMouseInViewport )
        {
            // Bias selection to closest plane
            int32_t const numHoveredAxes = uint32_t( axisInfo[0].m_isHovered ) + uint32_t( axisInfo[1].m_isHovered ) + uint32_t( axisInfo[2].m_isHovered );
            if ( numHoveredAxes > 1 )
            {
                if ( axisInfo[0].m_isHovered && axisInfo[0].m_distanceToCamera > lowestDistanceToCamera )
                {
                    axisInfo[0].m_isHovered = false;
                }

                if ( axisInfo[1].m_isHovered && axisInfo[1].m_distanceToCamera > lowestDistanceToCamera )
                {
                    axisInfo[1].m_isHovered = false;
                }

                if ( axisInfo[2].m_isHovered && axisInfo[2].m_distanceToCamera > lowestDistanceToCamera )
                {
                    axisInfo[2].m_isHovered = false;
                }
            }

            if ( axisInfo[0].m_isHovered )
            {
                EE_ASSERT( pHoveredAxisInfo == nullptr );
                pHoveredAxisInfo = &axisInfo[0];
            }

            if ( axisInfo[1].m_isHovered )
            {
                EE_ASSERT( pHoveredAxisInfo == nullptr );
                pHoveredAxisInfo = &axisInfo[1];
            }

            if ( axisInfo[2].m_isHovered )
            {
                EE_ASSERT( pHoveredAxisInfo == nullptr );
                pHoveredAxisInfo = &axisInfo[2];
            }
        }

        axisInfo[0].m_color = ( axisInfo[0].m_isHovered ) ? ConvertColor( g_hoveredAxisColorX ) : ConvertColor( g_axisColorX );
        axisInfo[1].m_color = ( axisInfo[1].m_isHovered ) ? ConvertColor( g_hoveredAxisColorY ) : ConvertColor( g_axisColorY );
        axisInfo[2].m_color = ( axisInfo[2].m_isHovered ) ? ConvertColor( g_hoveredAxisColorZ ) : ConvertColor( g_axisColorZ );

        // Draw Widgets
        //-------------------------------------------------------------------------

        // Draw origin
        pDrawList->AddCircleFilled( originSS.ToFloat2(), g_originCircleWidth, ConvertColor( g_originColor ) );

        // Draw preview
        if ( isManipulating )
        {
            constexpr uint32_t const numCirclePoints = 100;
            constexpr uint32_t const numPreviewCirclePoints = numCirclePoints / 2;
            ImVec2 previewCirclePointsSS[numCirclePoints + 1];

            Vector const scaledRotationStartDirection = ( m_rotationStartDirection * axesScale );
            float const angleStepDelta = Math::TwoPi / ( numCirclePoints - 1 );
            for ( uint32_t i = 0; i < numCirclePoints; i++ )
            {
                Quaternion deltaRot = Quaternion( pHoveredAxisInfo->m_axisDirWS, angleStepDelta * (float) i );
                Vector pointOnCircle_WS = originWS + ( deltaRot.RotateVector( scaledRotationStartDirection ) );
                Float2 pointOnCircle_SS = viewport.WorldSpaceToScreenSpace( pointOnCircle_WS );
                previewCirclePointsSS[i] = pointOnCircle_SS;
            }

            pDrawList->AddPolyline( previewCirclePointsSS, numCirclePoints, pHoveredAxisInfo->m_color, false, 4.0f );

            //-------------------------------------------------------------------------

            uint32_t const previewColor = ConvertColor( g_rotationPreviewColor );
            Vector textPositionOnCircleWS = originWS + ( m_rotationStartDirection * axesScale );
            if ( Math::Abs( m_rotationDeltaAngle.ToFloat() ) > Math::DegreesToRadians )
            {
                float const rotationDir = pHoveredAxisInfo->m_isAxisFlipped ? -1.0f : 1.0f;
                float const previewAngleStepDelta = m_rotationDeltaAngle.ToFloat() / ( numPreviewCirclePoints - 1 ) * rotationDir;
                for ( uint32_t i = 0; i < numPreviewCirclePoints; i++ )
                {
                    Quaternion deltaRot = Quaternion( pHoveredAxisInfo->m_axisDirWS, previewAngleStepDelta * (float) i );
                    textPositionOnCircleWS = originWS + ( deltaRot.RotateVector( scaledRotationStartDirection ) );
                    Float2 pointOnCircle_SS = viewport.WorldSpaceToScreenSpace( textPositionOnCircleWS );
                    previewCirclePointsSS[i] = pointOnCircle_SS;
                }

                
                pDrawList->AddPolyline( previewCirclePointsSS, numPreviewCirclePoints, previewColor, false, 8.0f );
                pDrawList->AddLine( originSS, previewCirclePointsSS[numPreviewCirclePoints - 1], previewColor, 1.0f );
                pDrawList->AddLine( originSS, previewCirclePointsSS[0], ConvertColor( Colors::White ), 1.0f );
            }

            //-------------------------------------------------------------------------

            static char buff[16];
            Printf( buff, 255, " %.2f", (float) m_rotationDeltaAngle.ToDegrees() );

            Vector const pointOnCircle_WS = textPositionOnCircleWS;
            Float2 const pointOnCircle_SS = viewport.WorldSpaceToScreenSpace( pointOnCircle_WS );

            Vector const axisLineEnd_WS = originWS + ( pHoveredAxisInfo->m_axisDirWS );
            Vector const axisLineEnd_SS = viewport.WorldSpaceToScreenSpace( axisLineEnd_WS );
            Float2 axisDirSS = ( axisLineEnd_SS - originSS ).GetNormalized2();

            pDrawList->AddLine( originSS - ( axisDirSS * 10000 ), originSS + ( axisDirSS * 10000 ), pHoveredAxisInfo->m_color );
            pDrawList->AddLine( originSS, pointOnCircle_SS, ConvertColor( Colors::White ), 1.0f );
            pDrawList->AddCircleFilled( pointOnCircle_SS, 3.0f, previewColor );

            auto textSize = ImGui::CalcTextSize( buff );
            Vector const textPosition = Vector( pointOnCircle_SS ) - Float2( textSize.x / 2, 0 );
            pDrawList->AddRectFilled( textPosition, textPosition + Float2( textSize.x + 3, textSize.y ), ConvertColor( Colors::Black.GetAlphaVersion( 0.5f ) ), 3.0f );
            pDrawList->AddText( textPosition, ConvertColor( Colors::White ), buff );
        }
        else // Draw manipulation handles
        {
            // Sort axes back to front
            TInlineVector<int32_t, 6> drawOrder = { 0, 1, 2 };

            auto Comparator = [&] ( int32_t const& a, int32_t const& b )
            {
                return axisInfo[a].m_distanceToCamera > axisInfo[b].m_distanceToCamera;
            };

            eastl::sort( drawOrder.begin(), drawOrder.end(), Comparator );

            // Draw curves
            for ( auto d : drawOrder )
            {
                pDrawList->AddPolyline( axisInfo[d].m_pointsSS, numPlaneManipulationPoints, axisInfo[d].m_color, 0, g_axisThickness );
            }
        }

        // User Manipulation
        //-------------------------------------------------------------------------

        Result result;

        if ( isMouseInViewport )
        {
            // Check if we should start manipulating
            //-------------------------------------------------------------------------

            bool const isAxisHovered = axisInfo[0].m_isHovered || axisInfo[1].m_isHovered || axisInfo[2].m_isHovered;
            if ( !isManipulating && isAxisHovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
            {
                LineSegment const mouseWorldRay = viewport.ScreenSpaceToWorldSpace( mousePos );
                LineSegment manipulationAxis( Vector::Zero, Vector::One );
                Plane rotationPlane = Plane::FromNormalAndPoint( viewport.GetViewForwardDirection(), originWS ); // View plane by default

                m_originalStartRotation = orientationWS;

                if ( axisInfo[0].m_isHovered )
                {
                    m_rotationAxis = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitX : axisInfo[0].m_axisDirWS;
                    rotationPlane = Plane::FromNormalAndPoint( m_rotationAxis, originWS );
                    m_manipulationMode = ManipulationMode::RotateX;
                }
                else if ( axisInfo[1].m_isHovered )
                {
                    m_rotationAxis = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitY : axisInfo[1].m_axisDirWS;
                    rotationPlane = Plane::FromNormalAndPoint( m_rotationAxis, originWS );
                    m_manipulationMode = ManipulationMode::RotateY;
                }
                else if ( axisInfo[2].m_isHovered )
                {
                    m_rotationAxis = ( m_coordinateSpace == CoordinateSpace::World ) ? Vector::UnitZ : axisInfo[2].m_axisDirWS;
                    rotationPlane = Plane::FromNormalAndPoint( m_rotationAxis, originWS );
                    m_manipulationMode = ManipulationMode::RotateZ;
                }
                else
                {
                    EE_UNREACHABLE_CODE();
                }

                // Project mouse onto manipulation plane
                Vector projectedPointWS;
                if ( rotationPlane.IntersectLine( mouseWorldRay.GetStartPoint(), mouseWorldRay.GetEndPoint(), projectedPointWS ) )
                {
                    m_rotationStartDirection = ( projectedPointWS - originWS ).GetNormalized3();
                    EE_ASSERT( !m_rotationStartDirection.IsNearZero3() );
                    result.m_state = State::StartedManipulating;
                }
                else // Projection failed - should never happen
                {
                    m_manipulationMode = ManipulationMode::None;
                }
            }

            // Handle widget manipulation
            //-------------------------------------------------------------------------

            if ( m_manipulationMode != ManipulationMode::None )
            {
                EE_ASSERT( !m_rotationStartDirection.IsNearZero3() );

                // Set the hovered ID so that we disable any click through in the UI
                ImGui::SetHoveredID( ImGui::GetID( "Gizmo" ) );

                Vector const mouseDelta( io.MouseDelta );
                if ( !mouseDelta.IsNearZero2() )
                {
                    // Project mouse into rotation plane and calculate desired end direction
                    LineSegment const mouseSegment = viewport.ScreenSpaceToWorldSpace( mousePos );
                    Plane const rotationPlane = Plane::FromNormalAndPoint( m_rotationAxis, originWS );

                    Vector intersectionPointWS;
                    rotationPlane.IntersectLine( mouseSegment, intersectionPointWS );
                    Vector const desiredDir = ( intersectionPointWS - originWS ).GetNormalized3();

                    // Calculate the desired angle required relative to the original start orientation
                    m_rotationDeltaAngle = Math::GetAngleBetweenVectors( m_rotationStartDirection, desiredDir );
                    bool const isRotationDirectionPositive = Vector::Dot3( Vector::Cross3( m_rotationStartDirection, desiredDir ).GetNormalized3(), m_rotationAxis ).ToFloat() > 0.0f;
                    if ( !isRotationDirectionPositive )
                    {
                        m_rotationDeltaAngle = -m_rotationDeltaAngle;
                    }

                    // Calculate the final delta orientation relative to the original orientation when we started the manipulation
                    Quaternion const desiredOrientationWS = m_originalStartRotation * Quaternion( m_rotationAxis, m_rotationDeltaAngle );
                    result.m_deltaOrientation = Quaternion::Delta( orientationWS, desiredOrientationWS ).GetNormalized();
                    result.m_deltaType = ResultDeltaType::Rotation;

                    if ( result.m_state == State::None ) // Don't override the start manipulating state
                    {
                        result.m_state = State::Manipulating;
                    }
                }
            }
        }

        // Should we stop manipulating?
        //-------------------------------------------------------------------------

        if ( isManipulating )
        {
            if ( ImGui::IsMouseReleased( ImGuiMouseButton_Left ) )
            {
                m_originalStartRotation = Quaternion::Identity;
                m_rotationStartDirection = Vector::Zero;
                m_rotationAxis = Vector::Zero;
                m_rotationDeltaAngle = 0.0f;
                m_manipulationMode = ManipulationMode::None;
                result.m_state = State::StoppedManipulating;
            }
        }

        return result;
    }
}
#endif