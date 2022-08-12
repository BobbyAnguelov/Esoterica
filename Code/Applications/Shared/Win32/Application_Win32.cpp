#ifdef _WIN32
#include "Application_Win32.h"
#include "System/IniFile.h"
#include "System/Platform/PlatformHelpers_Win32.h"
#include "System/FileSystem/FileSystemPath.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace
    {
        static Win32Application* g_pApplicationInstance = nullptr;

        static LRESULT CALLBACK wndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
        {
            if ( g_pApplicationInstance != nullptr && g_pApplicationInstance->IsInitialized() )
            {
                LRESULT const userResult = g_pApplicationInstance->WndProcess( hWnd, message, wParam, lParam );
                if ( userResult != 0 )
                {
                    return userResult;
                }

                //-------------------------------------------------------------------------

                switch ( message )
                {
                    case WM_CLOSE:
                    case WM_QUIT:
                    {
                        if ( g_pApplicationInstance->OnExitRequest() )
                        {
                            g_pApplicationInstance->Exit();
                        }
                    }
                    break;

                    //-------------------------------------------------------------------------

                    case WM_DESTROY:
                    {
                        g_pApplicationInstance->OnWindowDestruction();
                        PostQuitMessage( 0 );
                    }
                    break;
                }
            }

            //-------------------------------------------------------------------------

            return DefWindowProc( hWnd, message, wParam, lParam );
        }
    }

    //-------------------------------------------------------------------------

    Win32Application::Win32Application( HINSTANCE hInstance, char const* applicationName, int32_t iconResourceID, bool startMinimized )
        : m_pInstance( hInstance )
        , m_applicationName( applicationName )
        , m_applicationNameNoWhitespace( StringUtils::StripWhitespace( applicationName ) )
        , m_applicationIconResourceID( iconResourceID )
        , m_startMinimized( startMinimized )
    {
        Memory::MemsetZero( &m_message, sizeof( m_message ) );

        //-------------------------------------------------------------------------

        EE_ASSERT( g_pApplicationInstance == nullptr );
        g_pApplicationInstance = this;
    }

    Win32Application::~Win32Application()
    {
        g_pApplicationInstance = nullptr;

        //-------------------------------------------------------------------------

        ::DestroyWindow( m_windowHandle );
        ::UnregisterClass( m_windowClass.lpszClassName, m_windowClass.hInstance );
    }

    bool Win32Application::FatalError( String const& error )
    {
        MessageBox( GetActiveWindow(), error.c_str(), "Fatal Error Occurred!", MB_OK | MB_ICONERROR );
        return false;
    }

    void Win32Application::OnWindowDestruction()
    {
        WriteLayoutSettings();
    }

    //-------------------------------------------------------------------------

    void Win32Application::ReadLayoutSettings()
    {
        FileSystem::Path const layoutIniFilePath = FileSystem::Path( m_applicationNameNoWhitespace + ".layout.ini" );
        IniFile layoutIni( layoutIniFilePath );
        if ( !layoutIni.IsValid() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        int32_t v = 0;
        if ( layoutIni.TryGetInt( "WindowSettings:Left", v ) )
        {
            m_windowRect.left = v;
        }

        if ( layoutIni.TryGetInt( "WindowSettings:Right", v ) )
        {
            m_windowRect.right = v;
        }

        if ( layoutIni.TryGetInt( "WindowSettings:Top", v ) )
        {
            m_windowRect.top = v;
        }

        if ( layoutIni.TryGetInt( "WindowSettings:Bottom", v ) )
        {
            m_windowRect.bottom = v;
        }

        EE_ASSERT( ( m_windowRect.right - m_windowRect.left ) > 0 );
        EE_ASSERT( ( m_windowRect.bottom - m_windowRect.top ) > 0 );

        layoutIni.TryGetBool( "WindowSettings:WasMaximized", m_wasMaximized );

        //-------------------------------------------------------------------------

        uint32_t flags = 0;

        layoutIni.TryGetUInt( "Layout:UserFlags0", flags );
        m_userFlags = flags;

        layoutIni.TryGetUInt( "Layout:UserFlags1", flags );
        m_userFlags |= uint64_t( flags ) << 32;
    }

    void Win32Application::WriteLayoutSettings()
    {
        IniFile layoutIni;
        if ( layoutIni.IsValid() )
        {
            WINDOWPLACEMENT wndPlacement;
            wndPlacement.length = sizeof( WINDOWPLACEMENT );
            bool const result = GetWindowPlacement( m_windowHandle, &wndPlacement );

            // We should always have a valid window handle when calling this function
            EE_ASSERT( result );

            // Save window rect
            layoutIni.CreateSection( "WindowSettings" );
            layoutIni.SetInt( "WindowSettings:Left", (int32_t) wndPlacement.rcNormalPosition.left );
            layoutIni.SetInt( "WindowSettings:Right", (int32_t) wndPlacement.rcNormalPosition.right );
            layoutIni.SetInt( "WindowSettings:Top", (int32_t) wndPlacement.rcNormalPosition.top );
            layoutIni.SetInt( "WindowSettings:Bottom", (int32_t) wndPlacement.rcNormalPosition.bottom );
            layoutIni.SetBool( "WindowSettings:WasMaximized", wndPlacement.showCmd == SW_MAXIMIZE );

            // Save user flags
            layoutIni.CreateSection( "Layout" );
            layoutIni.SetInt( "Layout:UserFlags0", (uint32_t) m_userFlags );
            layoutIni.SetInt( "Layout:UserFlags1", (uint32_t) ( m_userFlags >> 32 ) );

            FileSystem::Path const layoutIniFilePath = FileSystem::Path( m_applicationNameNoWhitespace + ".layout.ini" );
            layoutIni.SaveToFile( layoutIniFilePath );
        }
    }

    //-------------------------------------------------------------------------

    bool Win32Application::TryCreateWindow()
    {
        EE_ASSERT( ( m_windowRect.right - m_windowRect.left ) > 0 );
        EE_ASSERT( ( m_windowRect.bottom - m_windowRect.top ) > 0 );

        m_windowIcon = LoadIcon( m_pInstance, MAKEINTRESOURCE( m_applicationIconResourceID ) );

        // Create the window
        //-------------------------------------------------------------------------

        m_windowClass.cbSize = sizeof( WNDCLASSEX );
        m_windowClass.style = CS_HREDRAW | CS_VREDRAW;
        m_windowClass.lpfnWndProc = wndProc;
        m_windowClass.cbClsExtra = 0;
        m_windowClass.cbWndExtra = 0;
        m_windowClass.hInstance = m_pInstance;
        m_windowClass.hIcon = m_windowIcon;
        m_windowClass.hCursor = nullptr;
        m_windowClass.hbrBackground = (HBRUSH) ( COLOR_WINDOW + 1 );
        m_windowClass.lpszMenuName = 0;
        m_windowClass.lpszClassName = m_applicationName.c_str();
        m_windowClass.hIconSm = 0;
        RegisterClassEx( &m_windowClass );

        long const windowDesiredWidth = Math::Max( m_windowRect.right - m_windowRect.left, 100l );
        long const windowDesiredHeight = Math::Max( m_windowRect.bottom - m_windowRect.top, 40l );

        // Create the window style (taking into account if the application was maximized
        DWORD windowStyle = WS_OVERLAPPEDWINDOW;
        if ( m_wasMaximized )
        {
            windowStyle |= WS_MAXIMIZE;
        }

        if ( m_startMinimized )
        {
            windowStyle |= WS_MINIMIZE;
        }

        // Try to create the window
        m_windowHandle = CreateWindow( m_windowClass.lpszClassName,
                                  m_windowClass.lpszClassName,
                                  windowStyle,
                                  m_windowRect.left,
                                  m_windowRect.top,
                                  windowDesiredWidth,
                                  windowDesiredHeight,
                                  NULL,
                                  NULL,
                                  m_pInstance,
                                  NULL );

        if ( m_windowHandle == nullptr )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        ShowWindow( m_windowHandle, SW_SHOW );
        UpdateWindow( m_windowHandle );

        if ( !m_startMinimized )
        {
            GetClientRect( m_windowHandle, &m_windowRect );
        }

        return true;
    }

    //-------------------------------------------------------------------------

    int Win32Application::Run( int32_t argc, char** argv )
    {
        // Read Settings
        //-------------------------------------------------------------------------

        if ( !ProcessCommandline( argc, argv ) )
        {
            return FatalError( "Application failed to read settings correctly!" );
        }

        ReadLayoutSettings();

        // Window
        //-------------------------------------------------------------------------

        if ( !TryCreateWindow() )
        {
            return FatalError( "Application failed to create window!" );
        }

        // Initialization
        //-------------------------------------------------------------------------

        if ( !Initialize() )
        {
            Shutdown();
            return FatalError( "Application failed to initialize correctly!" );
        }
        else
        {
            m_initialized = true;
        }

        // Application loop
        //-------------------------------------------------------------------------

        while ( !m_shouldExit )
        {
            if ( PeekMessage( &m_message, NULL, 0, 0, PM_REMOVE ) == TRUE )
            {
                TranslateMessage( &m_message );
                DispatchMessage( &m_message );
            }
            else // Run the application update
            {
                m_shouldExit = !ApplicationLoop();
            }

            GetWindowRect( m_windowHandle, &m_windowRect );
        }

        // Shutdown
        //-------------------------------------------------------------------------

        bool const shutdownResult = Shutdown();
        m_initialized = false;

        FileSystem::Path const LogFilePath( m_applicationNameNoWhitespace + "Log.txt" );
        Log::SaveToFile( LogFilePath );

        //-------------------------------------------------------------------------

        if ( !shutdownResult )
        {
            return FatalError( "Application failed to shutdown correctly!" );
        }

        return (int) m_message.wParam;
    }
}
#endif