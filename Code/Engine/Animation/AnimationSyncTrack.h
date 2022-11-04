#pragma once

#include "Engine/_Module/API.h"
#include "System/Types/Percentage.h"
#include "System/Types/Arrays.h"
#include "System/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    //-------------------------------------------------------------------------
    // Sync Track Time
    //-------------------------------------------------------------------------
    // Represent a position on a sync track in terms of event time (event idx and the percentage through that event)
    // This is primary mechanism for syncing together multiple animations

    struct SyncTrackTime
    {
        SyncTrackTime() = default;

        SyncTrackTime( int32_t inEventIndex, float inPercentageThrough )
            : m_eventIdx( inEventIndex )
            , m_percentageThrough( inPercentageThrough )
        {}

        inline float ToFloat() const
        {
            EE_ASSERT( m_percentageThrough >= 0.0f && m_percentageThrough <= 1.0f );
            return m_percentageThrough.ToFloat() + m_eventIdx;
        }

        int32_t                             m_eventIdx = 0;
        Percentage                          m_percentageThrough = 0;
    };

    //-------------------------------------------------------------------------
    // Sync Track Time Range
    //-------------------------------------------------------------------------
    // A time range specified in sync track event time, this is useful to specify an animation update range so as
    // to perfect synchronize a given update

    struct SyncTrackTimeRange
    {
        SyncTrackTimeRange() = default;

        SyncTrackTimeRange( SyncTrackTime const& time )
            : m_startTime( time )
            , m_endTime( time )
        {}

        SyncTrackTimeRange( SyncTrackTime const& InStartTime, SyncTrackTime const& InEndTime )
            : m_startTime( InStartTime )
            , m_endTime( InEndTime )
        {}

        inline void Reset( SyncTrackTime const& InTime = SyncTrackTime() )
        {
            m_startTime = InTime;
            m_endTime = InTime;
        }

    public:

        SyncTrackTime                       m_startTime;
        SyncTrackTime                       m_endTime;
    };

    //-------------------------------------------------------------------------
    // Animation Sync Track
    //-------------------------------------------------------------------------
    // This is used to synchronize multiple animations together based on some set of events
    // A sync track simply specifies the sync ranges in terms of events.
    // TODO: explain how sync tracks work in detail here

    class EE_ENGINE_API SyncTrack
    {
        EE_SERIALIZE( m_syncEvents, m_startEventOffset );

    public:

        // Calculates a new duration resulting from a blend of two sync tracks
        inline static float CalculateDurationSynchronized( float const duration0, float const duration1, int32_t const numEvents0, int32_t const numEvents1, int32_t const eventsLCM, float const blendWeight )
        {
            EE_ASSERT( numEvents0 > 0 && numEvents1 > 0 && eventsLCM > 0 );
            float const scaledDuration0 = duration0 * ( float( eventsLCM ) / numEvents0 );
            float const scaledDuration1 = duration1 * ( float( eventsLCM ) / numEvents1 );
            float const duration = Math::Lerp( scaledDuration0, scaledDuration1, blendWeight );
            return duration;
        }

        // A default sync-track with a single event
        static SyncTrack const s_defaultTrack;

        //-------------------------------------------------------------------------

        // A simple marker used to instantiate a sync track
        struct EventMarker
        {
            EE_SERIALIZE( m_ID, m_startTime );

            EventMarker() = default;

            EventMarker( Percentage const startTime, StringID ID )
                : m_startTime( startTime )
                , m_ID( ID )
            {}

            inline bool operator< ( EventMarker const& rhs ) const { return m_startTime < rhs.m_startTime; }

            Percentage                      m_startTime = 0.0f;
            StringID                        m_ID;
        };

    private:

        // A sync event
        struct Event
        {
            EE_SERIALIZE( m_ID, m_startTime, m_duration );

            Event() = default;

            Event( StringID ID, Percentage const startTime, Percentage duration = 0.0f )
                : m_ID( ID )
                , m_startTime( startTime )
                , m_duration( duration )
            {}

            StringID                        m_ID;
            Percentage                      m_startTime = 0.0f;
            Percentage                      m_duration = 1.0f;
        };

    public:

        SyncTrack();

        SyncTrack( TInlineVector<EventMarker, 10> const& inEvents, int32_t startEventOffset = 0 );

        // Create by blending two existing sync tracks. Only blends durations as it is meaningless to try and blend start times. Start times match up with blended durations.
        SyncTrack( SyncTrack const& track0, SyncTrack const& track1, float const blendWeight );

        inline int32_t GetNumEvents() const { return (int32_t) m_syncEvents.size(); }

        // The all the events in this sync track
        inline TInlineVector<Event, 10> const& GetEvents() const { return m_syncEvents; }

        // Does this sync track artificially start at a different event than the first one
        inline bool HasStartOffset() const { return m_startEventOffset != 0; }
        inline int32_t GetStartEventOffset() const { return m_startEventOffset; }

        // Get the event at specified index, includes offset
        inline Event const& GetEvent( int32_t i ) const
        {
            auto adjustedIndex = ClampIndexToTrack( i + m_startEventOffset );
            return m_syncEvents[adjustedIndex];
        }

        // Get the event at specified index, excludes offset
        inline Event const& GetEventWithoutOffset( int32_t i ) const
        {
            auto AdjustedIndex = ClampIndexToTrack( i );
            return m_syncEvents[AdjustedIndex];
        }

        // Get the duration of a specified event as a percentage of the whole track
        inline Percentage GetEventDuration( int32_t const eventIndex ) const { return GetEventDuration( eventIndex, true ); }
        inline Percentage GetEventDurationWithoutOffset( int32_t const eventIndex ) const { return GetEventDuration( eventIndex, false ); }

        // Time step
        //-------------------------------------------------------------------------

        // Calculates the track time that will result from a given start time and percentage time delta on the track
        inline SyncTrackTime UpdateEventTime( SyncTrackTime const& startTime, Percentage const deltaTime ) const
        {
            Percentage const startPercentageThrough = GetPercentageThrough( startTime, true );
            Percentage const endPercentageThrough = startPercentageThrough + deltaTime;
            return GetTime( endPercentageThrough, false );
        }

        // Calculates the track time that will result from a given start time and percentage time delta on the track (ignores the start offset)
        inline SyncTrackTime UpdateEventPositionWithoutOffset( SyncTrackTime const& startTime, Percentage const deltaTime ) const
        {
            Percentage const startPercentageThrough = GetPercentageThrough( startTime, false );
            Percentage const endPercentageThrough = startPercentageThrough + deltaTime;
            return GetTime( endPercentageThrough, true );
        }

        // Time Info
        //-------------------------------------------------------------------------

        inline SyncTrackTime GetStartTime() const { return SyncTrackTime( 0, 0.0f ); }
        inline SyncTrackTime GetEndTime() const { return SyncTrackTime( GetNumEvents() - 1, 1.0f ); }
        inline SyncTrackTime GetOffsetStartTime() const { return SyncTrackTime( m_startEventOffset, 0.0f ); }
        inline SyncTrackTime GetOffsetEndTime() const { return SyncTrackTime( ClampIndexToTrack( GetNumEvents() + m_startEventOffset - 1 ), 1.0f ); }

        // Return the percentage of the sync track covered by the specified range
        Percentage CalculatePercentageCovered( SyncTrackTime const& startTime, SyncTrackTime const& endTime ) const;

        // Return the percentage of the sync track covered by the specified range
        inline Percentage CalculatePercentageCovered( SyncTrackTimeRange const& range ) const { return CalculatePercentageCovered( range.m_startTime, range.m_endTime ); }

        // Time conversions
        //-------------------------------------------------------------------------

        // Get the sync track time for a percentage through the track - starting from the 'first' event i.e. the event offset
        inline SyncTrackTime GetTime( Percentage const percentage ) const { return GetTime( percentage, true ); }

        // Get the sync track time for a percentage through the track - starting at the actual first event
        inline SyncTrackTime GetTimeWithoutOffset( Percentage const percentage ) const { return GetTime( percentage, false ); }

        // Get the percentage through the track for a given tracking - starting from the 'first' event i.e. the event offset
        inline Percentage GetPercentageThrough( SyncTrackTime const& time ) const { return GetPercentageThrough( time, true ); }

        // Get the percentage through the track for a given tracking - starting at the actual first event
        inline Percentage GetPercentageThroughWithoutOffset( SyncTrackTime const& time ) const { return GetPercentageThrough( time, false ); }

    private:

        inline int32_t ClampIndexToTrack( int32_t const eventIndex ) const
        {
            EE_ASSERT( !m_syncEvents.empty() );

            int32_t const numEvents = GetNumEvents();
            int32_t clampedIndex = eventIndex % numEvents;
            if ( clampedIndex < 0 )
            {
                clampedIndex += numEvents;
            }

            return clampedIndex;
        }

        SyncTrackTime GetTime( Percentage const percentage, bool bWithOffset ) const;
        Percentage GetPercentageThrough( SyncTrackTime const& time, bool bWithOffset ) const;
        Percentage GetEventDuration( int32_t const eventIndex, bool bWithOffset ) const;

    private:

        TInlineVector<Event, 10>        m_syncEvents;               // The number and position of the sync periods
        int32_t                           m_startEventOffset = 0;     // The offset for which event signifies the track of the track
    };
}