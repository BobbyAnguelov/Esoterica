#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/TimelineEditor/TimelineTrack.h"
#include "Engine/Animation/AnimationEvent.h"
#include "System/TypeSystem/RegisteredType.h"

//-------------------------------------------------------------------------
// Base class for all animation event tracks
//-------------------------------------------------------------------------

namespace EE::Animation
{
    enum class EventType
    {
        EE_REGISTER_ENUM

        Immediate,
        Duration,
        Both
    };

    //-------------------------------------------------------------------------

    class EventTrack : public Timeline::Track
    {
        friend class EventEditor;
        EE_REGISTER_TYPE( EventTrack );

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
        virtual int32_t GetMaxEventsAllowedPerTrack() const { return -1; }
        virtual EventType GetAllowedEventType() const { return EventType::Duration; }

    protected:

        virtual bool HasContextMenu() const override { return true; }
        virtual void DrawContextMenu( TVector<Track*>& tracks, float playheadPosition ) override;
        virtual void CreateItemInternal( float itemStartTime ) override;
        virtual void DrawExtraHeaderWidgets( ImRect const& widgetsRect ) override;

    protected:

        EE_EXPOSE EventType                             m_eventType = EventType::Duration;
        EE_EXPOSE bool                                  m_isSyncTrack = false;
    };
}