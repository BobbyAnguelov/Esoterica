#ifdef _WIN32
#include "Base/Input/InputDevices/InputDevice_KeyboardMouse.h"
#include "Base/Types/HashMap.h"
#include "Base/Platform/Platform.h"
#include <windows.h>

//-------------------------------------------------------------------------

namespace EE::Input
{
    namespace WindowsKeyMap
    {
        // Virtual key code to EE keyboard buttons
        static THashMap<uint32_t, InputID> g_keyMappings;

        enum CustomVKeys
        {
            VK_CUSTOM_NUMPAD_ENTER = 0x00010000,
            VK_CUSTOM_NUMPAD_PERIOD = 0x00020000,
        };

        static bool ConvertKeyMessageToInputID( WPARAM virtualKey, LPARAM lParam, InputID& buttonID )
        {
            uint32_t const scanCode = ( lParam & 0x00ff0000 ) >> 16;
            bool isExtendedBitSet = ( lParam & 0x01000000 ) != 0;

            switch ( virtualKey )
            {
                case VK_SHIFT:
                virtualKey = MapVirtualKey( scanCode, MAPVK_VSC_TO_VK_EX );
                break;

                case VK_CONTROL:
                virtualKey = isExtendedBitSet ? VK_RCONTROL : VK_LCONTROL;
                break;

                case VK_MENU:
                virtualKey = isExtendedBitSet ? VK_RMENU : VK_LMENU;
                break;

                // Ensure that the num pad keys always return the correct VKey irrespective of the num lock state
                //-------------------------------------------------------------------------

                case VK_RETURN:
                if ( isExtendedBitSet ) virtualKey = WindowsKeyMap::VK_CUSTOM_NUMPAD_ENTER;
                break;

                case VK_INSERT:
                if ( !isExtendedBitSet ) virtualKey = VK_NUMPAD0;
                break;

                case VK_DELETE:
                if ( !isExtendedBitSet ) virtualKey = WindowsKeyMap::VK_CUSTOM_NUMPAD_PERIOD;
                break;

                case VK_HOME:
                if ( !isExtendedBitSet ) virtualKey = VK_NUMPAD7;
                break;

                case VK_END:
                if ( !isExtendedBitSet ) virtualKey = VK_NUMPAD1;
                break;

                case VK_PRIOR:
                if ( !isExtendedBitSet ) virtualKey = VK_NUMPAD9;
                break;

                case VK_NEXT:
                if ( !isExtendedBitSet ) virtualKey = VK_NUMPAD3;
                break;

                case VK_LEFT:
                if ( !isExtendedBitSet ) virtualKey = VK_NUMPAD4;
                break;

                case VK_RIGHT:
                if ( !isExtendedBitSet ) virtualKey = VK_NUMPAD6;
                break;

                case VK_UP:
                if ( !isExtendedBitSet ) virtualKey = VK_NUMPAD8;
                break;

                case VK_DOWN:
                if ( !isExtendedBitSet ) virtualKey = VK_NUMPAD9;
                break;

                case VK_CLEAR:
                if ( !isExtendedBitSet ) virtualKey = VK_NUMPAD5;
                break;
            }

            //-------------------------------------------------------------------------

            auto const keyIter = WindowsKeyMap::g_keyMappings.find( (uint32_t) virtualKey );
            if ( keyIter != WindowsKeyMap::g_keyMappings.end() )
            {
                buttonID = keyIter->second;
                return true;
            }

            return false;
        }

