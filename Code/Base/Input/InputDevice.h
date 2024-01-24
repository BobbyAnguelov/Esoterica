#pragma once

#include "InputID.h"
#include "Base/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    // Generic message data for various input messages (e.g. keyboard/mouse messages )
    struct GenericMessage
    {
        uint64_t m_data0;
        uint64_t m_data1;
        uint64_t m_data2;
        uint64_t m_data3;
    };

    //-------------------------------------------------------------------------

    enum class DeviceCategory
    {
        KeyboardMouse,
        Controller
    };

    //-------------------------------------------------------------------------

    class InputDevice
    {

    protected:

        class LogicalInput
        {
            friend class InputDevice;

            enum class State : uint8_t
            {
                None = 0,
                Pressed,
                Held,
                Released,
            };

            enum class Signal
            {
                None = 0,
                Pressed,
                Released,
                Impulse,
            };

        private:

            void Clear() { *this = LogicalInput(); }
            void Update();

        private:

            State           m_state = State::None;
            Signal          m_signal = Signal::None;
            float           m_rawValue = 0.0f;
            float           m_value = 0.0f;
        };

    public:

        virtual ~InputDevice() = default;
        virtual void Initialize() = 0;
        virtual void Shutdown() = 0;

        // Is this device currently connected
        inline bool IsConnected() const { return m_isConnected; }

        // Get the category for this device (controller/mice)
        virtual DeviceCategory GetDeviceCategory() const = 0;

        // Called before we process any input messages
        virtual void PrepareForNewMessages() {};

        // Called by the OS message pump to forward system input events, occurs after the frame end but before the start of the new frame
        virtual void ProcessMessage( GenericMessage const& message ) {}

        // Called at the start of the frame to update the current device state
        virtual void Update( Seconds deltaTime );

        // Called to clear all inputs that this device modifies
        virtual void Clear();

        //-------------------------------------------------------------------------

        inline bool WasPressed( InputID ID ) const
        {
            return ( GetInput( ID ).m_state == LogicalInput::State::Pressed );
        }

        inline bool WasReleased( InputID ID ) const
        {
            return GetInput( ID ).m_state == LogicalInput::State::Released;
        }

        inline bool IsHeldDown( InputID ID ) const
        {
            return WasPressed( ID ) || GetInput( ID ).m_state == LogicalInput::State::Held;
        }

        inline float GetValue( InputID ID ) const
        {
            return GetInput( ID ).m_value;
        }

        inline float GetRawValue( InputID ID ) const
        {
            return GetInput( ID ).m_rawValue;
        }

    protected:

        // Called when we detect a pressed event for a button
        void Press( InputID ID );

        // Called when we detect a pressed event for an analog button
        void Press( InputID ID, float rawValue );

        // Called when we detect a pressed event for an analog button
        void Press( InputID ID, float rawValue, float filteredValue );

        // Called when we detect an impulse event for an input
        void ApplyImpulse( InputID ID, float value );

        // Set a continuous value for an input
        void SetValue( InputID ID, float value );

        // Set a continuous value for an input
        void SetValue( InputID ID, float rawValue, float filteredValue );

        // Called when we detect a released event for a button
        void Release( InputID buttonIdx );

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE LogicalInput const& GetInput( InputID ID ) const { return m_inputs[(int32_t) ID]; }
        EE_FORCE_INLINE LogicalInput& GetInput( InputID ID ) { return m_inputs[(int32_t) ID]; }

    protected:

        TArray<LogicalInput, (size_t) InputID::NumInputs>       m_inputs;
        bool                                                    m_isConnected = false;
    };
}