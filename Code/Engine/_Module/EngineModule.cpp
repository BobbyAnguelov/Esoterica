#include "EngineModule.h"
#include "System/Resource/ResourceProviders/NetworkResourceProvider.h"
#include "System/Resource/ResourceProviders/PackagedResourceProvider.h"
#include "System/Network/NetworkSystem.h"
#include "Engine/Entity/EntityLog.h"

//-------------------------------------------------------------------------

#ifdef _WIN32
#include "System/Platform/PlatformHelpers_Win32.h"
#endif

//-------------------------------------------------------------------------

namespace EE
{
    namespace
    {
        constexpr static char const* const g_physicsMaterialDatabaseResourceID = "data://Physics/PhysicsMaterials.pmdb";
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    bool EnsureResourceServerIsRunning( FileSystem::Path const& resourceServerExecutablePath )
    {
        #if _WIN32
        bool shouldStartResourceServer = false;

        // If the resource server is not running then start it
        String const resourceServerExecutableName = resourceServerExecutablePath.GetFilename();
        uint32_t const resourceServerProcessID = Platform::Win32::GetProcessID( resourceServerExecutableName.c_str() );
        shouldStartResourceServer = ( resourceServerProcessID == 0 );

        // Ensure we are running the correct build of the resource server
        if ( !shouldStartResourceServer )
        {
            String const resourceServerPath = Platform::Win32::GetProcessPath( resourceServerProcessID );
            if ( !resourceServerPath.empty() )
            {
                FileSystem::Path const resourceServerProcessPath = FileSystem::Path( resourceServerPath ).GetParentDirectory();
                FileSystem::Path const applicationDirectoryPath = FileSystem::Path( Platform::Win32::GetCurrentModulePath() ).GetParentDirectory();

                if ( resourceServerProcessPath != applicationDirectoryPath )
                {
                    Platform::Win32::KillProcess( resourceServerProcessID );
                    shouldStartResourceServer = true;
                }
            }
            else
            {
                return false;
            }
        }

        // Try to start the resource server
        if ( shouldStartResourceServer )
        {
            FileSystem::Path const applicationDirectoryPath = FileSystem::Path( Platform::Win32::GetCurrentModulePath() ).GetParentDirectory();
            return Platform::Win32::StartProcess( resourceServerExecutablePath.c_str() ) != 0;
        }

        return true;
        #else
        return false;
        #endif
    }
    #endif

    //-------------------------------------------------------------------------

    void EngineModule::GetListOfAllRequiredModuleResources( TVector<ResourceID>& outResourceIDs )
    {
        outResourceIDs.emplace_back( ResourceID( g_physicsMaterialDatabaseResourceID ) );
    }

    //-------------------------------------------------------------------------

    bool EngineModule::InitializeCoreSystems( IniFile const& iniFile )
    {
        #if EE_DEVELOPMENT_TOOLS
        EntityModel::InitializeLogQueue();
        #endif

        if ( !Network::NetworkSystem::Initialize() )
        {
            EE_LOG_ERROR( "Render", nullptr, "Failed to initialize network system" );
            return false;
        }

        // Create and initialize resource provider
        //-------------------------------------------------------------------------

        Resource::ResourceSettings settings;
        if ( !settings.ReadSettings( iniFile ) )
        {
            EE_LOG_ERROR( "Resource Provider", nullptr, "Failed to read resource settings from ini file!" );
            return false;
        }

        #if EE_DEVELOPMENT_TOOLS
        {
            if ( !EnsureResourceServerIsRunning( settings.m_resourceServerExecutablePath ) )
            {
                EE_LOG_ERROR( "Resource Provider", nullptr, "Couldn't start resource server (%s)!", settings.m_resourceServerExecutablePath.c_str() );
                return false;
            }

            m_pResourceProvider = EE::New<Resource::NetworkResourceProvider>( settings );
        }
        #else
        {
            m_pResourceProvider = EE::New<Resource::PackagedResourceProvider>( settings );
        }
        #endif

        if ( m_pResourceProvider == nullptr )
        {
            EE_LOG_ERROR( "Resource", nullptr, "Failed to create resource provider" );
            return false;
        }

        if ( !m_pResourceProvider->Initialize() )
        {
            EE_LOG_ERROR( "Resource", nullptr, "Failed to intialize resource provider" );
            EE::Delete( m_pResourceProvider );
            return false;
        }

        // Create and initialize render device
        //-------------------------------------------------------------------------

        m_pRenderDevice = EE::New<Render::RenderDevice>();
        if ( !m_pRenderDevice->Initialize( iniFile ) )
        {
            EE_LOG_ERROR( "Render", nullptr, "Failed to create render device" );
            EE::Delete( m_pRenderDevice );
            return false;
        }

        // Initialize core systems
        //-------------------------------------------------------------------------

        m_taskSystem.Initialize();
        m_resourceSystem.Initialize( m_pResourceProvider );
        m_inputSystem.Initialize();
        m_physicsSystem.Initialize();

        #if EE_DEVELOPMENT_TOOLS
        m_imguiSystem.Initialize( m_pRenderDevice, &m_inputSystem, m_imguiViewportsEnabled );
        #endif

        // Initialize and register renderers
        //-------------------------------------------------------------------------

        if ( m_worldRenderer.Initialize( m_pRenderDevice ) )
        {
            m_rendererRegistry.RegisterRenderer( &m_worldRenderer );
        }
        else
        {
            EE_LOG_ERROR( "Render", nullptr, "Failed to initialize world renderer" );
            return false;
        }

        #if EE_DEVELOPMENT_TOOLS
        if ( m_debugRenderer.Initialize( m_pRenderDevice ) )
        {
            m_rendererRegistry.RegisterRenderer( &m_debugRenderer );
        }
        else
        {
            EE_LOG_ERROR( "Render", nullptr, "Failed to initialize debug renderer" );
            return false;
        }

        if ( m_imguiRenderer.Initialize( m_pRenderDevice ) )
        {
            m_rendererRegistry.RegisterRenderer( &m_imguiRenderer );
        }
        else
        {
            EE_LOG_ERROR( "Render", nullptr, "Failed to initialize imgui renderer" );
            return false;
        }

        if ( m_physicsRenderer.Initialize( m_pRenderDevice ) )
        {
            m_rendererRegistry.RegisterRenderer( &m_physicsRenderer );
        }
        else
        {
            EE_LOG_ERROR( "Render", nullptr, "Failed to initialize physics renderer" );
            return false;
        }
        #endif

        return true;
    }

