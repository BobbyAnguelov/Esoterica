#include "ImguiSystem.h"

#if EE_DEVELOPMENT_TOOLS
#include "ImguiXNotifications.h"
#include "Base/Render/RenderDevice.h"
#include "Base/Fonts/FontDecompressor.h"
#include "Base/Fonts/FontData_Lexend.h"
#include "Base/Fonts/FontData_MaterialDesign.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/ThirdParty/implot/implot.h"

//-------------------------------------------------------------------------

#if _WIN32
#include "Base/ThirdParty/imgui/misc/freetype/imgui_freetype.h"
#endif

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    bool ImguiSystem::Initialize( Render::RenderDevice* pRenderDevice, Input::InputSystem* pInputSystem, bool enableViewports )
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

        // Set render device in the user data
        EE_ASSERT( pRenderDevice != nullptr );
        io.BackendRendererUserData = pRenderDevice;

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

        m_imageCache.Initialize( pRenderDevice );
        NotificationSystem::Initialize();

        //-------------------------------------------------------------------------

        Style::Apply();

        return true;
    }

    void ImguiSystem::Shutdown()
    {
        NotificationSystem::Shutdown();
        m_imageCache.Shutdown();

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

        Blob fontData, boldFontData;
        Fonts::GetDecompressedFontData( Fonts::Lexend::Regular::GetData(), fontData );
        Fonts::GetDecompressedFontData( Fonts::Lexend::Bold::GetData(), boldFontData );

        ImWchar const icons_ranges[] = { EE_ICONRANGE_MIN, EE_ICONRANGE_MAX, 0 };
        Blob iconFontData;
        Fonts::GetDecompressedFontData( (uint8_t const*) Fonts::MaterialDesignIcons::GetData(), iconFontData );

        // Base font configs
        //-------------------------------------------------------------------------

        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;

        ImFontConfig iconFontConfig;
        iconFontConfig.FontDataOwnedByAtlas = false;
        iconFontConfig.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LoadColor | ImGuiFreeTypeBuilderFlags_Bitmap;
        iconFontConfig.MergeMode = true;
        iconFontConfig.PixelSnapH = true;
        iconFontConfig.RasterizerMultiply = 1.5f;

        auto CreateFont = [&] ( Blob& fontData, float fontSize, float iconFontSize, Font fontID, char const* pName, ImVec2 const& glyphOffset )
        {
            Printf( fontConfig.Name, 40, pName );
            ImFont* pFont = io.Fonts->AddFontFromMemoryTTF( fontData.data(), (int32_t) fontData.size(), fontSize, &fontConfig );
            SystemFonts::s_fonts[(uint8_t) fontID] = pFont;

            iconFontConfig.GlyphOffset = glyphOffset;
            iconFontConfig.GlyphMinAdvanceX = iconFontSize;
            io.Fonts->AddFontFromMemoryTTF( iconFontData.data(), (int32_t) iconFontData.size(), iconFontSize, &iconFontConfig, icons_ranges );
        };

        constexpr float const DPIScale = 1.0f;
        float const size12 = Math::Floor( 12 * DPIScale );
        float const size14 = Math::Floor( 14 * DPIScale );
        float const size16 = Math::Floor( 16 * DPIScale );
        float const size18 = Math::Floor( 18 * DPIScale );
        float const size24 = Math::Floor( 24 * DPIScale );

        CreateFont( fontData, size12, size14, Font::Tiny, "Tiny", ImVec2( 0, 2 ) );
        CreateFont( boldFontData, size12, size14, Font::TinyBold, "Tiny Bold", ImVec2( 0, 2 ) );

        CreateFont( fontData, size14, size16, Font::Small, "Small", ImVec2( 0, 2 ) );
        CreateFont( boldFontData, size14, size16, Font::SmallBold, "Small Bold", ImVec2( 0, 2 ) );

        CreateFont( fontData, size16, size18, Font::Medium, "Medium", ImVec2( 0, 2 ) );
        CreateFont( boldFontData, size16, size18, Font::MediumBold, "Medium Bold", ImVec2( 0, 2 ) );

        CreateFont( fontData, size24, size24, Font::Large, "Large", ImVec2( 0, 2 ) );
        CreateFont( boldFontData, size24, size24, Font::LargeBold, "Large Bold", ImVec2( 0, 2 ) );

        // Build font atlas
        //-------------------------------------------------------------------------

        io.Fonts->TexDesiredWidth = 4096;
        io.Fonts->Build();
        EE_ASSERT( io.Fonts->IsBuilt() );

        #if EE_DEVELOPMENT_TOOLS
        EE_ASSERT( SystemFonts::s_fonts[(uint8_t) Font::Small]->IsLoaded() );
        EE_ASSERT( SystemFonts::s_fonts[(uint8_t) Font::SmallBold]->IsLoaded() );
        EE_ASSERT( SystemFonts::s_fonts[(uint8_t) Font::Medium]->IsLoaded() );
        EE_ASSERT( SystemFonts::s_fonts[(uint8_t) Font::MediumBold]->IsLoaded() );
        EE_ASSERT( SystemFonts::s_fonts[(uint8_t) Font::Large]->IsLoaded() );
        EE_ASSERT( SystemFonts::s_fonts[(uint8_t) Font::LargeBold]->IsLoaded() );
        #endif

        io.FontDefault = SystemFonts::s_fonts[(uint8_t) Font::Medium];
    }

    void ImguiSystem::ShutdownFonts()
    {
        for ( int i = 0; i < (int8_t) Font::NumFonts; i++ )
        {
            SystemFonts::s_fonts[i] = nullptr;
        }
    }

    //-------------------------------------------------------------------------

    void ImguiSystem::StartFrame( float deltaTime )
    {
        // Style Test
        Style::Apply();

        //-------------------------------------------------------------------------

        ImGuiIO& io = ImGui::GetIO();
        io.DeltaTime = deltaTime;
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