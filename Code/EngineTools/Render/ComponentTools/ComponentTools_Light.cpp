#include "EngineTools/Entity/EntityEditor/EntityEditor_ComponentTools.h"
#include "EngineTools/Entity/EntityEditor/EntityEditor_Context.h"
#include "Engine/Render/Components/Component_Lights.h"
#include "Engine/Viewport/Viewport.h"
#include "Base/Imgui/EsotericaIcons.h"

namespace EE::Render
{
    //-------------------------------------------------------------------------

    namespace LightEditor
    {
        static float const          s_directionalArrowLength = 4.0F;
        static float const          s_directionalRingRadius = 4.0F;
        static float const          s_lineThickness = 4.0F;
        static float const          s_primaryThickness = 4.0F;
        static float const          s_editingThickness = 1.0F;
        static float const          s_secondaryAlpha = 0.35F;
        static float const          s_handleDotThickness = 30.0F;

        //-------------------------------------------------------------------------

        static Color const          s_neutralColor = Color( 255, 153, 51 );
        static Color const          s_hoveredColor = Color( 61, 224, 133 );
        static Color const          s_editingColor = Color( 0, 200, 255 );
        static Color const          s_secondaryColor = Colors::WhiteSmoke;

        //-------------------------------------------------------------------------

        static uint64_t const       s_gizmoHitTestID = 0xFFFF0001;
        static uint64_t const       s_falloffGizmoHitTestID = 0xFFFF0002;
        static uint64_t const       s_beamGizmoHitTestID = 0xFFFF0003;
        static uint64_t const       s_blendGizmoHitTestID = 0xFFFF0004;

        //-------------------------------------------------------------------------

        static float const          s_falloffAnchor = 0.25F;

        // Reasonable ranges for how far the handle is allowed to travel for comfortable editing
        //-------------------------------------------------------------------------

        static float const          s_maxFalloff = 50.0F;
        static float const          s_minBlend = 0.05F;
        static float const          s_maxBlend = 0.9F;

        //-------------------------------------------------------------------------

        static int32_t const        s_coneRimSegments = 48;
        static int32_t const        s_coneSolveSegments = 16;
        static int32_t const        s_coneSolveIterations = 16;
        static float const          s_coneTruncationScale = 0.12F;          // Fraction of the rim distance where the side lines start, keeping the apex clear

        //-------------------------------------------------------------------------

        enum class LightHandle
        {
            None,
            Range,
            Falloff,
            BeamAngle,
            Blend
        };

        static Color GetHandleColor( bool isHovered, bool isBeingEdited )
        {
            if ( isBeingEdited )
            {
                return s_editingColor;
            }

            return isHovered ? s_hoveredColor : s_neutralColor;
        }

        static float GetHandleThickness( float thickness, bool isManipulating )
        {
            return isManipulating ? s_editingThickness : thickness;
        }

        // Dot on a circular handle to make it easier to grab
        static void DrawHandleDot( DebugDrawContext& drawingCtx, Viewport const& viewport, Vector const& centre, Vector const& planeNormal, float radius, Vector const& fallbackDirection, Color const& color, bool isManipulating )
        {
            if ( isManipulating )
            {
                return;
            }

            Vector const viewUp = viewport.GetViewUpDirection().GetNormalized3();
            Vector inPlane = viewUp - ( planeNormal * viewUp.GetDot3( planeNormal ) );

            if ( inPlane.GetLengthSquared3() <= Math::Epsilon )
            {
                inPlane = fallbackDirection;
            }

            drawingCtx.DrawPoint( centre + ( inPlane.GetNormalized3() * radius ), color, s_handleDotThickness, DebugDrawLayer::Screen );
        }

        struct DragState
        {
            LightHandle m_handle = LightHandle::None;
            int32_t     m_side = 0;
            Vector      m_sideRadials[2];
            float       m_viewDepth = 1.0F;
            float       m_startRadius = 0.0F;
            float       m_radiusOffsetScreenSpace = 0.0F;
            float       m_scalePerMeterScreenSpace = 0.0F;
            float       m_angleOffset = 0.0F;
        };

        // Falloff Handle
        //-------------------------------------------------------------------------
        // The falloff handle is the sphere at which the light has dropped to s_falloffAnchor of its peak intensity.
        // Its position is derived from the renderer attenuation curve ( AttenuationNoCusp, PBR.esh ):
        //
        //     I( d ) / maxIntensity  =  ( 1 - s^2 )^2 / ( 1 + falloff * s^2 ),   s = d / maxRadius
        //
        // Writing u = s^2 and a = s_falloffAnchor, the anchor condition is
        //
        //     ( 1 - u )^2 = a * ( 1 + falloff * u )
        //
        // which rearranges to EITHER
        //
        //     u^2 - ( 2 + a * falloff ) * u + ( 1 - a ) = 0            ( solve for u given falloff )
        //
        // or, collecting the falloff instead,
        //
        //     falloff = ( ( 1 - u )^2 - a ) / ( a * u )                ( solve for falloff given u )
        //
        // Both are used below.

        static float GetFalloffAnchorRadius( float falloff, float maxRadius )
        {
            if ( !Math::IsFinite( falloff ) || falloff < 0.0F || !( maxRadius > 0.0F ) )
            {
                return 0.0F;
            }

            // The anchor is the smaller root: the larger one is >= 1, i.e. outside the influence radius, where there is no light left.
            // It is taken in the cancellation-free form, since for steep falloffs the quadratic formula subtracts nearly equal numbers
            float const b = 2.0F + ( s_falloffAnchor * falloff );
            float const discriminant = Math::Max( ( b * b ) - ( 4.0F * ( 1.0F - s_falloffAnchor ) ), 0.0F );
            float const sSquared = ( 2.0F * ( 1.0F - s_falloffAnchor ) ) / ( b + Math::Sqrt( discriminant ) );

            if ( !Math::IsFinite( sSquared ) || sSquared <= 0.0F )
            {
                return 0.0F;
            }

            return maxRadius * Math::Sqrt( sSquared );
        }

