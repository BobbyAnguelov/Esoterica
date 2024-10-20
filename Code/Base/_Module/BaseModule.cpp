#include "BaseModule.h"
#include "Base/Threading/Threading.h"
#include "Base/Network/NetworkSystem.h"
#include "Base/Resource/ResourceProviders/NetworkResourceProvider.h"
#include "Base/Resource/ResourceProviders/PackagedResourceProvider.h"
#include "Base/Resource/Settings/GlobalSettings_Resource.h"
#include "Base/Render/Settings/GlobalSettings_Render.h"

#ifdef _WIN32
#include "Base/Platform/PlatformUtils_Win32.h"
#endif

//-------------------------------------------------------------------------

namespace EE
{
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

    BaseModule::BaseModule()
        : m_taskSystem( Threading::GetProcessorInfo().m_numPhysicalCores - 1 )
        , m_settingsRegistry( m_typeRegistry )
        , m_resourceSystem( m_taskSystem )
    {}

    ModuleContext BaseModule::GetModuleContext() const
    {
        BaseModule* mutableModule = const_cast<BaseModule*>( this );

        ModuleContext moduleContext;
        moduleContext.m_pTaskSystem = &mutableModule->m_taskSystem;
        moduleContext.m_pTypeRegistry = &mutableModule->m_typeRegistry;
        moduleContext.m_pSettingsRegistry = &mutableModule->m_settingsRegistry;
        moduleContext.m_pResourceSystem = &mutableModule->m_resourceSystem;
        moduleContext.m_pSystemRegistry = &mutableModule->m_systemRegistry;
        moduleContext.m_pRenderDevice = &mutableModule->m_renderDevice;
        return moduleContext;
    }

    bool BaseModule::InitializeModule( ModuleContext const& context )
    {
        // Threading
        //-------------------------------------------------------------------------

        m_taskSystem.Initialize();

        // Network
        //-------------------------------------------------------------------------

        if ( !Network::NetworkSystem::Initialize() )
        {
            EE_LOG_ERROR( "Networking", nullptr, "Failed to initialize network system" );
            return false;
        }

        // Resource
        //-------------------------------------------------------------------------

        Resource::ResourceGlobalSettings const* pResourceSettings = m_settingsRegistry.GetGlobalSettings<Resource::ResourceGlobalSettings>();
        EE_ASSERT( pResourceSettings != nullptr );

        #if EE_DEVELOPMENT_TOOLS
        {
            if ( !EnsureResourceServerIsRunning( pResourceSettings->m_resourceServerExecutablePath ) )
            {
                EE_LOG_ERROR( "Resource Provider", nullptr, "Couldn't start resource server (%s)!", pResourceSettings->m_resourceServerExecutablePath.c_str() );
                return false;
            }

            m_pResourceProvider = EE::New<Resource::NetworkResourceProvider>( *pResourceSettings );
        }
        #else
        {
            m_pResourceProvider = EE::New<Resource::PackagedResourceProvider>( *pResourceSettings );
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

        m_resourceSystem.Initialize( m_pResourceProvider );

        // Input
        //-------------------------------------------------------------------------

        m_inputSystem.Initialize();

        // Rendering
        //-------------------------------------------------------------------------

        Render::RenderGlobalSettings const* pRenderSettings = m_settingsRegistry.GetGlobalSettings<Render::RenderGlobalSettings>();
        EE_ASSERT( pRenderSettings != nullptr );

        if ( !m_renderDevice.Initialize( *pRenderSettings ) )
        {
            EE_LOG_ERROR( "Render", nullptr, "Failed to create render device" );
            return false;
        }

        // ImGui
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_imguiSystem.Initialize( &m_renderDevice, &m_inputSystem, true );
        #endif

        // Register Systems
        //-------------------------------------------------------------------------

        m_systemRegistry.RegisterSystem( &m_typeRegistry );
        m_systemRegistry.RegisterSystem( &m_taskSystem );
        m_systemRegistry.RegisterSystem( &m_resourceSystem );
        m_systemRegistry.RegisterSystem( &m_inputSystem );
        m_systemRegistry.RegisterSystem( &m_settingsRegistry );

        return true;
    }

    void BaseModule::ShutdownModule( ModuleContext const& context )
    {
        // Unregister Systems
        //-------------------------------------------------------------------------

        m_systemRegistry.UnregisterSystem( &m_settingsRegistry );
        m_systemRegistry.UnregisterSystem( &m_inputSystem );
        m_systemRegistry.UnregisterSystem( &m_resourceSystem );
        m_systemRegistry.UnregisterSystem( &m_taskSystem );
        m_systemRegistry.UnregisterSystem( &m_typeRegistry );

        // ImGui
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_imguiSystem.Shutdown();
        #endif

        // Rendering
        //-------------------------------------------------------------------------

        m_renderDevice.Shutdown();

        // Input
        //-------------------------------------------------------------------------

        m_inputSystem.Shutdown();

        // Resource
        //-------------------------------------------------------------------------

        m_resourceSystem.Shutdown();

        if ( m_pResourceProvider != nullptr )
        {
            m_pResourceProvider->Shutdown();
            EE::Delete( m_pResourceProvider );
        }

        // Network
        //-------------------------------------------------------------------------

        Network::NetworkSystem::Shutdown();

        // Threading
        //-------------------------------------------------------------------------

        m_taskSystem.Shutdown();
    }
}