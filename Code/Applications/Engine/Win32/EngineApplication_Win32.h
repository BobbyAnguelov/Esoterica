#ifdef _WIN32
#pragma once

#include "Game/_Module/GameModule.h"
#include "Game/ToolsUI/GameDebugUI.h"
#include "Engine/Engine.h"
#include "Base/Application/Platform/Application_Win32.h"

//-------------------------------------------------------------------------

namespace EE
{
    class StandaloneEngine final : public Engine
    {
        friend class ResourceEditorApplication;

    public:

        StandaloneEngine( TFunction<bool( EE::String const& error )>&& errorHandler );

        void RegisterTypes() override;
        void UnregisterTypes() override;

        virtual void PostInitialize() override;
        virtual void PreShutdown() override;

        virtual void ResizeMainWindow( Int2 newMainWindowDimensions ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void CreateToolsUI() override { m_pToolsUI = EE::New<GameDebugUI>(); }
        #endif

    private:

        Viewport* m_pGameViewport = nullptr;
    };

    //-------------------------------------------------------------------------

    class EngineApplication : public Win32Application
    {

    public:

        EngineApplication( HINSTANCE hInstance );

    private:

        virtual bool Initialize( int32_t argc, char** argv ) override;
        virtual bool Shutdown() override;

        virtual void ResizeMainWindow( Int2 const& newWindowSize ) override;
        virtual void ProcessInputMessage( UINT message, WPARAM wParam, LPARAM lParam ) override;

        virtual bool ApplicationLoop() override;

        #if EE_ENABLE_LPP
        virtual void LivePP_PreReload() { m_engine.LivePP_PreReload(); }
        virtual void LivePP_PostReload() { m_engine.LivePP_PostReload(); }
        #endif

    private:

        StandaloneEngine            m_engine;
    };
}

#endif