        // The largest radius the handle can reach
        static float GetMaxFalloffAnchorRadius( float maxRadius )
        {
            return GetFalloffAnchorRadius( 0.0F, maxRadius );
        }

        // Inverse of the above: the falloff that puts the anchor at the requested radius
        static bool TryGetFalloffAtRadius( float radius, float maxRadius, float& outFalloff )
        {
            if ( !( maxRadius > 0.0F ) || !( radius > 0.0F ) || radius >= maxRadius )
            {
                return false;
            }

            float const s = radius / maxRadius;
            float const sSquared = s * s;

            outFalloff = ( Math::Sqr( 1.0F - sSquared ) - s_falloffAnchor ) / ( s_falloffAnchor * sSquared );

            return Math::IsFinite( outFalloff );
        }

        static float GetEditableFalloff( float falloff )
        {
            return Math::Clamp( falloff, 0.0F, s_maxFalloff );
        }

        static float GetEditableBlend( float blend )
        {
            return Math::Clamp( blend, s_minBlend, s_maxBlend );
        }

        static uint64_t GetPickedHitTestID( Viewport const& viewport )
        {
            PickingData const& pickingData = viewport.GetPickingData();
            return pickingData.empty() ? 0 : pickingData.front().m_primaryID;
        }

        static LightHandle GetPickedHandle( uint64_t pickedHitTestID )
        {
            if ( pickedHitTestID == s_gizmoHitTestID )
            {
                return LightHandle::Range;
            }

            if ( pickedHitTestID == s_falloffGizmoHitTestID )
            {
                return LightHandle::Falloff;
            }

            if ( pickedHitTestID == s_beamGizmoHitTestID )
            {
                return LightHandle::BeamAngle;
            }

            if ( pickedHitTestID == s_blendGizmoHitTestID )
            {
                return LightHandle::Blend;
            }

            return LightHandle::None;
        }

        // Handle Dragging
        //-------------------------------------------------------------------------

        static bool TryGrabHandle( LightHandle handle, LightHandle hoveredHandle )
        {
            return ImGui::IsMouseClicked( ImGuiMouseButton_Left ) && ( hoveredHandle == handle );
        }

        static bool IsHandleDragActive( DragState& drag, LightHandle handle )
        {
            if ( drag.m_handle != handle )
            {
                return false;
            }

            if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
            {
                drag.m_handle = LightHandle::None;
                return false;
            }

            return true;
        }

        static void UpdateHandleContextEdit( EntityModel::EditorContext& context, EntityComponent* pComponent, bool wasManipulating, bool isManipulating )
        {
            if ( wasManipulating == isManipulating )
            {
                return;
            }

            if ( isManipulating )
            {
                context.BeginEdit( { context.GetWorld()->FindEntity( pComponent->GetEntityID() ) } );
            }
            else
            {
                context.EndEdit();
            }
        }

    }

    // Point Light Editor
    //-------------------------------------------------------------------------

    class PointLightComponentTools final : public EntityModel::TComponentTools<PointLightComponent>
    {
    public:

        using TComponentTools::TComponentTools;

        virtual bool HasViewportSelectionWidget() const override { return true; }
        char const* GetViewportSelectionWidgetIcon() const override { return EE_ICON_EDITOR_POINTLIGHT; }
        Color GetViewportSelectionWidgetColor() const override { return Colors::Gold; }

        virtual bool DrawViewportEditor( EntityModel::EditorContext &context, Viewport const* pViewport, bool isFocused ) override
        {
            if ( pViewport == nullptr )
            {
                return false;
            }

            uint64_t const pickedHitTestID = LightEditor::GetPickedHitTestID( *pViewport );

            Vector const position = m_pComponent->GetPosition();
            float const maxRadius = Math::Max( m_pComponent->GetMaxRadius(), 0.001F );
            float const falloffAnchorRadius = LightEditor::GetFalloffAnchorRadius( LightEditor::GetEditableFalloff( m_pComponent->GetFalloff() ), maxRadius );

            LightEditor::LightHandle const hoveredHandle = LightEditor::GetPickedHandle( pickedHitTestID );

            DrawPointLight( context, *pViewport, position, hoveredHandle, maxRadius, falloffAnchorRadius );

            bool const wasManipulating = ( m_drag.m_handle != LightEditor::LightHandle::None );
            bool const isManipulating = UpdateFalloffDrag( pViewport, hoveredHandle ) || UpdateRangeDrag( pViewport, hoveredHandle );

            LightEditor::UpdateHandleContextEdit( context, m_pComponent, wasManipulating, isManipulating );

            return isManipulating;
        }

    private:

        // Screen space radius a camera-facing circle of the given world radius projects to
        static float MeasureCircleRadiusScreenSpace( Viewport const& viewport, Vector const& worldPosition, float worldRadius )
        {
            Float2 const centreScreenSpace = viewport.WorldSpaceToScreenSpace( worldPosition );
            Float2 const edgeScreenSpace = viewport.WorldSpaceToScreenSpace( worldPosition + ( viewport.GetViewRightDirection().GetNormalized3() * worldRadius ) );

            Vector directionScreenSpace;
            float radiusScreenSpace = 0.0F;
            Vector( edgeScreenSpace - centreScreenSpace ).ToDirectionAndLength2( directionScreenSpace, radiusScreenSpace );

            return radiusScreenSpace;
        }

        // Distance from a world position projected centre to the cursor, in pixels
        static float MeasureCursorRadiusScreenSpace( Viewport const& viewport, Vector const& worldPosition )
        {
            Vector directionScreenSpace;
            float radiusScreenSpace = 0.0F;
            Vector( ImGui::GetMousePos() - viewport.WorldSpaceToScreenSpace( worldPosition ) ).ToDirectionAndLength2( directionScreenSpace, radiusScreenSpace );

            return radiusScreenSpace;
        }

