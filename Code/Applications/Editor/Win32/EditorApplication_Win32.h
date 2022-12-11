#pragma once

#include "Engine.h"
#include "System/Application/Platform/Application_Win32.h"
#include "EngineTools/_Module/EngineToolsModule.h"
#include "GameTools/_Module/GameToolsModule.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EditorUI;

    //-------------------------------------------------------------------------

    class EditorEngine final : public Engine
    {
        friend class EditorApplication;

    public:

        EditorEngine( TFunction<bool( EE::String const& error )>&& errorHandler );

        virtual void RegisterTypes() override;
        virtual void UnregisterTypes() override;

        virtual bool InitializeToolsModulesAndSystems( ModuleContext& moduleContext, IniFile const& iniFile );
        virtual void ShutdownToolsModulesAndSystems( ModuleContext& moduleContext );

        virtual void CreateToolsUI();

    private:

        ResourceID                                      m_editorStartupMap;

        // Modules
        //-------------------------------------------------------------------------

        EngineToolsModule                               m_engineToolsModule;
        GameToolsModule                                 m_gameToolsModule;
    };

    //-------------------------------------------------------------------------

    class EditorApplication final : public Win32Application
    {

    public:

        EditorApplication( HINSTANCE hInstance );

    private:

        virtual bool ProcessCommandline( int32_t argc, char** argv ) override;
        virtual bool Initialize() override;
        virtual bool Shutdown() override;

        virtual void GetBorderlessTitleBarInfo( Math::ScreenSpaceRectangle& outTitlebarRect, bool& isInteractibleWidgetHovered ) const override;
        virtual void ProcessWindowResizeMessage( Int2 const& newWindowSize ) override;
        virtual void ProcessInputMessage( UINT message, WPARAM wParam, LPARAM lParam ) override;

        virtual bool ApplicationLoop() override;

    private:

        EditorEngine                                    m_engine;
    };
}