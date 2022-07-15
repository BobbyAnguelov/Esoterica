#ifdef _WIN32
#include "System/Input/InputDevices/InputDevice_KeyboardMouse.h"
#include "System/Math/Vector.h"
#include "System/Types/Arrays.h"
#include "System/Types/HashMap.h"
#include "System/Log.h"
#include <windows.h>
#include <XInput.h>

//-------------------------------------------------------------------------

namespace EE
{
    namespace Input
    {
        namespace WindowsKeyMap
        {
            // Virtual key code to EE keyboard buttons
            static THashMap<uint32_t, KeyboardButton> g_keyMappings;

            enum CustomVKeys
            {
                VK_CUSTOM_NUMPAD_ENTER = 0x00010000,
                VK_CUSTOM_NUMPAD_PERIOD = 0x00020000,
            };

            static bool GetButtonForKeyMessage( WPARAM virtualKey, LPARAM lParam, KeyboardButton& buttonID )
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
                g_keyMappings['A'] = KeyboardButton::Key_A;
                g_keyMappings['B'] = KeyboardButton::Key_B;
                g_keyMappings['C'] = KeyboardButton::Key_C;
                g_keyMappings['D'] = KeyboardButton::Key_D;
                g_keyMappings['E'] = KeyboardButton::Key_E;
                g_keyMappings['F'] = KeyboardButton::Key_F;
                g_keyMappings['G'] = KeyboardButton::Key_G;
                g_keyMappings['H'] = KeyboardButton::Key_H;
                g_keyMappings['I'] = KeyboardButton::Key_I;
                g_keyMappings['J'] = KeyboardButton::Key_J;
                g_keyMappings['K'] = KeyboardButton::Key_K;
                g_keyMappings['L'] = KeyboardButton::Key_L;
                g_keyMappings['M'] = KeyboardButton::Key_M;
                g_keyMappings['N'] = KeyboardButton::Key_N;
                g_keyMappings['O'] = KeyboardButton::Key_O;
                g_keyMappings['P'] = KeyboardButton::Key_P;
                g_keyMappings['Q'] = KeyboardButton::Key_Q;
                g_keyMappings['R'] = KeyboardButton::Key_R;
                g_keyMappings['S'] = KeyboardButton::Key_S;
                g_keyMappings['T'] = KeyboardButton::Key_T;
                g_keyMappings['U'] = KeyboardButton::Key_U;
                g_keyMappings['V'] = KeyboardButton::Key_V;
                g_keyMappings['W'] = KeyboardButton::Key_W;
                g_keyMappings['X'] = KeyboardButton::Key_X;
                g_keyMappings['Y'] = KeyboardButton::Key_Y;
                g_keyMappings['Z'] = KeyboardButton::Key_Z;
                g_keyMappings['0'] = KeyboardButton::Key_0;
                g_keyMappings['1'] = KeyboardButton::Key_1;
                g_keyMappings['2'] = KeyboardButton::Key_2;
                g_keyMappings['3'] = KeyboardButton::Key_3;
                g_keyMappings['4'] = KeyboardButton::Key_4;
                g_keyMappings['5'] = KeyboardButton::Key_5;
                g_keyMappings['6'] = KeyboardButton::Key_6;
                g_keyMappings['7'] = KeyboardButton::Key_7;
                g_keyMappings['8'] = KeyboardButton::Key_8;
                g_keyMappings['9'] = KeyboardButton::Key_9;
                g_keyMappings[VK_OEM_COMMA] = KeyboardButton::Key_Comma;
                g_keyMappings[VK_OEM_PERIOD] = KeyboardButton::Key_Period;
                g_keyMappings[VK_OEM_2] = KeyboardButton::Key_ForwardSlash;
                g_keyMappings[VK_OEM_1] = KeyboardButton::Key_SemiColon;
                g_keyMappings[VK_OEM_7] = KeyboardButton::Key_Quote;
                g_keyMappings[VK_OEM_4] = KeyboardButton::Key_LBracket;
                g_keyMappings[VK_OEM_6] = KeyboardButton::Key_RBracket;
                g_keyMappings[VK_OEM_5] = KeyboardButton::Key_BackSlash;
                g_keyMappings[VK_OEM_MINUS] = KeyboardButton::Key_Minus;
                g_keyMappings[VK_OEM_PLUS] = KeyboardButton::Key_Equals;
                g_keyMappings[VK_BACK] = KeyboardButton::Key_Backspace;
                g_keyMappings[VK_OEM_3] = KeyboardButton::Key_Tilde;
                g_keyMappings[VK_TAB] = KeyboardButton::Key_Tab;
                g_keyMappings[VK_CAPITAL] = KeyboardButton::Key_CapsLock;
                g_keyMappings[VK_RETURN] = KeyboardButton::Key_Enter;
                g_keyMappings[VK_ESCAPE] = KeyboardButton::Key_Escape;
                g_keyMappings[VK_SPACE] = KeyboardButton::Key_Space;
                g_keyMappings[VK_LEFT] = KeyboardButton::Key_Left;
                g_keyMappings[VK_UP] = KeyboardButton::Key_Up;
                g_keyMappings[VK_RIGHT] = KeyboardButton::Key_Right;
                g_keyMappings[VK_DOWN] = KeyboardButton::Key_Down;
                g_keyMappings[VK_NUMLOCK] = KeyboardButton::Key_NumLock;
                g_keyMappings[VK_NUMPAD0] = KeyboardButton::Key_Numpad0;
                g_keyMappings[VK_NUMPAD1] = KeyboardButton::Key_Numpad1;
                g_keyMappings[VK_NUMPAD2] = KeyboardButton::Key_Numpad2;
                g_keyMappings[VK_NUMPAD3] = KeyboardButton::Key_Numpad3;
                g_keyMappings[VK_NUMPAD4] = KeyboardButton::Key_Numpad4;
                g_keyMappings[VK_NUMPAD5] = KeyboardButton::Key_Numpad5;
                g_keyMappings[VK_NUMPAD6] = KeyboardButton::Key_Numpad6;
                g_keyMappings[VK_NUMPAD7] = KeyboardButton::Key_Numpad7;
                g_keyMappings[VK_NUMPAD8] = KeyboardButton::Key_Numpad8;
                g_keyMappings[VK_NUMPAD9] = KeyboardButton::Key_Numpad9;
                g_keyMappings[VK_CUSTOM_NUMPAD_ENTER] = KeyboardButton::Key_NumpadEnter;
                g_keyMappings[VK_MULTIPLY] = KeyboardButton::Key_NumpadMultiply;
                g_keyMappings[VK_ADD] = KeyboardButton::Key_NumpadPlus;
                g_keyMappings[VK_SUBTRACT] = KeyboardButton::Key_NumpadMinus;
                g_keyMappings[VK_CUSTOM_NUMPAD_PERIOD] = KeyboardButton::Key_NumpadPeriod;
                g_keyMappings[VK_DIVIDE] = KeyboardButton::Key_NumpadDivide;
                g_keyMappings[VK_F1] = KeyboardButton::Key_F1;
                g_keyMappings[VK_F2] = KeyboardButton::Key_F2;
                g_keyMappings[VK_F3] = KeyboardButton::Key_F3;
                g_keyMappings[VK_F4] = KeyboardButton::Key_F4;
                g_keyMappings[VK_F5] = KeyboardButton::Key_F5;
                g_keyMappings[VK_F6] = KeyboardButton::Key_F6;
                g_keyMappings[VK_F7] = KeyboardButton::Key_F7;
                g_keyMappings[VK_F8] = KeyboardButton::Key_F8;
                g_keyMappings[VK_F9] = KeyboardButton::Key_F9;
                g_keyMappings[VK_F10] = KeyboardButton::Key_F10;
                g_keyMappings[VK_F11] = KeyboardButton::Key_F11;
                g_keyMappings[VK_F12] = KeyboardButton::Key_F12;
                g_keyMappings[VK_F13] = KeyboardButton::Key_F13;
                g_keyMappings[VK_F14] = KeyboardButton::Key_F14;
                g_keyMappings[VK_F15] = KeyboardButton::Key_F15;
                g_keyMappings[VK_SNAPSHOT] = KeyboardButton::Key_PrintScreen;
                g_keyMappings[VK_SCROLL] = KeyboardButton::Key_ScrollLock;
                g_keyMappings[VK_PAUSE] = KeyboardButton::Key_Pause;
                g_keyMappings[VK_INSERT] = KeyboardButton::Key_Insert;
                g_keyMappings[VK_DELETE] = KeyboardButton::Key_Delete;
                g_keyMappings[VK_HOME] = KeyboardButton::Key_Home;
                g_keyMappings[VK_END] = KeyboardButton::Key_End;
                g_keyMappings[VK_PRIOR] = KeyboardButton::Key_PageUp;
                g_keyMappings[VK_NEXT] = KeyboardButton::Key_PageDown;
                g_keyMappings[VK_APPS] = KeyboardButton::Key_Application;
                g_keyMappings[VK_LSHIFT] = KeyboardButton::Key_LShift;
                g_keyMappings[VK_RSHIFT] = KeyboardButton::Key_RShift;
                g_keyMappings[VK_LCONTROL] = KeyboardButton::Key_LCtrl;
                g_keyMappings[VK_RCONTROL] = KeyboardButton::Key_RCtrl;
                g_keyMappings[VK_LMENU] = KeyboardButton::Key_LAlt;
                g_keyMappings[VK_RMENU] = KeyboardButton::Key_RAlt;
            }

