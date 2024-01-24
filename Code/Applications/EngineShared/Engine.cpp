#include "Engine.h"
#include "Engine/Console/Console.h"
#include "Base/Network/NetworkSystem.h"
#include "Base/Profiling.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Time/Timers.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/Logging/LoggingSystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    Engine::Engine( TFunction<bool( EE::String const& error )>&& errorHandler )
        : m_fatalErrorHandler( errorHandler )
    {}

    //-------------------------------------------------------------------------

    bool Engine::Initialize( Int2 const& windowDimensions )
    {
        EE_LOG_INFO( "System", nullptr, "Engine Application Startup" );

        //-------------------------------------------------------------------------
        // Register all known types
        //-------------------------------------------------------------------------

        m_initializationStageReached = Stage::RegisterTypeInfo;
        m_pTypeRegistry = m_baseModule.GetTypeRegistry();
        RegisterTypes();

        //-------------------------------------------------------------------------
        // Read settings INI file
        //-------------------------------------------------------------------------

        m_initializationStageReached = Stage::InitializeSettings;

        auto pSettingsRegistry = m_baseModule.GetSettingsRegistry();
        FileSystem::Path const iniFilePath = FileSystem::GetCurrentProcessPath().Append( "Esoterica.ini" );
        if ( !pSettingsRegistry->Initialize( iniFilePath ) )
        {
            return m_fatalErrorHandler( "Failed to initialize settings!" );
        }

        //-------------------------------------------------------------------------
        // Initialize Base
        //-------------------------------------------------------------------------

        m_initializationStageReached = Stage::InitializeBase;

        if ( !m_baseModule.InitializeModule() )
        {
            return m_fatalErrorHandler( "Failed to initialize base module!" );
        }

        m_pTaskSystem = m_baseModule.GetTaskSystem();
        m_pSystemRegistry = m_baseModule.GetSystemRegistry();
        m_pInputSystem = m_baseModule.GetInputSystem();
        m_pResourceSystem = m_baseModule.GetResourceSystem();
        m_pRenderDevice = m_baseModule.GetRenderDevice();
        EE_DEVELOPMENT_TOOLS_ONLY( m_pImguiSystem = m_baseModule.GetImguiSystem() );

        // Setup update context
        m_updateContext.m_pSystemRegistry = m_pSystemRegistry;

        //-------------------------------------------------------------------------
        // Initialize Engine Modules
        //-------------------------------------------------------------------------

        m_initializationStageReached = Stage::InitializeModules;

        // Setup module initialization context
        ModuleContext moduleContext;
        moduleContext.m_pTaskSystem = m_pTaskSystem;
        moduleContext.m_pTypeRegistry = m_pTypeRegistry;
        moduleContext.m_pSettingsRegistry = pSettingsRegistry;
        moduleContext.m_pResourceSystem = m_pResourceSystem;
        moduleContext.m_pSystemRegistry = m_pSystemRegistry;
        moduleContext.m_pRenderDevice = m_pRenderDevice;

        if ( !m_engineModule.InitializeModule( moduleContext ) )
        {
            return m_fatalErrorHandler( "Failed to initialize engine module!" );
        }

        m_pEntityWorldManager = m_engineModule.GetEntityWorldManager();
        moduleContext.m_pEntityWorldManager = m_pEntityWorldManager;

        if ( !m_gameModule.InitializeModule( moduleContext ) )
        {
            return m_fatalErrorHandler( "Failed to initialize game module" );
        }

        #if EE_DEVELOPMENT_TOOLS
        if ( !InitializeToolsModulesAndSystems( moduleContext ) )
        {
            return false;
        }
        #endif

        //-------------------------------------------------------------------------
        // Load Required Module Resources
        //-------------------------------------------------------------------------

        m_initializationStageReached = Stage::LoadModuleResources;

        m_engineModule.LoadModuleResources( *m_pResourceSystem );
        m_gameModule.LoadModuleResources( *m_pResourceSystem );

        // Wait for all load requests to complete
        while ( m_pResourceSystem->IsBusy() )
        {
            Network::NetworkSystem::Update();
            m_pResourceSystem->Update();
        }

        // Verify that all module resources have been correctly loaded
        if ( !m_engineModule.VerifyModuleResourceLoadingComplete() || !m_gameModule.VerifyModuleResourceLoadingComplete() )
        {
            return m_fatalErrorHandler( "Failed to load required engine resources - See EngineApplication log for details" );
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
        m_renderingSystem.Initialize( m_pRenderDevice, Float2( windowDimensions ), m_engineModule.GetRendererRegistry(), m_pEntityWorldManager );
        m_pSystemRegistry->RegisterSystem( &m_renderingSystem );

        // Create tools UI
        #if EE_DEVELOPMENT_TOOLS
        CreateToolsUI();
        EE_ASSERT( m_pToolsUI != nullptr );
        m_pToolsUI->Initialize( m_updateContext, m_pImguiSystem->GetImageCache() );
        #endif

        //-------------------------------------------------------------------------

        m_initializationStageReached = Stage::FullyInitialized;

        return true;
    }

    bool Engine::Shutdown()
    {
        EE_LOG_INFO( "System", nullptr, "Engine Application Shutdown Started" );

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

        //-------------------------------------------------------------------------
        // Shutdown core engine state
        //-------------------------------------------------------------------------

        if ( m_initializationStageReached == Stage::InitializeEngine )
        {
            // Destroy development tools
            #if EE_DEVELOPMENT_TOOLS
            EE_ASSERT( m_pToolsUI != nullptr );
            m_pToolsUI->Shutdown( m_updateContext );
            DestroyToolsUI();
            EE_ASSERT( m_pToolsUI == nullptr );
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
                m_gameModule.UnloadModuleResources( *m_pResourceSystem );
                m_engineModule.UnloadModuleResources( *m_pResourceSystem );

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
            ModuleContext moduleContext;
            moduleContext.m_pTaskSystem = m_pTaskSystem;
            moduleContext.m_pTypeRegistry = m_pTypeRegistry;
            moduleContext.m_pResourceSystem = m_pResourceSystem;
            moduleContext.m_pSystemRegistry = m_pSystemRegistry;
            moduleContext.m_pRenderDevice = m_pRenderDevice;
            moduleContext.m_pEntityWorldManager = m_pEntityWorldManager;

            #if EE_DEVELOPMENT_TOOLS
            ShutdownToolsModulesAndSystems( moduleContext );
            #endif

            m_gameModule.ShutdownModule( moduleContext );

            m_engineModule.ShutdownModule( moduleContext );
            moduleContext.m_pEntityWorldManager = nullptr;
            m_pEntityWorldManager = nullptr;

            m_initializationStageReached = Stage::InitializeBase;
        }

        //-------------------------------------------------------------------------
        // Shutdown Base
        //-------------------------------------------------------------------------

        if ( m_initializationStageReached == Stage::InitializeBase )
        {
            m_updateContext.m_pSystemRegistry = nullptr;

            EE_DEVELOPMENT_TOOLS_ONLY( m_pImguiSystem = nullptr );
            m_pTaskSystem = nullptr;
            m_pSystemRegistry = nullptr;
            m_pInputSystem = nullptr;
            m_pResourceSystem = nullptr;
            m_pRenderDevice = nullptr;

            m_baseModule.ShutdownModule();

            m_initializationStageReached = Stage::InitializeSettings;
        }

        //-------------------------------------------------------------------------
        // Shutdown Settings
        //-------------------------------------------------------------------------

        if ( m_initializationStageReached == Stage::InitializeSettings )
        {
            auto pSettingsRegistry = m_baseModule.GetSettingsRegistry();
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

        if ( Log::System::HasFatalErrorOccurred() )
        {
            return m_fatalErrorHandler( Log::System::GetFatalError().m_message );
        }

        // Perform Frame Update
        //-------------------------------------------------------------------------

        Profiling::StartFrame();

        Milliseconds deltaTime = 0;
        {
            ScopedTimer<PlatformClock> frameTimer( deltaTime );

            // Frame Start
            //-------------------------------------------------------------------------
            {
                EE_PROFILE_SCOPE_ENTITY( "Frame Start" );
                m_updateContext.m_stage = UpdateStage::FrameStart;

                //-------------------------------------------------------------------------

                {
                    EE_PROFILE_SCOPE_NETWORK( "Networking" );
                    Network::NetworkSystem::Update();
                }

                //-------------------------------------------------------------------------

                m_pEntityWorldManager->StartFrame();

                //-------------------------------------------------------------------------

                #if EE_DEVELOPMENT_TOOLS
                m_pImguiSystem->StartFrame( m_updateContext.GetDeltaTime() );
                m_pToolsUI->StartFrame( m_updateContext );
                m_engineModule.GetConsole()->Update( m_updateContext );
                #endif

                //-------------------------------------------------------------------------

                {
                    EE_PROFILE_SCOPE_RESOURCE( "Resource System" );

                    m_pResourceSystem->Update();

                    // Handle hot-reloading of entities
                    #if EE_DEVELOPMENT_TOOLS
                    if ( m_pResourceSystem->RequiresHotReloading() )
                    {
                        m_pToolsUI->HotReload_UnloadResources( m_pResourceSystem->GetUsersToBeReloaded(), m_pResourceSystem->GetResourcesToBeReloaded() );
                        m_pEntityWorldManager->HotReload_UnloadEntities( m_pResourceSystem->GetUsersToBeReloaded() );

                        // Ensure that all resource requests (both load/unload are completed before continuing with the hot-reload)
                        m_pResourceSystem->ClearHotReloadRequests();
                        while ( m_pResourceSystem->IsBusy() )
                        {
                            Network::NetworkSystem::Update();
                            m_pResourceSystem->Update( true );
                        }

                        m_pToolsUI->HotReload_ReloadResources();
                        m_pEntityWorldManager->HotReload_ReloadEntities();
                    }
                    #endif
                }

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

                #if EE_DEVELOPMENT_TOOLS
                m_pToolsUI->Update( m_updateContext );
                #endif

                m_pEntityWorldManager->UpdateWorlds( m_updateContext );

                //-------------------------------------------------------------------------

                #if EE_DEVELOPMENT_TOOLS
                m_pToolsUI->EndFrame( m_updateContext );
                m_pImguiSystem->EndFrame();
                #endif

                m_pEntityWorldManager->EndFrame();

                m_renderingSystem.Update( m_updateContext );
                m_pInputSystem->PrepareForNewMessages();
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
        EngineClock::Update( deltaTime );
        Profiling::EndFrame();

        // Should we exit?
        //-------------------------------------------------------------------------

        return true;
    }
}