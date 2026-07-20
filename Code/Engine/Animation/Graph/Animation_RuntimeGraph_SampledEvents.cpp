#include "Animation_RuntimeGraph_SampledEvents.h"
#include "Engine/Animation/AnimationEvent.h"
#include "Base/Encoding/Hash.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    #if EE_DEVELOPMENT_TOOLS
    char const* GetNameForGraphEventType( GraphEventType type )
    {
        constexpr static char const* const names[] =
        {
            "Entry",
            "FullyInState",
            "Exit",
            "Timed",
            "Generic"
        };

        return names[(uint8_t) type];
    }
    #endif

    //-------------------------------------------------------------------------

    SampledEvent::SampledEvent()
    {
        m_animEventData.m_percentageThrough = 0.0f;
        m_animEventData.m_pEvent = nullptr;
    }

    SampledEvent::SampledEvent( SampledEvent const &rhs )
    {
        operator=( rhs );
    }

    SampledEvent::SampledEvent( SourcePath const& sourcePath, float weight, bool isFromActiveBranch, GraphEventType eventType, StringID eventID )
        : m_sourcePath( sourcePath )
        , m_weight( weight )
        , m_isFromActiveBranch( isFromActiveBranch )
        , m_isIgnored( false )
        , m_isGraphEvent( true )
    {
        EE_ASSERT( eventID.IsValid() );
        m_graphEventData.m_ID = eventID;
        m_graphEventData.m_type = eventType;
        m_uniqueID = CalculateUniqueID( m_sourcePath );
    }

    SampledEvent::SampledEvent( SourcePath const& sourcePath, float weight, bool isFromActiveBranch, Event const* pEvent, Percentage percentageThrough, Seconds eventDuration )
        : m_sourcePath( sourcePath )
        , m_weight( weight )
        , m_isFromActiveBranch( isFromActiveBranch )
        , m_isIgnored( false )
        , m_isGraphEvent( false )
    {
        EE_ASSERT( pEvent != nullptr );
        EE_ASSERT( percentageThrough >= 0 && percentageThrough <= 1.0f );
        m_animEventData.m_pEvent = pEvent;
        m_animEventData.m_percentageThrough = percentageThrough;
        m_animEventData.m_eventDuration = eventDuration;
        m_uniqueID = CalculateUniqueID( m_sourcePath );
    }

    bool SampledEvent::IsLessRelevant( SampledEvent const &rhs ) const
    {
        // If we are ignored but the other event is not ignored
        if ( m_isIgnored && !rhs.m_isIgnored )
        {
            return true;
        }

        // If we are from an inactive branch but the other event is from an active branch
        if ( !m_isFromActiveBranch && rhs.m_isFromActiveBranch )
        {
            return true;
        }

        return m_weight < rhs.m_weight;
    }

    uint64_t SampledEvent::CalculateUniqueID( SourcePath const& path ) const
    {
        uint64_t buffer[3] = { path.m_hash, 0, 0 };

        if ( m_isGraphEvent )
        {
            buffer[1] = m_graphEventData.m_ID.ToUint();
            buffer[2] = (uint64_t) m_graphEventData.m_type;
        }
        else
        {
            buffer[1] = (uintptr_t) m_animEventData.m_pEvent;
        }

        return Hash::GetHash64( buffer, 3 * sizeof( uint64_t ) );
    }

    SampledEvent& SampledEvent::operator=( SampledEvent const &rhs )
    {
        m_uniqueID = rhs.m_uniqueID;
        m_weight = rhs.m_weight;
        m_isFromActiveBranch = rhs.m_isFromActiveBranch;
        m_isIgnored = rhs.m_isIgnored;
        m_isGraphEvent = rhs.m_isGraphEvent;

        if ( m_isGraphEvent )
        {
            m_graphEventData = rhs.m_graphEventData;
        }
        else
        {
            m_animEventData = rhs.m_animEventData;
        }

        return *this;
    }

    //-------------------------------------------------------------------------

    void SampledEventsBuffer::Clear()
    {
        m_sampledEvents.clear();
        m_newEvents.clear();
        m_continuousEvents.clear();
        m_endedContinuousEvents.clear();

        m_sampledEvents.clear();
        m_numAnimEventsSampled = m_numGraphEventsSampled = 0;
    }

    void SampledEventsBuffer::ResetForNewUpdate()
    {
        for ( SampledEvent const &evt : m_newEvents )
        {
            // Do not add immediate events to the active list
            if ( evt.IsAnimationEvent() && evt.m_animEventData.m_pEvent->IsImmediateEvent() )
            {
                continue;
            }

            // Only keep the most relevant version of the same event (by design we will received the same event multiple times due to state transitions)
            int32_t const foundIdx = VectorFindIndex( m_continuousEvents, evt );
            if ( foundIdx == InvalidIndex )
            {
                m_continuousEvents.emplace_back( evt );
            }
            else
            {
                if ( m_continuousEvents[foundIdx].IsLessRelevant( evt ) )
                {
                    m_continuousEvents[foundIdx] = evt;
                }
            }
        }

        m_newEvents.clear();
        m_sampledEvents.clear();
        m_endedContinuousEvents.clear();

        m_numAnimEventsSampled = m_numGraphEventsSampled = 0;
    }

    void SampledEventsBuffer::UpdateEventTracking()
    {
        TInlineVector<SampledEvent*, 20> maintainedEvents;

        for ( SampledEvent &evt : m_sampledEvents )
        {
            if ( evt.IsIgnored() )
            {
                continue;
            }

            // Check if this event is already in the active list
            int32_t const foundEvtIdx = VectorFindIndex( m_continuousEvents, evt );
            if ( foundEvtIdx != InvalidIndex )
            {
                // Add it to the maintenance list and only keep the most relevant version (by design we will received the same event multiple times due to state transitions)
                int32_t const foundMaintainedEvtIdx = VectorFindIndex( maintainedEvents, evt, [] ( SampledEvent* const& pValue, SampledEvent const& evt ) { return *pValue == evt; } );
                if ( foundMaintainedEvtIdx == InvalidIndex )
                {
                    maintainedEvents.emplace_back( &evt );
                }
                else // Keep the most relevant version of the event
                {
                    if ( maintainedEvents[foundMaintainedEvtIdx]->IsLessRelevant( evt ) )
                    {
                        maintainedEvents[foundMaintainedEvtIdx] = &evt;
                    }
                }

                m_continuousEvents.erase( m_continuousEvents.begin() + foundEvtIdx );
            }
            else // This is a new event
            {
                // We need to check that it hasnt already been moved to the maintained list
                int32_t const foundMaintainedEvtIdx = VectorFindIndex( maintainedEvents, evt, [] ( SampledEvent* const& pValue, SampledEvent const& evt ) { return *pValue == evt; }  );
                if ( foundMaintainedEvtIdx == InvalidIndex )
                {
                    m_newEvents.emplace_back( evt );
                }
                else // If it has, then keep the most relevant version of the event
                {
                    if ( maintainedEvents[foundMaintainedEvtIdx]->IsLessRelevant( evt ) )
                    {
                        maintainedEvents[foundMaintainedEvtIdx] = &evt;
                    }
                }
            }
        }

        // Set the list of exited event to those that were not maintained
        //-------------------------------------------------------------------------

        EE_ASSERT( m_endedContinuousEvents.empty() );
        m_endedContinuousEvents = m_continuousEvents;

        // Update the list of active events
        //-------------------------------------------------------------------------

        m_continuousEvents.clear();
        m_continuousEvents.reserve( maintainedEvents.size() );
        for ( SampledEvent *pEvt : maintainedEvents )
        {
            m_continuousEvents.emplace_back( *pEvt );
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        for ( SampledEvent const& evt : m_endedContinuousEvents )
        {
            EE_ASSERT( VectorFindIndex( m_newEvents, evt ) == InvalidIndex );
            EE_ASSERT( VectorFindIndex( m_continuousEvents, evt ) == InvalidIndex );
        }
        #endif
    }

    //-------------------------------------------------------------------------

    bool SampledEventsBuffer::ContainsGraphEvent( StringID ID, bool onlyFromActiveBranch ) const
    {
        if ( onlyFromActiveBranch )
        {
            for ( auto const& se : m_sampledEvents )
            {
                if ( !se.IsGraphEvent() )
                {
                    continue;
                }

                if ( !se.IsIgnored() && se.IsFromActiveBranch() && se.GetGraphEventID() == ID )
                {
                    return true;
                }
            }
        }
        else
        {
            for ( auto const& se : m_sampledEvents )
            {
                if ( !se.IsGraphEvent() )
                {
                    continue;
                }

                if ( !se.IsIgnored() && se.GetGraphEventID() == ID )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool SampledEventsBuffer::ContainsSpecificGraphEvent( GraphEventType eventType, StringID ID, bool onlyFromActiveBranch ) const
    {
        if ( onlyFromActiveBranch )
        {
            for ( auto const& se : m_sampledEvents )
            {
                if ( !se.IsGraphEvent() )
                {
                    continue;
                }

                if ( !se.IsIgnored() && se.IsFromActiveBranch() && se.GetGraphEventType() == eventType && se.GetGraphEventID() == ID )
                {
                    return true;
                }
            }
        }
        else
        {
            for ( auto const& se : m_sampledEvents )
            {
                if ( !se.IsGraphEvent() )
                {
                    continue;
                }

                if ( !se.IsIgnored() && se.GetGraphEventType() == eventType && se.GetGraphEventID() == ID )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool SampledEventsBuffer::ContainsGraphEvent( SampledEventRange const& range, StringID ID, bool onlyFromActiveBranch ) const
    {
        EE_ASSERT( IsValidRange( range ) );

        for ( int32_t i = range.m_startIdx; i < range.m_endIdx; i++ )
        {
            auto const& se = m_sampledEvents[i];

            if ( !se.IsGraphEvent() )
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

            if ( se.GetGraphEventID() == ID )
            {
                return true;
            }
        }

        return false;
    }

    bool SampledEventsBuffer::ContainsSpecificGraphEvent( SampledEventRange const& range, GraphEventType eventType, StringID ID, bool onlyFromActiveBranch ) const
    {
        EE_ASSERT( IsValidRange( range ) );

        for ( int32_t i = range.m_startIdx; i < range.m_endIdx; i++ )
        {
            auto const& se = m_sampledEvents[i];

            if ( !se.IsGraphEvent() )
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

            if ( se.GetGraphEventType() == eventType && se.GetGraphEventID() == ID )
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

        // Sample events for both sources and updated sampled event weights
        uint32_t const numEventsSource0 = eventRange0.GetLength();
        uint32_t const numEventsSource1 = eventRange1.GetLength();

        if ( ( numEventsSource0 + numEventsSource1 ) > 0 )
        {
            UpdateWeights( eventRange0, 1.0f - blendWeight );
            UpdateWeights( eventRange1, blendWeight );

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

        EE_ASSERT( IsValidRange( combinedRange ) );
        return combinedRange;
    }

    SampledEventRange SampledEventsBuffer::AppendBuffer( SampledEventsBuffer const& otherBuffer )
    {
        SampledEventRange newEventRange( (uint16_t) m_sampledEvents.size() );

        //-------------------------------------------------------------------------

        m_sampledEvents.reserve( m_sampledEvents.size() + otherBuffer.m_sampledEvents.size() );
        m_sampledEvents.insert( m_sampledEvents.end(), otherBuffer.m_sampledEvents.begin(), otherBuffer.m_sampledEvents.end() );

        m_numAnimEventsSampled += otherBuffer.m_numAnimEventsSampled;
        m_numGraphEventsSampled += otherBuffer.m_numGraphEventsSampled;

        //-------------------------------------------------------------------------

        newEventRange.m_endIdx = (uint16_t) m_sampledEvents.size();
        return newEventRange;
    }
}