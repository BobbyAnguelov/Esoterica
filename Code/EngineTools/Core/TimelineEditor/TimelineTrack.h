#pragma once

#include "EngineTools/_Module/API.h"
#include "System/Serialization/JsonSerialization.h"
#include "System/TypeSystem/ReflectedType.h"
#include "System/Types/Arrays.h"
#include "System/Math/NumericRange.h"
#include "System/Types/Color.h"

//-------------------------------------------------------------------------

struct ImRect;
struct ImDrawList;

//-------------------------------------------------------------------------

namespace EE::Timeline
{
    class EE_ENGINETOOLS_API TrackItem final : public IReflectedType
    {
        EE_REFLECT_TYPE( TrackItem );

        friend class TrackContainer;

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

        // Data
        //-------------------------------------------------------------------------

        inline IReflectedType* GetData() { return m_pData; }
        inline IReflectedType const* GetData() const { return m_pData; }
        TypeSystem::TypeInfo const* GetDataTypeInfo() const { return m_pData->GetTypeInfo(); }

        // Dirty state
        //-------------------------------------------------------------------------

        inline bool IsDirty() const { return m_isDirty; }
        inline void MarkDirty() { m_isDirty = true; }
        inline void ClearDirty() { m_isDirty = false; }

        // Serialization
        //-------------------------------------------------------------------------

        void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& dataObjectValue );
        void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const;

    private:

        // Set the time range of the item in timeline units
        void SetTimeRange( FloatRange const& inRange )
        {
            m_startTime = inRange.m_begin;
            m_duration = inRange.GetLength();
            m_isDirty = true;
        }

        TrackItem( TrackItem const& ) = delete;
        TrackItem& operator=( TrackItem& ) = delete;

    private:

        EE_REFLECT() float         m_startTime = 0.0f; // Timeline units
        EE_REFLECT() float         m_duration = 0.0f; // Timeline units
        IReflectedType*        m_pData = nullptr;
        bool                    m_isDirty = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API Track : public IReflectedType
    {
        EE_REFLECT_TYPE( Track );
        friend class TrackContainer;

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

        //-------------------------------------------------------------------------

        // Can this track have a custom name?
        virtual bool IsRenameable() const { return false; }

        // Get the name of this track type
        virtual const char* GetTypeName() const;

        // Gets the track's name (some tracks are renameable - for non-renameable tracks this is the same as the type label)
        // Note: names are not guaranteed to be unique and are only there to help user organize multiple tracks of the same type.
        virtual const char* GetName() const { return GetTypeName(); }

        // Sets the track's name
        // Note: names are not guaranteed to be unique and are only there to help user organize multiple tracks of the same type.
        virtual void SetName( String const& newName ) { EE_UNREACHABLE_CODE(); }

        // Get the track status
        virtual Status GetValidationStatus( float timelineLength ) const { return Status::Valid; }

        // Get the message to show in the status indicator tooltip
        String const& GetStatusMessage() const { return m_statusMessage; }

        // Dirty State
        //-------------------------------------------------------------------------

        bool IsDirty() const;
        void ClearDirty();
        inline void MarkDirty() { m_isDirty = true; }

        // Visuals
        //-------------------------------------------------------------------------

        void DrawHeader( ImRect const& headerRect, FloatRange const& timelineRange );

        virtual float GetTrackHeight() const { return 30.0f; }
        virtual bool HasContextMenu() const { return false; }
        virtual void DrawContextMenu( TVector<Track*>& tracks, float playheadPosition ) {}

        // Draws an immediate item and returns the hover rect for the drawn item. This rect defines drag zone.
        virtual ImRect DrawImmediateItem( ImDrawList* pDrawList, TrackItem* pItem, Float2 const& itemPos, ItemState itemState );

        // Draws a duration item and returns the hover/interaction rect for the drawn item. This rect defines the resize handle locations and drag zone.
        virtual ImRect DrawDurationItem( ImDrawList* pDrawList, TrackItem* pItem, Float2 const& itemStartPos, Float2 const& itemEndPos, ItemState itemState );

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
        virtual Color GetItemColor( TrackItem const* pItem ) const { return Color( 0xAAAAAAFF ); }

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

        static uint32_t GetItemBackgroundColor( ItemState itemState, bool isHovered );

        void ResetStatusMessage() const { m_statusMessage = "Track is Valid"; }

    private:

        // Create a new item at the specified start time
        void CreateItem( float itemStartTime );

        // Try to delete the item from the track if it exists, return true if the item was found and remove, false otherwise
        bool DeleteItem( TrackItem* pItem );

        // Needs to be implemented by the derived track
        virtual TrackItem* CreateItemInternal( float itemStartTime ) = 0;

        // Optional function called whenever a new item is created
        virtual void PostCreateItem( TrackItem* pNewItem ) {}

        Track( Track const& ) = delete;
        Track& operator=( Track& ) = delete;

    protected:

        TVector<TrackItem*>      m_items;
        mutable String           m_statusMessage;
        bool                     m_isDirty = false;
    };
}