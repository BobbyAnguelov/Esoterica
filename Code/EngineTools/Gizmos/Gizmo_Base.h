#pragma once

#include "EngineTools/_Module/API.h"
#include "Engine/Viewport/Viewport.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Math/Matrix.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------
// Debug Draw Gizmos
//-------------------------------------------------------------------------

namespace EE
{
    enum class GizmoState
    {
        None = 0,
        StartedManipulating,
        Manipulating,
        StoppedManipulating
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API GizmoBase
    {
    protected:

        struct Style
        {
            static Color const s_axisColors[3];

        public:

            virtual ~Style() = default;
            virtual void Reset() = 0;
            virtual void SetScale( float scale );

            inline float GetHoverDetectionDistance() const { return m_lineThickness + m_axisAdditionalHoverBorder; }

        public:

            // All sizes are in pixels
            float                           m_originCircleRadius = 4.0f;
            float                           m_axisLength = 100.0f;
            float                           m_lineThickness = 3.0f;
            float                           m_axisAdditionalHoverBorder = 8.0f;
        };

        //-------------------------------------------------------------------------

        struct Context
        {
            Context( Viewport const& viewport ) : m_viewport( viewport ) {}

            // Project a world space point into screen space
            inline Vector WorldSpaceToScreenSpace( Vector const& pointWS ) const { return Vector( m_viewport.WorldSpaceToScreenSpace( pointWS ) ); }

            // Convert a size in pixels to a world space size at the gizmo's depth
            inline float PixelsToWorld( float numPixels ) const { return numPixels * m_worldUnitsPerPixel; }

        public:

            Viewport const&                 m_viewport;
            Vector                          m_viewDirectionWS;
            Vector                          m_viewPositionWS;
            Vector                          m_positionWS;
            Vector                          m_positionSS;
            Quaternion                      m_rotationWS;
            Vector                          m_mousePositionSS;
            Vector                          m_mouseDeltaSS;
            float                           m_worldUnitsPerPixel = 0.0f;
            bool                            m_isMouseInViewport = false;
        };

        //-------------------------------------------------------------------------

        using PointList = TInlineVector<Vector, 74>;

        constexpr static uint64_t const s_hitTestID_axisX = 0xFFFFFFFFFFFFFFF0;
        constexpr static uint64_t const s_hitTestID_axisY = 0xFFFFFFFFFFFFFFF1;
        constexpr static uint64_t const s_hitTestID_axisZ = 0xFFFFFFFFFFFFFFF2;

    public:

        GizmoBase() = default;
        virtual ~GizmoBase() = default;

        virtual void Reset() = 0;
        virtual bool IsManipulating() const = 0;

        GizmoState UpdateAndDraw( Vector const& positionWS, Quaternion const& rotationWS, Viewport const& viewport, bool isFocused, char const* pOptionalLabel = nullptr );

        inline void SetCoordinateSystemSpace( CoordinateSpace space ) { m_coordinateSpace = space; }
        inline CoordinateSpace GetCoordinateSystemSpace() const { return m_coordinateSpace; }

        inline void EnableDebug( bool isEnabled ) { m_debugEnabled = isEnabled; }

    protected:

        virtual Style* GetStyle() = 0;

        virtual void SetupManipulators( Context const& ctx ) = 0;
        virtual void UpdateHoverState( Context const& ctx ) = 0;
        virtual void DrawManipulators( Context const& ctx ) = 0;

        virtual bool TryStartManipulating( Context const& ctx ) = 0;
        virtual void Manipulate( Context const& ctx ) = 0;
        virtual void StopManipulating( Context const& ctx ) = 0;

        virtual void DrawDebug( Context const& ctx ) = 0;

        uint64_t CalculateUniqueHitTestID( uint64_t baseHitTestID ) const;

        float PixelHeightToWorldHeight( Context const& ctx, Vector const& position, float pixelHeight );

        // Get the world space point for a screen space position, on the view aligned plane that passes through the gizmo origin
        Vector ScreenSpaceToWorldSpaceAtGizmoDepth( Context const& ctx, Vector const& pointSS );

        // Drawing Helpers
        //-------------------------------------------------------------------------

        // Draw the origin dot
        void DrawOrigin( Context const& ctx, Color color );

        // Draw a line through the gizmo origin that extends to the edges of the view volume
        void DrawInfiniteLine( Context const& ctx, Vector const& directionWS, Color color, float thickness );

        // Draw a line through the gizmo origin that extends to the edges of the view volume
        void DrawPolyLine( DebugDrawContext& drawingCtx, PointList const& pointsWS, Color color, float thickness );

        // Dash and gap lengths are in pixels
        void DrawDashedLine( Context const& ctx, Vector const& startWS, Vector const& endWS, Color color, float thickness, float dashLength = 10.0f, float gapLength = 10.0f );

        // The debug renderer back-face culls triangles, so draw both windings for anything that needs to be visible from both sides
        static void DrawDoubleSidedTriangle( DebugDrawContext& drawingCtx, Vector const& v0, Vector const& v1, Vector const& v2, Color color );

        // Draw a solid cone with the tip at 'baseCenterWS + ( directionWS * length )'
        void DrawSolidCone( DebugDrawContext& drawingCtx, Vector const& baseCenterWS, Vector const& directionWS, float length, float radius, Color color );

    protected:

        CoordinateSpace             m_coordinateSpace = CoordinateSpace::World;
        bool                        m_debugEnabled = false;
    };
}
