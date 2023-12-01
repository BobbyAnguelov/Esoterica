#ifdef _WIN32
#include "Application_Win32.h"
#include "Base/IniFile.h"
#include "Base/Imgui/Platform/ImguiPlatform_Win32.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/Math/Rectangle.h"
#include "Base/Logging/LoggingSystem.h"

#include <dwmapi.h>
#include <windowsx.h>
#pragma comment(lib, "dwmapi.lib")

//-------------------------------------------------------------------------

namespace EE
{
    // Code below was adapted from: https://github.com/melak47/BorderlessWindow

    // we cannot just use WS_POPUP style
    // WS_THICKFRAME: without this the window cannot be resized and so aero snap, de-maximizing and minimizing won't work
    // WS_SYSMENU: enables the context menu with the move, close, maximize, minimize... commands (shift + right-click on the task bar item)
    // WS_CAPTION: enables aero minimize animation/transition
    // WS_MAXIMIZEBOX, WS_MINIMIZEBOX: enable minimize/maximize
    enum class WindowStyle : DWORD
    {
        Windowed = WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        AeroBorderless = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
        BasicBorderless = WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX
    };

    static bool IsWindowMaximized( HWND hWnd )
    {
        WINDOWPLACEMENT placement;
        if ( !::GetWindowPlacement( hWnd, &placement ) )
        {
            return false;
        }

        return placement.showCmd == SW_MAXIMIZE;
    }

    static bool IsCompositionEnabled()
    {
        BOOL compositionEnabled = FALSE;
        if ( ::DwmIsCompositionEnabled( &compositionEnabled ) == S_OK )
        {
            return compositionEnabled;
        }

        return false;
    }

    //-------------------------------------------------------------------------

    static LRESULT CALLBACK wndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
    {
        // Store application information in window user data
        if ( message == WM_NCCREATE )
        {
            auto pUserdata = reinterpret_cast<CREATESTRUCTW*>( lParam )->lpCreateParams;
            ::SetWindowLongPtrW( hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( pUserdata ) );
        }

        //-------------------------------------------------------------------------

        // Get application information from window user data
        auto pApplication = reinterpret_cast<Win32Application*>( ::GetWindowLongPtrW( hWnd, GWLP_USERDATA ) );
        if ( pApplication != nullptr )
        {
            return pApplication->WindowMessageProcessor( hWnd, message, wParam, lParam );
        }

        return DefWindowProc( hWnd, message, wParam, lParam );
    }

    //-------------------------------------------------------------------------

    Win32Application::Win32Application( HINSTANCE hInstance, char const* applicationName, int32_t iconResourceID, TBitFlags<InitOptions> options )
        : m_applicationName( applicationName )
        , m_applicationNameNoWhitespace( StringUtils::StripAllWhitespace( String( applicationName ) ) )
        , m_applicationIconResourceID( iconResourceID )
        , m_pInstance( hInstance )
        , m_startMinimized( options.IsFlagSet( InitOptions::StartMinimized ) )
        , m_isBorderLess( options.IsFlagSet( InitOptions::Borderless ) )
    {}

    Win32Application::~Win32Application()
    {
        ::DestroyWindow( m_windowHandle );
        ::UnregisterClass( m_windowClass.lpszClassName, m_windowClass.hInstance );
    }

    bool Win32Application::FatalError( String const& error )
    {
        MessageBox( GetActiveWindow(), error.c_str(), "Fatal Error Occurred!", MB_OK | MB_ICONERROR );
        return false;
    }

    //-------------------------------------------------------------------------

    bool Win32Application::TryCreateWindow()
    {
        EE_ASSERT( ( m_windowRect.right - m_windowRect.left ) > 0 );
        EE_ASSERT( ( m_windowRect.bottom - m_windowRect.top ) > 0 );

        // Set DPI awareness
        //-------------------------------------------------------------------------

        SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );

        // Get window icon
        //-------------------------------------------------------------------------

        m_windowIcon = LoadIcon( m_pInstance, MAKEINTRESOURCE( m_applicationIconResourceID ) );

