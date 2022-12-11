#include "ResourceServerApplication.h"
#include "Resources/Resource.h"
#include "Applications/Shared/LivePP/LivePP.h"
#include "System/Log.h"
#include "System/Time/Timers.h"
#include "System/FileSystem/FileSystemUtils.h"
#include "System/Imgui/Platform/ImguiPlatform_win32.h"
#include "System/Platform/PlatformHelpers_Win32.h"
#include "System/IniFile.h"
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

        // Hack to fix client area offset bug
        RECT rect;
        GetWindowRect( m_windowHandle, &rect );
        MoveWindow( m_windowHandle, rect.left + 1, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE );
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

        m_imguiSystem.Initialize( m_pRenderDevice );
        m_imguiRenderer.Initialize( m_pRenderDevice );
        m_resourceServerUI.Initialize();

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
                    m_busyOverlaySet = false;
                }
            }
            else // Idle
            {
                if ( isBusy )
                {
                    m_pTaskbarInterface->SetOverlayIcon( m_windowHandle, m_busyOverlayIcon, L"" );
                    m_pTaskbarInterface->SetProgressState( m_windowHandle, TBPF_NORMAL );
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