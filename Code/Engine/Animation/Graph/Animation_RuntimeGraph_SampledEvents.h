#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Animation/AnimationSourcePath.h"
#include "Base/Types/Percentage.h"
#include "Base/Types/StringID.h"
#include "Base/Types/BitFlags.h"
#include "Base/Types/Arrays.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Event;

    //-------------------------------------------------------------------------

    enum class GraphEventType : uint8_t
    {
        Entry = 0,         // Is this a state transition in event
        FullyInState,      // Is this a "fully in state" event
        Exit,              // Is this a state transition out event
        Timed,             // Timed event coming from a state
        Generic,           // Event emitted from non-state nodes
    };

    // Use this enum when performing event type comparisons
    enum class GraphEventTypeCondition : uint8_t
    {
        EE_REFLECT_ENUM

        Entry = 0,         // Is this a state transition in event
        FullyInState,      // Is this a "fully in state" event
        Exit,              // Is this a state transition out event
        Timed,             // Timed event coming from a state
        Generic,           // Event emitted from non-state nodes
        Any                // Any kind of graph event
    };

    #if EE_DEVELOPMENT_TOOLS
    EE_ENGINE_API char const* GetNameForGraphEventType( GraphEventType type );
    #endif

    EE_FORCE_INLINE bool DoesGraphEventTypesMatchCondition( GraphEventTypeCondition condition, GraphEventType eventType )
    {
        if ( condition == GraphEventTypeCondition::Any ) 
        {
            return true;
        }

        return (uint8_t) condition == (uint8_t) eventType;
    }

    //-------------------------------------------------------------------------
    // A sampled event from the graph
    //-------------------------------------------------------------------------

    struct EE_ENGINE_API SampledEvent
    {
        friend class SampledEventsBuffer;

    public:

        struct AnimationEventData
        {
            Event const*                        m_pEvent = nullptr;
            Percentage                          m_percentageThrough = 1.0f;     // The percentage through the event when we sampled it
            Seconds                             m_eventDuration = -1.0f;
        };

        struct GraphEventData
        {
            StringID                            m_ID;
            GraphEventType                      m_type;
        };

    public:

        SampledEvent();
        SampledEvent( SampledEvent const &rhs );
        explicit SampledEvent( SourcePath const& path, float weight, bool isFromActiveBranch, GraphEventType eventType, StringID eventID );
        explicit SampledEvent( SourcePath const& path, float weight, bool isFromActiveBranch, Event const* pEvent, Percentage percentageThrough, Seconds eventDuration );

        // Sampled Event
        //-------------------------------------------------------------------------

        SourcePath const& GetSourcePath() const { return m_sourcePath; }

        inline bool IsAnimationEvent() const { return !m_isGraphEvent; }
        inline bool IsGraphEvent() const { return m_isGraphEvent; }

        inline bool IsFromActiveBranch() const { return m_isFromActiveBranch; }
        inline bool IsIgnored() const { return m_isIgnored; }
        inline float GetWeight() const { return m_weight; }

        // Animation Events
        //-------------------------------------------------------------------------

        // Get the raw animation event
        inline Event const* GetEvent() const { EE_ASSERT( IsAnimationEvent() ); return m_animEventData.m_pEvent; }

        // Get the percentage through the event when it was sampled
        inline Percentage GetPercentageThrough() const { EE_ASSERT( IsAnimationEvent() ); return m_animEventData.m_percentageThrough; }

        // Checks if the sampled event is of a specified runtime type
        template<typename T>
        inline bool IsEventOfType() const { EE_ASSERT( IsAnimationEvent() ); return IsOfType<T>( m_animEventData.m_pEvent ); }

        // Returns the event cast to the desired type! Warning: this function assumes you know the exact type of the event!
        template<typename T>
        inline T const* GetEvent() const { EE_ASSERT( IsAnimationEvent() ); return Cast<T>( m_animEventData.m_pEvent ); }

        // Attempts to return the event cast to the desired type! This function will return null if the event cant be cast successfully
        template<typename T>
        inline T const* TryGetEvent() const { return IsAnimationEvent() ? TryCast<T>( m_animEventData.m_pEvent ) : nullptr; }

        // Graph Events
        //-------------------------------------------------------------------------

        inline StringID GetGraphEventID() const { EE_ASSERT( IsGraphEvent() ); return m_graphEventData.m_ID; }
        inline GraphEventType GetGraphEventType() const { EE_ASSERT( IsGraphEvent() ); return m_graphEventData.m_type; }

        inline bool IsEntryEvent() const { EE_ASSERT( IsGraphEvent() ); return m_graphEventData.m_type == GraphEventType::Entry; }
        inline bool IsFullyInStateEvent() const { EE_ASSERT( IsGraphEvent() ); return m_graphEventData.m_type == GraphEventType::FullyInState; }
        inline bool IsExitEvent() const { EE_ASSERT( IsGraphEvent() ); return m_graphEventData.m_type == GraphEventType::Exit; }
        inline bool IsTimedEvent() const { EE_ASSERT( IsGraphEvent() ); return m_graphEventData.m_type == GraphEventType::Timed; }
        inline bool IsStateEvent() const { EE_ASSERT( IsGraphEvent() ); return m_graphEventData.m_type <= GraphEventType::Timed; }
        inline bool IsGenericEvent() const { EE_ASSERT( IsGraphEvent() ); return m_graphEventData.m_type == GraphEventType::Generic; }

        // Operators/Comparison
        //-------------------------------------------------------------------------

        SampledEvent &operator=( SampledEvent const &rhs );

        // Only compares path and payload, ignores all transient elements
        bool operator==( SampledEvent const &rhs ) const { return m_uniqueID == rhs.m_uniqueID; }

        // Only compares path and payload, ignores all transient elements
        bool operator!=( SampledEvent const &rhs ) const { return m_uniqueID != rhs.m_uniqueID; }

        // If this event less relevant than the other one, checks ignored state, active branch and weight
        bool IsLessRelevant( SampledEvent const &rhs ) const;

    private:

        uint64_t CalculateUniqueID( SourcePath const& path ) const;

    private:

        uint64_t                            m_uniqueID = 0;     // Use for fast comparison
        SourcePath                          m_sourcePath;
        float                               m_weight = 1.0f;    // The weight of the event when sampled
        bool                                m_isFromActiveBranch = false;
        bool                                m_isIgnored = false;
        bool                                m_isGraphEvent = false;

        union
        {
            AnimationEventData              m_animEventData;
            GraphEventData                  m_graphEventData;
        };
    };

    //-------------------------------------------------------------------------
    // Sample Event Range
    //-------------------------------------------------------------------------
    // A range of indices into the sampled events buffer: [startIndex, endIndex);
    // The end index is not part of each range, it is the start index for the next range or the end of the sampled events buffer

    struct SampledEventRange
    {
        SampledEventRange() = default;
        EE_FORCE_INLINE explicit SampledEventRange( int16_t index ) : m_startIdx( index ), m_endIdx( index ) {}
        EE_FORCE_INLINE explicit SampledEventRange( int16_t startIndex, int16_t endIndex ) : m_startIdx( startIndex ), m_endIdx( endIndex ) {}

        EE_FORCE_INLINE bool IsValid() const { return m_startIdx != InvalidIndex && m_endIdx >= m_startIdx; }
        EE_FORCE_INLINE int32_t GetLength() const { return m_endIdx - m_startIdx; }
        EE_FORCE_INLINE void Reset() { m_startIdx = m_endIdx = InvalidIndex; }

    public:

        int16_t                               m_startIdx = InvalidIndex;

        // The end index is not part of each range, it is the start index for the next range or the end of the sampled events buffer
        int16_t                               m_endIdx = InvalidIndex;
    };

    //-------------------------------------------------------------------------
    // Sample Event Buffer
    //-------------------------------------------------------------------------

    class EE_ENGINE_API SampledEventsBuffer
    {
        friend class AnimationDebugView;

    public:

        // Empty the buffer
        void Clear();

        // Reset the buffer for a new update
        void ResetForNewUpdate();

        // Get the total number of sampled events (for both anim and graph events )
        inline int16_t GetNumSampledEvents() const { return (int16_t) m_sampledEvents.size(); }

        // Get the total number of sampled events (for both anim and graph events )
        inline TVector<SampledEvent> const& GetSampledEvents() const { return m_sampledEvents; }

        // Get the event at specified index
        EE_FORCE_INLINE SampledEvent& GetEvent( uint32_t i ) { EE_ASSERT( i < m_sampledEvents.size() ); return m_sampledEvents[i]; }

        // Get the event at specified index
        EE_FORCE_INLINE SampledEvent const& GetEvent( uint32_t i ) const { EE_ASSERT( i < m_sampledEvents.size() ); return m_sampledEvents[i]; }

        // Is the supplied range valid for the current state of the buffer?
        inline bool IsValidRange( SampledEventRange range ) const
        {
            if ( m_sampledEvents.empty() )
            {
                return range.m_startIdx == 0 && range.m_endIdx == 0;
            }
            else
            {
                return range.m_startIdx >= 0 && range.m_endIdx <= m_sampledEvents.size();
            }
        }

        // Update all event weights for the supplied range
        inline void UpdateWeights( SampledEventRange range, float weightMultiplier )
        {
            EE_ASSERT( IsValidRange( range ) );
            for ( int16_t i = range.m_startIdx; i < range.m_endIdx; i++ )
            {
                m_sampledEvents[i].m_weight *= weightMultiplier;
            }
        }

        // Set a flag on all events for the supplied range
        inline void MarkEvents( SampledEventRange range, bool isIgnored, bool isFromActiveBranch )
        {
            EE_ASSERT( IsValidRange( range ) );
            for ( int16_t i = range.m_startIdx; i < range.m_endIdx; i++ )
            {
                m_sampledEvents[i].m_isIgnored = isIgnored;
                m_sampledEvents[i].m_isFromActiveBranch = isFromActiveBranch;
            }
        }

        // Mark all events in the range as ignored
        inline void MarkEventsAsIgnored( SampledEventRange range )
        {
            EE_ASSERT( IsValidRange( range ) );
            for ( int16_t i = range.m_startIdx; i < range.m_endIdx; i++ )
            {
                m_sampledEvents[i].m_isIgnored = true;
            }
        }

        // Mark all events in the range as ignored and clear their weights
        inline void MarkEventsAsIgnoredAndClearWeights( SampledEventRange range )
        {
            EE_ASSERT( IsValidRange( range ) );
            for ( int16_t i = range.m_startIdx; i < range.m_endIdx; i++ )
            {
                m_sampledEvents[i].m_isIgnored = true;
                m_sampledEvents[i].m_weight = 0.0f;
            }
        }

        // Mark all the events in the range as coming from an inactive branch
        inline void MarkEventsAsFromInactiveBranch( SampledEventRange range )
        {
            EE_ASSERT( IsValidRange( range ) );
            for ( int16_t i = range.m_startIdx; i < range.m_endIdx; i++ )
            {
                m_sampledEvents[i].m_isFromActiveBranch = false;
            }
        }

        // Blend two neighboring event ranges together
        [[nodiscard]] SampledEventRange BlendEventRanges( SampledEventRange const& eventRange0, SampledEventRange const& eventRange1, float const blendWeight );

        // Append another buffer and return the sampled events range for the newly added events
        SampledEventRange AppendBuffer( SampledEventsBuffer const& otherBuffer );

        // Animation Events
        //-------------------------------------------------------------------------

        inline SampledEvent& EmplaceAnimationEvent( SourcePath const& path, Event const* pEvent, Percentage percentageThrough, Seconds eventDuration, bool isFromActiveBranch = true )
        {
            m_numAnimEventsSampled++;
            return m_sampledEvents.emplace_back( path, 1.0f, isFromActiveBranch, pEvent, percentageThrough, eventDuration );
        }

        inline int16_t GetNumAnimationEventsSampled() const { return m_numAnimEventsSampled; }

        // Graph Events
        //-------------------------------------------------------------------------

        inline SampledEvent& EmplaceGraphEvent( SourcePath const& path, GraphEventType type, StringID ID, bool isFromActiveBranch )
        {
            m_numGraphEventsSampled++;
            return m_sampledEvents.emplace_back( path, 1.0f, isFromActiveBranch, type, ID );
        }

        inline int16_t GetNumGraphEventsSampled() const { return m_numGraphEventsSampled; }

        bool ContainsGraphEvent( StringID ID, bool onlyFromActiveBranch = false ) const;
        bool ContainsGraphEvent( SampledEventRange const& range, StringID ID, bool onlyFromActiveBranch = false ) const;
        bool ContainsSpecificGraphEvent( GraphEventType eventType, StringID ID, bool onlyFromActiveBranch = false ) const;
        bool ContainsSpecificGraphEvent( SampledEventRange const& range, GraphEventType eventType, StringID ID, bool onlyFromActiveBranch = false ) const;

        // Operators
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE TVector<SampledEvent>::iterator begin() { return m_sampledEvents.begin(); }
        EE_FORCE_INLINE TVector<SampledEvent>::iterator end() { return m_sampledEvents.end(); }
        EE_FORCE_INLINE TVector<SampledEvent>::const_iterator begin() const { return m_sampledEvents.begin(); }
        EE_FORCE_INLINE TVector<SampledEvent>::const_iterator end() const{ return m_sampledEvents.end(); }
        EE_FORCE_INLINE SampledEvent& operator[]( uint32_t i ) { return GetEvent(i); }
        EE_FORCE_INLINE SampledEvent const& operator[]( uint32_t i ) const { return GetEvent( i ); }

        // Event Tracking
        //-------------------------------------------------------------------------
        // There's a bunch of event tracking to provide client code additional information about what has occurred
        // Use the below functions to get the list of new events vs events that we were tracking but have stopped
        // This is useful for things like stopping sounds/particle systems once we stop sampling an event
        // 
        // NOTE!	You may still get multiple events with the same ID or payloads since they come from different sources.
        //			So you may have a graph event with the same ID both exiting and entering on the same update.
        //			This is the responsibility of the client code to figure out and deal with.

        // Update the event tracking lists
        void UpdateEventTracking();

        // Get the list of new events that have been received that we didn't have the previous update
        // Immediate/Instant events will only ever be in this list
        TInlineVector<SampledEvent, 20> const& GetNewlyReceivedEvents() const { return m_newEvents; }

        // Get the list of event that are continuing from a previous update (i.e. were received (entered) in a previous update and are still being received)
        // This list should only ever contain duration anim events and graph events
        TInlineVector<SampledEvent, 20> const& GetContinuousEvents() const { return m_continuousEvents; }

        // Get the list of previously continuing events that we have stopped receiving
        // This list should only ever contain duration anim events and graph events
        TInlineVector<SampledEvent, 20> const& GetEndedContinuousEvents() const { return m_endedContinuousEvents; }

    public:

        TVector<SampledEvent>                       m_sampledEvents;
        int16_t                                     m_numAnimEventsSampled = 0;
        int16_t                                     m_numGraphEventsSampled = 0;

        TInlineVector<SampledEvent, 20>             m_newEvents;
        TInlineVector<SampledEvent, 20>             m_continuousEvents;
        TInlineVector<SampledEvent, 20>             m_endedContinuousEvents;

    };
}