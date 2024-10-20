#ifdef _WIN32
#pragma once

#include "Game/_Module/GameModule.h"
#include "Engine/Engine.h"
#include "Engine/ToolsUI/EngineDebugUI.h"
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

        #if EE_DEVELOPMENT_TOOLS
        virtual void CreateDevelopmentToolsUI() override { m_pDevelopmentToolsUI = EE::New<EngineDebugUI>(); }
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