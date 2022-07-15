#pragma once

#include "System/Input/InputState.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    enum class KeyboardButton : uint16_t
    {
        Key_A = 0,
        Key_B,
        Key_C,
        Key_D,
        Key_E,
        Key_F,
        Key_G,
        Key_H,
        Key_I,
        Key_J,
        Key_K,
        Key_L,
        Key_M,
        Key_N,
        Key_O,
        Key_P,
        Key_Q,
        Key_R,
        Key_S,
        Key_T,
        Key_U,
        Key_V,
        Key_W,
        Key_X,
        Key_Y,
        Key_Z,
        Key_0,
        Key_1,
        Key_2,
        Key_3,
        Key_4,
        Key_5,
        Key_6,
        Key_7,
        Key_8,
        Key_9,
        Key_Comma,
        Key_Period,
        Key_ForwardSlash,
        Key_SemiColon,
        Key_Quote,
        Key_LBracket,
        Key_RBracket,
        Key_BackSlash,
        Key_Underscore,
        Key_Minus = Key_Underscore,
        Key_Equals,
        Key_Plus = Key_Equals,
        Key_Backspace,
        Key_Tilde,
        Key_Tab,
        Key_CapsLock,
        Key_Enter,
        Key_Escape,
        Key_Space,
        Key_Left,
        Key_Up,
        Key_Right,
        Key_Down,
        Key_NumLock,
        Key_Numpad0,
        Key_Numpad1,
        Key_Numpad2,
        Key_Numpad3,
        Key_Numpad4,
        Key_Numpad5,
        Key_Numpad6,
        Key_Numpad7,
        Key_Numpad8,
        Key_Numpad9,
        Key_NumpadEnter,
        Key_NumpadMultiply,
        Key_NumpadPlus,
        Key_NumpadMinus,
        Key_NumpadPeriod,
        Key_NumpadDivide,
        Key_F1,
        Key_F2,
        Key_F3,
        Key_F4,
        Key_F5,
        Key_F6,
        Key_F7,
        Key_F8,
        Key_F9,
        Key_F10,
        Key_F11,
        Key_F12,
        Key_F13,
        Key_F14,
        Key_F15,
        Key_PrintScreen,
        Key_ScrollLock,
        Key_Pause,
        Key_Insert,
        Key_Delete,
        Key_Home,
        Key_End,
        Key_PageUp,
        Key_PageDown,
        Key_Application,
        Key_LShift,
        Key_RShift,
        Key_LCtrl,
        Key_RCtrl,
        Key_LAlt,
        Key_RAlt,

        NumButtons,
    };

    //-------------------------------------------------------------------------

    class KeyboardInputState : public ButtonStates<105>
    {
        friend class KeyboardMouseInputDevice;

    public:

        void Clear() { ResetFrameState( ResetType::Full ); }

        void ReflectFrom( Seconds const deltaTime, float timeScale, KeyboardInputState const& sourceState )
        {
            ButtonStates<105>::ReflectFrom( deltaTime, timeScale, sourceState );
        }

        // Get the char key pressed this frame. If no key pressed, this returns 0;
        inline uint8_t GetCharKeyPressed() const { return m_charKeyPressed; }

        // Was the button just pressed (i.e. went from up to down this frame)
        EE_FORCE_INLINE bool WasPressed( KeyboardButton buttonID ) const { return ButtonStates::WasPressed( (uint32_t) buttonID ); }

        // Was the button just release (i.e. went from down to up this frame). Also optionally returns how long the button was held for
        EE_FORCE_INLINE bool WasReleased( KeyboardButton buttonID, Seconds* pHeldDownDuration = nullptr ) const { return ButtonStates::WasReleased( (uint32_t) buttonID, pHeldDownDuration ); }

        // Is the button being held down?
        EE_FORCE_INLINE bool IsHeldDown( KeyboardButton buttonID, Seconds* pHeldDownDuration = nullptr ) const { return ButtonStates::IsHeldDown( (uint32_t) buttonID, pHeldDownDuration ); }

        // How long has the button been held down for?
        EE_FORCE_INLINE Seconds GetHeldDuration( KeyboardButton buttonID ) const { return ButtonStates::GetHeldDuration( (uint32_t) buttonID ); }

        // Syntactic Sugar
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE bool WasEnterPressed() const { return WasPressed( KeyboardButton::Key_Enter ) || WasPressed( KeyboardButton::Key_NumpadEnter ); }
        EE_FORCE_INLINE bool WasEnterReleased( Seconds* pHeldDownDuration = nullptr ) const { return WasReleased( KeyboardButton::Key_Enter, pHeldDownDuration ) || WasReleased( KeyboardButton::Key_NumpadEnter, pHeldDownDuration ); }
        EE_FORCE_INLINE bool IsEnterPressed() const { return IsHeldDown( KeyboardButton::Key_Enter ) || IsHeldDown( KeyboardButton::Key_NumpadEnter ); }
        EE_FORCE_INLINE Seconds GetEnterPressedDuration() const { return Math::Max( GetHeldDuration( KeyboardButton::Key_Enter ), GetHeldDuration( KeyboardButton::Key_NumpadEnter ) ); }

        EE_FORCE_INLINE bool WasShiftPressed() const { return WasPressed( KeyboardButton::Key_LShift ) || WasPressed( KeyboardButton::Key_RShift ); }
        EE_FORCE_INLINE bool WasShiftReleased( Seconds* pHeldDownDuration = nullptr ) const { return WasReleased( KeyboardButton::Key_LShift, pHeldDownDuration ) || WasReleased( KeyboardButton::Key_RShift, pHeldDownDuration ); }
        EE_FORCE_INLINE bool IsShiftHeldDown( Seconds* pHeldDownDuration = nullptr ) const { return IsHeldDown( KeyboardButton::Key_LShift, pHeldDownDuration ) || IsHeldDown( KeyboardButton::Key_RShift, pHeldDownDuration ); }
        EE_FORCE_INLINE Seconds GetShiftPressedDuration() const { return Math::Max( GetHeldDuration( KeyboardButton::Key_LShift ), GetHeldDuration( KeyboardButton::Key_RShift ) ); }

        EE_FORCE_INLINE bool WasCtrlPressed() const { return WasPressed( KeyboardButton::Key_LCtrl ) || WasPressed( KeyboardButton::Key_RCtrl ); }
        EE_FORCE_INLINE bool WasCtrlReleased( Seconds* pHeldDownDuration = nullptr ) const { return WasReleased( KeyboardButton::Key_LCtrl, pHeldDownDuration ) || WasReleased( KeyboardButton::Key_RCtrl, pHeldDownDuration ); }
        EE_FORCE_INLINE bool IsCtrlHeldDown( Seconds* pHeldDownDuration = nullptr ) const { return IsHeldDown( KeyboardButton::Key_LCtrl, pHeldDownDuration ) || IsHeldDown( KeyboardButton::Key_RCtrl, pHeldDownDuration ); }
        EE_FORCE_INLINE Seconds GetCtrlPressedDuration() const { return Math::Max( GetHeldDuration( KeyboardButton::Key_LCtrl ), GetHeldDuration( KeyboardButton::Key_RCtrl ) ); }

        EE_FORCE_INLINE bool WasAltPressed() const { return WasPressed( KeyboardButton::Key_LAlt ) || WasPressed( KeyboardButton::Key_RAlt ); }
        EE_FORCE_INLINE bool WasAltReleased( Seconds* pHeldDownDuration = nullptr ) const { return WasReleased( KeyboardButton::Key_LAlt, pHeldDownDuration ) || WasReleased( KeyboardButton::Key_RAlt, pHeldDownDuration ); }
        EE_FORCE_INLINE bool IsAltHeldDown( Seconds* pHeldDownDuration = nullptr ) const { return IsHeldDown( KeyboardButton::Key_LAlt, pHeldDownDuration ) || IsHeldDown( KeyboardButton::Key_RAlt, pHeldDownDuration ); }
        EE_FORCE_INLINE Seconds GetAltPressedDuration() const { return Math::Max( GetHeldDuration( KeyboardButton::Key_LAlt ), GetHeldDuration( KeyboardButton::Key_RAlt ) ); }

    private:

        EE_FORCE_INLINE void Press( KeyboardButton buttonID ) { ButtonStates::Press( (uint32_t) buttonID ); }
        EE_FORCE_INLINE void Release( KeyboardButton buttonID ) { ButtonStates::Release( (uint32_t) buttonID ); }

        inline void ResetFrameState( ResetType resetType )
        {
            m_charKeyPressed = 0;

            if ( resetType == ResetType::Full )
            {
                ClearButtonState();
            }
        }

    private:

        uint8_t  m_charKeyPressed;
    };
}