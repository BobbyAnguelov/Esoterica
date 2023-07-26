#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/Serialization/JsonSerialization.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Types/Arrays.h"
#include "Base/Math/NumericRange.h"
#include "Base/Types/Color.h"
#include "Base/Types/Event.h"
#include "Base/Time/Time.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

struct ImRect;
struct ImDrawList;

//-------------------------------------------------------------------------

namespace EE::Timeline
{
    enum class ItemType
    {
        EE_REFLECT_ENUM

        Immediate,
        Duration,
        Both
    };

    //-------------------------------------------------------------------------

    class TrackContext
    {
        friend class TimelineData;

    public:

        TrackContext() = default;

        // Start a timeline modification action (starts recording into the undo buffer)
        void BeginModification() const { m_beginModification(); }

        // Ends a timeline modification action (end recording into the undo buffer)
        void EndModification() const { m_endModification(); }

        // Get the current timeline length
        EE_FORCE_INLINE float const& GetTimelineLength() const { return m_timelineLength; }

    private:

        float               m_timelineLength;
        TFunction<void()>   m_beginModification;
        TFunction<void()>   m_endModification;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API [[nodiscard]] ScopedTimelineModification
    {
    public:

        ScopedTimelineModification( TrackContext const& context ) : m_context( context ) { context.BeginModification(); }
        ~ScopedTimelineModification() { m_context.EndModification(); }

    private:

        TrackContext const& m_context;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API TrackItem final : public IReflectedType
    {
        EE_REFLECT_TYPE( TrackItem );

        friend class TimelineData;

        constexpr static char const* s_itemDataKey = "ItemData";

    public:

        TrackItem() = default;
        TrackItem( FloatRange const& inRange, IReflectedType* pData );
        ~TrackItem();

        // Info
        //-------------------------------------------------------------------------

        // Return the start time of the item in timeline units
        inline float GetStartTime() const { return m_startTime; }

        // Return the end time of the item in timeline units
        inline float GetEndTime() const { return m_startTime + m_duration; }

        // Return the time range of the item in timeline units
        inline FloatRange GetTimeRange() const { return FloatRange( m_startTime, m_startTime + m_duration ); };

        // Get the duration of the item in timeline units
        inline float GetDuration() const { return m_duration; }

        // Is this an immediate (duration = 0) item
        inline bool IsImmediateItem() const { return GetTimeRange().m_begin == GetTimeRange().m_end; }

        // Is this a duration item
        inline bool IsDurationItem() const { return GetTimeRange().m_begin != GetTimeRange().m_end; }

        // Set the time range of the item in timeline units
        void SetTimeRange( FloatRange const& inRange )
        {
            m_startTime = inRange.m_begin;
            m_duration = inRange.GetLength();
            m_isDirty = true;
        }

        // Data
        //-------------------------------------------------------------------------

        inline IReflectedType* GetData() { return m_pData; }
        inline IReflectedType const* GetData() const { return m_pData; }
        TypeSystem::TypeInfo const* GetDataTypeInfo() const { return m_pData->GetTypeInfo(); }

        // Serialization
        //-------------------------------------------------------------------------

        void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& dataObjectValue );
        void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const;

    private:

        TrackItem( TrackItem const& ) = delete;
        TrackItem& operator=( TrackItem& ) = delete;

    private:

        EE_REFLECT()
        float                       m_startTime = 0.0f; // Timeline units

        EE_REFLECT() 
        float                       m_duration = 0.0f; // Timeline units
        IReflectedType*             m_pData = nullptr;
        bool                        m_isDirty = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API Track : public IReflectedType
    {
        EE_REFLECT_TYPE( Track );
        friend class TimelineData;

    public:

        enum class Status
        {
            Valid = 0,
            HasWarnings,
            HasErrors
        };

        enum class ItemState
        {
            None,
            Selected,
            Edited
        };

    public:

        Track() { ResetStatusMessage(); }
        virtual ~Track();

        // Track Rules
        //-------------------------------------------------------------------------

        // Can this track have a custom name?
        virtual bool IsRenameable() const { return false; }

        // Are we allowed multiple instances of this track?
        virtual bool AllowMultipleTracks() const { return false; }

        // What item types are allowed for this track
        virtual ItemType GetAllowedItemType() const { return ItemType::Duration; }

        // Does this track have custom context menu options
        virtual bool HasContextMenu() const { return false; }

        // Track Info
        //-------------------------------------------------------------------------

        // Get the name of this track type
        virtual const char* GetTypeName() const;

        // Gets the track's name (some tracks are renameable - for non-renameable tracks this is the same as the type label)
        // Note: names are not guaranteed to be unique and are only there to help user organize multiple tracks of the same type.
        virtual const char* GetName() const { return GetTypeName(); }

        // Sets the track's name
        // Note: names are not guaranteed to be unique and are only there to help user organize multiple tracks of the same type.
        virtual void SetName( String const& newName ) { EE_UNREACHABLE_CODE(); }

        // Get the track status
        virtual Status GetValidationStatus( TrackContext const& context ) const { return Status::Valid; }

        // Get the message to show in the validation status indicator tooltip
        String const& GetStatusMessage() const { return m_validationStatueMessage; }

        // Visuals
        //-------------------------------------------------------------------------

        // Draw the track header section
        void DrawHeader( TrackContext const& context, ImRect const& headerRect );

        // Get the desired track height (in pixels)
        virtual float GetTrackHeight() const { return 30.0f; }

        // Draw the custom context menu options
        virtual void DrawContextMenu( TrackContext const& context, TVector<Track*>& tracks, float playheadPosition ) {}

        // Draws an immediate item and returns the hover rect for the drawn item. This rect defines drag zone.
        virtual ImRect DrawImmediateItem( TrackContext const& context, ImDrawList* pDrawList, TrackItem* pItem, Float2 const& itemPos, ItemState itemState );

        // Draws a duration item and returns the hover/interaction rect for the drawn item. This rect defines the resize handle locations and drag zone.
        virtual ImRect DrawDurationItem( TrackContext const& context, ImDrawList* pDrawList, TrackItem* pItem, Float2 const& itemStartPos, Float2 const& itemEndPos, ItemState itemState );

        // Get extra info for a given item
        virtual InlineString GetItemTooltip( TrackItem* pItem ) const { return InlineString(); }

        // Items
        //-------------------------------------------------------------------------
        // Item details are provided by the track so that when we implement new item types we only need to derive the track

        // Get the number of items in this track
        inline int32_t GetNumItems() const { return (int32_t) m_items.size(); }

        // Get all the items in this track
        inline TVector<TrackItem*> const& GetItems() const { return m_items; }

        // Does this track contain a specific item
        inline bool Contains( TrackItem const* pItem ) const { return eastl::find( m_items.begin(), m_items.end(), pItem ) != m_items.end(); }

        // Get the label for a given item
        virtual InlineString GetItemLabel( TrackItem const* pItem ) const { return ""; }

        // Get the color for a given item
        virtual Color GetItemColor( TrackItem const* pItem ) const { return Color( 0xFFAAAAAA ); }

        // Does this item have any custom context menu options
        virtual bool HasItemContextMenu( TrackItem const* pItem ) const { return false; }

        // Draw the custom context menu options for this item
        virtual void DrawItemContextMenu( TrackItem* pItem ) {}

        // Can we create new items on this track (certain tracks have restrictions regarding this)
        virtual bool CanCreateNewItems() const { return true; }

        // Serialization
        //-------------------------------------------------------------------------

        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue ) {}
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const {}

    protected:

        virtual void DrawExtraHeaderWidgets( ImRect const& widgetsRect ) {}

        static Color GetItemBackgroundColor( ItemState itemState, bool isHovered );

        void ResetStatusMessage() const { m_validationStatueMessage = "Track is Valid"; }

    private:

        // Create a new item at the specified start time
        void CreateItem( TrackContext const& context, float itemStartTime );

        // Try to delete the item from the track if it exists, return true if the item was found and remove, false otherwise
        bool DeleteItem( TrackContext const& context, TrackItem* pItem );

        // Needs to be implemented by the derived track
        virtual TrackItem* CreateItemInternal( TrackContext const& context, float itemStartTime ) = 0;

        // Optional function called whenever a new item is created
        virtual void PostCreateItem( TrackItem* pNewItem ) {}

        Track( Track const& ) = delete;
        Track& operator=( Track& ) = delete;

    protected:

        // The specified item type for this track - only relevant if the allowed item type is "Both"
        EE_REFLECT()
        ItemType                    m_itemType = ItemType::Duration;

        // The actual items
        TVector<TrackItem*>         m_items;

        // String set via validation 
        mutable String              m_validationStatueMessage;
    };

