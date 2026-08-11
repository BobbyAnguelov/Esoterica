#pragma once

#include "ImguiGizmo_Base.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    class EE_ENGINE_API RotationGizmo final : public GizmoBase
    {
        struct Style final : public GizmoBase::Style
        {
            static inline Color const       s_defaultColor = EE::Colors::White;
            static inline Color const       s_trackballColor = EE::Colors::White.GetAlphaVersion( 0.45f );
            constexpr static float const    s_minAngleRadiansBetweenPoints = Math::DegreesToRadians * 1.0f;
            constexpr static float const    s_minAngleThresholdRadiansBetweenViewAndAxis = Math::DegreesToRadians * 10.0f;

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
            Vector                          m_axisDirSS = Vector::Zero;
            Vector                          m_axisStartInfSS = Vector::Zero;
            Vector                          m_axisEndInfSS = Vector::Zero;
            TInlineVector<ImVec2,60>        m_pointsSS;
            Color                           m_color = 0;
            Manipulator                     m_manipulator = Manipulator::None;
            bool                            m_isHovered = false;
        };

        struct TrackballManipulator
        {
            Color                           m_color = 0;
            bool                            m_isHovered = false;
            TInlineVector<ImVec2, 60>       m_pointsSS;
            float                           m_manipulationRadius = 0.0f;
        };

        struct ScreenManipulator
        {
            Color                           m_color = 0;
            TInlineVector<ImVec2, 60>       m_pointsSS;
            bool                            m_isHovered = false;
        };

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
#endif