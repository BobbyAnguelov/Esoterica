#pragma once

#include "../_Module/API.h"
#include "System/Types/String.h"

#if EE_DEVELOPMENT_TOOLS
#include "System/ThirdParty/imgui/imgui.h"

//-------------------------------------------------------------------------
// Base ImGui integration
//-------------------------------------------------------------------------

namespace EE::Render { class RenderDevice; }

namespace EE::ImGuiX
{
    class EE_SYSTEM_API ImguiSystem
    {

    public:

        void StartFrame( float deltaTime );
        void EndFrame();

        bool Initialize( String const& settingsIniFilename, Render::RenderDevice* pRenderDevice, bool enableViewports = false );
        void Shutdown();

    private:

        void InitializeFonts();

    private:

        String                  m_iniFilename;
    };
}
#endif