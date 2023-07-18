#include "EngineDebugUI.h"
#include "Engine/DebugViews/DebugView.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Player/Systems/WorldSystem_PlayerManager.h"
#include "Engine/Camera/Systems/WorldSystem_CameraManager.h"
#include "Engine/Camera/Components/Component_DebugCamera.h"
#include "Base/Logging/LoggingSystem.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Input/InputSystem.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    constexpr static float const g_menuHeight = 19.0f;
    constexpr static float const g_minTimeScaleValue = 0.1f;
    constexpr static float const g_maxTimeScaleValue = 3.5f;

    //-------------------------------------------------------------------------

    EngineDebugUI::Menu::MenuPath EngineDebugUI::Menu::CreatePathFromString( String const& pathString )
    {
        EE_ASSERT( !pathString.empty() );
        size_t const categoryNameLength = pathString.length();

        // Validation
        EE_ASSERT( categoryNameLength < 256 );
        if ( categoryNameLength > 0 )
        {
            for ( auto i = 0; i < categoryNameLength - 1; i++ )
            {
                EE_ASSERT( isalnum( pathString[i] ) || pathString[i] == ' ' || pathString[i] == '/' );
            }
        }

        //-------------------------------------------------------------------------

        MenuPath path;
        StringUtils::Split( pathString, path, "/" );
        return path;
    }

    void EngineDebugUI::Menu::Clear()
    {
        for ( auto& childMenu : m_childMenus )
        {
            childMenu.Clear();
        }

        m_childMenus.clear();
        m_debugViews.clear();
        m_title.clear();
    }

    void EngineDebugUI::Menu::AddChildMenu( DebugView* pDebugView )
    {
        EE_ASSERT( !TryFindMenu( pDebugView ) );

        MenuPath const menuPath = CreatePathFromString( pDebugView->m_menuPath );
        auto& subMenu = FindOrAddMenu( menuPath );

        // Add new callback and sort callback list
        //-------------------------------------------------------------------------

        subMenu.m_debugViews.emplace_back( pDebugView );

        auto callbackSortPredicate = [] ( DebugView* const& pA, DebugView* const& pB )
        {
            return pA->m_menuPath < pB->m_menuPath;
        };

        eastl::sort( subMenu.m_debugViews.begin(), subMenu.m_debugViews.end(), callbackSortPredicate );
    }

    void EngineDebugUI::Menu::RemoveChildMenu( DebugView* pDebugView )
    {
        EE_ASSERT( pDebugView != nullptr );
        EE_ASSERT( pDebugView->HasMenu() );
        bool const result = TryFindAndRemoveMenu( pDebugView );
        EE_ASSERT( result );
    }

    EngineDebugUI::Menu& EngineDebugUI::Menu::FindOrAddMenu( MenuPath const& path )
    {
        EE_ASSERT( !path.empty() );

        int32_t currentPathLevel = 0;

        Menu* pCurrentMenu = this;
        while ( true )
        {
            // Check all child menus for a menu that matches the current path level name
            bool childMenuFound = false;
            for ( auto& childMenu : pCurrentMenu->m_childMenus )
            {
                // If the child menu matches the title, continue the search from the child menu
                if ( childMenu.m_title == path[currentPathLevel] )
                {
                    pCurrentMenu = &childMenu;
                    currentPathLevel++;

                    if ( currentPathLevel == path.size() )
                    {
                        return childMenu;
                    }
                    else
                    {
                        childMenuFound = true;
                        break;
                    }
                }
            }

            //-------------------------------------------------------------------------

            // If we didn't find a matching menu add a new one and redo the search
            if ( !childMenuFound )
            {
                // Add new child menu and sort the child menu list
                pCurrentMenu->m_childMenus.emplace_back( Menu( path[currentPathLevel] ) );

                auto menuSortPredicate = [] ( Menu const& menuA, Menu const& menuB )
                {
                    return menuA.m_title < menuB.m_title;
                };

                eastl::sort( pCurrentMenu->m_childMenus.begin(), pCurrentMenu->m_childMenus.end(), menuSortPredicate );

                // Do not change current menu or current path level, we will redo the child menu 
                // search in the next loop update and find the new menu, this is simpler than trying
                // to sort, and redo the search here
            }
        }

        //-------------------------------------------------------------------------

        EE_UNREACHABLE_CODE();
        return *this;
    }

    bool EngineDebugUI::Menu::TryFindMenu( DebugView* pDebugView )
    {
        EE_ASSERT( pDebugView != nullptr );
        EE_ASSERT( pDebugView->HasMenu() );

        auto iter = VectorFind( m_debugViews, pDebugView );
        if ( iter != m_debugViews.end() )
        {
            return true;
        }
        else
        {
            for ( auto& childMenu : m_childMenus )
            {
                if ( childMenu.TryFindMenu( pDebugView ) )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool EngineDebugUI::Menu::TryFindAndRemoveMenu( DebugView* pDebugView )
    {
        EE_ASSERT( pDebugView != nullptr );
        EE_ASSERT( pDebugView->HasMenu() );

        auto iter = VectorFind( m_debugViews, pDebugView );
        if ( iter != m_debugViews.end() )
        {
            m_debugViews.erase( iter );
            return true;
        }
        else
        {
            for ( auto& childMenu : m_childMenus )
            {
                if ( childMenu.TryFindAndRemoveMenu( pDebugView ) )
                {
                    RemoveEmptyChildMenus();
                    return true;
                }
            }
        }

        return false;
    }

    void EngineDebugUI::Menu::RemoveEmptyChildMenus()
    {
        for ( auto iter = m_childMenus.begin(); iter != m_childMenus.end(); )
        {
            ( *iter ).RemoveEmptyChildMenus();

            if ( ( *iter ).IsEmpty() )
            {
                iter = m_childMenus.erase( iter );
            }
            else
            {
                ++iter;
            }
        }
    }

    void EngineDebugUI::Menu::Draw( EntityWorldUpdateContext const& context )
    {
        for ( auto& childMenu : m_childMenus )
        {
            EE_ASSERT( !childMenu.IsEmpty() );

            if ( ImGui::BeginMenu( childMenu.m_title.c_str() ) )
            {
                childMenu.Draw( context );
                ImGui::EndMenu();
            }
        }

        //-------------------------------------------------------------------------

        for ( auto& pDebugView : m_debugViews )
        {
            pDebugView->DrawMenu( context );
        }
    }

    void EngineDebugUI::EditorPreviewUpdate( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        m_isInEditorPreviewMode = true;
        m_pWindowClass = pWindowClass;
        EndFrame( context );
    }

    //-------------------------------------------------------------------------

    void EngineDebugUI::Initialize( UpdateContext const& context, ImGuiX::ImageCache* pImageCache )
    {
        auto pWorldManager = context.GetSystem<EntityWorldManager>();
        for ( auto pWorld : pWorldManager->GetWorlds() )
        {
            if ( pWorld->IsGameWorld() )
            {
                m_pGameWorld = pWorld;
                break;
            }
        }

        //-------------------------------------------------------------------------

        if ( m_pGameWorld != nullptr )
        {
            auto pTypeRegistry = context.GetSystem<TypeSystem::TypeRegistry>();
            TVector<TypeSystem::TypeInfo const*> debugViewTypeInfos = pTypeRegistry->GetAllDerivedTypes( DebugView::GetStaticTypeID(), false, false, true );

            for ( auto pTypeInfo : debugViewTypeInfos )
            {
                auto pDebugView = Cast<DebugView>( pTypeInfo->CreateType() );
                pDebugView->Initialize( *context.GetSystemRegistry(), m_pGameWorld );
                m_debugViews.push_back( pDebugView );

                if ( pDebugView->HasMenu() )
                {
                    m_mainMenu.AddChildMenu( pDebugView );
                }
            }
        }
    }

    void EngineDebugUI::Shutdown( UpdateContext const& context )
    {
        m_mainMenu.Clear();

        for ( auto pDebugView : m_debugViews )
        {
            pDebugView->Shutdown();
            EE::Delete( pDebugView );
        }

        m_pGameWorld = nullptr;
    }

    //-------------------------------------------------------------------------

    void EngineDebugUI::EndFrame( UpdateContext const& context )
    {
        UpdateStage const updateStage = context.GetUpdateStage();
        EE_ASSERT( updateStage == UpdateStage::FrameEnd );

        // Update internal state
        //-------------------------------------------------------------------------

        float const k = 2.0f / ( context.GetFrameID() + 1 );
        m_averageDeltaTime = context.GetDeltaTime() * k + m_averageDeltaTime * ( 1.0f - k );

        // Get game world
        //-------------------------------------------------------------------------

        if ( m_pGameWorld == nullptr )
        {
            return;
        }

        // Always show the log if any errors/warnings occurred
        //-------------------------------------------------------------------------

        auto const unhandledWarningsAndErrors = Log::System::GetUnhandledWarningsAndErrors();
        if ( !unhandledWarningsAndErrors.empty() )
        {
            if ( auto pSystemDebugView = FindDebugView<SystemDebugView>() )
            {
                pSystemDebugView->m_windows[0].m_isOpen = true;
                pSystemDebugView->m_logView.m_showLogMessages = true;
                pSystemDebugView->m_logView.m_showLogWarnings = true;
                pSystemDebugView->m_logView.m_showLogErrors = true;
            }
        }

        // Process user input
        //-------------------------------------------------------------------------

        HandleUserInput( context );

        // Update all debug views
        //-------------------------------------------------------------------------

        EntityWorldUpdateContext worldUpdateContext( context, m_pGameWorld );
        for ( auto pDebugView : m_debugViews )
        {
            pDebugView->Update( worldUpdateContext );
        }

        // Draw overlay window and menu
        //-------------------------------------------------------------------------

        if ( !m_isInEditorPreviewMode )
        {
            Render::Viewport const* pViewport = m_pGameWorld->GetViewport();

            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus;
            if ( m_debugOverlayEnabled )
            {
                windowFlags |= ImGuiWindowFlags_MenuBar;
            }

            ImGuiViewport const* viewport = ImGui::GetMainViewport();
            ImVec2 const windowPos = viewport->WorkPos;
            ImVec2 const windowSize = viewport->WorkSize;

            ImGui::SetNextWindowPos( windowPos );
            ImGui::SetNextWindowSize( windowSize );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
            ImGui::SetNextWindowBgAlpha( 0.0f );
            bool const shouldDrawOverlayWindow = ImGui::Begin( "ViewportOverlay", nullptr, windowFlags );
            ImGui::PopStyleVar( 2 );

            // Draw Menu and overlay elements
            //-------------------------------------------------------------------------

            if ( shouldDrawOverlayWindow )
            {
                // The overlay elements should always be drawn
                DrawOverlayElements( context, pViewport );

                //-------------------------------------------------------------------------

                if ( m_debugOverlayEnabled )
                {
                    if ( ImGui::BeginMenuBar() )
                    {
                        DrawMenu( context );
                        ImGui::EndMenuBar();
                    }
                }
            }

            ImGui::End();
        }

        // Draw all debug windows
        //-------------------------------------------------------------------------

        for ( auto pDebugView : m_debugViews )
        {
            for ( auto& window : pDebugView->m_windows )
            {
                if ( !window.m_isOpen )
                {
                    continue;
                }

                if ( m_pWindowClass != nullptr )
                {
                    ImGui::SetNextWindowClass( m_pWindowClass );
                }

                ImGui::SetNextWindowBgAlpha( window.m_alpha );
                if ( ImGui::Begin( window.m_name.c_str(), &window.m_isOpen ) )
                {
                    bool const isWindowFocused = ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_DockHierarchy );
                    window.m_drawFunction( worldUpdateContext, isWindowFocused, window.m_userData );
                }
                ImGui::End();
            }
        }

        //-------------------------------------------------------------------------

        m_isInEditorPreviewMode = false;
        m_pWindowClass = nullptr;
    }

    //-------------------------------------------------------------------------
    // Drawing
    //-------------------------------------------------------------------------

    void EngineDebugUI::DrawMenu( UpdateContext const& context )
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

        if ( m_mainMenu.IsEmpty() )
        {
            ImGui::Text( "No Menu Options" );
        }
        else
        {
            EntityWorldUpdateContext worldUpdateContext( context, m_pGameWorld );
            m_mainMenu.Draw( worldUpdateContext );
        }

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
                if ( m_pGameWorld->IsPaused() )
                {
                    if ( ImGui::Button( EE_ICON_PLAY"##ResumeWorld", buttonSize ) )
                    {
                        ToggleWorldPause();
                    }
                    ImGuiX::ItemTooltip( "Resume" );
                }
                else
                {
                    if ( ImGui::Button( EE_ICON_PAUSE"##PauseWorld", buttonSize ) )
                    {
                        ToggleWorldPause();
                    }
                    ImGuiX::ItemTooltip( "Pause" );
                }

                // Step
                ImGui::SameLine();
                ImGui::BeginDisabled( !m_pGameWorld->IsPaused() );
                if ( ImGui::Button( EE_ICON_ARROW_RIGHT_BOLD"##StepFrame", buttonSize ) )
                {
                    RequestWorldTimeStep();
                }
                ImGuiX::ItemTooltip( "Step Frame" );
                ImGui::EndDisabled();

                // Slider
                ImGui::SameLine();
                ImGui::SetNextItemWidth( 100 );
                float currentTimeScale = m_timeScale;
                if ( ImGui::SliderFloat( "##TimeScale", &currentTimeScale, g_minTimeScaleValue, g_maxTimeScaleValue, "%.2f", ImGuiSliderFlags_NoInput ) )
                {
                    SetWorldTimeScale( currentTimeScale );
                }
                ImGuiX::ItemTooltip( "Time Scale" );

                // Reset
                ImGui::SameLine();
                if ( ImGui::Button( EE_ICON_UNDO"##ResetTimeScale", buttonSize ) )
                {
                    ResetWorldTimeScale();
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
            DrawPlayerDebugOptionsMenu( context );
            ImGui::EndMenu();
        }
        ImGuiX::ItemTooltip( "Player Debug Options" );

        ImGuiX::SameLineSeparator( gapSize );

        SystemDebugView::DrawFrameLimiterCombo( const_cast<UpdateContext&>( context ) );
        ImGuiX::ItemTooltip( "Frame Rate Limiter" );

        // Log
        //-------------------------------------------------------------------------

        ImGuiX::SameLineSeparator( gapSize );

        ImGui::PushStyleColor( ImGuiCol_Text, Colors::Yellow );
        if ( ImGuiX::FlatButton( warningsStr.c_str() ) )
        {
            if ( auto pSystemDebugView = FindDebugView<SystemDebugView>() )
            {
                pSystemDebugView->m_windows[0].m_isOpen = true;
                pSystemDebugView->m_logView.m_showLogMessages = false;
                pSystemDebugView->m_logView.m_showLogWarnings = true;
                pSystemDebugView->m_logView.m_showLogErrors = false;
            }
        }
        ImGui::PopStyleColor();
        ImGuiX::ItemTooltip( "Warnings" );

        ImGuiX::SameLineSeparator( gapSize );

        ImGui::PushStyleColor( ImGuiCol_Text, Colors::Red );
        if ( ImGuiX::FlatButton( errorsStr.c_str() ) )
        {
            if ( auto pSystemDebugView = FindDebugView<SystemDebugView>() )
            {
                pSystemDebugView->m_windows[0].m_isOpen = true;
                pSystemDebugView->m_logView.m_showLogMessages = false;
                pSystemDebugView->m_logView.m_showLogWarnings = false;
                pSystemDebugView->m_logView.m_showLogErrors = true;
            }
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

    void EngineDebugUI::DrawOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        EE_ASSERT( context.GetUpdateStage() == UpdateStage::FrameEnd );

        EntityWorldUpdateContext worldUpdateContext( context, m_pGameWorld );
        for ( auto pDebugView : m_debugViews )
        {
            pDebugView->DrawOverlayElements( worldUpdateContext );
        }

        if ( m_debugOverlayEnabled )
        {
            ImVec2 const guidePosition = ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMax() - ( ImGuiX::OrientationGuide::GetSize() / 2 );
            ImGuiX::OrientationGuide::Draw( guidePosition, *pViewport );
        }
    }

    void EngineDebugUI::BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToReload, TVector<ResourceID> const& resourcesToBeReloaded )
    {
        for ( auto pDebugView : m_debugViews )
        {
            pDebugView->BeginHotReload( usersToReload, resourcesToBeReloaded );
        }
    }

    void EngineDebugUI::EndHotReload()
    {
        for ( auto pDebugView : m_debugViews )
        {
            pDebugView->EndHotReload();
        }
    }

    void EngineDebugUI::DrawPlayerDebugOptionsMenu( UpdateContext const& context )
    {
        auto pPlayerManager = m_pGameWorld->GetWorldSystem<PlayerManager>();
        auto pCameraManager = m_pGameWorld->GetWorldSystem<CameraManager>();

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

    void EngineDebugUI::HandleUserInput( UpdateContext const& context )
    {
        auto pInputSystem = context.GetSystem<Input::InputSystem>();
        EE_ASSERT( pInputSystem != nullptr );
        auto const pKeyboardState = pInputSystem->GetKeyboardState();

        // Enable/disable debug overlay
        //-------------------------------------------------------------------------

        if ( !m_isInEditorPreviewMode )
        {
            if ( pKeyboardState->WasReleased( Input::KeyboardButton::Key_Tilde ) )
            {
                m_debugOverlayEnabled = !m_debugOverlayEnabled;
            }
        }

        // Time Controls
        //-------------------------------------------------------------------------

        if ( pKeyboardState->WasReleased( Input::KeyboardButton::Key_Pause ) )
        {
            ToggleWorldPause();
        }

        if ( pKeyboardState->WasReleased( Input::KeyboardButton::Key_PageUp ) )
        {
            SetWorldTimeScale( m_timeScale + 0.1f );
        }

        if ( pKeyboardState->WasReleased( Input::KeyboardButton::Key_PageDown ) )
        {
            SetWorldTimeScale( m_timeScale - 0.1f );
        }

        if ( pKeyboardState->WasReleased( Input::KeyboardButton::Key_Home ) )
        {
            ResetWorldTimeScale();
        }

        if ( pKeyboardState->WasReleased( Input::KeyboardButton::Key_End ) )
        {
            RequestWorldTimeStep();
        }
    }

    void EngineDebugUI::ToggleWorldPause()
    {
        // Unpause
        if ( m_pGameWorld->IsPaused() )
        {
            m_pGameWorld->SetTimeScale( m_timeScale );
        }
        else // Pause
        {
            m_timeScale = m_pGameWorld->GetTimeScale();
            m_pGameWorld->SetTimeScale( -1.0f );
        }
    }

    void EngineDebugUI::SetWorldTimeScale( float newTimeScale )
    {
        m_timeScale = Math::Clamp( newTimeScale, g_minTimeScaleValue, g_maxTimeScaleValue );
        m_pGameWorld->SetTimeScale( m_timeScale );
    }

    void EngineDebugUI::ResetWorldTimeScale()
    {
        m_timeScale = 1.0f;

        if ( !m_pGameWorld->IsPaused() )
        {
            m_pGameWorld->SetTimeScale( m_timeScale );
        }
    }

    void EngineDebugUI::RequestWorldTimeStep()
    {
        if ( m_pGameWorld->IsPaused() )
        {
            m_pGameWorld->RequestTimeStep();
        }
    }
}
#endif