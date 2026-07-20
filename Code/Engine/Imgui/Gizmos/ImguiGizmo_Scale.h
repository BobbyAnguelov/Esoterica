#pragma once

#include "ImguiGizmo_Base.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    class EE_ENGINE_API ScaleGizmo final : public GizmoBase
    {
        struct Style : public GizmoBase::Style
        {
            static inline Color const       s_defaultColor = EE::Colors::White;
            constexpr static float const    s_dimColorScale = 0.75f;

        public:

            virtual void Reset() override { *this = ScaleGizmo::Style(); }
            virtual void SetScale( float scale ) override;

        public:

            float                           m_originCircleRadius = 4.0f;
            float                           m_axisLength = 100.0f;
            float                           m_axisThickness = 3.0f;
            float                           m_axisEndCapRadius = 6.0f;
            float                           m_axisAdditionalHoverBorder = 8.0f;
            float                           m_axisOffsetFromOrigin = m_originCircleRadius * 3;
            float                           m_hoverDetectionDistance = m_axisThickness + m_axisAdditionalHoverBorder;
        };

        enum class Manipulator
        {
            None = 0,

            ScaleX,
            ScaleY,
            ScaleZ,
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

    public:

        inline Vector GetDelta() const { return m_delta; }
        inline void SetNonUniformScaleAllowed( bool isAllowed ) { m_allowNonUniformScale = isAllowed; }

        virtual void Reset() override { *this = ScaleGizmo(); }
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

    private:

        Style                       m_style;
        Manipulator                 m_activeManipulator = Manipulator::None;
        bool                        m_allowNonUniformScale = false;

        AxisManipulator             m_axes[3];
        float                       m_closestAxisToCameraDistance = FLT_MAX;
        bool                        m_isAnyManipulatorHovered = false;

        Vector                      m_manipulationStartMousePositionSS = Vector::Zero;
        float                       m_previousScaleFactor = FLT_MAX;
        Vector                      m_delta = Vector::Zero;
    };
}
#endif