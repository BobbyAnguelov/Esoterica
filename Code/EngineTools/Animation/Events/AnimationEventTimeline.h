#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Timeline/Timeline.h"
#include "Engine/Animation/AnimationEvent.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------
// Base class for all animation event tracks
//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EventTrackItem : public Timeline::TrackItem
    {
        EE_REFLECT_TYPE( EventTrackItem );

    public:

        using Timeline::TrackItem::TrackItem;
        EventTrackItem( FloatRange const& inRange, Event* pEvent );

        // Data
        //-------------------------------------------------------------------------

        inline Event* GetEvent() { return m_event.Get(); }
        inline Event const* GetEvent() const { return m_event.Get(); }
        TypeSystem::TypeInfo const* GetEventTypeInfo() const { return m_event.GetInstanceTypeInfo(); }

    private:

        EE_REFLECT()
        TTypeInstance<Event>           m_event;
    };

    //-------------------------------------------------------------------------

    class EventTrack : public Timeline::Track
    {
        friend class EventTimeline;
        EE_REFLECT_TYPE( EventTrack );

    protected:

        // Helper to cast the item data to the actual anim event required
        template<typename T>
        static T* GetAnimEvent( Timeline::TrackItem* pItem )
        {
            EventTrackItem* pEventItem = Cast<EventTrackItem>( pItem );
            return Cast<T>( pEventItem->GetEvent() );
        }

        template<typename T>
        static T const* GetAnimEvent( Timeline::TrackItem const* pItem )
        {
            EventTrackItem const* pEventItem = Cast<EventTrackItem>( pItem );
            return Cast<T>( pEventItem->GetEvent() );
        }

    public:

        EventTrack() = default;

        inline bool IsSyncTrack() const { return m_isSyncTrack; }

        // Track Rules
        //-------------------------------------------------------------------------

        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const = 0;

    protected:

        virtual Status GetValidationStatus( Timeline::TrackContext const& context ) const override;
        virtual bool DrawContextMenu( Timeline::TrackContext const& context, TVector<TTypeInstance<Track>>& tracks, float playheadPosition ) override;
        virtual Timeline::TrackItem* CreateItemInternal( Timeline::TrackContext const& context, float itemStartTime ) override;
        virtual void DrawExtraHeaderWidgets( ImRect const& widgetsRect ) override;

    protected:

        EE_REFLECT()
        bool                                    m_isSyncTrack = false;
    };

    //-------------------------------------------------------------------------

    class EventTimeline : public Timeline::TimelineData
    {
        EE_REFLECT_TYPE( EventTimeline );

    public:

        virtual void FillAllowedTrackTypes( TypeSystem::TypeRegistry const& typeRegistry ) override;
    };
}