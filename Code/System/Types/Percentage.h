#pragma once

#include "../_Module/API.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/Math/Math.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_SYSTEM_API Percentage
    {
        EE_SERIALIZE( m_value );

    public:

        static Percentage Clamp( Percentage value, bool allowLooping = true );

    public:

        Percentage() = default;

        inline Percentage( float percentage )
            : m_value( percentage )
        {}

        EE_FORCE_INLINE operator float() const { return m_value; }
        EE_FORCE_INLINE float ToFloat() const { return m_value; }
        EE_FORCE_INLINE int32_t GetLoopCount() const { return (int32_t) Math::Floor( m_value ); }

        EE_FORCE_INLINE void Invert() { m_value = 1.0f - m_value; }
        EE_FORCE_INLINE Percentage GetInverse() const { return Percentage( 1.0f - m_value ); }

        EE_FORCE_INLINE Percentage& Clamp( bool allowLooping = true ) { *this = Clamp( *this, allowLooping ); return *this; }
        EE_FORCE_INLINE Percentage GetClamped( bool allowLooping = true ) const { return Clamp( *this, allowLooping ); }

        inline void GetLoopCountAndNormalizedTime( int32_t& loopCount, Percentage& normalizedValue ) const;
        inline Percentage GetNormalizedTime() const;

        EE_FORCE_INLINE Percentage operator+( Percentage const f ) const { return Percentage( m_value + f.m_value ); }
        EE_FORCE_INLINE Percentage operator-( Percentage const f ) const { return Percentage( m_value - f.m_value ); }
        EE_FORCE_INLINE Percentage& operator+=( Percentage const f ) { m_value += f.m_value; return *this; }
        EE_FORCE_INLINE Percentage& operator-=( Percentage const f ) { m_value -= f.m_value; return *this; }
        EE_FORCE_INLINE Percentage operator*( Percentage const f ) const { return Percentage( m_value * f.m_value ); }
        EE_FORCE_INLINE Percentage operator/( Percentage const f ) const { return Percentage( m_value / f.m_value ); }
        EE_FORCE_INLINE Percentage& operator*=( Percentage const f ) { m_value *= f.m_value; return *this; }
        EE_FORCE_INLINE Percentage& operator/=( Percentage const f ) { m_value /= f.m_value; return *this; }

        EE_FORCE_INLINE Percentage operator+( float const f ) const { return Percentage( m_value + f ); }
        EE_FORCE_INLINE Percentage operator-( float const f ) const { return Percentage( m_value - f ); }
        EE_FORCE_INLINE Percentage& operator+=( float const f ) { m_value += f; return *this; }
        EE_FORCE_INLINE Percentage& operator-=( float const f ) { m_value -= f; return *this; }
        EE_FORCE_INLINE Percentage operator*( float const f ) const { return Percentage( m_value * f ); }
        EE_FORCE_INLINE Percentage operator/ ( float const f ) const { return Percentage( m_value / f ); }
        EE_FORCE_INLINE Percentage& operator*=( float const f ) { m_value *= f; return *this; }
        EE_FORCE_INLINE Percentage& operator/=( float const f ) { m_value /= f; return *this; }

    private:

        float m_value = 0.0f;
    };

    //-------------------------------------------------------------------------

    inline void Percentage::GetLoopCountAndNormalizedTime( int32_t& loopCount, Percentage& normalizedValue ) const
    {
        float integerPortion;
        normalizedValue = Percentage( Math::ModF( m_value, &integerPortion ) );
        loopCount = (int32_t) integerPortion;
    }

    inline Percentage Percentage::GetNormalizedTime() const
    {
        float loopCount;
        float normalizedValue = Math::ModF( m_value, &loopCount );
        if ( loopCount > 0 && normalizedValue == 0.0f )
        {
            normalizedValue = 1.0f;
        }

        return Percentage( normalizedValue );
    }
}