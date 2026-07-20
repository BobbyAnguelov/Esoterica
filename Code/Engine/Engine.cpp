#include "Engine.h"
#include "Engine/Render/RenderViewport.h"
#include "Base/Profiling.h"
#include "Base/Time/Timers.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/Logging/SystemLog.h"
#include "Base/Utils/CommandLineParser.h"
#include "Base/Resource/Settings/Settings_Resource.h"
#include "Base/Resource/ResourceProvider.h"

//-------------------------------------------------------------------------

namespace EE
{
    Engine::Engine( TFunction<bool( EE::String const& error )>&& errorHandler )
        : m_fatalErrorHandler( errorHandler )
    {
        m_modules.emplace_back( EE::New<BaseModule>() );
        m_modules.emplace_back( EE::New<EngineModule>() );
    }

    Engine::~Engine()
    {
        for ( auto iter = m_modules.rbegin(); iter != m_modules.rend(); ++iter )
        {
            EE::Delete( *iter );
        }
    }

    //-------------------------------------------------------------------------

    void Engine::UpdateResourceLoadingRequests_Blocking()
    {
        EE_ASSERT( Threading::IsMainThread() );

        m_pResourceSystem->Update( true );

        m_pRenderSystem->StartResourceUpdates( true );
        m_pRenderSystem->SubmitResourceUpdates( true );

        m_pRenderSystem->StartAsyncResourceUpdates( true );
        m_pRenderSystem->SubmitAsyncResourceUpdates( true );
    }

