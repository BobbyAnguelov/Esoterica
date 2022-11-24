#pragma once

#include "TimelineTrack.h"
#include "System/Types/Event.h"

//-------------------------------------------------------------------------

struct ImRect;

//-------------------------------------------------------------------------

namespace EE::Timeline
{
    class EE_ENGINETOOLS_API TrackContainer
    {

    public:

        static TEvent<TrackContainer*>      s_onBeginModification;
        static TEvent<TrackContainer*>      s_onEndModification;

        constexpr static char const*        s_trackContainerKey = "TimelineTracks";

    public:

        virtual ~TrackContainer();

        // Frees all tracks
        void Reset();

        // Info
        //-------------------------------------------------------------------------

        inline int32_t GetNumTracks() const { return (int32_t) m_tracks.size(); }

        TVector<Track*>& GetTracks() { return m_tracks; }
        TVector<Track*> const& GetTracks() const { return m_tracks; }
        inline Track* GetTrack( size_t i ) { return m_tracks[i]; }
        inline Track const* GetTrack( size_t i ) const { return m_tracks[i]; }

        // Get the track that this item belongs to, if it exists
        Track* GetTrackForItem( TrackItem const* pItem );

        // Get the track that this item belongs to, if it exists
        Track const* GetTrackForItem( TrackItem const* pItem ) const;

        // Does this container own this track?
        bool Contains( Track const* pTrack ) const;

        // Does this container own this item?
        bool Contains( TrackItem const* pItem ) const;

        // Get the overall status for all tracks
        Track::Status GetValidationStatus( float timelineLength ) const;

        // Track Manipulation
        //-------------------------------------------------------------------------

        Track* CreateTrack( TypeSystem::TypeInfo const* pTrackTypeInfo );

        void DeleteTrack( Track* pTrack );

        void CreateItem( Track* pTrack, float itemStartTime );

        void UpdateItemTimeRange( TrackItem* pItem, FloatRange const& newTimeRange );

        void DeleteItem( TrackItem* pItem );

        // Undo/Redo support
        //-------------------------------------------------------------------------

        void BeginModification();
        void EndModification();

        // Dirty state
        //-------------------------------------------------------------------------

        bool IsDirty() const;
        void ClearDirtyFlags();
        inline void MarkDirty() { m_isDirty = true; }

        // Serialization
        //-------------------------------------------------------------------------

        virtual bool Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue );
        virtual void Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer );

    public:

        TVector<Track*>     m_tracks;
        int32_t             m_beginModificationCallCount = 0;
        bool                m_isDirty = false;
    };
}