#pragma once

#include "EngineTools/_Module/API.h"
#include "System/Serialization/JsonSerialization.h"
#include "System/TypeSystem/RegisteredType.h"
#include "System/Types/Arrays.h"
#include "System/Math/NumericRange.h"
#include "System/Types/Color.h"
#include "System/Types/String.h"
#include "System/Types/Event.h"

//-------------------------------------------------------------------------

struct ImRect;

//-------------------------------------------------------------------------

namespace EE::Timeline
{
    class EE_ENGINETOOLS_API TrackItem : public IRegisteredType
    {
        EE_REGISTER_TYPE( TrackItem );

        friend class TrackContainer;

    public:

        TrackItem() = default;
        virtual ~TrackItem() = default;

        // Dirty state
        //-------------------------------------------------------------------------

        inline bool IsDirty() const { return m_isDirty; }
        inline void MarkDirty() { m_isDirty = true; }
        inline void ClearDirtyFlag() { m_isDirty = false; }

        // Basic Info
        //-------------------------------------------------------------------------

        virtual InlineString GetLabel() const { return "Item"; }
        virtual Color GetColor() const { return Color( 0x606060FF ); }

        // Time manipulation
        //-------------------------------------------------------------------------
        // This is done to abstract the timeline's time format from the item's format since they may differ (e.g. item time range might be in seconds, while timeline is in frames)

        // Return the time range of the item in timeline units
        virtual FloatRange GetTimeRange() const = 0;

        // Get the length of the item in timeline units
        inline float GetLength() const { return GetTimeRange().GetLength(); }

        inline bool IsImmediateItem() const { return GetTimeRange().m_begin == GetTimeRange().m_end; }

        inline bool IsDurationItem() const { return GetTimeRange().m_begin != GetTimeRange().m_end; }

        // Context menu
        //-------------------------------------------------------------------------

        virtual bool HasContextMenu() const { return false; }
        virtual void DrawContextMenu() {}

        // Serialization
        //-------------------------------------------------------------------------

        // Optional function called when loading to initialize the event time range if not stored in the event itself
        virtual void InitializeTimeRange( FloatRange const& inRange ) {}

        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& dataObjectValue ) {}
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const {}

    private:

        // Set the time range of the item in timeline units
        void SetTimeRange( FloatRange const& inRange )
        {
            SetTimeRangeInternal( inRange );
            m_isDirty = true;
        }

        // Converts from timeline units to the unit that the item is stored in (e.g. converts from frames to seconds)
        virtual void SetTimeRangeInternal( FloatRange const& inRange ) = 0;

        TrackItem( TrackItem const& ) = delete;
        TrackItem& operator=( TrackItem& ) = delete;

    private:

        bool                    m_isDirty = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API Track : public IRegisteredType
    {
        EE_REGISTER_TYPE( Track );
        friend class TrackContainer;

    public:

        Track() = default;
        virtual ~Track();

        // Info
        //-------------------------------------------------------------------------

        virtual const char* GetLabel() const { return "Track"; }
        virtual void DrawHeader( ImRect const& headerRect );

        bool IsDirty() const;
        void ClearDirtyFlags();
        inline void MarkDirty() { m_isDirty = true; }

        // Optional call to ensure that the authored track data is valid!
        virtual bool ValidateTrackContents() const { return true; }

        // UI
        //-------------------------------------------------------------------------

        virtual bool HasContextMenu() const { return false; }
        virtual void DrawContextMenu( TVector<Track*>& tracks, float playheadPosition ) {}

        // Items
        //-------------------------------------------------------------------------

        inline bool Contains( TrackItem const* pItem ) const { return eastl::find( m_items.begin(), m_items.end(), pItem ) != m_items.end(); }
        inline TVector<TrackItem*> const& GetItems() const { return m_items; }

        // Serialization
        //-------------------------------------------------------------------------

        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue ) {}
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const {}

    private:

        // Create a new item at the specified start time
        inline void CreateItem( float itemStartTime )
        {
            CreateItemInternal( itemStartTime );
            m_isDirty = true;
        };

        // Try to delete the item from the track if it exists, return true if the item was found and remove, false otherwise
        inline bool DeleteItem( TrackItem* pItem )
        {
            auto foundIter = eastl::find( m_items.begin(), m_items.end(), pItem );
            if ( foundIter != m_items.end() )
            {
                EE::Delete( *foundIter );
                m_items.erase( foundIter );
                m_isDirty = true;
                return true;
            }

            return false;
        }

        // Needs to be implemented by the derived track
        virtual void CreateItemInternal( float itemStartTime ) = 0;

        Track( Track const& ) = delete;
        Track& operator=( Track& ) = delete;

    protected:

        TVector<TrackItem*>      m_items;
        bool                     m_isDirty = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API TrackContainer
    {

    public:

        static TEvent<TrackContainer*>      s_onBeginModification;
        static TEvent<TrackContainer*>      s_onEndModification;

        constexpr static char const*        s_trackContainerKey = "TimelineTracks";

    public:

        // Frees all tracks
        void Reset();

        // Dirty state
        //-------------------------------------------------------------------------

        bool IsDirty() const;
        void ClearDirtyFlags();
        inline void MarkDirty() { m_isDirty = true; }

        // Undo/Redo support
        //-------------------------------------------------------------------------

        void BeginModification();
        void EndModification();

        // Query
        //-------------------------------------------------------------------------

        bool Contains( Track const* pTrack ) const;
        bool Contains( TrackItem const* pItem ) const;

        // Tracks and Item Edition
        //-------------------------------------------------------------------------

        inline int32_t GetNumTracks() const { return (int32_t) m_tracks.size(); }
        TVector<Track*>& GetTracks() { return m_tracks; }
        TVector<Track*> const& GetTracks() const { return m_tracks; }
        inline Track*& GetTrack( size_t i ) { return m_tracks[i]; }
        inline Track* const& GetTrack( size_t i ) const { return m_tracks[i]; }

        template<typename T> T* CreateTrack()
        {
            BeginModification();
            static_assert( std::is_base_of<Track, T>::value, "T has to derive from Timeline::Track" );
            auto pCreatedTrack = static_cast<T*>( m_tracks.emplace_back( EE::New<T>() ) );
            EndModification();

            return pCreatedTrack;
        }

        void DeleteTrack( Track* pTrack );

        void CreateItem( Track* pTrack, float itemStartTime );

        void UpdateItemTimeRange( TrackItem* pItem, FloatRange const& newTimeRange );

        void DeleteItem( TrackItem* pItem );

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