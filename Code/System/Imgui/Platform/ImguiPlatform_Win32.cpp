#if _WIN32
#include "ImguiPlatform_Win32.h"
#include "System/Imgui/ImguiSystem.h"
#include "System/ThirdParty/imgui/imgui.h"
#include "System/Input/InputSystem.h"
#include "System/Platform/PlatformHelpers_Win32.h"
#include "System/Memory/Memory.h"
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    namespace Platform
    {
        //-------------------------------------------------------------------------
        // This code is taken from the imgui docking branch examples
        //-------------------------------------------------------------------------

        struct ImGui_ImplWin32_Data
        {
            HWND                        hWnd;
            HWND                        MouseHwnd;
            int                         MouseTrackedArea;   // 0: not tracked, 1: client are, 2: non-client area
            int                         MouseButtonsDown = 0;
            ImGuiMouseCursor            LastMouseCursor;
            bool                        WantUpdateHasGamepad = false;
            bool                        WantUpdateMonitors = false;

            WNDCLASSEX                  m_imguiWindowClass;
            Input::InputSystem*         m_pEngineInputSystem = nullptr;
        };

        // Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
        // It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
        // FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
        // FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
        static ImGui_ImplWin32_Data* ImGui_ImplWin32_GetBackendData()
        {
            return ImGui::GetCurrentContext() ? (ImGui_ImplWin32_Data*) ImGui::GetIO().BackendPlatformUserData : nullptr;
        }

        // DPI
        //-------------------------------------------------------------------------

        namespace
        {
            // Perform our own check with RtlVerifyVersionInfo() instead of using functions from <VersionHelpers.h> as they
            // require a manifest to be functional for checks above 8.1. See https://github.com/ocornut/imgui/issues/4200
            static BOOL _IsWindowsVersionOrGreater( WORD major, WORD minor, WORD )
            {
                typedef LONG( WINAPI* PFN_RtlVerifyVersionInfo )( OSVERSIONINFOEXW*, ULONG, ULONGLONG );
                static PFN_RtlVerifyVersionInfo RtlVerifyVersionInfoFn = nullptr;
                if ( RtlVerifyVersionInfoFn == nullptr )
                    if ( HMODULE ntdllModule = ::GetModuleHandleA( "ntdll.dll" ) )
                        RtlVerifyVersionInfoFn = (PFN_RtlVerifyVersionInfo)GetProcAddress( ntdllModule, "RtlVerifyVersionInfo" );
                if ( RtlVerifyVersionInfoFn == nullptr )
                    return FALSE;

                RTL_OSVERSIONINFOEXW versionInfo = { };
                ULONGLONG conditionMask = 0;
                versionInfo.dwOSVersionInfoSize = sizeof( RTL_OSVERSIONINFOEXW );
                versionInfo.dwMajorVersion = major;
                versionInfo.dwMinorVersion = minor;
                VER_SET_CONDITION( conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
                VER_SET_CONDITION( conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL );
                return ( RtlVerifyVersionInfoFn( &versionInfo, VER_MAJORVERSION | VER_MINORVERSION, conditionMask ) == 0 ) ? TRUE : FALSE;
            }

            #define _IsWindowsVistaOrGreater()   _IsWindowsVersionOrGreater(HIBYTE(0x0600), LOBYTE(0x0600), 0) // _WIN32_WINNT_VISTA
            #define _IsWindows8OrGreater()       _IsWindowsVersionOrGreater(HIBYTE(0x0602), LOBYTE(0x0602), 0) // _WIN32_WINNT_WIN8
            #define _IsWindows8Point1OrGreater() _IsWindowsVersionOrGreater(HIBYTE(0x0603), LOBYTE(0x0603), 0) // _WIN32_WINNT_WINBLUE
            #define _IsWindows10OrGreater()      _IsWindowsVersionOrGreater(HIBYTE(0x0A00), LOBYTE(0x0A00), 0) // _WIN32_WINNT_WINTHRESHOLD / _WIN32_WINNT_WIN10

            #ifndef DPI_ENUMS_DECLARED
            typedef enum { MDT_EFFECTIVE_DPI = 0, MDT_ANGULAR_DPI = 1, MDT_RAW_DPI = 2, MDT_DEFAULT = MDT_EFFECTIVE_DPI } MONITOR_DPI_TYPE;
            #endif
            typedef HRESULT( WINAPI* PFN_GetDpiForMonitor )( HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT* );        // Shcore.lib + dll, Windows 8.1+

            #if defined(_MSC_VER) && !defined(NOGDI)
            #pragma comment(lib, "gdi32")   // Link with gdi32.lib for GetDeviceCaps(). MinGW will require linking with '-lgdi32'
            #endif

            static float ImGui_ImplWin32_GetDpiScaleForMonitor( void* monitor )
            {
                UINT xdpi = 96, ydpi = 96;
                if ( _IsWindows8Point1OrGreater() )
                {
                    static HINSTANCE shcore_dll = ::LoadLibraryA( "shcore.dll" ); // Reference counted per-process
                    static PFN_GetDpiForMonitor GetDpiForMonitorFn = nullptr;
                    if ( GetDpiForMonitorFn == nullptr && shcore_dll != nullptr )
                        GetDpiForMonitorFn = ( PFN_GetDpiForMonitor )::GetProcAddress( shcore_dll, "GetDpiForMonitor" );
                    if ( GetDpiForMonitorFn != nullptr )
                    {
                        GetDpiForMonitorFn( (HMONITOR)monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi );
                        IM_ASSERT( xdpi == ydpi ); // Please contact me if you hit this assert!
                        return xdpi / 96.0f;
                    }
                }
                #ifndef NOGDI
                const HDC dc = ::GetDC( nullptr );
                xdpi = ::GetDeviceCaps( dc, LOGPIXELSX );
                ydpi = ::GetDeviceCaps( dc, LOGPIXELSY );
                IM_ASSERT( xdpi == ydpi ); // Please contact me if you hit this assert!
                ::ReleaseDC( nullptr, dc );
                #endif
                return xdpi / 96.0f;
            }

            static float ImGui_ImplWin32_GetDpiScaleForHwnd( void* hwnd )
            {
                HMONITOR monitor = ::MonitorFromWindow( (HWND)hwnd, MONITOR_DEFAULTTONEAREST );
                return ImGui_ImplWin32_GetDpiScaleForMonitor( monitor );
            }
        }

        // Monitors
        //-------------------------------------------------------------------------

        static BOOL CALLBACK ImGui_ImplWin32_UpdateMonitors_EnumFunc( HMONITOR monitor, HDC, LPRECT, LPARAM )
        {
            MONITORINFO info = { 0 };
            info.cbSize = sizeof( MONITORINFO );
            if ( !::GetMonitorInfo( monitor, &info ) )
            {
                return TRUE;
            }

            ImGuiPlatformMonitor imgui_monitor;
            imgui_monitor.MainPos = ImVec2( (float) info.rcMonitor.left, (float) info.rcMonitor.top );
            imgui_monitor.MainSize = ImVec2( (float) ( info.rcMonitor.right - info.rcMonitor.left ), (float) ( info.rcMonitor.bottom - info.rcMonitor.top ) );
            imgui_monitor.WorkPos = ImVec2( (float) info.rcWork.left, (float) info.rcWork.top );
            imgui_monitor.WorkSize = ImVec2( (float) ( info.rcWork.right - info.rcWork.left ), (float) ( info.rcWork.bottom - info.rcWork.top ) );
            imgui_monitor.DpiScale = ImGui_ImplWin32_GetDpiScaleForMonitor( monitor );

            ImGuiPlatformIO& io = ImGui::GetPlatformIO();
            if ( info.dwFlags & MONITORINFOF_PRIMARY )
            {
                io.Monitors.push_front( imgui_monitor );
            }
            else
            {
                io.Monitors.push_back( imgui_monitor );
            }
            return TRUE;
        }

        static void ImGui_ImplWin32_UpdateMonitors()
        {
            ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();
            ImGui::GetPlatformIO().Monitors.resize( 0 );
            ::EnumDisplayMonitors( nullptr, nullptr, ImGui_ImplWin32_UpdateMonitors_EnumFunc, 0 );
            bd->WantUpdateMonitors = false;
        }

        // Windows
        //-------------------------------------------------------------------------

        struct ImGui_ImplWin32_ViewportData
        {
            HWND                        Hwnd;
            bool                        HwndOwned;
            DWORD                       DwStyle;
            DWORD                       DwExStyle;

            ImGui_ImplWin32_ViewportData() { Hwnd = NULL; HwndOwned = false;  DwStyle = DwExStyle = 0; }
            ~ImGui_ImplWin32_ViewportData() { EE_ASSERT( Hwnd == NULL ); }
        };

        static void ImGui_ImplWin32_GetWin32StyleFromViewportFlags( ImGuiViewportFlags flags, DWORD* out_style, DWORD* out_ex_style )
        {
            if ( flags & ImGuiViewportFlags_NoDecoration )
                *out_style = WS_POPUP;
            else
                *out_style = WS_OVERLAPPEDWINDOW;

            if ( flags & ImGuiViewportFlags_NoTaskBarIcon )
                *out_ex_style = WS_EX_TOOLWINDOW;
            else
                *out_ex_style = WS_EX_APPWINDOW;

            if ( flags & ImGuiViewportFlags_TopMost )
                *out_ex_style |= WS_EX_TOPMOST;
        }

        static void ImGui_ImplWin32_CreateWindow( ImGuiViewport* viewport )
        {
            ImGui_ImplWin32_ViewportData* vd = EE::New<ImGui_ImplWin32_ViewportData>();
            viewport->PlatformUserData = vd;

            // Select style and parent window
            ImGui_ImplWin32_GetWin32StyleFromViewportFlags( viewport->Flags, &vd->DwStyle, &vd->DwExStyle );
            HWND parent_window = nullptr;
            if ( viewport->ParentViewportId != 0 )
                if ( ImGuiViewport* parent_viewport = ImGui::FindViewportByID( viewport->ParentViewportId ) )
                    parent_window = (HWND)parent_viewport->PlatformHandle;

            // Create window
            RECT rect = { (LONG)viewport->Pos.x, (LONG)viewport->Pos.y, (LONG)( viewport->Pos.x + viewport->Size.x ), (LONG)( viewport->Pos.y + viewport->Size.y ) };
            ::AdjustWindowRectEx( &rect, vd->DwStyle, FALSE, vd->DwExStyle );
            vd->Hwnd = ::CreateWindowEx(
                vd->DwExStyle, _T( "ImGui Platform" ), _T( "Untitled" ), vd->DwStyle,       // Style, class name, window name
                rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,    // Window area
                parent_window, nullptr, ::GetModuleHandle( nullptr ), nullptr );           // Parent window, Menu, Instance, Param
            vd->HwndOwned = true;
            viewport->PlatformRequestResize = false;
            viewport->PlatformHandle = viewport->PlatformHandleRaw = vd->Hwnd;

            //auto const errorMessage = EE::Platform::Win32::GetLastErrorMessage();
            EE_ASSERT( vd->Hwnd != 0 );
        }

        static void ImGui_ImplWin32_DestroyWindow( ImGuiViewport* viewport )
        {
            ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();
            if ( ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData )
            {
                if ( ::GetCapture() == vd->Hwnd )
                {
                    // Transfer capture so if we started dragging from a window that later disappears, we'll still receive the MOUSEUP event.
                    ::ReleaseCapture();
                    ::SetCapture( bd->hWnd );
                }
                if ( vd->Hwnd && vd->HwndOwned )
                    ::DestroyWindow( vd->Hwnd );
                vd->Hwnd = nullptr;
                EE::Delete( vd );
            }
            viewport->PlatformUserData = viewport->PlatformHandle = nullptr;
        }

        static void ImGui_ImplWin32_ShowWindow( ImGuiViewport* viewport )
        {
            ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
            IM_ASSERT( vd->Hwnd != 0 );
            if ( viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing )
                ::ShowWindow( vd->Hwnd, SW_SHOWNA );
            else
                ::ShowWindow( vd->Hwnd, SW_SHOW );
        }

        static void ImGui_ImplWin32_UpdateWindow( ImGuiViewport* viewport )
        {
            // (Optional) Update Win32 style if it changed _after_ creation.
            // Generally they won't change unless configuration flags are changed, but advanced uses (such as manually rewriting viewport flags) make this useful.
            ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
            IM_ASSERT( vd->Hwnd != 0 );
            DWORD new_style;
            DWORD new_ex_style;
            ImGui_ImplWin32_GetWin32StyleFromViewportFlags( viewport->Flags, &new_style, &new_ex_style );

            // Only reapply the flags that have been changed from our point of view (as other flags are being modified by Windows)
            if ( vd->DwStyle != new_style || vd->DwExStyle != new_ex_style )
            {
                // (Optional) Update TopMost state if it changed _after_ creation
                bool top_most_changed = ( vd->DwExStyle & WS_EX_TOPMOST ) != ( new_ex_style & WS_EX_TOPMOST );
                HWND insert_after = top_most_changed ? ( ( viewport->Flags & ImGuiViewportFlags_TopMost ) ? HWND_TOPMOST : HWND_NOTOPMOST ) : 0;
                UINT swp_flag = top_most_changed ? 0 : SWP_NOZORDER;

                // Apply flags and position (since it is affected by flags)
                vd->DwStyle = new_style;
                vd->DwExStyle = new_ex_style;
                ::SetWindowLong( vd->Hwnd, GWL_STYLE, vd->DwStyle );
                ::SetWindowLong( vd->Hwnd, GWL_EXSTYLE, vd->DwExStyle );
                RECT rect = { (LONG)viewport->Pos.x, (LONG)viewport->Pos.y, (LONG)( viewport->Pos.x + viewport->Size.x ), (LONG)( viewport->Pos.y + viewport->Size.y ) };
                ::AdjustWindowRectEx( &rect, vd->DwStyle, FALSE, vd->DwExStyle ); // Client to Screen
                ::SetWindowPos( vd->Hwnd, insert_after, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, swp_flag | SWP_NOACTIVATE | SWP_FRAMECHANGED );
                ::ShowWindow( vd->Hwnd, SW_SHOWNA ); // This is necessary when we alter the style
                viewport->PlatformRequestMove = viewport->PlatformRequestResize = true;
            }
        }

        static ImVec2 ImGui_ImplWin32_GetWindowPos( ImGuiViewport* viewport )
        {
            ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
            IM_ASSERT( vd->Hwnd != 0 );
            POINT pos = { 0, 0 };
            ::ClientToScreen( vd->Hwnd, &pos );
            return ImVec2( (float)pos.x, (float)pos.y );
        }

        static void ImGui_ImplWin32_SetWindowPos( ImGuiViewport* viewport, ImVec2 pos )
        {
            ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
            IM_ASSERT( vd->Hwnd != 0 );
            RECT rect = { (LONG)pos.x, (LONG)pos.y, (LONG)pos.x, (LONG)pos.y };
            ::AdjustWindowRectEx( &rect, vd->DwStyle, FALSE, vd->DwExStyle );
            ::SetWindowPos( vd->Hwnd, nullptr, rect.left, rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE );
        }

        static ImVec2 ImGui_ImplWin32_GetWindowSize( ImGuiViewport* viewport )
        {
            ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
            IM_ASSERT( vd->Hwnd != 0 );
            RECT rect;
            ::GetClientRect( vd->Hwnd, &rect );
            return ImVec2( float( rect.right - rect.left ), float( rect.bottom - rect.top ) );
        }

        static void ImGui_ImplWin32_SetWindowSize( ImGuiViewport* viewport, ImVec2 size )
        {
            ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
            IM_ASSERT( vd->Hwnd != 0 );
            RECT rect = { 0, 0, (LONG)size.x, (LONG)size.y };
            ::AdjustWindowRectEx( &rect, vd->DwStyle, FALSE, vd->DwExStyle ); // Client to Screen
            ::SetWindowPos( vd->Hwnd, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE );
        }

        static void ImGui_ImplWin32_SetWindowFocus( ImGuiViewport* viewport )
        {
            ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
            IM_ASSERT( vd->Hwnd != 0 );
            ::BringWindowToTop( vd->Hwnd );
            ::SetForegroundWindow( vd->Hwnd );
            ::SetFocus( vd->Hwnd );
        }

        static bool ImGui_ImplWin32_GetWindowFocus( ImGuiViewport* viewport )
        {
            ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
            IM_ASSERT( vd->Hwnd != 0 );
            return ::GetForegroundWindow() == vd->Hwnd;
        }

        static bool ImGui_ImplWin32_GetWindowMinimized( ImGuiViewport* viewport )
        {
            ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
            IM_ASSERT( vd->Hwnd != 0 );
            return ::IsIconic( vd->Hwnd ) != 0;
        }

        static void ImGui_ImplWin32_SetWindowTitle( ImGuiViewport* viewport, const char* title )
        {
            // ::SetWindowTextA() doesn't properly handle UTF-8 so we explicitely convert our string.
            ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
            IM_ASSERT( vd->Hwnd != 0 );
            int n = ::MultiByteToWideChar( CP_UTF8, 0, title, -1, nullptr, 0 );
            ImVector<wchar_t> title_w;
            title_w.resize( n );
            ::MultiByteToWideChar( CP_UTF8, 0, title, -1, title_w.Data, n );
            ::SetWindowTextW( vd->Hwnd, title_w.Data );
        }

        static void ImGui_ImplWin32_SetWindowAlpha( ImGuiViewport* viewport, float alpha )
        {
            ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
            IM_ASSERT( vd->Hwnd != 0 );
            IM_ASSERT( alpha >= 0.0f && alpha <= 1.0f );
            if ( alpha < 1.0f )
            {
                DWORD style = ::GetWindowLongW( vd->Hwnd, GWL_EXSTYLE ) | WS_EX_LAYERED;
                ::SetWindowLongW( vd->Hwnd, GWL_EXSTYLE, style );
                ::SetLayeredWindowAttributes( vd->Hwnd, 0, (BYTE)( 255 * alpha ), LWA_ALPHA );
            }
            else
            {
                DWORD style = ::GetWindowLongW( vd->Hwnd, GWL_EXSTYLE ) & ~WS_EX_LAYERED;
                ::SetWindowLongW( vd->Hwnd, GWL_EXSTYLE, style );
            }
        }

        static float ImGui_ImplWin32_GetWindowDpiScale( ImGuiViewport* viewport )
        {
            ImGui_ImplWin32_ViewportData* vd = (ImGui_ImplWin32_ViewportData*)viewport->PlatformUserData;
            IM_ASSERT( vd->Hwnd != 0 );
            return ImGui_ImplWin32_GetDpiScaleForHwnd( vd->Hwnd );
        }

        // FIXME-DPI: Testing DPI related ideas
        static void ImGui_ImplWin32_OnChangedViewport( ImGuiViewport* viewport )
        {
            (void)viewport;
            //#if 0
            ImGuiStyle default_style;
            //default_style.WindowPadding = ImVec2(0, 0);
            //default_style.WindowBorderSize = 0.0f;
            //default_style.ItemSpacing.y = 3.0f;
            //default_style.FramePadding = ImVec2(0, 0);
            default_style.ScaleAllSizes( viewport->DpiScale );
            ImGuiStyle& style = ImGui::GetStyle();
            style = default_style;
            //#endif
        }

        // Input
        //-------------------------------------------------------------------------

        static bool ImGui_ImplWin32_UpdateMouseCursor()
        {
            ImGuiIO& io = ImGui::GetIO();
            if ( io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange )
                return false;

            ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
            if ( imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor )
            {
                // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
                ::SetCursor( nullptr );
            }
            else
            {
                // Show OS mouse cursor
                LPTSTR win32_cursor = IDC_ARROW;
                switch ( imgui_cursor )
                {
                    case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
                    case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
                    case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
                    case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
                    case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
                    case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
                    case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
                    case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
                    case ImGuiMouseCursor_NotAllowed:   win32_cursor = IDC_NO; break;
                }
                ::SetCursor( ::LoadCursor( nullptr, win32_cursor ) );
            }
            return true;
        }

        static void ImGui_ImplWin32_AddKeyEvent( ImGuiKey key, bool down, int native_keycode, int native_scancode = -1 )
        {
            ImGuiIO& io = ImGui::GetIO();
            io.AddKeyEvent( key, down );
            io.SetKeyEventNativeData( key, native_keycode, native_scancode ); // To support legacy indexing (<1.87 user code)
            IM_UNUSED( native_scancode );
        }

        // There is no distinct VK_xxx for keypad enter, instead it is VK_RETURN + KF_EXTENDED, we assign it an arbitrary value to make code more readable (VK_ codes go up to 255)
        #define IM_VK_KEYPAD_ENTER      (VK_RETURN + 256)

        static bool IsVkDown( int vk )
        {
            return ( ::GetKeyState( vk ) & 0x8000 ) != 0;
        }

        static ImGuiKey ImGui_ImplWin32_VirtualKeyToImGuiKey( WPARAM wParam )
        {
            switch ( wParam )
            {
                case VK_TAB: return ImGuiKey_Tab;
                case VK_LEFT: return ImGuiKey_LeftArrow;
                case VK_RIGHT: return ImGuiKey_RightArrow;
                case VK_UP: return ImGuiKey_UpArrow;
                case VK_DOWN: return ImGuiKey_DownArrow;
                case VK_PRIOR: return ImGuiKey_PageUp;
                case VK_NEXT: return ImGuiKey_PageDown;
                case VK_HOME: return ImGuiKey_Home;
                case VK_END: return ImGuiKey_End;
                case VK_INSERT: return ImGuiKey_Insert;
                case VK_DELETE: return ImGuiKey_Delete;
                case VK_BACK: return ImGuiKey_Backspace;
                case VK_SPACE: return ImGuiKey_Space;
                case VK_RETURN: return ImGuiKey_Enter;
                case VK_ESCAPE: return ImGuiKey_Escape;
                case VK_OEM_7: return ImGuiKey_Apostrophe;
                case VK_OEM_COMMA: return ImGuiKey_Comma;
                case VK_OEM_MINUS: return ImGuiKey_Minus;
                case VK_OEM_PERIOD: return ImGuiKey_Period;
                case VK_OEM_2: return ImGuiKey_Slash;
                case VK_OEM_1: return ImGuiKey_Semicolon;
                case VK_OEM_PLUS: return ImGuiKey_Equal;
                case VK_OEM_4: return ImGuiKey_LeftBracket;
                case VK_OEM_5: return ImGuiKey_Backslash;
                case VK_OEM_6: return ImGuiKey_RightBracket;
                case VK_OEM_3: return ImGuiKey_GraveAccent;
                case VK_CAPITAL: return ImGuiKey_CapsLock;
                case VK_SCROLL: return ImGuiKey_ScrollLock;
                case VK_NUMLOCK: return ImGuiKey_NumLock;
                case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
                case VK_PAUSE: return ImGuiKey_Pause;
                case VK_NUMPAD0: return ImGuiKey_Keypad0;
                case VK_NUMPAD1: return ImGuiKey_Keypad1;
                case VK_NUMPAD2: return ImGuiKey_Keypad2;
                case VK_NUMPAD3: return ImGuiKey_Keypad3;
                case VK_NUMPAD4: return ImGuiKey_Keypad4;
                case VK_NUMPAD5: return ImGuiKey_Keypad5;
                case VK_NUMPAD6: return ImGuiKey_Keypad6;
                case VK_NUMPAD7: return ImGuiKey_Keypad7;
                case VK_NUMPAD8: return ImGuiKey_Keypad8;
                case VK_NUMPAD9: return ImGuiKey_Keypad9;
                case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
                case VK_DIVIDE: return ImGuiKey_KeypadDivide;
                case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
                case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
                case VK_ADD: return ImGuiKey_KeypadAdd;
                case IM_VK_KEYPAD_ENTER: return ImGuiKey_KeypadEnter;
                case VK_LSHIFT: return ImGuiKey_LeftShift;
                case VK_LCONTROL: return ImGuiKey_LeftCtrl;
                case VK_LMENU: return ImGuiKey_LeftAlt;
                case VK_LWIN: return ImGuiKey_LeftSuper;
                case VK_RSHIFT: return ImGuiKey_RightShift;
                case VK_RCONTROL: return ImGuiKey_RightCtrl;
                case VK_RMENU: return ImGuiKey_RightAlt;
                case VK_RWIN: return ImGuiKey_RightSuper;
                case VK_APPS: return ImGuiKey_Menu;
                case '0': return ImGuiKey_0;
                case '1': return ImGuiKey_1;
                case '2': return ImGuiKey_2;
                case '3': return ImGuiKey_3;
                case '4': return ImGuiKey_4;
                case '5': return ImGuiKey_5;
                case '6': return ImGuiKey_6;
                case '7': return ImGuiKey_7;
                case '8': return ImGuiKey_8;
                case '9': return ImGuiKey_9;
                case 'A': return ImGuiKey_A;
                case 'B': return ImGuiKey_B;
                case 'C': return ImGuiKey_C;
                case 'D': return ImGuiKey_D;
                case 'E': return ImGuiKey_E;
                case 'F': return ImGuiKey_F;
                case 'G': return ImGuiKey_G;
                case 'H': return ImGuiKey_H;
                case 'I': return ImGuiKey_I;
                case 'J': return ImGuiKey_J;
                case 'K': return ImGuiKey_K;
                case 'L': return ImGuiKey_L;
                case 'M': return ImGuiKey_M;
                case 'N': return ImGuiKey_N;
                case 'O': return ImGuiKey_O;
                case 'P': return ImGuiKey_P;
                case 'Q': return ImGuiKey_Q;
                case 'R': return ImGuiKey_R;
                case 'S': return ImGuiKey_S;
                case 'T': return ImGuiKey_T;
                case 'U': return ImGuiKey_U;
                case 'V': return ImGuiKey_V;
                case 'W': return ImGuiKey_W;
                case 'X': return ImGuiKey_X;
                case 'Y': return ImGuiKey_Y;
                case 'Z': return ImGuiKey_Z;
                case VK_F1: return ImGuiKey_F1;
                case VK_F2: return ImGuiKey_F2;
                case VK_F3: return ImGuiKey_F3;
                case VK_F4: return ImGuiKey_F4;
                case VK_F5: return ImGuiKey_F5;
                case VK_F6: return ImGuiKey_F6;
                case VK_F7: return ImGuiKey_F7;
                case VK_F8: return ImGuiKey_F8;
                case VK_F9: return ImGuiKey_F9;
                case VK_F10: return ImGuiKey_F10;
                case VK_F11: return ImGuiKey_F11;
                case VK_F12: return ImGuiKey_F12;
                default: return ImGuiKey_None;
            }
        }

        static void ImGui_ImplWin32_ProcessKeyEventsWorkarounds()
        {
            // Left & right Shift keys: when both are pressed together, Windows tend to not generate the WM_KEYUP event for the first released one.
            if ( ImGui::IsKeyDown( ImGuiKey_LeftShift ) && !IsVkDown( VK_LSHIFT ) )
                ImGui_ImplWin32_AddKeyEvent( ImGuiKey_LeftShift, false, VK_LSHIFT );
            if ( ImGui::IsKeyDown( ImGuiKey_RightShift ) && !IsVkDown( VK_RSHIFT ) )
                ImGui_ImplWin32_AddKeyEvent( ImGuiKey_RightShift, false, VK_RSHIFT );

            // Sometimes WM_KEYUP for Win key is not passed down to the app (e.g. for Win+V on some setups, according to GLFW).
            if ( ImGui::IsKeyDown( ImGuiKey_LeftSuper ) && !IsVkDown( VK_LWIN ) )
                ImGui_ImplWin32_AddKeyEvent( ImGuiKey_LeftSuper, false, VK_LWIN );
            if ( ImGui::IsKeyDown( ImGuiKey_RightSuper ) && !IsVkDown( VK_RWIN ) )
                ImGui_ImplWin32_AddKeyEvent( ImGuiKey_RightSuper, false, VK_RWIN );
        }

        static void ImGui_ImplWin32_UpdateKeyModifiers()
        {
            ImGuiIO& io = ImGui::GetIO();
            io.AddKeyEvent( ImGuiMod_Ctrl, IsVkDown( VK_CONTROL ) );
            io.AddKeyEvent( ImGuiMod_Shift, IsVkDown( VK_SHIFT ) );
            io.AddKeyEvent( ImGuiMod_Alt, IsVkDown( VK_MENU ) );
            io.AddKeyEvent( ImGuiMod_Super, IsVkDown( VK_APPS ) );
        }

        static void ImGui_ImplWin32_UpdateMouseData()
        {
            ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();
            ImGuiIO& io = ImGui::GetIO();
            IM_ASSERT( bd->hWnd != 0 );

            POINT mouse_screen_pos;
            bool has_mouse_screen_pos = ::GetCursorPos( &mouse_screen_pos ) != 0;

            HWND focused_window = ::GetForegroundWindow();
            const bool is_app_focused = ( focused_window && ( focused_window == bd->hWnd || ::IsChild( focused_window, bd->hWnd ) || ImGui::FindViewportByPlatformHandle( (void*)focused_window ) ) );
            if ( is_app_focused )
            {
                // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
                // When multi-viewports are enabled, all Dear ImGui positions are same as OS positions.
                if ( io.WantSetMousePos )
                {
                    POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
                    if ( ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable ) == 0 )
                        ::ClientToScreen( focused_window, &pos );
                    ::SetCursorPos( pos.x, pos.y );
                }

                // (Optional) Fallback to provide mouse position when focused (WM_MOUSEMOVE already provides this when hovered or captured)
                // This also fills a short gap when clicking non-client area: WM_NCMOUSELEAVE -> modal OS move -> gap -> WM_NCMOUSEMOVE
                if ( !io.WantSetMousePos && bd->MouseTrackedArea == 0 && has_mouse_screen_pos )
                {
                    // Single viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
                    // (This is the position you can get with ::GetCursorPos() + ::ScreenToClient() or WM_MOUSEMOVE.)
                    // Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
                    // (This is the position you can get with ::GetCursorPos() or WM_MOUSEMOVE + ::ClientToScreen(). In theory adding viewport->Pos to a client position would also be the same.)
                    POINT mouse_pos = mouse_screen_pos;
                    if ( !( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable ) )
                        ::ScreenToClient( bd->hWnd, &mouse_pos );
                    io.AddMousePosEvent( (float)mouse_pos.x, (float)mouse_pos.y );
                }
            }

            // (Optional) When using multiple viewports: call io.AddMouseViewportEvent() with the viewport the OS mouse cursor is hovering.
            // If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, Dear imGui will ignore this field and infer the information using its flawed heuristic.
            // - [X] Win32 backend correctly ignore viewports with the _NoInputs flag (here using ::WindowFromPoint with WM_NCHITTEST + HTTRANSPARENT in WndProc does that)
            //       Some backend are not able to handle that correctly. If a backend report an hovered viewport that has the _NoInputs flag (e.g. when dragging a window
            //       for docking, the viewport has the _NoInputs flag in order to allow us to find the viewport under), then Dear ImGui is forced to ignore the value reported
            //       by the backend, and use its flawed heuristic to guess the viewport behind.
            // - [X] Win32 backend correctly reports this regardless of another viewport behind focused and dragged from (we need this to find a useful drag and drop target).
            ImGuiID mouse_viewport_id = 0;
            if ( has_mouse_screen_pos )
                if ( HWND hovered_hwnd = ::WindowFromPoint( mouse_screen_pos ) )
                    if ( ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle( (void*)hovered_hwnd ) )
                        mouse_viewport_id = viewport->ID;
            io.AddMouseViewportEvent( mouse_viewport_id );
        }

        // See https://learn.microsoft.com/en-us/windows/win32/tablet/system-events-and-mouse-messages
        // Prefer to call this at the top of the message handler to avoid the possibility of other Win32 calls interfering with this.
        static ImGuiMouseSource GetMouseSourceFromMessageExtraInfo()
        {
            LPARAM extra_info = ::GetMessageExtraInfo();
            if ( ( extra_info & 0xFFFFFF80 ) == 0xFF515700 )
                return ImGuiMouseSource_Pen;
            if ( ( extra_info & 0xFFFFFF80 ) == 0xFF515780 )
                return ImGuiMouseSource_TouchScreen;
            return ImGuiMouseSource_Mouse;
        }

        // Window Event Processor
        //-------------------------------------------------------------------------

        // Returns 0 when the message isnt handled, used to embed into another wnd proc
        static LRESULT ImGui_ImplWin32_WndProcHandler( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
        {
            ImGuiIO& io = ImGui::GetIO();
            ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();

            switch ( msg )
            {
                case WM_MOUSEMOVE:
                case WM_NCMOUSEMOVE:
                {
                    // We need to call TrackMouseEvent in order to receive WM_MOUSELEAVE events
                    ImGuiMouseSource mouse_source = GetMouseSourceFromMessageExtraInfo();
                    const int area = ( msg == WM_MOUSEMOVE ) ? 1 : 2;
                    bd->MouseHwnd = hwnd;
                    if ( bd->MouseTrackedArea != area )
                    {
                        TRACKMOUSEEVENT tme_cancel = { sizeof( tme_cancel ), TME_CANCEL, hwnd, 0 };
                        TRACKMOUSEEVENT tme_track = { sizeof( tme_track ), (DWORD)( ( area == 2 ) ? ( TME_LEAVE | TME_NONCLIENT ) : TME_LEAVE ), hwnd, 0 };
                        if ( bd->MouseTrackedArea != 0 )
                            ::TrackMouseEvent( &tme_cancel );
                        ::TrackMouseEvent( &tme_track );
                        bd->MouseTrackedArea = area;
                    }
                    POINT mouse_pos = { (LONG)GET_X_LPARAM( lParam ), (LONG)GET_Y_LPARAM( lParam ) };
                    bool want_absolute_pos = ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable ) != 0;
                    if ( msg == WM_MOUSEMOVE && want_absolute_pos )    // WM_MOUSEMOVE are client-relative coordinates.
                        ::ClientToScreen( hwnd, &mouse_pos );
                    if ( msg == WM_NCMOUSEMOVE && !want_absolute_pos ) // WM_NCMOUSEMOVE are absolute coordinates.
                        ::ScreenToClient( hwnd, &mouse_pos );
                    io.AddMouseSourceEvent( mouse_source );
                    io.AddMousePosEvent( (float)mouse_pos.x, (float)mouse_pos.y );
                    break;
                }
                break;

                case WM_MOUSELEAVE:
                case WM_NCMOUSELEAVE:
                {
                    const int area = ( msg == WM_MOUSELEAVE ) ? 1 : 2;
                    if ( bd->MouseTrackedArea == area )
                    {
                        if ( bd->MouseHwnd == hwnd )
                            bd->MouseHwnd = nullptr;
                        bd->MouseTrackedArea = 0;
                        io.AddMousePosEvent( -FLT_MAX, -FLT_MAX );
                    }
                    break;
                }
                break;

                case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
                case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
                case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
                case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
                {
                    ImGuiMouseSource mouse_source = GetMouseSourceFromMessageExtraInfo();
                    int button = 0;
                    if ( msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK ) { button = 0; }
                    if ( msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK ) { button = 1; }
                    if ( msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK ) { button = 2; }
                    if ( msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK ) { button = ( GET_XBUTTON_WPARAM( wParam ) == XBUTTON1 ) ? 3 : 4; }
                    if ( bd->MouseButtonsDown == 0 && ::GetCapture() == nullptr )
                        ::SetCapture( hwnd );
                    bd->MouseButtonsDown |= 1 << button;
                    io.AddMouseSourceEvent( mouse_source );
                    io.AddMouseButtonEvent( button, true );
                    return 0;
                }
                break;

                case WM_LBUTTONUP:
                case WM_RBUTTONUP:
                case WM_MBUTTONUP:
                case WM_XBUTTONUP:
                {
                    ImGuiMouseSource mouse_source = GetMouseSourceFromMessageExtraInfo();
                    int button = 0;
                    if ( msg == WM_LBUTTONUP ) { button = 0; }
                    if ( msg == WM_RBUTTONUP ) { button = 1; }
                    if ( msg == WM_MBUTTONUP ) { button = 2; }
                    if ( msg == WM_XBUTTONUP ) { button = ( GET_XBUTTON_WPARAM( wParam ) == XBUTTON1 ) ? 3 : 4; }
                    bd->MouseButtonsDown &= ~( 1 << button );
                    if ( bd->MouseButtonsDown == 0 && ::GetCapture() == hwnd )
                        ::ReleaseCapture();
                    io.AddMouseSourceEvent( mouse_source );
                    io.AddMouseButtonEvent( button, false );
                }
                break;

                case WM_MOUSEWHEEL:
                {
                    io.AddMouseWheelEvent( 0.0f, (float) GET_WHEEL_DELTA_WPARAM( wParam ) / (float) WHEEL_DELTA );
                }
                break;

                case WM_MOUSEHWHEEL:
                {
                    io.AddMouseWheelEvent( (float) GET_WHEEL_DELTA_WPARAM( wParam ) / (float) WHEEL_DELTA, 0.0f );
                }
                break;

                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                {
                    const bool is_key_down = ( msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN );
                    if ( wParam < 256 )
                    {
                        // Submit modifiers
                        ImGui_ImplWin32_UpdateKeyModifiers();

                        // Obtain virtual key code
                        // (keypad enter doesn't have its own... VK_RETURN with KF_EXTENDED flag means keypad enter, see IM_VK_KEYPAD_ENTER definition for details, it is mapped to ImGuiKey_KeyPadEnter.)
                        int vk = (int)wParam;
                        if ( ( wParam == VK_RETURN ) && ( HIWORD( lParam ) & KF_EXTENDED ) )
                            vk = IM_VK_KEYPAD_ENTER;

                        // Submit key event
                        const ImGuiKey key = ImGui_ImplWin32_VirtualKeyToImGuiKey( vk );
                        const int scancode = (int)LOBYTE( HIWORD( lParam ) );
                        if ( key != ImGuiKey_None )
                            ImGui_ImplWin32_AddKeyEvent( key, is_key_down, vk, scancode );

                        // Submit individual left/right modifier events
                        if ( vk == VK_SHIFT )
                        {
                            // Important: Shift keys tend to get stuck when pressed together, missing key-up events are corrected in ImGui_ImplWin32_ProcessKeyEventsWorkarounds()
                            if ( IsVkDown( VK_LSHIFT ) == is_key_down ) { ImGui_ImplWin32_AddKeyEvent( ImGuiKey_LeftShift, is_key_down, VK_LSHIFT, scancode ); }
                            if ( IsVkDown( VK_RSHIFT ) == is_key_down ) { ImGui_ImplWin32_AddKeyEvent( ImGuiKey_RightShift, is_key_down, VK_RSHIFT, scancode ); }
                        }
                        else if ( vk == VK_CONTROL )
                        {
                            if ( IsVkDown( VK_LCONTROL ) == is_key_down ) { ImGui_ImplWin32_AddKeyEvent( ImGuiKey_LeftCtrl, is_key_down, VK_LCONTROL, scancode ); }
                            if ( IsVkDown( VK_RCONTROL ) == is_key_down ) { ImGui_ImplWin32_AddKeyEvent( ImGuiKey_RightCtrl, is_key_down, VK_RCONTROL, scancode ); }
                        }
                        else if ( vk == VK_MENU )
                        {
                            if ( IsVkDown( VK_LMENU ) == is_key_down ) { ImGui_ImplWin32_AddKeyEvent( ImGuiKey_LeftAlt, is_key_down, VK_LMENU, scancode ); }
                            if ( IsVkDown( VK_RMENU ) == is_key_down ) { ImGui_ImplWin32_AddKeyEvent( ImGuiKey_RightAlt, is_key_down, VK_RMENU, scancode ); }
                        }
                    }
                }
                break;

                case WM_SETFOCUS:
                case WM_KILLFOCUS:
                {
                    io.AddFocusEvent( msg == WM_SETFOCUS );
                }
                break;

                case WM_CHAR:
                {
                    if ( ::IsWindowUnicode( hwnd ) )
                    {
                        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
                        if ( wParam > 0 && wParam < 0x10000 )
                            io.AddInputCharacterUTF16( (unsigned short)wParam );
                    }
                    else
                    {
                        wchar_t wch = 0;
                        ::MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, (char*)&wParam, 1, &wch, 1 );
                        io.AddInputCharacter( wch );
                    }
                }
                break;

                case WM_SETCURSOR:
                {
                    if ( LOWORD( lParam ) == HTCLIENT && ImGui_ImplWin32_UpdateMouseCursor() )
                    {
                        return 1;
                    }
                }
                break;

                case WM_DISPLAYCHANGE:
                {
                    bd->WantUpdateMonitors = true;
                }
                break;
            }

            // Imgui Viewports
            //-------------------------------------------------------------------------

            if ( ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle( (void*) hwnd ) )
            {
                switch ( msg )
                {
                    case WM_CLOSE:
                    {
                        viewport->PlatformRequestClose = true;
                    }
                    break;

                    case WM_MOVE:
                    {
                        viewport->PlatformRequestMove = true;
                    }
                    break;

                    case WM_SIZE:
                    {
                        viewport->PlatformRequestResize = true;
                    }
                    break;

                    case WM_MOUSEACTIVATE:
                    if ( viewport->Flags & ImGuiViewportFlags_NoFocusOnClick )
                    {
                        return MA_NOACTIVATE;
                    }
                    break;

                    case WM_NCHITTEST:
                    {
                        // Let mouse pass-through the window. This will allow the back-end to set io.MouseHoveredViewport properly (which is OPTIONAL).
                        // The ImGuiViewportFlags_NoInputs flag is set while dragging a viewport, as want to detect the window behind the one we are dragging.
                        // If you cannot easily access those viewport flags from your windowing/event code: you may manually synchronize its state e.g. in
                        // your main loop after calling UpdatePlatformWindows(). Iterate all viewports/platform windows and pass the flag to your windowing system.
                        if ( viewport->Flags & ImGuiViewportFlags_NoInputs )
                        {
                            return HTTRANSPARENT;
                        }
                    }
                    break;
                }
            }

            return 0;
        }

        // Return the default wnd proc if the message isn't handled, needed for child windows
        static LRESULT ImGui_ImplWin32_ChildWndProcHandler( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
        {
            // Store application information in window user data
            if ( msg == WM_NCCREATE )
            {
                auto pUserdata = reinterpret_cast<CREATESTRUCTW*>( lParam )->lpCreateParams;
                ::SetWindowLongPtrW( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( pUserdata ) );
            }

            //-------------------------------------------------------------------------

            LRESULT result = ImGui_ImplWin32_WndProcHandler( hwnd, msg, wParam, lParam );
            if ( result != 0 )
            {
                return result;
            }

            //-------------------------------------------------------------------------

            ImGuiIO& io = ImGui::GetIO();
            ImGui_ImplWin32_Data* pBackendData = reinterpret_cast<ImGui_ImplWin32_Data*>( io.BackendPlatformUserData );
            if ( pBackendData->m_pEngineInputSystem != nullptr )
            {
                switch ( msg )
                {
                    // Forward input messages to the input system
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
                        pBackendData->m_pEngineInputSystem->ForwardInputMessageToInputDevices( { msg, (uintptr_t) wParam, (uintptr_t) lParam } );
                    }
                    break;
                }
            }

            return DefWindowProc( hwnd, msg, wParam, lParam );
        }

        //-------------------------------------------------------------------------

        intptr_t WindowMessageProcessor( HWND hWnd, uint32_t message, uintptr_t wParam, intptr_t lParam )
        {
            return ImGui_ImplWin32_WndProcHandler( hWnd, message, wParam, lParam );
        }
    }

    //-------------------------------------------------------------------------

    void ImguiSystem::InitializePlatform()
    {
        ImGuiIO& io = ImGui::GetIO();

        //-------------------------------------------------------------------------

        auto pBackendData = EE::New<Platform::ImGui_ImplWin32_Data>();

        pBackendData->hWnd = GetActiveWindow();
        pBackendData->WantUpdateHasGamepad = true;
        pBackendData->WantUpdateMonitors = true;
        pBackendData->LastMouseCursor = ImGuiMouseCursor_COUNT;
        pBackendData->m_pEngineInputSystem = m_pInputSystem;

        io.BackendPlatformUserData = pBackendData;
        io.BackendPlatformName = "imgui_impl_win32";

        //-------------------------------------------------------------------------

        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
        io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
        io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)

        ImGuiViewport* pMainViewport = ImGui::GetMainViewport();
        pMainViewport->PlatformHandle = pMainViewport->PlatformHandleRaw = (void*) pBackendData->hWnd;

        // Set up platform IO bindings
        //-------------------------------------------------------------------------

        if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
        {
            Platform::ImGui_ImplWin32_UpdateMonitors();

            ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
            platform_io.Platform_CreateWindow = Platform::ImGui_ImplWin32_CreateWindow;
            platform_io.Platform_DestroyWindow = Platform::ImGui_ImplWin32_DestroyWindow;
            platform_io.Platform_ShowWindow = Platform::ImGui_ImplWin32_ShowWindow;
            platform_io.Platform_SetWindowPos = Platform::ImGui_ImplWin32_SetWindowPos;
            platform_io.Platform_GetWindowPos = Platform::ImGui_ImplWin32_GetWindowPos;
            platform_io.Platform_SetWindowSize = Platform::ImGui_ImplWin32_SetWindowSize;
            platform_io.Platform_GetWindowSize = Platform::ImGui_ImplWin32_GetWindowSize;
            platform_io.Platform_SetWindowFocus = Platform::ImGui_ImplWin32_SetWindowFocus;
            platform_io.Platform_GetWindowFocus = Platform::ImGui_ImplWin32_GetWindowFocus;
            platform_io.Platform_GetWindowMinimized = Platform::ImGui_ImplWin32_GetWindowMinimized;
            platform_io.Platform_SetWindowTitle = Platform::ImGui_ImplWin32_SetWindowTitle;
            platform_io.Platform_SetWindowAlpha = Platform::ImGui_ImplWin32_SetWindowAlpha;
            platform_io.Platform_UpdateWindow = Platform::ImGui_ImplWin32_UpdateWindow;

            pBackendData->m_imguiWindowClass.cbSize = sizeof( WNDCLASSEX );
            pBackendData->m_imguiWindowClass.style = CS_HREDRAW | CS_VREDRAW;
            pBackendData->m_imguiWindowClass.lpfnWndProc = Platform::ImGui_ImplWin32_ChildWndProcHandler;
            pBackendData->m_imguiWindowClass.cbClsExtra = 0;
            pBackendData->m_imguiWindowClass.cbWndExtra = 0;
            pBackendData->m_imguiWindowClass.hInstance = ::GetModuleHandle( NULL );
            pBackendData->m_imguiWindowClass.hIcon = NULL;
            pBackendData->m_imguiWindowClass.hCursor = NULL;
            pBackendData->m_imguiWindowClass.hbrBackground = (HBRUSH) ( COLOR_BACKGROUND + 1 );
            pBackendData->m_imguiWindowClass.lpszMenuName = NULL;
            pBackendData->m_imguiWindowClass.lpszClassName = _T( "ImGui Platform" );
            pBackendData->m_imguiWindowClass.hIconSm = NULL;
            ::RegisterClassEx( &pBackendData->m_imguiWindowClass );

            //auto const errorMessage = EE::Platform::Win32::GetLastErrorMessage();

            //-------------------------------------------------------------------------

            auto pVD = EE::New<Platform::ImGui_ImplWin32_ViewportData>();
            pVD->Hwnd = pBackendData->hWnd;
            pVD->HwndOwned = false;
            pMainViewport->PlatformUserData = pVD;
        }
    }

    void ImguiSystem::ShutdownPlatform()
    {
        Platform::ImGui_ImplWin32_Data* pBackendData = Platform::ImGui_ImplWin32_GetBackendData();

        //-------------------------------------------------------------------------

        ::UnregisterClass( pBackendData->m_imguiWindowClass.lpszClassName, pBackendData->m_imguiWindowClass.hInstance );
        ImGui::DestroyPlatformWindows();

        //-------------------------------------------------------------------------

        ImGuiIO& io = ImGui::GetIO();
        io.BackendPlatformName = nullptr;
        io.BackendPlatformUserData = nullptr;
        EE::Delete( pBackendData );
    }

    void ImguiSystem::PlatformNewFrame()
    {
        ImGuiIO& io = ImGui::GetIO();
        auto pBackendData = reinterpret_cast<Platform::ImGui_ImplWin32_Data*>( io.BackendPlatformUserData );

        //-------------------------------------------------------------------------

        RECT rect = { 0, 0, 0, 0 };
        ::GetClientRect( pBackendData->hWnd, &rect );
        io.DisplaySize = ImVec2( (float)( rect.right - rect.left ), (float)( rect.bottom - rect.top ) );

        if ( pBackendData->WantUpdateMonitors )
        {
            Platform::ImGui_ImplWin32_UpdateMonitors();
        }

        //-------------------------------------------------------------------------

        // Update OS mouse position
        Platform::ImGui_ImplWin32_UpdateMouseData();

        // Process workarounds for known Windows key handling issues
        Platform::ImGui_ImplWin32_ProcessKeyEventsWorkarounds();

        // Update OS mouse cursor with the cursor requested by imgui
        ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
        if ( pBackendData->LastMouseCursor != mouse_cursor )
        {
            pBackendData->LastMouseCursor = mouse_cursor;
            Platform::ImGui_ImplWin32_UpdateMouseCursor();
        }
    }
}
#endif
#endif