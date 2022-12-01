#pragma once
#include "System/Render/RenderViewport.h"
#include "System/Types/BitFlags.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    class EE_SYSTEM_API Gizmo
    {
    public:

        enum class GizmoMode
        {
            Translation,
            Rotation,
            Scale
        };

        enum class ManipulationMode
        {
            None = 0,

            RotateX,
            RotateY,
            RotateZ,

            TranslateX,
            TranslateY,
            TranslateZ,
            TranslateXY,
            TranslateYZ,
            TranslateXZ,

            ScaleX,
            ScaleY,
            ScaleZ,
        };

        enum class Options
        {
            AllowAxesFlipping = 0,
            AllowCoordinateSpaceSwitching,
            AllowScale,
            AllowNonUniformScale,
        };

        enum class State
        {
            None = 0,
            StartedManipulating,
            Manipulating,
            StoppedManipulating
        };

        enum class ResultDeltaType
        {
            None,
            Rotation,
            Translation,
            Scale,
            NonUniformScale
        };

        struct EE_SYSTEM_API Result
        {
            // Apply the results of the gizmo manipulation to a transform
            void ApplyResult( Transform& transform ) const;

            // Get a modified version of a transform with the gizmo manipulation applied
            inline Transform GetModifiedTransform( Transform const& transform ) const
            {
                Transform t = transform;
                ApplyResult( t );
                return t;
            }

        public:

            State               m_state = State::None;
            Vector              m_deltaPositionScale = Vector::Zero;
            Quaternion          m_deltaOrientation = Quaternion::Identity;
            ResultDeltaType     m_deltaType = ResultDeltaType::None;
        };

    public:

        Gizmo();
        Gizmo( TBitFlags<Options> options );

        Result Draw( Vector const& position, Quaternion const& orientation, Render::Viewport const& viewport );

        inline bool IsManipulating() const { return m_manipulationMode != ManipulationMode::None; }

        inline void SetOption( Options option, bool isEnabled ) { m_options.SetFlag( option, isEnabled ); }
        inline void SetOptions( TBitFlags<Options> options) { m_options = options; }

        inline GizmoMode GetMode() const { return m_gizmoMode; }
        void SwitchMode( GizmoMode newMode );
        void SwitchToNextMode();

        void SetCoordinateSystemSpace( CoordinateSpace space );
        inline bool IsInWorldSpace() const { return m_coordinateSpace == CoordinateSpace::World && m_gizmoMode != GizmoMode::Scale; }
        inline bool IsInLocalSpace() const { return m_coordinateSpace == CoordinateSpace::Local || m_gizmoMode == GizmoMode::Scale; }

    private:

        Result DrawTranslationGizmo( Vector const& originWS, Quaternion const& orientationWS, Render::Viewport const& viewport );
        Result DrawRotationGizmo( Vector const& originWS, Quaternion const& orientationWS, Render::Viewport const& viewport );
        Result DrawScaleGizmo( Vector const& originWS, Quaternion const& orientationWS, Render::Viewport const& viewport );

    protected:

        TBitFlags<Options>          m_options;
        CoordinateSpace             m_coordinateSpace = CoordinateSpace::World;
        GizmoMode                   m_gizmoMode = GizmoMode::Translation;

        // Manipulation state
        ManipulationMode            m_manipulationMode = ManipulationMode::None;
        Vector                      m_translationScaleDeltaOriginWS = Vector::Zero;

        Quaternion                  m_originalStartRotation = Quaternion::Identity;
        Vector                      m_rotationStartDirection;
        Vector                      m_rotationAxis = Vector::Zero;
        Radians                     m_rotationDeltaAngle = Radians( 0.0f );
    };
}
#endif