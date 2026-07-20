#include "ImguiSystem.h"

#if EE_DEVELOPMENT_TOOLS
#include "ImguiXNotifications.h"
#include "ImguiFont.h"
#include "MaterialDesignIcons.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/ThirdParty/implot/implot.h"
#include "Base/Fonts/Font_Roboto_Regular.h"
#include "Base/Fonts/Font_Roboto_Italic.h"
#include "Base/Fonts/Font_Roboto_Bold.h"
#include "Base/Fonts/Font_Roboto_BoldItalic.h"
#include "Base/Fonts/Font_MaterialDesignIcons.h"

//-------------------------------------------------------------------------

#if _WIN32
#include "Base/ThirdParty/imgui/misc/freetype/imgui_freetype.h"
#endif

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    bool ImguiSystem::Initialize( Input::InputSystem* pInputSystem, bool enableViewports )
    {
        m_pInputSystem = pInputSystem;

        //-------------------------------------------------------------------------

        ImGui::CreateContext();
        ImPlot::CreateContext();

        //-------------------------------------------------------------------------

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigWindowsMoveFromTitleBarOnly = true;

        if ( enableViewports )
        {
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            io.ConfigViewportsNoDefaultParent = true;
        }

        //-------------------------------------------------------------------------

        FileSystem::Path const outputDir = FileSystem::GetCurrentProcessPath();

        FileSystem::Path const imguiIniPath = outputDir + "EE.imgui.ini";
        m_iniFilename = imguiIniPath.GetString();

        FileSystem::Path const imguiLogPath = outputDir + "EE.imgui_log.txt";
        m_logFilename = imguiLogPath.GetString();

        io.IniFilename = m_iniFilename.c_str();
        io.LogFilename = m_logFilename.c_str();

        //-------------------------------------------------------------------------

        InitializePlatform();
        InitializeFonts();

        NotificationSystem::Initialize();

        //-------------------------------------------------------------------------

        Style::Apply();

        return true;
    }

    void ImguiSystem::Shutdown()
    {
        NotificationSystem::Shutdown();

        //-------------------------------------------------------------------------

        ShutdownFonts();
        ShutdownPlatform();

        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }

    void ImguiSystem::InitializeFonts()
    {
        ImGuiIO& io = ImGui::GetIO();

        // Decompress fonts
        //-------------------------------------------------------------------------

        Blob const fontData_regular = Embed::Font_Roboto_Regular::GetFileData();
        Blob const fontData_italic = Embed::Font_Roboto_Italic::GetFileData();
        Blob const fontData_bold = Embed::Font_Roboto_Bold::GetFileData();
        Blob const fontData_bolditalic = Embed::Font_Roboto_BoldItalic::GetFileData();

        ImWchar const icons_ranges[] = { EE_ICONRANGE_MIN, EE_ICONRANGE_MAX, 0 };
        Blob const iconFontData = Embed::Font_MDI::GetFileData();

        // Base font configs
        //-------------------------------------------------------------------------

        float const dpiScale = Style::GetMaxDpiScale();
        float const defaultFontSize = SystemFonts::s_fontSizes[2] * dpiScale;

        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        fontConfig.SizePixels = defaultFontSize;

        ImFontConfig iconFontConfig;
        iconFontConfig.FontDataOwnedByAtlas = false;
        iconFontConfig.MergeMode = true;
        iconFontConfig.SizePixels = defaultFontSize;
        iconFontConfig.GlyphOffset = ImVec2( 0, 2 );

        ImFont* pRegularFont = io.Fonts->AddFontFromMemoryTTF( (void*) fontData_regular.data(), (int32_t) fontData_regular.size(), defaultFontSize, &fontConfig);
        io.Fonts->AddFontFromMemoryTTF( (void*) iconFontData.data(), (int32_t) iconFontData.size(), 0.0f, &iconFontConfig, icons_ranges );
        EE_ASSERT( pRegularFont->IsLoaded() );
        SystemFonts::s_fonts[(int32_t) FontType::Regular] = pRegularFont;

        ImFont* pItalicFont = io.Fonts->AddFontFromMemoryTTF( (void*) fontData_italic.data(), (int32_t) fontData_italic.size(), defaultFontSize, &fontConfig );
        io.Fonts->AddFontFromMemoryTTF( (void*) iconFontData.data(), (int32_t) iconFontData.size(), 0.0f, &iconFontConfig, icons_ranges );
        EE_ASSERT( pItalicFont->IsLoaded() );
        SystemFonts::s_fonts[(int32_t) FontType::Italic] = pItalicFont;

        ImFont* pBoldFont = io.Fonts->AddFontFromMemoryTTF( (void*) fontData_bold.data(), (int32_t) fontData_bold.size(), defaultFontSize, &fontConfig );
        io.Fonts->AddFontFromMemoryTTF( (void*) iconFontData.data(), (int32_t) iconFontData.size(), 0.0f, &iconFontConfig );
        EE_ASSERT( pBoldFont->IsLoaded() );
        SystemFonts::s_fonts[(int32_t) FontType::Bold] = pBoldFont;

        ImFont* pBoldItalicFont = io.Fonts->AddFontFromMemoryTTF( (void*) fontData_bolditalic.data(), (int32_t) fontData_bolditalic.size(), defaultFontSize, &fontConfig );
        io.Fonts->AddFontFromMemoryTTF( (void*) iconFontData.data(), (int32_t) iconFontData.size(), 0.0f, &iconFontConfig );
        EE_ASSERT( pBoldItalicFont->IsLoaded() );
        SystemFonts::s_fonts[(int32_t) FontType::BoldItalic] = pBoldItalicFont;

        io.FontDefault = pRegularFont;
    }

    void ImguiSystem::ShutdownFonts()
    {
        SystemFonts::s_fonts[(int32_t) FontType::Regular] = nullptr;
        SystemFonts::s_fonts[(int32_t) FontType::Italic] = nullptr;
        SystemFonts::s_fonts[(int32_t) FontType::Bold] = nullptr;
        SystemFonts::s_fonts[(int32_t) FontType::BoldItalic] = nullptr;
    }

    //-------------------------------------------------------------------------

    void ImguiSystem::StartFrame( float deltaTime )
    {
        // Style Test
        Style::Apply();

        //-------------------------------------------------------------------------

        ImGuiIO& io = ImGui::GetIO();
        io.DeltaTime = Math::Max( deltaTime, Math::Epsilon ); // Imgui assert with 0.0f delta time
        PlatformNewFrame();
        ImGui::NewFrame();
    }

    void ImguiSystem::EndFrame()
    {
        NotificationSystem::Render();
        ImGui::EndFrame();
    }
}
#endif