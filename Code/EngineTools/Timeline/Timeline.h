#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Types/Arrays.h"
#include "Base/Math/NumericRange.h"
#include "Base/Types/Color.h"
#include "Base/Types/Event.h"
#include "Base/Time/Time.h"
#include "Base/Types/Function.h"
#include "Base/TypeSystem/TypeInstance.h"

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
        friend class TimelineEditor;

    public:

        TrackContext() = default;

        TrackContext( float length, float unitsPerSecond = 1.0f )
            : m_timelineLength( length )
            , m_unitsPerSeconds( unitsPerSecond )
        {
            EE_ASSERT( m_timelineLength > 0 );
            EE_ASSERT( m_unitsPerSeconds > 0 );
        }

        // Start a timeline modification action (starts recording into the undo buffer)
        void BeginModification() const;

        // Ends a timeline modification action (end recording into the undo buffer)
        void EndModification() const;

        // Get the current timeline length
        EE_FORCE_INLINE float const& GetTimelineLength() const { return m_timelineLength; }

        // Get the units per second conversion factor
        EE_FORCE_INLINE float const& GetUnitsPerSecondConversionFactor() const { return m_unitsPerSeconds; }

    private:

        float               m_timelineLength = 0.0f;
        float               m_unitsPerSeconds = 1.0f;
        TFunction<void()>   m_beginModification;
        TFunction<void()>   m_endModification;
        mutable int32_t     m_beginModificationCallCount = 0;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API [[nodiscard]] ScopedModification
    {
    public:

        ScopedModification( TrackContext const& context ) : m_context( context ) { context.BeginModification(); }
        ~ScopedModification() { m_context.EndModification(); }

    private:

        TrackContext const& m_context;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API TrackItem : public IReflectedType
    {
        EE_REFLECT_TYPE( TrackItem );

        friend class TimelineData;

    public:

        TrackItem() = default;
        TrackItem( FloatRange const& inRange );

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

    private:

        TrackItem( TrackItem const& ) = delete;
        TrackItem& operator=( TrackItem& ) = delete;

    private:

        EE_REFLECT()
        float                       m_startTime = 0.0f; // Timeline units

        EE_REFLECT() 
        float                       m_duration = 0.0f; // Timeline units

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
            Hovered,
            Selected,
            SelectedAndHovered,
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

        // What item types are allowed for this track
        inline ItemType GetActualItemType() const { return m_itemType; }

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
        String const& GetStatusMessage() const { return m_validationStatusMessage; }

        // Visuals
        //-------------------------------------------------------------------------

        // Draw the track header section
        void DrawHeader( TrackContext const& context, ImRect const& headerRect );

        // Get the desired track height (in pixels)
        virtual float GetTrackHeight() const { return 30.0f; }

        // Draw the custom context menu options - return true if you've made major changes to the the track i.e. deleted an item
        virtual bool DrawContextMenu( TrackContext const& context, TVector<TTypeInstance<Track>>& tracks, float playheadPosition ) { return false; }

        // Calculate the rect needed to draw this item
        virtual ImRect CalculateImmediateItemRect( TrackItem* pItem, Float2 const& itemPos ) const;

        // Calculate the rect needed to draw this item
        virtual ImRect CalculateDurationItemRect( TrackItem* pItem, Float2 const& itemStartPos, float itemWidth ) const;

        // Draws an immediate item and returns the hover rect for the drawn item. This rect defines drag zone.
        virtual void DrawImmediateItem( TrackContext const& context, ImDrawList* pDrawList, ImRect const& itemRect, ItemState itemState, TrackItem* pItem );

        // Draws a duration item and returns the hover/interaction rect for the drawn item. This rect defines the resize handle locations and drag zone.
        virtual void DrawDurationItem( TrackContext const& context, ImDrawList* pDrawList, ImRect const& itemRect, ItemState itemState, TrackItem* pItem );

        // Get extra info for a given item
        virtual InlineString GetItemTooltip( TrackItem* pItem ) const { return InlineString(); }

        // Items
        //-------------------------------------------------------------------------
        // Item details are provided by the track so that when we implement new item types we only need to derive the track

        // Get the number of items in this track
        inline int32_t GetNumItems() const { return (int32_t) m_items.size(); }

        // Get all the items in this track
        inline TVector<TTypeInstance<TrackItem>> const& GetItems() const { return m_items; }

        // Get all the items in this track
        inline TVector<TTypeInstance<TrackItem>>& GetItems() { return m_items; }

        // Does this track contain a specific item
        inline bool Contains( TrackItem const* pItem ) const { return eastl::find( m_items.begin(), m_items.end(), pItem ) != m_items.end(); }

        // Get the label for a given item
        virtual InlineString GetItemLabel( TrackItem const* pItem ) const { return ""; }

        // Get the color for a given item
        virtual Color GetItemColor( TrackItem const* pItem ) const { return Color( 0xFFAAAAAA ); }

        // Draw the custom context menu options for this item - return true if you've made major changes to the the track i.e. deleted an item
        virtual bool DrawItemContextMenu( TrackContext const& context, TrackItem* pItem ) { return false; }

        // Can we create new items on this track (certain tracks have restrictions regarding this)
        virtual bool CanCreateNewItems() const { return true; }

        // Utilities
        //-------------------------------------------------------------------------

        // Expand specified duration item to fill the gaps
        void GrowItemToFillGap( TrackContext const& context, TrackItem const* pItem );

        // Fill all gaps between duration items
        void FillGapsForDurationItems( TrackContext const& context );

    protected:

        virtual void DrawExtraHeaderWidgets( ImRect const& widgetsRect ) {}

        static Color GetItemBackgroundColor( ItemState itemState );

        Color GetItemDisplayColor( TrackItem const* pItem, ItemState itemState );

        int32_t GetItemIndex( TrackItem const* pItem ) const;

        void ResetStatusMessage() const { m_validationStatusMessage = "Track is Valid"; }

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
        ItemType                                    m_itemType = ItemType::Duration;

        // The actual items
        EE_REFLECT()
        TVector<TTypeInstance<TrackItem>>           m_items;

        // String set via validation
        mutable String                              m_validationStatusMessage;
    };

    //-------------------------------------------------------------------------
    // Timeline Data
    //-------------------------------------------------------------------------
    // This is the timeline that the timeline editor operates on
    // Provides information about time units, available tracks, etc...

    class EE_ENGINETOOLS_API TimelineData : public IReflectedType
    {

        EE_REFLECT_TYPE( TimelineData );

    public:

        TimelineData() = default;
        virtual ~TimelineData();

        // Frees all tracks - not part of undo/redo
        void Reset();

        // Info
        //-------------------------------------------------------------------------

        virtual void FillAllowedTrackTypes( TypeSystem::TypeRegistry const& typeRegistry ) {};

        // Get all the track types that are allowed to be added to this timeline
        inline TVector<TypeSystem::TypeInfo const*> const& GetAllowedTrackTypes() const { return m_allowedTrackTypes; }

        // Get the overall status for all tracks
        Track::Status GetValidationStatus( TrackContext const& context ) const;

        // Tracks
        //-------------------------------------------------------------------------

        inline int32_t GetNumTracks() const { return (int32_t) m_tracks.size(); }

        TVector<TTypeInstance<Track>>& GetTracks() { return m_tracks; }

        TVector<TTypeInstance<Track>> const& GetTracks() const { return m_tracks; }

        inline Track* GetTrack( size_t i ) { return m_tracks[i].Get(); }

        inline Track const* GetTrack( size_t i ) const { return m_tracks[i].Get(); }

        // Does this container own this track?
        bool Contains( Track const* pTrack ) const;

        // Create a new track
        Track* CreateTrack( TrackContext const& context, TypeSystem::TypeInfo const* pTrackTypeInfo, ItemType itemType );

        // Delete a track and all items
        void DeleteTrack( TrackContext const& context, Track* pTrack );

        // Items
        //-------------------------------------------------------------------------

        // Create a new item
        void CreateItem( TrackContext const& context, Track* pTrack, float itemStartTime );

        // Delete an item
        void DeleteItem( TrackContext const& context, TrackItem* pItem );

        // Update the start/end time of an item
        void UpdateItemTimeRange( TrackContext const& context, TrackItem* pItem, FloatRange const& newTimeRange );

        // Get the track that this item belongs to, if it exists
        Track* GetTrackForItem( TrackItem const* pItem );

        // Get the track that this item belongs to, if it exists
        Track const* GetTrackForItem( TrackItem const* pItem ) const;

        // Does this container own this item?
        bool Contains( TrackItem const* pItem ) const;

    protected:

        EE_REFLECT()
        TVector<TTypeInstance<Track>>               m_tracks;

        // Not serialized - expected to be filled manually
        TVector<TypeSystem::TypeInfo const*>        m_allowedTrackTypes;
    };
}