        static void BeginCircleDrag( LightEditor::DragState& drag, Viewport const& viewport, Vector const& position, LightEditor::LightHandle handle, float startRadius, float cursorRadiusScreenSpace )
        {
            float const startRimDistanceScreenSpace = MeasureCircleRadiusScreenSpace( viewport, position, startRadius );

            drag.m_handle = handle;
            drag.m_viewDepth = Math::Max( position.GetDistance3( viewport.GetViewVolume().GetViewPosition() ), 0.001F );
            drag.m_startRadius = startRadius;
            drag.m_radiusOffsetScreenSpace = startRimDistanceScreenSpace - cursorRadiusScreenSpace;
            drag.m_scalePerMeterScreenSpace = ( startRadius > 0.0F ) ? ( startRimDistanceScreenSpace / startRadius ) : 0.0F;
        }

        static float GetCircleDragRadius( LightEditor::DragState const& drag, float cursorRadiusScreenSpace )
        {
            if ( drag.m_scalePerMeterScreenSpace <= 0.0F )
            {
                return 0.0F;
            }

            return Math::Max( ( cursorRadiusScreenSpace + drag.m_radiusOffsetScreenSpace ) / drag.m_scalePerMeterScreenSpace, 0.0F );
        }

        // Two camera-facing circles, one per editable property:
        //
        //  * The influence radius                                              - the radius handle
        //  * The radius at which intensity has fallen to the falloff anchor    - the falloff handle
        //-------------------------------------------------------------------------

        void DrawPointLight( EntityModel::EditorContext& context, Viewport const& viewport, Vector const& position, LightEditor::LightHandle hoveredHandle, float maxRadius, float falloffAnchorRadius )
        {
            auto drawingCtx = context.GetDebugDrawContext();

            Vector const viewDirection = viewport.GetViewForwardDirection().GetNormalized3();
            Vector const viewRightDirection = viewport.GetViewRightDirection().GetNormalized3();

            bool const isManipulating = ( m_drag.m_handle != LightEditor::LightHandle::None );

            Color const rangeColor = LightEditor::GetHandleColor( hoveredHandle == LightEditor::LightHandle::Range, m_drag.m_handle == LightEditor::LightHandle::Range );
            Color const falloffColor = LightEditor::GetHandleColor( hoveredHandle == LightEditor::LightHandle::Falloff, m_drag.m_handle == LightEditor::LightHandle::Falloff );

            Transform const circleTransform( Quaternion::FromRotationBetweenUnitVectors( Vector::UnitZ, viewDirection ), position );

            // Influence radius handle
            drawingCtx.SetHitTestID( LightEditor::s_gizmoHitTestID );
            drawingCtx.DrawCircle( circleTransform, Axis::Z, maxRadius, rangeColor, LightEditor::GetHandleThickness( LightEditor::s_lineThickness, isManipulating ), DebugDrawLayer::Screen );
            LightEditor::DrawHandleDot( drawingCtx, viewport, position, viewDirection, maxRadius, viewRightDirection, rangeColor, isManipulating );
            drawingCtx.ClearHitTestID();

            // Falloff anchor handle.
            if ( falloffAnchorRadius > 0.0F )
            {
                drawingCtx.SetHitTestID( LightEditor::s_falloffGizmoHitTestID );
                drawingCtx.DrawCircle( circleTransform, Axis::Z, falloffAnchorRadius, falloffColor, LightEditor::GetHandleThickness( LightEditor::s_primaryThickness, isManipulating ), DebugDrawLayer::Screen );
                LightEditor::DrawHandleDot( drawingCtx, viewport, position, viewDirection, falloffAnchorRadius, viewRightDirection, falloffColor, isManipulating );
                drawingCtx.ClearHitTestID();
            }
        }

        //-------------------------------------------------------------------------

        bool UpdateRangeDrag( Viewport const* pViewport, LightEditor::LightHandle hoveredHandle )
        {
            Vector const position = m_pComponent->GetPosition();
            float const cursorRadiusScreenSpace = MeasureCursorRadiusScreenSpace( *pViewport, position );

            if ( m_drag.m_handle == LightEditor::LightHandle::None )
            {
                if ( !LightEditor::TryGrabHandle( LightEditor::LightHandle::Range, hoveredHandle ) )
                {
                    return false;
                }

                BeginCircleDrag( m_drag, *pViewport, position, LightEditor::LightHandle::Range, m_pComponent->GetMaxRadius(), cursorRadiusScreenSpace );
                return true;
            }

            if ( !LightEditor::IsHandleDragActive( m_drag, LightEditor::LightHandle::Range ) )
            {
                return false;
            }

            float const radiusCap = Math::Max( m_drag.m_viewDepth * 0.99F, m_drag.m_startRadius );

            m_pComponent->SetMaxRadius( Math::Min( GetCircleDragRadius( m_drag, cursorRadiusScreenSpace ), radiusCap ) );

            return true;
        }

        //-------------------------------------------------------------------------

        bool UpdateFalloffDrag( Viewport const* pViewport, LightEditor::LightHandle hoveredHandle )
        {
            Vector const position = m_pComponent->GetPosition();
            float const cursorRadiusScreenSpace = MeasureCursorRadiusScreenSpace( *pViewport, position );

            float const maxRadius = Math::Max( m_pComponent->GetMaxRadius(), 0.001F );

            if ( m_drag.m_handle == LightEditor::LightHandle::None )
            {
                if ( !LightEditor::TryGrabHandle( LightEditor::LightHandle::Falloff, hoveredHandle ) )
                {
                    return false;
                }

                float const falloffRadius = LightEditor::GetFalloffAnchorRadius( LightEditor::GetEditableFalloff( m_pComponent->GetFalloff() ), maxRadius );

                BeginCircleDrag( m_drag, *pViewport, position, LightEditor::LightHandle::Falloff, falloffRadius, cursorRadiusScreenSpace );
                return true;
            }

            if ( !LightEditor::IsHandleDragActive( m_drag, LightEditor::LightHandle::Falloff ) )
            {
                return false;
            }

            float const handleRadius = Math::Min( GetCircleDragRadius( m_drag, cursorRadiusScreenSpace ), LightEditor::GetMaxFalloffAnchorRadius( maxRadius ) );

            float falloff = 0.0F;
            if ( LightEditor::TryGetFalloffAtRadius( handleRadius, maxRadius, falloff ) )
            {
                m_pComponent->SetFalloff( LightEditor::GetEditableFalloff( falloff ) );
            }

            return true;
        }

