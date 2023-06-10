#pragma once

#include "Engine/_Module/API.h"
#include "System/Types/Percentage.h"
#include "System/Types/StringID.h"
#include "System/Types/BitFlags.h"
#include "System/Types/Arrays.h"
#include "System/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Event;

    //-------------------------------------------------------------------------

    enum class StateEventType : uint8_t
    {
        Entry = 0,         // Is this a state transition in event
        FullyInState,      // Is this a "fully in state" event
        Exit,              // Is this a state transition out event
        Timed,             // Timed event coming from a state
    };

    // Use this enum when performing event type comparisons
    enum class StateEventTypeCondition : uint8_t
    {
        EE_REFLECT_ENUM

        Entry = 0,         // Is this a state transition in event
        FullyInState,      // Is this a "fully in state" event
        Exit,              // Is this a state transition out event
        Timed,             // Timed event coming from a state
        Any                // Any kind of state event
    };

    EE_FORCE_INLINE bool DoesStateEventTypesMatchCondition( StateEventTypeCondition condition, StateEventType eventType )
    {
        if ( condition == StateEventTypeCondition::Any ) 
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

        struct AnimationData
        {
            Event const*                        m_pEvent = nullptr;
            Percentage                          m_percentageThrough = 1.0f;     // The percentage through the event when we sampled it
        };

        struct StateData
        {
            StringID                            m_ID;
            StateEventType                      m_type;
        };

    public:

        explicit SampledEvent( int16_t sourceNodeIdx, float weight, bool isFromActiveBranch, StateEventType eventType, StringID eventID )
            : m_weight( weight )
            , m_sourceNodeIdx( sourceNodeIdx )
            , m_isFromActiveBranch( isFromActiveBranch )
            , m_isIgnored( false )
            , m_isStateEvent( true )
        {
            EE_ASSERT( eventID.IsValid() );
            m_stateData.m_ID = eventID;
            m_stateData.m_type = eventType;
        }

        explicit SampledEvent( int16_t sourceNodeIdx, float weight, bool isFromActiveBranch, Event const* pEvent, Percentage percentageThrough )
            : m_weight( weight )
            , m_sourceNodeIdx( sourceNodeIdx )
            , m_isFromActiveBranch( isFromActiveBranch )
            , m_isIgnored( false )
            , m_isStateEvent( false )
        {
            EE_ASSERT( pEvent != nullptr );
            EE_ASSERT( percentageThrough >= 0 && percentageThrough <= 1.0f );
            m_animData.m_pEvent = pEvent;
            m_animData.m_percentageThrough = percentageThrough;
        }

        // Sampled Event
        //-------------------------------------------------------------------------

        inline int16_t GetSourceNodeIndex() const { return m_sourceNodeIdx; }

        inline bool IsAnimationEvent() const { return !m_isStateEvent; }
        inline bool IsStateEvent() const { return m_isStateEvent; }

        inline bool IsFromActiveBranch() const { return m_isFromActiveBranch; }
        inline bool IsIgnored() const { return m_isIgnored; }
        inline float GetWeight() const { return m_weight; }

        // Animation Events
        //-------------------------------------------------------------------------

        // Get the raw animation event
        inline Event const* GetEvent() const { EE_ASSERT( IsAnimationEvent() ); return m_animData.m_pEvent; }

        // Get the percentage through the event when it was sampled
        inline Percentage GetPercentageThrough() const { EE_ASSERT( IsAnimationEvent() ); return m_animData.m_percentageThrough; }

        // Checks if the sampled event is of a specified runtime type
        template<typename T>
        inline bool IsEventOfType() const { EE_ASSERT( IsAnimationEvent() ); return IsOfType<T>( m_animData.m_pEvent ); }

        // Returns the event cast to the desired type! Warning: this function assumes you know the exact type of the event!
        template<typename T>
        inline T const* GetEvent() const { EE_ASSERT( IsAnimationEvent() ); return Cast<T>( m_animData.m_pEvent ); }

        // Attempts to return the event cast to the desired type! This function will return null if the event cant be cast successfully
        template<typename T>
        inline T const* TryGetEvent() const { return IsAnimationEvent() ? TryCast<T>( m_animData.m_pEvent ) : nullptr; }

        // State Events
        //-------------------------------------------------------------------------

        inline StringID GetStateEventID() const { EE_ASSERT( IsStateEvent() ); return m_stateData.m_ID; }
        inline StateEventType GetStateEventType() const { EE_ASSERT( IsStateEvent() ); return m_stateData.m_type; }

        inline bool IsEntryEvent() const { EE_ASSERT( IsStateEvent() ); return m_stateData.m_type == StateEventType::Entry; }
        inline bool IsFullyInStateEvent() const { EE_ASSERT( IsStateEvent() ); return m_stateData.m_type == StateEventType::FullyInState; }
        inline bool IsExitEvent() const { EE_ASSERT( IsStateEvent() ); return m_stateData.m_type == StateEventType::Exit; }
        inline bool IsTimedEvent() const { EE_ASSERT( IsStateEvent() ); return m_stateData.m_type == StateEventType::Timed; }

    private:

        float                               m_weight = 1.0f;                // The weight of the event when sampled
        int16_t                             m_sourceNodeIdx = InvalidIndex; // The index of the node that this event was sampled from
        bool                                m_isFromActiveBranch : 1;
        bool                                m_isIgnored : 1;
        bool                                m_isStateEvent : 1;

        union
        {
            AnimationData                   m_animData;
            StateData                       m_stateData;
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
        int16_t                               m_endIdx = InvalidIndex;
    };

    //-------------------------------------------------------------------------
    // Sample Event Buffer
    //-------------------------------------------------------------------------

    class EE_ENGINE_API SampledEventsBuffer
    {
    public:

        // Empty the buffer
        void Clear();

        // Get the total number of sampled events (for both anim and state events )
        inline int16_t GetNumSampledEvents() const { return (int16_t) m_sampledEvents.size(); }

        // Get the total number of sampled events (for both anim and state events )
        inline TVector<SampledEvent> const& GetSampledEvents() const { return m_sampledEvents; }

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

        inline SampledEvent& EmplaceAnimationEvent( int16_t sourceNodeIdx, Event const* pEvent, Percentage percentageThrough, bool isFromActiveBranch = true )
        {
            m_numAnimEventsSampled++;
            return m_sampledEvents.emplace_back( sourceNodeIdx, 1.0f, isFromActiveBranch, pEvent, percentageThrough );
        }

        inline int16_t GetNumAnimationEventsSampled() const { return m_numAnimEventsSampled; }

        // State Events
        //-------------------------------------------------------------------------

        inline SampledEvent& EmplaceStateEvent( int16_t sourceNodeIdx, StateEventType type, StringID ID, bool isFromActiveBranch )
        {
            m_numStateEventsSampled++;
            return m_sampledEvents.emplace_back( sourceNodeIdx, 1.0f, isFromActiveBranch, type, ID );
        }

        inline int16_t GetNumStateEventsSampled() const { return m_numStateEventsSampled; }

        bool ContainsStateEvent( StringID ID, bool onlyFromActiveBranch = false ) const;
        bool ContainsStateEvent( SampledEventRange const& range, StringID ID, bool onlyFromActiveBranch = false ) const;
        bool ContainsSpecificStateEvent( StateEventType eventType, StringID ID, bool onlyFromActiveBranch = false ) const;
        bool ContainsSpecificStateEvent( SampledEventRange const& range, StateEventType eventType, StringID ID, bool onlyFromActiveBranch = false ) const;

        // Mark all state events in the range as ignored
        inline void MarkOnlyStateEventsAsIgnored( SampledEventRange range )
        {
            EE_ASSERT( IsValidRange( range ) );
            for ( int16_t i = range.m_startIdx; i < range.m_endIdx; i++ )
            {
                if ( m_sampledEvents[i].IsStateEvent() )
                {
                    m_sampledEvents[i].m_isIgnored = true;
                }
            }
        }

        // Operators
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE TVector<SampledEvent>::iterator begin() { return m_sampledEvents.begin(); }
        EE_FORCE_INLINE TVector<SampledEvent>::iterator end() { return m_sampledEvents.end(); }
        EE_FORCE_INLINE TVector<SampledEvent>::const_iterator begin() const { return m_sampledEvents.begin(); }
        EE_FORCE_INLINE TVector<SampledEvent>::const_iterator end() const{ return m_sampledEvents.end(); }
        EE_FORCE_INLINE SampledEvent& operator[]( uint32_t i ) { EE_ASSERT( i < m_sampledEvents.size() ); return m_sampledEvents[i]; }
        EE_FORCE_INLINE SampledEvent const& operator[]( uint32_t i ) const { EE_ASSERT( i < m_sampledEvents.size() ); return m_sampledEvents[i]; }

    public:

        TVector<SampledEvent>                       m_sampledEvents;
        int16_t                                     m_numAnimEventsSampled = 0;
        int16_t                                     m_numStateEventsSampled = 0;
    };
}