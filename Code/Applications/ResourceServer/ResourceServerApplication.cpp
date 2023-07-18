#include "ResourceServerApplication.h"
#include "Resources/Resource.h"
#include "Applications/Shared/LivePP/LivePP.h"

#include "Base/Time/Timers.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/Imgui/Platform/ImguiPlatform_win32.h"
#include "Base/Platform/PlatformUtils_Win32.h"
#include "Base/IniFile.h"
#include <tchar.h>
#include <shobjidl_core.h>

//-------------------------------------------------------------------------

namespace EE
{
    constexpr static int32_t const g_shellIconCallbackMessageID = WM_USER + 1;

    //-------------------------------------------------------------------------

    ResourceServerApplication::ResourceServerApplication( HINSTANCE pInstance )
        : Win32Application( pInstance, "Esoterica Resource Server", IDI_RESOURCESERVER, TBitFlags<InitOptions>( Win32Application::InitOptions::StartMinimized, Win32Application::InitOptions::Borderless ) )
        , m_resourceServerUI( m_resourceServer, m_imguiSystem.GetImageCache() )
    {}

    LRESULT ResourceServerApplication::WindowMessageProcessor( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
    {
        if ( message == RegisterWindowMessage( "TaskbarCreated" ) )
        {
            if ( !Shell_NotifyIcon( NIM_ADD, &m_systemTrayIconData ) )
            {
                FatalError( "Failed to recreate system tray icon after explorer crash!" );
                RequestApplicationExit();
                return -1;
            }
        }

        //-------------------------------------------------------------------------

        switch ( message )
        {
            case g_shellIconCallbackMessageID:
            {
                switch ( LOWORD( lParam ) )
                {
                    case WM_LBUTTONDBLCLK:
                    {
                        ShowApplicationWindow();
                    }
                    break;

                    case WM_RBUTTONDOWN:
                    {
                        if ( !ShowSystemTrayMenu() )
                        {
                            return 0;
                        }
                    }
                    break;
                }
            }
            break;

            //-------------------------------------------------------------------------

            case WM_COMMAND:
            {
                switch ( LOWORD( wParam ) )
                {
                    case ID_REQUEST_EXIT:
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
                }
            }
            break;

            //-------------------------------------------------------------------------

            case WM_CLOSE:
            {
                HideApplicationWindow();
                return 0;
            }
            break;
        }

        return Win32Application::WindowMessageProcessor( hWnd, message, wParam, lParam );
    }

    void ResourceServerApplication::ProcessWindowDestructionMessage()
    {
        if ( m_pTaskbarInterface != nullptr )
        {
            m_pTaskbarInterface->SetOverlayIcon( m_windowHandle, nullptr, L"" );
            m_pTaskbarInterface->Release();
            m_pTaskbarInterface = nullptr;
        }
        CoUninitialize();

        m_busyOverlaySet = false;
        DestroyIcon( m_busyOverlayIcon );

        //-------------------------------------------------------------------------

        Win32Application::ProcessWindowDestructionMessage();
    }

    void ResourceServerApplication::ProcessWindowResizeMessage( Int2 const& newWindowSize )
    {
        m_pRenderDevice->ResizePrimaryWindowRenderTarget( newWindowSize );
        m_viewport.Resize( Int2::Zero, newWindowSize );
    }

    bool ResourceServerApplication::OnUserExitRequest()
    {
        // Allow exiting resource server immediately if not connected clients
        if ( m_resourceServer.GetNumConnectedClients() == 0 )
        {
            return true;
        }

        //-------------------------------------------------------------------------

        // Confirm exit if there are still connected clients
        int32_t const closeDialogResult = MessageBox( m_windowHandle, "There are still connected clients!\r\nAre you sure you want to exit the Resource Server?", "Resource Server", MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2 | MB_TOPMOST );
        if ( closeDialogResult == IDYES )
        {
            return true;
        }

        return false;
    }

    //-------------------------------------------------------------------------

    void ResourceServerApplication::ShowApplicationWindow()
    {
        ShowWindow( m_windowHandle, SW_RESTORE );
        SetForegroundWindow( m_windowHandle );
        m_applicationWindowHidden = false;
    }

    void ResourceServerApplication::HideApplicationWindow()
    {
        ShowWindow( m_windowHandle, SW_HIDE );
        m_applicationWindowHidden = true;
    }

    bool ResourceServerApplication::CreateSystemTrayIcon( int32_t iconID )
    {
        m_systemTrayIconData.cbSize = sizeof( NOTIFYICONDATA );
        m_systemTrayIconData.hWnd = m_windowHandle;
        m_systemTrayIconData.uID = IDI_TRAY_IDLE;
        m_systemTrayIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        m_systemTrayIconData.hIcon = LoadIcon( m_pInstance, (LPCTSTR) MAKEINTRESOURCE( iconID ) );
        m_systemTrayIconData.uCallbackMessage = g_shellIconCallbackMessageID;
        strcpy( m_systemTrayIconData.szTip, "Esoterica Resource Server" );

        if ( !Shell_NotifyIcon( NIM_ADD, &m_systemTrayIconData ) )
        {
            return false;
        }

        m_currentIconID = iconID;
        return true;
    }

    bool ResourceServerApplication::ShowSystemTrayMenu()
    {
        HMENU hMenu, hSubMenu;

        // Get mouse cursor position x and y as lParam has the Message itself
        POINT lpClickPoint;
        GetCursorPos( &lpClickPoint );

        // Load menu resource
        hMenu = LoadMenu( m_pInstance, MAKEINTRESOURCE( IDR_SYSTRAY_MENU ) );
        if ( !hMenu )
        {
            return false;
        }

        hSubMenu = GetSubMenu( hMenu, 0 );
        if ( !hSubMenu )
        {
            DestroyMenu( hMenu );
            return false;
        }

        // Display menu
        SetForegroundWindow( m_windowHandle );
        TrackPopupMenu( hSubMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, lpClickPoint.x, lpClickPoint.y, 0, m_windowHandle, NULL );
        SendMessage( m_windowHandle, WM_NULL, 0, 0 );

        // Kill off objects we're done with
        DestroyMenu( hMenu );

        return true;
    }

    void ResourceServerApplication::DestroySystemTrayIcon()
    {
        Shell_NotifyIcon( NIM_DELETE, &m_systemTrayIconData );
    }

    void ResourceServerApplication::RefreshSystemTrayIcon( int32_t iconID )
    {
        if ( iconID != m_currentIconID )
        {
            DestroySystemTrayIcon();
            CreateSystemTrayIcon( iconID );
        }
    }

    //-------------------------------------------------------------------------

    bool ResourceServerApplication::Initialize()
    {
        m_busyOverlayIcon = LoadIcon( m_pInstance, MAKEINTRESOURCE( IDI_RESOURCESERVER_BUSYOVERLAY ) );

        CoInitialize( nullptr );
        auto const result = CoCreateInstance( CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( &m_pTaskbarInterface ) );
        if ( result == S_OK )
        {
            m_pTaskbarInterface->HrInit();
        }
        else
        {
            return false;
        }

        //-------------------------------------------------------------------------

        m_pRenderDevice = EE::New<Render::RenderDevice>();
        if ( !m_pRenderDevice->Initialize() )
        {
            EE::Delete( m_pRenderDevice );
            return FatalError( "Failed to create render device!" );
        }

        Int2 const windowDimensions( ( m_windowRect.right - m_windowRect.left ), ( m_windowRect.bottom - m_windowRect.top ) );
        m_pRenderDevice->ResizePrimaryWindowRenderTarget( windowDimensions );
        m_viewport = Render::Viewport( Int2( 0, 0 ), windowDimensions, Math::ViewVolume( Float2( windowDimensions ), FloatRange( 0.1f, 100.0f ) ) );

        //-------------------------------------------------------------------------

        FileSystem::Path const iniFilePath = FileSystem::GetCurrentProcessPath().Append( "Esoterica.ini" );
        IniFile iniFile( iniFilePath );
        if ( !iniFile.IsValid() )
        {
            InlineString const errorMessage( InlineString::CtorSprintf(), "Failed to load settings from INI file: %s", iniFilePath.c_str() );
            return FatalError( errorMessage.c_str() );
        }

        if ( !m_resourceServer.Initialize( iniFile ) )
        {
            return FatalError( "Resource server failed to initialize!" );
        }

        //-------------------------------------------------------------------------

        m_imguiSystem.Initialize( m_pRenderDevice );
        m_imguiRenderer.Initialize( m_pRenderDevice );
        m_resourceServerUI.Initialize();

        CreateSystemTrayIcon( IDI_TRAY_IDLE );
        HideApplicationWindow();
        return true;
    }

    bool ResourceServerApplication::Shutdown()
    {
        if ( IsInitialized() )
        {
            m_resourceServerUI.Shutdown();
            m_imguiRenderer.Shutdown();
            m_imguiSystem.Shutdown();
        }

        //-------------------------------------------------------------------------

        m_resourceServer.Shutdown();

        if ( m_pRenderDevice != nullptr )
        {
            m_pRenderDevice->Shutdown();
            EE::Delete( m_pRenderDevice );
        }

        //-------------------------------------------------------------------------

        DestroySystemTrayIcon();

        return true;
    }

    //-------------------------------------------------------------------------

    bool ResourceServerApplication::ApplicationLoop()
    {
        Milliseconds deltaTime = 0;
        {
            ScopedTimer<PlatformClock> frameTimer( deltaTime );

            // Update resource server
            //-------------------------------------------------------------------------

            m_resourceServer.Update();

            // Sleep when idle to reduce CPU load
            auto const isBusy = m_resourceServer.IsBusy();
            if( !isBusy )
            {
                Threading::Sleep( 1 );
            }

            // Update task bar
            //-------------------------------------------------------------------------

            EE_ASSERT( m_pTaskbarInterface != nullptr );

            // Showing progress bar
            if ( m_busyOverlaySet )
            {
                // Switch to idle
                if ( !isBusy )
                {
                    m_pTaskbarInterface->SetOverlayIcon( m_windowHandle, nullptr, L"" );
                    m_pTaskbarInterface->SetProgressState( m_windowHandle, TBPF_NOPROGRESS );
                    RefreshSystemTrayIcon( IDI_TRAY_IDLE );
                    m_busyOverlaySet = false;
                }
            }
            else // Idle
            {
                if ( isBusy )
                {
                    m_pTaskbarInterface->SetOverlayIcon( m_windowHandle, m_busyOverlayIcon, L"" );
                    m_pTaskbarInterface->SetProgressState( m_windowHandle, TBPF_NORMAL );
                    RefreshSystemTrayIcon( IDI_TRAY_BUSY );
                    m_busyOverlaySet = true;
                }
            }

            // Draw UI
            //-------------------------------------------------------------------------

            if ( !IsIconic( m_windowHandle ) )
            {
                m_imguiSystem.StartFrame( m_deltaTime );
              
                m_resourceServerUI.Draw();

                m_imguiSystem.EndFrame();

                m_imguiRenderer.RenderViewport( m_deltaTime, m_viewport, *m_pRenderDevice->GetPrimaryWindowRenderTarget() );
                m_pRenderDevice->PresentFrame();
            }
        }

        m_deltaTime = deltaTime;
        EngineClock::Update( deltaTime );

        return true;
    }
}

//-------------------------------------------------------------------------

int APIENTRY _tWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
    HANDLE pSingletonMutex = CreateMutex( NULL, TRUE, "Esoterica Resource Server" );
    if ( GetLastError() == ERROR_ALREADY_EXISTS )
    {
        MessageBox( GetActiveWindow(), "Only a single instance of the Resource Server is allowed to run!", "Fatal Error Occurred!", MB_OK | MB_ICONERROR );
        return -1;
    }

    //-------------------------------------------------------------------------

    int result = 0;
    {
        #if EE_ENABLE_LPP
        auto lppAgent = EE::ScopedLPPAgent();
        #endif

        EE::ApplicationGlobalState globalState;
        EE::ResourceServerApplication engineApplication( hInstance );
        result = engineApplication.Run( __argc, __argv );
    }

    //-------------------------------------------------------------------------

    ReleaseMutex( pSingletonMutex );
    CloseHandle( pSingletonMutex );

    return result;
}