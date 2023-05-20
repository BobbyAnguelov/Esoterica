#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/TimelineEditor/TimelineTrack.h"
#include "Engine/Animation/AnimationEvent.h"
#include "System/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------
// Base class for all animation event tracks
//-------------------------------------------------------------------------

namespace EE::Animation
{
    enum class EventType
    {
        EE_REFLECT_ENUM

        Immediate,
        Duration,
        Both
    };

    //-------------------------------------------------------------------------

    class EventTrack : public Timeline::Track
    {
        friend class EventEditor;
        EE_REFLECT_TYPE( EventTrack );

    protected:

        // Helper to cast the item data to the actual anim event required
        template<typename T>
        static T const* GetAnimEvent( Timeline::TrackItem const* pItem )
        {
            return Cast<T>( pItem->GetData() );
        }

    public:

        EventTrack() = default;

        inline bool IsSyncTrack() const { return m_isSyncTrack; }

        // Track Rules
        //-------------------------------------------------------------------------

        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const = 0;
        virtual bool AllowMultipleTracks() const { return false; }
        virtual EventType GetAllowedEventType() const { return EventType::Duration; }

    protected:

        virtual Status GetValidationStatus( float timelineLength ) const override;
        virtual bool HasContextMenu() const override { return true; }
        virtual void DrawContextMenu( TVector<Track*>& tracks, float playheadPosition ) override;
        virtual Timeline::TrackItem* CreateItemInternal( float itemStartTime ) override;
        virtual void DrawExtraHeaderWidgets( ImRect const& widgetsRect ) override;

    protected:

        EE_REFLECT() EventType                             m_eventType = EventType::Duration;
        EE_REFLECT() bool                                  m_isSyncTrack = false;
    };
}