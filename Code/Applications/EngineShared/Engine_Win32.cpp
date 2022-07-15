#include "Engine_Win32.h"
#include "Engine.h"
#include "System/Imgui/Platform/ImguiPlatform_win32.h"

//-------------------------------------------------------------------------

namespace EE
{
    intptr_t DefaultEngineWindowProcessor( class Engine* pEngine, HWND__* hWnd, uint32_t message, uintptr_t wParam, intptr_t lParam )
    {
        EE_ASSERT( pEngine != nullptr );

        // ImGui specific message processing
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        auto const imguiResult = ImGuiX::Platform::WindowsMessageHandler( hWnd, message, wParam, lParam );
        if ( imguiResult != 0 )
        {
            return imguiResult;
        }
        #endif

        //-------------------------------------------------------------------------

        switch ( message )
        {
            case WM_SIZE:
            {
                Int2 const newDimensions( LOWORD( lParam ), HIWORD( lParam ) );
                if ( newDimensions.m_x > 0 && newDimensions.m_y > 0 )
                {
                    pEngine->GetRenderingSystem()->ResizePrimaryRenderTarget( newDimensions );

                    // Hack to fix client area offset bug
                    RECT rect;
                    GetWindowRect( hWnd, &rect );
                    MoveWindow( hWnd, rect.left + 1, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE );
                }
            }
            break;

            //-------------------------------------------------------------------------

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
                pEngine->GetInputSystem()->ForwardInputMessageToInputDevices( { message, (uintptr_t) wParam, (uintptr_t) lParam } );
            }
            break;
        }

        // Default
        //-------------------------------------------------------------------------

        return 0;
    }
}