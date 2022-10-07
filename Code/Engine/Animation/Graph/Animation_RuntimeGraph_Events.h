#pragma once

#include "Engine/_Module/API.h"
#include "System/Types/Percentage.h"
#include "System/Types/StringID.h"
#include "System/Types/BitFlags.h"
#include "System/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Event;

    //-------------------------------------------------------------------------
    // A sampled event from the graph
    //-------------------------------------------------------------------------

    struct EE_ENGINE_API SampledEvent
    {
        friend class SampledEventsBuffer;

        union EventData
        {
            EventData() : m_pEvent( nullptr ) {}
            EventData( Event const* pEvent ) : m_pEvent( pEvent ) {}
            EventData( StringID ID ) : m_stateEventID( ID ) {}

            Event const*                        m_pEvent = nullptr;
            StringID                            m_stateEventID;
        };

    public:

        enum class Flags
        {
            Ignored,                // Should we ignore this event?
            FromInactiveBranch,     // Is this event from an inactive branch in the graph/blend
            SampledInReverse,       // Was this event sampled from an animation playing in reverse

            // State events
            StateEntry,             // Is this a state transition in event
            StateExecute,           // Is this a "fully in state" event
            StateExit,              // Is this a state transition out event
            StateTimed,             // Timed event coming from a state
        };

    public:

        SampledEvent() = default;
        SampledEvent( int16_t sourceNodeIdx, Event const* pEvent, Percentage percentageThrough );
        SampledEvent( int16_t sourceNodeIdx, Flags stateEventType, StringID ID, Percentage percentageThrough );

        inline int16_t GetSourceNodeIndex() const { return m_sourceNodeIdx; }
        inline bool IsStateEvent() const { return m_flags.AreAnyFlagsSet( Flags::StateEntry, Flags::StateExecute, Flags::StateExit, Flags::StateTimed ); }
        inline bool IsFromActiveBranch() const { return !m_flags.IsFlagSet( Flags::FromInactiveBranch ); }
        inline StringID GetStateEventID() const { EE_ASSERT( IsStateEvent() ); return m_eventData.m_stateEventID; }
        inline float GetWeight() const { return m_weight; }
        inline void SetWeight( float weight ) { EE_ASSERT( weight >= 0.0f && weight <= 1.0f ); m_weight = weight; }
        inline Percentage GetPercentageThrough() const { return m_percentageThrough; }

        //-------------------------------------------------------------------------

        inline Event const* GetEvent() const { EE_ASSERT( !IsStateEvent() ); return m_eventData.m_pEvent; }

        // Checks if the sampled event is of a specified runtime type
        template<typename T>
        inline bool IsEventOfType() const
        {
            EE_ASSERT( !IsStateEvent() );
            return IsOfType<T>( m_eventData.m_pEvent );
        }

        // Returns the event cast to the desired type! Warning: this function assumes you know the exact type of the event!
        template<typename T>
        inline T const* GetEvent() const
        {
            EE_ASSERT( !IsStateEvent() );
            return Cast<T>( m_eventData.m_pEvent );
        }

        // Attempts to return the event cast to the desired type! This function will return null if the event cant be cast successfully
        template<typename T>
        inline T const* TryGetEvent() const
        {
            EE_ASSERT( !IsStateEvent() );
            return TryCast<T>( m_eventData.m_pEvent );
        }

        // Flags
        //-------------------------------------------------------------------------

        inline TBitFlags<Flags>& GetFlags() { return m_flags; }
        inline TBitFlags<Flags> const& GetFlags() const { return m_flags; }
        inline void SetFlag( Flags flag, bool value ) { m_flags.SetFlag( flag, value ); }

    private:

        EventData                           m_eventData;                    // The event data
        float                               m_weight = 1.0f;                // The weight of the event when sampled
        Percentage                          m_percentageThrough = 1.0f;     // The percentage through the event we were when sampling
        TBitFlags<Flags>                    m_flags;                        // Misc flags
        int16_t                             m_sourceNodeIdx = InvalidIndex; // The index of the node that this event was sampled from
    };

    //-------------------------------------------------------------------------
    // Sample Event Range
    //-------------------------------------------------------------------------
    // A range of indices into the sampled events buffer: [startIndex, endIndex);
    // The end index is not part of each range, it is the start index for the next range or the end of the sampled events buffer

    struct SampledEventRange
    {
        SampledEventRange() = default;
        EE_FORCE_INLINE SampledEventRange( int16_t index ) : m_startIdx( index ), m_endIdx( index ) {}
        EE_FORCE_INLINE SampledEventRange( int16_t startIndex, int16_t endIndex ) : m_startIdx( startIndex ), m_endIdx( endIndex ) {}

        EE_FORCE_INLINE bool IsValid() const { return m_startIdx != InvalidIndex && m_endIdx >= m_startIdx; }
        EE_FORCE_INLINE int32_t GetLength() const { return m_endIdx - m_startIdx; }
        EE_FORCE_INLINE void Reset() { m_startIdx = m_endIdx = InvalidIndex; }

    public:

        int16_t                               m_startIdx = InvalidIndex;
        int16_t                               m_endIdx = InvalidIndex;
    };

    //-------------------------------------------------------------------------
    // Sample Event Buffer
    //-------------------------------------------------------------------------

    class EE_ENGINE_API SampledEventsBuffer
    {
    public:

        inline void Reset() { m_events.clear(); }

        inline TVector<SampledEvent> const& GetEvents() const { return m_events; }

        inline int16_t GetNumEvents() const { return (int16_t) m_events.size(); }

        // Is the supplied range valid for the current state of the buffer?
        inline bool IsValidRange( SampledEventRange range ) const
        {
            if ( m_events.empty() )
            {
                return range.m_startIdx == 0 && range.m_endIdx == 0;
            }
            else
            {
                return range.m_startIdx >= 0 && range.m_endIdx <= m_events.size();
            }
        }

        // Update all event weights for the supplied range
        inline void UpdateWeights( SampledEventRange range, float weightMultiplier )
        {
            EE_ASSERT( range.m_startIdx >= 0 && range.m_endIdx <= m_events.size() );
            for ( int16_t i = range.m_startIdx; i < range.m_endIdx; i++ )
            {
                m_events[i].m_weight *= weightMultiplier;
            }
        }

        // Set a flag on all events for the supplied range
        inline void SetFlag( SampledEventRange range, SampledEvent::Flags flag )
        {
            EE_ASSERT( range.m_startIdx >= 0 && range.m_endIdx < m_events.size() );
            for ( int16_t i = range.m_startIdx; i < range.m_endIdx; i++ )
            {
                m_events[i].m_flags.SetFlag( flag );
            }
        }

        //-------------------------------------------------------------------------

        inline SampledEvent& EmplaceAnimEvent( int16_t sourceNodeIdx, Event const* pEvent, Percentage percentageThrough )
        {
            return m_events.emplace_back( sourceNodeIdx, pEvent, percentageThrough );
        }

        inline SampledEvent& EmplaceStateEvent( int16_t sourceNodeIdx, SampledEvent::Flags stateEventType, StringID ID )
        {
            EE_ASSERT( stateEventType >= SampledEvent::Flags::StateEntry );
            return m_events.emplace_back( sourceNodeIdx, stateEventType, ID, 1.0f );
        }

        // Helpers
        //-------------------------------------------------------------------------
        // These are not the cheapest, so use sparingly. These functions also take the ignored flag into account.

        inline bool ContainsStateEvent( SampledEventRange const& range, StringID ID, bool onlyFromActiveBranch = false ) const
        {
            EE_ASSERT( range.m_startIdx >= 0 && range.m_endIdx < m_events.size() );

            for ( int32_t i = range.m_startIdx; i < range.m_endIdx; i++ )
            {
                auto& event = m_events[i];

                if ( event.m_flags.IsFlagSet( SampledEvent::Flags::Ignored ) )
                {
                    continue;
                }

                if ( onlyFromActiveBranch && event.m_flags.IsFlagSet( SampledEvent::Flags::FromInactiveBranch ) )
                {
                    continue;
                }

                if ( event.IsStateEvent() && event.GetStateEventID() == ID )
                {
                    return true;
                }
            }

            return false;
        }

        inline bool ContainsSpecificStateEvent( SampledEventRange const& range, StringID ID, SampledEvent::Flags flag, bool onlyFromActiveBranch = false ) const
        {
            EE_ASSERT( range.m_startIdx >= 0 && range.m_endIdx < m_events.size() );

            for ( int32_t i = range.m_startIdx; i < range.m_endIdx; i++ )
            {
                auto& event = m_events[i];

                if ( event.m_flags.IsFlagSet( SampledEvent::Flags::Ignored ) )
                {
                    continue;
                }

                if ( onlyFromActiveBranch && event.m_flags.IsFlagSet( SampledEvent::Flags::FromInactiveBranch ) )
                {
                    continue;
                }

                if ( event.IsStateEvent() && event.m_flags.IsFlagSet( flag ) && !event.m_flags.IsFlagSet( SampledEvent::Flags::FromInactiveBranch ) && event.GetStateEventID() == ID )
                {
                    return true;
                }
            }

            return false;
        }

        inline bool ContainsStateEvent( StringID ID, bool onlyFromActiveBranch = false ) const
        {
            return ContainsStateEvent( SampledEventRange( 0, (int16_t) m_events.size() ), ID, onlyFromActiveBranch );
        }

        inline bool ContainsSpecificStateEvent( StringID ID, SampledEvent::Flags flag, bool onlyFromActiveBranch = false ) const
        {
            return ContainsSpecificStateEvent( SampledEventRange( 0, (int16_t) m_events.size() ), ID, flag, onlyFromActiveBranch );
        }

        // Operators
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE TVector<SampledEvent>::iterator begin() { return m_events.begin(); }
        EE_FORCE_INLINE TVector<SampledEvent>::iterator end() { return m_events.end(); }
        EE_FORCE_INLINE TVector<SampledEvent>::const_iterator begin() const { return m_events.begin(); }
        EE_FORCE_INLINE TVector<SampledEvent>::const_iterator end() const{ return m_events.end(); }
        EE_FORCE_INLINE SampledEvent& operator[]( uint32_t i ) { EE_ASSERT( i < m_events.size() ); return m_events[i]; }
        EE_FORCE_INLINE SampledEvent const& operator[]( uint32_t i ) const { EE_ASSERT( i < m_events.size() ); return m_events[i]; }

        // Misc
        //-------------------------------------------------------------------------

        inline void Append( SampledEventsBuffer const& otherBuffer )
        {
            m_events.reserve( m_events.size() + otherBuffer.m_events.size() );
            m_events.insert( m_events.end(), otherBuffer.m_events.begin(), otherBuffer.m_events.end() );
        }

    public:

        TVector<SampledEvent>               m_events;
    };

    //-------------------------------------------------------------------------
    // Helpers
    //-------------------------------------------------------------------------

    [[nodiscard]] SampledEventRange CombineAndUpdateEvents( SampledEventsBuffer& sampledEventsBuffer, SampledEventRange const& eventRange0, SampledEventRange const& eventRange1, float const blendWeight );
}