#include "AnimationEventEditor.h"
#include "AnimationEventData.h"
#include "Engine/Animation/Components/Component_AnimationClipPlayer.h"
#include "Engine/Animation/AnimationEvent.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EventEditor::EventEditor( TypeSystem::TypeRegistry const& typeRegistry )
        : TimelineEditor( IntRange( 0, 30 ) )
        , m_eventTypes( typeRegistry.GetAllDerivedTypes( Event::GetStaticTypeID(), false, false, true ) )
    {}

    void EventEditor::Reset()
    {
        ClearSelection();
        m_trackContainer.Reset();
    }

    void EventEditor::SetAnimationLengthAndFPS( uint32_t numFrames, float FPS )
    {
        EE_ASSERT( numFrames > 0.0f );
        EE_ASSERT( FPS > 0.0f );

        if ( m_timeRange.m_end != (int32_t) numFrames || m_FPS != FPS )
        {
            SetTimeRange( IntRange( 0, numFrames ) );

            m_FPS = FPS;
            UpdateTimelineTrackFPS();
        }
    }

    //-------------------------------------------------------------------------

    void EventEditor::UpdateAndDraw( UpdateContext const& context, AnimationClipPlayerComponent* pPreviewAnimationComponent )
    {
        EE_ASSERT( pPreviewAnimationComponent != nullptr );

        // Handle play state changes
        //-------------------------------------------------------------------------

        if ( IsPlaying() && pPreviewAnimationComponent->IsPosed() )
        {
            pPreviewAnimationComponent->SetPlayMode( IsLoopingEnabled() ? AnimationClipPlayerComponent::PlayMode::Loop : AnimationClipPlayerComponent::PlayMode::PlayOnce );
            pPreviewAnimationComponent->SetAnimTime( GetPlayheadPositionAsPercentage() );
        }
        else if ( IsPaused() && !pPreviewAnimationComponent->IsPosed() )
        {
            pPreviewAnimationComponent->SetPlayMode( AnimationClipPlayerComponent::PlayMode::Posed );
        }

        // Draw track editor and manage playhead data
        //-------------------------------------------------------------------------

        // If we are playing, then update the playhead position
        if ( IsPlaying() )
        {
            SetPlayheadPositionAsPercentage( pPreviewAnimationComponent->GetAnimTime() );
        }

        Draw();

        // Is we are paused, then update the animation component pose
        if ( IsPaused() )
        {
            pPreviewAnimationComponent->SetAnimTime( GetPlayheadPositionAsPercentage() );
        }
    }

    void EventEditor::DrawAddTracksMenu()
    {
        int32_t numAvailableTracks = 0;
        for ( auto pTypeInfo : m_eventTypes )
        {
            Event const* const pDefaultEventInstance = reinterpret_cast<Event const*>( pTypeInfo->m_pDefaultInstance );

            bool isAllowedTrackType = true;
            if ( !pDefaultEventInstance->AllowMultipleTracks() )
            {
                // Check if a track of this type already exists
                for ( auto pTrack : m_trackContainer.GetTracks() )
                {
                    auto pAnimTrack = static_cast<EventTrack*>( pTrack );
                    if ( pAnimTrack->GetEventTypeInfo() == pDefaultEventInstance->GetTypeInfo() )
                    {
                        isAllowedTrackType = false;
                        break;
                    }
                }
            }

            //-------------------------------------------------------------------------

            if ( isAllowedTrackType )
            {
                numAvailableTracks++;

                auto CreateTrackOption = [this, pDefaultEventInstance, pTypeInfo] ( Event::EventType type, char const* pNameSuffix = nullptr )
                {
                    InlineString menuItemName = pDefaultEventInstance->GetEventName();
                    if ( pNameSuffix != nullptr )
                    {
                        menuItemName += pNameSuffix;
                    }

                    if ( ImGui::MenuItem( menuItemName.c_str() ) )
                    {
                        auto pCreatedTrack = m_trackContainer.CreateTrack<EventTrack>();
                        pCreatedTrack->m_animFrameRate = m_FPS;
                        pCreatedTrack->m_eventTypeID = pTypeInfo->m_ID;
                        pCreatedTrack->m_pEventTypeInfo = pTypeInfo;
                        pCreatedTrack->m_eventType = type;
                    }
                };

                //-------------------------------------------------------------------------

                if ( pDefaultEventInstance->GetEventType() == Event::EventType::Both )
                {
                    CreateTrackOption( Event::EventType::Immediate, "(Immediate)" );
                    CreateTrackOption( Event::EventType::Duration, "(Duration)" );
                }
                else
                {
                    CreateTrackOption( pDefaultEventInstance->GetEventType() );
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( numAvailableTracks == 0 )
        {
            ImGui::Text( "No Available Tracks" );
        }
    }

    //-------------------------------------------------------------------------

    bool EventEditor::Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& objectValue )
    {
        if ( !TimelineEditor::Serialize( typeRegistry, objectValue ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        int32_t numSyncTracks = 0;
        for ( auto pTrack : m_trackContainer.m_tracks )
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

        UpdateTimelineTrackFPS();

        return true;
    }

    void EventEditor::UpdateTimelineTrackFPS()
    {
        for ( auto pTrack : m_trackContainer.m_tracks )
        {
            auto pEventTrack = reinterpret_cast<EventTrack*>( pTrack );
            pEventTrack->m_animFrameRate = m_FPS;

            for ( auto pItem : pEventTrack->m_items )
            {
                auto pEventItem = Cast<EventItem>( pItem );
                pEventItem->m_animFrameRate = m_FPS;
            }
        }
    }
}