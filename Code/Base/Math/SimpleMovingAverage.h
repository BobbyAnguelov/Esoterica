#pragma once

#include "Math.h"

//-------------------------------------------------------------------------

namespace EE
{
    template<uint32_t HistorySize>
    class SimpleMovingAverage
    {
    public:

        SimpleMovingAverage( float startValue )
            : m_average( startValue )
            , m_nextFreeIdx( 0 )
        {
            static_assert( HistorySize > 1, "History size has to be at least 1" );
            for ( auto i = 0; i < HistorySize; i++ )
            {
                m_values[i] = startValue;
            }
        }

        inline float GetAverage() const { return m_average; }

        inline void AddValue( float const value )
        {
            m_values[m_nextFreeIdx++] = value;
            if ( m_nextFreeIdx == HistorySize )
            {
                m_nextFreeIdx = 0;
            }

            m_average = 0;
            for ( float v : m_values )
            {
                m_average += v;
            }
            m_average /= HistorySize;
        }

    private:

        float                       m_values[HistorySize];
        float                       m_average;
        int32_t                     m_nextFreeIdx;
    };
}