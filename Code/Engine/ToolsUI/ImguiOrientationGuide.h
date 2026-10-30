#pragma once
#include "Engine/_Module/API.h"
#include "Base/Math/Math.h"

//-------------------------------------------------------------------------

namespace EE { class Viewport; }

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    // A simple 3D gizmo to show the orientation of a camera in a scene
    struct EE_ENGINE_API OrientationGuide
    {
        constexpr static float const g_windowPadding = 4.0f;
        constexpr static float const g_windowRounding = 2.0f;
        constexpr static float const g_guideDimension = 55.0f;
        constexpr static float const g_axisHeadRadius = 3.0f;
        constexpr static float const g_axisHalfLength = ( g_guideDimension / 2 ) - g_axisHeadRadius - 4.0f;
        constexpr static float const g_worldRenderDistanceZ = 5.0f;
        constexpr static float const g_axisThickness = 2.0f;

    public:

        static Float2 GetSize() { return Float2( g_guideDimension, g_guideDimension ); }
        static float GetWidth() { return g_guideDimension / 2; }
        static void Draw( Float2 const& guideOrigin, Viewport const& viewport );
    };
}
#endif