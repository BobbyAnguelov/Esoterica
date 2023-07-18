#include "MathRandom.h"
#include "Base/Threading/Threading.h"
#include <random>

//-------------------------------------------------------------------------

namespace EE::Math
{
    RNG::RNG()
    {
        m_rng.seed( pcg_extras::seed_seq_from<std::random_device>() );
    }

    RNG::RNG( uint32_t seed )
        : m_rng( seed )
    {
        EE_ASSERT( seed != 0 );
    }

    //-------------------------------------------------------------------------

    namespace
    {
        RNG                 g_rng;
        Threading::Mutex    g_globalRandomMutex;
    }

    bool GetRandomBool()
    {
        Threading::ScopeLock lock( g_globalRandomMutex );
        return g_rng.GetUInt( 0, 1 ) == 1;
    }

    uint32_t GetRandomUInt( uint32_t min, uint32_t max )
    {
        EE_ASSERT( min < max );
        Threading::ScopeLock lock( g_globalRandomMutex );
        return g_rng.GetUInt( min, max );
    }

    int32_t GetRandomInt( int32_t min, int32_t max )
    {
        EE_ASSERT( min < max );

        uint32_t const umax = max - min;
        int64_t randomValue = GetRandomUInt( 0, umax );
        return static_cast<int32_t>( randomValue + min );
    }

    float GetRandomFloat( float min, float max )
    {
        Threading::ScopeLock lock( g_globalRandomMutex );
        return g_rng.GetFloat( min, max );
    }
}