    //-------------------------------------------------------------------------
    // Timeline Data
    //-------------------------------------------------------------------------
    // This is the timeline that the timeline editor operates on
    // Provides information about time units, available tracks, etc...

    class EE_ENGINETOOLS_API TimelineData
    {

    public:

        constexpr static char const* s_timelineDataKey = "TimelineTracks";

    public:

        TimelineData( TFunction<void()>& onBeginModification, TFunction<void()>& onEndModification );
        TimelineData( TFunction<void()>&& onBeginModification, TFunction<void()>&& onEndModification ) : TimelineData( onBeginModification, onEndModification ) {}
        virtual ~TimelineData();

        // Frees all tracks - not part of undo/redo
        void Reset();

        // Info
        //-------------------------------------------------------------------------

        // Get all the track types that are allowed to be added to this timeline
        inline TVector<TypeSystem::TypeInfo const*> const& GetAllowedTrackTypes() const { return m_allowedTrackTypes; }

        // Convert from seconds to timeline units - allows for preview in the tool - common will convert from seconds to frames
        virtual float ConvertSecondsToTimelineUnit( Seconds const inTime ) const { return inTime.ToFloat(); }

        // Get the overall status for all tracks
        Track::Status GetValidationStatus() const;

        // Get the length for this timeline
        EE_FORCE_INLINE float const& GetLength() const { return m_length; }

