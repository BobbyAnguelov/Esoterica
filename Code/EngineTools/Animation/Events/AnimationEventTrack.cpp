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
            ImGui::SameLine( widgetsRect.Max.x - textSize.x );
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
                    static_cast<EventTrack*>( pTrack )->m_isSyncTrack = false;
                }

                m_isSyncTrack = true;
                MarkDirty();
            }
        }
    }

    void EventTrack::CreateItemInternal( float itemStartTime )
    {
        auto pAnimEvent = Cast<Event>( GetEventTypeInfo()->CreateType() );
        float const duration = ( m_eventType == EventType::Duration ) ? 1.0f : 0.0f;
        FloatRange const itemRange( itemStartTime, itemStartTime + duration );
        m_items.emplace_back( EE::New<Timeline::TrackItem>( itemRange, pAnimEvent ) );
    };
}