#pragma once

#include "Math.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Types/Percentage.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct FloatRange
    {
        EE_SERIALIZE( m_begin, m_end );

        FloatRange() = default;

        inline FloatRange( float value )
            : m_begin( value )
            , m_end( value )
        {}

        inline FloatRange( float min, float max )
            : m_begin( min )
            , m_end( max )
        {
            EE_ASSERT( min <= max );
        }

        // Reset the range to an invalid value
        inline void Clear() { *this = FloatRange(); }

        // Is the range initialized
        inline bool IsSet() const { return m_begin != FLT_MAX; }

        // Is the range contained valid i.e. is the end greater than the start
        inline bool IsValid() const { EE_ASSERT( IsSet() ); return m_end >= m_begin; }

        // Is the range set and valid
        inline bool IsSetAndValid() const { return IsSet() && IsValid(); }

        // Get the length of the range
        inline float GetLength() const { return m_end - m_begin; }

        // Get the midpoint of the range
        inline float GetMidpoint() const { return m_begin + ( ( m_end - m_begin ) / 2 ); }

        // Does this range overlap the specified range
        inline bool Overlaps( FloatRange const& rhs ) const
        {
            EE_ASSERT( IsSetAndValid() );
            return Math::Max( m_begin, rhs.m_begin ) <= Math::Min( m_end, rhs.m_end );
        }

        // Shifts the range by the supplied delta
        inline void ShiftRange( float delta )
        {
            m_begin += delta;
            m_end += delta;
        }

        // Does the range [begin, end] contain the specified range
        inline bool ContainsInclusive( FloatRange const& rhs ) const
        {
            EE_ASSERT( IsSetAndValid() && rhs.IsSetAndValid() );
            return m_begin <= rhs.m_begin && m_end >= rhs.m_end;
        }

        // Does the range [begin, end] contain the specified value
        inline bool ContainsInclusive( float const& v ) const
        {
            EE_ASSERT( IsSetAndValid() );
            return v >= m_begin && v <= m_end;
        }

        // Does the range (min, max) contain the specified range
        inline bool ContainsExclusive( FloatRange const& rhs ) const
        {
            EE_ASSERT( IsSetAndValid() && rhs.IsSetAndValid() );
            return m_begin < rhs.m_begin && m_end > rhs.m_end;
        }

        // Does the range (min, max) contain the specified value
        inline bool ContainsExclusive( float const& v ) const
        {
            EE_ASSERT( IsSetAndValid() );
            return v > m_begin && v < m_end;
        }

        // Get a value clamped to this range i.e. Clamp to [begin, end]
        inline float GetClampedValue( float const& v ) const
        {
            EE_ASSERT( IsSetAndValid() );
            return Math::Clamp( v, m_begin, m_end );
        }

        // Get the percentage through this range that specified value lies at. This is not clamped and returns a value between [-FLT_MAX, FLT_MAX]
        inline Percentage GetPercentageThrough( float const& v ) const
        {
            EE_ASSERT( IsSet() );
            float const length = GetLength();
            Percentage percentageThrough = 0.0f;
            if ( length != 0 )
            {
                percentageThrough = Percentage( ( v - m_begin ) / length );
            }
            return percentageThrough;
        }

        // Get the percentage through this range that specified value lies at. This is clamped between [0, 1]
        inline Percentage GetPercentageThroughClamped( float const& v ) const
        {
            return Math::Clamp( GetPercentageThrough( v ).ToFloat(), 0.0f, 1.0f );
        }

        // Get the value in this range at the specified percentage through. Unclamped so returns [-FLT_MAX, FLT_MAX]
        inline float GetValueForPercentageThrough( Percentage const percentageThrough ) const
        {
            EE_ASSERT( IsSet() );
            return ( GetLength() * percentageThrough ) + m_begin;
        }

        // Get the value in this range at the specified percentage through. Clamped to [begin, end]
        inline float GetValueForPercentageThroughClamped( Percentage const percentageThrough ) const
        {
            return GetValueForPercentageThrough( percentageThrough.GetClamped( false ) );
        }

        // Ensure the range is valid if the values are set incorrectly
        inline void MakeValid()
        {
            EE_ASSERT( IsSet() );

            if ( !IsValid() )
            {
                float originalEnd = m_end;
                m_end = m_begin;
                m_begin = originalEnd;
            }
        }

        // Insert a new value into the range and grow it if necessary
        inline void GrowRange( float newValue )
        {
            if ( IsSet() )
            {
                EE_ASSERT( IsValid() );
                m_begin = Math::Min( m_begin, newValue );
                m_end = Math::Max( m_end, newValue );
            }
            else
            {
                m_begin = m_end = newValue;
            }
        }

        // Grow this range with the supplied range
        inline void Merge( FloatRange const& rhs )
        {
            EE_ASSERT( IsSetAndValid() && rhs.IsSetAndValid() );
            m_begin = Math::Min( m_begin, rhs.m_begin );
            m_end = Math::Max( m_end, rhs.m_end );
        }

        // Get the combined range of this and the supplied range
        inline FloatRange GetMerged( FloatRange const& rhs ) const
        {
            FloatRange mergedRange = *this;
            mergedRange.Merge( rhs );
            return mergedRange;
        }

        inline bool operator==( FloatRange const& rhs ) const
        {
            return m_begin == rhs.m_begin && m_end == rhs.m_end;
        }

        inline bool operator!=( FloatRange const& rhs ) const
        {
            return m_begin != rhs.m_begin || m_end != rhs.m_end;
        }

    public:

        float m_begin = FLT_MAX;
        float m_end = -FLT_MAX;
    };

    //-------------------------------------------------------------------------

    struct IntRange
    {
        EE_SERIALIZE( m_begin, m_end );

        IntRange() = default;

        inline IntRange( int32_t value )
            : m_begin( value )
            , m_end( value )
        {}

        inline IntRange( int32_t min, int32_t max )
            : m_begin( min )
            , m_end( max )
        {
            EE_ASSERT( IsValid() );
        }

        // Reset the range to an invalid value
        inline void Clear() { *this = IntRange(); }

        // Is the range initialized
        inline bool IsSet() const { return m_begin != INT_MAX; }

        // Is the range contained valid i.e. is the end greater than the start
        inline bool IsValid() const { EE_ASSERT( IsSet() ); return m_end >= m_begin; }

        // Is the range set and valid
        inline bool IsSetAndValid() const { return IsSet() && IsValid(); }

        // Get the length of the range
        inline int32_t GetLength() const { return m_end - m_begin; }

        // Get the midpoint of the range
        inline int32_t GetMidpoint() const { return m_begin + ( ( m_end - m_begin ) / 2 ); }

        // Does this range overlap the specified range
        inline bool Overlaps( IntRange const& rhs ) const
        {
            EE_ASSERT( IsSetAndValid() );
            return Math::Max( m_begin, rhs.m_begin ) <= Math::Min( m_end, rhs.m_end );
        }

        // Shifts the range by the supplied delta
        inline void ShiftRange( int32_t delta )
        {
            m_begin += delta;
            m_end += delta;
        }

        // Does the range [begin, end] contain the specified range
        inline bool ContainsInclusive( IntRange const& rhs ) const
        {
            EE_ASSERT( IsSetAndValid() && rhs.IsSetAndValid() );
            return m_begin <= rhs.m_begin && m_end >= rhs.m_end;
        }

        // Does the range [begin, end] contain the specified value
        inline bool ContainsInclusive( int32_t const& v ) const
        {
            EE_ASSERT( IsSetAndValid() );
            return v >= m_begin && v <= m_end;
        }

        // Does the range (min, max) contain the specified range
        inline bool ContainsExclusive( IntRange const& rhs ) const
        {
            EE_ASSERT( IsSetAndValid() && rhs.IsSetAndValid() );
            return m_begin < rhs.m_begin && m_end > rhs.m_end;
        }

        // Does the range (min, max) contain the specified value
        inline bool ContainsExclusive( int32_t const& v ) const
        {
            EE_ASSERT( IsSetAndValid() );
            return v > m_begin && v < m_end;
        }

        // Get a value clamped to this range i.e. Clamp to [begin, end]
        inline int32_t GetClampedValue( int32_t const& v ) const
        {
            EE_ASSERT( IsSetAndValid() );
            return Math::Clamp( v, m_begin, m_end );
        }

        // Get the percentage through this range that specified value lies at. This is not clamped and returns a value between [-FLT_MAX, FLT_MAX]
        inline Percentage GetPercentageThrough( int32_t const& v ) const
        {
            EE_ASSERT( IsSet() );
            int32_t const length = GetLength();
            Percentage percentageThrough = 0.0f;
            if ( length != 0 )
            {
                percentageThrough = Percentage( float( v - m_begin ) / length );
            }
            return percentageThrough;
        }

        // Get the percentage through this range that specified value lies at. This is clamped between [0, 1]
        inline Percentage GetPercentageThroughClamped( int32_t const& v ) const
        {
            return Math::Clamp( GetPercentageThrough( v ).ToFloat(), 0.0f, 1.0f );
        }

        // Get the value in this range at the specified percentage through. Unclamped so returns [-FLT_MAX, FLT_MAX]
        inline int32_t GetValueForPercentageThrough( Percentage const percentageThrough ) const
        {
            EE_ASSERT( IsSet() );
            return Math::RoundToInt( ( GetLength() * percentageThrough ) + m_begin );
        }

        // Get the value in this range at the specified percentage through. Clamped to [begin, end]
        inline int32_t GetValueForPercentageThroughClamped( Percentage const percentageThrough ) const
        {
            return GetValueForPercentageThrough( percentageThrough.GetClamped( false ) );
        }

        // Ensure the range is valid if the values are set incorrectly
        inline void MakeValid()
        {
            EE_ASSERT( IsSet() );

            if ( !IsValid() )
            {
                int32_t originalEnd = m_end;
                m_end = m_begin;
                m_begin = originalEnd;
            }
        }

        // Insert a new value into the range and grow it if necessary
        inline void GrowRange( int32_t newValue )
        {
            if ( IsSet() )
            {
                EE_ASSERT( IsValid() );
                m_begin = Math::Min( m_begin, newValue );
                m_end = Math::Max( m_end, newValue );
            }
            else
            {
                m_begin = m_end = newValue;
            }
        }

        // Grow this range with the supplied range
        inline void Merge( IntRange const& rhs )
        {
            EE_ASSERT( IsSetAndValid() && rhs.IsSetAndValid() );
            m_begin = Math::Min( m_begin, rhs.m_begin );
            m_end = Math::Max( m_end, rhs.m_end );
        }

        // Get the combined range of this and the supplied range
        inline IntRange GetMerged( IntRange const& rhs ) const
        {
            IntRange mergedRange = *this;
            mergedRange.Merge( rhs );
            return mergedRange;
        }

        inline bool operator==( IntRange const& rhs ) const
        {
            return m_begin == rhs.m_begin && m_end == rhs.m_end;
        }

        inline bool operator!=( IntRange const& rhs ) const
        {
            return m_begin != rhs.m_begin || m_end != rhs.m_end;
        }

    public:

        int32_t m_begin = INT_MAX;
        int32_t m_end = INT_MIN;
    };
}