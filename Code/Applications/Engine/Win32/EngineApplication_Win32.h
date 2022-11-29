#ifdef _WIN32
#pragma once

#include "System/Application/Platform/Application_Win32.h"
#include "Applications/EngineShared/Engine.h"
#include "Engine/ToolsUI/EngineToolsUI.h"

//-------------------------------------------------------------------------

namespace EE
{
    class StandaloneEngine final : public Engine
    {
        friend class ResourceEditorApplication;

    public:

        using Engine::Engine;

        #if EE_DEVELOPMENT_TOOLS
        virtual void CreateToolsUI() { m_pToolsUI = EE::New<EngineToolsUI>(); }
        #endif
    };

    //-------------------------------------------------------------------------

    class EngineApplication : public Win32Application
    {

    public:

        EngineApplication( HINSTANCE hInstance );

    private:

        virtual bool ProcessCommandline( int32_t argc, char** argv ) override;
        virtual bool Initialize();
        virtual bool Shutdown();

        virtual void ProcessWindowResizeMessage( Int2 const& newWindowSize ) override;
        virtual void ProcessInputMessage( UINT message, WPARAM wParam, LPARAM lParam ) override;

        virtual bool ApplicationLoop() override;

    private:

        StandaloneEngine            m_engine;
    };
}

#endif