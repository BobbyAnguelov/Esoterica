#include "EngineDebugUI.h"
#include "Engine/Debug/DebugView.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/Debug/DebugView_EntityWorld.h"
#include "Engine/Input/Systems/WorldSystem_GameInput.h"
#include "Engine/Imgui/ImguiOrientationGuide.h"
#include "Engine/Camera/Systems/WorldSystem_Camera.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Input/InputSystem.h"
#include "EASTL/sort.h"
#include "Engine/Debug/Widgets/FrameLimiterWidget.h"
#include "Engine/Debug/Widgets/PerformanceStatsWidget.h"

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
        EE_ASSERT( pDebugView->HasMenuOptions() );

        Menu* pSubMenu = nullptr;
        String const pathString = pDebugView->GetMenuPath();
        if ( pathString.empty() )
        {
            pSubMenu = this;
        }
        else
        {
            MenuPath const menuPath = CreatePathFromString( pathString );
            pSubMenu = &FindOrAddMenu( menuPath );
        }

        EE_ASSERT( pSubMenu != nullptr );

        // Add new callback and sort callback list
        //-------------------------------------------------------------------------

        pSubMenu->m_debugViews.emplace_back( pDebugView );

        auto callbackSortPredicate = [] ( DebugView* const& pA, DebugView* const& pB )
        {
            return StringUtils::Stricmp( pA->GetMenuPath(), pB->GetMenuPath() ) < 0;
        };

        eastl::sort( pSubMenu->m_debugViews.begin(), pSubMenu->m_debugViews.end(), callbackSortPredicate );
    }

    void EngineDebugUI::Menu::RemoveChildMenu( DebugView* pDebugView )
    {
        EE_ASSERT( pDebugView != nullptr );
        EE_ASSERT( pDebugView->HasMenuOptions() );
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
        EE_ASSERT( pDebugView->HasMenuOptions() );

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
        EE_ASSERT( pDebugView->HasMenuOptions() );

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
        for ( Menu& childMenu : m_childMenus )
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

                if ( pDebugView->HasMenuOptions() )
                {
                    if ( pDebugView->GetCategory() == DebugView::Category::Engine )
                    {
                        m_engineMenu.AddChildMenu( pDebugView );
                    }
                    else if ( pDebugView->GetCategory() == DebugView::Category::Game )
                    {
                        m_gameMenu.AddChildMenu( pDebugView );
                    }
                }
            }
        }
    }

    void EngineDebugUI::Shutdown( UpdateContext const& context )
    {
        m_engineMenu.Clear();
        m_gameMenu.Clear();

        for ( auto pDebugView : m_debugViews )
        {
            pDebugView->Shutdown();
            EE::Delete( pDebugView );
        }

        m_pGameWorld = nullptr;
    }

    //-------------------------------------------------------------------------

    void EngineDebugUI::StartFrame( UpdateContext const& context )
    {
        EE_ASSERT( !m_isInEditorMode );

        // Update tool camera
        //-------------------------------------------------------------------------
        // If we are in editor mode, the editor will update the tool camera

        if ( !m_isInEditorMode )
        {
            auto pCameraSystem = m_pGameWorld->GetWorldSystem<CameraSystem>();
            if ( pCameraSystem != nullptr && pCameraSystem->IsToolsCameraEnabled() )
            {
                EntityWorldUpdateContext updateContext( context, m_pGameWorld );
                pCameraSystem->UpdateToolsCamera( updateContext );
            }
        }
    }

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

        if ( !m_isInEditorMode )
        {
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
                // Menu
                //-------------------------------------------------------------------------

                if ( m_debugOverlayEnabled )
                {
                    if ( ImGui::BeginMenuBar() )
                    {
                        DrawMenu( context );
                        ImGui::EndMenuBar();
                    }
                }

                // Overlay
                //-------------------------------------------------------------------------

                EE_ASSERT( m_pGameWorld->HasViewports() );
                DrawOverlayUI( context, m_pGameWorld->GetMainViewport() );
            }

            //-------------------------------------------------------------------------

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

        // System Log
        //-------------------------------------------------------------------------

        if ( !m_isInEditorMode && m_showSystemLog )
        {
            if ( ImGui::Begin( "System Log", &m_showSystemLog ) )
            {
                m_systemLogWidget.UpdateAndDraw( context );
            }
            ImGui::End();
        }
    }

    //-------------------------------------------------------------------------
    // Drawing
    //-------------------------------------------------------------------------

    void EngineDebugUI::DrawMenu( UpdateContext const& context )
    {
        // Draw Debug View Menus
        //-------------------------------------------------------------------------

        if ( ImGui::BeginMenu( EE_ICON_BUG" Preview" ) )
        {
            DrawWorldDebugOptionsMenu( context );
            ImGui::EndMenu();
        }

        if ( !m_engineMenu.IsEmpty() )
        {
            if ( ImGui::BeginMenu( m_engineMenu.m_title.c_str() ) )
            {
                EntityWorldUpdateContext worldUpdateContext( context, m_pGameWorld );
                m_engineMenu.Draw( worldUpdateContext );
                ImGui::EndMenu();
            }
        }

        if ( !m_gameMenu.IsEmpty() )
        {
            if ( ImGui::BeginMenu( m_gameMenu.m_title.c_str() ) )
            {
                EntityWorldUpdateContext worldUpdateContext( context, m_pGameWorld );
                m_gameMenu.Draw( worldUpdateContext );
                ImGui::EndMenu();
            }
        }

        // System Info
        //-------------------------------------------------------------------------

        if ( !m_isInEditorMode )
        {
            constexpr static float const gapSize = 1;
            constexpr static float const menuButtonSize = 35;

            auto MenuSeparator = [] ()
            {
                ImVec2 const startPos = ImGui::GetCursorScreenPos();
                ImGui::Dummy( ImVec2( gapSize, 0 ) );
                ImVec2 min = startPos + ImVec2( ( gapSize / 2 ) - 1, 0 );
                ImVec2 max = ImVec2( min.x + 2.0f, min.y + ImGui::GetFrameHeight() ); // Adjust thickness (2.0f) and height as needed
                ImGui::GetWindowDrawList()->AddRectFilled( min, max, ImGui::GetColorU32( ImGuiCol_Separator ) );
            };

            FrameLimiterWidget const frameLimiter( menuButtonSize );
            SystemLogSummaryWidget const logSummary;
            PerformanceStatsWidget const perfStats( context );

            ImVec2 const totalAvailableSpace = ImGui::GetContentRegionAvail();
            float const totalWidthNeeded = frameLimiter.m_width + perfStats.m_totalSize.x + logSummary.m_size.x + ( 4 * ImGui::GetStyle().ItemSpacing.x );
            float const startOffset = totalAvailableSpace.x - totalWidthNeeded;

            ImGui::SameLine( 0, startOffset );
            if ( logSummary.Draw() )
            {
                m_systemLogWidget.m_showLogWarnings = true;
                m_systemLogWidget.m_showLogErrors = true;
                m_showSystemLog = true;
            }

            MenuSeparator();
            frameLimiter.Draw( const_cast<UpdateContext&>( context ) );

            MenuSeparator();
            perfStats.Draw();
        }
    }

    void EngineDebugUI::DrawOverlayUI( UpdateContext const& context, Viewport const* pViewport )
    {
        EE_ASSERT( context.GetUpdateStage() == UpdateStage::FrameEnd );

        ImVec2 const startCursorPos = ImGui::GetCursorPos();

        EntityWorldUpdateContext worldUpdateContext( context, m_pGameWorld );
        for ( auto pDebugView : m_debugViews )
        {
            ImGui::SetCursorPos( startCursorPos );
            pDebugView->DrawOverlayUI( worldUpdateContext );
        }

        if ( m_debugOverlayEnabled )
        {
            ImVec2 const guidePosition = startCursorPos + ImGui::GetContentRegionAvail() - ( ImGuiX::OrientationGuide::GetSize() / 2 );
            ImGuiX::OrientationGuide::Draw( ImGui::GetWindowPos() + guidePosition, *pViewport );
        }
    }

    void EngineDebugUI::HotReload_UnloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded )
    {
        for ( auto pDebugView : m_debugViews )
        {
            pDebugView->HotReload_UnloadResources( usersToReload, resourcesToBeReloaded );
        }
    }

    void EngineDebugUI::HotReload_ReloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded )
    {
        for ( auto pDebugView : m_debugViews )
        {
            pDebugView->HotReload_ReloadResources( usersToReload, resourcesToBeReloaded );
        }
    }

    void EngineDebugUI::DrawWorldDebugOptionsMenu( UpdateContext const& context )
    {
        auto pGameInputSystem = m_pGameWorld->GetWorldSystem<Input::GameInputSystem>();
        auto pCameraSystem = m_pGameWorld->GetWorldSystem<CameraSystem>();

        ImGui::SeparatorText( "Time Controls" );

        EntityWorldTimeControlsWidget timeControlsWidget( m_pGameWorld );
        timeControlsWidget.Draw();

        //-------------------------------------------------------------------------

        ImGui::SeparatorText( "Input" );

        bool isGameInputEnabled = pGameInputSystem->IsGameInputEnabled();
        if ( ImGui::Checkbox( EE_ICON_MICROSOFT_XBOX_CONTROLLER " Enable Game Input", &isGameInputEnabled ) )
        {
            pGameInputSystem->SetGameInputEnabled( isGameInputEnabled );
        }

        ImGui::SeparatorText( "Camera" );

        bool isDebugCameraEnabled = pCameraSystem->IsToolsCameraEnabled();
        if ( ImGui::Checkbox( EE_ICON_VIDEO_BOX" Enable Debug Camera", &isDebugCameraEnabled ) )
        {
            if ( isDebugCameraEnabled )
            {
                pCameraSystem->EnableToolsCamera();
            }
            else
            {
                pCameraSystem->DisableToolsCamera();
            }
        }
    }

    //-------------------------------------------------------------------------

    void EngineDebugUI::HandleUserInput( UpdateContext const& context )
    {
        auto pInputSystem = context.GetSystem<Input::InputSystem>();
        EE_ASSERT( pInputSystem != nullptr );
        auto const pKeyboardMouse = pInputSystem->GetKeyboardMouse();

        // Enable/disable debug overlay
        //-------------------------------------------------------------------------

        if ( !m_isInEditorMode )
        {
            if ( pKeyboardMouse->WasReleased( Input::InputID::Keyboard_Tilde ) )
            {
                m_debugOverlayEnabled = !m_debugOverlayEnabled;
            }
        }

        // Time Controls
        //-------------------------------------------------------------------------

        if ( pKeyboardMouse->WasReleased( Input::InputID::Keyboard_Pause ) )
        {
            m_pGameWorld->IsPaused() ? m_pGameWorld->Unpause() : m_pGameWorld->Pause();
        }

        if ( pKeyboardMouse->WasReleased( Input::InputID::Keyboard_PageUp ) )
        {
            m_pGameWorld->SetTimeScale( m_pGameWorld->GetTimeScale() + 0.1f );
        }

        if ( pKeyboardMouse->WasReleased( Input::InputID::Keyboard_PageDown ) )
        {
            m_pGameWorld->SetTimeScale( Math::Max( 0.1f, m_pGameWorld->GetTimeScale() - 0.1f ) );
        }

        if ( pKeyboardMouse->WasReleased( Input::InputID::Keyboard_Home ) )
        {
            m_pGameWorld->SetTimeScale( 0.0f );
        }

        if ( pKeyboardMouse->WasReleased( Input::InputID::Keyboard_End ) )
        {
            m_pGameWorld->RequestTimeStep();
        }
    }
}
#endif