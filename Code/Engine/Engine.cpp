#include "Engine.h"
#include "Engine/Console/Console.h"
#include "Base/Network/NetworkSystem.h"
#include "Base/Profiling.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Time/Timers.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/Logging/SystemLog.h"

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

    bool Engine::Initialize( Int2 const& windowDimensions )
    {
        EE_LOG_INFO( "System", nullptr, "Engine Application Startup" );

        BaseModule* pBaseModule = GetBaseModule();
        EngineModule* pEngineModule = GetEngineModule();

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
        m_pRenderDevice = pBaseModule->GetRenderDevice();
        m_pEntityWorldManager = pEngineModule->GetEntityWorldManager();
        EE_DEVELOPMENT_TOOLS_ONLY( m_pImguiSystem = pBaseModule->GetImguiSystem() );
        EE_DEVELOPMENT_TOOLS_ONLY( m_pConsole = pEngineModule->GetConsole(); );

        // Setup update context
        m_updateContext.m_pSystemRegistry = m_pSystemRegistry;

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
            Network::NetworkSystem::Update();
            m_pResourceSystem->Update();
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
        if ( m_startupMap.IsValid() )
        {
            auto const mapResourceID = EE::ResourceID( m_startupMap );
            m_pEntityWorldManager->GetWorlds()[0]->LoadMap( mapResourceID );
        }

        // Initialize rendering system
        m_renderingSystem.Initialize( m_pRenderDevice, Float2( windowDimensions ), pEngineModule->GetRendererRegistry(), m_pEntityWorldManager );
        m_pSystemRegistry->RegisterSystem( &m_renderingSystem );

        // Create tools UI
        #if EE_DEVELOPMENT_TOOLS
        CreateDevelopmentToolsUI();
        EE_ASSERT( m_pDevelopmentToolsUI != nullptr );
        m_pDevelopmentToolsUI->Initialize( m_updateContext, m_pImguiSystem->GetImageCache() );
        #endif

        m_initializationStageReached = Stage::FullyInitialized;

        //-------------------------------------------------------------------------

        PostInitialize();

        return true;
    }

    bool Engine::Shutdown()
    {
        EE_LOG_INFO( "System", nullptr, "Engine Application Shutdown Started" );

        BaseModule* pBaseModule = GetBaseModule();

        //-------------------------------------------------------------------------
        // Wait for shutdown
        //-------------------------------------------------------------------------

        if ( m_pTaskSystem != nullptr )
        {
            m_pTaskSystem->WaitForAll();
        }

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
            EE_ASSERT( m_pDevelopmentToolsUI != nullptr );
            m_pDevelopmentToolsUI->Shutdown( m_updateContext );
            EE::Delete( m_pDevelopmentToolsUI );
            #endif

            // Shutdown rendering system
            m_pSystemRegistry->UnregisterSystem( &m_renderingSystem );
            m_renderingSystem.Shutdown();

            // Wait for resource/object systems to complete all resource unloading
            m_pEntityWorldManager->Shutdown();

            while ( m_pEntityWorldManager->IsBusyLoading() || m_pResourceSystem->IsBusy() )
            {
                m_pResourceSystem->Update();
            }

            m_initializationStageReached = Stage::LoadModuleResources;
        }

        //-------------------------------------------------------------------------
        // Unload engine resources
        //-------------------------------------------------------------------------

        if( m_initializationStageReached == Stage::LoadModuleResources )
        {
            if ( m_pResourceSystem != nullptr )
            {
                for ( Module* pModule : m_modules )
                {
                    pModule->UnloadModuleResources( *m_pResourceSystem );
                }

                while ( m_pResourceSystem->IsBusy() )
                {
                    m_pResourceSystem->Update();
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
            EE_DEVELOPMENT_TOOLS_ONLY( m_pConsole = nullptr );
            m_pEntityWorldManager = nullptr;
            m_pTaskSystem = nullptr;
            m_pSystemRegistry = nullptr;
            m_pInputSystem = nullptr;
            m_pResourceSystem = nullptr;
            m_pRenderDevice = nullptr;

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

    //-------------------------------------------------------------------------

    bool Engine::Update()
    {
        EE_ASSERT( m_initializationStageReached == Stage::FullyInitialized );

        // Check for fatal errors
        //-------------------------------------------------------------------------

        if ( SystemLog::HasFatalErrorOccurred() )
        {
            return m_fatalErrorHandler( SystemLog::GetFatalError().m_message );
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
                Network::NetworkSystem::Update();
            }

            // Resource System Update
            //-------------------------------------------------------------------------

            bool runEngineUpdate = true;

            {
                EE_PROFILE_SCOPE_RESOURCE( "Resource System" );

                m_pResourceSystem->Update();

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
                    m_pDevelopmentToolsUI->HotReload_UnloadResources( usersToReload, resourcesToReload );

                    if ( !usersToReload.empty() )
                    {
                        m_pEntityWorldManager->HotReload_UnloadEntities( usersToReload );
                    }

                    // Ensure that all resource requests (both load/unload are completed before continuing with the hot-reload)
                    m_pResourceSystem->ClearHotReloadRequests();
                    while ( m_pResourceSystem->IsBusy() )
                    {
                        Network::NetworkSystem::Update();
                        m_pResourceSystem->Update( true );
                    }

                    // Reload game and tool resources
                    m_pDevelopmentToolsUI->HotReload_ReloadResources( usersToReload, resourcesToReload );

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
                            Network::NetworkSystem::Update();
                            m_pResourceSystem->Update( true );
                        }

                        for ( auto pModule : m_modules )
                        {
                            Module::LoadingState const state = pModule->GetModuleResourceLoadingState();
                            EE_ASSERT( state != Module::LoadingState::Busy );
                            if ( state == Module::LoadingState::Failed )
                            {
                                EE_LOG_FATAL_ERROR( "Resource", "Hot-Reload", "Failed to hot-reload module resources! Check log for details!" );
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

                    #if EE_DEVELOPMENT_TOOLS
                    m_pImguiSystem->StartFrame( m_updateContext.GetDeltaTime() );
                    m_pDevelopmentToolsUI->StartFrame( m_updateContext );
                    m_pConsole->Update( m_updateContext );
                    #endif

                    //-------------------------------------------------------------------------

                    {
                        EE_PROFILE_SCOPE_ENTITY( "World Loading" );
                        m_pEntityWorldManager->UpdateLoading();
                    }

                    //-------------------------------------------------------------------------

                    {
                        EE_PROFILE_SCOPE_DEVTOOLS( "Input System" );
                        m_pInputSystem->Update( m_updateContext.GetDeltaTime() );
                    }

                    #if EE_DEVELOPMENT_TOOLS
                    m_pDevelopmentToolsUI->Update( m_updateContext );
                    #endif

                    m_pEntityWorldManager->UpdateWorlds( m_updateContext );
                }

                // Pre-Physics
                //-------------------------------------------------------------------------
                {
                    EE_PROFILE_SCOPE_ENTITY( "Pre-Physics Update" );
                    m_updateContext.m_stage = UpdateStage::PrePhysics;

                    #if EE_DEVELOPMENT_TOOLS
                    m_pDevelopmentToolsUI->Update( m_updateContext );
                    #endif

                    m_pEntityWorldManager->UpdateWorlds( m_updateContext );
                }

                // Physics
                //-------------------------------------------------------------------------
                {
                    EE_PROFILE_SCOPE_ENTITY( "Physics Update" );
                    m_updateContext.m_stage = UpdateStage::Physics;

                    #if EE_DEVELOPMENT_TOOLS
                    m_pDevelopmentToolsUI->Update( m_updateContext );
                    #endif

                    m_pEntityWorldManager->UpdateWorlds( m_updateContext );
                }

                // Post-Physics
                //-------------------------------------------------------------------------
                {
                    EE_PROFILE_SCOPE_ENTITY( "Post-Physics Update" );
                    m_updateContext.m_stage = UpdateStage::PostPhysics;

                    #if EE_DEVELOPMENT_TOOLS
                    m_pDevelopmentToolsUI->Update( m_updateContext );
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
                    m_pDevelopmentToolsUI->Update( m_updateContext );
                    #endif

                    m_pEntityWorldManager->UpdateWorlds( m_updateContext );
                }

                // Frame End
                //-------------------------------------------------------------------------
                {
                    EE_PROFILE_SCOPE_ENTITY( "Frame End" );
                    m_updateContext.m_stage = UpdateStage::FrameEnd;

                    #if EE_DEVELOPMENT_TOOLS
                    m_pDevelopmentToolsUI->Update( m_updateContext );
                    #endif

                    m_pEntityWorldManager->UpdateWorlds( m_updateContext );

                    //-------------------------------------------------------------------------

                    #if EE_DEVELOPMENT_TOOLS
                    m_pDevelopmentToolsUI->EndFrame( m_updateContext );
                    m_pImguiSystem->EndFrame();
                    #endif

                    m_pEntityWorldManager->EndFrame();

                    m_renderingSystem.Update( m_updateContext );
                    m_pInputSystem->PrepareForNewMessages();
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
}