#pragma once
#include "Engine/_Module/API.h"
#include "Engine/Render/RenderViewport.h"
#include "System/Math/Matrix.h"
#include "System/Types/Color.h"
#include "System/Types/Event.h"
#include "System/Types/BitFlags.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    class EE_ENGINE_API Gizmo
    {
        enum class AxisCap
        {
            Dot,
            Arrow
        };

    public:

        enum class GizmoMode
        {
            None = 0,
            Rotation,
            Translation,
            Scale
        };

        enum class ManipulationMode
        {
            None = 0,

            RotateX,
            RotateY,
            RotateZ,
            RotateScreen,

            TranslateX,
            TranslateY,
            TranslateZ,
            TranslateXY,
            TranslateXZ,
            TranslateYZ,

            ScaleX,
            ScaleY,
            ScaleZ,
            ScaleXYZ,
        };

        enum class Options
        {
            DrawManipulationPlanes = 0,
            AllowScale,
            AllowCoordinateSpaceSwitching
        };

        enum class Result
        {
            NoResult = 0,
            StartedManipulating,
            Manipulating,
            StoppedManipulating
        };

    public:

        Gizmo();

        // Draw the gizmo, returns true if any manipulation was performed
        Result Draw( Render::Viewport const& viewport );
        void SetTargetTransform( Transform* pTargetTransform );
        inline Transform const& GetTransform() const { return *m_pTargetTransform; }

        //-------------------------------------------------------------------------

        inline void SetOption( Options option, bool isEnabled ) { m_options.SetFlag( option, isEnabled ); }
        inline bool IsManipulating() const { return m_isManipulating; }

        //-------------------------------------------------------------------------

        inline GizmoMode GetMode() const { return m_gizmoMode; }
        void SwitchMode( GizmoMode newMode );
        void SwitchToNextMode();

        //-------------------------------------------------------------------------

        void SetCoordinateSystemSpace( CoordinateSpace space );
        inline bool IsInWorldSpace() const { return m_coordinateSpace == CoordinateSpace::World; }
        inline bool IsInLocalSpace() const { return m_coordinateSpace == CoordinateSpace::Local; }

    private:

        void ResetState();

        // Rotation
        bool Rotation_DrawScreenGizmo( Render::Viewport const& viewport );
        bool Rotation_DrawWidget( Render::Viewport const& viewport, Vector const& axisOfRotation_WS, Color color );
        void Rotation_DrawManipulationWidget( Render::Viewport const& viewport, Vector const& axisOfRotation_WS, Vector const& axisOfRotation_ss, Color color );
        void Rotation_UpdateMode( Render::Viewport const& viewport );
        void Rotation_PerformManipulation( Render::Viewport const& viewport );
        void Rotation_Update( Render::Viewport const& viewport );

        // Translation
        void Translation_UpdateMode( Render::Viewport const& viewport );
        void Translation_PerformManipulation( Render::Viewport const& viewport );
        void Translation_DrawAndUpdate( Render::Viewport const& viewport );

        // Scale
        void Scale_UpdateMode( Render::Viewport const& viewport );
        void Scale_PerformManipulation( Render::Viewport const& viewport );
        void Scale_DrawAndUpdate( Render::Viewport const& viewport );

        // Draw Helpers
        void DrawAxisGuide( Vector const& axis, Color color );
        bool DrawAxisWidget( Vector const& start, Vector const& end, Color color, AxisCap cap, float originOffset );
        bool DrawPlaneWidget( Vector const& origin, Vector const& axis0, Vector const& axis1, Color color );

    protected:

        Transform*                  m_pTargetTransform = nullptr;
        Transform                   m_manipulationTransform;
        CoordinateSpace             m_coordinateSpace = CoordinateSpace::World;
        GizmoMode                   m_gizmoMode = GizmoMode::None;
        ManipulationMode            m_manipulationMode = ManipulationMode::None;
        bool                        m_isManipulating = false; // Switched whenever we return a "StartManipulating" result

        TBitFlags<Options>          m_options;

        // Updated each frame
        Vector                      m_origin_WS;
        Vector                      m_axisEndPoint_WS_X;
        Vector                      m_axisEndPoint_WS_Y;
        Vector                      m_axisEndPoint_WS_Z;
        Vector                      m_axisDir_WS_X;
        Vector                      m_axisDir_WS_Y;
        Vector                      m_axisDir_WS_Z;

        Vector                      m_origin_SS;
        Vector                      m_axisEndPoint_SS_X;
        Vector                      m_axisEndPoint_SS_Y;
        Vector                      m_axisEndPoint_SS_Z;
        Vector                      m_axisDir_SS_X;
        Vector                      m_axisDir_SS_Y;
        Vector                      m_axisDir_SS_Z;
        float                       m_axisLength_SS_X;
        float                       m_axisLength_SS_Y;
        float                       m_axisLength_SS_Z;
        float                       m_axisScale_SS_X;
        float                       m_axisScale_SS_Y;
        float                       m_axisScale_SS_Z;

        Radians                     m_offsetBetweenViewFwdAndAxis_WS_X;
        Radians                     m_offsetBetweenViewFwdAndAxis_WS_Y;
        Radians                     m_offsetBetweenViewFwdAndAxis_WS_Z;

        bool                        m_shouldDrawAxis_X = false;
        bool                        m_shouldDrawAxis_Y = false;
        bool                        m_shouldDrawAxis_Z = false;

        // Rotation State
        bool                        m_isScreenRotationWidgetHovered = false;
        bool                        m_isAxisRotationWidgetHoveredX = false;
        bool                        m_isAxisRotationWidgetHoveredY = false;
        bool                        m_isAxisRotationWidgetHoveredZ = false;

        Float2                      m_rotationStartMousePosition = Float2::Zero;
        Quaternion                  m_originalStartRotation = Quaternion::Identity;
        Vector                      m_rotationAxis = Vector::Zero;
        Radians                     m_rotationDeltaAngle = Radians( 0.0f );

        // Translation State
        Vector                      m_translationOffset = Vector::Zero;
        bool                        m_isAxisHoveredX = false;
        bool                        m_isAxisHoveredY = false;
        bool                        m_isAxisHoveredZ = false;
        bool                        m_isPlaneHoveredXY = false;
        bool                        m_isPlaneHoveredXZ = false;
        bool                        m_isPlaneHoveredYZ = false;

        // Scale State
        Vector                      m_positionOffset = Vector::Zero;
        bool                        m_isOriginHovered = false;
    };
}
#endif