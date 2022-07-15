#pragma once

#include "System/_Module/API.h"
#include "System/Math/Math.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE
{
    //-------------------------------------------------------------------------
    // Forward Declarations
    //-------------------------------------------------------------------------

    class Nanoseconds;
    class Milliseconds;
    class Seconds;

    //-------------------------------------------------------------------------
    // Data Types
    //-------------------------------------------------------------------------

    class Microseconds
    {
        friend Milliseconds;
        friend Seconds;
        EE_SERIALIZE( m_value );

    public:

        Microseconds() : m_value(0) {}
        Microseconds( float time ) : m_value( time ) {}
        inline explicit Microseconds( Nanoseconds time );
        inline explicit Microseconds( Milliseconds time );
        inline explicit Microseconds( Seconds time );

        inline Microseconds& operator= ( float time ) { m_value = time; return *this; }
        inline Microseconds& operator= ( Milliseconds time );
        inline Microseconds& operator= ( Seconds time );

        inline operator float() const { return m_value; }
        inline float ToFloat() const { return m_value; }
        inline explicit operator Nanoseconds() const;
        inline explicit operator Milliseconds() const;
        inline explicit operator Seconds() const;

        inline Nanoseconds ToNanoseconds() const;
        inline Milliseconds ToMilliseconds() const;
        inline Seconds ToSeconds() const;

        inline Microseconds operator+ ( float const time ) const;
        inline Microseconds operator+ ( Microseconds const time )  const;
        inline Microseconds operator+ ( Milliseconds const time ) const;
        inline Microseconds operator+ ( Seconds const time )  const;

        inline Microseconds operator- ( float const time )  const;
        inline Microseconds operator- ( Microseconds const time ) const;
        inline Microseconds operator- ( Milliseconds const time ) const;
        inline Microseconds operator- ( Seconds const time ) const;

        inline Microseconds& operator+= ( float const time );
        inline Microseconds& operator+= ( Microseconds const time );
        inline Microseconds& operator+= ( Milliseconds const time );
        inline Microseconds& operator+= ( Seconds const time );

        inline Microseconds& operator-= ( float const time );
        inline Microseconds& operator-= ( Microseconds const time );
        inline Microseconds& operator-= ( Milliseconds const time );
        inline Microseconds& operator-= ( Seconds const time );

    private:

        float m_value;
    };

    //-------------------------------------------------------------------------

    class Milliseconds
    {
        friend Microseconds;
        friend Seconds;
        EE_SERIALIZE( m_value );

    public:

        Milliseconds() : m_value( 0 ) {}
        Milliseconds( float time ) : m_value( time ) {}
        inline explicit Milliseconds( Nanoseconds time );
        inline explicit Milliseconds( Microseconds time );
        inline explicit Milliseconds( Seconds time );

        inline Milliseconds& operator= ( float time ) { m_value = time; return *this; }
        inline Milliseconds& operator= ( Microseconds time );
        inline Milliseconds& operator= ( Seconds time );

        inline operator float() const { return m_value; }
        inline float ToFloat() const { return m_value; }
        inline explicit operator Nanoseconds() const;
        inline explicit operator Microseconds() const;
        inline explicit operator Seconds() const;

        inline Nanoseconds ToNanoseconds() const;
        inline Microseconds ToMicroseconds() const;
        inline Seconds ToSeconds() const;

        inline Milliseconds operator+ ( float const time ) const;
        inline Milliseconds operator+ ( Milliseconds const time ) const;
        inline Milliseconds operator+ ( Microseconds const time ) const;
        inline Milliseconds operator+ ( Seconds const time ) const;

        inline Milliseconds operator- ( float const time ) const;
        inline Milliseconds operator- ( Milliseconds const time ) const;
        inline Milliseconds operator- ( Microseconds const time ) const;
        inline Milliseconds operator- ( Seconds const time ) const;

        inline Milliseconds& operator+= ( float const time );
        inline Milliseconds& operator+= ( Milliseconds const time );
        inline Milliseconds& operator+= ( Microseconds const time );
        inline Milliseconds& operator+= ( Seconds const time );

        inline Milliseconds& operator-= ( float const time );
        inline Milliseconds& operator-= ( Milliseconds const time );
        inline Milliseconds& operator-= ( Microseconds const time );
        inline Milliseconds& operator-= ( Seconds const time );

    private:

        float m_value;
    };

    //-------------------------------------------------------------------------

    class Seconds
    {
        friend Microseconds;
        friend Milliseconds;
        EE_SERIALIZE( m_value );

    public:

        Seconds() : m_value( 0 ) {}
        Seconds( float time ) : m_value( time ) {}
        inline explicit Seconds( Nanoseconds time );
        inline explicit Seconds( Microseconds time );
        inline explicit Seconds( Milliseconds time );

        inline Seconds& operator= ( float time ) { m_value = time; return *this; }
        inline Seconds& operator= ( Microseconds time );
        inline Seconds& operator= ( Milliseconds time );

        inline operator float() const { return m_value; }
        inline float ToFloat() const { return m_value; }
        inline explicit operator Nanoseconds() const;
        inline explicit operator Microseconds() const;
        inline explicit operator Milliseconds() const;

        inline Nanoseconds ToNanoseconds() const;
        inline Microseconds ToMicroseconds() const;
        inline Milliseconds ToMilliseconds() const;

        inline Seconds operator+ ( float const time ) const;
        inline Seconds operator+ ( Seconds const time ) const;
        inline Seconds operator+ ( Microseconds const time ) const;
        inline Seconds operator+ ( Milliseconds const time ) const;

        inline Seconds operator- ( float const time ) const;
        inline Seconds operator- ( Seconds const time ) const;
        inline Seconds operator- ( Microseconds const time ) const;
        inline Seconds operator- ( Milliseconds const time ) const;

        inline Seconds& operator+= ( float const time );
        inline Seconds& operator+= ( Seconds const time );
        inline Seconds& operator+= ( Microseconds const time );
        inline Seconds& operator+= ( Milliseconds const time );

        inline Seconds& operator-= ( float const time );
        inline Seconds& operator-= ( Seconds const time );
        inline Seconds& operator-= ( Microseconds const time );
        inline Seconds& operator-= ( Milliseconds const time );

        inline Seconds operator* ( float const time ) const;
        inline Seconds operator* ( Seconds const time ) const;
        inline Seconds operator* ( Microseconds const time ) const;
        inline Seconds operator* ( Milliseconds const time ) const;

        inline Seconds operator/ ( float const time ) const;
        inline Seconds operator/ ( Seconds const time ) const;
        inline Seconds operator/ ( Microseconds const time ) const;
        inline Seconds operator/ ( Milliseconds const time ) const;

        inline Seconds& operator*= ( float const time );
        inline Seconds& operator*= ( Seconds const time );
        inline Seconds& operator*= ( Microseconds const time );
        inline Seconds& operator*= ( Milliseconds const time );

        inline Seconds& operator/= ( float const time );
        inline Seconds& operator/= ( Seconds const time );
        inline Seconds& operator/= ( Microseconds const time );
        inline Seconds& operator/= ( Milliseconds const time );

    private:

        float m_value;
    };

    //-------------------------------------------------------------------------
    // System high resolution timing
    //-------------------------------------------------------------------------

    class EE_SYSTEM_API Nanoseconds
    {
        EE_SERIALIZE( m_value );

    public:

        Nanoseconds() : m_value( 0 ) {}
        Nanoseconds( uint64_t time ) : m_value( time ) {}
        explicit Nanoseconds( Nanoseconds const& time ) : m_value( time.m_value ) {}

        inline Nanoseconds& operator= ( uint64_t time ) { m_value = time; return *this; }
        inline Nanoseconds& operator= ( Nanoseconds time ) { m_value = time.m_value; return *this; }

        inline operator uint64_t() const { return m_value; }
        inline uint64_t ToU64() const { return m_value; }
        explicit operator Microseconds() const;
        inline explicit operator Milliseconds() const { return Milliseconds( operator Microseconds() ); }
        inline explicit operator Seconds() const { return Seconds( operator Microseconds() ); }

        inline Microseconds ToMicroseconds() const { return operator Microseconds(); }
        inline Milliseconds ToMilliseconds() const { return operator Milliseconds(); }
        inline Seconds ToSeconds() const { return operator Seconds(); }

        inline Nanoseconds operator+ ( uint64_t const time ) const { return Nanoseconds( m_value + time ); }
        inline Nanoseconds operator+ ( Nanoseconds const time ) const { return Nanoseconds( m_value + time.m_value ); }

        inline Nanoseconds operator- ( uint64_t const time ) const { return Nanoseconds( m_value - time ); }
        inline Nanoseconds operator- ( Nanoseconds const time ) const { return Nanoseconds( m_value - time.m_value ); }

        inline Nanoseconds& operator+= ( uint64_t const time ) { m_value += time; return *this; };
        inline Nanoseconds& operator+= ( Nanoseconds const time ) { m_value += time.m_value; return *this; };

        inline Nanoseconds& operator-= ( uint64_t const time ) { m_value -= time; return *this; }
        inline Nanoseconds& operator-= ( Nanoseconds const time ) { m_value -= time.m_value; return *this; }

    private:

        uint64_t m_value;
    };

    //-------------------------------------------------------------------------
    // Conversion functions
    //-------------------------------------------------------------------------

    inline Microseconds::Microseconds( Nanoseconds time ) : m_value( time.ToMicroseconds() ) {}
    inline Microseconds::Microseconds( Milliseconds time ) : m_value( time.m_value * 1000 ) {}
    inline Microseconds::Microseconds( Seconds time ) : m_value( time.m_value * 1000000 ) {}
    inline Microseconds& Microseconds::operator= ( Milliseconds time ) { m_value = time.m_value * 1000; return *this; }
    inline Microseconds& Microseconds::operator= ( Seconds time ) { m_value = time.m_value * 1000000; return *this; }
    inline Microseconds::operator Nanoseconds() const { return Nanoseconds( uint64_t( double( m_value ) * 1000 ) ); }
    inline Microseconds::operator Milliseconds() const { return Milliseconds( m_value / 1000 ); }
    inline Microseconds::operator Seconds() const { return Seconds( m_value / 1000000 ); }
    inline Nanoseconds Microseconds::ToNanoseconds() const { return operator Nanoseconds(); }
    inline Milliseconds Microseconds::ToMilliseconds() const { return operator Milliseconds(); }
    inline Seconds Microseconds::ToSeconds() const { return operator Seconds(); }

    //-------------------------------------------------------------------------

    inline Milliseconds::Milliseconds( Nanoseconds time ) : m_value( time.ToMilliseconds() ) {}
    inline Milliseconds::Milliseconds( Microseconds time ) : m_value( time.m_value / 1000 ) {}
    inline Milliseconds::Milliseconds( Seconds time ) : m_value( time.m_value * 1000 ) {}
    inline Milliseconds& Milliseconds::operator= ( Microseconds time ) { m_value = time.m_value / 1000; return *this; }
    inline Milliseconds& Milliseconds::operator= ( Seconds time ) { m_value = time.m_value * 1000; return *this; }
    inline Milliseconds::operator Nanoseconds() const { return Nanoseconds( uint64_t( double( m_value ) * 1e+6 ) ); }
    inline Milliseconds::operator Microseconds() const { return Microseconds( m_value * 1000 ); }
    inline Milliseconds::operator Seconds() const { return Seconds( m_value / 1000 ); }
    inline Nanoseconds Milliseconds::ToNanoseconds() const { return operator Nanoseconds(); }
    inline Microseconds Milliseconds::ToMicroseconds() const { return operator Microseconds(); }
    inline Seconds Milliseconds::ToSeconds() const { return operator Seconds(); }

    //-------------------------------------------------------------------------

    inline Seconds::Seconds( Nanoseconds time ) : m_value( time.ToSeconds() ) {}
    inline Seconds::Seconds( Microseconds time ) : m_value( time.m_value / 1000000 ) {}
    inline Seconds::Seconds( Milliseconds time ) : m_value( time.m_value / 1000 ) {}
    inline Seconds& Seconds::operator= ( Microseconds time ) { m_value = time.m_value / 1000000; return *this; }
    inline Seconds& Seconds::operator= ( Milliseconds time ) { m_value = time.m_value / 1000; return *this; }
    inline Seconds::operator Nanoseconds() const { return Nanoseconds( uint64_t( double(m_value) / 1e+9 ) ); }
    inline Seconds::operator Microseconds() const { return Microseconds( m_value * 1000000 ); }
    inline Seconds::operator Milliseconds() const { return Milliseconds( m_value * 1000 ); }
    inline Nanoseconds Seconds::ToNanoseconds() const { return operator Nanoseconds(); }
    inline Microseconds Seconds::ToMicroseconds() const { return operator Microseconds(); }
    inline Milliseconds Seconds::ToMilliseconds() const { return operator Milliseconds(); }

    //-------------------------------------------------------------------------
    // Arithmetic functions
    //-------------------------------------------------------------------------

    inline Microseconds Microseconds::operator+ ( float const time ) const { return Microseconds( m_value + time ); }
    inline Microseconds Microseconds::operator+ ( Microseconds const time ) const { return Microseconds( m_value + time.m_value ); }
    inline Microseconds Microseconds::operator+ ( Milliseconds const time ) const { return *this + (Microseconds) time; }
    inline Microseconds Microseconds::operator+ ( Seconds const time ) const { return *this + (Microseconds) time; }

    inline Microseconds Microseconds::operator- ( float const time ) const { return Microseconds( m_value - time ); }
    inline Microseconds Microseconds::operator- ( Microseconds const time ) const { return Microseconds( m_value - time.m_value ); }
    inline Microseconds Microseconds::operator- ( Milliseconds const time ) const { return *this - (Microseconds) time; }
    inline Microseconds Microseconds::operator- ( Seconds const time ) const { return *this - (Microseconds) time; }

    inline Microseconds& Microseconds::operator+= ( float const time ) { m_value += time; return *this; }
    inline Microseconds& Microseconds::operator+= ( Microseconds const time ) { m_value += time.m_value; return *this; }
    inline Microseconds& Microseconds::operator+= ( Milliseconds const time ) { *this += (Microseconds) time; return *this; }
    inline Microseconds& Microseconds::operator+= ( Seconds const time ) { *this += (Microseconds) time; return *this; }

    inline Microseconds& Microseconds::operator-= ( float const time ) { m_value -= time; return *this; }
    inline Microseconds& Microseconds::operator-= ( Microseconds const time ) { m_value -= time.m_value; return *this; }
    inline Microseconds& Microseconds::operator-= ( Milliseconds const time ) { *this -= (Microseconds) time; return *this; }
    inline Microseconds& Microseconds::operator-= ( Seconds const time ) { *this -= (Microseconds) time; return *this; }

    //-------------------------------------------------------------------------

    inline Milliseconds Milliseconds::operator+ ( float const time ) const { return Milliseconds( m_value + time ); }
    inline Milliseconds Milliseconds::operator+ ( Milliseconds const time ) const { return Milliseconds( m_value + time.m_value ); }
    inline Milliseconds Milliseconds::operator+ ( Microseconds const time ) const { return *this + (Milliseconds) time; }
    inline Milliseconds Milliseconds::operator+ ( Seconds const time ) const { return *this + (Milliseconds) time; }

    inline Milliseconds Milliseconds::operator- ( float const time ) const { return Milliseconds( m_value - time ); }
    inline Milliseconds Milliseconds::operator- ( Milliseconds const time ) const { return Milliseconds( m_value - time.m_value ); }
    inline Milliseconds Milliseconds::operator- ( Microseconds const time ) const { return *this - (Milliseconds) time; }
    inline Milliseconds Milliseconds::operator- ( Seconds const time ) const { return *this - (Milliseconds) time; }

    inline Milliseconds& Milliseconds::operator+= ( float const time ) { m_value += time; return *this; }
    inline Milliseconds& Milliseconds::operator+= ( Milliseconds const time ) { m_value += time.m_value; return *this; }
    inline Milliseconds& Milliseconds::operator+= ( Microseconds const time ) { *this += (Milliseconds) time; return *this; }
    inline Milliseconds& Milliseconds::operator+= ( Seconds const time ) { *this += (Milliseconds) time; return *this; }

    inline Milliseconds& Milliseconds::operator-= ( float const time ) { m_value -= time; return *this; }
    inline Milliseconds& Milliseconds::operator-= ( Milliseconds const time ) { m_value -= time.m_value; return *this; }
    inline Milliseconds& Milliseconds::operator-= ( Microseconds const time ) { *this -= (Milliseconds) time; return *this; }
    inline Milliseconds& Milliseconds::operator-= ( Seconds const time ) { *this -= (Milliseconds) time; return *this; }

    //-------------------------------------------------------------------------

    inline Seconds Seconds::operator+ ( float const time ) const { return Seconds( m_value + time ); }
    inline Seconds Seconds::operator+ ( Seconds const time ) const { return Seconds( m_value + time.m_value ); }
    inline Seconds Seconds::operator+ ( Microseconds const time ) const { return *this + (Seconds) time; }
    inline Seconds Seconds::operator+ ( Milliseconds const time ) const { return *this + (Seconds) time; }

    inline Seconds Seconds::operator- ( float const time ) const { return Seconds( m_value - time ); }
    inline Seconds Seconds::operator- ( Seconds const time ) const { return Seconds( m_value - time.m_value ); }
    inline Seconds Seconds::operator- ( Microseconds const time ) const { return *this - (Seconds) time; }
    inline Seconds Seconds::operator- ( Milliseconds const time ) const { return *this - (Seconds) time; }

    inline Seconds& Seconds::operator+= ( float const time ) { m_value += time; return *this; }
    inline Seconds& Seconds::operator+= ( Seconds const time ) { m_value += time.m_value; return *this; }
    inline Seconds& Seconds::operator+= ( Microseconds const time ) { *this += (Seconds) time; return *this; }
    inline Seconds& Seconds::operator+= ( Milliseconds const time ) { *this += (Seconds) time; return *this; }

    inline Seconds& Seconds::operator-= ( float const time ) { m_value -= time; return *this; }
    inline Seconds& Seconds::operator-= ( Seconds const time ) { m_value -= time.m_value; return *this; }
    inline Seconds& Seconds::operator-= ( Microseconds const time ) { *this -= (Seconds) time; return *this; }
    inline Seconds& Seconds::operator-= ( Milliseconds const time ) { *this -= (Seconds) time; return *this; }

    inline Seconds Seconds::operator* ( float const time ) const { return Seconds( m_value * time ); }
    inline Seconds Seconds::operator* ( Seconds const time ) const { return Seconds( m_value * time.m_value ); }
    inline Seconds Seconds::operator* ( Microseconds const time ) const { return *this * (Seconds) time; }
    inline Seconds Seconds::operator* ( Milliseconds const time ) const { return *this * (Seconds) time; }

    inline Seconds Seconds::operator/ ( float const time ) const { return Seconds( m_value / time ); }
    inline Seconds Seconds::operator/ ( Seconds const time ) const { return Seconds( m_value / time.m_value ); }
    inline Seconds Seconds::operator/ ( Microseconds const time ) const { return *this / (Seconds) time; }
    inline Seconds Seconds::operator/ ( Milliseconds const time ) const { return *this / (Seconds) time; }

    inline Seconds& Seconds::operator*= ( float const time ) { m_value *= time; return *this; }
    inline Seconds& Seconds::operator*= ( Seconds const time ) { m_value *= time.m_value; return *this; }
    inline Seconds& Seconds::operator*= ( Microseconds const time ) { *this *= (Seconds) time; return *this; }
    inline Seconds& Seconds::operator*= ( Milliseconds const time ) { *this *= (Seconds) time; return *this; }

    inline Seconds& Seconds::operator/= ( float const time ) { m_value /= time; return *this; }
    inline Seconds& Seconds::operator/= ( Seconds const time ) { m_value /= time.m_value; return *this; }
    inline Seconds& Seconds::operator/= ( Microseconds const time ) { *this /= (Seconds) time; return *this; }
    inline Seconds& Seconds::operator/= ( Milliseconds const time ) { *this /= (Seconds) time; return *this; }

    //-------------------------------------------------------------------------
    // Clocks
    //-------------------------------------------------------------------------

    // Actual platform (OS) time
    struct EE_SYSTEM_API PlatformClock
    {
        static Nanoseconds GetTime();

        static inline Microseconds GetTimeInMicroseconds() { return Microseconds( GetTime() ); }
        static inline Milliseconds GetTimeInMilliseconds() { return Milliseconds( GetTime() ); }
        static inline Seconds GetTimeInSeconds() { return Seconds( GetTime() ); }
    };

    // Game engine system time - updated each frame using the frame delta
    class EE_SYSTEM_API EngineClock
    {
        static Nanoseconds CurrentTime;

    public:

        static inline Nanoseconds GetTime() { return CurrentTime; }
        static void Update( Milliseconds deltaTime );

        static inline Microseconds GetTimeInMicroseconds() { return Microseconds( GetTime() ); }
        static inline Milliseconds GetTimeInMilliseconds() { return Milliseconds( GetTime() ); }
        static inline Seconds GetTimeInSeconds() { return Seconds( GetTime() ); }
    };
}