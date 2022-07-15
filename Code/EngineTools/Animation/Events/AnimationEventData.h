#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/TimelineEditor/TimelineData.h"
#include "Engine/Animation/AnimationEvent.h"
#include "System/TypeSystem/RegisteredType.h"

//-------------------------------------------------------------------------
// Stores tools only information for event track data
//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EventItem final : public Timeline::TrackItem
    {
        friend class EventEditor;
        EE_REGISTER_TYPE( EventItem );

        constexpr static char const* s_eventDataKey = "EventData";

    public:

        EventItem() = default;
        EventItem( Event* pEvent, float frameRate );
        ~EventItem();

        virtual InlineString GetLabel() const override;
        virtual FloatRange GetTimeRange() const override;

        Event* GetEvent() const { return m_pEvent; }
        TypeSystem::TypeInfo const* GetEventTypeInfo() const { return m_pEvent->GetTypeInfo(); }

    private:

        virtual void SetTimeRangeInternal( FloatRange const& inRange ) override;
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue ) override;
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const override;

    private:

        Event*                                          m_pEvent = nullptr;
        float                                           m_animFrameRate = 0.0f;
    };

    //-------------------------------------------------------------------------

    class EventTrack final : public Timeline::Track
    {
        friend class EventEditor;
        EE_REGISTER_TYPE( EventTrack );

    public:

        EventTrack() = default;

        inline TypeSystem::TypeInfo const* GetEventTypeInfo() const { return m_pEventTypeInfo; }
        inline bool IsSyncTrack() const { return m_isSyncTrack; }

        virtual const char* GetLabel() const override;
        virtual void DrawHeader( ImRect const& headerRect ) override;
        virtual bool HasContextMenu() const override { return true; }
        virtual void DrawContextMenu( TVector<Track*>& tracks, float playheadPosition ) override;

    private:

        virtual void CreateItemInternal( float itemStartTime ) override;
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue ) override;

    private:

        EE_EXPOSE Event::EventType                     m_eventType = Event::EventType::Duration;
        EE_EXPOSE bool                                 m_isSyncTrack = false;
        EE_EXPOSE TypeSystem::TypeID                   m_eventTypeID;
        TypeSystem::TypeInfo const*                     m_pEventTypeInfo = nullptr;
        float                                           m_animFrameRate = 0.0f;
    };
}