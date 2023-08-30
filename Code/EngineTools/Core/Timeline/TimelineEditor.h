#pragma once

#include "EngineTools/_Module/API.h"
#include "Timeline.h"
#include "Base/Time/Time.h"
#include "Base/ThirdParty/imgui/imgui_internal.h"

//-------------------------------------------------------------------------
// Timeline Widget
//-------------------------------------------------------------------------
// Basic timeline/sequencer style widget

namespace EE::Timeline
{
    class EE_ENGINETOOLS_API TimelineEditor final
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
            void Reset();

            Track*                  m_pTrack = nullptr;
            TrackItem*              m_pItem = nullptr;
            float                   m_playheadTimeForMouse = -1.0f;
        };

        struct VisualTrackItem
        {
            VisualTrackItem() = default;
            VisualTrackItem( TrackItem* pItem, ImRect const& rect ) : m_pItem( pItem ), m_rect( rect ) {}

        public:

            TrackItem*                  m_pItem = nullptr;
            ImRect                      m_rect;
            Track::ItemState            m_itemState = Track::ItemState::None;
        };

        struct VisualTrack
        {
            VisualTrack() = default;
            VisualTrack( Track* pTrack ) : m_pTrack( pTrack ) {}

        public:

            Track*                      m_pTrack = nullptr;
            ImRect                      m_rect;
            ImRect                      m_headerRect;
            TVector<VisualTrackItem>    m_items;
        };

    public:

        TimelineEditor( TimelineData* pTimelineData, TFunction<void()>& onBeginModification, TFunction<void()>& onEndModification );
        TimelineEditor( TimelineData* pTimelineData, TFunction<void()>&& onBeginModification, TFunction<void()>&& onEndModification ) : TimelineEditor( pTimelineData, onBeginModification, onEndModification ) {}

        // Set the max length on the time line
        void SetLength( float length );

        // Set the conversion factor to convert from seconds to timeline units
        void SetTimeUnitConversionFactor( float unitsPerSecond );

        // View
        //-------------------------------------------------------------------------

        inline bool IsFocused() const { return m_isFocused; }

        inline void SetViewRange( FloatRange const& inRange ) { EE_ASSERT( inRange.IsSetAndValid() ); m_viewRange = inRange; }

        // Tracks
        //-------------------------------------------------------------------------

        inline TimelineData const* GetTrackContainer() const { return m_pTimeline; }

        // Get the overall status for the track container
        inline Track::Status GetTrackContainerValidationStatus() const { return m_pTimeline->GetValidationStatus( m_context ); }

        // Update
        //-------------------------------------------------------------------------

        inline void TogglePlayState() { IsPlaying() ? m_playState = PlayState::Paused : m_playState = PlayState::Playing; }
        inline void Play() {  m_playState = PlayState::Playing; }
        inline void Pause() { m_playState = PlayState::Paused; }
        inline bool IsPlaying() const { return m_playState == PlayState::Playing; }
        inline bool IsPaused() const { return m_playState == PlayState::Paused; }

        inline bool IsLoopingEnabled() const { return m_isLoopingEnabled; }
        inline void SetLooping( bool enabled ) { m_isLoopingEnabled = enabled; }

        inline bool IsFrameSnappingEnabled() const { return m_isFrameSnappingEnabled; }
        inline void SetFrameSnapping( bool enabled ) { m_isFrameSnappingEnabled = enabled; }

        // Update and draw the timeline
        void UpdateAndDraw( Seconds deltaTime );

        // Allow any users to handle the default keyboard inputs at a global level in their workspace
        void HandleGlobalKeyboardInputs();

        // Playhead position
        //-------------------------------------------------------------------------

        // Set the playhead time
        void SetPlayheadTime( float desiredPlayheadTime );

        // Get the current playhead time
        inline float GetPlayheadTime() const { return m_playheadTime; }

        // Get the current position as a percentage of the time line
        inline Percentage GetPlayheadTimeAsPercentage() const { return FloatRange( 0, m_context.m_timelineLength ).GetPercentageThrough( m_playheadTime ); }

        // Selection
        //-------------------------------------------------------------------------

        inline TVector<TrackItem*> const& GetSelectedItems() const { return m_selectedItems; }
        void ClearSelection();

        // Extensions
        //-------------------------------------------------------------------------

    private:

        inline float ConvertPixelsToFrames( float inPixels ) const { return inPixels / m_pixelsPerFrame; }
        inline float ConvertFramesToPixels( float inTime ) const { return inTime * m_pixelsPerFrame; }
        inline void SetViewToStart() { m_viewUpdateMode = ViewUpdateMode::GoToStart; }
        inline void SetViewToEnd() { m_viewUpdateMode = ViewUpdateMode::GoToEnd; }

        //-------------------------------------------------------------------------

        // Delete specified track
        inline void DeleteTrack( Track* pTrack ) { m_pTimeline->DeleteTrack( m_context, pTrack ); }

        //-------------------------------------------------------------------------

        // Set the current state of the timeline editor
        void SetPlayState( PlayState newPlayState );

        //-------------------------------------------------------------------------

        // Calculate the size of the regions in the timeline editor
        void CalculateLayout();

        // Create all the child windows and the default ImGui widgets
        void CreateWindowsAndWidgets();

        // Perform all ImGui draw list operations
        void DrawUI();

        // Draw the list of available tracks to add
        void DrawAddTracksMenu();

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

        // The timeline data
        //-------------------------------------------------------------------------

        TimelineData*               m_pTimeline = nullptr;
        TrackContext                m_context;

        // Play State
        //-------------------------------------------------------------------------

        FloatRange                  m_viewRange = FloatRange( 0, 0 );
        float                       m_playheadTime = 0.0f;
        PlayState                   m_playState = PlayState::Paused;

        // Options
        //-------------------------------------------------------------------------

        bool                        m_isLoopingEnabled = false;
        bool                        m_isFrameSnappingEnabled = true;

        // Layout
        //-------------------------------------------------------------------------

        ImDrawList*                 m_pDrawlist = nullptr;
        float                       m_pixelsPerFrame = 10.0f;
        bool                        m_hasScrollbar[2] = { false, false };
        TVector<VisualTrack>        m_visualTracks;
        ImVec2                      m_scrollbarValues;
        ImVec2                      m_desiredTrackAreaSize;
        ImRect                      m_controlsRowRect;
        ImRect                      m_timelineRowRect;
        ImRect                      m_timelineRect;
        ImRect                      m_playheadRect;
        ImRect                      m_trackAreaRect;

        //-------------------------------------------------------------------------

        MouseState                  m_mouseState;
        ItemEditState               m_itemEditState;
        ContextMenuState            m_contextMenuState;

        TVector<TrackItem*>         m_selectedItems;
        TVector<Track*>             m_selectedTracks;

        ViewUpdateMode              m_viewUpdateMode = ViewUpdateMode::ShowFullTimeRange;
        bool                        m_isPlayheadVisible = false;
        bool                        m_isPlayheadHovered = false;
        bool                        m_isDraggingPlayhead = false;
        bool                        m_isContextMenuRequested = false;
        bool                        m_isFocused = false;
    };
}