    private:

        LightEditor::DragState m_drag;
    };

    // Spot Light Editor
    //-------------------------------------------------------------------------

    class SpotLightComponentTools final : public EntityModel::TComponentTools<SpotLightComponent>
    {
    public:

        using TComponentTools::TComponentTools;

        virtual bool HasViewportSelectionWidget() const override { return true; }
        char const* GetViewportSelectionWidgetIcon() const override { return EE_ICON_EDITOR_SPOTLIGHT; }
        Color GetViewportSelectionWidgetColor() const override { return Colors::Orange; }

        virtual bool DrawViewportEditor( EntityModel::EditorContext &context, Viewport const* pViewport, bool isFocused ) override
        {
            if ( pViewport == nullptr )
            {
                return false;
            }

            float const maxRadius = Math::Max( m_pComponent->GetMaxRadius(), 0.001F );
            float const falloffAnchorRadius = LightEditor::GetFalloffAnchorRadius( LightEditor::GetEditableFalloff( m_pComponent->GetFalloff() ), maxRadius );

            SpotCone cone;
            BuildSpotCone( *pViewport, m_pComponent, falloffAnchorRadius, cone );

            // While the beam drag is in progress the cone keeps the azimuths the line was grabbed in.
            //
            // The side line azimuth is derived from the beam angle and the beam drag solves for that same angle, so solving against a moving azimuth creates a feedback loop.
            // Near the beam angle the azimuth is ill conditioned and this feedback loop gets rather violent.
            if ( m_drag.m_handle == LightEditor::LightHandle::BeamAngle )
            {
                cone.m_radials[0] = m_drag.m_sideRadials[0];
                cone.m_radials[1] = m_drag.m_sideRadials[1];
            }

            uint64_t const pickedHitTestID = LightEditor::GetPickedHitTestID( *pViewport );
            LightEditor::LightHandle const hoveredHandle = LightEditor::GetPickedHandle( pickedHitTestID );

            DrawSpotLight( context, *pViewport, cone, hoveredHandle );

            bool const wasManipulating = ( m_drag.m_handle != LightEditor::LightHandle::None );
            bool const isManipulating = UpdateSpotDrag( pViewport, cone, hoveredHandle );

            LightEditor::UpdateHandleContextEdit( context, m_pComponent, wasManipulating, isManipulating );

            return isManipulating;
        }

    private:
        //-------------------------------------------------------------------------

        // The cone is undefined at a half angle of 90 degrees and 0 degrees, cap them at reasonable ranges.
        static constexpr float const MinBeamAngle = 1.0F;
        static constexpr float const MaxBeamAngle = 178.0F;

    private:

        //-------------------------------------------------------------------------

        struct SpotCone
        {
            Vector          m_apex = {};
            Vector          m_axis = {};
            Vector          m_right = {};
            Vector          m_up = {};
            Vector          m_radials[2] = {};
            float           m_beamHalfAngle = 0.0F;
            float           m_blendHalfAngle = 0.0F;
            float           m_rangeRadius = 0.0F;
            float           m_falloffRadius = 0.0F;
            float           m_sinAxisToView = 1.0F;
        };

    private:

        //-------------------------------------------------------------------------

        // Circle that the cone cuts out of the sphere of the given radius.
        static void GetConeRim( SpotCone const& cone, float lightRadius, float halfAngle, Vector& outCentre, float& outRadius )
        {
            outCentre = cone.m_apex + ( cone.m_axis * ( lightRadius * Math::Cos( halfAngle ) ) );
            outRadius = lightRadius * Math::Sin( halfAngle );
        }

        // Point on the circle above
        static Vector GetConeRimPoint( SpotCone const& cone, float lightRadius, float halfAngle, float angle )
        {
            Vector rimCentre;
            float rimRadius = 0.0F;
            GetConeRim( cone, lightRadius, halfAngle, rimCentre, rimRadius );

            return rimCentre + ( ( ( cone.m_right * Math::Cos( angle ) ) + ( cone.m_up * Math::Sin( angle ) ) ) * rimRadius );
        }

        static void BuildSpotCone( Viewport const& viewport, SpotLightComponent const* pLight, float falloffAnchorRadius, SpotCone& outCone )
        {
            outCone.m_apex = pLight->GetPosition();
            outCone.m_axis = -pLight->GetLightDirection().GetNormalized3();

            Vector reference = Vector::WorldUp;
            if ( Math::Abs( outCone.m_axis.GetDot3( reference ) ) > 0.99F )
            {
                reference = Vector::WorldRight;
            }

            outCone.m_right = outCone.m_axis.Cross3( reference ).GetNormalized3();
            outCone.m_up = outCone.m_axis.Cross3( outCone.m_right ).GetNormalized3();

            float const beamHalfAngleDegrees = Math::Clamp( pLight->GetBeamAngle().ToFloat() * 0.5F, MinBeamAngle * 0.5F, MaxBeamAngle * 0.5F );
            outCone.m_beamHalfAngle = beamHalfAngleDegrees * Math::DegreesToRadians;

            outCone.m_blendHalfAngle = outCone.m_beamHalfAngle * ( 1.0F - LightEditor::GetEditableBlend( pLight->GetBlend() ) );
            outCone.m_rangeRadius = Math::Max( pLight->GetMaxRadius(), 0.001F );
            outCone.m_falloffRadius = falloffAnchorRadius;

            // The cone projected edges are its silhouette sides, at the azimuth about the beam where its surface faces the view square on:
            //
            //     cos( azimuth ) = tan( beamHalfAngle ) / tan( A )
            //
            // with A the angle between the beam and the view direction.
            Vector const viewDirection = viewport.GetViewForwardDirection().GetNormalized3();
            float const cosAxisToView = outCone.m_axis.GetDot3( viewDirection );

            outCone.m_sinAxisToView = Math::Sqrt( Math::Max( 1.0F - ( cosAxisToView * cosAxisToView ), 0.0F ) );

            float const sinBeam = Math::Sin( outCone.m_beamHalfAngle );
            float const cosBeam = Math::Cos( outCone.m_beamHalfAngle );
            float const tanBeam = sinBeam / Math::Max( cosBeam, Math::Epsilon );
            float const tanAxisToView = outCone.m_sinAxisToView / Math::Max( Math::Abs( cosAxisToView ), Math::Epsilon );
            float const azimuthRatio = tanBeam / Math::Max( tanAxisToView, Math::Epsilon );
            float const cosAzimuth = Math::Min( azimuthRatio, 1.0F / Math::Max( azimuthRatio, Math::Epsilon ) );
            float const sinAzimuth = Math::Sqrt( Math::Max( 1.0F - ( cosAzimuth * cosAzimuth ), 0.0F ) );

            Vector inPlane = viewDirection - ( outCone.m_axis * cosAxisToView );
            if ( inPlane.GetLengthSquared3() > Math::Epsilon )
            {
                inPlane = inPlane.GetNormalized3();
            }
            else
            {
                inPlane = outCone.m_right;
            }

            Vector const acrossPlane = outCone.m_axis.Cross3( inPlane ).GetNormalized3();

            for ( int32_t i = 0; i < 2; i++ )
            {
                float const sideSign = ( i == 0 ) ? -1.0F : 1.0F;

                outCone.m_radials[i] = ( inPlane * cosAzimuth ) + ( acrossPlane * ( sinAzimuth * sideSign ) );
            }
        }

