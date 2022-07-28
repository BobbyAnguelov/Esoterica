#pragma once

#include "AnimationEventTrack.h"

#include "Engine/Animation/Events/AnimationEvent_ID.h"
#include "Engine/Animation/Events/AnimationEvent_Foot.h"
#include "Engine/Animation/Events/AnimationEvent_Warp.h"

//-------------------------------------------------------------------------
// Animation Event Tracks
//-------------------------------------------------------------------------

namespace EE::Animation
{
    class IDEventTrack : public EventTrack
    {
        EE_REGISTER_TYPE( IDEventTrack );

        virtual const char* GetTypeName() const override { return "ID"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual EventType GetAllowedEventType() const override { return EventType::Both; }
        virtual bool AllowMultipleTracks() const override { return true; }
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;
    };

    //-------------------------------------------------------------------------

    class FootEventTrack : public EventTrack
    {
        EE_REGISTER_TYPE( FootEventTrack );

        virtual const char* GetTypeName() const override { return "Foot"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;
        virtual Color GetItemColor( Timeline::TrackItem const* pItem ) const override;
    };

    //-------------------------------------------------------------------------

    class WarpEventTrack : public EventTrack
    {
        EE_REGISTER_TYPE( WarpEventTrack );

        virtual const char* GetTypeName() const override { return "Warp"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;
    };

    //-------------------------------------------------------------------------
}