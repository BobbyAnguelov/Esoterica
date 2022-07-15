#pragma once

#include "EngineTools/_Module/API.h"
#include "TimelineData.h"
#include "imgui.h"
#include "imgui_internal.h"

//-------------------------------------------------------------------------
// Timeline Widget
//-------------------------------------------------------------------------
// Basic timeline/sequencer style widget

namespace EE::Timeline
{
    class EE_ENGINETOOLS_API TimelineEditor
    {

    protected:

        enum class ViewUpdateMode : uint8_t
        {
            None,
            ShowFullTimeRange,
            GoToStart,
            GoToEnd,
            TrackPlayhead,
        };

        enum class PlayState : uint8_t
        {
            Playing,
            Paused
        };

        enum class ItemEditMode : uint8_t
        {
            None,
            Move,
            ResizeLeft,
            ResizeRight
        };

        struct MouseState
        {
            void Reset();

            Track*                  m_pHoveredTrack = nullptr;
            TrackItem*              m_pHoveredItem = nullptr;
            float                   m_playheadTimeForMouse = -1.0f;
            float                   m_snappedPlayheadTimeForMouse = -1.0f;
            ItemEditMode            m_hoveredItemMode = ItemEditMode::None;
            bool                    m_isHoveredOverTrackEditor = false;
        };

        struct ItemEditState
        {
            void Reset();

            ItemEditMode            m_mode = ItemEditMode::None;
            Track const*            m_pTrackForEditedItem = nullptr;
            TrackItem*              m_pEditedItem = nullptr;
            FloatRange              m_originalTimeRange;
            bool                    m_isEditing = false;
        };

        // The state for the given open context menu
        struct ContextMenuState
        {
            char const* GetContextMenuName() const;
            void Reset();

            Track*                  m_pTrack = nullptr;
            TrackItem*              m_pItem = nullptr;
            float                   m_playheadTimeForMouse = -1.0f;
            bool                    m_isOpen = false;
        };

    public:

        TimelineEditor( IntRange const& inTimeRange );
        virtual ~TimelineEditor();

        TrackContainer const* GetTrackContainer() const { return &m_trackContainer; }

        inline bool IsFocused() const { return m_isFocused; }

        inline void TogglePlayState() { IsPlaying() ? m_playState = PlayState::Paused : m_playState = PlayState::Playing; }
        inline void Play() {  m_playState = PlayState::Playing; }
        inline void Pause() { m_playState = PlayState::Paused; }
        inline bool IsPlaying() const { return m_playState == PlayState::Playing; }
        inline bool IsPaused() const { return m_playState == PlayState::Paused; }

        inline bool IsLoopingEnabled() const { return m_isLoopingEnabled; }
        inline void SetLooping( bool enabled ) { m_isLoopingEnabled = enabled; }

        inline bool IsFrameSnappingEnabled() const { return m_isFrameSnappingEnabled; }
        inline void SetFrameSnapping( bool enabled ) { m_isFrameSnappingEnabled = enabled; }

        // Allow any users to handle the default keyboard inputs at a global level in their workspace
        void HandleGlobalKeyboardInputs();

        // Playhead position
        //-------------------------------------------------------------------------

        // Set the playhead position
        void SetPlayheadPosition( float inPosition );

        // Get the current playhead position
        inline float GetPlayheadPosition() const { return m_playheadTime; }

        // Get the current position as a percentage of the time line
        inline Percentage GetPlayheadPositionAsPercentage() const { return Percentage( m_playheadTime / m_timeRange.m_end ); }

        inline IntRange GetTimeRange() const { return m_timeRange; }

        // Selection
        //-------------------------------------------------------------------------

        inline TVector<TrackItem*> const& GetSelectedItems() const { return m_selectedItems; }
        void ClearSelection();

        // Dirty State
        //-------------------------------------------------------------------------

        // Has any modifications been made to the tracks/events?
        virtual bool IsDirty() const { return m_trackContainer.IsDirty(); }

        inline void MarkDirty() { m_trackContainer.MarkDirty(); }

        // Serialization
        //-------------------------------------------------------------------------