        static void Initialize()
        {
            g_keyMappings['A'] = InputID::Keyboard_A;
            g_keyMappings['B'] = InputID::Keyboard_B;
            g_keyMappings['C'] = InputID::Keyboard_C;
            g_keyMappings['D'] = InputID::Keyboard_D;
            g_keyMappings['E'] = InputID::Keyboard_E;
            g_keyMappings['F'] = InputID::Keyboard_F;
            g_keyMappings['G'] = InputID::Keyboard_G;
            g_keyMappings['H'] = InputID::Keyboard_H;
            g_keyMappings['I'] = InputID::Keyboard_I;
            g_keyMappings['J'] = InputID::Keyboard_J;
            g_keyMappings['K'] = InputID::Keyboard_K;
            g_keyMappings['L'] = InputID::Keyboard_L;
            g_keyMappings['M'] = InputID::Keyboard_M;
            g_keyMappings['N'] = InputID::Keyboard_N;
            g_keyMappings['O'] = InputID::Keyboard_O;
            g_keyMappings['P'] = InputID::Keyboard_P;
            g_keyMappings['Q'] = InputID::Keyboard_Q;
            g_keyMappings['R'] = InputID::Keyboard_R;
            g_keyMappings['S'] = InputID::Keyboard_S;
            g_keyMappings['T'] = InputID::Keyboard_T;
            g_keyMappings['U'] = InputID::Keyboard_U;
            g_keyMappings['V'] = InputID::Keyboard_V;
            g_keyMappings['W'] = InputID::Keyboard_W;
            g_keyMappings['X'] = InputID::Keyboard_X;
            g_keyMappings['Y'] = InputID::Keyboard_Y;
            g_keyMappings['Z'] = InputID::Keyboard_Z;
            g_keyMappings['0'] = InputID::Keyboard_0;
            g_keyMappings['1'] = InputID::Keyboard_1;
            g_keyMappings['2'] = InputID::Keyboard_2;
            g_keyMappings['3'] = InputID::Keyboard_3;
            g_keyMappings['4'] = InputID::Keyboard_4;
            g_keyMappings['5'] = InputID::Keyboard_5;
            g_keyMappings['6'] = InputID::Keyboard_6;
            g_keyMappings['7'] = InputID::Keyboard_7;
            g_keyMappings['8'] = InputID::Keyboard_8;
            g_keyMappings['9'] = InputID::Keyboard_9;
            g_keyMappings[VK_OEM_COMMA] = InputID::Keyboard_Comma;
            g_keyMappings[VK_OEM_PERIOD] = InputID::Keyboard_Period;
            g_keyMappings[VK_OEM_2] = InputID::Keyboard_ForwardSlash;
            g_keyMappings[VK_OEM_1] = InputID::Keyboard_SemiColon;
            g_keyMappings[VK_OEM_7] = InputID::Keyboard_Quote;
            g_keyMappings[VK_OEM_4] = InputID::Keyboard_LBracket;
            g_keyMappings[VK_OEM_6] = InputID::Keyboard_RBracket;
            g_keyMappings[VK_OEM_5] = InputID::Keyboard_BackSlash;
            g_keyMappings[VK_OEM_MINUS] = InputID::Keyboard_Minus;
            g_keyMappings[VK_OEM_PLUS] = InputID::Keyboard_Equals;
            g_keyMappings[VK_BACK] = InputID::Keyboard_Backspace;
            g_keyMappings[VK_OEM_3] = InputID::Keyboard_Tilde;
            g_keyMappings[VK_TAB] = InputID::Keyboard_Tab;
            g_keyMappings[VK_CAPITAL] = InputID::Keyboard_CapsLock;
            g_keyMappings[VK_RETURN] = InputID::Keyboard_Enter;
            g_keyMappings[VK_ESCAPE] = InputID::Keyboard_Escape;
            g_keyMappings[VK_SPACE] = InputID::Keyboard_Space;
            g_keyMappings[VK_LEFT] = InputID::Keyboard_Left;
            g_keyMappings[VK_UP] = InputID::Keyboard_Up;
            g_keyMappings[VK_RIGHT] = InputID::Keyboard_Right;
            g_keyMappings[VK_DOWN] = InputID::Keyboard_Down;
            g_keyMappings[VK_NUMLOCK] = InputID::Keyboard_NumLock;
            g_keyMappings[VK_NUMPAD0] = InputID::Keyboard_Numpad0;
            g_keyMappings[VK_NUMPAD1] = InputID::Keyboard_Numpad1;
            g_keyMappings[VK_NUMPAD2] = InputID::Keyboard_Numpad2;
            g_keyMappings[VK_NUMPAD3] = InputID::Keyboard_Numpad3;
            g_keyMappings[VK_NUMPAD4] = InputID::Keyboard_Numpad4;
            g_keyMappings[VK_NUMPAD5] = InputID::Keyboard_Numpad5;
            g_keyMappings[VK_NUMPAD6] = InputID::Keyboard_Numpad6;
            g_keyMappings[VK_NUMPAD7] = InputID::Keyboard_Numpad7;
            g_keyMappings[VK_NUMPAD8] = InputID::Keyboard_Numpad8;
            g_keyMappings[VK_NUMPAD9] = InputID::Keyboard_Numpad9;
            g_keyMappings[VK_CUSTOM_NUMPAD_ENTER] = InputID::Keyboard_NumpadEnter;
            g_keyMappings[VK_MULTIPLY] = InputID::Keyboard_NumpadMultiply;
            g_keyMappings[VK_ADD] = InputID::Keyboard_NumpadPlus;
            g_keyMappings[VK_SUBTRACT] = InputID::Keyboard_NumpadMinus;
            g_keyMappings[VK_CUSTOM_NUMPAD_PERIOD] = InputID::Keyboard_NumpadPeriod;
            g_keyMappings[VK_DIVIDE] = InputID::Keyboard_NumpadDivide;
            g_keyMappings[VK_F1] = InputID::Keyboard_F1;
            g_keyMappings[VK_F2] = InputID::Keyboard_F2;
            g_keyMappings[VK_F3] = InputID::Keyboard_F3;
            g_keyMappings[VK_F4] = InputID::Keyboard_F4;
            g_keyMappings[VK_F5] = InputID::Keyboard_F5;
            g_keyMappings[VK_F6] = InputID::Keyboard_F6;
            g_keyMappings[VK_F7] = InputID::Keyboard_F7;
            g_keyMappings[VK_F8] = InputID::Keyboard_F8;
            g_keyMappings[VK_F9] = InputID::Keyboard_F9;
            g_keyMappings[VK_F10] = InputID::Keyboard_F10;
            g_keyMappings[VK_F11] = InputID::Keyboard_F11;
            g_keyMappings[VK_F12] = InputID::Keyboard_F12;
            g_keyMappings[VK_F13] = InputID::Keyboard_F13;
            g_keyMappings[VK_F14] = InputID::Keyboard_F14;
            g_keyMappings[VK_F15] = InputID::Keyboard_F15;
            g_keyMappings[VK_SNAPSHOT] = InputID::Keyboard_PrintScreen;
            g_keyMappings[VK_SCROLL] = InputID::Keyboard_ScrollLock;
            g_keyMappings[VK_PAUSE] = InputID::Keyboard_Pause;
            g_keyMappings[VK_INSERT] = InputID::Keyboard_Insert;
            g_keyMappings[VK_DELETE] = InputID::Keyboard_Delete;
            g_keyMappings[VK_HOME] = InputID::Keyboard_Home;
            g_keyMappings[VK_END] = InputID::Keyboard_End;
            g_keyMappings[VK_PRIOR] = InputID::Keyboard_PageUp;
            g_keyMappings[VK_NEXT] = InputID::Keyboard_PageDown;
            g_keyMappings[VK_APPS] = InputID::Keyboard_Application;
            g_keyMappings[VK_LSHIFT] = InputID::Keyboard_LShift;
            g_keyMappings[VK_RSHIFT] = InputID::Keyboard_RShift;
            g_keyMappings[VK_LCONTROL] = InputID::Keyboard_LCtrl;
            g_keyMappings[VK_RCONTROL] = InputID::Keyboard_RCtrl;
            g_keyMappings[VK_LMENU] = InputID::Keyboard_LAlt;
            g_keyMappings[VK_RMENU] = InputID::Keyboard_RAlt;
        }

