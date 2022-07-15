#include "AnimationEventData.h"
#include "System/Serialization/TypeSerialization.h"
#include "System/Imgui/ImguiX.h"
#include "System/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct EventManipulator
    {
        static void SetEventTime( Event* pEvent, Seconds startTime, Seconds duration )
        {
            EE_ASSERT( pEvent != nullptr && duration >= 0 );
            pEvent->m_startTime = startTime;
            pEvent->m_duration = duration;
        }
    };
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EventItem::EventItem( Event* pEvent, float frameRate )
        : m_pEvent( pEvent )
        , m_animFrameRate( frameRate )
    {
        EE_ASSERT( m_pEvent != nullptr && m_animFrameRate > 0 );
    }

    EventItem::~EventItem()
    {
        EE::Delete( m_pEvent );
    }

    InlineString EventItem::GetLabel() const
    {
        return m_pEvent->GetDisplayText();
    }

    FloatRange EventItem::GetTimeRange() const
    {
        EE_ASSERT( m_animFrameRate != 0 && m_pEvent != nullptr );

        // Convert to frame-time
        float const startTime = m_pEvent->GetStartTime().ToFloat() * m_animFrameRate;
        float const duration = m_pEvent->GetDuration().ToFloat() * m_animFrameRate;
        float const endTime = startTime + duration;

        return FloatRange( startTime, endTime );
    }

    void EventItem::SetTimeRangeInternal( FloatRange const& inRange )
    {
        EE_ASSERT( m_animFrameRate != 0 && m_pEvent != nullptr );

        // Convert to seconds
        float const startTime = inRange.m_begin / m_animFrameRate;
        float const duration = inRange.GetLength() / m_animFrameRate;
        EventManipulator::SetEventTime( m_pEvent, startTime, duration );
    }

    void EventItem::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue )
    {
        m_pEvent = Serialization::CreateAndReadNativeType<Event>( typeRegistry, typeObjectValue[s_eventDataKey] );
    }

    void EventItem::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const
    {
        writer.Key( s_eventDataKey );
        Serialization::WriteNativeType( typeRegistry, m_pEvent, writer );
    }

    //-------------------------------------------------------------------------

    const char* EventTrack::GetLabel() const
    {
        return reinterpret_cast<Event const*>( m_pEventTypeInfo->m_pDefaultInstance )->GetEventName();
    }

    void EventTrack::CreateItemInternal( float itemStartTime )
    {
        EE_ASSERT( m_animFrameRate > 0 );

        Seconds const startTime = itemStartTime / m_animFrameRate;

        //-------------------------------------------------------------------------

        auto pNewEvent = (Event*) m_pEventTypeInfo->CreateType();

        if ( m_eventType == Event::EventType::Immediate )
        {
            EventManipulator::SetEventTime( pNewEvent, startTime, 0.0f );
        }
        else
        {
            EventManipulator::SetEventTime( pNewEvent, startTime, 1.0f / m_animFrameRate );
        }

        m_items.emplace_back( EE::New<EventItem>( pNewEvent, m_animFrameRate ) );
    };

    void EventTrack::DrawHeader( ImRect const& headerRect )
    {
        ImGui::SameLine( 10 );
        ImGui::AlignTextToFramePadding();
        if ( m_isSyncTrack )
        {
            ImGui::Text( EE_ICON_CLOCK " %s", GetLabel() );
        }
        else
        {
            ImGui::Text( GetLabel() );
        }
    }

    void EventTrack::DrawContextMenu( TVector<Track*>& tracks, float playheadPosition )
    {
        if ( m_isSyncTrack )
        {
            if ( ImGui::MenuItem( "Clear Sync Track" ) )
            {
                m_isSyncTrack = false;
                MarkDirty();
            }
        }
        else // Allow setting of sync track
        {
            if ( ImGui::MenuItem( "Set As Sync Track" ) )
            {
                // Clear sync track from any other track
                for ( auto pTrack : tracks )
                {
                    static_cast<EventTrack*>( pTrack )->m_isSyncTrack = false;
                }

                m_isSyncTrack = true;
                MarkDirty();
            }
        }
    }

    void EventTrack::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue )
    {
        m_pEventTypeInfo = typeRegistry.GetTypeInfo( m_eventTypeID );
        EE_ASSERT( m_pEventTypeInfo != nullptr );
    }
}