        // Create the window
        //-------------------------------------------------------------------------

        m_windowClass.cbSize = sizeof( WNDCLASSEX );
        m_windowClass.style = CS_HREDRAW | CS_VREDRAW;
        m_windowClass.hInstance = m_pInstance;
        m_windowClass.lpfnWndProc = wndProc;
        m_windowClass.cbClsExtra = 0;
        m_windowClass.cbWndExtra = 0;
        m_windowClass.hIcon = m_windowIcon;
        m_windowClass.hCursor = ::LoadCursor( nullptr, IDC_ARROW );
        m_windowClass.hbrBackground = CreateSolidBrush( RGB( 23, 23, 23 ) );
        m_windowClass.lpszMenuName = 0;
        m_windowClass.lpszClassName = m_applicationName.c_str();
        m_windowClass.hIconSm = 0;
        RegisterClassEx( &m_windowClass );

        long const windowDesiredWidth = Math::Max( m_windowRect.right - m_windowRect.left, 100l );
        long const windowDesiredHeight = Math::Max( m_windowRect.bottom - m_windowRect.top, 40l );

        // Create the window style
        DWORD windowStyle = 0;
        if ( m_isBorderLess )
        {
            windowStyle = static_cast<DWORD>( IsCompositionEnabled() ? WindowStyle::AeroBorderless : WindowStyle::BasicBorderless );
        }
        else
        {
            windowStyle = static_cast<DWORD>( WindowStyle::Windowed );
        }

        // Handle recorded window state
        if ( m_wasMaximized )
        {
            windowStyle |= WS_MAXIMIZE;
        }

        if ( m_startMinimized )
        {
            windowStyle |= WS_MINIMIZE;
        }

        // Try to create the window
        m_windowHandle = CreateWindow
        (
            m_windowClass.lpszClassName,
            m_windowClass.lpszClassName,
            windowStyle,
            m_windowRect.left,
            m_windowRect.top,
            windowDesiredWidth,
            windowDesiredHeight,
            NULL,
            NULL,
            m_pInstance,
            this
        );

        if ( m_windowHandle == nullptr )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        // Redraw frame
        ::SetWindowPos( m_windowHandle, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE );
        ::ShowWindow( m_windowHandle, SW_SHOW );

        //-------------------------------------------------------------------------

        // Disable shadow (causes white border around window in Win10)
        if ( IsCompositionEnabled() ) 
        {
            static MARGINS const shadowOffset = { 0 };
            ::DwmExtendFrameIntoClientArea( m_windowHandle, &shadowOffset );
        }

        //-------------------------------------------------------------------------

        // Update the window rect to the exact size of the created window (if we're not minimized)
        if ( !m_startMinimized )
        {
            GetClientRect( m_windowHandle, &m_windowRect );
        }

        //-------------------------------------------------------------------------

