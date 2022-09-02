#ifdef _WIN32
#include "EngineApplication_win32.h"
#include "Engine_Win32.h"
#include "Resource.h"
#include "Applications/Shared/cmdParser/cmdParser.h"
#include "Applications/Shared/LivePP/LivePP.h"
#include "System/Imgui/ImguiStyle.h"
#include <tchar.h>
#include <windows.h>

//-------------------------------------------------------------------------

namespace EE
{
    EngineApplication::EngineApplication( HINSTANCE hInstance )
        : Win32Application( hInstance, "Esoterica Engine", IDI_ENGINE_ICON )
        , m_engine( TFunction<bool( EE::String const& error )>( [this] ( String const& error )-> bool  { return FatalError( error ); } ) )
    {}

    bool EngineApplication::ProcessCommandline( int32_t argc, char** argv )
    {
        cli::Parser cmdParser( argc, argv );
        cmdParser.set_optional<std::string>( "map", "map", "", "The startup map." );

        if ( !cmdParser.run() )
        {
            return FatalError( "Invalid command line arguments!" );
        }

        std::string const map = cmdParser.get<std::string>( "map" );
        if ( !map.empty() )
        {
            m_engine.m_startupMap = ResourcePath( map.c_str() );
        }

        return true;
    }

    bool EngineApplication::Initialize()
    {
        Int2 const windowDimensions( ( m_windowRect.right - m_windowRect.left ), ( m_windowRect.bottom - m_windowRect.top ) );
        if ( !m_engine.Initialize( windowDimensions ) )
        {
            return FatalError( "Failed to initialize engine" );
        }

        return true;
    }

    bool EngineApplication::Shutdown()
    {
        return m_engine.Shutdown();
    }

    bool EngineApplication::ApplicationLoop()
    {
        // Uncomment for live editing of ImguiTheme
        //ImGuiX::Style::Apply();
        return m_engine.Update();
    }

    //-------------------------------------------------------------------------

    LRESULT EngineApplication::WndProcess( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
    {
        return DefaultEngineWindowProcessor( &m_engine, hWnd, message, wParam, lParam );
    }
}

//-------------------------------------------------------------------------

int APIENTRY _tWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
    int result = 0;
    {
        #if EE_ENABLE_LPP
        auto lppAgent = EE::ScopedLPPAgent();
        #endif

        EE::ApplicationGlobalState globalState;
        EE::EngineApplication engineApplication( hInstance );
        result = engineApplication.Run( __argc, __argv );
    }

    return result;
}

#endif