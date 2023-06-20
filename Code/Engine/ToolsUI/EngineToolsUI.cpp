#include "EngineToolsUI.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Engine/Entity/EntityWorldDebugger.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Player/Systems/WorldSystem_PlayerManager.h"
#include "Engine/Camera/Systems/WorldSystem_CameraManager.h"
#include "Engine/Camera/Components/Component_DebugCamera.h"
#include "System/Logging/LoggingSystem.h"
#include "System/Imgui/ImguiX.h"
#include "System/Input/InputSystem.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    constexpr static float const g_menuHeight = 19.0f;
    constexpr static float const g_minTimeScaleValue = 0.1f;
    constexpr static float const g_maxTimeScaleValue = 3.5f;

    //-------------------------------------------------------------------------

    void EngineToolsUI::Initialize( UpdateContext const& context, ImGuiX::ImageCache* pImageCache )
    {
        m_pWorldManager = context.GetSystem<EntityWorldManager>();
        for ( auto pWorld : m_pWorldManager->GetWorlds() )
        {
            if ( pWorld->IsGameWorld() )
            {
                m_pWorldDebugger = EE::New<EntityWorldDebugger>( pWorld );
                break;
            }
        }
    }

    void EngineToolsUI::Shutdown( UpdateContext const& context )
    {
        EE::Delete( m_pWorldDebugger );
        m_pWorldManager = nullptr;
    }

    //-------------------------------------------------------------------------

    void EngineToolsUI::EndFrame( UpdateContext const& context )
    {
        UpdateStage const updateStage = context.GetUpdateStage();
        EE_ASSERT( updateStage == UpdateStage::FrameEnd );

        // Update internal state
        //-------------------------------------------------------------------------

        float const k = 2.0f / ( context.GetFrameID() + 1 );
        m_averageDeltaTime = context.GetDeltaTime() * k + m_averageDeltaTime * ( 1.0f - k );

        // Get game world
        //-------------------------------------------------------------------------

        EntityWorld* pGameWorld = nullptr;
        for ( auto pWorld : m_pWorldManager->GetWorlds() )
        {
            if ( pWorld->IsGameWorld() )
            {
                pGameWorld = pWorld;
                break;
            }
        }

        if ( pGameWorld == nullptr )
        {
            return;
        }

        // Process user input
        //-------------------------------------------------------------------------

        HandleUserInput( context, pGameWorld );

        // Draw overlay window
        //-------------------------------------------------------------------------

        Render::Viewport const* pViewport = pGameWorld->GetViewport();

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus;
        if ( m_debugOverlayEnabled )
        {
            windowFlags |= ImGuiWindowFlags_MenuBar;
        }
        
        ImVec2 windowPos, windowSize;

        if ( !m_windowName.empty() )
        {
            auto pWindow = ImGui::FindWindowByName( m_windowName.c_str() );
            EE_ASSERT( pWindow != nullptr );
            windowPos = pWindow->Pos;
            windowSize = pWindow->Size;
        }
        else
        {
            ImGuiViewport const* viewport = ImGui::GetMainViewport();
            windowPos = viewport->WorkPos;
            windowSize = viewport->WorkSize;
        }

        ImGui::SetNextWindowPos( windowPos );
        ImGui::SetNextWindowSize( windowSize );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
        ImGui::SetNextWindowBgAlpha( 0.0f );
        bool const shouldDrawOverlayWindow = ImGui::Begin( "ViewportOverlay", nullptr, windowFlags );
        ImGui::PopStyleVar( 2 );

        if ( shouldDrawOverlayWindow )
        {
            // The overlay elements should always be drawn
            DrawOverlayElements( context, pViewport );

            //-------------------------------------------------------------------------

            if ( m_debugOverlayEnabled )
            {
                if ( ImGui::BeginMenuBar() )
                {
                    DrawMenu( context, pGameWorld );
                    ImGui::EndMenuBar();
                }
            }
        }
        ImGui::End();

        // The debug windows should be always be drawn if enabled
        DrawWindows( context, pGameWorld );

        // Always show the log if any errors/warnings occurred
        auto const unhandledWarningsAndErrors = Log::System::GetUnhandledWarningsAndErrors();
        if ( !unhandledWarningsAndErrors.empty() )
        {
            m_isLogWindowOpen = true;
        }
    }

    //-------------------------------------------------------------------------
    // Drawing
    //-------------------------------------------------------------------------

    void EngineToolsUI::DrawMenu( UpdateContext const& context, EntityWorld* pGameWorld )
    {
        ImVec2 const totalAvailableSpace = ImGui::GetContentRegionAvail();

        float const currentFPS = 1.0f / context.GetDeltaTime();
        float const allocatedMemory = Memory::GetTotalAllocatedMemory() / 1024.0f / 1024.0f;

        TInlineString<10> const warningsStr( TInlineString<10>::CtorSprintf(), EE_ICON_ALERT" %d", Log::System::GetNumWarnings() );
        TInlineString<10> const errorsStr( TInlineString<10>::CtorSprintf(), EE_ICON_ALERT_CIRCLE_OUTLINE" %d", Log::System::GetNumErrors() );
        TInlineString<40> const perfStatsStr( TInlineString<40>::CtorSprintf(), "FPS: %3.0f", currentFPS );
        TInlineString<40> const memStatsStr( TInlineString<40>::CtorSprintf(), "MEM: %.2fMB", allocatedMemory );

        ImVec2 const warningsTextSize = ImGui::CalcTextSize( warningsStr.c_str() );
        ImVec2 const errorTextSize = ImGui::CalcTextSize( errorsStr.c_str() );
        ImVec2 const memStatsTextSize = ImGui::CalcTextSize( memStatsStr.c_str() );
        ImVec2 const perfStatsTextSize = ImVec2( 64, 0 ) ;

        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const framePadding = ImGui::GetStyle().FramePadding.x;
        float const memStatsOffset = totalAvailableSpace.x - memStatsTextSize.x - ( itemSpacing );
        float const perfStatsOffset = memStatsOffset - perfStatsTextSize.x;
        float const warningsAndErrorsOffset = perfStatsOffset - warningsTextSize.x - errorTextSize.x - ( itemSpacing * 6 ) - ( framePadding * 4 );

        constexpr static float const gapSize = 13;
        constexpr static float const menuButtonSize = 35;

        float const frameLimiterOffset = warningsAndErrorsOffset - menuButtonSize;
        float const debugCameraOffset = frameLimiterOffset - menuButtonSize;
        float const timeControlsCameraOffset = debugCameraOffset - menuButtonSize;

        // Draw Debug Views
        //-------------------------------------------------------------------------

        m_pWorldDebugger->DrawMenu( context );

        // Time Controls
        //-------------------------------------------------------------------------

        ImGui::SameLine( timeControlsCameraOffset, 0 );

        ImGui::PushStyleColor( ImGuiCol_Text, Colors::LimeGreen.ToFloat4() );
        bool const drawDebugMenu = ImGui::BeginMenu( EE_ICON_CLOCK_FAST );
        ImGui::PopStyleColor();
        if ( drawDebugMenu )
        {
            ImGuiX::TextSeparator( EE_ICON_CLOCK" Time Controls" );
            {
                ImVec2 const buttonSize( 24, 0 );

                // Play/Pause
                if ( pGameWorld->IsPaused() )
                {
                    if ( ImGui::Button( EE_ICON_PLAY"##ResumeWorld", buttonSize ) )
                    {
                        ToggleWorldPause( pGameWorld );
                    }
                    ImGuiX::ItemTooltip( "Resume" );
                }
                else
                {
                    if ( ImGui::Button( EE_ICON_PAUSE"##PauseWorld", buttonSize ) )
                    {
                        ToggleWorldPause( pGameWorld );
                    }
                    ImGuiX::ItemTooltip( "Pause" );
                }

                // Step
                ImGui::SameLine();
                ImGui::BeginDisabled( !pGameWorld->IsPaused() );
                if ( ImGui::Button( EE_ICON_ARROW_RIGHT_BOLD"##StepFrame", buttonSize ) )
                {
                    RequestWorldTimeStep( pGameWorld );
                }
                ImGuiX::ItemTooltip( "Step Frame" );
                ImGui::EndDisabled();

                // Slider
                ImGui::SameLine();
                ImGui::SetNextItemWidth( 100 );
                float currentTimeScale = m_timeScale;
                if ( ImGui::SliderFloat( "##TimeScale", &currentTimeScale, g_minTimeScaleValue, g_maxTimeScaleValue, "%.2f", ImGuiSliderFlags_NoInput ) )
                {
                    SetWorldTimeScale( pGameWorld, currentTimeScale );
                }
                ImGuiX::ItemTooltip( "Time Scale" );

                // Reset
                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_UNDO"##ResetTimeScale", buttonSize ) )
                {
                    ResetWorldTimeScale( pGameWorld );
                }
                ImGuiX::ItemTooltip( "Reset TimeScale" );
            }

            ImGui::EndMenu();
        }
        ImGuiX::ItemTooltip( "World Time Controls" );

        // Player Debug Options
        //-------------------------------------------------------------------------

        ImGuiX::SameLineSeparator( gapSize );

        if ( ImGui::BeginMenu( EE_ICON_CONTROLLER_CLASSIC "##PlayerDebugOptions") )
        {
            DrawPlayerDebugOptionsMenu( context, pGameWorld );
            ImGui::EndMenu();
        }
        ImGuiX::ItemTooltip( "Player Debug Options" );

        ImGuiX::SameLineSeparator( gapSize );

        SystemDebugView::DrawFrameLimiterCombo( const_cast<UpdateContext&>( context ) );
        ImGuiX::ItemTooltip( "Frame Rate Limiter" );

        // Log
        //-------------------------------------------------------------------------

        ImGuiX::SameLineSeparator( gapSize );

        ImGui::PushStyleColor( ImGuiCol_Text, ImGuiX::ImColors::Yellow.Value );
        if ( ImGuiX::FlatButton( warningsStr.c_str() ) )
        {
            m_isLogWindowOpen = true;
            m_systemLogView.m_showLogMessages = false;
            m_systemLogView.m_showLogWarnings = true;
            m_systemLogView.m_showLogErrors = false;
        }
        ImGui::PopStyleColor();
        ImGuiX::ItemTooltip( "Warnings" );

        ImGuiX::SameLineSeparator( gapSize );

        ImGui::PushStyleColor( ImGuiCol_Text, ImGuiX::ImColors::Red.Value );
        if ( ImGuiX::FlatButton( errorsStr.c_str() ) )
        {
            m_isLogWindowOpen = true;
            m_systemLogView.m_showLogMessages = false;
            m_systemLogView.m_showLogWarnings = false;
            m_systemLogView.m_showLogErrors = true;
        }
        ImGui::PopStyleColor();
        ImGuiX::ItemTooltip( "Errors" );

        ImGuiX::SameLineSeparator( gapSize );

        // Draw Performance Stats
        //-------------------------------------------------------------------------

        ImGui::Text( perfStatsStr.c_str() );

        ImGui::SameLine();
        ImGui::Text( memStatsStr.c_str() );
    }

    void EngineToolsUI::DrawOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        m_pWorldDebugger->DrawOverlayElements( context );

        if ( m_debugOverlayEnabled )
        {
            ImVec2 const guidePosition = ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMax() - ( ImGuiX::OrientationGuide::GetSize() / 2 );
            ImGuiX::OrientationGuide::Draw( guidePosition, *pViewport );
        }
    }

    void EngineToolsUI::DrawWindows( UpdateContext const& context, EntityWorld* pGameWorld, ImGuiWindowClass* pWindowClass )
    {
        m_pWorldDebugger->DrawWindows( context, pWindowClass );

        //-------------------------------------------------------------------------

        if ( m_isLogWindowOpen )
        {
            ImGui::SetNextWindowBgAlpha( 0.75f );
            m_isLogWindowOpen = m_systemLogView.Draw( context );
        }
    }

    void EngineToolsUI::BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToReload, TVector<ResourceID> const& resourcesToBeReloaded )
    {
        m_pWorldDebugger->BeginHotReload( usersToReload, resourcesToBeReloaded );
    }

    void EngineToolsUI::EndHotReload()
    {
        m_pWorldDebugger->EndHotReload();
    }

    void EngineToolsUI::DrawPlayerDebugOptionsMenu( UpdateContext const& context, EntityWorld* pGameWorld )
    {
        auto pPlayerManager = pGameWorld->GetWorldSystem<PlayerManager>();
        auto pCameraManager = pGameWorld->GetWorldSystem<CameraManager>();

        bool const hasValidPlayer = pPlayerManager->HasPlayer();

        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( !hasValidPlayer );

        bool isControllerEnabled = hasValidPlayer ? pPlayerManager->IsPlayerEnabled() : false;
        if ( ImGui::Checkbox( EE_ICON_MICROSOFT_XBOX_CONTROLLER " Enable Player Controller", &isControllerEnabled ) )
        {
            pPlayerManager->SetPlayerControllerState( isControllerEnabled );
        }

        //-------------------------------------------------------------------------

        bool isDebugCameraEnabled = pCameraManager->IsDebugCameraEnabled();
        if ( ImGui::Checkbox( EE_ICON_CCTV " Enable Debug Camera", &isDebugCameraEnabled ) )
        {
            if ( isDebugCameraEnabled )
            {
                CameraComponent const* pPlayerCamera = pPlayerManager->GetPlayerCamera();
                pCameraManager->EnableDebugCamera( pPlayerCamera->GetPosition(), pPlayerCamera->GetForwardVector() );
            }
            else
            {
                pCameraManager->DisableDebugCamera();
            }
        }

        ImGui::EndDisabled();
    }

    //-------------------------------------------------------------------------

    void EngineToolsUI::HandleUserInput( UpdateContext const& context, EntityWorld* pGameWorld )
    {
        auto pInputSystem = context.GetSystem<Input::InputSystem>();
        EE_ASSERT( pInputSystem != nullptr );

        // Enable/disable debug overlay
        //-------------------------------------------------------------------------

        auto const pKeyboardState = pInputSystem->GetKeyboardState();
        if ( pKeyboardState->WasReleased( Input::KeyboardButton::Key_Tilde ) )
        {
            m_debugOverlayEnabled = !m_debugOverlayEnabled;
        }

        // Time Controls
        //-------------------------------------------------------------------------

        if ( pKeyboardState->WasReleased( Input::KeyboardButton::Key_Pause ) )
        {
            ToggleWorldPause( pGameWorld );
        }

        if ( pKeyboardState->WasReleased( Input::KeyboardButton::Key_PageUp ) )
        {
            SetWorldTimeScale( pGameWorld, m_timeScale + 0.1f );
        }

        if ( pKeyboardState->WasReleased( Input::KeyboardButton::Key_PageDown ) )
        {
            SetWorldTimeScale( pGameWorld, m_timeScale - 0.1f );
        }

        if ( pKeyboardState->WasReleased( Input::KeyboardButton::Key_Home ) )
        {
            ResetWorldTimeScale( pGameWorld );
        }

        if ( pKeyboardState->WasReleased( Input::KeyboardButton::Key_End ) )
        {
            RequestWorldTimeStep( pGameWorld );
        }
    }

    void EngineToolsUI::ToggleWorldPause( EntityWorld* pGameWorld )
    {
        // Unpause
        if ( pGameWorld->IsPaused() )
        {
            pGameWorld->SetTimeScale( m_timeScale );
        }
        else // Pause
        {
            m_timeScale = pGameWorld->GetTimeScale();
            pGameWorld->SetTimeScale( -1.0f );
        }
    }

    void EngineToolsUI::SetWorldTimeScale( EntityWorld* pGameWorld, float newTimeScale )
    {
        m_timeScale = Math::Clamp( newTimeScale, g_minTimeScaleValue, g_maxTimeScaleValue );
        pGameWorld->SetTimeScale( m_timeScale );
    }

    void EngineToolsUI::ResetWorldTimeScale( EntityWorld* pGameWorld )
    {
        m_timeScale = 1.0f;

        if ( !pGameWorld->IsPaused() )
        {
            pGameWorld->SetTimeScale( m_timeScale );
        }
    }

    void EngineToolsUI::RequestWorldTimeStep( EntityWorld* pGameWorld )
    {
        if ( pGameWorld->IsPaused() )
        {
            pGameWorld->RequestTimeStep();
        }
    }
}
#endif