#include "EngineModule.h"
#include "Engine/Entity/EntityLog.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Navmesh/NavPower.h"
#include "Engine/Physics/Physics.h"
#include "Engine/ThirdParty/meshoptimizer/meshoptimizer_esoterica.h"
#include "Base/Render/Settings/Settings_Render.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/Platform/Platform.h"

//-------------------------------------------------------------------------

#if EE_ENABLE_LPP
#include "Base/Memory/Memory.h"
#include "LPP_API_x64_CPP.h"
#include "Base/Types/Event.h"

namespace EE
{
    static TEvent<const wchar_t* const* const, unsigned int> g_event;

    static void FunctionCalledBeforePatching( lpp::LppHotReloadPrepatchHookId, const wchar_t* const recompiledModulePath, const wchar_t* const* const modifiedFiles, unsigned int modifiedFilesCount, const wchar_t* const* const, unsigned int )
    {
        Memory::InitializeThreadHeap();
        g_event.Execute( modifiedFiles, modifiedFilesCount );
    }

    LPP_HOTRELOAD_PREPATCH_HOOK( FunctionCalledBeforePatching );
}
#endif

//-------------------------------------------------------------------------

namespace EE
{
    bool EngineModule::InitializeModule( ModuleContext const& context )
    {
        #if EE_ENABLE_LPP
        auto PreLoad = [this] ( const wchar_t* const* const modifiedFiles, unsigned int modifiedFilesCount )
        {
            for ( uint32_t i = 0; i < modifiedFilesCount; i++ )
            {
                m_modifiedFiles.emplace_back( String( String::CtorConvert(), modifiedFiles[i] ) );
            }
        };

        m_lppReloadEventBindingID = g_event.Bind( PreLoad );
        #endif

        //-------------------------------------------------------------------------
        // Initialize core systems
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        EntityModel::InitializeLogQueue();
        #endif

        Render::MeshOptimizer::Initialize();
        Physics::Core::Initialize();
        m_physicsMaterialRegistry.Initialize();

        #if EE_ENABLE_NAVPOWER
        Navmesh::NavPower::Initialize();
        #endif

        //-------------------------------------------------------------------------
        // Animation
        //-------------------------------------------------------------------------

        Animation::TaskSystem::InitializeTaskTypesList( *context.m_pTypeRegistry );

        //-------------------------------------------------------------------------
        // Register systems
        //-------------------------------------------------------------------------

        context.m_pSystemRegistry->RegisterSystem( &m_entityWorldManager );
        context.m_pSystemRegistry->RegisterSystem( &m_physicsMaterialRegistry );

        context.m_pSystemRegistry->RegisterSystem( &m_renderSystem );

        #if EE_DEVELOPMENT_TOOLS
        context.m_pSystemRegistry->RegisterSystem( &m_debugMeshRegistry );
        context.m_pSystemRegistry->RegisterSystem( &m_imguiRenderer );
        #endif

        //-------------------------------------------------------------------------
        // Register resource loaders
        //-------------------------------------------------------------------------

        m_entityCollectionLoader.SetTypeRegistryPtr( context.m_pTypeRegistry );
        context.m_pResourceSystem->RegisterResourceLoader( &m_entityCollectionLoader );

        //-------------------------------------------------------------------------

        m_meshLoader.SetRenderSystem( &m_renderSystem );
        m_textureLoader.SetRenderSystem( &m_renderSystem );
        m_materialLoader.SetRenderSystem( &m_renderSystem );

        context.m_pResourceSystem->RegisterResourceLoader( &m_meshLoader );
        context.m_pResourceSystem->RegisterResourceLoader( &m_textureLoader );
        context.m_pResourceSystem->RegisterResourceLoader( &m_materialLoader );

        //-------------------------------------------------------------------------

        m_animationClipLoader.SetTypeRegistryPtr( context.m_pTypeRegistry );
        m_graphLoader.SetTypeRegistryPtr( context.m_pTypeRegistry );

        context.m_pResourceSystem->RegisterResourceLoader( &m_skeletonLoader );
        context.m_pResourceSystem->RegisterResourceLoader( &m_animationClipLoader );
        context.m_pResourceSystem->RegisterResourceLoader( &m_graphLoader );

        //-------------------------------------------------------------------------

        m_physicsMaterialLoader.SetMaterialRegistryPtr( &m_physicsMaterialRegistry );

        context.m_pResourceSystem->RegisterResourceLoader( &m_physicsCollisionMeshLoader );
        context.m_pResourceSystem->RegisterResourceLoader( &m_physicsMaterialLoader );
        context.m_pResourceSystem->RegisterResourceLoader( &m_physicsRagdollLoader );

        //-------------------------------------------------------------------------

        context.m_pResourceSystem->RegisterResourceLoader( &m_navmeshLoader );

        //-------------------------------------------------------------------------

        context.m_pResourceSystem->RegisterResourceLoader( &m_hitboxLoader );

        //-------------------------------------------------------------------------
        // Initialize and register renderers
        //-------------------------------------------------------------------------

        auto const pRenderSettings = context.m_pSettingsRegistry->GetSettings<Render::RenderSettings>();

        m_renderSystem.Initialize( *pRenderSettings );
        m_renderWindow.SetNativeWindowHandle( Platform::GetMainWindowHandle() );
        m_renderSystem.RegisterRenderWindow( &m_renderWindow );

        m_renderSystem.StartResourceUpdates( true );

        m_forwardShadingRenderer.Initialize( context.m_pSystemRegistry, *pRenderSettings );

        #if EE_DEVELOPMENT_TOOLS
        m_debugMeshRegistry.Initialize( &m_renderSystem );
        m_imguiRenderer.Initialize( &m_renderWindow, &m_renderSystem );
        #endif

        m_renderSystem.SubmitResourceUpdates( true );

        return true;
    }