    void EngineModule::ShutdownCoreSystems()
    {
        EE_ASSERT( !m_moduleInitialized );

        bool const coreSystemsInitialized = m_pRenderDevice != nullptr;

        // Unregister and shutdown renderers
        //-------------------------------------------------------------------------

        if ( m_pRenderDevice != nullptr )
        {
            #if EE_DEVELOPMENT_TOOLS
            if ( m_physicsRenderer.IsInitialized() )
            {
                m_rendererRegistry.UnregisterRenderer( &m_physicsRenderer );
            }
            m_physicsRenderer.Shutdown();

            if ( m_imguiRenderer.IsInitialized() )
            {
                m_rendererRegistry.UnregisterRenderer( &m_imguiRenderer );
            }
            m_imguiRenderer.Shutdown();

            if ( m_debugRenderer.IsInitialized() )
            {
                m_rendererRegistry.UnregisterRenderer( &m_debugRenderer );
            }
            m_debugRenderer.Shutdown();
            #endif

            if ( m_worldRenderer.IsInitialized() )
            {
                m_rendererRegistry.UnregisterRenderer( &m_worldRenderer );
            }
            m_worldRenderer.Shutdown();
        }

        // Shutdown core systems
        //-------------------------------------------------------------------------

        if ( coreSystemsInitialized )
        {
            #if EE_DEVELOPMENT_TOOLS
            m_imguiSystem.Shutdown();
            #endif

            m_physicsSystem.Shutdown();
            m_inputSystem.Shutdown();
            m_resourceSystem.Shutdown();
            m_taskSystem.Shutdown();
        }

        // Destroy render device and resource provider
        //-------------------------------------------------------------------------

        if ( m_pRenderDevice != nullptr )
        {
            m_pRenderDevice->Shutdown();
            EE::Delete( m_pRenderDevice );
        }

        if ( m_pResourceProvider != nullptr )
        {
            m_pResourceProvider->Shutdown();
            EE::Delete( m_pResourceProvider );
        }

        Network::NetworkSystem::Shutdown();

        #if EE_DEVELOPMENT_TOOLS
        EntityModel::ShutdownLogQueue();
        #endif
    }

    //-------------------------------------------------------------------------

    bool EngineModule::InitializeModule()
    {
        EE_ASSERT( m_pRenderDevice != nullptr );

        // Register systems
        //-------------------------------------------------------------------------

        m_systemRegistry.RegisterSystem( &m_typeRegistry );
        m_systemRegistry.RegisterSystem( &m_taskSystem );
        m_systemRegistry.RegisterSystem( &m_resourceSystem );
        m_systemRegistry.RegisterSystem( &m_inputSystem );
        m_systemRegistry.RegisterSystem( &m_entityWorldManager );
        m_systemRegistry.RegisterSystem( &m_physicsSystem );
        m_systemRegistry.RegisterSystem( &m_rendererRegistry );

        #if EE_ENABLE_NAVPOWER
        m_systemRegistry.RegisterSystem( &m_navmeshSystem );
        #endif

        // Register resource loaders
        //-------------------------------------------------------------------------

        m_entityCollectionLoader.SetTypeRegistryPtr( &m_typeRegistry );
        m_resourceSystem.RegisterResourceLoader( &m_entityCollectionLoader );

        //-------------------------------------------------------------------------

        m_renderMeshLoader.SetRenderDevicePtr( m_pRenderDevice );
        m_shaderLoader.SetRenderDevicePtr( m_pRenderDevice );
        m_textureLoader.SetRenderDevicePtr( m_pRenderDevice );

        m_resourceSystem.RegisterResourceLoader( &m_renderMeshLoader );
        m_resourceSystem.RegisterResourceLoader( &m_shaderLoader );
        m_resourceSystem.RegisterResourceLoader( &m_textureLoader );
        m_resourceSystem.RegisterResourceLoader( &m_materialLoader );

        //-------------------------------------------------------------------------

        m_animationClipLoader.SetTypeRegistryPtr( &m_typeRegistry );
        m_graphLoader.SetTypeRegistryPtr( &m_typeRegistry );

        m_resourceSystem.RegisterResourceLoader( &m_skeletonLoader );
        m_resourceSystem.RegisterResourceLoader( &m_boneMaskLoader );
        m_resourceSystem.RegisterResourceLoader( &m_animationClipLoader );
        m_resourceSystem.RegisterResourceLoader( &m_graphLoader );

        //-------------------------------------------------------------------------

        m_physicsMeshLoader.SetPhysicsSystemPtr( &m_physicsSystem );
        m_physicsMaterialLoader.SetPhysicsSystemPtr( &m_physicsSystem );
        m_physicsRagdollLoader.SetPhysicsSystemPtr( &m_physicsSystem );

        m_resourceSystem.RegisterResourceLoader( &m_physicsMeshLoader );
        m_resourceSystem.RegisterResourceLoader( &m_physicsMaterialLoader );
        m_resourceSystem.RegisterResourceLoader( &m_physicsRagdollLoader );

        //-------------------------------------------------------------------------

        m_resourceSystem.RegisterResourceLoader( &m_navmeshLoader );

        //-------------------------------------------------------------------------

        m_moduleInitialized = true;

        return true;
    }

