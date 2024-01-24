#pragma once

#include "RenderingSystem.h"
#include "Game/_Module/GameModule.h"
#include "Engine/_Module/EngineModule.h"
#include "Engine/ToolsUI/IDevelopmentToolsUI.h"
#include "Engine/UpdateContext.h"
#include "Base/_Module/BaseModule.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Engine
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
            InitializeBase,
            InitializeModules,
            LoadModuleResources,
            InitializeEngine,
            FullyInitialized
        };

    public:

        Engine( TFunction<bool( EE::String const& error )>&& errorHandler );
        virtual ~Engine() = default;

        bool Initialize( Int2 const& windowDimensions );
        bool Shutdown();
        bool Update();

        // Needed for window processor access
        Render::RenderingSystem* GetRenderingSystem() { return &m_renderingSystem; }
        Input::InputSystem* GetInputSystem() { return m_pInputSystem; }

    protected:

        virtual void RegisterTypes() = 0;
        virtual void UnregisterTypes() = 0;

        #if EE_DEVELOPMENT_TOOLS
        virtual bool InitializeToolsModulesAndSystems( ModuleContext& moduleContext ) { return true; }
        virtual void ShutdownToolsModulesAndSystems( ModuleContext& moduleContext ) {}
        virtual void CreateToolsUI() = 0;
        void DestroyToolsUI() { EE::Delete( m_pToolsUI ); }
        #endif

    protected:

        TFunction<bool( EE::String const& error )>     m_fatalErrorHandler;

        // Modules
        //-------------------------------------------------------------------------

        BaseModule                                      m_baseModule;
        EngineModule                                    m_engineModule;
        GameModule                                      m_gameModule;

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
        ImGuiX::IDevelopmentToolsUI*                    m_pToolsUI = nullptr;
        #endif

        // Application data
        //-------------------------------------------------------------------------

        ResourcePath                                    m_startupMap;
        Stage                                           m_initializationStageReached = Stage::Uninitialized;
        bool                                            m_exitRequested = false;
    };
}