    void EngineModule::ShutdownModule( ModuleContext const& context )
    {
        #if EE_ENABLE_LPP
        g_event.Unbind( m_lppReloadEventBindingID );
        #endif

        //-------------------------------------------------------------------------
        // Unregister resource loaders
        //-------------------------------------------------------------------------

        context.m_pResourceSystem->UnregisterResourceLoader( &m_hitboxLoader );

        //-------------------------------------------------------------------------

        context.m_pResourceSystem->UnregisterResourceLoader( &m_navmeshLoader );

        //-------------------------------------------------------------------------

        context.m_pResourceSystem->UnregisterResourceLoader( &m_physicsRagdollLoader );
        context.m_pResourceSystem->UnregisterResourceLoader( &m_physicsMaterialLoader );
        context.m_pResourceSystem->UnregisterResourceLoader( &m_physicsCollisionMeshLoader );

        m_physicsMaterialLoader.ClearMaterialRegistryPtr();
        m_physicsMaterialRegistry.Shutdown();

        //-------------------------------------------------------------------------

        context.m_pResourceSystem->UnregisterResourceLoader( &m_graphLoader );
        context.m_pResourceSystem->UnregisterResourceLoader( &m_animationClipLoader );
        context.m_pResourceSystem->UnregisterResourceLoader( &m_skeletonLoader );

        m_animationClipLoader.ClearTypeRegistryPtr();
        m_graphLoader.ClearTypeRegistryPtr();

        //-------------------------------------------------------------------------

        context.m_pResourceSystem->UnregisterResourceLoader( &m_meshLoader );
        context.m_pResourceSystem->UnregisterResourceLoader( &m_textureLoader );
        context.m_pResourceSystem->UnregisterResourceLoader( &m_materialLoader );

        //-------------------------------------------------------------------------

        context.m_pResourceSystem->UnregisterResourceLoader( &m_entityCollectionLoader );
        m_entityCollectionLoader.ClearTypeRegistryPtr();

        //-------------------------------------------------------------------------
        // Unregister systems
        //-------------------------------------------------------------------------

        context.m_pSystemRegistry->UnregisterSystem( &m_physicsMaterialRegistry );
        context.m_pSystemRegistry->UnregisterSystem( &m_entityWorldManager );

        //-------------------------------------------------------------------------
        // Animation
        //-------------------------------------------------------------------------

        Animation::TaskSystem::ShutdownTaskTypesList();

        //-------------------------------------------------------------------------
        // Unregister and shutdown renderers
        //-------------------------------------------------------------------------

        m_renderSystem.UnregisterRenderWindow( &m_renderWindow );
        m_renderWindow.DestroySwapchain( m_renderSystem.GetContextRHI() );

        #if EE_DEVELOPMENT_TOOLS
        m_imguiRenderer.Shutdown();
        m_debugMeshRegistry.Shutdown();

        context.m_pSystemRegistry->UnregisterSystem( &m_imguiRenderer );
        context.m_pSystemRegistry->UnregisterSystem( &m_debugMeshRegistry );
        #endif

        m_forwardShadingRenderer.Shutdown();
        m_renderSystem.Shutdown();
        context.m_pSystemRegistry->UnregisterSystem( &m_renderSystem );

        //-------------------------------------------------------------------------
        // Shutdown core systems
        //-------------------------------------------------------------------------

        #if EE_ENABLE_NAVPOWER
        Navmesh::NavPower::Shutdown();
        #endif

        m_physicsMaterialRegistry.Shutdown();
        Physics::Core::Shutdown();
        Render::MeshOptimizer::Shutdown();

        #if EE_DEVELOPMENT_TOOLS
        EntityModel::ShutdownLogQueue();
        #endif
    }