        // The side line at the given half angle, down one of the cone two edge azimuths.
        static Vector GetConeSideDirection( SpotCone const& cone, int32_t side, float halfAngle )
        {
            return ( cone.m_axis * Math::Cos( halfAngle ) ) + ( cone.m_radials[side] * Math::Sin( halfAngle ) );
        }

        // Point where a side line crosses the rim own plane.
        static Vector GetConeSideEndPoint( SpotCone const& cone, int32_t side, float halfAngle )
        {
            float const rimDistance = cone.m_rangeRadius * Math::Cos( cone.m_beamHalfAngle );

            return cone.m_apex + ( cone.m_axis * rimDistance ) + ( cone.m_radials[side] * ( rimDistance * Math::Tan( halfAngle ) ) );
        }

        // The screen angle of a side line, signed, measured from the projected axis
        static float MeasureSideAngleScreenSpace( Viewport const& viewport, SpotCone const& cone, int32_t side, float halfAngle )
        {
            Float2 const apexScreenSpace = viewport.WorldSpaceToScreenSpace( cone.m_apex );
            Float2 const axisScreenSpace = viewport.WorldSpaceToScreenSpace( cone.m_apex + cone.m_axis );
            Float2 const sideScreenSpace = viewport.WorldSpaceToScreenSpace( cone.m_apex + GetConeSideDirection( cone, side, halfAngle ) );

            Vector axisDirectionScreenSpace;
            float axisLengthScreenSpace = 0.0F;
            Vector( axisScreenSpace - apexScreenSpace ).ToDirectionAndLength2( axisDirectionScreenSpace, axisLengthScreenSpace );

            if ( axisLengthScreenSpace <= 0.0F )
            {
                return 0.0F;
            }

            Vector const perpendicularDirectionScreenSpace( -axisDirectionScreenSpace.GetY(), axisDirectionScreenSpace.GetX(), 0.0F, 0.0F );
            Vector const toSideScreenSpace( sideScreenSpace - apexScreenSpace );

            return Math::ATan2( toSideScreenSpace.GetDot3( perpendicularDirectionScreenSpace ), toSideScreenSpace.GetDot3( axisDirectionScreenSpace ) );
        }

        // The half angle whose side line points at the cursor, by bisection on that screen angle.
        static float SolveSideHalfAngle( Viewport const& viewport, SpotCone const& cone, int32_t side, float cursorAngle, float minHalfAngle, float maxHalfAngle )
        {
            float lower = minHalfAngle;
            float upper = Math::Max( maxHalfAngle, minHalfAngle + 0.0001F );

            for ( int32_t i = 0; i < LightEditor::s_coneSolveIterations; i++ )
            {
                float const middle = ( lower + upper ) * 0.5F;

                if ( Math::Abs( MeasureSideAngleScreenSpace( viewport, cone, side, middle ) ) < cursorAngle )
                {
                    lower = middle;
                }
                else
                {
                    upper = middle;
                }
            }

            return ( lower + upper ) * 0.5F;
        }

        // Screen space reach of a rim, measured from the apex along the cursor direction.
        static float MeasureRimExtentScreenSpace( Viewport const& viewport, SpotCone const& cone, float lightRadius, float halfAngle, Vector const& directionScreenSpace )
        {
            Float2 const apexScreenSpace = viewport.WorldSpaceToScreenSpace( cone.m_apex );

            float extent = 0.0F;
            for ( int32_t i = 0; i < LightEditor::s_coneSolveSegments; i++ )
            {
                float const angle = Math::TwoPi * ( float( i ) / float( LightEditor::s_coneSolveSegments ) );
                Vector const offsetScreenSpace( viewport.WorldSpaceToScreenSpace( GetConeRimPoint( cone, lightRadius, halfAngle, angle ) ) - apexScreenSpace );

                extent = Math::Max( extent, offsetScreenSpace.GetDot3( directionScreenSpace ) );
            }

            return extent;
        }

        // The light radius that puts the rim where the cursor is.
        static float SolveRimRadius( Viewport const& viewport, SpotCone const& cone, Vector const& directionScreenSpace, float targetExtentScreenSpace, float minRadius, float maxRadius )
        {
            float lower = minRadius;
            float upper = Math::Max( maxRadius, minRadius + 0.001F );

            for ( int32_t i = 0; i < LightEditor::s_coneSolveIterations; i++ )
            {
                float const middle = ( lower + upper ) * 0.5F;

                if ( MeasureRimExtentScreenSpace( viewport, cone, middle, cone.m_beamHalfAngle, directionScreenSpace ) < targetExtentScreenSpace )
                {
                    lower = middle;
                }
                else
                {
                    upper = middle;
                }
            }

            return ( lower + upper ) * 0.5F;
        }

