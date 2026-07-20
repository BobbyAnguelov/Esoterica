#pragma once
#include "Base/_Module/API.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE
{
    class SystemRegistry;
    class TaskSystem;
    class ResourceID;
    class SettingsRegistry;
    namespace TypeSystem { class TypeRegistry; }
    namespace Resource { class ResourceSystem; class ResourcePtr; }
    namespace Render { class Window; }

    //-------------------------------------------------------------------------

    struct ModuleContext
    {
        SystemRegistry*                     m_pSystemRegistry = nullptr;
        TaskSystem*                         m_pTaskSystem = nullptr;
        TypeSystem::TypeRegistry*           m_pTypeRegistry = nullptr;
        SettingsRegistry*                   m_pSettingsRegistry = nullptr;
        Resource::ResourceSystem*           m_pResourceSystem = nullptr;
    };

    //-------------------------------------------------------------------------
    // Base class for all modules
    //-------------------------------------------------------------------------

    class EE_BASE_API Module
    {
    public:

        enum LoadingState
        {
            Busy,
            Succeeded,
            Failed
        };

    public:

        virtual ~Module() = default;

        virtual bool InitializeModule( ModuleContext const& context ) = 0;
        virtual void ShutdownModule( ModuleContext const& context ) = 0;

        // Module Resources
        //-------------------------------------------------------------------------
        // These are resources that are persistent for the lifetime of the engine.
        // They are loaded after modules fully initialize but before we start updating the engine.

        virtual TInlineVector<Resource::ResourcePtr*, 10> GetModuleResources() const;

        void LoadModuleResources( Resource::ResourceSystem& resourceSystem );
        LoadingState GetModuleResourceLoadingState();
        void UnloadModuleResources( Resource::ResourceSystem& resourceSystem );

        // Hot Reload
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void HotReload_UnloadResources( Resource::ResourceSystem& resourceSystem, TInlineVector<ResourceID, 20> const& resourcesToUnload );
        void HotReload_ReloadResources( Resource::ResourceSystem& resourceSystem, TInlineVector<ResourceID, 20> const& resourcesToReload );
        #endif

        #if EE_ENABLE_LPP
        virtual void LivePP_PreReload( ModuleContext const& context ) {}
        virtual void LivePP_PostReload( ModuleContext const& context ) {}
        #endif

    protected:

        // Called whenever a module resource is fully loaded
        virtual void OnModuleResourceLoaded( Resource::ResourcePtr* pResource ) {}

        // Called just before a module resource is unloaded
        virtual void OnModuleResourceUnload( Resource::ResourcePtr* pResource ) {}

    private:

        TVector<Resource::ResourcePtr*>     m_currentlyLoadingResources;
        bool                                m_hasResourceLoadingFailed = false;
    };
}