#pragma once

#include "Gizmo_Base.h"
#include "Base/Math/Plane.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINETOOLS_API RotationGizmo final : public GizmoBase
    {
        struct Style final : public GizmoBase::Style
        {
            static inline Color const       s_defaultColor = EE::Colors::White;
            static inline Color const       s_trackballColor = EE::Colors::White.GetAlphaVersion( 0.065f );
            static inline Color const       s_trackballHoverColor = EE::Colors::White.GetAlphaVersion( 0.1f );
            constexpr static float const    s_angleRadiansBetweenPoints = Math::DegreesToRadians * 5.0f;
            constexpr static float const    s_minAngleRadiansBetweenViewAndAxis = Math::DegreesToRadians * 0.2f; // Below this, we cannot calculate the camera facing half of the manipulator
            constexpr static float const    s_manipulationVisualizationAlpha = 0.75f;

        public:

            void Reset() override { *this = RotationGizmo::Style(); }
            void SetScale( float scale ) override;

        public:

            float                           m_axisManipulatorDeadZone = 2.0f;
            float                           m_trackballDeadZone = 8.0f;
            float                           m_trackBallSizeMultiplier = 1.1f;
            float                           m_screenManipulatorSizeMultiplier = 1.25f;
        };

        enum class Manipulator
        {
            None = 0,

            RotateX,
            RotateY,
            RotateZ,

            RotateTrackball,
            RotateScreen,
        };

        struct AxisManipulator
        {
            Vector                          m_axisDirWS = Vector::Zero;
            Vector                          m_vectorInRotationPlane = Vector::Zero;
            PointList                       m_pointsWS;
            Color                           m_color = 0;
            uint64_t                        m_hitTestID = 0;
            Manipulator                     m_manipulator = Manipulator::None;
            bool                            m_isHovered = false;
        };

        struct TrackballManipulator
        {
            Color                           m_color = 0;
            bool                            m_isHovered = false;
            PointList                       m_pointsWS;
            uint64_t                        m_hitTestID = 0;
            float                           m_manipulationRadius = 0.0f;
        };

        struct ScreenManipulator
        {
            Color                           m_color = 0;
            PointList                       m_pointsWS;
            uint64_t                        m_hitTestID = 0;
            bool                            m_isHovered = false;
        };

        constexpr static uint64_t const s_hitTestID_screen = 0xFFFFFFFFFFFFFFF6;
        constexpr static uint64_t const s_hitTestID_trackball = 0xFFFFFFFFFFFFFFF7;

    public:

        inline Quaternion GetDelta() const { return m_delta; }

        virtual void Reset() override { *this = RotationGizmo(); }
        virtual bool IsManipulating() const override { return m_activeManipulator != Manipulator::None; }

    private:

        virtual Style* GetStyle() override { return &m_style; }
        virtual void SetupManipulators( Context const& ctx ) override;
        virtual void UpdateHoverState( Context const& ctx ) override;
        virtual void DrawManipulators( Context const& ctx ) override;
        virtual bool TryStartManipulating( Context const& ctx ) override;
        virtual void Manipulate( Context const& ctx ) override;
        virtual void StopManipulating( Context const& ctx ) override;
        virtual void DrawDebug( Context const& ctx ) override;

        void DrawAxisRotationManipulationVisualization( Context const& ctx, AxisManipulator const& axis );
        void DrawScreenRotationManipulationVisualization( Context const& ctx );
        void DrawTrackballRotationManipulationVisualization( Context const& ctx );

        // Generate the points for an arc around the gizmo origin, starting at the supplied offset and rotating around the supplied axis
        // Returns the max screen space distance from the origin of the generated points
        float GenerateArcPoints( Context const& ctx, Vector const& rotationAxisWS, Vector const& startOffsetWS, float arcAngleRadians, PointList& outPointsWS ) const;

        // Draw a filled arc (i.e. a pie slice) centered on the gizmo origin
        void DrawFilledArc( Context const& ctx, Vector const& rotationAxisWS, Vector const& startOffsetWS, float arcAngleRadians, Color color );

        Vector ProjectScreenSpacePositionOntoRotationPlane( Context const& ctx, Vector const &positionSS ) const;

    private:

        Style                       m_style;
        Manipulator                 m_activeManipulator = Manipulator::None;

        AxisManipulator             m_axes[3];
        TrackballManipulator        m_trackballManipulator;
        ScreenManipulator           m_screenManipulator;
        bool                        m_isAnyManipulatorHovered = false;

        Quaternion                  m_delta = Quaternion::Identity;
        Vector                      m_rotationAxis = Vector::Zero;
        Vector                      m_manipulationStartMousePositionSS = Vector::Zero;
        Plane                       m_rotationPlane;
        Vector                      m_projectedStartMousePosWS = Vector::Zero;
        Radians                     m_previousRotationAngle = 0.0f;
        Quaternion                  m_manipulationStartRotationWS = Quaternion::Identity;
        Quaternion                  m_previousRotation = Quaternion::Identity;
    };
}
