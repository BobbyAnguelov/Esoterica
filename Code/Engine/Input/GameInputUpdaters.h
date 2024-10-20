#pragma once

#include "GameInput.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    class InputSystem;

    //-------------------------------------------------------------------------
    // Axes
    //-------------------------------------------------------------------------

    class EE_ENGINE_API GameInputAxisUpdater
    {

    public:

        virtual ~GameInputAxisUpdater() = default;

        virtual bool IsValid() const { return m_sensitivity.m_x > 0 && m_sensitivity.m_y > 0.0f; }
        virtual void Update( InputSystem const* pInputSystem, Seconds timeDelta, Seconds scaledTimeDelta ) = 0;

    public: // Temp for now

        Float2      m_value = Float2::Zero;
        Float2      m_sensitivity = Float2::One;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API MouseAxisUpdater : public GameInputAxisUpdater
    {
    public: // Temp for now

        virtual void Update( InputSystem const* pInputSystem, Seconds timeDelta, Seconds scaledTimeDelta ) override;

    public: // Temp for now

        // Should we invert the Y (up/down) value?
        bool        m_invertY = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ControllerAxisUpdater : public GameInputAxisUpdater
    {
        struct DeadZone
        {
            inline bool IsValid() const { return m_inner >= 0 && m_inner <= 1.0f && m_outer >= 0 && m_outer <= 1.0f; }

            float m_inner = 0.0f;
            float m_outer = 0.0f;
        };

    public: // Temp for now

        virtual bool IsValid() const override;
        virtual void Update( InputSystem const* pInputSystem, Seconds timeDelta, Seconds scaledTimeDelta ) override;

    public: // Temp for now

        // Controller Deadzone
        DeadZone    m_deadZone;

        // Raw controller value
        Float2      m_rawValue = Float2::Zero;

        // Which stick to use?
        bool        m_useLeftStick = true;

        // Should we invert the Y (up/down) value?
        bool        m_invertY = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API KeyboardAxisUpdater : public GameInputAxisUpdater
    {

    public:

        virtual bool IsValid() const override;
        virtual void Update( InputSystem const* pInputSystem, Seconds timeDelta, Seconds scaledTimeDelta ) override;

    public: // Temp for now

        InputID m_up = InputID::None;
        InputID m_down = InputID::None;
        InputID m_left = InputID::None;
        InputID m_right = InputID::None;
    };

    //-------------------------------------------------------------------------
    // Values
    //-------------------------------------------------------------------------

    class EE_ENGINE_API GameInputValueUpdater
    {
    public:

        bool IsValid() const;

        void Update( InputSystem const* pInputSystem, Seconds timeDelta, Seconds scaledTimeDelta );

    public: // Temp for now

        // Input source
        InputID     m_ID = InputID::None;

        // Input Threshold
        float       m_threshold = 0.2f;

        // Value
        float       m_value;
    };

    //-------------------------------------------------------------------------
    // Signals
    //-------------------------------------------------------------------------

    class EE_ENGINE_API GameInputSignalUpdater
    {
    public:

        bool IsValid() const;

        void Update( InputSystem const* pInputSystem, Seconds timeDelta, Seconds scaledTimeDelta );

    public: // Temp for now

        // Input source
        InputID     m_ID = InputID::None;

        // Input Threshold
        float       m_threshold = 0.2f;

        // State
        InputState  m_state;
    };
}