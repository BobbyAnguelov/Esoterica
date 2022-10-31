#include "Animation_RuntimeGraph_Events.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void SampledEventsBuffer::Clear()
    {
        m_sampledEvents.clear();
        m_numAnimEventsSampled = m_numStateEventsSampled = 0;
    }

    //-------------------------------------------------------------------------

    bool SampledEventsBuffer::ContainsStateEvent( StringID ID, bool onlyFromActiveBranch ) const
    {
        if ( onlyFromActiveBranch )
        {
            for ( auto const& se : m_sampledEvents )
            {
                if ( !se.IsStateEvent() )
                {
                    continue;
                }

                if ( !se.IsIgnored() && se.IsFromActiveBranch() && se.GetStateEventID() == ID )
                {
                    return true;
                }
            }
        }
        else
        {
            for ( auto const& se : m_sampledEvents )
            {
                if ( !se.IsStateEvent() )
                {
                    continue;
                }

                if ( !se.IsIgnored() && se.GetStateEventID() == ID )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool SampledEventsBuffer::ContainsSpecificStateEvent( StateEventType eventType, StringID ID, bool onlyFromActiveBranch ) const
    {
        if ( onlyFromActiveBranch )
        {
            for ( auto const& se : m_sampledEvents )
            {
                if ( !se.IsStateEvent() )
                {
                    continue;
                }

                if ( !se.IsIgnored() && se.IsFromActiveBranch() && se.GetEventType() == eventType && se.GetStateEventID() == ID )
                {
                    return true;
                }
            }
        }
        else
        {
            for ( auto const& se : m_sampledEvents )
            {
                if ( !se.IsStateEvent() )
                {
                    continue;
                }

                if ( !se.IsIgnored() && se.GetEventType() == eventType && se.GetStateEventID() == ID )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool SampledEventsBuffer::ContainsStateEvent( SampledEventRange const& range, StringID ID, bool onlyFromActiveBranch ) const
    {
        EE_ASSERT( IsValidRange( range ) );

        for ( int32_t i = range.m_startIdx; i < range.m_endIdx; i++ )
        {
            auto const& se = m_sampledEvents[i];

            if ( !se.IsStateEvent() )
            {
                continue;
            }

            if ( se.IsIgnored() )
            {
                continue;
            }

            if ( onlyFromActiveBranch && !se.IsFromActiveBranch() )
            {
                continue;
            }

            if ( se.GetStateEventID() == ID )
            {
                return true;
            }
        }

        return false;
    }

    bool SampledEventsBuffer::ContainsSpecificStateEvent( SampledEventRange const& range, StateEventType eventType, StringID ID, bool onlyFromActiveBranch ) const
    {
        EE_ASSERT( IsValidRange( range ) );

        for ( int32_t i = range.m_startIdx; i < range.m_endIdx; i++ )
        {
            auto const& se = m_sampledEvents[i];

            if ( !se.IsStateEvent() )
            {
                continue;
            }

            if ( se.IsIgnored() )
            {
                continue;
            }

            if ( onlyFromActiveBranch && !se.IsFromActiveBranch() )
            {
                continue;
            }

            if ( se.GetEventType() == eventType && se.GetStateEventID() == ID )
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    SampledEventRange SampledEventsBuffer::BlendEventRanges( SampledEventRange const& eventRange0, SampledEventRange const& eventRange1, float const blendWeight )
    {
        EE_ASSERT( IsValidRange( eventRange0 ) && IsValidRange( eventRange1 ) );
        EE_ASSERT( eventRange0.m_endIdx == eventRange1.m_startIdx );

        SampledEventRange combinedRange;

        // If we are fully in one or the other source, then only sample events for the active source
        if ( blendWeight == 0.0f )
        {
            combinedRange = eventRange0;
            UpdateWeights( eventRange1, 0.0f );
        }
        else if ( blendWeight == 1.0f )
        {
            combinedRange = eventRange1;
            UpdateWeights( eventRange0, 0.0f );
        }
        else // Combine events
        {
            // Sample events for both sources and updated sampled event weights
            uint32_t const numEventsSource0 = eventRange0.GetLength();
            uint32_t const numEventsSource1 = eventRange1.GetLength();

            if ( ( numEventsSource0 + numEventsSource1 ) > 0 )
            {
                UpdateWeights( eventRange0, blendWeight );
                UpdateWeights( eventRange1, 1.0f - blendWeight );

                // Combine sampled event range - source0's range must always be before source1's range
                if ( numEventsSource0 > 0 && numEventsSource1 > 0 )
                {
                    EE_ASSERT( eventRange0.m_endIdx <= eventRange1.m_startIdx );
                    combinedRange.m_startIdx = eventRange0.m_startIdx;
                    combinedRange.m_endIdx = eventRange1.m_endIdx;
                }
                else if ( numEventsSource0 > 0 ) // Only source0 has sampled events
                {
                    combinedRange = eventRange0;
                }
                else // Only source1 has sampled events
                {
                    combinedRange = eventRange1;
                }
            }
            else
            {
                combinedRange = SampledEventRange( GetNumSampledEvents() );
            }
        }

        EE_ASSERT( IsValidRange( combinedRange ) );
        return combinedRange;
    }

    SampledEventRange SampledEventsBuffer::AppendBuffer( SampledEventsBuffer const& otherBuffer )
    {
        SampledEventRange newEventRange( (uint16_t) m_sampledEvents.size() );

        //-------------------------------------------------------------------------

        m_sampledEvents.reserve( m_sampledEvents.size() + otherBuffer.m_sampledEvents.size() );
        m_sampledEvents.insert( m_sampledEvents.end(), otherBuffer.m_sampledEvents.begin(), otherBuffer.m_sampledEvents.end() );

        //-------------------------------------------------------------------------

        newEventRange.m_endIdx = (uint16_t) m_sampledEvents.size();
        return newEventRange;
    }
}