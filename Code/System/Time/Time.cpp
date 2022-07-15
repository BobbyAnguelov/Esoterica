#include "Time.h"
#include <chrono>

//-------------------------------------------------------------------------

namespace EE
{
    EE::Nanoseconds EngineClock::CurrentTime = 0;

    //-------------------------------------------------------------------------

    Nanoseconds::operator Microseconds() const
    {
        auto const duration = std::chrono::duration<uint64_t, std::chrono::steady_clock::period>( m_value );
        uint64_t const numMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>( duration ).count();
        return float( numMicroseconds );
    }

    //-------------------------------------------------------------------------

    Nanoseconds PlatformClock::GetTime()
    {
        auto const time = std::chrono::high_resolution_clock::now();
        uint64_t const numNanosecondsSinceEpoch = time.time_since_epoch().count();
        return Nanoseconds( numNanosecondsSinceEpoch );
    }

    //-------------------------------------------------------------------------

    void EngineClock::Update( Milliseconds deltaTime )
    {
        EE_ASSERT( deltaTime >= 0 );
        CurrentTime += deltaTime.ToNanoseconds();
    }
}