        static void Shutdown()
        {
            g_keyMappings.clear( true );
        }
    }

    //-------------------------------------------------------------------------

    void KeyboardMouseDevice::Initialize()
    {
        RAWINPUTDEVICE inputDevices[2];

        // Mouse
        inputDevices[0].usUsagePage = 0x01;
        inputDevices[0].usUsage = 0x02;
        inputDevices[0].dwFlags = 0;
        inputDevices[0].hwndTarget = (HWND) Platform::GetMainWindowHandle();

        if ( !RegisterRawInputDevices( inputDevices, 1, sizeof( RAWINPUTDEVICE ) ) )
        {
            EE_LOG_ERROR( "Input", nullptr, "Raw input device registration failed" );
        }

        WindowsKeyMap::Initialize();
    }

    void KeyboardMouseDevice::Shutdown()
    {
        WindowsKeyMap::Shutdown();
    }

    void KeyboardMouseDevice::ProcessMessage( GenericMessage const& msg )
    {
        uint64_t const messageID = msg.m_data0;
        uintptr_t const paramW = msg.m_data1;
        uintptr_t const paramL = msg.m_data2;

        // Focus handling
        //-------------------------------------------------------------------------

        if ( messageID == WM_SETFOCUS || messageID == WM_KILLFOCUS )
        {
            Clear();
        }

        // Record char inputs
        //-------------------------------------------------------------------------

        if ( messageID == WM_CHAR )
        {
            if ( paramW > 0 && paramW < 0x10000 )
            {
                m_charKeyPressed = (char) paramW;
            }
        }

        // Mouse absolute positioning
        //-------------------------------------------------------------------------

        else if ( messageID == WM_MOUSEMOVE )
        {
            POINTS pt;
            pt = MAKEPOINTS( paramL );
            m_position.m_x = pt.x;
            m_position.m_y = pt.y;
        }

        // Mouse Input
        //-------------------------------------------------------------------------

        else if ( messageID == WM_INPUT )
        {
            uint32_t dbSize = 0;
            GetRawInputData( (HRAWINPUT) paramL, RID_INPUT, nullptr, &dbSize, sizeof( RAWINPUTHEADER ) );

            // Allocate stack storage for the input message
            auto pInputData = EE_STACK_ARRAY_ALLOC( uint8_t, dbSize );
            Memory::MemsetZero( pInputData, dbSize );

            // Get raw input data buffer size
            GetRawInputData( (HRAWINPUT) paramL, RID_INPUT, pInputData, &dbSize, sizeof( RAWINPUTHEADER ) );
            auto pRawInputData = reinterpret_cast<RAWINPUT*>( pInputData );

            if ( pRawInputData->header.dwType == RIM_TYPEMOUSE )
            {
                RAWMOUSE const& rawMouse = pRawInputData->data.mouse;

                // Mouse Movement
                m_movementDelta.m_x += (float) rawMouse.lLastX * m_sensitivity.m_x;
                m_movementDelta.m_y += (float) rawMouse.lLastY * m_sensitivity.m_y;

                // Mouse button state
                if ( rawMouse.usButtonFlags != 0 )
                {
                    // Vertical Wheel
                    //-------------------------------------------------------------------------

                    if ( rawMouse.usButtonFlags & RI_MOUSE_WHEEL )
                    {
                        float const wheelDelta = ( (float) (int16_t) rawMouse.usButtonData ) / WHEEL_DELTA;
                        ApplyImpulse( InputID::Mouse_WheelVertical, wheelDelta );
                    }

                    // Horizontal Wheel
                    //-------------------------------------------------------------------------

                    if ( rawMouse.usButtonFlags & RI_MOUSE_HWHEEL )
                    {
                        float const wheelDelta = ( (float) (int16_t) rawMouse.usButtonData ) / WHEEL_DELTA;
                        ApplyImpulse( InputID::Mouse_WheelHorizontal, (float) wheelDelta );
                    }

                    // Mouse Buttons
                    //-------------------------------------------------------------------------

                    if ( rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP )
                    {
                        Release( InputID::Mouse_Left );
                    }

                    if ( rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN )
                    {
                        Press( InputID::Mouse_Left );
                    }

                    if ( rawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP )
                    {
                        Release( InputID::Mouse_Right );
                    }

                    if ( rawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN )
                    {
                        Press( InputID::Mouse_Right );
                    }

                    if ( rawMouse.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_UP )
                    {
                        Release( InputID::Mouse_Middle );
                    }

                    if ( rawMouse.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_DOWN )
                    {
                        Press( InputID::Mouse_Middle );
                    }

                    if ( rawMouse.usButtonFlags == RI_MOUSE_BUTTON_4_UP )
                    {
                        Release( InputID::Mouse_Button4 );
                    }

                    if ( rawMouse.usButtonFlags == RI_MOUSE_BUTTON_4_DOWN )
                    {
                        Press( InputID::Mouse_Button4 );
                    }

                    if ( rawMouse.usButtonFlags == RI_MOUSE_BUTTON_5_UP )
                    {
                        Release( InputID::Mouse_Button5 );
                    }

                    if ( rawMouse.usButtonFlags == RI_MOUSE_BUTTON_5_DOWN )
                    {
                        Press( InputID::Mouse_Button5 );
                    }
                }
            }
        }

        // Keyboard
        //-------------------------------------------------------------------------

        else if ( messageID == WM_KEYDOWN || messageID == WM_KEYUP || messageID == WM_SYSKEYDOWN || messageID == WM_SYSKEYUP )
        {
            InputID inputID;
            if ( WindowsKeyMap::ConvertKeyMessageToInputID( paramW, paramL, inputID ) )
            {
                if ( messageID == WM_KEYDOWN || messageID == WM_SYSKEYDOWN )
                {
                    Press( inputID );
                }
                else if ( messageID == WM_KEYUP || messageID == WM_SYSKEYUP )
                {
                    Release( inputID );
                }
            }
        }
    }

    void KeyboardMouseDevice::Update( Seconds deltaTime )
    {
        SetValue( InputID::Mouse_DeltaMovementHorizontal, m_movementDelta.m_x );
        SetValue( InputID::Mouse_DeltaMovementVertical, m_movementDelta.m_y );
        InputDevice::Update( deltaTime );
    }
}
#endif