        // Half angle of the rim under the cursor on it
        static float SolveRimHalfAngle( Viewport const& viewport, SpotCone const& cone, float lightRadius, Vector const& directionScreenSpace, float targetExtentScreenSpace, float minHalfAngle, float maxHalfAngle )
        {
            float lower = minHalfAngle;
            float upper = Math::Max( maxHalfAngle, minHalfAngle + 0.0001F );

            for ( int32_t i = 0; i < LightEditor::s_coneSolveIterations; i++ )
            {
                float const middle = ( lower + upper ) * 0.5F;

                if ( MeasureRimExtentScreenSpace( viewport, cone, lightRadius, middle, directionScreenSpace ) < targetExtentScreenSpace )
                {
                    lower = middle;
                }
                else
                {
                    upper = middle;
                }
            }

            return ( lower + upper ) * 0.5F;
        }

        static void DrawConePolyline( DebugDrawContext& drawingCtx, Vector const* pPoints, int32_t numPoints, Color const& color, float thickness )
        {
            for ( int32_t i = 1; i < numPoints; i++ )
            {
                drawingCtx.DrawLine( pPoints[i - 1], pPoints[i], color, thickness, DebugDrawLayer::Screen );
            }
        }

        // The cursor signed angle from the projected beam axis
        static float MeasureConeCursorAngleScreenSpace( Viewport const& viewport, SpotCone const& cone, Float2 const& mousePosition )
        {
            Float2 const apexScreenSpace = viewport.WorldSpaceToScreenSpace( cone.m_apex );
            Float2 const alongScreenSpace = viewport.WorldSpaceToScreenSpace( cone.m_apex + cone.m_axis );

            Vector axisDirectionScreenSpace;
            float axisLengthScreenSpace = 0.0F;
            Vector( alongScreenSpace - apexScreenSpace ).ToDirectionAndLength2( axisDirectionScreenSpace, axisLengthScreenSpace );

            if ( axisLengthScreenSpace <= 0.0F )
            {
                return 0.0F;
            }

            Vector const perpendicularDirectionScreenSpace( -axisDirectionScreenSpace.GetY(), axisDirectionScreenSpace.GetX(), 0.0F, 0.0F );
            Vector const toCursor( mousePosition - apexScreenSpace );

            return Math::ATan2( toCursor.GetDot3( perpendicularDirectionScreenSpace ), toCursor.GetDot3( axisDirectionScreenSpace ) );
        }

        // The half angle range a drag is allowed to sweep, per handle
        static void GetHalfAngleRange( SpotCone const& cone, LightEditor::LightHandle handle, float& outMinHalfAngle, float& outMaxHalfAngle )
        {
            if ( handle == LightEditor::LightHandle::Blend )
            {
                outMinHalfAngle = cone.m_beamHalfAngle * ( 1.0F - LightEditor::s_maxBlend );
                outMaxHalfAngle = cone.m_beamHalfAngle * ( 1.0F - LightEditor::s_minBlend );
            }
            else
            {
                outMinHalfAngle = MinBeamAngle * 0.5F * Math::DegreesToRadians;
                outMaxHalfAngle = MaxBeamAngle * 0.5F * Math::DegreesToRadians;
            }
        }

        // Grab one of the cone side lines.
        // The side the cursor is on is whichever of the two side lines it is closer in angle
        static void BeginSideDrag( LightEditor::DragState& drag, Viewport const& viewport, SpotCone const& cone, LightEditor::LightHandle handle, float cursorAngle )
        {
            float const firstAngle = Math::Abs( MeasureSideAngleScreenSpace( viewport, cone, 0, cone.m_beamHalfAngle ) );
            float const secondAngle = Math::Abs( MeasureSideAngleScreenSpace( viewport, cone, 1, cone.m_beamHalfAngle ) );

            drag.m_handle = handle;
            drag.m_side = ( Math::Abs( cursorAngle - firstAngle ) <= Math::Abs( cursorAngle - secondAngle ) ) ? 0 : 1;
            drag.m_sideRadials[0] = cone.m_radials[0];
            drag.m_sideRadials[1] = cone.m_radials[1];

            float minHalfAngle = 0.0F;
            float maxHalfAngle = 0.0F;
            GetHalfAngleRange( cone, handle, minHalfAngle, maxHalfAngle );

            float const startHalfAngle = cone.m_beamHalfAngle;

            drag.m_angleOffset = startHalfAngle - SolveSideHalfAngle( viewport, cone, drag.m_side, Math::Abs( cursorAngle ), minHalfAngle, maxHalfAngle );
        }

        // Grab a rim.
        static void BeginRimDrag( LightEditor::DragState& drag, Viewport const& viewport, SpotCone const& cone, LightEditor::LightHandle handle, float lightRadius, float halfAngle, Vector const& cursorDirectionScreenSpace, float cursorExtentScreenSpace )
        {
            drag.m_handle = handle;
            drag.m_viewDepth = Math::Max( cone.m_apex.GetDistance3( viewport.GetViewVolume().GetViewPosition() ), 0.001F );
            drag.m_startRadius = lightRadius;
            drag.m_radiusOffsetScreenSpace = MeasureRimExtentScreenSpace( viewport, cone, lightRadius, halfAngle, cursorDirectionScreenSpace ) - cursorExtentScreenSpace;
        }


