#include "ResourceServerContext.h"
#include "Base/Network/NetworkSystem.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceServerContext::ResourceServerContext( TypeSystem::TypeRegistry const& typeRegistry )
        : m_typeRegistry( typeRegistry )
        , m_settingsRegistry( typeRegistry )
    {}

    ResourceServerContext::~ResourceServerContext()
    {
        EE_ASSERT( m_pCompilerRegistry == nullptr );
        EE_ASSERT( m_pSettings == nullptr );
    }

    bool ResourceServerContext::Initialize( FileSystem::Path const& iniFilePath )
    {
        // Initialize Settings
        //-------------------------------------------------------------------------

        if ( !m_settingsRegistry.Initialize( iniFilePath ) )
        {
            return false;
        }

        m_pSettings = m_settingsRegistry.GetSettings<Resource::ResourceSettings>();
        EE_ASSERT( m_pSettings != nullptr );

        m_pSettings->m_sourceDataDirectoryPath.EnsureDirectoryExists();
        m_pSettings->m_compiledResourceDirectoryPath.EnsureDirectoryExists();

        // Register types
        //-------------------------------------------------------------------------

        m_pCompilerRegistry = EE::New<CompilerRegistry>( m_typeRegistry, m_pSettings->m_sourceDataDirectoryPath );

        // Open network connection
        //-------------------------------------------------------------------------

        if ( !Network::NetworkSystem::Initialize() )
        {
            EE_LOG_ERROR( LogCategory::Resource, "Resource Server", "Failed to initialize network system" );
            return false;
        }

        if ( !m_networkServer.Start( m_pSettings->m_resourceServerPort ) )
        {
            EE_LOG_ERROR( LogCategory::Resource, "Resource Server", "Cant open network connection on port: %d", m_pSettings->m_resourceServerPort );
            return false;
        }

        // Compiled Resource DB
        //-------------------------------------------------------------------------

        if ( !m_compileResourceDB.Connect( m_pSettings->m_compiledResourceDatabasePath ) )
        {
            EE_LOG_ERROR( LogCategory::Resource, "Resource Server", "Cant connect to the compiled resource DB" );
            return false;
        }

        // Task System
        //-------------------------------------------------------------------------

        m_taskSystem.Initialize();

        return true;
    }

    void ResourceServerContext::Shutdown()
    {
        m_taskSystem.WaitForAll();
        m_taskSystem.Shutdown();

        m_compileResourceDB.Disconnect();

        m_networkServer.Stop();
        Network::NetworkSystem::Shutdown();

        //-------------------------------------------------------------------------

        EE::Delete( m_pCompilerRegistry );

        //-------------------------------------------------------------------------

        m_pSettings = nullptr;
        m_settingsRegistry.Shutdown();
    }
}
