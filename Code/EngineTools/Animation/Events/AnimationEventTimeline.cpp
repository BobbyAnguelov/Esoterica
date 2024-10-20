#include "AnimationEventTimeline.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void EventTrack::DrawExtraHeaderWidgets( ImRect const& widgetsRect )
    {
        if ( m_isSyncTrack )
        {
            bool shouldShowWarning = false;
            InlineString warningMessage;
            int32_t const numItems = GetNumItems();
            if ( numItems == 0 )
            {
                shouldShowWarning = true;
                warningMessage = "No sync events!";
            }
            else
            {
                if ( m_items[0]->GetStartTime() > 0.0f )
                {
                    shouldShowWarning = true;
                    warningMessage = "Sync Track - Last event will wrap around!";
                }
            }

            //-------------------------------------------------------------------------

            if ( shouldShowWarning )
            {
                ImVec2 const textSize = ImGui::CalcTextSize( EE_ICON_SYNC_ALERT );
                ImGui::SameLine( ImGui::GetCursorPosX() + widgetsRect.GetSize().x - textSize.x );
                ImGui::TextColored( Colors::Yellow.ToFloat4(), EE_ICON_SYNC_ALERT );
                ImGuiX::TextTooltip( warningMessage.c_str() );
            }
            else
            {
                ImVec2 const textSize = ImGui::CalcTextSize( EE_ICON_SYNC );
                ImGui::SameLine( ImGui::GetCursorPosX() + widgetsRect.GetSize().x - textSize.x );
                ImGui::Text( EE_ICON_SYNC );
                ImGuiX::TextTooltip( "Sync Track" );
            }
        }
    }

    bool EventTrack::DrawContextMenu( Timeline::TrackContext const& context, TVector<TTypeInstance<Track>>& tracks, float playheadPosition )
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
                for ( TTypeInstance<Track>& track : tracks )
                {
                    track.GetAs<EventTrack>()->m_isSyncTrack = false;
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

        TTypeInstance<Timeline::TrackItem>& createdItem = m_items.emplace_back();
        createdItem.SetInstance( EE::New<EventTrackItem>( itemRange, pAnimEvent ) );
        return createdItem.Get();
    };

    Timeline::Track::Status EventTrack::GetValidationStatus( Timeline::TrackContext const& context ) const
    {
        ResetStatusMessage();

        bool hasValidEvents = false;
        bool hasInvalidEvents = false;

        for ( TTypeInstance<Timeline::TrackItem> const& item : m_items )
        {
            auto pEventItem = Cast<EventTrackItem>( item.Get() );
            auto pAnimEvent = pEventItem->GetEvent();
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
            m_validationStatusMessage = "Track contains invalid events, these events will be ignored";
            return Status::HasWarnings;
        }
        // Error
        else if ( hasInvalidEvents && !hasValidEvents )
        {
            m_validationStatusMessage = "Track doesnt contain any valid events!";
            return Status::HasErrors;
        }

        return Status::Valid;
    }

    //-------------------------------------------------------------------------

    EventTrackItem::EventTrackItem( FloatRange const& inRange, Event* pEvent )
        : Timeline::TrackItem( inRange )
        , m_event( pEvent )
    {
        EE_ASSERT( pEvent != nullptr );
    }

    //-------------------------------------------------------------------------

    void EventTimeline::FillAllowedTrackTypes( TypeSystem::TypeRegistry const& typeRegistry )
    {
        m_allowedTrackTypes = typeRegistry.GetAllDerivedTypes( EventTrack::GetStaticTypeID(), false, false, true );
    }
}