    bool Engine::Initialize( int32_t argc, char** argv, Int2 const& windowDimensions )
    {
        BaseModule* pBaseModule = GetBaseModule();
        EngineModule* pEngineModule = GetEngineModule();

        //-------------------------------------------------------------------------
        // Process command line
        //-------------------------------------------------------------------------

        CommandLineParser cl;
        cl.AddOptionalStringArg( "map", "The startup map." );

        #if EE_DEVELOPMENT_TOOLS
        cl.AddOptionalBoolArg( "packaged", "Should we use packaged data instead of the networked resource server", false );
        #endif

        if ( !cl.Parse( argc, argv ) )
        {
            return m_fatalErrorHandler( "Invalid command line arguments!" );
        }

        //-------------------------------------------------------------------------
        // Register all known types
        //-------------------------------------------------------------------------

        m_initializationStageReached = Stage::RegisterTypeInfo;
        m_pTypeRegistry = pBaseModule->GetTypeRegistry();
        RegisterTypes();

        //-------------------------------------------------------------------------
        // Read settings INI file
        //-------------------------------------------------------------------------

        m_initializationStageReached = Stage::InitializeSettings;

        auto pSettingsRegistry = pBaseModule->GetSettingsRegistry();
        FileSystem::Path const iniFilePath = FileSystem::GetCurrentProcessPath().Append( "Esoterica.ini" );
        if ( !pSettingsRegistry->Initialize( iniFilePath ) )
        {
            return m_fatalErrorHandler( "Failed to initialize settings!" );
        }

        // Set resource provider to use
        #if EE_DEVELOPMENT_TOOLS
        auto pResourceSettings = pSettingsRegistry->GetSettings<Resource::ResourceSettings>();
        EE_ASSERT( pResourceSettings != nullptr );
        bool const usePackagedResourceProvider = cl.GetBoolArg( "packaged" );
        pResourceSettings->SetUsePackagedResourceProvider( usePackagedResourceProvider );
        #endif

        //-------------------------------------------------------------------------
        // Initialize Engine Modules
        //-------------------------------------------------------------------------

        m_initializationStageReached = Stage::InitializeModules;

        ModuleContext moduleContext = pBaseModule->GetModuleContext();

        for ( Module* pModule : m_modules )
        {
            if ( !pModule->InitializeModule( moduleContext ) )
            {
                return m_fatalErrorHandler( "Failed to initialize engine module!" );
            }
        }

        //-------------------------------------------------------------------------
        // Setup core system pointers and Update Context
        //-------------------------------------------------------------------------

        m_pTaskSystem = pBaseModule->GetTaskSystem();
        m_pSystemRegistry = pBaseModule->GetSystemRegistry();
        m_pInputSystem = pBaseModule->GetInputSystem();
        m_pResourceSystem = pBaseModule->GetResourceSystem();
        m_pResourceProvider = pBaseModule->GetResourceProvider();

        m_pEntityWorldManager = pEngineModule->GetEntityWorldManager();
        EE_DEVELOPMENT_TOOLS_ONLY( m_pImguiSystem = pBaseModule->GetImguiSystem() );
        EE_DEVELOPMENT_TOOLS_ONLY( m_pImguiRenderer = pEngineModule->GetImguiRenderer() );

        m_pRenderSystem = m_pSystemRegistry->GetSystem<Render::RenderSystem>();
        m_pRenderWindow = pEngineModule->GetRenderWindow();
        m_pForwardShadingRenderer = pEngineModule->GetForwardShadingRenderer();

        // Setup update context
        m_updateContext.m_pSystemRegistry = m_pSystemRegistry;

        // Wait for resource provider to connect
        if ( !m_pResourceProvider->IsReady() )
        {
            while ( m_pResourceProvider->IsConnecting() )
            {
                m_pResourceProvider->Update();
                Threading::Sleep( 25 );
            }

            if ( !m_pResourceProvider->IsReady() )
            {
                return m_fatalErrorHandler( "Resource provider failed to connect - See EngineApplication log for details" );
            }
        }

        //-------------------------------------------------------------------------
        // Load Required Module Resources
        //-------------------------------------------------------------------------

        m_initializationStageReached = Stage::LoadModuleResources;

        for ( Module* pModule : m_modules )
        {
            pModule->LoadModuleResources( *m_pResourceSystem );
        }

        // Wait for all load requests to complete
        while ( m_pResourceSystem->IsBusy() )
        {
            UpdateResourceLoadingRequests_Blocking();
        }

        for ( Module* pModule : m_modules )
        {
            Module::LoadingState const loadingState = pModule->GetModuleResourceLoadingState();
            if ( loadingState == Module::LoadingState::Failed )
            {
                return m_fatalErrorHandler( "Failed to load required engine module resources - See EngineApplication log for details" );
            }
        }

        //-------------------------------------------------------------------------
        // Initialize core engine state
        //-------------------------------------------------------------------------

        m_initializationStageReached = Stage::InitializeEngine;

        // Initialize entity world manager and load startup map
        m_pEntityWorldManager->Initialize( *m_pSystemRegistry );

        // Create tools UI
        #if EE_DEVELOPMENT_TOOLS
        m_pRenderSystem->StartResourceUpdates( true );

        CreateToolsUI();
        EE_ASSERT( m_pToolsUI != nullptr );
        m_pToolsUI->Initialize( m_updateContext, m_pImguiRenderer->GetImageCache() );

        m_pRenderSystem->SubmitResourceUpdates( true );

        m_pRenderSystem->StartAsyncResourceUpdates( true );
        m_pRenderSystem->SubmitAsyncResourceUpdates( true );

        #endif

        // Set startup map if provided
        String const map = cl.GetStringArg( "map" );
        if ( !map.empty() )
        {
            ResourceID const startupMapID = ResourceID( map.c_str() );
            if ( startupMapID.IsValid() )
            {
                SetStartupMap( startupMapID );
            }
        }

        m_initializationStageReached = Stage::FullyInitialized;

        //-------------------------------------------------------------------------

        ResizeMainWindow( windowDimensions );
        PostInitialize();

        return true;
    }