        return true;
    }

    LRESULT Win32Application::WindowMessageProcessor( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
    {
        #if EE_DEVELOPMENT_TOOLS
        if ( IsInitialized() )
        {
            auto const imguiResult = ImGuiX::Platform::WindowMessageProcessor( hWnd, message, wParam, lParam );
            if ( imguiResult != 0 )
            {
                return imguiResult;
            }
        }
        #endif

        //-------------------------------------------------------------------------

        switch ( message )
        {
            // This message has to be handled explicitly even if we dont do anything if we want the border less window to work
            case WM_NCCALCSIZE:
            {
                if ( wParam == TRUE && m_isBorderLess )
                {
                    NCCALCSIZE_PARAMS& parameters = *reinterpret_cast<NCCALCSIZE_PARAMS*>( lParam );

                    if ( IsWindowMaximized( m_windowHandle ) )
                    {
                        auto pMonitor = ::MonitorFromRect( &parameters.rgrc[0], MONITOR_DEFAULTTONEAREST );
                        if ( pMonitor )
                        {
                            MONITORINFO monitorInfo{};
                            monitorInfo.cbSize = sizeof( monitorInfo );
                            if ( ::GetMonitorInfoW( pMonitor, &monitorInfo ) )
                            {
                                // When maximized, make the client area fill just the monitor (without task bar) rect and not the whole window rect which extends beyond the monitor.
                                parameters.rgrc[0] = monitorInfo.rcWork;
                            }
                        }
                    }
                    else // Offset the hit-test borders by the system border metric to replicate Windows10+ resize behavior
                    {
                        auto& requestedClientArea = parameters.rgrc[0];

                        int32_t const borderWidthX = ::GetSystemMetrics( SM_CXFRAME ) + ::GetSystemMetrics( SM_CXPADDEDBORDER );
                        requestedClientArea.right -= borderWidthX;
                        requestedClientArea.left += borderWidthX;

                        int32_t const borderWidthY = ::GetSystemMetrics( SM_CYFRAME ) + ::GetSystemMetrics( SM_CXPADDEDBORDER );
                        requestedClientArea.bottom -= borderWidthY;
                    }

                    return 0;
                }
            }
            break;

            // Set window min size!
            case WM_GETMINMAXINFO:
            {
                MINMAXINFO* mmi = (MINMAXINFO*) lParam;
                mmi->ptMinTrackSize.x = 320;
                mmi->ptMinTrackSize.y = 240;
                return 0;
            }

            case WM_NCHITTEST:
            {
                // When we have no border or title bar, we need to perform our
                // own hit testing to allow resizing and moving.
                if ( m_isBorderLess )
                {
                    return BorderlessWindowHitTest( POINT{ GET_X_LPARAM( lParam ), GET_Y_LPARAM( lParam ) } );
                }
            }
            break;

            case WM_NCACTIVATE:
            {
                if ( !IsCompositionEnabled() )
                {
                    // Prevents window frame reappearing on window activation
                    // in "basic" theme, where no aero shadow is present.
                    return 1;
                }
            }
            break;

            //-------------------------------------------------------------------------

            case WM_SIZE:
            {
                if ( IsInitialized() )
                {
                    Int2 const newWindowSize( LOWORD( lParam ), HIWORD( lParam ) );
                    if ( newWindowSize.m_x > 0 && newWindowSize.m_y > 0 )
                    {
                        ProcessWindowResizeMessage( newWindowSize );
                    }
                }
            }
            break;

            //-------------------------------------------------------------------------

            case WM_SETFOCUS:
            case WM_KILLFOCUS:
            case WM_INPUT:
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_CHAR:
            case WM_MOUSEMOVE:
            {
                if ( IsInitialized() )
                {
                    ProcessInputMessage( message, wParam, lParam );
                }
            }
            break;

            //-------------------------------------------------------------------------

            case WM_CLOSE:
            case WM_QUIT:
            {
                if ( OnUserExitRequest() )
                {
                    RequestApplicationExit();
                }
                else
                {
                    return 0;
                }
            }
            break;

            //-------------------------------------------------------------------------

            case WM_DESTROY:
            {
                ProcessWindowDestructionMessage();
                PostQuitMessage( 0 );
                RequestApplicationExit();
            }
            break;
        }

        //-------------------------------------------------------------------------

        return DefWindowProc( hWnd, message, wParam, lParam );
    }

    void Win32Application::ProcessWindowDestructionMessage()
    {
        WriteLayoutSettings();
    }

    //-------------------------------------------------------------------------

    void Win32Application::ReadLayoutSettings()
    {
        FileSystem::Path const layoutIniFilePath = FileSystem::GetCurrentProcessPath() + m_applicationNameNoWhitespace + ".layout.ini";
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

            FileSystem::Path const layoutIniFilePath = FileSystem::GetCurrentProcessPath() + m_applicationNameNoWhitespace + ".layout.ini";
            layoutIni.SaveToFile( layoutIniFilePath );
        }
    }

    //-------------------------------------------------------------------------

    LRESULT Win32Application::BorderlessWindowHitTest( POINT cursor )
    {
        EE_ASSERT( m_isBorderLess );

        // Get border sizes to allow for detecting of resize zones
        POINT const borderSize
        {
            ::GetSystemMetrics( SM_CXFRAME ) + ::GetSystemMetrics( SM_CXPADDEDBORDER ),
            ::GetSystemMetrics( SM_CYFRAME ) + ::GetSystemMetrics( SM_CXPADDEDBORDER )
        };

        // Get the window dimensions
        RECT window;
        if ( !::GetWindowRect( m_windowHandle, &window ) ) 
        {
            return HTNOWHERE;
        }

        //-------------------------------------------------------------------------

        enum region_mask
        {
            client = 0b0000,
            left = 0b0001,
            right = 0b0010,
            top = 0b0100,
            bottom = 0b1000,
        };

        const auto result =
            left * ( cursor.x < ( window.left + borderSize.x ) ) |
            right * ( cursor.x >= ( window.right - borderSize.x ) ) |
            top * ( cursor.y < ( window.top + borderSize.y ) ) |
            bottom * ( cursor.y >= ( window.bottom - borderSize.y ) );

        switch ( result )
        {
            case left: return HTLEFT;
            case right: return HTRIGHT;
            case top: return HTTOP;
            case bottom: return HTBOTTOM;
            case top | left: return HTTOPLEFT;
            case top | right: return HTTOPRIGHT;
            case bottom | left: return HTBOTTOMLEFT;
            case bottom | right: return HTBOTTOMRIGHT;

            case client:
            {
                Math::ScreenSpaceRectangle titleBarRect;
                bool isAnyInteractibleWidgetHovered = false;
                GetBorderlessTitleBarInfo( titleBarRect, isAnyInteractibleWidgetHovered );

                //-------------------------------------------------------------------------

                if ( !isAnyInteractibleWidgetHovered )
                {
                    int32_t const titleBarTop = window.top + (int32_t) titleBarRect.GetTopLeft().m_y;
                    int32_t const titleBarLeft = window.left + (int32_t) titleBarRect.GetTopLeft().m_x;
                    int32_t const titleBarBottom = titleBarTop + (int32_t) titleBarRect.GetSize().m_y;
                    int32_t const titleBarRight = titleBarLeft + (int32_t) titleBarRect.GetSize().m_x;

                    bool const isCursorWithinTitleBarX = cursor.x > titleBarLeft + borderSize.x && cursor.x < titleBarRight + borderSize.x;
                    bool const isCursorWithinTitleBarY = cursor.y > titleBarTop && cursor.y < titleBarBottom;
                    if ( isCursorWithinTitleBarX && isCursorWithinTitleBarY )
                    {
                        return HTCAPTION;
                    }
                }

                return HTCLIENT;
            }

            default: return HTNOWHERE;
        }
    }

    //-------------------------------------------------------------------------

    int32_t Win32Application::Run( int32_t argc, char** argv )
    {
        FileSystem::Path const logFilePath = FileSystem::GetCurrentProcessPath() + m_applicationNameNoWhitespace + "Log.txt";
        Log::System::SetLogFilePath( logFilePath );

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

        int32_t exitCode = 0;

        bool shouldExit = false;
        while ( !shouldExit )
        {
            MSG message;
            if ( PeekMessage( &message, NULL, 0, 0, PM_REMOVE ) == TRUE )
            {
                exitCode = (int32_t) message.wParam;
                TranslateMessage( &message );
                DispatchMessage( &message );
            }
            else // Run the application update
            {
                GetWindowRect( m_windowHandle, &m_windowRect );

                if ( m_applicationRequestedExit )
                {
                    shouldExit = true;
                }
                else
                {
                    shouldExit = !ApplicationLoop();
                }
            }
        }

        // Shutdown
        //-------------------------------------------------------------------------

        bool const shutdownResult = Shutdown();
        m_initialized = false;

        Log::System::SaveToFile();

        //-------------------------------------------------------------------------

        if ( !shutdownResult )
        {
            return FatalError( "Application failed to shutdown correctly!" );
        }

        return exitCode;
    }
}
#endif