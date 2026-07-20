#pragma once

#include "ImguiGizmo_Base.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    class EE_ENGINE_API TranslationGizmo final : public GizmoBase
    {
        struct Style final : public GizmoBase::Style
        {
            static inline Color const       s_defaultColor = EE::Colors::White;
            constexpr static float const    s_dimColorScale = 0.75f;
            constexpr static float const    s_planeManipulatorAlpha = 0.75f;

        public:

            virtual void Reset() override { *this = TranslationGizmo::Style(); }
            virtual void SetScale( float scale ) override;

        public:

            float                           m_originCircleRadius = 4.0f;
            float                           m_axisLength = 75.0f;
            float                           m_axisThickness = 3.0f;
            float                           m_axisArrowThickness = 6.0f;
            float                           m_axisAdditionalHoverBorder = 8.0f;
            float                           m_axisPlaneManipulatorOffsetRatio = 0.5f;
            float                           m_axisPlaneManipulatorWidthRatio = 0.2f;
            float                           m_axisPlaneManipulatorBorderThickness = 1.0f;
            float                           m_axisOffsetFromOrigin = m_originCircleRadius * 3;
            float                           m_hoverDetectionDistance = m_axisThickness + m_axisAdditionalHoverBorder;
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
            Vector                          m_axisStartSS = Vector::Zero;
            Vector                          m_axisEndSS = Vector::Zero;
            Vector                          m_axisStartInfSS = Vector::Zero;
            Vector                          m_axisEndInfSS = Vector::Zero;
            Vector                          m_axisDirSS = Vector::Zero;
            float                           m_axisLengthSS = 0;
            Color                           m_color = 0;
            Manipulator                     m_manipulator = Manipulator::None;
            float                           m_distanceToCamera = FLT_MAX;
            bool                            m_isHovered = false;
            bool                            m_isFlipped = false;
        };

        struct PlaneManipulator
        {
            Vector                          m_manipulationPlaneAxis;
            ImVec2                          m_pointsSS[5];
            float                           m_distanceToCamera = FLT_MAX;
            Color                           m_color = 0;
            Manipulator                     m_manipulator = Manipulator::None;
            Vector                          m_planeDirSS0 = Vector::Zero;
            Vector                          m_planeDirSS1 = Vector::Zero;
            Vector                          m_offsetNear0 = Vector::Zero;
            Vector                          m_offsetFar0 = Vector::Zero;
            Vector                          m_offsetNear1 = Vector::Zero;
            Vector                          m_offsetFar1 = Vector::Zero;
            bool                            m_isHovered = false;
            bool                            m_isHidden = false;
        };

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
        float                       m_closestAxisToCameraDistance = FLT_MAX;
        bool                        m_isAnyManipulatorHovered = false;

        Plane                       m_translationPlane;
        Vector                      m_manipulationStartPosWS = Vector::Zero;
        Vector                      m_previousPosWS = Vector::Zero;
        Vector                      m_deltaWS = Vector::Zero;
    };
}
#endif