    bool Engine::Shutdown()
    {
        EE_LOG_MESSAGE( LogCategory::System, "Shutdown", "Engine Application Shutdown Started" );

        BaseModule* pBaseModule = GetBaseModule();

        //-------------------------------------------------------------------------
        // Wait for shutdown
        //-------------------------------------------------------------------------

        if ( m_pTaskSystem != nullptr )
        {
            m_pTaskSystem->WaitForAll();
        }

        m_pRenderWindow = nullptr;

        m_pRenderSystem->WaitAllQueuesIdle();

        if ( m_initializationStageReached == Stage::FullyInitialized )
        {
            m_initializationStageReached = Stage::InitializeEngine;
        }

        PreShutdown();

        //-------------------------------------------------------------------------
        // Shutdown core engine state
        //-------------------------------------------------------------------------

        if ( m_initializationStageReached == Stage::InitializeEngine )
        {
            // Destroy development tools
            #if EE_DEVELOPMENT_TOOLS
            EE_ASSERT( m_pToolsUI != nullptr );
            m_pToolsUI->Shutdown( m_updateContext );
            EE::Delete( m_pToolsUI );
            #endif

            // Wait for resource/object systems to complete all resource unloading
            m_pEntityWorldManager->Shutdown();

            while ( m_pEntityWorldManager->IsBusyLoading() || m_pResourceSystem->IsBusy() )
            {
                UpdateResourceLoadingRequests_Blocking();
            }

            m_initializationStageReached = Stage::LoadModuleResources;
        }

        //-------------------------------------------------------------------------
        // Unload engine resources
        //-------------------------------------------------------------------------

        if ( m_initializationStageReached == Stage::LoadModuleResources )
        {
            if ( m_pResourceSystem != nullptr )
            {
                for ( Module* pModule : m_modules )
                {
                    pModule->UnloadModuleResources( *m_pResourceSystem );
                }

                while ( m_pResourceSystem->IsBusy() )
                {
                    UpdateResourceLoadingRequests_Blocking();
                }
            }

            m_initializationStageReached = Stage::InitializeModules;
        }

        //-------------------------------------------------------------------------
        // Shutdown Engine Modules
        //-------------------------------------------------------------------------

        if ( m_initializationStageReached == Stage::InitializeModules )
        {

            m_updateContext.m_pSystemRegistry = nullptr;

            EE_DEVELOPMENT_TOOLS_ONLY( m_pImguiSystem = nullptr );
            m_pEntityWorldManager = nullptr;
            m_pTaskSystem = nullptr;
            m_pSystemRegistry = nullptr;
            m_pInputSystem = nullptr;
            m_pResourceSystem = nullptr;
            m_pResourceProvider = nullptr;
            m_pRenderSystem = nullptr;
            m_pRenderWindow = nullptr;

            ModuleContext moduleContext = pBaseModule->GetModuleContext();

            for ( auto iter = m_modules.rbegin(); iter != m_modules.rend(); ++iter )
            {
                ( *iter )->ShutdownModule( moduleContext );
            }

            m_initializationStageReached = Stage::InitializeSettings;
        }

        //-------------------------------------------------------------------------
        // Shutdown Settings
        //-------------------------------------------------------------------------

        if ( m_initializationStageReached == Stage::InitializeSettings )
        {
            auto pSettingsRegistry = pBaseModule->GetSettingsRegistry();
            pSettingsRegistry->Shutdown();

            m_initializationStageReached = Stage::RegisterTypeInfo;
        }

        //-------------------------------------------------------------------------
        // Unregister All Types
        //-------------------------------------------------------------------------

        if ( m_initializationStageReached == Stage::RegisterTypeInfo )
        {
            UnregisterTypes();
            m_pTypeRegistry = nullptr;

            m_initializationStageReached = Stage::Uninitialized;
        }

        //-------------------------------------------------------------------------

        return true;
    }

    void Engine::SetStartupMap( ResourceID const& mapID )
    {
        EE_ASSERT( mapID.IsValid() );
        m_pEntityWorldManager->GetWorlds()[0]->LoadMap( mapID );
    }

    //-------------------------------------------------------------------------