    void EngineModule::ShutdownModule()
    {
        EE_ASSERT( m_pRenderDevice != nullptr );

        // Unregister resource loaders
        //-------------------------------------------------------------------------

        m_resourceSystem.UnregisterResourceLoader( &m_navmeshLoader );

        //-------------------------------------------------------------------------

        m_resourceSystem.UnregisterResourceLoader( &m_physicsRagdollLoader );
        m_resourceSystem.UnregisterResourceLoader( &m_physicsMaterialLoader );
        m_resourceSystem.UnregisterResourceLoader( &m_physicsMeshLoader );

        m_physicsRagdollLoader.ClearPhysicsSystemPtr();
        m_physicsMaterialLoader.ClearPhysicsSystemPtr();
        m_physicsMeshLoader.ClearPhysicsSystemPtr();

        //-------------------------------------------------------------------------

        m_resourceSystem.UnregisterResourceLoader( &m_animationClipLoader );
        m_resourceSystem.UnregisterResourceLoader( &m_graphLoader );
        m_resourceSystem.UnregisterResourceLoader( &m_boneMaskLoader );
        m_resourceSystem.UnregisterResourceLoader( &m_skeletonLoader );

        m_animationClipLoader.ClearTypeRegistryPtr();
        m_graphLoader.ClearTypeRegistryPtr();

        //-------------------------------------------------------------------------

        m_resourceSystem.UnregisterResourceLoader( &m_renderMeshLoader );
        m_resourceSystem.UnregisterResourceLoader( &m_shaderLoader );
        m_resourceSystem.UnregisterResourceLoader( &m_textureLoader );
        m_resourceSystem.UnregisterResourceLoader( &m_materialLoader );

        m_renderMeshLoader.ClearRenderDevicePtr();
        m_shaderLoader.ClearRenderDevicePtr();
        m_textureLoader.ClearRenderDevice();

        //-------------------------------------------------------------------------

        m_resourceSystem.UnregisterResourceLoader( &m_entityCollectionLoader );
        m_entityCollectionLoader.ClearTypeRegistryPtr();

        // Unregister systems
        //-------------------------------------------------------------------------

        m_systemRegistry.UnregisterSystem( &m_rendererRegistry );

        m_systemRegistry.UnregisterSystem( &m_entityWorldManager );
        m_systemRegistry.UnregisterSystem( &m_inputSystem );
        m_systemRegistry.UnregisterSystem( &m_resourceSystem );
        m_systemRegistry.UnregisterSystem( &m_taskSystem );
        m_systemRegistry.UnregisterSystem( &m_typeRegistry );
        m_systemRegistry.UnregisterSystem( &m_physicsSystem );

        #if EE_ENABLE_NAVPOWER
        m_systemRegistry.UnregisterSystem( &m_navmeshSystem );
        #endif

        //-------------------------------------------------------------------------

        m_moduleInitialized = false;
    }

    void EngineModule::LoadModuleResources( Resource::ResourceSystem& resourceSystem )
    {
        m_pPhysicMaterialDB = ResourceID( g_physicsMaterialDatabaseResourceID );
        EE_ASSERT( m_pPhysicMaterialDB.IsSet() );
        resourceSystem.LoadResource( m_pPhysicMaterialDB );
    }

    bool EngineModule::VerifyModuleResourceLoadingComplete()
    {
        return m_pPhysicMaterialDB.IsLoaded() && m_pPhysicMaterialDB->IsValid();
    }

    void EngineModule::UnloadModuleResources( Resource::ResourceSystem& resourceSystem )
    {
        resourceSystem.UnloadResource( m_pPhysicMaterialDB );
    }
}