#pragma once

#include "_Module/API.h"
#include "Engine/_Module/EngineModule.h"
#include "Engine/ToolsUI/ToolsUI.h"
#include "Engine/UpdateContext.h"
#include "Base/_Module/BaseModule.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINE_API Engine
    {
        class EngineUpdateContext : public UpdateContext
        {
            friend Engine;
        };

        enum class Stage
        {
            Uninitialized,
            RegisterTypeInfo,
            InitializeSettings,
            InitializeModules,
            LoadModuleResources,
            InitializeEngine,
            FullyInitialized
        };

    public:

        Engine( TFunction<bool( EE::String const& error )>&& errorHandler );
        virtual ~Engine();

        bool Initialize( int32_t argc, char** argv, Int2 const& windowDimensions );
        bool Shutdown();
        bool Update();

        // Needed for window processor access
        Input::InputSystem* GetInputSystem() { return m_pInputSystem; }

        // Resize the main window for the engine
        virtual void ResizeMainWindow( Int2 newMainWindowDimensions );

        // Live++
        #if EE_ENABLE_LPP
        void LivePP_PreReload();
        void LivePP_PostReload();
        #endif

    protected:

        virtual void RegisterTypes() = 0;
        virtual void UnregisterTypes() = 0;

        // Called once the core engine has been successfully initialized, allows derived engine classes to create tools UI and initialize various other systems
        virtual void PostInitialize() {};

        // Called just before we shutdown the engine
        virtual void PreShutdown() {};

        // Set the startup map - only called if there is a startup map set
        virtual void SetStartupMap( ResourceID const& mapID );

        #if EE_DEVELOPMENT_TOOLS
        virtual void CreateToolsUI() = 0;
        #endif

        inline BaseModule* GetBaseModule() { return static_cast<BaseModule*>( m_modules[0] ); }
        inline EngineModule* GetEngineModule() { return static_cast<EngineModule*>( m_modules[1] ); }

    private:

        // This is a blocking call to update the resource and render system to process all pending load/unloads requests
        void UpdateResourceLoadingRequests_Blocking();

    protected:

        TFunction<bool( EE::String const& error )>      m_fatalErrorHandler;

        // Modules
        //-------------------------------------------------------------------------

        TVector<Module*>                                m_modules;

        // Contexts
        //-------------------------------------------------------------------------

        EngineUpdateContext                             m_updateContext;

        // Core Engine Systems
        //-------------------------------------------------------------------------

        SystemRegistry*                                 m_pSystemRegistry = nullptr;
        TaskSystem*                                     m_pTaskSystem = nullptr;
        TypeSystem::TypeRegistry*                       m_pTypeRegistry = nullptr;
        Resource::ResourceSystem*                       m_pResourceSystem = nullptr;
        Resource::ResourceProvider*                     m_pResourceProvider = nullptr;
        Render::RenderSystem*                           m_pRenderSystem = nullptr;
        Render::Window*                                 m_pRenderWindow = nullptr;
        Render::ForwardShadingRenderer*                 m_pForwardShadingRenderer = nullptr;
        EntityWorldManager*                             m_pEntityWorldManager = nullptr;
        Input::InputSystem*                             m_pInputSystem = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        Render::ImguiRenderer*                          m_pImguiRenderer = nullptr;
        ImGuiX::ImguiSystem*                            m_pImguiSystem = nullptr;
        ImGuiX::ToolsUI*                                m_pToolsUI = nullptr;
        #endif

        // Application data
        //-------------------------------------------------------------------------

        Stage                                           m_initializationStageReached = Stage::Uninitialized;
        bool                                            m_exitRequested = false;
    };
}