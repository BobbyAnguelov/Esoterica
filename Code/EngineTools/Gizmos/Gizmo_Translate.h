#pragma once

#include "Gizmo_Base.h"
#include "Base/Math/Plane.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINETOOLS_API TranslationGizmo final : public GizmoBase
    {
        struct Style final : public GizmoBase::Style
        {
            static inline Color const       s_defaultColor = EE::Colors::White;
            constexpr static float const    s_dimColorScale = 0.75f;
            constexpr static float const    s_planeManipulatorAlpha = 0.75f;

        public:

            inline float GetAxisArrowLength() const { return m_axisArrowThickness * 3; }

            virtual void Reset() override { *this = TranslationGizmo::Style(); }
            virtual void SetScale( float scale ) override;

        public:

            float                           m_axisArrowThickness = 6.0f;

            float                           m_axisPlaneManipulatorOffsetRatio = 0.5f;
            float                           m_axisPlaneManipulatorWidthRatio = 0.2f;
            float                           m_axisPlaneManipulatorBorderThickness = 1.0f;
            float                           m_axisOffsetFromOrigin = m_originCircleRadius * 3;
            float                           m_minimumRequiredAreaForPlaneManipulators = 100;
        };

        enum class Manipulator
        {
            None = 0,

            TranslateX,
            TranslateY,
            TranslateZ,
            TranslateXY,
            TranslateYZ,
            TranslateXZ,
        };

        struct AxisManipulator
        {
            Vector                          m_axisDirWS = Vector::Zero;
            Vector                          m_scaledAxisDirWS = Vector::Zero;
            Vector                          m_axisStartWS = Vector::Zero;
            Vector                          m_axisEndWS = Vector::Zero;
            Vector                          m_axisStartSS = Vector::Zero;
            Vector                          m_axisEndSS = Vector::Zero;
            Vector                          m_axisEndInfSS = Vector::Zero;
            Vector                          m_axisDirSS = Vector::Zero;
            float                           m_axisLengthSS = 0;
            Color                           m_color = 0;
            uint64_t                        m_hitTestID = 0;
            Manipulator                     m_manipulator = Manipulator::None;
            bool                            m_isHovered = false;
            bool                            m_isFlipped = false;
        };

        struct PlaneManipulator
        {
            Vector                          m_manipulationPlaneAxis;
            Vector                          m_pointsWS[4];
            Color                           m_color = 0;
            uint64_t                        m_hitTestID = 0;
            Manipulator                     m_manipulator = Manipulator::None;
            bool                            m_isHovered = false;
            bool                            m_isHidden = false;
        };

        constexpr static uint64_t const s_hitTestID_axisXY = 0xFFFFFFFFFFFFFFF3;
        constexpr static uint64_t const s_hitTestID_axisYZ = 0xFFFFFFFFFFFFFFF4;
        constexpr static uint64_t const s_hitTestID_axisXZ = 0xFFFFFFFFFFFFFFF5;

    public:

        inline Vector GetDelta() const { return m_deltaWS; }

        virtual void Reset() override { *this = TranslationGizmo(); }
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

        bool TryGetProjectedManipulationPoint( Context const& ctx, AxisManipulator const& axis, Vector const& axisOriginPosWS, Vector const& pointSS, Vector& projectManipulationPoint );

    private:

        Style                       m_style;
        Manipulator                 m_activeManipulator = Manipulator::None;

        AxisManipulator             m_axes[3];
        PlaneManipulator            m_planes[3];
        bool                        m_isAnyManipulatorHovered = false;

        Plane                       m_translationPlane;
        Vector                      m_manipulationStartPosWS = Vector::Zero;
        Vector                      m_previousPosWS = Vector::Zero;
        Vector                      m_deltaWS = Vector::Zero;
    };
}
