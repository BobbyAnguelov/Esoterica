#ifdef _WIN32
#pragma once

#include "System/Esoterica.h"
#include "System/Types/String.h"
#include "ApplicationGlobalState.h"
#include "System/Math/Math.h"
#include <windows.h>

//-------------------------------------------------------------------------

namespace EE
{
    class Win32Application
    {

    public:

        Win32Application( HINSTANCE hInstance, char const* applicationName, int32_t iconResourceID );
        virtual ~Win32Application();

        int Run( int32_t argc, char** argv );

        inline bool IsInitialized() const { return m_initialized; }

        // Win32 Window process
        virtual LRESULT WndProcess( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) = 0;

        // Called just before destroying the window
        virtual void OnWindowDestruction();

        // Called whenever we receive an application exit request. Return true to allow the exit
        virtual bool OnExitRequest() { return true; }

        // Call this to shutdown the application
        inline void Exit() { m_shouldExit = true; }

    protected:

        bool FatalError( String const& error );

        // Window creation
        bool TryCreateWindow();

        // This function allows the application to process all command line settings. Will be called before initialize.
        virtual bool ProcessCommandline( int32_t argc, char** argv ) { return true; }

        // These function allows the application to read/write any layout/positioning specific settings it needs
        virtual void WriteLayoutSettings();
        virtual void ReadLayoutSettings();

        // Initialize/Shutdown
        virtual bool Initialize() = 0;
        virtual bool Shutdown() = 0;

        // The actual application loop
        virtual bool ApplicationLoop() = 0;

    protected:

        String const                    m_applicationName;
        String const                    m_applicationNameNoWhitespace;
        int32_t                         m_applicationIconResourceID = -1;
        WNDCLASSEX                      m_windowClass;
        HINSTANCE                       m_pInstance = nullptr;
        HICON                           m_windowIcon = nullptr;
        HWND                            m_windowHandle = nullptr;
        RECT                            m_windowRect = { 0, 0, 640, 480 };
        MSG                             m_message;

        // Custom flags that user applications can set to specify what modes were enabled or what windows were open (saved in the layout.ini)
        uint64_t                        m_userFlags = 0; 

    private:

        bool                            m_wasMaximized = false; // Read from the layout settings
        bool                            m_initialized = false;
        bool                            m_shouldExit = false;
    };
}

#endif