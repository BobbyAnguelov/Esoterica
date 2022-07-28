#pragma once

#include "EngineTools/_Module/API.h"
#include "System/Serialization/JsonSerialization.h"
#include "System/TypeSystem/RegisteredType.h"
#include "System/Types/Arrays.h"
#include "System/Math/NumericRange.h"
#include "System/Types/Color.h"

//-------------------------------------------------------------------------

struct ImRect;

//-------------------------------------------------------------------------

namespace EE::Timeline
{
    class EE_ENGINETOOLS_API TrackItem final : public IRegisteredType
    {
        EE_REGISTER_TYPE( TrackItem );

        friend class TrackContainer;

        constexpr static char const* s_itemDataKey = "ItemData";

    public:

        TrackItem() = default;
        TrackItem( FloatRange const& inRange, IRegisteredType* pData );
        ~TrackItem();

        // Info
        //-------------------------------------------------------------------------

        // Return the time range of the item in timeline units
        inline FloatRange GetTimeRange() const { return FloatRange( m_startTime, m_startTime + m_duration ); };

        // Get the length of the item in timeline units
        inline float GetLength() const { return GetTimeRange().GetLength(); }

        // Is this an immediate (duration = 0) item
        inline bool IsImmediateItem() const { return GetTimeRange().m_begin == GetTimeRange().m_end; }

        // Is this a duration item
        inline bool IsDurationItem() const { return GetTimeRange().m_begin != GetTimeRange().m_end; }

        // Data
        //-------------------------------------------------------------------------

        inline IRegisteredType* GetData() { return m_pData; }
        inline IRegisteredType const* GetData() const { return m_pData; }
        TypeSystem::TypeInfo const* GetDataTypeInfo() const { return m_pData->GetTypeInfo(); }

        // Dirty state
        //-------------------------------------------------------------------------

        inline bool IsDirty() const { return m_isDirty; }
        inline void MarkDirty() { m_isDirty = true; }
        inline void ClearDirtyFlag() { m_isDirty = false; }

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

        EE_EXPOSE float         m_startTime = 0.0f;
        EE_EXPOSE float         m_duration = 0.0f;
        IRegisteredType*        m_pData = nullptr;
        bool                    m_isDirty = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API Track : public IRegisteredType
    {
        EE_REGISTER_TYPE( Track );
        friend class TrackContainer;

    public:

        enum class Status
        {
            Valid = 0,
            HasWarnings,
            HasErrors
        };

    public:

        Track() = default;
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
        virtual Status GetStatus() const { return Status::Valid; }

        // Get the message to show in the status indicator tooltip
        String const& GetStatusMessage() const { return m_statusMessage; }

        // Dirty State
        //-------------------------------------------------------------------------

        bool IsDirty() const;
        void ClearDirtyFlags();
        inline void MarkDirty() { m_isDirty = true; }

        // UI
        //-------------------------------------------------------------------------

        void DrawHeader( ImRect const& headerRect );
        virtual bool HasContextMenu() const { return false; }
        virtual void DrawContextMenu( TVector<Track*>& tracks, float playheadPosition ) {}

        // Items
        //-------------------------------------------------------------------------
        // Item details are provided by the track so that when we implement new item types we only need to derive the track

        inline bool Contains( TrackItem const* pItem ) const { return eastl::find( m_items.begin(), m_items.end(), pItem ) != m_items.end(); }
        inline TVector<TrackItem*> const& GetItems() const { return m_items; }

        // Get the label for a given item
        virtual InlineString GetItemLabel( TrackItem const* pItem ) const { return "Item"; }

        // Get the color for a given item
        virtual Color GetItemColor( TrackItem const* pItem ) const { return Color( 0xAAAAAAFF ); }

        // Does this item have any custom context menu options
        virtual bool HasItemContextMenu( TrackItem const* pItem ) const { return false; }

        // Draw the custom context menu options for this item
        virtual void DrawItemContextMenu( TrackItem* pItem ) {}

        // Serialization
        //-------------------------------------------------------------------------

        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue ) {}
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const {}

    protected:

        virtual void DrawExtraHeaderWidgets( ImRect const& widgetsRect ) {}

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
        mutable String           m_statusMessage = "Track is Valid";
        bool                     m_isDirty = false;
    };
}