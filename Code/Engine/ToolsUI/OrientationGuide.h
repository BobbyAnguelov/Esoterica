#pragma once
#include "Engine/_Module/API.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render { class Viewport; }

//-------------------------------------------------------------------------

namespace EE::ImGuiX::OrientationGuide
{
    EE_ENGINE_API Float2 GetSize();
    EE_ENGINE_API float GetWidth();
    EE_ENGINE_API void Draw( Float2 const& guideOrigin, Render::Viewport const& viewport );
}