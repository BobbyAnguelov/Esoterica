#include "ResourceServerApplication.h"
#include "Resources/Resource.h"
#include "System/Log.h"
#include "System/Time/Timers.h"
#include "System/FileSystem/FileSystemUtils.h"
#include "System/Imgui/Platform/ImguiPlatform_win32.h"
#include "System/Platform/PlatformHelpers_Win32.h"
#include "System/IniFile.h"
#include <tchar.h>
#include <shobjidl_core.h>

#if EE_ENABLE_LPP
#include "LPP_API_x64_CPP.h"
#endif

//-------------------------------------------------------------------------

namespace EE
{
    constexpr static int32_t const g_shellIconCallbackMessageID = WM_USER + 1;

    //-------------------------------------------------------------------------

    ResourceServerApplication::ResourceServerApplication( HINSTANCE pInstance )
        : Win32Application( pInstance, "Esoterica Resource Server", IDI_RESOURCESERVER, true )
        , m_resourceServerUI( m_resourceServer )
    {}

    LRESULT ResourceServerApplication::WndProcess( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
    {
        // ImGui specific message processing
        //-------------------------------------------------------------------------

        auto const imguiResult = ImGuiX::Platform::WindowsMessageHandler( hWnd, message, wParam, lParam );
        if ( imguiResult != 0 )
        {
            return imguiResult;
        }

        // General
        //-------------------------------------------------------------------------

        switch ( message )
        {
            case WM_SIZE:
            {
                Int2 const newDimensions( LOWORD( lParam ), HIWORD( lParam ) );
                if ( newDimensions.m_x > 0 && newDimensions.m_y > 0 )
                {
                    m_pRenderDevice->ResizePrimaryWindowRenderTarget( newDimensions );
                    m_viewport.Resize( Int2::Zero, newDimensions );

                    // Hack to fix client area offset bug
                    RECT rect;
                    GetWindowRect( hWnd, &rect );
                    MoveWindow( hWnd, rect.left + 1, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE );
                }
            }
            break;
        }

        return 0;
    }

    void ResourceServerApplication::OnWindowDestruction()
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

        Win32Application::OnWindowDestruction();
    }

    bool ResourceServerApplication::OnExitRequest()
    {
        int32_t const closeDialogResult = MessageBox( m_windowHandle, "Close Resource Server?\r\n\r\nSelect 'No' to minimize instead.", "Resource Server", MB_YESNOCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2 | MB_TOPMOST );
        if ( closeDialogResult == IDYES )
        {
            return true;
        }
        else if ( closeDialogResult == IDNO )
        {
            ShowWindow( m_windowHandle, SW_MINIMIZE );
        }

        return false;
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

        m_imguiSystem.Initialize( m_applicationNameNoWhitespace + ".imgui.ini", m_pRenderDevice );
        m_imguiRenderer.Initialize( m_pRenderDevice );

        //-------------------------------------------------------------------------

        // Always start resource server minimized
        ShowWindow( m_windowHandle, SW_MINIMIZE );

        return true;
    }

    bool ResourceServerApplication::Shutdown()
    {
        if ( IsInitialized() )
        {
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
            auto const resourceServerBusyState = m_resourceServer.GetBusyState();
            if( !resourceServerBusyState.m_isBusy )
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
                if ( !resourceServerBusyState.m_isBusy )
                {
                    m_pTaskbarInterface->SetOverlayIcon( m_windowHandle, nullptr, L"" );
                    m_pTaskbarInterface->SetProgressState( m_windowHandle, TBPF_NOPROGRESS );
                    m_busyOverlaySet = false;
                }
                else // Update percentage
                {
                    m_pTaskbarInterface->SetProgressValue( m_windowHandle, resourceServerBusyState.m_completedRequests, resourceServerBusyState.m_totalRequests );
                }
            }
            else // Idle
            {
                if ( resourceServerBusyState.m_isBusy )
                {
                    m_pTaskbarInterface->SetOverlayIcon( m_windowHandle, m_busyOverlayIcon, L"" );
                    m_pTaskbarInterface->SetProgressState( m_windowHandle, TBPF_NORMAL );
                    m_pTaskbarInterface->SetProgressValue( m_windowHandle, resourceServerBusyState.m_completedRequests, resourceServerBusyState.m_totalRequests );
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
    // Live++ Support
    //-------------------------------------------------------------------------

    #if EE_ENABLE_LPP
    //auto lppAgent = lpp::LppCreateDefaultAgent( L"../../External/LivePP", L"" );
    //lppAgent.EnableModule( lpp::LppGetCurrentModulePath(), lpp::LPP_MODULES_OPTION_NONE );
    #endif

    //-------------------------------------------------------------------------
    
    EE::ApplicationGlobalState globalState;
    EE::ResourceServerApplication engineApplication( hInstance );
    int const result = engineApplication.Run( __argc, __argv );

    //-------------------------------------------------------------------------

    ReleaseMutex( pSingletonMutex );
    CloseHandle( pSingletonMutex );

    //-------------------------------------------------------------------------

    #if EE_ENABLE_LPP
    //lpp::LppDestroyDefaultAgent( &lppAgent );
    #endif

    return result;
}