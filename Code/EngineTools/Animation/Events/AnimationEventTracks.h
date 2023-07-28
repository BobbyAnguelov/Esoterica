#pragma once

#include "AnimationEventTrack.h"

#include "Engine/Animation/Events/AnimationEvent_ID.h"
#include "Engine/Animation/Events/AnimationEvent_SnapToFrame.h"
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
        virtual Timeline::ItemType GetAllowedItemType() const override { return Timeline::ItemType::Both; }
        virtual bool AllowMultipleTracks() const override { return true; }
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;
    };

    //-------------------------------------------------------------------------

    class SnapToFrameEventTrack final : public EventTrack
    {
        EE_REFLECT_TYPE( SnapToFrameEventTrack );

        virtual const char* GetTypeName() const override { return "Snap To Frame"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual Timeline::ItemType GetAllowedItemType() const override { return Timeline::ItemType::Duration; }
        virtual bool AllowMultipleTracks() const override { return false; }
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
        virtual bool DrawContextMenu( Timeline::TrackContext const& context, TVector<Track*>& tracks, float playheadPosition ) override;

        void AutoSetPhases( Timeline::TrackContext const& context, FootEvent::Phase startingPhase );
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

        virtual Status GetValidationStatus( Timeline::TrackContext const& context ) const override;
    };

    //-------------------------------------------------------------------------

    class RootMotionEventTrack final : public EventTrack
    {
        EE_REFLECT_TYPE( RootMotionEventTrack );

        virtual const char* GetTypeName() const override { return "Root Motion"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;

    private:

        virtual Status GetValidationStatus( Timeline::TrackContext const& context ) const override;
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

        virtual Status GetValidationStatus( Timeline::TrackContext const& context ) const override;
    };

    //-------------------------------------------------------------------------

    class RagdollEventTrack final : public EventTrack
    {
        EE_REFLECT_TYPE( RagdollEventTrack );

        virtual const char* GetTypeName() const override { return "Ragdoll"; }
        virtual TypeSystem::TypeInfo const* GetEventTypeInfo() const override;
        virtual bool AllowMultipleTracks() const override { return false; }
        virtual bool CanCreateNewItems() const override;
        virtual InlineString GetItemLabel( Timeline::TrackItem const* pItem ) const override;

    private:

        virtual Status GetValidationStatus( Timeline::TrackContext const& context ) const override;
        virtual ImRect DrawDurationItem( Timeline::TrackContext const& context, ImDrawList* pDrawList, Timeline::TrackItem* pItem, Float2 const& itemStartPos, Float2 const& itemEndPos, ItemState itemState ) override;
        virtual float GetTrackHeight() const override { return 70; }
    };
}