        virtual bool Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& objectValue );
        virtual void Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer );

    protected:

        // This is protected, since most of the time the client will want to provide a custom update function 
        // that handles the side-effects of the draw e.g. play head position update, track changes, etc...
        void Draw();

        inline FloatRange GetTimeRangeAsFloatRange() const{ return FloatRange( float( m_timeRange.m_begin ), float( m_timeRange.m_end ) ); }
        inline FloatRange GetViewRangeAsFloatRange() const{ return FloatRange( float( m_viewRange.m_begin ), float( m_viewRange.m_end ) ); }

        inline void SetTimeRange( IntRange const& inRange ) { EE_ASSERT( inRange.IsSetAndValid() ); m_timeRange = inRange; }
        inline void SetViewRange( IntRange const& inRange ) { EE_ASSERT( inRange.IsSetAndValid() ); m_viewRange = inRange; }

        // Called whenever the play state is switched
        virtual void OnPlayStateChanged() {}

        // Set the playhead position from a percentage over the time range
        inline void SetPlayheadPositionAsPercentage( Percentage inPercentage ) { m_playheadTime = inPercentage.GetClamped( m_isLoopingEnabled ).ToFloat() * m_timeRange.m_end; }

    private:

        inline float ConvertPixelsToFrames( float inPixels ) const { return inPixels / m_pixelsPerFrame; }
        inline float ConvertFramesToPixels( float inTime ) const { return inTime * m_pixelsPerFrame; }
        inline void SetViewToStart() { m_viewUpdateMode = ViewUpdateMode::GoToStart; }
        inline void SetViewToEnd() { m_viewUpdateMode = ViewUpdateMode::GoToEnd; }

        //-------------------------------------------------------------------------

        // Delete specified track
        inline void DeleteTrack( Track* pTrack ) { m_trackContainer.DeleteTrack( pTrack ); }

        //-------------------------------------------------------------------------

        // Set the current state of the timeline editor
        void SetPlayState( PlayState newPlayState );

        //-------------------------------------------------------------------------

        // Provided rect overs only the area within with the controls can be drawn
        void DrawTimelineControls( ImRect const& controlsRect );

        // Provided rect covers only the area for the timeline display, excludes track header region
        void DrawTimeline( ImRect const& timelineRect );

        // Provided rect covers only the area for the timeline display, excludes track header region
        void DrawPlayhead( ImRect const& timelineRect );

        // Provided rect defines the area available to draw multiple tracks (incl. headers and items)
        void DrawTracks( ImRect const& trackAreaRect );

        // Draw the add track menu
        virtual void DrawAddTracksMenu() = 0;

        // Draw the various context menus
        void DrawContextMenu();

        //-------------------------------------------------------------------------

        // Called each frame to process any mouse/keyboard input
        void HandleUserInput();

        // Split out since it can be used globally too
        void HandleKeyboardInput();

        // Called each frame to update the view range
        void UpdateViewRange();

        // Reset the view to the full time range
        void ResetViewRange() { m_viewUpdateMode = ViewUpdateMode::ShowFullTimeRange; }

        // Selection
        //-------------------------------------------------------------------------

        inline bool IsSelected( TrackItem const* pItem ) const { return VectorContains( m_selectedItems, const_cast<TrackItem*>( pItem ) ); }
        inline bool IsSelected( Track const* pTrack ) const { return VectorContains( m_selectedTracks, const_cast<Track*>( pTrack ) ); }

        // Set the selection to a single item
        void SetSelection( TrackItem* pItem );
        void SetSelection( Track* pTrack );

        // Add item to the current selection
        void AddToSelection( TrackItem* pItem );
        void AddToSelection( Track* pTrack );

        // Remove an item from the current selection
        void RemoveFromSelection( TrackItem* pItem );
        void RemoveFromSelection( Track* pTrack );

        // Sanitize the current selection and remove any invalid items
        void EnsureValidSelection();

    protected:

        TrackContainer              m_trackContainer;

        // The total editable time range
        IntRange                    m_timeRange = IntRange( 0, 0 );

        // The current visible time range
        IntRange                    m_viewRange = IntRange( 0, 0 );

        float                       m_pixelsPerFrame = 10.0f;
        float                       m_playheadTime = 0.0f;
        ImRect                      m_timelineRect;

        PlayState                   m_playState = PlayState::Paused;
        ViewUpdateMode              m_viewUpdateMode = ViewUpdateMode::ShowFullTimeRange;
        bool                        m_isLoopingEnabled = false;
        bool                        m_isFrameSnappingEnabled = true;
        bool                        m_isDraggingPlayhead = false;

        MouseState                  m_mouseState;
        ItemEditState               m_itemEditState;
        ContextMenuState            m_contextMenuState;

        TVector<TrackItem*>         m_selectedItems;
        TVector<Track*>             m_selectedTracks;

        bool                        m_isFocused = false;
    };
}