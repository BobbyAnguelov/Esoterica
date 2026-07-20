#pragma once
#include "Engine/_Module/API.h"
#include "Gizmos/ImguiGizmo_Translate.h"
#include "Gizmos/ImguiGizmo_Rotate.h"
#include "Gizmos/ImguiGizmo_Scale.h"
#include "Base/Types/BitFlags.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    class EE_ENGINE_API Gizmo
    {

    public:

        enum class Mode
        {
            Translation,
            Rotation,
            Scale
        };

        enum class Options
        {
            AllowCoordinateSpaceSwitching = 0,
            AllowScale,
            AllowNonUniformScale,
        };

        enum class ResultDeltaType
        {
            None,
            Rotation,
            Translation,
            Scale,
            NonUniformScale
        };

        struct EE_ENGINE_API Result
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

            bool IsManipulating() const { return m_state == GizmoState::StartedManipulating || m_state == GizmoState::Manipulating; }

            inline bool IsTranslation() const { return m_deltaType == ResultDeltaType::Translation; }
            inline bool IsRotation() const { return m_deltaType == ResultDeltaType::Rotation; }
            inline bool IsScale() const { return m_deltaType == ResultDeltaType::Scale || m_deltaType == ResultDeltaType::NonUniformScale; }

        public:

            GizmoState          m_state = GizmoState::None;
            Vector              m_delta = Vector::Zero; // either a position, scale or quat delta
            ResultDeltaType     m_deltaType = ResultDeltaType::None;
        };

    public:

        Gizmo();
        Gizmo( TBitFlags<Options> options );

        Result Draw( Vector const& position, Quaternion const& orientation, Viewport const& viewport, char const* pOptionalLabel = nullptr );

        void Reset();
        bool IsManipulating() const;

        inline void SetOption( Options option, bool isEnabled ) { m_options.SetFlag( option, isEnabled ); }
        inline void SetOptions( TBitFlags<Options> options) { m_options = options; }

        inline Mode GetMode() const { return m_mode; }
        void SetMode( Mode newMode );
        void SwitchToNextMode();

        void SetCoordinateSystemSpace( CoordinateSpace space );
        inline CoordinateSpace GetCoordinateSystemSpace() const { return GetActiveGizmo()->GetCoordinateSystemSpace(); }
        inline bool IsInWorldSpace() const { return GetActiveGizmo()->GetCoordinateSystemSpace() == CoordinateSpace::World; }
        inline bool IsInLocalSpace() const { return GetActiveGizmo()->GetCoordinateSystemSpace() == CoordinateSpace::Local; }

    private:

        GizmoBase* GetActiveGizmo();
        GizmoBase const* GetActiveGizmo() const { return const_cast<Gizmo*>( this )->GetActiveGizmo(); }

    protected:

        Style                       m_style;
        TBitFlags<Options>          m_options;
        Mode                        m_mode = Mode::Translation;

        // Manipulation state
        TranslationGizmo            m_translationGizmo;
        RotationGizmo               m_rotationGizmo;
        ScaleGizmo                  m_scaleGizmo;
    };
}
#endif