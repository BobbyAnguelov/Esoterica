#include "EngineModule.h"
#include "Engine/Entity/EntityLog.h"
#include "Engine/Navmesh/NavPower.h"
#include "Engine/Physics/Physics.h"
#include "Base/Resource/ResourceSystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool EngineModule::InitializeModule( ModuleContext const& context )
    {
        //-------------------------------------------------------------------------
        // Initialize core systems
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_console.Initialize( *context.m_pSettingsRegistry );
        EntityModel::InitializeLogQueue();
        #endif

        Physics::Core::Initialize();
        m_physicsMaterialRegistry.Initialize();

        #if EE_ENABLE_NAVPOWER
        Navmesh::NavPower::Initialize();
        #endif

        //-------------------------------------------------------------------------
        // Initialize and register renderers
        //-------------------------------------------------------------------------

        if ( m_worldRenderer.Initialize( context.m_pRenderDevice ) )
        {
            m_rendererRegistry.RegisterRenderer( &m_worldRenderer );
        }
        else
        {
            EE_LOG_ERROR( "Render", nullptr, "Failed to initialize world renderer" );
            return false;
        }

        #if EE_DEVELOPMENT_TOOLS
        if ( m_debugRenderer.Initialize( context.m_pRenderDevice ) )
        {
            m_rendererRegistry.RegisterRenderer( &m_debugRenderer );
        }
        else
        {
            EE_LOG_ERROR( "Render", nullptr, "Failed to initialize debug renderer" );
            return false;
        }

        if ( m_imguiRenderer.Initialize( context.m_pRenderDevice ) )
        {
            m_rendererRegistry.RegisterRenderer( &m_imguiRenderer );
        }
        else
        {
            EE_LOG_ERROR( "Render", nullptr, "Failed to initialize imgui renderer" );
            return false;
        }

        if ( m_physicsRenderer.Initialize( context.m_pRenderDevice ) )
        {
            m_rendererRegistry.RegisterRenderer( &m_physicsRenderer );
        }
        else
        {
            EE_LOG_ERROR( "Render", nullptr, "Failed to initialize physics renderer" );
            return false;
        }
        #endif

        //-------------------------------------------------------------------------
        // Register systems
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        context.m_pSystemRegistry->RegisterSystem( &m_console );
        #endif

        context.m_pSystemRegistry->RegisterSystem( &m_entityWorldManager );
        context.m_pSystemRegistry->RegisterSystem( &m_rendererRegistry );
        context.m_pSystemRegistry->RegisterSystem( &m_physicsMaterialRegistry );

        //-------------------------------------------------------------------------
        // Register resource loaders
        //-------------------------------------------------------------------------

        m_entityCollectionLoader.SetTypeRegistryPtr( context.m_pTypeRegistry );
        context.m_pResourceSystem->RegisterResourceLoader( &m_entityCollectionLoader );

        //-------------------------------------------------------------------------

        m_renderMeshLoader.SetRenderDevicePtr( context.m_pRenderDevice );
        m_shaderLoader.SetRenderDevicePtr( context.m_pRenderDevice );
        m_textureLoader.SetRenderDevicePtr( context.m_pRenderDevice );

        context.m_pResourceSystem->RegisterResourceLoader( &m_renderMeshLoader );
        context.m_pResourceSystem->RegisterResourceLoader( &m_shaderLoader );
        context.m_pResourceSystem->RegisterResourceLoader( &m_textureLoader );
        context.m_pResourceSystem->RegisterResourceLoader( &m_materialLoader );

        //-------------------------------------------------------------------------

        m_animationClipLoader.SetTypeRegistryPtr( context.m_pTypeRegistry );
        m_graphLoader.SetTypeRegistryPtr( context.m_pTypeRegistry );

        context.m_pResourceSystem->RegisterResourceLoader( &m_skeletonLoader );
        context.m_pResourceSystem->RegisterResourceLoader( &m_animationClipLoader );
        context.m_pResourceSystem->RegisterResourceLoader( &m_graphLoader );
        context.m_pResourceSystem->RegisterResourceLoader( &m_IKRigLoader );

        //-------------------------------------------------------------------------

        m_physicsMaterialLoader.SetMaterialRegistryPtr( &m_physicsMaterialRegistry );

        context.m_pResourceSystem->RegisterResourceLoader( &m_physicsCollisionMeshLoader );
        context.m_pResourceSystem->RegisterResourceLoader( &m_physicsMaterialLoader );
        context.m_pResourceSystem->RegisterResourceLoader( &m_physicsRagdollLoader );

        //-------------------------------------------------------------------------

        context.m_pResourceSystem->RegisterResourceLoader( &m_navmeshLoader );

        return true;
    }

    void EngineModule::ShutdownModule( ModuleContext const& context )
    {
        //-------------------------------------------------------------------------
        // Unregister resource loaders
        //-------------------------------------------------------------------------

        context.m_pResourceSystem->UnregisterResourceLoader( &m_navmeshLoader );

        //-------------------------------------------------------------------------

        context.m_pResourceSystem->UnregisterResourceLoader( &m_physicsRagdollLoader );
        context.m_pResourceSystem->UnregisterResourceLoader( &m_physicsMaterialLoader );
        context.m_pResourceSystem->UnregisterResourceLoader( &m_physicsCollisionMeshLoader );

        m_physicsMaterialLoader.ClearMaterialRegistryPtr();
        m_physicsMaterialRegistry.Shutdown();

        //-------------------------------------------------------------------------

        context.m_pResourceSystem->UnregisterResourceLoader( &m_IKRigLoader );
        context.m_pResourceSystem->UnregisterResourceLoader( &m_graphLoader );
        context.m_pResourceSystem->UnregisterResourceLoader( &m_animationClipLoader );
        context.m_pResourceSystem->UnregisterResourceLoader( &m_skeletonLoader );

        m_animationClipLoader.ClearTypeRegistryPtr();
        m_graphLoader.ClearTypeRegistryPtr();

        //-------------------------------------------------------------------------

        context.m_pResourceSystem->UnregisterResourceLoader( &m_renderMeshLoader );
        context.m_pResourceSystem->UnregisterResourceLoader( &m_shaderLoader );
        context.m_pResourceSystem->UnregisterResourceLoader( &m_textureLoader );
        context.m_pResourceSystem->UnregisterResourceLoader( &m_materialLoader );

        m_renderMeshLoader.ClearRenderDevicePtr();
        m_shaderLoader.ClearRenderDevicePtr();
        m_textureLoader.ClearRenderDevice();

        //-------------------------------------------------------------------------

        context.m_pResourceSystem->UnregisterResourceLoader( &m_entityCollectionLoader );
        m_entityCollectionLoader.ClearTypeRegistryPtr();

        //-------------------------------------------------------------------------
        // Unregister systems
        //-------------------------------------------------------------------------

        context.m_pSystemRegistry->UnregisterSystem( &m_physicsMaterialRegistry );
        context.m_pSystemRegistry->UnregisterSystem( &m_rendererRegistry );
        context.m_pSystemRegistry->UnregisterSystem( &m_entityWorldManager );

        #if EE_DEVELOPMENT_TOOLS
        context.m_pSystemRegistry->UnregisterSystem( &m_console );
        #endif

        //-------------------------------------------------------------------------
        // Unregister and shutdown renderers
        //-------------------------------------------------------------------------

        if ( context.m_pRenderDevice != nullptr )
        {
            #if EE_DEVELOPMENT_TOOLS
            if ( m_physicsRenderer.WasInitialized() )
            {
                m_rendererRegistry.UnregisterRenderer( &m_physicsRenderer );
            }
            m_physicsRenderer.Shutdown();

            if ( m_imguiRenderer.WasInitialized() )
            {
                m_rendererRegistry.UnregisterRenderer( &m_imguiRenderer );
            }
            m_imguiRenderer.Shutdown();

            if ( m_debugRenderer.WasInitialized() )
            {
                m_rendererRegistry.UnregisterRenderer( &m_debugRenderer );
            }
            m_debugRenderer.Shutdown();
            #endif

            if ( m_worldRenderer.WasInitialized() )
            {
                m_rendererRegistry.UnregisterRenderer( &m_worldRenderer );
            }
            m_worldRenderer.Shutdown();
        }

        //-------------------------------------------------------------------------
        // Shutdown core systems
        //-------------------------------------------------------------------------

        #if EE_ENABLE_NAVPOWER
        Navmesh::NavPower::Shutdown();
        #endif

        m_physicsMaterialRegistry.Shutdown();
        Physics::Core::Shutdown();

        #if EE_DEVELOPMENT_TOOLS
        m_console.Shutdown();
        EntityModel::ShutdownLogQueue();
        #endif
    }

    //-------------------------------------------------------------------------

    TInlineVector<Resource::ResourcePtr*, 4> EngineModule::GetModuleResources() const
    {
        EE_ASSERT( m_physicsMaterialDB.IsSet() );

        EngineModule* pMutableModule = const_cast<EngineModule*>( this );

        TInlineVector<Resource::ResourcePtr*, 4> resources;
        resources.emplace_back( &pMutableModule->m_physicsMaterialDB );
        return resources;
    }
}