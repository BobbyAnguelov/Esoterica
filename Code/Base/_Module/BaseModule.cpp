#include "BaseModule.h"
#include "Base/Threading/Threading.h"
#include "Base/Network/NetworkSystem.h"
#include "Base/Resource/ResourceProviders/ResourceProvider_Network.h"
#include "Base/Resource/ResourceProviders/ResourceProvider_Package.h"
#include "Base/Resource/Settings/Settings_Resource.h"
#include "Base/Render/RHI.h"

#ifdef _WIN32
#include "Base/Platform/PlatformUtils_Win32.h"
#endif

//-------------------------------------------------------------------------

namespace EE
{
    #if EE_DEVELOPMENT_TOOLS
    bool EnsureResourceServerIsRunning( FileSystem::Path const& resourceServerExecutablePath, String const& resourceServerNetworkAddress )
    {
        #if _WIN32

        // Don't start resource server when it's running on a remote PC
        if ( resourceServerNetworkAddress != "127.0.0.1" && resourceServerNetworkAddress != "localhost" )
        {
            EE_LOG_MESSAGE( LogCategory::Resource, "Resource Server", "Resource server is running remotely" );
            return true;
        }
        
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
            EE_LOG_ERROR( LogCategory::Network, nullptr, "Failed to initialize network system" );
            return false;
        }

        // Resource
        //-------------------------------------------------------------------------

        Resource::ResourceSettings const* pResourceSettings = m_settingsRegistry.GetSettings<Resource::ResourceSettings>();
        EE_ASSERT( pResourceSettings != nullptr );
    
        if ( pResourceSettings->UsePackagedResourceProvider() )
        {
            m_pResourceProvider = EE::New<Resource::PackagedResourceProvider>( *pResourceSettings );
        }
        else
        {
            #if EE_DEVELOPMENT_TOOLS
            if ( !EnsureResourceServerIsRunning( pResourceSettings->m_resourceServerExecutablePath, pResourceSettings->m_resourceServerNetworkAddress ) )
            {
                EE_LOG_ERROR( LogCategory::Resource, "Resource Provider", "Couldn't start resource server (%s)!", pResourceSettings->m_resourceServerExecutablePath.c_str() );
                return false;
            }

            m_pResourceProvider = EE::New<Resource::NetworkResourceProvider>( *pResourceSettings );
            #else
            EE_UNREACHABLE_CODE();
            #endif
        }

        if ( m_pResourceProvider == nullptr )
        {
            EE_LOG_ERROR( LogCategory::Resource, nullptr, "Failed to create resource provider" );
            return false;
        }

        if ( !m_pResourceProvider->Initialize() )
        {
            EE_LOG_ERROR( LogCategory::Resource, nullptr, "Failed to intialize resource provider" );
            EE::Delete( m_pResourceProvider );
            return false;
        }

        m_resourceSystem.Initialize( m_pResourceProvider );

        // Input
        //-------------------------------------------------------------------------

        m_inputSystem.Initialize();

        // ImGui
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_imguiSystem.Initialize( &m_inputSystem, true );
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

        Render::RHI::ReportDeviceMemoryLeaks();

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