    void EngineModule::OnModuleResourceLoaded( Resource::ResourcePtr* pResource )
    {
        if ( pResource->GetResourceID() == m_tonemapLUT.GetResourceID() )
        {
            Render::TextureResource const* pTextureResource = pResource->GetPtr<Render::TextureResource>();
            m_renderSystem.SetTonemapLUT( pTextureResource->GetTexture() );
        }

        if ( pResource->GetResourceID() == m_smaaAreaTexture.GetResourceID() )
        {
            Render::TextureResource const* pTextureResource = pResource->GetPtr<Render::TextureResource>();
            m_renderSystem.SetSMAAAreaTexture( pTextureResource->GetTexture() );
        }

        if ( pResource->GetResourceID() == m_smaaSearchTexture.GetResourceID() )
        {
            Render::TextureResource const* pTextureResource = pResource->GetPtr<Render::TextureResource>();
            m_renderSystem.SetSMAASearchTexture( pTextureResource->GetTexture() );
        }

        if ( pResource->GetResourceID() == m_placeholderMaterial.GetResourceID() )
        {
            Render::Material const* pMaterialResource = pResource->GetPtr<Render::Material>();
            m_renderSystem.SetPlaceholderMaterial( pMaterialResource );
        }
    }

    void EngineModule::OnModuleResourceUnload( Resource::ResourcePtr* pResource )
    {
        // Do Nothing
    }

    //-------------------------------------------------------------------------

    TInlineVector<Resource::ResourcePtr*, 10> EngineModule::GetModuleResources() const
    {
        EE_ASSERT( m_physicsMaterialDB.IsSet() );

        EngineModule* pMutableModule = const_cast<EngineModule*>( this );

        TInlineVector<Resource::ResourcePtr*, 10> resources;
        resources.emplace_back( &pMutableModule->m_tonemapLUT );
        resources.emplace_back( &pMutableModule->m_smaaAreaTexture );
        resources.emplace_back( &pMutableModule->m_smaaSearchTexture );
        resources.emplace_back( &pMutableModule->m_physicsMaterialDB );
        resources.emplace_back( &pMutableModule->m_placeholderMaterial );
        return resources;
    }

    //-------------------------------------------------------------------------

    #if EE_ENABLE_LPP
    void EngineModule::LivePP_PreReload( ModuleContext const& context )
    {
        m_needShaderReload = false;
        for ( FileSystem::Path const& modifiedFilePath : m_modifiedFiles )
        {
            String const& pathString = modifiedFilePath.GetString();
            if ( pathString.find( "_Module\\_AutoGenerated\\Shaders" ) != String::npos )
            {
                m_needShaderReload = true;
                break;
            }
        }

        if ( m_needShaderReload )
        {
            m_renderSystem.WaitAllQueuesIdle();

            #if EE_DEVELOPMENT_TOOLS
            m_imguiRenderer.PreShaderHotReload();
            #endif
            m_forwardShadingRenderer.Shutdown();

            m_renderSystem.ShutdownShaders();
        }
    }

    void EngineModule::LivePP_PostReload( ModuleContext const& context )
    {
        if ( m_needShaderReload )
        {
            auto const pRenderSettings = context.m_pSettingsRegistry->GetSettings<Render::RenderSettings>();

            m_renderSystem.InitializeShaders();
            m_forwardShadingRenderer.Initialize( context.m_pSystemRegistry, *pRenderSettings );

            #if EE_DEVELOPMENT_TOOLS
            m_imguiRenderer.PostShaderHotReload();
            #endif
        }

        m_modifiedFiles.clear();
        m_needShaderReload = false;
    }
    #endif
}
