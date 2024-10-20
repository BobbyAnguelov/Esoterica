#pragma once

#include "Game/_Module/GameModule.h"
#include "GameTools/_Module/GameToolsModule.h"
#include "EngineTools/_Module/EngineToolsModule.h"
#include "Engine/Engine.h"
#include "Base/Application/Platform/Application_Win32.h"
#include "../EditorUI.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EditorEngine final : public Engine
    {
        friend class EditorApplication;

    public:

        EditorEngine( TFunction<bool( EE::String const& error )>&& errorHandler );

        virtual void RegisterTypes() override;
        virtual void UnregisterTypes() override;
        virtual void CreateDevelopmentToolsUI() override { m_pDevelopmentToolsUI = EE::New<EditorUI>(); }
        virtual void PostInitialize() override;

    private:

        ResourceID                                      m_editorStartupMap;
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