#pragma once

#include "_Module/API.h"
#include "Engine/Render/RenderingSystem.h"
#include "Engine/_Module/EngineModule.h"
#include "Engine/ToolsUI/IDevelopmentToolsUI.h"
#include "Engine/UpdateContext.h"
#include "Base/_Module/BaseModule.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINE_API Engine
    {
        friend class EngineApplication;

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

        bool Initialize( Int2 const& windowDimensions );
        bool Shutdown();
        bool Update();

        // Needed for window processor access
        Render::RenderingSystem* GetRenderingSystem() { return &m_renderingSystem; }
        Input::InputSystem* GetInputSystem() { return m_pInputSystem; }

    protected:

        virtual void RegisterTypes() = 0;
        virtual void UnregisterTypes() = 0;

        // Called once the core engine has been successfully initialized, allows derived engine classes to create tools UI and initialize various other systems
        virtual void PostInitialize() {};

        // Called just before we shutdown the engine
        virtual void PreShutdown() {};

        #if EE_DEVELOPMENT_TOOLS
        virtual void CreateDevelopmentToolsUI() = 0;
        #endif

        inline BaseModule* GetBaseModule() { return static_cast<BaseModule*>( m_modules[0] ); }
        inline EngineModule* GetEngineModule() { return static_cast<EngineModule*>( m_modules[1] ); }

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
        Render::RenderDevice*                           m_pRenderDevice = nullptr;
        Render::RenderingSystem                         m_renderingSystem;
        EntityWorldManager*                             m_pEntityWorldManager = nullptr;
        Input::InputSystem*                             m_pInputSystem = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        ImGuiX::ImguiSystem*                            m_pImguiSystem = nullptr;
        ImGuiX::IDevelopmentToolsUI*                    m_pDevelopmentToolsUI = nullptr;
        Console*                                        m_pConsole = nullptr;
        #endif

        // Application data
        //-------------------------------------------------------------------------

        DataPath                                    m_startupMap;
        Stage                                           m_initializationStageReached = Stage::Uninitialized;
        bool                                            m_exitRequested = false;
    };
}