            static void Shutdown()
            {
                g_keyMappings.clear( true );
            }
        }

        //-------------------------------------------------------------------------

        void KeyboardMouseInputDevice::Initialize()
        {
            RAWINPUTDEVICE inputDevices[2];

            HWND activeWindow = GetActiveWindow();

            // Mouse
            inputDevices[0].usUsagePage = 0x01;
            inputDevices[0].usUsage = 0x02;
            inputDevices[0].dwFlags = 0;
            inputDevices[0].hwndTarget = activeWindow;

            if ( !RegisterRawInputDevices( inputDevices, 1, sizeof( RAWINPUTDEVICE ) ) )
            {
                EE_LOG_ERROR( "Input", "Raw input device registration failed" );
            }

            WindowsKeyMap::Initialize();
        }

        void KeyboardMouseInputDevice::Shutdown()
        {
            WindowsKeyMap::Shutdown();
        }

        void KeyboardMouseInputDevice::ProcessMessage( GenericMessage const& msg )
        {
            uint64_t const messageID = msg.m_data0;
            uintptr_t const paramW = msg.m_data1;
            uintptr_t const paramL = msg.m_data2;

            // Focus handling
            //-------------------------------------------------------------------------

            if ( messageID == WM_SETFOCUS || messageID == WM_KILLFOCUS )
            {
                m_mouseState.ClearButtonState();
                m_keyboardState.ClearButtonState();
            }

            // Record char inputs
            //-------------------------------------------------------------------------

            if ( messageID == WM_CHAR )
            {
                if ( paramW > 0 && paramW < 0x10000 )
                {
                    m_keyboardState.m_charKeyPressed = (char) paramW;
                }
            }

            // Mouse absolute positioning
            //-------------------------------------------------------------------------

            else if ( messageID == WM_MOUSEMOVE )
            {
                POINTS pt;
                pt = MAKEPOINTS( paramL );
                m_mouseState.m_position.m_x = pt.x;
                m_mouseState.m_position.m_y = pt.y;
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
                    m_mouseState.m_movementDelta.m_x += (float) rawMouse.lLastX;
                    m_mouseState.m_movementDelta.m_y += (float) rawMouse.lLastY;

                    // Mouse button state
                    if ( rawMouse.usButtonFlags != 0 )
                    {
                        if ( rawMouse.usButtonFlags & RI_MOUSE_WHEEL )
                        {
                            int16_t wheelDelta = ( (int16_t) rawMouse.usButtonData ) / WHEEL_DELTA;
                            m_mouseState.m_verticalWheelDelta = wheelDelta;
                        }

                        if ( rawMouse.usButtonFlags & RI_MOUSE_HWHEEL )
                        {
                            int16_t wheelDelta = ( (int16_t) rawMouse.usButtonData ) / WHEEL_DELTA;
                            m_mouseState.m_horizontalWheelDelta = wheelDelta;
                        }

                        if ( rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP )
                        {
                            m_mouseState.Release( MouseButton::Left );
                        }

                        if ( rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN )
                        {
                            m_mouseState.Press( MouseButton::Left );
                        }

                        if ( rawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP )
                        {
                            m_mouseState.Release( MouseButton::Right );
                        }

                        if ( rawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN )
                        {
                            m_mouseState.Press( MouseButton::Right );
                        }

                        if ( rawMouse.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_UP )
                        {
                            m_mouseState.Release( MouseButton::Middle );
                        }

                        if ( rawMouse.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_DOWN )
                        {
                            m_mouseState.Press( MouseButton::Middle );
                        }

                        if ( rawMouse.usButtonFlags == RI_MOUSE_BUTTON_4_UP )
                        {
                            m_mouseState.Release( MouseButton::Back );
                        }

                        if ( rawMouse.usButtonFlags == RI_MOUSE_BUTTON_4_DOWN )
                        {
                            m_mouseState.Press( MouseButton::Back );
                        }

                        if ( rawMouse.usButtonFlags == RI_MOUSE_BUTTON_5_UP )
                        {
                            m_mouseState.Release( MouseButton::Forward );
                        }

                        if ( rawMouse.usButtonFlags == RI_MOUSE_BUTTON_5_DOWN )
                        {
                            m_mouseState.Press( MouseButton::Forward );
                        }
                    }
                }
            }

            // Keyboard
            //-------------------------------------------------------------------------

            else if ( messageID == WM_KEYDOWN || messageID == WM_KEYUP || messageID == WM_SYSKEYDOWN || messageID == WM_SYSKEYUP )
            {
                KeyboardButton buttonID;
                if ( WindowsKeyMap::GetButtonForKeyMessage( paramW, paramL, buttonID ) )
                {
                    if ( messageID == WM_KEYDOWN || messageID == WM_SYSKEYDOWN )
                    {
                        m_keyboardState.Press( buttonID );
                    }
                    else if ( messageID == WM_KEYUP|| messageID == WM_SYSKEYUP )
                    {
                        m_keyboardState.Release( buttonID );
                    }
                }
            }
        }
    }
}

#endif