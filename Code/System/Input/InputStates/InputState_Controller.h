#pragma once

#include "System/Input/InputState.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    enum class ControllerButton : uint16_t
    {
        DPadUp = 0,
        DPadDown,
        DPadLeft,
        DPadRight,
        ThumbstickLeft,
        ThumbstickRight,
        ShoulderLeft,
        ShoulderRight,
        System0,
        System1,
        FaceButtonDown,
        FaceButtonRight,
        FaceButtonLeft,
        FaceButtonUp,
    };

    //-------------------------------------------------------------------------

    class ControllerInputState : public ButtonStates<14>
    {
        friend class ControllerInputDevice;

        enum Direction
        {
            Left = 0,
            Right = 1,
        };

    public:

        ControllerInputState() = default;

        inline void ReflectFrom( Seconds const deltaTime, float timeScale, ControllerInputState const& sourceState )
        {
            m_analogInputRaw[0] = sourceState.m_analogInputRaw[0];
            m_analogInputRaw[1] = sourceState.m_analogInputRaw[1];
            m_analogInputFiltered[0] = sourceState.m_analogInputFiltered[0];
            m_analogInputFiltered[1] = sourceState.m_analogInputFiltered[1];

            m_triggerRaw[0] = sourceState.m_triggerRaw[0];
            m_triggerRaw[1] = sourceState.m_triggerRaw[1];
            m_triggerFiltered[0] = sourceState.m_triggerFiltered[0];
            m_triggerFiltered[1] = sourceState.m_triggerFiltered[1];

            ButtonStates::ReflectFrom( deltaTime, timeScale, sourceState );
        }

        inline void Clear()
        {
            m_analogInputRaw[0] = m_analogInputRaw[1] = Float2::Zero;
            m_analogInputFiltered[0] = m_analogInputFiltered[1] = Float2::Zero;
            m_triggerRaw[0] = m_triggerRaw[1] = 0.0f;
            m_triggerFiltered[0] = m_triggerFiltered[1] = 0.0f;
            ClearButtonState();
        }

        // Get the filtered value of the left analog stick once the deadzone has been applied
        inline Float2 GetLeftAnalogStickValue() const { return m_analogInputFiltered[Left]; }

        // Get the filtered value of the right analog stick once the deadzone has been applied
        inline Float2 GetRightAnalogStickValue() const { return m_analogInputFiltered[Right]; }

        // Get the filtered value of the left trigger once the trigger thresholds has been applied
        inline float GetLeftTriggerValue() const { return m_triggerFiltered[Left]; }

        // Get the filtered value of the right trigger once the trigger thresholds has been applied
        inline float GetRightTriggerValue() const { return m_triggerFiltered[Right]; }

        // Get the raw unfiltered value of the left analog stick
        inline Float2 GetLeftAnalogStickRawValue() const { return m_analogInputRaw[Left]; }

        // Get the raw unfiltered value of the right analog stick
        inline Float2 GetRightAnalogStickRawValue() const { return m_analogInputRaw[Right]; }

        // Get the raw unfiltered value of the left trigger
        inline float GetLeftTriggerRawValue() const { return m_triggerRaw[Left]; }

        // Get the raw unfiltered value of the right trigger
        inline float GetRightTriggerRawValue() const { return m_triggerRaw[Right]; }

        //-------------------------------------------------------------------------

        // Was the button just pressed (i.e. went from up to down this frame)
        EE_FORCE_INLINE bool WasPressed( ControllerButton buttonID ) const { return ButtonStates::WasPressed( (uint32_t) buttonID ); }

        // Was the button just release (i.e. went from down to up this frame). Also optionally returns how long the button was held for
        EE_FORCE_INLINE bool WasReleased( ControllerButton buttonID, Seconds* pHeldDownDuration = nullptr ) const { return ButtonStates::WasReleased( (uint32_t) buttonID, pHeldDownDuration ); }

        // Is the button being held down?
        EE_FORCE_INLINE bool IsHeldDown( ControllerButton buttonID, Seconds* pHeldDownDuration = nullptr ) const { return ButtonStates::IsHeldDown( (uint32_t) buttonID, pHeldDownDuration ); }

        // How long has the button been held down for?
        EE_FORCE_INLINE Seconds GetHeldDuration( ControllerButton buttonID ) const { return ButtonStates::GetHeldDuration( (uint32_t) buttonID ); }

    private:

        EE_FORCE_INLINE void Press( ControllerButton buttonID ) { ButtonStates::Press( (uint32_t) buttonID ); }
        EE_FORCE_INLINE void Release( ControllerButton buttonID ) { ButtonStates::Release( (uint32_t) buttonID ); }

    private:

        Float2                                  m_analogInputRaw[2] = { Float2::Zero, Float2::Zero };
        Float2                                  m_analogInputFiltered[2] = { Float2::Zero, Float2::Zero };
        float                                   m_triggerRaw[2] = { 0.0f, 0.0f };
        float                                   m_triggerFiltered[2] = { 0.0f, 0.0f };
    };
}