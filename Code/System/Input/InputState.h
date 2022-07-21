#pragma once

#include "System\Types\Arrays.h"
#include "System\Time\Time.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    class Button
    {
        template<uint32_t> friend class ButtonStates;

        enum class State : uint8_t
        {
            None = 0,
            Pressed,
            Held,
            Released,
        };

        enum class UpdateState : uint8_t
        {
            None, 
            ChangedThisFrame,
            RequiresStateProgression
        };

    public:

        EE_FORCE_INLINE bool WasPressed() const { return m_state == State::Pressed; }
        EE_FORCE_INLINE bool WasReleased() const { return m_state == State::Released; }
        EE_FORCE_INLINE bool IsHeldDown() const { return m_state == State::Held; }

        EE_FORCE_INLINE Seconds GetTimeHeld() const
        {
            EE_ASSERT( IsHeldDown() || WasPressed() );
            return m_holdTime;
        }

        void Update( Seconds deltaTime )
        {
            // If the state changed this frame, register the button for an update on the next frame
            if ( m_updateState == Button::UpdateState::ChangedThisFrame )
            {
                m_updateState = Button::UpdateState::RequiresStateProgression;
            }
            // Transform the pressed and released events into their subsequent states
            else if ( m_updateState == Button::UpdateState::RequiresStateProgression )
            {
                if ( m_state == Button::State::Pressed )
                {
                    m_state = Button::State::Held;
                    m_holdTime = 0.0f;
                }
                else if ( m_state == Button::State::Released )
                {
                    m_state = Button::State::None;
                    m_holdTime = 0.0f;
                }

                m_updateState = UpdateState::None;
            }

            //-------------------------------------------------------------------------

            // Update hold time
            if ( m_state == Button::State::Held )
            {
                m_holdTime += deltaTime;
            }
        }

    private:

        // Sets the state of the button and flags that the button state has changed
        EE_FORCE_INLINE void SetState( State inState )
        {
            m_state = inState;

            // Track state changes
            if ( inState == State::Pressed )
            {
                m_updateState = UpdateState::ChangedThisFrame;
                m_holdTime = 0.0f;
            }
            else if ( inState == State::Released )
            {
                m_updateState = UpdateState::ChangedThisFrame;
                m_holdTime = 0.0f;
            }
        }

        EE_FORCE_INLINE void ReflectFrom( Seconds deltaTime, Button const& sourceButton )
        {
            // Change button state if needed
            if ( m_state != sourceButton.m_state )
            {
                m_state = sourceButton.m_state;
                m_holdTime = 0.0f;
            }

            // Update hold time
            if ( m_state == Button::State::Held )
            {
                m_holdTime += deltaTime;
            }
        }

    private:

        Seconds             m_holdTime;
        State               m_state = State::None;
        UpdateState         m_updateState = UpdateState::None;
    };

    //-------------------------------------------------------------------------

    template<uint32_t numButtons>
    class ButtonStates
    {
        friend class InputDevice;

    public:

        inline bool WasPressed( uint32_t buttonIdx ) const
        {
            EE_ASSERT( buttonIdx < numButtons );
            return ( m_buttons[buttonIdx].WasPressed() );
        }

        inline bool WasReleased( uint32_t buttonIdx, Seconds* pHeldDownDuration = nullptr ) const
        {
            EE_ASSERT( buttonIdx < numButtons );
            if ( m_buttons[buttonIdx].WasReleased() )
            {
                if ( pHeldDownDuration != nullptr )
                {
                    *pHeldDownDuration = m_buttons[buttonIdx].m_holdTime;
                }

                return true;
            }

            return false;
        }

        inline bool IsHeldDown( uint32_t buttonIdx, Seconds* pHeldDownDuration = nullptr ) const
        {
            if ( pHeldDownDuration != nullptr )
            {
                *pHeldDownDuration = GetHeldDuration( buttonIdx );
            }

            return WasPressed( buttonIdx ) || m_buttons[buttonIdx].IsHeldDown();
        }

        inline Seconds GetHeldDuration( uint32_t buttonIdx ) const
        {
            EE_ASSERT( buttonIdx < numButtons );
            return m_buttons[buttonIdx].IsHeldDown() ? m_buttons[buttonIdx].GetTimeHeld() : Seconds( 0.0f );
        }

    protected:

        inline void ClearButtonState()
        {
            EE::Memory::MemsetZero( m_buttons.data(), m_buttons.size() * sizeof( Button ) );
        }

        inline void Update( Seconds deltaTime )
        {
            for ( auto i = 0; i < numButtons; i++ )
            {
                m_buttons[i].Update( deltaTime );
            }
        }

        // Called when we detect a pressed event for a button
        inline void Press( uint32_t buttonIdx )
        {
            m_buttons[buttonIdx].SetState( Button::State::Pressed );
        }

        // Called when we detect a released event for a button
        inline void Release( uint32_t buttonIdx )
        {
            m_buttons[buttonIdx].SetState( Button::State::Released );
        }

        // Update the state based on a reference state - taking into account the time delta and time scale
        void ReflectFrom( Seconds const deltaTime, float timeScale, ButtonStates<numButtons> const& sourceState )
        {
            Seconds const scaledDeltaTime = deltaTime * Math::Max( 0.0f, timeScale );
            for ( auto i = 0; i < numButtons; i++ )
            {
                auto& button = m_buttons[i];
                auto const& sourceButton = sourceState.m_buttons[i];
                button.ReflectFrom( scaledDeltaTime, sourceButton );
            }
        }

    private:

        TArray<Button, numButtons>   m_buttons;
    };
}