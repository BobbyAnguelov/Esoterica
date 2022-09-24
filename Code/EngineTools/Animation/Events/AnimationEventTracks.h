#pragma once

#include "AnimationEventTrack.h"

#include "Engine/Animation/Events/AnimationEvent_ID.h"
#include "Engine/Animation/Events/AnimationEvent_Foot.h"
#include "Engine/Animation/Events/AnimationEvent_Warp.h"
#include "Engine/Animation/Events/AnimationEvent_Ragdoll.h"
#include "EngineTools/Core/Widgets/CurveEditor.h"

//-------------------------------------------------------------------------
// Animation Event Tracks
//-------------------------------------------------------------------------

namespace EE::Animation
{
    class IDEventTrack final : public EventTrack
    {
        EE_REGISTER_TYPE( IDEventTrack );

        virtual const char* GetTypeName() const override { return "ID"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual EventType GetAllowedEventType() const override { return EventType::Both; }
        virtual bool AllowMultipleTracks() const override { return true; }
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;
    };

    //-------------------------------------------------------------------------

    class FootEventTrack final : public EventTrack
    {
        EE_REGISTER_TYPE( FootEventTrack );

        virtual const char* GetTypeName() const override { return "Foot"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;
        virtual Color GetItemColor( Timeline::TrackItem const* pItem ) const override;
    };

    //-------------------------------------------------------------------------

    class WarpEventTrack final : public EventTrack
    {
        EE_REGISTER_TYPE( WarpEventTrack );

        virtual const char* GetTypeName() const override { return "Warp"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;
    };

    //-------------------------------------------------------------------------

    class RagdollEventTrack final : public EventTrack
    {
        EE_REGISTER_TYPE( RagdollEventTrack );

        virtual const char* GetTypeName() const override { return "Ragdoll"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual EventType GetAllowedEventType() const override { return EventType::Duration; }
        virtual bool AllowMultipleTracks() const override { return false; }
        virtual bool CanCreateNewItems() const override;
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;

    private:

        virtual Status GetValidationStatus() const override;
        virtual ImRect DrawDurationItem( ImDrawList* pDrawList, Timeline::TrackItem* pItem, Float2 const& itemStartPos, Float2 const& itemEndPos, ItemState itemState );
        virtual float GetTrackHeight() const override { return 70; }
    };
}