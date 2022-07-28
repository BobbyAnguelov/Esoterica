#include "AnimationEventTracks.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    TypeSystem::TypeInfo const* IDEventTrack::GetEventTypeInfo() const
    { 
        return IDEvent::s_pTypeInfo;
    }

    InlineString IDEventTrack::GetItemLabel( Timeline::TrackItem const* pItem ) const
    {
        auto pAnimEvent = GetAnimEvent<IDEvent>( pItem );
        return pAnimEvent->GetID().IsValid() ? pAnimEvent->GetID().c_str() : "Invalid ID";
    }

    //-------------------------------------------------------------------------

    TypeSystem::TypeInfo const* FootEventTrack::GetEventTypeInfo() const
    {
        return FootEvent::s_pTypeInfo;
    }

    InlineString FootEventTrack::GetItemLabel( Timeline::TrackItem const* pItem ) const
    {
        auto pAnimEvent = GetAnimEvent<FootEvent>( pItem );
        return FootEvent::GetPhaseName( pAnimEvent->GetFootPhase() );
    }

    Color FootEventTrack::GetItemColor( Timeline::TrackItem const * pItem ) const
    {
        auto pAnimEvent = GetAnimEvent<FootEvent>( pItem );
        return FootEvent::GetPhaseColor( pAnimEvent->GetFootPhase() );
    }

    //-------------------------------------------------------------------------

    TypeSystem::TypeInfo const* WarpEventTrack::GetEventTypeInfo() const
    {
        return WarpEvent::s_pTypeInfo;
    }

    InlineString WarpEventTrack::GetItemLabel( Timeline::TrackItem const * pItem ) const
    {
        auto pAnimEvent = GetAnimEvent<WarpEvent>( pItem );
        return pAnimEvent->GetDebugText();
    }
}