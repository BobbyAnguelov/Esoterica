#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/Timeline/Timeline.h"
#include "Engine/Animation/AnimationEvent.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------
// Base class for all animation event tracks
//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EventTrack : public Timeline::Track
    {
        friend class EventTimeline;
        EE_REFLECT_TYPE( EventTrack );

    protected:

        // Helper to cast the item data to the actual anim event required
        template<typename T>
        static T* GetAnimEvent( Timeline::TrackItem* pItem )
        {
            return Cast<T>( pItem->GetData() );
        }

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

    protected:

        virtual Status GetValidationStatus( Timeline::TrackContext const& context ) const override;
        virtual bool DrawContextMenu( Timeline::TrackContext const& context, TVector<Track*>& tracks, float playheadPosition ) override;
        virtual Timeline::TrackItem* CreateItemInternal( Timeline::TrackContext const& context, float itemStartTime ) override;
        virtual void DrawExtraHeaderWidgets( ImRect const& widgetsRect ) override;

    protected:

        EE_REFLECT()
        bool                                    m_isSyncTrack = false;
    };

    //-------------------------------------------------------------------------

    class EventTimeline : public Timeline::TimelineData
    {
    public:

        EventTimeline( TypeSystem::TypeRegistry const& typeRegistry );

        virtual bool Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue ) override;
    };
}