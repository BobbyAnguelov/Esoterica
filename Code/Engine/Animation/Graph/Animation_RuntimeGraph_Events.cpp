#include "Animation_RuntimeGraph_Events.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    SampledEvent::SampledEvent( int16_t sourceNodeIdx, Event const* pEvent, Percentage percentageThrough )
        : m_eventData( pEvent )
        , m_percentageThrough( percentageThrough.GetClamped( false ) )
        , m_sourceNodeIdx( sourceNodeIdx )
    {
        EE_ASSERT( pEvent != nullptr && sourceNodeIdx != InvalidIndex );
    }

    SampledEvent::SampledEvent( int16_t sourceNodeIdx, Flags stateEventType, StringID ID, Percentage percentageThrough )
        : m_eventData( ID )
        , m_percentageThrough( percentageThrough.GetClamped( false ) )
        , m_flags( stateEventType )
        , m_sourceNodeIdx( sourceNodeIdx )
    {
        EE_ASSERT( stateEventType >= Flags::StateEntry && sourceNodeIdx != InvalidIndex );
    }

    //-------------------------------------------------------------------------
    
    SampledEventRange CombineAndUpdateEvents( SampledEventsBuffer& sampledEventsBuffer, SampledEventRange const& eventRange0, SampledEventRange const& eventRange1, float const blendWeight )
    {
        SampledEventRange combinedRange;

        // If we are fully in one or the other source, then only sample events for the active source
        if ( blendWeight == 0.0f )
        {
            combinedRange = eventRange0;
            EE_ASSERT( sampledEventsBuffer.IsValidRange( eventRange1 ) );
            sampledEventsBuffer.UpdateWeights( eventRange1, 0.0f );
        }
        else if ( blendWeight == 1.0f )
        {
            combinedRange = eventRange1;
            EE_ASSERT( sampledEventsBuffer.IsValidRange( eventRange0 ) );
            sampledEventsBuffer.UpdateWeights( eventRange0, 0.0f );
        }
        else // Combine events
        {
            // Sample events for both sources and updated sampled event weights
            uint32_t const numEventsSource0 = eventRange0.GetLength();
            uint32_t const numEventsSource1 = eventRange1.GetLength();

            if ( ( numEventsSource0 + numEventsSource1 ) > 0 )
            {
                sampledEventsBuffer.UpdateWeights( eventRange0, blendWeight );
                sampledEventsBuffer.UpdateWeights( eventRange1, 1.0f - blendWeight );

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
                combinedRange = SampledEventRange( sampledEventsBuffer.GetNumEvents() );
            }
        }

        EE_ASSERT( sampledEventsBuffer.IsValidRange( combinedRange ) );
        return combinedRange;
    }
}