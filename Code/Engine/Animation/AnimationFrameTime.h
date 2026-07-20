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
        FrameTime( int32_t frameIndex, Percentage percentageThrough = Percentage( 0 ) );
        FrameTime( Percentage percent, int32_t numFrames );

        // Reset the frame time to zero
        inline void Reset() { m_frameIndex = 0; m_percentageThrough = 0.0f; }

        // Are we at the start of a frame based range
        inline bool IsZero() const { return m_frameIndex == 0 && m_percentageThrough == 0.0f; }

        // Are we at the start of a frame based range
        inline bool IsAtStart() const { return IsZero(); }

        // Get the frame index
        inline int32_t GetFrameIndex() const { return m_frameIndex; }

        // Get the percentage through the frame
        inline Percentage GetPercentageThrough() const { return m_percentageThrough; }

        // Is this an exact keyframe i.e. percentage through == 0.0
        inline bool IsExactlyAtKeyFrame() const { return m_percentageThrough == 0.0f; }

        // Get the nearest frame index to the current time (basically acts as a round)
        inline int32_t GetNearestFrameIndex() const { return m_frameIndex + (int32_t) Math::Round( m_percentageThrough.ToFloat() ); }

        // Get the lower bound frame index for the current time
        inline int32_t GetLowerBoundFrameIndex() const { return m_frameIndex; }

        // Get the upper bound frame index for the current time
        inline int32_t GetUpperBoundFrameIndex() const { return ( m_percentageThrough > 0.0f ) ? m_frameIndex + 1 : m_frameIndex; }

        // Convert to a single float where the fraction part is the percentage through
        inline float ToFloat() const { EE_ASSERT( m_percentageThrough.ToFloat() < 1.0f ); return m_percentageThrough.ToFloat() + m_frameIndex; }

        // Get the time in seconds
        inline Seconds GetTimeInSeconds( Seconds frameLength ) const { EE_ASSERT( frameLength > 0.0f ); return ToFloat() * frameLength; }

        inline FrameTime operator+( FrameTime const& RHS ) const;
        inline FrameTime& operator+=( FrameTime const& RHS );
        inline FrameTime operator-( FrameTime const& RHS ) const;
        inline FrameTime& operator-=( FrameTime const& RHS );

        inline FrameTime operator+( int32_t const& RHS ) const;
        inline FrameTime& operator+=( int32_t const& RHS );
        inline FrameTime operator-( int32_t const& RHS ) const;
        inline FrameTime& operator-=( int32_t const& RHS );

        FrameTime operator+( Percentage const& RHS ) const;
        FrameTime& operator+=( Percentage const& RHS );
        FrameTime operator-( Percentage const& RHS ) const;
        FrameTime& operator-=( Percentage const& RHS );

        inline bool operator <( FrameTime const &rhs ) const;
        inline bool operator <=( FrameTime const &rhs ) const;
        inline bool operator >( FrameTime const &rhs ) const;
        inline bool operator >=( FrameTime const &rhs ) const;
        inline bool operator ==( FrameTime const &rhs ) const;
        inline bool operator !=( FrameTime const &rhs ) const;

    private:

        int32_t                 m_frameIndex = 0;
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

    inline FrameTime FrameTime::operator+( int32_t const& RHS ) const
    {
        FrameTime newInterval = *this;
        newInterval.m_frameIndex += RHS;
        return newInterval;
    }

    inline FrameTime& FrameTime::operator+=( int32_t const& RHS )
    {
        m_frameIndex += RHS;
        return *this;
    }

    inline FrameTime FrameTime::operator-( int32_t const& RHS ) const
    {
        FrameTime newInterval = *this;
        newInterval.m_frameIndex -= RHS;
        return newInterval;
    }

    inline FrameTime& FrameTime::operator-=( int32_t const& RHS )
    {
        m_frameIndex -= RHS;
        return *this;
    }

    inline bool FrameTime::operator<( FrameTime const &rhs ) const
    {
        if ( m_frameIndex < rhs.m_frameIndex )
        {
            return true;
        }

        return ( m_frameIndex == rhs.m_frameIndex ) && m_percentageThrough < rhs.m_percentageThrough;
    }

    inline bool FrameTime::operator<=( FrameTime const &rhs ) const
    {
        if ( m_frameIndex < rhs.m_frameIndex )
        {
            return true;
        }

        return ( m_frameIndex == rhs.m_frameIndex ) && m_percentageThrough <= rhs.m_percentageThrough;
    }

    inline bool FrameTime::operator >( FrameTime const &rhs ) const
    {
        if ( m_frameIndex > rhs.m_frameIndex )
        {
            return true;
        }

        return ( m_frameIndex == rhs.m_frameIndex ) && m_percentageThrough > rhs.m_percentageThrough;
    }

    inline bool FrameTime::operator>=( FrameTime const &rhs ) const
    {
        if ( m_frameIndex > rhs.m_frameIndex )
        {
            return true;
        }

        return ( m_frameIndex == rhs.m_frameIndex ) && m_percentageThrough >= rhs.m_percentageThrough;
    }

    inline bool FrameTime::operator==( FrameTime const &rhs ) const
    {
        return ( m_frameIndex == rhs.m_frameIndex ) && ( m_percentageThrough == rhs.m_percentageThrough );
    }

    inline bool FrameTime::operator!=( FrameTime const &rhs ) const
    {
        return ( m_frameIndex != rhs.m_frameIndex ) || ( m_percentageThrough != rhs.m_percentageThrough );
    }
}