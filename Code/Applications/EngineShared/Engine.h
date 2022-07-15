#pragma once

#include "RenderingSystem.h"
#include "Engine/ToolsUI/IToolsUI.h"
#include "System/Types/Function.h"
#include "Engine/UpdateContext.h"

#include "Engine/_Module/EngineModule.h"
#include "Game/_Module/GameModule.h"

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

    public:

        Engine( TFunction<bool( EE::String const& error )>&& errorHandler );
        virtual ~Engine() = default;

        bool Initialize( String const& applicationName, Int2 const& windowDimensions );
        bool Shutdown();
        bool Update();

        // Needed for window processor access
        Render::RenderingSystem* GetRenderingSystem() { return &m_renderingSystem; }
        Input::InputSystem* GetInputSystem() { return m_pInputSystem; }

    protected:

        virtual void RegisterTypes();
        virtual void UnregisterTypes();

        #if EE_DEVELOPMENT_TOOLS
        virtual void CreateToolsUI() = 0;
        void DestroyToolsUI() { EE::Delete( m_pToolsUI ); }
        #endif

    protected:

        TFunction<bool( EE::String const& error )>     m_fatalErrorHandler;

        // Modules
        //-------------------------------------------------------------------------

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
        Physics::PhysicsSystem*                         m_pPhysicsSystem = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        ImGuiX::ImguiSystem*                            m_pImguiSystem = nullptr;
        ImGuiX::IToolsUI*                               m_pToolsUI = nullptr;
        #endif

        // Application data
        //-------------------------------------------------------------------------

        ResourcePath                                    m_startupMap;
        bool                                            m_moduleInitStageReached = false;
        bool                                            m_moduleResourcesInitStageReached = false;
        bool                                            m_finalInitStageReached = false;
        bool                                            m_initialized = false;

        bool                                            m_exitRequested = false;
    };
}