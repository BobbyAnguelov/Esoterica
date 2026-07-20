#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Viewport/Viewport.h"
#include "Base/Types/BitFlags.h"
#include "Base/Types/Color.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    enum class GizmoState
    {
        None = 0,
        StartedManipulating,
        Manipulating,
        StoppedManipulating
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API GizmoBase
    {
    protected:

        struct Style
        {
            static Color const s_axisColors[3];

        public:

            virtual ~Style() = default;
            virtual void Reset() = 0;
            virtual void SetScale( float scale ) = 0;
        };

        struct Context
        {
            Context( Viewport const& viewport ) : m_viewport( viewport ) {}

        public:

            Viewport const&                 m_viewport;
            Vector                          m_viewDirectionWS;
            Vector                          m_viewPositionWS;
            Vector                          m_positionWS;
            Vector                          m_positionSS;
            Quaternion                      m_rotationWS;
            Vector                          m_mousePositionSS;
            Vector                          m_mouseDeltaSS;
            bool                            m_isMouseInViewport = false;
        };

    public:

        GizmoBase() = default;
        virtual ~GizmoBase() = default;

        virtual void Reset() = 0;
        virtual bool IsManipulating() const = 0;

        GizmoState UpdateAndDraw( Vector const& positionWS, Quaternion const& rotationWS, Viewport const& viewport, char const* pOptionalLabel = nullptr );

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

        float GetPixelLength( Context const& ctx, Vector const startWS, Vector const endWS );
        float PixelHeightToWorldHeight( Context const& ctx, Vector const& position, float pixelHeight );
        float PixelWidthToWorldHeight( Context const& ctx, Vector const& position, float pixelWidth );

    protected:

        CoordinateSpace             m_coordinateSpace = CoordinateSpace::World;
        bool                        m_debugEnabled = false;
    };
}
#endif