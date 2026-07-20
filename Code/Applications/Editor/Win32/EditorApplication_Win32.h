#pragma once

#include "../EditorUI.h"
#include "Game/_Module/GameModule.h"
#include "GameTools/_Module/GameToolsModule.h"
#include "EngineTools/_Module/EngineToolsModule.h"
#include "Engine/Engine.h"
#include "Base/Application/Platform/Application_Win32.h"

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
        virtual void CreateToolsUI() override { m_pToolsUI = EE::New<EditorUI>(); }
        virtual void SetStartupMap( ResourceID const& mapID ) override;
    };

    //-------------------------------------------------------------------------

    class EditorApplication final : public Win32Application
    {

    public:

        EditorApplication( HINSTANCE hInstance );

    private:

        virtual bool Initialize( int32_t argc, char** argv ) override;
        virtual bool Shutdown() override;

        virtual bool FatalError( String const& error ) const override;

        virtual void GetBorderlessTitleBarInfo( Math::ScreenSpaceRectangle& outTitlebarRect, bool& isInteractibleWidgetHovered ) const override;
        virtual void ResizeMainWindow( Int2 const& newWindowSize ) override;
        virtual void ProcessInputMessage( UINT message, WPARAM wParam, LPARAM lParam ) override;

        virtual bool ApplicationLoop() override;

        #if EE_ENABLE_LPP
        virtual void LivePP_PreReload() override { m_engine.LivePP_PreReload(); }
        virtual void LivePP_PostReload() override { m_engine.LivePP_PostReload(); }
        #endif

    private:

        EditorEngine                                    m_engine;
    };
}