    bool Engine::Update()
    {
        EE_ASSERT( m_initializationStageReached == Stage::FullyInitialized );

        // Check for fatal errors
        //-------------------------------------------------------------------------

        if ( SystemLog::HasFatalErrorOccurred() )
        {
            return m_fatalErrorHandler( SystemLog::GetFatalError().m_message.c_str() );
        }

        // Perform Frame Update
        //-------------------------------------------------------------------------

        Profiling::StartFrame();

        Milliseconds deltaTime = 0;
        {
            ScopedTimer<PlatformClock> frameTimer( deltaTime );

            // Network Update
            //-------------------------------------------------------------------------

            {
                EE_PROFILE_SCOPE_NETWORK( "Networking" );
                // TODO
            }

            // Resource System Update
            //-------------------------------------------------------------------------

            bool runEngineUpdate = true;

            {
                EE_PROFILE_SCOPE_RESOURCE( "Resource System" );

                // Wait for resource provider to connect
                if ( !m_pResourceProvider->IsReady() )
                {
                    while ( m_pResourceProvider->IsConnecting() )
                    {
                        m_pResourceProvider->Update();
                        Threading::Sleep( 1 );
                    }

                    if ( !m_pResourceProvider->IsReady() )
                    {
                        return m_fatalErrorHandler( "Resource provider connection failed - See EngineApplication log for details" );
                    }
                }

                // Update resource system
                m_pRenderSystem->StartAsyncResourceUpdates( false );
                m_pResourceSystem->Update();
                m_pRenderSystem->SubmitAsyncResourceUpdates( false );

                // Handle hot-reloading of entities
                #if EE_DEVELOPMENT_TOOLS
                if ( m_pResourceSystem->RequiresHotReloading() )
                {
                    TInlineVector<ResourceID, 20> const resourcesToReload = m_pResourceSystem->GetResourcesToBeReloaded();
                    TInlineVector<Resource::ResourceRequesterID, 20> const usersToReload = m_pResourceSystem->GetUsersToBeReloaded();

                    // Unload module resources
                    for ( auto pModule : m_modules )
                    {
                        pModule->HotReload_UnloadResources( *m_pResourceSystem, resourcesToReload );
                    }

                    // Unload game and tool resources
                    m_pToolsUI->HotReload_UnloadResources( usersToReload, resourcesToReload );

                    if ( !usersToReload.empty() )
                    {
                        m_pEntityWorldManager->HotReload_UnloadEntities( usersToReload );
                    }

                    // Ensure that all resource requests (both load/unload are completed before continuing with the hot-reload)
                    m_pResourceSystem->ClearHotReloadRequests();
                    while ( m_pResourceSystem->IsBusy() )
                    {
                        UpdateResourceLoadingRequests_Blocking();
                    }

                    // Reload game and tool resources
                    m_pToolsUI->HotReload_ReloadResources( usersToReload, resourcesToReload );

                    if ( !usersToReload.empty() )
                    {
                        m_pEntityWorldManager->HotReload_ReloadEntities( usersToReload );
                    }

                    // Reload module resources
                    bool shouldWaitForModuleResourceLoad = false;
                    for ( auto pModule : m_modules )
                    {
                        pModule->HotReload_ReloadResources( *m_pResourceSystem, resourcesToReload );
                        if ( pModule->GetModuleResourceLoadingState() == Module::LoadingState::Busy )
                        {
                            shouldWaitForModuleResourceLoad = true;
                        }
                    }

                    // Wait for module resources to fully load back in
                    if ( shouldWaitForModuleResourceLoad )
                    {
                        while ( m_pResourceSystem->IsBusy() )
                        {
                            UpdateResourceLoadingRequests_Blocking();
                        }

                        for ( auto pModule : m_modules )
                        {
                            Module::LoadingState const state = pModule->GetModuleResourceLoadingState();
                            EE_ASSERT( state != Module::LoadingState::Busy );
                            if ( state == Module::LoadingState::Failed )
                            {
                                EE_LOG_FATAL_ERROR( LogCategory::Resource, "Hot-Reload", "Failed to hot-reload module resources! Check log for details!" );
                                runEngineUpdate = false;
                            }
                        }
                    }
                }
                #endif
            }

            // Engine Update
            //-------------------------------------------------------------------------

            if ( runEngineUpdate )
            {
                // Frame Start
                //-------------------------------------------------------------------------
                {
                    EE_PROFILE_SCOPE_ENTITY( "Frame Start" );
                    m_updateContext.m_stage = UpdateStage::FrameStart;

                    //-------------------------------------------------------------------------

                    m_pEntityWorldManager->StartFrame();

                    //-------------------------------------------------------------------------

                    {
                        EE_PROFILE_SCOPE_ENTITY( "World Loading" );
                        m_pEntityWorldManager->UpdateLoading();
                    }

                    //-------------------------------------------------------------------------

                    #if EE_DEVELOPMENT_TOOLS
                    m_pImguiSystem->StartFrame( m_updateContext.GetDeltaTime() );
                    m_pToolsUI->StartFrame( m_updateContext );
                    #endif

                    //-------------------------------------------------------------------------

                    {
                        EE_PROFILE_SCOPE_DEVTOOLS( "Input System" );
                        m_pInputSystem->Update( m_updateContext.GetDeltaTime() );
                    }

                    #if EE_DEVELOPMENT_TOOLS
                    m_pToolsUI->Update( m_updateContext );
                    #endif

                    m_pEntityWorldManager->UpdateWorlds( m_updateContext );
                }

                // Game Start
                //-------------------------------------------------------------------------
                {
                    EE_PROFILE_SCOPE_ENTITY( "Game Setup Update" );
                    m_updateContext.m_stage = UpdateStage::GameSetup;

                    #if EE_DEVELOPMENT_TOOLS
                    m_pToolsUI->Update( m_updateContext );
                    #endif

                    m_pEntityWorldManager->UpdateWorlds( m_updateContext );
                }

                // Game Simulation
                //-------------------------------------------------------------------------
                {
                    EE_PROFILE_SCOPE_ENTITY( "Game Pre-Physics Update" );
                    m_updateContext.m_stage = UpdateStage::GamePrePhysics;

                    #if EE_DEVELOPMENT_TOOLS
                    m_pToolsUI->Update( m_updateContext );
                    #endif

                    m_pEntityWorldManager->UpdateWorlds( m_updateContext );
                }

                // Pre-Physics
                //-------------------------------------------------------------------------
                {
                    EE_PROFILE_SCOPE_ENTITY( "Pre-Physics Update" );
                    m_updateContext.m_stage = UpdateStage::PrePhysics;

                    #if EE_DEVELOPMENT_TOOLS
                    m_pToolsUI->Update( m_updateContext );
                    #endif

                    m_pEntityWorldManager->UpdateWorlds( m_updateContext );
                }

                // Physics
                //-------------------------------------------------------------------------
                {
                    EE_PROFILE_SCOPE_ENTITY( "Physics Update" );
                    m_updateContext.m_stage = UpdateStage::Physics;

                    #if EE_DEVELOPMENT_TOOLS
                    m_pToolsUI->Update( m_updateContext );
                    #endif

                    m_pEntityWorldManager->UpdateWorlds( m_updateContext );
                }

                // Post-Physics
                //-------------------------------------------------------------------------
                {
                    EE_PROFILE_SCOPE_ENTITY( "Post-Physics Update" );
                    m_updateContext.m_stage = UpdateStage::PostPhysics;

                    #if EE_DEVELOPMENT_TOOLS
                    m_pToolsUI->Update( m_updateContext );
                    #endif

                    m_pEntityWorldManager->UpdateWorlds( m_updateContext );
                }

                // Game End
                //-------------------------------------------------------------------------
                {
                    EE_PROFILE_SCOPE_ENTITY( "Game Post-Physics Update" );
                    m_updateContext.m_stage = UpdateStage::GamePostPhysics;

                    #if EE_DEVELOPMENT_TOOLS
                    m_pToolsUI->Update( m_updateContext );
                    #endif

                    m_pEntityWorldManager->UpdateWorlds( m_updateContext );
                }

                // Pause Updates
                //-------------------------------------------------------------------------
                // This is an optional update that's only run when a world is "paused"
                {
                    EE_PROFILE_SCOPE_ENTITY( "Paused Update" );
                    m_updateContext.m_stage = UpdateStage::Paused;

                    #if EE_DEVELOPMENT_TOOLS
                    m_pToolsUI->Update( m_updateContext );
                    #endif

                    m_pEntityWorldManager->UpdateWorlds( m_updateContext );
                }

                // Frame End
                //-------------------------------------------------------------------------
                {
                    EE_PROFILE_SCOPE_ENTITY( "Frame End" );
                    m_updateContext.m_stage = UpdateStage::FrameEnd;

                    m_pEntityWorldManager->UpdateWorlds( m_updateContext );

                    #if EE_DEVELOPMENT_TOOLS
                    m_pEntityWorldManager->DebugDrawWorlds( m_updateContext );
                    #endif

                    //-------------------------------------------------------------------------

                    #if EE_DEVELOPMENT_TOOLS
                    m_pToolsUI->Update( m_updateContext );
                    m_pToolsUI->EndFrame( m_updateContext );
                    m_pImguiSystem->EndFrame();
                    #endif

                    m_pEntityWorldManager->EndFrame();
                    m_pInputSystem->PrepareForNewMessages();
                }

                // Render everything
                //-------------------------------------------------------------------------
                {
                    EE_PROFILE_SCOPE_ENTITY( "Frame Render" );

                    m_pRenderSystem->WaitForFrameStart();

                    m_pRenderSystem->StartResourceUpdates( false );

                    #if EE_DEVELOPMENT_TOOLS
                    ImGui::Render();

                    m_pImguiRenderer->UpdateDeviceResources();
                    #endif

                    m_pForwardShadingRenderer->UpdateDeviceResources( m_updateContext );

                    // Per-world, viewport independent
                    for ( EntityWorld* pWorld : m_pEntityWorldManager->GetWorlds() )
                    {
                        m_pForwardShadingRenderer->UpdateWorldDeviceResources( m_updateContext, pWorld );
                    }

                    m_pRenderSystem->SubmitResourceUpdates( false );

                    // Main rendering
                    m_pRenderSystem->StartFrame();
                    #if EE_DEVELOPMENT_TOOLS
                    m_pImguiRenderer->StartFrame();
                    #endif

                    // Render all worlds to their corresponding viewports
                    bool needSwapchainClear = true;
                    for ( EntityWorld* pWorld : m_pEntityWorldManager->GetWorlds() )
                    {
                        // TODO: Hacky, need a way for dispatching stuff that is shared between viewports
                        bool worldDispatched = false;

                        for ( Viewport* pViewport : pWorld->GetViewports() )
                        {
                            if ( !pViewport->IsValid() )
                            {
                                continue;
                            }

                            Render::RenderViewport* pRenderViewport = static_cast<Render::RenderViewport*>( pViewport );

                            if ( pRenderViewport->IsStandalone() )
                            {
                                needSwapchainClear = false;
                            }

                            if ( !worldDispatched )
                            {
                                m_pForwardShadingRenderer->DispatchWorld( m_updateContext, pRenderViewport, pWorld );
                                worldDispatched = true;
                            }

                            m_pForwardShadingRenderer->UpdateViewportDeviceResources( m_updateContext, pRenderViewport, pWorld );
                            m_pForwardShadingRenderer->DrawWorldToViewport( m_updateContext, pRenderViewport, pWorld );
                        }
                    }

                    #if EE_DEVELOPMENT_TOOLS
                    m_pImguiRenderer->SubmitFrame( needSwapchainClear );
                    #endif

                    m_pRenderSystem->SubmitFrame();

                    #if EE_DEVELOPMENT_TOOLS
                    m_pImguiRenderer->EndFrame();
                    #endif
                }
            }
        }

        // Update Time
        //-------------------------------------------------------------------------

        // Ensure we dont get crazy time delta's when we hit breakpoints
        #if EE_DEVELOPMENT_TOOLS
        if ( deltaTime.ToSeconds() > 1.0f )
        {
            deltaTime = m_updateContext.GetDeltaTime(); // Keep last frame delta
        }
        #endif

        // Frame rate limiter
        if ( m_updateContext.HasFrameRateLimit() )
        {
            float const minimumFrameTime = m_updateContext.GetLimitedFrameTime();
            if ( deltaTime < minimumFrameTime )
            {
                Threading::Sleep( minimumFrameTime - deltaTime );
                deltaTime = minimumFrameTime;
            }
        }

        m_updateContext.UpdateDeltaTime( deltaTime );
        Profiling::EndFrame();
        EngineClock::Update( deltaTime );

        // Should we exit?
        //-------------------------------------------------------------------------

        return true;
    }

    void Engine::ResizeMainWindow( Int2 newMainWindowDimensions )
    {
        Float2 const newWindowDimensions = Float2( newMainWindowDimensions );
        if ( m_pRenderWindow->GetSwapchainSize() != newMainWindowDimensions )
        {
            m_pRenderSystem->WaitGraphicsQueueIdle();
            m_pRenderWindow->ResizeSwapchain( m_pRenderSystem->GetContextRHI(),
                                              m_pRenderSystem->GetGraphicsQueue(),
                                              m_pRenderSystem->GetComputeQueue(),
                                              newMainWindowDimensions );
        }
    }

    #if EE_ENABLE_LPP
    void Engine::LivePP_PreReload()
    {
        ModuleContext moduleContext = GetBaseModule()->GetModuleContext();

        for ( Module* pModule : m_modules )
        {
            pModule->LivePP_PreReload( moduleContext );
        }

        UpdateResourceLoadingRequests_Blocking();
    }

    void Engine::LivePP_PostReload()
    {
        ModuleContext moduleContext = GetBaseModule()->GetModuleContext();

        for ( Module* pModule : m_modules )
        {
            pModule->LivePP_PostReload( moduleContext );
        }

        UpdateResourceLoadingRequests_Blocking();
    }
    #endif
}