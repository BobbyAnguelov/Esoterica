#pragma once

#include "Engine/_Module/API.h"
#include "Base/Math/Math.h"
#include "Base/Types/Arrays.h"
#include "Base/Input/Input.h"
#include "Base/Time/Time.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    class InputSystem;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API GameInputAxis
    {
        friend class GameInputMap;

    public:

        inline bool HasValue() const { return !m_value.IsNearZero(); }
        inline Float2 const& GetValue() const { return m_value; }
        inline operator bool() const { return HasValue(); }

    public: // Temp for now

        void Clear() { m_value = Float2::Zero; }

    public: // Temp for now

        Float2  m_value = Float2::Zero;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API GameInputValue
    {
        friend class GameInputMap;

    public:

        inline float const& GetValue() const { return m_value; }

    public: // Temp for now

        void Clear() { m_value = 0.0f; }

    public: // Temp for now

        float m_value;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API GameInputSignal
    {
        friend class GameInputMap;

    public:

        inline bool WasPressed() const { return m_state == InputState::Pressed; }
        inline bool IsHeld() const { return m_state == InputState::Pressed || m_state == InputState::Held; }
        inline bool WasReleased() const { return m_state == InputState::Released; }

        inline Seconds GetHoldTime() const { return m_holdTime; }

    public: // Temp for now

        void Clear()
        {
            m_value = 0.0f;
            m_state = InputState::None;
            m_holdTime = 0.0f;
        }

    public: // Temp for now

        float       m_value = 0.0f;
        InputState  m_state = InputState::None;
        Seconds     m_holdTime = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API GameInputMap : public IReflectedType
    {
        EE_REFLECT_TYPE( GameInputMap );

    public:

        virtual ~GameInputMap() = default;

        virtual void RegisterInputs() = 0;
        virtual void UnregisterInputs() final;

        virtual void Update( InputSystem const* pInputSystem, Seconds timeDelta, Seconds scaledTimeDelta ) = 0;

    protected:

        inline void RegisterInput( GameInputAxis& axis )
        {
            EE_ASSERT( !VectorContains( m_axes, &axis ) );
            m_axes.emplace_back( &axis );
        }

        inline void RegisterInput( GameInputValue& value )
        {
            EE_ASSERT( !VectorContains( m_values, &value ) );
            m_values.emplace_back( &value );
        }

        inline void RegisterInput( GameInputSignal& signal )
        {
            EE_ASSERT( !VectorContains( m_signals, &signal ) );
            m_signals.emplace_back( &signal );
        }

    private:

        TInlineVector<GameInputAxis*, 2>        m_axes;
        TInlineVector<GameInputValue*, 2>       m_values;
        TInlineVector<GameInputSignal*, 2>      m_signals;
    };
}