        static void DrawConeRim( DebugDrawContext& drawingCtx, Viewport const& viewport, SpotCone const& cone, float lightRadius, float halfAngle, Color const& color, float thickness, bool isManipulating, uint64_t hitTestID )
        {
            Vector points[LightEditor::s_coneRimSegments + 1];

            for ( int32_t i = 0; i <= LightEditor::s_coneRimSegments; i++ )
            {
                points[i] = GetConeRimPoint( cone, lightRadius, halfAngle, Math::TwoPi * ( float( i % LightEditor::s_coneRimSegments ) / float( LightEditor::s_coneRimSegments ) ) );
            }

            Vector rimCentre;
            float rimRadius = 0.0F;
            GetConeRim( cone, lightRadius, halfAngle, rimCentre, rimRadius );

            drawingCtx.SetHitTestID( hitTestID );
            DrawConePolyline( drawingCtx, points, LightEditor::s_coneRimSegments + 1, color, LightEditor::GetHandleThickness( thickness, isManipulating ) );
            LightEditor::DrawHandleDot( drawingCtx, viewport, rimCentre, cone.m_axis, rimRadius, cone.m_right, color, isManipulating );
            drawingCtx.ClearHitTestID();
        }

        // The four handles are:
        //
        //  * The rim against the sphere of maxRadius
        //  * The rim against the falloff anchor sphere
        //  * The edge of the fully lit core
        //  * The beam two side lines (both moving together)
        //-------------------------------------------------------------------------

        void DrawSpotLight( EntityModel::EditorContext& context, Viewport const& viewport, SpotCone const& cone, LightEditor::LightHandle hoveredHandle )
        {
            auto drawingCtx = context.GetDebugDrawContext();

            Color const rangeColor = LightEditor::GetHandleColor( hoveredHandle == LightEditor::LightHandle::Range, m_drag.m_handle == LightEditor::LightHandle::Range );
            Color const falloffColor = LightEditor::GetHandleColor( hoveredHandle == LightEditor::LightHandle::Falloff, m_drag.m_handle == LightEditor::LightHandle::Falloff );
            Color const beamColor = LightEditor::GetHandleColor( hoveredHandle == LightEditor::LightHandle::BeamAngle, m_drag.m_handle == LightEditor::LightHandle::BeamAngle );
            Color const blendColor = LightEditor::GetHandleColor( hoveredHandle == LightEditor::LightHandle::Blend, m_drag.m_handle == LightEditor::LightHandle::Blend );

            bool const isManipulating = ( m_drag.m_handle != LightEditor::LightHandle::None );

            // Range handle
            DrawConeRim( drawingCtx, viewport, cone, cone.m_rangeRadius, cone.m_beamHalfAngle, rangeColor, LightEditor::s_lineThickness, isManipulating, LightEditor::s_gizmoHitTestID );

            // Falloff handle
            if ( cone.m_falloffRadius > 0.0F )
            {
                DrawConeRim( drawingCtx, viewport, cone, cone.m_falloffRadius, cone.m_beamHalfAngle, falloffColor, LightEditor::s_primaryThickness, isManipulating, LightEditor::s_falloffGizmoHitTestID );
            }

            // Blend handle
            DrawConeRim( drawingCtx, viewport, cone, cone.m_rangeRadius, cone.m_blendHalfAngle, blendColor, LightEditor::s_primaryThickness, isManipulating, LightEditor::s_blendGizmoHitTestID );

            // The cone projected edges, running from part way out to the plane of the rim
            float const truncation = cone.m_rangeRadius * LightEditor::s_coneTruncationScale;
            Vector points[2];
            for ( int32_t i = 0; i < 2; i++ )
            {
                points[0] = cone.m_apex + ( GetConeSideDirection( cone, i, cone.m_beamHalfAngle ) * truncation );
                points[1] = GetConeSideEndPoint( cone, i, cone.m_beamHalfAngle );

                drawingCtx.SetHitTestID( LightEditor::s_beamGizmoHitTestID );
                DrawConePolyline( drawingCtx, points, 2, beamColor, LightEditor::GetHandleThickness( LightEditor::s_lineThickness, isManipulating ) );
                drawingCtx.ClearHitTestID();
            }
        }

        //-------------------------------------------------------------------------

