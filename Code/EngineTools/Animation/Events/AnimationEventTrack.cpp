#include "AnimationEventTrack.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void EventTrack::DrawExtraHeaderWidgets( ImRect const& widgetsRect )
    {
        if ( m_isSyncTrack )
        {
            auto const textSize = ImGui::CalcTextSize( EE_ICON_SYNC );
            ImGui::SameLine( ImGui::GetCursorPosX() + widgetsRect.GetSize().x - textSize.x );
            ImGui::Text( EE_ICON_SYNC );
            ImGuiX::TextTooltip( "Sync Track" );
        }
    }

    bool EventTrack::DrawContextMenu( Timeline::TrackContext const& context, TVector<Track*>& tracks, float playheadPosition )
    {
        if ( m_isSyncTrack )
        {
            if ( ImGui::MenuItem( EE_ICON_SYNC" Clear Sync Track" ) )
            {
                Timeline::ScopedModification const stm( context );
                m_isSyncTrack = false;
            }
        }
        else // Allow setting of sync track
        {
            if ( ImGui::MenuItem( EE_ICON_SYNC" Set As Sync Track" ) )
            {
                Timeline::ScopedModification const stm( context );

                // Clear sync track from any other track
                for ( auto pTrack : tracks )
                {
                    reinterpret_cast<EventTrack*>( pTrack )->m_isSyncTrack = false;
                }

                m_isSyncTrack = true;
            }
        }

        return false;
    }

    Timeline::TrackItem* EventTrack::CreateItemInternal( Timeline::TrackContext const& context, float itemStartTime )
    {
        auto pAnimEvent = Cast<Event>( GetEventTypeInfo()->CreateType() );
        float const duration = ( m_itemType == Timeline::ItemType::Duration ) ? 1.0f : 0.0f;
        FloatRange const itemRange( itemStartTime, itemStartTime + duration );
        auto pCreatedItem = m_items.emplace_back( EE::New<Timeline::TrackItem>( itemRange, pAnimEvent ) );
        return pCreatedItem;
    };

    Timeline::Track::Status EventTrack::GetValidationStatus( Timeline::TrackContext const& context ) const
    {
        bool hasValidEvents = false;
        bool hasInvalidEvents = false;

        for ( auto pItem : m_items )
        {
            auto pAnimEvent = GetAnimEvent<Event>( pItem );
            if ( pAnimEvent->IsValid() )
            {
                hasValidEvents = true;
            }
            else
            {
                hasInvalidEvents = true;
            }
        }

        //-------------------------------------------------------------------------

        // Warning
        if ( hasInvalidEvents && hasValidEvents )
        {
            m_validationStatueMessage = "Track contains invalid events, these events will be ignored";
            return Status::HasWarnings;
        }
        // Error
        else if ( hasInvalidEvents && !hasValidEvents )
        {
            m_validationStatueMessage = "Track doesnt contain any valid events!";
            return Status::HasErrors;
        }

        return Status::Valid;
    }

    //-------------------------------------------------------------------------

    EventTimeline::EventTimeline( TFunction<void()>&& onBeginModification, TFunction<void()>&& onEndModification, TypeSystem::TypeRegistry const& typeRegistry )
        : TimelineData( onBeginModification, onEndModification )
    {
        m_allowedTrackTypes = typeRegistry.GetAllDerivedTypes( EventTrack::GetStaticTypeID(), false, false, true );
    }

    void EventTimeline::SetAnimationInfo( uint32_t numFrames, float FPS )
    {
        EE_ASSERT( numFrames > 0.0f );
        if ( m_length != (int32_t) numFrames - 1 )
        {
            SetLength( float( numFrames - 1 ) );
        }

        EE_ASSERT( FPS >= 0.0f );
        if ( m_FPS != FPS )
        {
            m_FPS = FPS;
        }
    }

    bool EventTimeline::Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue )
    {
        if ( !Timeline::TimelineData::Serialize( typeRegistry, typeObjectValue ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        int32_t numSyncTracks = 0;
        for ( auto pTrack : m_tracks )
        {
            auto pEventTrack = reinterpret_cast<EventTrack*>( pTrack );
            if ( pEventTrack->m_isSyncTrack )
            {
                numSyncTracks++;

                // If we have more than one sync track, clear the rest
                if ( numSyncTracks > 1 )
                {
                    pEventTrack->m_isSyncTrack = false;
                }
            }
        }

        return true;
    }

    
}