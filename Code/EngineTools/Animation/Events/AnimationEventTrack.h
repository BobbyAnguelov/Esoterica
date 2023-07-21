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
        virtual bool HasContextMenu() const override { return true; }
        virtual void DrawContextMenu( Timeline::TrackContext const& context, TVector<Track*>& tracks, float playheadPosition ) override;
        virtual Timeline::TrackItem* CreateItemInternal( Timeline::TrackContext const& context, float itemStartTime ) override;
        virtual void DrawExtraHeaderWidgets( ImRect const& widgetsRect ) override;

    protected:

        EE_REFLECT() bool                                  m_isSyncTrack = false;
    };

    //-------------------------------------------------------------------------

    class EventTimeline : public Timeline::TimelineData
    {
    public:

        EventTimeline( TFunction<void()>&& onBeginModification, TFunction<void()>&& onEndModification, TypeSystem::TypeRegistry const& typeRegistry );

        // Set the time info for the animation this event data is attached to, has to be done after the fact since we need to load the animation first
        void SetAnimationInfo( uint32_t numFrames, float FPS );

        // Custom serialization
        virtual bool Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue ) override;

    private:

        virtual float ConvertSecondsToTimelineUnit( Seconds const inTime ) const override { return inTime.ToFloat() * m_FPS; }

    private:

        float                                       m_FPS = 0.0f;
    };
}