#ifdef _WIN32
#pragma once

#include "Base/Application/ApplicationGlobalState.h"
#include "Base/Esoterica.h"
#include "Base/Types/String.h"
#include "Base/Math/Math.h"
#include "Base/Types/BitFlags.h"
#include <windows.h>

#if EE_ENABLE_LPP
#include "LPP_API_x64_CPP.h"
#endif

//-------------------------------------------------------------------------

namespace EE
{
    namespace Math { class ScreenSpaceRectangle; }

    //-------------------------------------------------------------------------

    class EE_BASE_API Win32Application
    {
    protected:

        enum class InitOptions
        {
            StartMinimized,
            Borderless
        };

    public:

        Win32Application( HINSTANCE hInstance, char const* applicationName, int32_t iconResourceID, int32_t splashScreenResourceID = -1, TBitFlags<InitOptions> options = TBitFlags<InitOptions>() );
        virtual ~Win32Application();

        int32_t Run( int32_t argc, char** argv );

        inline bool WasInitialized() const { return m_initialized; }

        // Win32 Window process
        virtual LRESULT WindowMessageProcessor( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

        // Called whenever we receive an application exit request. Return true to allow the exit
        virtual bool OnUserExitRequest() { return true; }

        // Get the application icon
        inline HICON GetIcon() const { return m_windowIcon; }

    protected:

        virtual bool FatalError( String const& error ) const;

        //-------------------------------------------------------------------------

        // Called after showing the main window for the first time
        virtual void OnFirstShowMainWindow() {}

        // Called just before destroying the window
        virtual void ProcessWindowDestructionMessage();

        // Handle windows resize messages
        virtual void ResizeMainWindow( Int2 const& newWindowSize ) = 0;

        // Handle user application input messages
        virtual void ProcessInputMessage( UINT message, WPARAM wParam, LPARAM lParam ) {};

        // Get title bar region for border less windows
        virtual void GetBorderlessTitleBarInfo( Math::ScreenSpaceRectangle& outTitlebarRect, bool& isInteractibleWidgetHovered ) const {};

        //-------------------------------------------------------------------------

        // These function allows the application to read/write any window layout/positioning specific settings it needs
        virtual void WriteWindowSettings();
        virtual void ReadWindowSettings();

        // Initialize/Shutdown
        virtual bool Initialize( int32_t argc, char** argv ) = 0;
        virtual bool Shutdown() = 0;

        // The actual application loop
        virtual bool ApplicationLoop() = 0;

        //-------------------------------------------------------------------------

        void RequestApplicationExit() { m_applicationRequestedExit = true; }

        // Live++
        //-------------------------------------------------------------------------

        #if EE_ENABLE_LPP
        bool LivePP_CreateAgent();
        void LivePP_DestroyAgent();
        void LivePP_UpdateAgent();

        virtual void LivePP_PreReload() {}
        virtual void LivePP_PostReload() {}

        // This needs to be inlined so that it is called from the application module and can automatically enable all imported modules
        EE_FORCE_INLINE void LivePP_EnableModules()
        {
            if ( lpp::LppIsValidSynchronizedAgent( &m_agent ) && m_agent.EnableModule )
            {
                m_agent.EnableModule( lpp::LppGetCurrentModulePath(), lpp::LPP_MODULES_OPTION_ALL_IMPORT_MODULES, nullptr, nullptr );
            }
        }
        #endif

    private:

        bool TryCreateMainWindow();
        void ShowMainWindow();

        bool TryCreateSplashScreen();
        void DestroySplashScreen();

        LRESULT BorderlessWindowHitTest( POINT cursor );

    protected:

        String const                    m_applicationName;
        String const                    m_applicationNameNoWhitespace;
        int32_t                         m_applicationIconResourceID = -1;
        HINSTANCE                       m_pInstance = nullptr;

        WNDCLASSEX                      m_windowClass = { 0 };
        HICON                           m_windowIcon = nullptr;
        HWND                            m_windowHandle = nullptr;
        RECT                            m_windowRect = { 0, 0, 640, 480 };

        int32_t                         m_splashScreenBitmapResourceID = -1;
        WNDCLASSEX                      m_splashScreenWindowClass = { 0 };
        HWND                            m_splashScreenHandle = nullptr;
        HBITMAP                         m_splashScreenBitmapHandle = nullptr;
        HDC                             m_splashScreenDeviceContext = nullptr;
        HDC                             m_splashScreenMemoryDeviceContext = nullptr;
        POINT                           m_splashScreenStartPoint = { 0, 0 };

    private:

        bool                            m_wasMaximized = false; // Read from the layout settings
        bool                            m_startMinimized = false; // Specifies the initial state of the application
        bool                            m_initialized = false;
        bool                            m_applicationRequestedExit = false;
        bool                            m_isBorderLess = false;

        #if EE_ENABLE_LPP
        lpp::LppSynchronizedAgent       m_agent;
        #endif
    };
}
#endif