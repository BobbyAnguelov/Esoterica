#pragma once

#include "Engine/_Module/API.h"
#include "Base/Types/Percentage.h"
#include "Base/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API FrameTime
    {
    public:

        FrameTime() = default;

        inline FrameTime( uint32_t frameIndex, Percentage percentageThrough = Percentage( 0 ) )
            : m_frameIndex( frameIndex )
            , m_percentageThrough( percentageThrough )
        {
            EE_ASSERT( percentageThrough.ToFloat() >= 0.0f && percentageThrough < 1.0f );
        }

        inline FrameTime( Percentage percent, uint32_t numFrames )
        {
            EE_ASSERT( numFrames > 0 && percent.ToFloat() >= 0.0f );
            uint32_t const lastFrameIdx = numFrames - 1;

            if ( percent == 0.0f )
            {
                m_frameIndex = 0;
                m_percentageThrough = 0.0f;
            }
            else if ( percent == 1.0f )
            {
                m_frameIndex = lastFrameIdx;
                m_percentageThrough = 0.0f;
            }
            else
            {
                percent.Clamp( true );
                float integerPortion = 0;
                m_percentageThrough = Percentage( Math::ModF( percent.ToFloat() * lastFrameIdx, integerPortion ) );
                if ( Math::IsNearZero( m_percentageThrough, Math::LargeEpsilon ) )
                {
                    m_percentageThrough = 0.0f;
                }
                m_frameIndex = (uint32_t) integerPortion;
            }
        }

        //-------------------------------------------------------------------------

        inline uint32_t GetFrameIndex() const { return m_frameIndex; }
        inline Percentage GetPercentageThrough() const { return m_percentageThrough; }
        inline bool IsExactlyAtKeyFrame() const { return m_percentageThrough == 0.0f; }

        // Get the nearest frame index to the current time (basically acts as a round)
        inline uint32_t GetNearestFrameIndex() const { return m_frameIndex + (uint32_t) Math::Round( m_percentageThrough.ToFloat() ); }

        // Get the lower bound frame index for the current time
        inline uint32_t GetLowerBoundFrameIndex() const { return m_frameIndex; }

        // Get the upper bound frame index for the current time
        inline uint32_t GetUpperBoundFrameIndex() const { return ( m_percentageThrough > 0.0f ) ? m_frameIndex + 1 : m_frameIndex; }

        inline float ToFloat() const { return m_percentageThrough.ToFloat() + m_frameIndex; }

        inline Seconds GetTimeInSeconds( Seconds frameLength ) const { EE_ASSERT( frameLength > 0.0f );  return ToFloat() * frameLength; }

        inline FrameTime operator+( FrameTime const& RHS ) const;
        inline FrameTime& operator+=( FrameTime const& RHS );
        inline FrameTime operator-( FrameTime const& RHS ) const;
        inline FrameTime& operator-=( FrameTime const& RHS );

        inline FrameTime operator+( uint32_t const& RHS ) const;
        inline FrameTime& operator+=( uint32_t const& RHS );
        inline FrameTime operator-( uint32_t const& RHS ) const;
        inline FrameTime& operator-=( uint32_t const& RHS );

        FrameTime operator+( Percentage const& RHS ) const;
        FrameTime& operator+=( Percentage const& RHS );
        FrameTime operator-( Percentage const& RHS ) const;
        FrameTime& operator-=( Percentage const& RHS );

    private:

        uint32_t                m_frameIndex = 0;
        Percentage              m_percentageThrough = Percentage( 0.0f );
    };

    //-------------------------------------------------------------------------

    inline FrameTime FrameTime::operator+( FrameTime const& RHS ) const
    {
        FrameTime newInterval = *this;
        newInterval.m_frameIndex += RHS.m_frameIndex;
        newInterval.m_percentageThrough += RHS.m_percentageThrough;
        return newInterval;
    }

    inline FrameTime& FrameTime::operator+=( FrameTime const& RHS )
    {
        m_frameIndex += RHS.m_frameIndex;
        m_percentageThrough += RHS.m_percentageThrough;
        return *this;
    }

    inline FrameTime FrameTime::operator-( FrameTime const& RHS ) const
    {
        FrameTime newInterval = *this;
        newInterval.m_frameIndex -= RHS.m_frameIndex;
        newInterval.m_percentageThrough -= RHS.m_percentageThrough;
        return newInterval;
    }

    inline FrameTime& FrameTime::operator-=( FrameTime const& RHS )
    {
        m_frameIndex -= RHS.m_frameIndex;
        m_percentageThrough -= RHS.m_percentageThrough;
        return *this;
    }

    inline FrameTime FrameTime::operator+( uint32_t const& RHS ) const
    {
        FrameTime newInterval = *this;
        newInterval.m_frameIndex += RHS;
        return newInterval;
    }

    inline FrameTime& FrameTime::operator+=( uint32_t const& RHS )
    {
        m_frameIndex += RHS;
        return *this;
    }

    inline FrameTime FrameTime::operator-( uint32_t const& RHS ) const
    {
        FrameTime newInterval = *this;
        newInterval.m_frameIndex -= RHS;
        return newInterval;
    }

    inline FrameTime& FrameTime::operator-=( uint32_t const& RHS )
    {
        m_frameIndex -= RHS;
        return *this;
    }
}