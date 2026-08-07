#pragma once

#include "Base/_Module/API.h"
#include "Base/Esoterica.h"

#if EE_DEVELOPMENT_TOOLS
#include "Base/Types/String.h"
#include "Base/Types/Arrays.h"

// Fonts
#include "Base/Fonts/Font_Roboto_Regular.h"
#include "Base/Fonts/Font_Roboto_Italic.h"
#include "Base/Fonts/Font_Roboto_Bold.h"
#include "Base/Fonts/Font_Roboto_BoldItalic.h"
#include "Base/Fonts/Font_MaterialDesignIcons.h"

//-------------------------------------------------------------------------
// Base ImGui integration
//-------------------------------------------------------------------------

namespace EE
{
    namespace Input{ class InputSystem; }
}

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    //-------------------------------------------------------------------------
    // Main integration point for DearImGui in Esoterica
    //-------------------------------------------------------------------------

    class EE_BASE_API ImguiSystem final
    {

    public:

        bool Initialize( Input::InputSystem* pInputSystem = nullptr, bool enableViewports = false );
        void Shutdown();

        //-------------------------------------------------------------------------


        void StartFrame( float deltaTime );
        void EndFrame();

    private:

        void InitializePlatform();
        void ShutdownPlatform();
        void PlatformNewFrame();

        void InitializeFonts();
        void ShutdownFonts();

    private:

        Input::InputSystem*             m_pInputSystem = nullptr;
        String                          m_iniFilename;
        String                          m_logFilename;

        Blob const                      m_fontData_regular = Embed::Font_Roboto_Regular::GetFileData();
        Blob const                      m_fontData_italic = Embed::Font_Roboto_Italic::GetFileData();
        Blob const                      m_fontData_bold = Embed::Font_Roboto_Bold::GetFileData();
        Blob const                      m_fontData_bolditalic = Embed::Font_Roboto_BoldItalic::GetFileData();
        Blob const                      m_iconFontData = Embed::Font_MDI::GetFileData();
    };
}
#endif