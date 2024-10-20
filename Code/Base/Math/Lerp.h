#pragma once

#include "Base/Time/Time.h"
#include "Base/Types/Percentage.h"

//-------------------------------------------------------------------------

namespace EE::Math
{
    // Basic Linear Interpolation (Lerp)
    template<typename T>
    EE_FORCE_INLINE T Lerp( T from, T to, float t )
    {
        return from + ( to - from ) * t;
    }

    // Frame-rate independent Lerp
    // HalfLifeTime is the time need to get halfway through the lerp in seconds
    template<typename T>
    EE_FORCE_INLINE T LerpSmooth( T from, T to, Seconds deltaTime, Seconds halfLifeTime )
    {
        to + ( from - to ) * Math::Exp2( -deltaTime.ToFloat() / halfLifeTime.ToFloat() );
    }

    // Helper to calculate the HalfLifeTime for a LerpSmooth for a given total duration and precision.
    // Precision defines the allowable distance from the target after duration has elapsed
    // So for a precision of 0.01 (1%), the result of LerpSmooth after "duration" will be the 99% of the way to the target
    EE_FORCE_INLINE Seconds CalculateLerpSmoothHalfLifeTime( Seconds duration, Percentage precision )
    {
        return -duration.ToFloat() / Math::Log2( precision );
    }
}