        // Set the overall time range for this timeline
        inline void SetLength( float length ) { EE_ASSERT( length > 0.0f ); m_length = length; m_context.m_timelineLength = length; }

        // Tracks
        //-------------------------------------------------------------------------

        inline TrackContext const& GetTrackContext() const { return m_context; }

        inline int32_t GetNumTracks() const { return (int32_t) m_tracks.size(); }

        TVector<Track*>& GetTracks() { return m_tracks; }

        TVector<Track*> const& GetTracks() const { return m_tracks; }

        inline Track* GetTrack( size_t i ) { return m_tracks[i]; }

        inline Track const* GetTrack( size_t i ) const { return m_tracks[i]; }

        // Does this container own this track?
        bool Contains( Track const* pTrack ) const;

        // Create a new track
        Track* CreateTrack( TypeSystem::TypeInfo const* pTrackTypeInfo, ItemType itemType );

        // Delete a track and all items
        void DeleteTrack( Track* pTrack );

        // Items
        //-------------------------------------------------------------------------

        // Create a new item
        void CreateItem( Track* pTrack, float itemStartTime );

        // Delete an item
        void DeleteItem( TrackItem* pItem );

        // Update the start/end time of an item
        void UpdateItemTimeRange( TrackItem* pItem, FloatRange const& newTimeRange );

        // Get the track that this item belongs to, if it exists
        Track* GetTrackForItem( TrackItem const* pItem );

        // Get the track that this item belongs to, if it exists
        Track const* GetTrackForItem( TrackItem const* pItem ) const;

        // Does this container own this item?
        bool Contains( TrackItem const* pItem ) const;

        // Undo/Redo
        //-------------------------------------------------------------------------

        // Start an operation that modifies timeline data
        void BeginModification();

        // End an operation that modifies timeline data
        void EndModification();

        // Serialization
        //-------------------------------------------------------------------------

        virtual bool Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue );
        virtual void Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer );

    protected:

        TVector<Track*>                             m_tracks;
        float                                       m_length = 0;
        TVector<TypeSystem::TypeInfo const*>        m_allowedTrackTypes;

        TrackContext                                m_context;
        TFunction<void()>                           m_onBeginModification;
        TFunction<void()>                           m_onEndModification;
        mutable int32_t                             m_beginModificationCallCount = 0;
    };
}