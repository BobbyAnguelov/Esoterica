#include "AnimationEventEditor.h"
#include "AnimationEventTrack.h"
#include "Engine/Animation/AnimationEvent.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EventEditor::EventEditor( TypeSystem::TypeRegistry const& typeRegistry )
        : TimelineEditor( FloatRange( 0, 30 ) )
        , m_eventTrackTypes( typeRegistry.GetAllDerivedTypes( EventTrack::GetStaticTypeID(), false, false, true ) )
    {}

    void EventEditor::Reset()
    {
        ClearSelection();
        m_trackContainer.Reset();
    }

    void EventEditor::SetAnimationInfo( uint32_t numFrames, float FPS )
    {
        EE_ASSERT( numFrames > 0.0f );
        if ( m_timeRange.m_end != (int32_t) numFrames - 1 )
        {
            SetTimeRange( FloatRange( 0, float( numFrames - 1 ) ) );
        }

        EE_ASSERT( FPS >= 0.0f );
        if ( m_FPS != FPS )
        {
            m_FPS = FPS;
        }
    }

    //-------------------------------------------------------------------------

    void EventEditor::DrawAddTracksMenu()
    {
        int32_t numAvailableTracks = 0;
        for ( auto pTypeInfo : m_eventTrackTypes )
        {
            auto const pDefaultTrackInstance = Cast<EventTrack>( pTypeInfo->m_pDefaultInstance );

            bool isAllowedTrackType = true;
            if ( !pDefaultTrackInstance->AllowMultipleTracks() )
            {
                // Check if a track of this type already exists
                for ( auto pTrack : m_trackContainer.GetTracks() )
                {
                    auto pExistingEventTrack = reinterpret_cast<EventTrack*>( pTrack );
                    if ( pExistingEventTrack->GetTypeInfo() == pDefaultTrackInstance->GetTypeInfo() )
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

                auto CreateTrackOption = [this, pDefaultTrackInstance, pTypeInfo] ( EventType type, char const* pNameSuffix = nullptr )
                {
                    InlineString menuItemName = pDefaultTrackInstance->GetTypeName();
                    if ( pNameSuffix != nullptr )
                    {
                        menuItemName += pNameSuffix;
                    }

                    if ( ImGui::MenuItem( menuItemName.c_str() ) )
                    {
                        auto pCreatedTrack = Cast<EventTrack>( m_trackContainer.CreateTrack( pTypeInfo ) );
                        pCreatedTrack->m_eventType = type;
                    }
                };

                //-------------------------------------------------------------------------

                if ( pDefaultTrackInstance->GetAllowedEventType() == EventType::Both )
                {
                    CreateTrackOption( EventType::Immediate, "(Immediate)" );
                    CreateTrackOption( EventType::Duration, "(Duration)" );
                }
                else
                {
                    CreateTrackOption( pDefaultTrackInstance->GetAllowedEventType() );
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

        return true;
    }
}