        bool UpdateSpotDrag( Viewport const* pViewport, SpotCone const& cone, LightEditor::LightHandle hoveredHandle )
        {
            Float2 const mousePosition = ImGui::GetMousePos();
            Float2 const apexScreenSpace = pViewport->WorldSpaceToScreenSpace( cone.m_apex );

            Vector cursorDirectionScreenSpace;
            float cursorExtentScreenSpace = 0.0F;
            Vector( mousePosition - apexScreenSpace ).ToDirectionAndLength2( cursorDirectionScreenSpace, cursorExtentScreenSpace );

            if ( m_drag.m_handle == LightEditor::LightHandle::None )
            {
                if ( LightEditor::TryGrabHandle( LightEditor::LightHandle::Range, hoveredHandle ) )
                {
                    BeginRimDrag( m_drag, *pViewport, cone, LightEditor::LightHandle::Range, cone.m_rangeRadius, cone.m_beamHalfAngle, cursorDirectionScreenSpace, cursorExtentScreenSpace );
                    return true;
                }

                if ( LightEditor::TryGrabHandle( LightEditor::LightHandle::Falloff, hoveredHandle ) )
                {
                    BeginRimDrag( m_drag, *pViewport, cone, LightEditor::LightHandle::Falloff, cone.m_falloffRadius, cone.m_beamHalfAngle, cursorDirectionScreenSpace, cursorExtentScreenSpace );
                    return true;
                }

                if ( LightEditor::TryGrabHandle( LightEditor::LightHandle::Blend, hoveredHandle ) )
                {
                    BeginRimDrag( m_drag, *pViewport, cone, LightEditor::LightHandle::Blend, cone.m_rangeRadius, cone.m_blendHalfAngle, cursorDirectionScreenSpace, cursorExtentScreenSpace );
                    return true;
                }

                if ( LightEditor::TryGrabHandle( LightEditor::LightHandle::BeamAngle, hoveredHandle ) )
                {
                    BeginSideDrag( m_drag, *pViewport, cone, LightEditor::LightHandle::BeamAngle, MeasureConeCursorAngleScreenSpace( *pViewport, cone, mousePosition ) );
                    return true;
                }

                return false;
            }

            switch ( m_drag.m_handle )
            {
                case LightEditor::LightHandle::Range:
                {
                    if ( !LightEditor::IsHandleDragActive( m_drag, LightEditor::LightHandle::Range ) )
                    {
                        return false;
                    }

                    float const radiusCap = Math::Max( m_drag.m_viewDepth * 0.99F, m_drag.m_startRadius );
                    float const radius = SolveRimRadius( *pViewport, cone, cursorDirectionScreenSpace, cursorExtentScreenSpace + m_drag.m_radiusOffsetScreenSpace, 0.01F, radiusCap );

                    m_pComponent->SetMaxRadius( radius );
                    return true;
                }

                case LightEditor::LightHandle::Falloff:
                {
                    if ( !LightEditor::IsHandleDragActive( m_drag, LightEditor::LightHandle::Falloff ) )
                    {
                        return false;
                    }

                    float const maxRadius = Math::Max( m_pComponent->GetMaxRadius(), 0.001F );
                    float const radius = SolveRimRadius( *pViewport, cone, cursorDirectionScreenSpace, cursorExtentScreenSpace + m_drag.m_radiusOffsetScreenSpace, 0.001F, LightEditor::GetMaxFalloffAnchorRadius( maxRadius ) );

                    float falloff = 0.0F;
                    if ( LightEditor::TryGetFalloffAtRadius( radius, maxRadius, falloff ) )
                    {
                        m_pComponent->SetFalloff( LightEditor::GetEditableFalloff( falloff ) );
                    }

                    return true;
                }

                case LightEditor::LightHandle::Blend:
                {
                    if ( !LightEditor::IsHandleDragActive( m_drag, LightEditor::LightHandle::Blend ) )
                    {
                        return false;
                    }

                    float minHalfAngle = 0.0F;
                    float maxHalfAngle = 0.0F;
                    GetHalfAngleRange( cone, LightEditor::LightHandle::Blend, minHalfAngle, maxHalfAngle );

                    float const halfAngle = SolveRimHalfAngle( *pViewport, cone, cone.m_rangeRadius, cursorDirectionScreenSpace, cursorExtentScreenSpace + m_drag.m_radiusOffsetScreenSpace, minHalfAngle, maxHalfAngle );

                    m_pComponent->SetBlend( LightEditor::GetEditableBlend( 1.0F - ( halfAngle / cone.m_beamHalfAngle ) ) );

                    return true;
                }

                case LightEditor::LightHandle::BeamAngle:
                {
                    if ( !LightEditor::IsHandleDragActive( m_drag, LightEditor::LightHandle::BeamAngle ) )
                    {
                        return false;
                    }

                    float const cursorAngle = MeasureConeCursorAngleScreenSpace( *pViewport, cone, mousePosition );

                    float minHalfAngle = 0.0F;
                    float maxHalfAngle = 0.0F;
                    GetHalfAngleRange( cone, LightEditor::LightHandle::BeamAngle, minHalfAngle, maxHalfAngle );

                    float const halfAngle = SolveSideHalfAngle( *pViewport, cone, m_drag.m_side, Math::Abs( cursorAngle ), minHalfAngle, maxHalfAngle ) + m_drag.m_angleOffset;

                    m_pComponent->SetBeamAngle( Degrees( Math::Clamp( halfAngle * Math::RadiansToDegrees * 2.0F, MinBeamAngle, MaxBeamAngle ) ) );

                    return true;
                }

                default:
                {
                    return false;
                }
            }
        }

    private:

        LightEditor::DragState m_drag;
    };

    // Directional Light Editor
    //-------------------------------------------------------------------------

    class DirectionalLightComponentTools : public EntityModel::TComponentTools<DirectionalLightComponent>
    {
    public:

        using TComponentTools::TComponentTools;

        virtual bool HasViewportSelectionWidget() const override { return true; }
        char const* GetViewportSelectionWidgetIcon() const override { return EE_ICON_EDITOR_DIRECTIONALLIGHT; }
        Color GetViewportSelectionWidgetColor() const override { return Colors::Yellow; }

        virtual bool DrawViewportEditor( EntityModel::EditorContext &context, Viewport const* pViewport, bool isFocused ) override
        {
            if ( pViewport == nullptr )
            {
                return false;
            }

            DrawDirectionalLight( context, LightEditor::GetPickedHitTestID( *pViewport ) == LightEditor::s_gizmoHitTestID );

            return false;
        }

    private:

        // * An arrow along the light direction
        // * An azimuth ring for the light direction
        //-------------------------------------------------------------------------

        void DrawDirectionalLight( EntityModel::EditorContext& context, bool isHovered )
        {
            auto drawingCtx = context.GetDebugDrawContext();

            Vector const position = m_pComponent->GetPosition();
            Vector const direction = m_pComponent->GetLightDirection().GetNormalized3();

            Color const color = LightEditor::GetHandleColor( isHovered, false );

            // Direction arrow
            drawingCtx.SetHitTestID( LightEditor::s_gizmoHitTestID );
            drawingCtx.DrawArrow( position, position + direction * LightEditor::s_directionalArrowLength, color, LightEditor::s_primaryThickness, DebugDrawLayer::Screen );
            drawingCtx.ClearHitTestID();

            // Azimuth ring, in the plane perpendicular to the light direction
            Transform const ringTransform( Quaternion::FromRotationBetweenUnitVectors( Vector::UnitZ, direction ), position );
            drawingCtx.DrawCircle( ringTransform, Axis::Z, LightEditor::s_directionalRingRadius, LightEditor::s_secondaryColor.GetAlphaVersion( LightEditor::s_secondaryAlpha ), LightEditor::s_lineThickness, DebugDrawLayer::Screen );
        }
    };

    //-------------------------------------------------------------------------

    EE_COMPONENT_TOOLS( PointLightComponent, PointLightComponentTools );
    EE_COMPONENT_TOOLS( SpotLightComponent, SpotLightComponentTools );
    EE_COMPONENT_TOOLS( DirectionalLightComponent, DirectionalLightComponentTools );
}
