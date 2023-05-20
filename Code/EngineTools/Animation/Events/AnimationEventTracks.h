#pragma once

#include "AnimationEventTrack.h"

#include "Engine/Animation/Events/AnimationEvent_ID.h"
#include "Engine/Animation/Events/AnimationEvent_Foot.h"
#include "Engine/Animation/Events/AnimationEvent_Warp.h"
#include "Engine/Animation/Events/AnimationEvent_Ragdoll.h"
#include "Engine/Animation/Events/AnimationEvent_Transition.h"
#include "Engine/Animation/Events/AnimationEvent_RootMotion.h"
#include "EngineTools/Core/Widgets/CurveEditor.h"

//-------------------------------------------------------------------------
// Animation Event Tracks
//-------------------------------------------------------------------------

namespace EE::Animation
{
    class IDEventTrack final : public EventTrack
    {
        EE_REFLECT_TYPE( IDEventTrack );

        virtual const char* GetTypeName() const override { return "ID"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual EventType GetAllowedEventType() const override { return EventType::Both; }
        virtual bool AllowMultipleTracks() const override { return true; }
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;
    };

    //-------------------------------------------------------------------------

    class FootEventTrack final : public EventTrack
    {
        EE_REFLECT_TYPE( FootEventTrack );

        virtual const char* GetTypeName() const override { return "Foot"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;
        virtual Color GetItemColor( Timeline::TrackItem const* pItem ) const override;
    };

    //-------------------------------------------------------------------------

    class TransitionEventTrack final : public EventTrack
    {
        EE_REFLECT_TYPE( TransitionEventTrack );

        virtual const char* GetTypeName() const override { return "Transition"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;
        virtual Color GetItemColor( Timeline::TrackItem const* pItem ) const override;
    };

    //-------------------------------------------------------------------------

    class OrientationWarpEventTrack final : public EventTrack
    {
        EE_REFLECT_TYPE( OrientationWarpEventTrack );

        virtual const char* GetTypeName() const override { return "Orientation Warp"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual bool CanCreateNewItems() const override;

    private:

        virtual Status GetValidationStatus( float timelineLength ) const override;
    };

    //-------------------------------------------------------------------------

    class RootMotionEventTrack final : public EventTrack
    {
        EE_REFLECT_TYPE( RootMotionEventTrack );

        virtual const char* GetTypeName() const override { return "Root Motion"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;

    private:

        virtual Status GetValidationStatus( float timelineLength ) const override;
    };

    //-------------------------------------------------------------------------

    class TargetWarpEventTrack final : public EventTrack
    {
        EE_REFLECT_TYPE( TargetWarpEventTrack );

        virtual const char* GetTypeName() const override { return "Target Warp"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;
        virtual Color GetItemColor( Timeline::TrackItem const* pItem ) const override;

    private:

        virtual Status GetValidationStatus( float timelineLength ) const override;
    };

    //-------------------------------------------------------------------------

    class RagdollEventTrack final : public EventTrack
    {
        EE_REFLECT_TYPE( RagdollEventTrack );

        virtual const char* GetTypeName() const override { return "Ragdoll"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual EventType GetAllowedEventType() const override { return EventType::Duration; }
        virtual bool AllowMultipleTracks() const override { return false; }
        virtual bool CanCreateNewItems() const override;
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;

    private:

        virtual Status GetValidationStatus( float timelineLength ) const override;
        virtual ImRect DrawDurationItem( ImDrawList* pDrawList, Timeline::TrackItem* pItem, Float2 const& itemStartPos, Float2 const& itemEndPos, ItemState itemState ) override;
        virtual float GetTrackHeight() const override { return 70; }
    };
}