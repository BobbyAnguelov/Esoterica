#include "AnimationEventTrack.h"
#include "System/Imgui/ImguiX.h"

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

    void EventTrack::DrawContextMenu( TVector<Track*>& tracks, float playheadPosition )
    {
        if ( m_isSyncTrack )
        {
            if ( ImGui::MenuItem( EE_ICON_SYNC" Clear Sync Track" ) )
            {
                m_isSyncTrack = false;
                MarkDirty();
            }
        }
        else // Allow setting of sync track
        {
            if ( ImGui::MenuItem( EE_ICON_SYNC" Set As Sync Track" ) )
            {
                // Clear sync track from any other track
                for ( auto pTrack : tracks )
                {
                    reinterpret_cast<EventTrack*>( pTrack )->m_isSyncTrack = false;
                }

                m_isSyncTrack = true;
                MarkDirty();
            }
        }
    }

    Timeline::TrackItem* EventTrack::CreateItemInternal( float itemStartTime )
    {
        auto pAnimEvent = Cast<Event>( GetEventTypeInfo()->CreateType() );
        float const duration = ( m_eventType == EventType::Duration ) ? 1.0f : 0.0f;
        FloatRange const itemRange( itemStartTime, itemStartTime + duration );
        auto pCreatedItem = m_items.emplace_back( EE::New<Timeline::TrackItem>( itemRange, pAnimEvent ) );
        return pCreatedItem;
    };

    Timeline::Track::Status EventTrack::GetValidationStatus( float timelineLength ) const
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
            m_statusMessage = "Track contains invalid events, these events will be ignored";
            return Status::HasWarnings;
        }
        // Error
        else if ( hasInvalidEvents && !hasValidEvents )
        {
            m_statusMessage = "Track doesnt contain any valid events!";
            return Status::HasErrors;
        }

        return Status::Valid;
    }
}