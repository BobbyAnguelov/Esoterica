#include "TimelineEditor.h"
#include "Engine/UpdateContext.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Timeline
{
    constexpr static float const g_controlRowHeight = 30;
    constexpr static float const g_trackHeaderWidth = 200;
    constexpr static float const g_trackHeaderHorizontalPadding = 8.0f;
    constexpr static float const g_timelineHeight = 40;
    constexpr static float const g_timelineBottomPadding = 8.0f;
    constexpr static float const g_trackRowSpacing = 7.0f;
    constexpr static float const g_trackRowHalfSpacing = ( ( g_trackRowSpacing - 1 ) / 2 );
    constexpr static float const g_playheadHalfWidth = 7.0f;
    constexpr static float const g_gapBetweenTimelineAndTrackArea = 4.0f;
    constexpr static float const g_itemHandleWidth = 4;

    static Color const g_headerBackgroundColor( 0xFF3D3837 );
    static Color const g_timelineLabelColor( 0xFFBBBBBB );
    static Color const g_timelineLargeLineColor( 0xFF606060 );
    static Color const g_timelineSmallLineColor( 0xFF444444 );
    static Color const g_timelineRangeEndLineColor( 0x990000FF );

    static Color const g_playheadDefaultColor = 0xFF32CD32;
    static Color const g_playheadHoveredColor = g_playheadDefaultColor.GetScaledColor( 1.20f );
    static Color const g_playheadShadowColor = 0x44000000;
    static Color const g_playheadBorderColor = g_playheadDefaultColor.GetScaledColor( 1.25f );
    static Color const g_trackSeparatorColor( 0xFF999999 );

    //-------------------------------------------------------------------------

    static Color GetItemBaseColor( bool isSelected, bool isItemHovered )
    {
        Color baseItemColor = ImGuiX::Style::s_colorGray0;

        if ( isSelected )
        {
            baseItemColor.ScaleColor( 1.45f );
        }
        else if ( isItemHovered )
        {
            baseItemColor.ScaleColor( 1.15f );
        }

        return baseItemColor;
    }

    //-------------------------------------------------------------------------

    void TimelineEditor::MouseState::Reset()
    {
        m_isHoveredOverTrackEditor = false;
        m_pHoveredTrack = nullptr;
        m_pHoveredItem = nullptr;
        m_hoveredItemMode = ItemEditMode::None;
        m_playheadTimeForMouse = -1.0f;
        m_snappedPlayheadTimeForMouse = -1.0f;
    }

    inline void TimelineEditor::ItemEditState::Reset()
    {
        m_isEditing = false;
        m_pTrackForEditedItem = nullptr;
        m_pEditedItem = nullptr;
        m_mode = ItemEditMode::None;
        m_originalTimeRange.Clear();
    }

    void TimelineEditor::ContextMenuState::Reset()
    {
        m_pTrack = nullptr;
        m_pItem = nullptr;
        m_playheadTimeForMouse = -1.0f;
    }

    //-------------------------------------------------------------------------

    TimelineEditor::TimelineEditor( TimelineData* pTimelineData, TFunction<void()>& onBeginModification, TFunction<void()>& onEndModification )
        : m_pTimeline( pTimelineData )
    {
        EE_ASSERT( pTimelineData != nullptr );

        // Set context modification function bindings
        m_context.m_beginModification = onBeginModification;
        m_context.m_endModification = onEndModification;
    }

    void TimelineEditor::SetLength( float length )
    {
        EE_ASSERT( length >= 0 );
        m_context.m_timelineLength = length;
        m_playheadTime = 0.0f;
        m_viewRange = FloatRange( 0, length );
        m_viewUpdateMode = ViewUpdateMode::ShowFullTimeRange;
    }

    void TimelineEditor::SetTimeUnitConversionFactor( float unitsPerSecond )
    {
        m_context.m_unitsPerSeconds = unitsPerSecond;
    }

    //-------------------------------------------------------------------------

    void TimelineEditor::SetPlayState( PlayState newPlayState )
    {
        if ( m_playState == newPlayState )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( newPlayState == PlayState::Playing )
        {
            m_viewUpdateMode = ViewUpdateMode::TrackPlayhead;
            m_playState = PlayState::Playing;
        }
        else
        {
            m_viewUpdateMode = ViewUpdateMode::None;
            m_playState = PlayState::Paused;
        }
    }

    void TimelineEditor::UpdateViewRange()
    {
        EE_ASSERT( m_timelineRect.GetWidth() > 0.0f );
        float const maxVisibleUnits = Math::Max( 0.0f, m_timelineRect.GetWidth() / m_pixelsPerFrame );
        EE_ASSERT( maxVisibleUnits >= 0.0f );

        // Adjust visible range based on the canvas size
        if ( m_viewRange.GetLength() != maxVisibleUnits )
        {
            m_viewRange.m_end = m_viewRange.m_begin + maxVisibleUnits;
            EE_ASSERT( m_viewRange.IsSetAndValid() );
        }

        // Process any update requests
        //-------------------------------------------------------------------------

        switch ( m_viewUpdateMode )
        {
            case ViewUpdateMode::ShowFullTimeRange:
            {
                float const viewRangeLength = m_context.m_timelineLength + 1;
                m_pixelsPerFrame = Math::Max( 1.0f, Math::Floor( m_timelineRect.GetWidth() / viewRangeLength ) );
                m_viewRange = FloatRange( 0.0f, viewRangeLength );
                m_viewUpdateMode = ViewUpdateMode::None;
                EE_ASSERT( m_viewRange.IsSetAndValid() );
            }
            break;

            case ViewUpdateMode::GoToStart:
            {
                m_viewRange.m_begin = 0;
                m_viewRange.m_end = maxVisibleUnits;
                m_playheadTime = 0;
                m_viewUpdateMode = ViewUpdateMode::None;
                EE_ASSERT( m_viewRange.IsSetAndValid() );
            }
            break;

            case ViewUpdateMode::GoToEnd:
            {
                m_viewRange.m_begin = Math::Max( 0.0f, m_context.m_timelineLength - maxVisibleUnits );
                m_viewRange.m_end = m_viewRange.m_begin + maxVisibleUnits;
                m_playheadTime = m_context.m_timelineLength;
                m_viewUpdateMode = ViewUpdateMode::None;
                EE_ASSERT( m_viewRange.IsSetAndValid() );
            }
            break;

            case ViewUpdateMode::TrackPlayhead:
            {
                if ( !m_viewRange.ContainsInclusive( m_playheadTime ) )
                {
                    // If the playhead is in the last visible range
                    if ( m_playheadTime + maxVisibleUnits >= m_context.m_timelineLength )
                    {
                        m_viewRange.m_begin = Math::Round( Math::Max( 0.0f, m_context.m_timelineLength - maxVisibleUnits ) );
                        m_viewRange.m_end = m_context.m_timelineLength;
                    }
                    else
                    {
                        m_viewRange.m_begin = Math::Round( m_playheadTime );
                        m_viewRange.m_end = m_viewRange.m_begin + maxVisibleUnits;
                    }

                    EE_ASSERT( m_viewRange.IsSetAndValid() );
                }
            }
            break;

            default:
            break;
        }
    }

    void TimelineEditor::SetPlayheadTime( float desiredPlayheadTime )
    {
        m_playheadTime = FloatRange( 0.0f, m_context.m_timelineLength ).GetClampedValue( desiredPlayheadTime );

        if ( m_isFrameSnappingEnabled )
        {
            m_playheadTime = Math::Round( m_playheadTime );
        }

        EE_ASSERT( m_playheadTime >= 0.0f );
    }

    //-------------------------------------------------------------------------

    void TimelineEditor::ClearSelection()
    {
        m_selectedItems.clear();
        m_itemEditState.Reset();
    }

    void TimelineEditor::SetSelection( TrackItem* pItem )
    {
        EE_ASSERT( pItem != nullptr );
        m_selectedTracks.clear();
        m_selectedItems.clear();
        m_selectedItems.emplace_back( pItem );
    }

    void TimelineEditor::SetSelection( Track* pTrack )
    {
        EE_ASSERT( pTrack != nullptr );
        m_selectedItems.clear();
        m_selectedTracks.clear();
        m_selectedTracks.emplace_back( pTrack );
    }

    void TimelineEditor::AddToSelection( TrackItem* pItem )
    {
        EE_ASSERT( pItem != nullptr );
        m_selectedTracks.clear();
        if ( !VectorContains( m_selectedItems, pItem ) )
        {
            m_selectedItems.emplace_back( pItem );
        }
    }

    void TimelineEditor::AddToSelection( Track* pTrack )
    {
        EE_ASSERT( pTrack != nullptr );
        m_selectedItems.clear();
        if ( !VectorContains( m_selectedTracks, pTrack ) )
        {
            m_selectedTracks.emplace_back( pTrack );
        }
    }

    void TimelineEditor::RemoveFromSelection( TrackItem* pItem )
    {
        EE_ASSERT( pItem != nullptr );
        m_selectedItems.erase_first( pItem );
    }

    void TimelineEditor::RemoveFromSelection( Track* pTrack )
    {
        EE_ASSERT( pTrack != nullptr );
        m_selectedTracks.erase_first( pTrack );
    }

    //-------------------------------------------------------------------------

    void TimelineEditor::EnsureValidSelection()
    {
        for ( int32_t i = (int32_t) m_selectedItems.size() - 1; i >= 0; i-- )
        {
            if ( !m_pTimeline->Contains( m_selectedItems[i] ) )
            {
                m_selectedItems.erase_unsorted( m_selectedItems.begin() + i );
            }
        }

        //-------------------------------------------------------------------------

        for ( int32_t i = (int32_t) m_selectedTracks.size() - 1; i >= 0; i-- )
        {
            if ( !m_pTimeline->Contains( m_selectedTracks[i] ) )
            {
                m_selectedTracks.erase_unsorted( m_selectedTracks.begin() + i );
            }
        }
    }

    //-------------------------------------------------------------------------

    void TimelineEditor::DrawAddTracksMenu()
    {
        int32_t numAvailableTracks = 0;
        TVector<TypeSystem::TypeInfo const*> const& trackTypes = m_pTimeline->GetAllowedTrackTypes();
        for ( auto pTypeInfo : trackTypes )
        {
            Track const* const pDefaultTrackInstance = Cast<Track>( pTypeInfo->m_pDefaultInstance );

            bool isAllowedTrackType = true;
            if ( !pDefaultTrackInstance->AllowMultipleTracks() )
            {
                // Check if a track of this type already exists
                for ( TTypeInstance<Track>& track : m_pTimeline->GetTracks() )
                {
                    if ( track.GetInstanceTypeInfo() == pDefaultTrackInstance->GetTypeInfo() )
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

                auto CreateTrackOption = [this, pDefaultTrackInstance, pTypeInfo] ( ItemType itemType, char const* pNameSuffix = nullptr )
                {
                    InlineString menuItemName = pDefaultTrackInstance->GetTypeName();
                    if ( pNameSuffix != nullptr )
                    {
                        menuItemName += pNameSuffix;
                    }

                    if ( ImGui::MenuItem( menuItemName.c_str() ) )
                    {
                        m_pTimeline->CreateTrack( m_context, pTypeInfo, itemType );
                    }
                };

                //-------------------------------------------------------------------------

                if ( pDefaultTrackInstance->GetAllowedItemType() == ItemType::Both )
                {
                    CreateTrackOption( ItemType::Immediate, "(Immediate)" );
                    CreateTrackOption( ItemType::Duration, "(Duration)" );
                }
                else
                {
                    CreateTrackOption( pDefaultTrackInstance->GetAllowedItemType() );
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( numAvailableTracks == 0 )
        {
            ImGui::Text( "No Available Tracks" );
        }
    }

    void TimelineEditor::DrawContextMenu()
    {
        if ( m_isContextMenuRequested )
        {
            ImGui::OpenPopup( "TimelineContextMenu" );
            m_isContextMenuRequested = false;
        }

        //-------------------------------------------------------------------------

        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 4, 8 ) );
        if ( ImGui::BeginPopupContextItem( "TimelineContextMenu" ) )
        {
            // Item Context Menu
            if ( m_contextMenuState.m_pItem != nullptr )
            {
                auto pTrack = m_pTimeline->GetTrackForItem( m_contextMenuState.m_pItem );
                if ( pTrack != nullptr )
                {
                    bool shouldClearSelection = false;

                    // Misc Common options
                    //-------------------------------------------------------------------------

                    shouldClearSelection = pTrack->DrawItemContextMenu( m_context, m_contextMenuState.m_pItem );

                    // Default options
                    //-------------------------------------------------------------------------

                    if ( ImGui::MenuItem( EE_ICON_ARROW_LEFT_RIGHT" Expand event to fill gap" ) )
                    {
                        pTrack->GrowItemToFillGap( m_context, m_contextMenuState.m_pItem );
                    }

                    if ( ImGui::MenuItem( EE_ICON_DELETE" Delete Item" ) )
                    {
                        shouldClearSelection = true;
                        m_pTimeline->DeleteItem( m_context, m_contextMenuState.m_pItem );
                    }

                    //-------------------------------------------------------------------------

                    if ( shouldClearSelection )
                    {
                        ClearSelection();
                    }
                }
            }
            // Track Context Menu
            else if ( m_contextMenuState.m_pTrack != nullptr )
            {
                if ( m_pTimeline->Contains( m_contextMenuState.m_pTrack ) )
                {
                    bool shouldClearSelection = false;

                    // Default Options
                    //-------------------------------------------------------------------------

                    if ( m_contextMenuState.m_pTrack->CanCreateNewItems() && ImGui::MenuItem( EE_ICON_PLUS" Add Item" ) )
                    {
                        // Calculate the appropriate item start time
                        float itemStartTime = ( m_contextMenuState.m_playheadTimeForMouse < 0.0f ) ? m_playheadTime : m_contextMenuState.m_playheadTimeForMouse;

                        if ( m_isFrameSnappingEnabled )
                        {
                            itemStartTime = Math::Round( itemStartTime );
                        }

                        m_pTimeline->CreateItem( m_context, m_contextMenuState.m_pTrack, itemStartTime );
                    }

                    // Custom Options
                    //-------------------------------------------------------------------------

                    shouldClearSelection = m_contextMenuState.m_pTrack->DrawContextMenu( m_context, m_pTimeline->GetTracks(), m_contextMenuState.m_playheadTimeForMouse < 0.0f ? m_playheadTime : m_contextMenuState.m_playheadTimeForMouse );

                    // Default Options
                    //-------------------------------------------------------------------------

                    if ( m_contextMenuState.m_pTrack->GetActualItemType() == Timeline::ItemType::Duration )
                    {
                        if ( ImGui::MenuItem( EE_ICON_ARROW_LEFT_RIGHT" Expand events to fill gaps" ) )
                        {
                            m_contextMenuState.m_pTrack->FillGapsForDurationItems( m_context );
                        }
                    }

                    if ( ImGui::MenuItem( EE_ICON_DELETE" Delete Track" ) )
                    {
                        shouldClearSelection = true;
                        DeleteTrack( m_contextMenuState.m_pTrack );
                    }

                    //-------------------------------------------------------------------------

                    if ( shouldClearSelection )
                    {
                        ClearSelection();
                    }
                }
            }
            else // General Options
            {
                if ( ImGui::BeginMenu( EE_ICON_PLUS" Add Track" ) )
                {
                    DrawAddTracksMenu();
                    ImGui::EndMenu();
                }
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
    }

    //-------------------------------------------------------------------------

    void TimelineEditor::CalculateLayout()
    {
        auto const& style = ImGui::GetStyle();
        ImVec2 const mousePos = ImGui::GetMousePos();
        ImVec2 const canvasSize = ImGui::GetContentRegionAvail();
        ImVec2 const cursorScreenPos = ImGui::GetCursorScreenPos();

        ImGui::PushID( this );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );

        m_controlsRowRect = ImRect( cursorScreenPos, cursorScreenPos + ImVec2( canvasSize.x, g_controlRowHeight + ( style.WindowPadding.y * 2 ) ) );
        m_timelineRowRect = ImRect( m_controlsRowRect.GetBL(), m_controlsRowRect.GetBL() + ImVec2( canvasSize.x, g_timelineHeight ) );
        m_timelineRect = ImRect( m_timelineRowRect.GetTL() + ImVec2( g_trackHeaderWidth, 0 ), m_timelineRowRect.GetBR() - ImVec2( g_playheadHalfWidth, 0 ) );

        //-------------------------------------------------------------------------
        // Calculate Track Area
        //-------------------------------------------------------------------------

        m_trackAreaRect = ImRect( m_timelineRowRect.GetBL() + ImVec2( 0, g_gapBetweenTimelineAndTrackArea ), ImVec2(m_timelineRowRect.GetBL().x + canvasSize.x, cursorScreenPos.y + canvasSize.y) + ImVec2( 0, g_gapBetweenTimelineAndTrackArea ) );

        if ( m_trackAreaRect.Contains( mousePos ) )
        {
            m_mouseState.m_isHoveredOverTrackEditor = true;

            FloatRange const playheadValidRange( (float) Math::Max( m_viewRange.m_begin, 0.0f ), (float) Math::Min( m_viewRange.m_end, m_context.m_timelineLength ) );
            m_mouseState.m_playheadTimeForMouse = m_viewRange.m_begin + ConvertPixelsToFrames( mousePos.x - g_trackHeaderWidth - m_trackAreaRect.Min.x );
            m_mouseState.m_playheadTimeForMouse = playheadValidRange.GetClampedValue( m_mouseState.m_playheadTimeForMouse );
            m_mouseState.m_snappedPlayheadTimeForMouse = m_mouseState.m_playheadTimeForMouse;

            if ( m_isFrameSnappingEnabled )
            {
                m_mouseState.m_snappedPlayheadTimeForMouse = Math::Round( m_mouseState.m_snappedPlayheadTimeForMouse );
            }
        }
        else
        {
            m_mouseState.m_isHoveredOverTrackEditor = false;
        }

        //-------------------------------------------------------------------------
        // Calculate Track Regions
        //-------------------------------------------------------------------------

        m_visualTracks.clear();

        {
            m_desiredTrackAreaSize = ImVec2( ( m_context.m_timelineLength + 1 ) * m_pixelsPerFrame, 0 ); // Always show 1 more unit than the length of the timeline
            ImVec2 trackStartPos = m_trackAreaRect.GetTL() + ImVec2( 0, g_trackRowHalfSpacing );
            for ( TTypeInstance<Track>& track : m_pTimeline->GetTracks() )
            {
                m_desiredTrackAreaSize.y += track->GetTrackHeight();
                m_desiredTrackAreaSize.y += g_trackRowSpacing;

                VisualTrack& visualTrack = m_visualTracks.emplace_back( track.Get() );

                // Calculate full track rect
                visualTrack.m_rect = ImRect( trackStartPos, trackStartPos + ImVec2( m_trackAreaRect.GetWidth(), track->GetTrackHeight() ) );
                if ( visualTrack.m_rect.Contains( mousePos ) )
                {
                    m_mouseState.m_pHoveredTrack = track.Get();
                }

                trackStartPos = visualTrack.m_rect.GetBL() + ImVec2( 0, g_trackRowSpacing );

                // Calculate header rect
                visualTrack.m_headerRect = ImRect( visualTrack.m_rect.GetTL(), ImVec2( visualTrack.m_rect.GetTL().x + g_trackHeaderWidth, visualTrack.m_rect.GetBR().y ) );

                // Evaluate items
                for ( TTypeInstance<TrackItem>& item : track->GetItems() )
                {
                    FloatRange const itemTimeRange = item->GetTimeRange();
                    if ( !m_viewRange.Overlaps( itemTimeRange ) )
                    {
                        continue;
                    }

                    VisualTrackItem* pVisualItem = nullptr;

                    // Calculate position and rect
                    //-------------------------------------------------------------------------

                    Float2 itemPos;
                    itemPos.m_x = visualTrack.m_rect.GetTL().x + g_trackHeaderWidth + ( ( itemTimeRange.m_begin - m_viewRange.m_begin ) * m_pixelsPerFrame );
                    itemPos.m_y = visualTrack.m_rect.GetCenter().y - m_scrollbarValues.y;

                    ImRect const itemRect  = ( item->IsImmediateItem() ) ? track->CalculateImmediateItemRect( item.Get(), itemPos ) : track->CalculateDurationItemRect( item.Get(), itemPos, itemTimeRange.GetLength() * m_pixelsPerFrame );
                    if ( m_trackAreaRect.Overlaps( itemRect ) )
                    {
                        pVisualItem = &visualTrack.m_items.emplace_back( item.Get(), itemRect);
                    }

                    // Set mouse state
                    //-------------------------------------------------------------------------

                    if ( pVisualItem != nullptr && pVisualItem->m_rect.Contains( mousePos ) )
                    {
                        m_mouseState.m_pHoveredItem = item.Get();

                        if ( item->IsImmediateItem() )
                        {
                            m_mouseState.m_hoveredItemMode = ItemEditMode::Move;
                        }
                        else
                        {
                            ImVec2 const visualItemStart = pVisualItem->m_rect.GetTL();
                            ImVec2 const visualItemEnd = pVisualItem->m_rect.GetBR() - ImVec2( 0, m_scrollbarValues.y );
                            bool const isHoveredOverLeftHandle = ImRect( visualItemStart, ImVec2( visualItemStart.x + g_itemHandleWidth, visualItemEnd.y ) ).Contains( mousePos );
                            bool const isHoveredOverRightHandle = ( !isHoveredOverLeftHandle ) ? ImRect( ImVec2( visualItemEnd.x - g_itemHandleWidth, visualItemStart.y ), visualItemEnd ).Contains( mousePos ) : false;

                            if ( isHoveredOverLeftHandle )
                            {
                                m_mouseState.m_hoveredItemMode = ItemEditMode::ResizeLeft;
                            }
                            else if ( isHoveredOverRightHandle )
                            {
                                m_mouseState.m_hoveredItemMode = ItemEditMode::ResizeRight;
                            }
                            else
                            {
                                m_mouseState.m_hoveredItemMode = ItemEditMode::Move;
                            }
                        }
                    }

                    // Set item state
                    //-------------------------------------------------------------------------

                    if ( pVisualItem != nullptr )
                    {
                        pVisualItem->m_itemState = Track::ItemState::None;

                        bool const isEdited = m_itemEditState.m_pEditedItem == item.Get();
                        bool const isHovered = m_mouseState.m_pHoveredItem == item.Get();

                        if ( isEdited )
                        {
                            pVisualItem->m_itemState = Track::ItemState::Edited;
                        }
                        else if ( IsSelected( item.Get() ) )
                        {
                            if ( isHovered )
                            {
                                pVisualItem->m_itemState = Track::ItemState::SelectedAndHovered;
                            }
                            else
                            {
                                pVisualItem->m_itemState = Track::ItemState::Selected;
                            }
                        }
                        else if ( isHovered )
                        {
                            pVisualItem->m_itemState = Track::ItemState::Hovered;
                        }
                    }
                }
            }
        }

        //-------------------------------------------------------------------------
        // Calculate Playhead
        //-------------------------------------------------------------------------

        if ( m_viewRange.ContainsInclusive( m_playheadTime ) )
        {
            constexpr static float const playHeadVerticalPadding = 3.0f;
            float const playheadPosX = m_timelineRect.GetTL().x + ConvertFramesToPixels( m_playheadTime - m_viewRange.m_begin );
            m_playheadRect = ImRect( ImVec2( playheadPosX - g_playheadHalfWidth, m_timelineRect.GetTL().y + playHeadVerticalPadding ), ImVec2( playheadPosX + g_playheadHalfWidth, m_timelineRect.GetBR().y - playHeadVerticalPadding ) );

            m_isPlayheadVisible = true;
            m_isPlayheadHovered = ImGui::IsWindowHovered( ImGuiHoveredFlags_ChildWindows ) && m_playheadRect.Contains( mousePos );
        }
        else
        {
            m_isPlayheadVisible = false;
            m_isPlayheadHovered = false;
        }
    }

    void TimelineEditor::CreateWindowsAndWidgets()
    {
        auto const& style = ImGui::GetStyle();
        ImVec2 const originalItemSpacing = style.ItemSpacing;

        //-------------------------------------------------------------------------
        // Controls Row
        //-------------------------------------------------------------------------

        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
        bool const drawControls = ImGui::BeginChild( "ControlsRow", m_controlsRowRect.GetSize(), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, originalItemSpacing );

        ImGui::BeginDisabled( m_context.m_timelineLength <= 0 );
        if ( drawControls )
        {
            ImVec2 buttonSize = ImVec2( g_controlRowHeight, g_controlRowHeight );

            //-------------------------------------------------------------------------
            // Playback controls
            //-------------------------------------------------------------------------

            if ( ImGui::Button( EE_ICON_REWIND "##GoToStart", buttonSize ) )
            {
                SetViewToStart();
            }

            ImGuiX::ItemTooltip( "Rewind to start" );

            //-------------------------------------------------------------------------

            ImGui::SameLine();

            if ( m_playState == PlayState::Playing )
            {
                if ( ImGui::Button( EE_ICON_PAUSE "##Pause", buttonSize ) )
                {
                    SetPlayState( PlayState::Paused );
                }

                ImGuiX::ItemTooltip( "Pause" );
            }
            else // Paused
            {
                if ( ImGui::Button( EE_ICON_PLAY "##Play", buttonSize ) )
                {
                    SetPlayState( PlayState::Playing );
                }

                ImGuiX::ItemTooltip( "Play" );
            }

            //-------------------------------------------------------------------------

            ImGui::SameLine();

            if ( ImGui::Button( EE_ICON_FAST_FORWARD "##GoToEnd", buttonSize ) )
            {
                SetViewToEnd();
            }

            ImGuiX::ItemTooltip( "Fast-forward to end" );

            //-------------------------------------------------------------------------
            // Separator
            //-------------------------------------------------------------------------

            ImGuiX::SameLineSeparator( 9 );

            //-------------------------------------------------------------------------
            // Options
            //-------------------------------------------------------------------------

            ImGuiX::ToggleButton( EE_ICON_CURSOR_DEFAULT_CLICK"##On", EE_ICON_CURSOR_DEFAULT_CLICK_OUTLINE"##Off", m_isFrameSnappingEnabled, buttonSize, ImGuiX::Style::s_colorText, ImGuiX::Style::s_colorTextDisabled );

            ImGuiX::ItemTooltip( m_isFrameSnappingEnabled ? "Disable frame snapping" : "Enable frame snapping" );

            //-------------------------------------------------------------------------

            ImGui::SameLine();

            if ( IsLoopingEnabled() )
            {
                if ( ImGui::Button( EE_ICON_REPEAT"##PlayOnce", buttonSize ) )
                {
                    m_isLoopingEnabled = false;
                }

                ImGuiX::ItemTooltip( "Disable looping" );
            }
            else // Playing Once
            {
                if ( ImGui::Button( EE_ICON_REPEAT_OFF"##Loop", buttonSize ) )
                {
                    m_isLoopingEnabled = true;
                }

                ImGuiX::ItemTooltip( "Enable looping" );
            }

            //-------------------------------------------------------------------------

            ImGui::SameLine();

            if ( ImGui::Button( EE_ICON_ARROW_EXPAND_HORIZONTAL"##FitToView", buttonSize ) )
            {
                ResetViewRange();
            }

            ImGuiX::ItemTooltip( "Fit To View" );

            //-------------------------------------------------------------------------
            // Time Info
            //-------------------------------------------------------------------------

            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Large, Colors::Green );
                ImGui::SameLine( 0, 8 );
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "%.2f/%.2f", m_playheadTime, m_context.m_timelineLength );
            }
        }
        ImGui::EndDisabled();

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::PopStyleVar();

        //-------------------------------------------------------------------------
        // Timeline Row
        //-------------------------------------------------------------------------

        // Size needs to take into the playhead width
        ImVec2 const timelineHeaderSize( g_trackHeaderWidth - g_playheadHalfWidth, m_timelineRowRect.GetHeight() );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
        bool const drawHeader = ImGui::BeginChild( "TimelineHeader", timelineHeaderSize, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, originalItemSpacing );

        if ( drawHeader )
        {
            ImGui::BeginDisabled( m_context.m_timelineLength <= 0 );
            if( ImGui::Button( EE_ICON_MOVIE_PLUS" Add Tracks", ImVec2( -1, 0 ) ) )
            {
                ImGui::OpenPopup( "AddTracksMenu" );
            }
            ImGui::EndDisabled();

            ImGui::SetNextWindowPos( ImGui::GetCursorScreenPos() - ImVec2( 0, style.ItemSpacing.y - 1 ) );
            ImGui::SetNextWindowSize( ImVec2( ImGui::GetContentRegionAvail().x, 0 ) );
            if ( ImGui::BeginPopup( "AddTracksMenu" ) )
            {
                DrawAddTracksMenu();
                ImGui::EndPopup();
            }
        }

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::PopStyleVar();

        //-------------------------------------------------------------------------
        // Track Area
        //-------------------------------------------------------------------------

        ImVec2 const mousePos = ImGui::GetMousePos();
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + g_gapBetweenTimelineAndTrackArea );
        ImVec2 const trackAreaCursorStartPos = ImGui::GetCursorPos();

        // Tracks
        //-------------------------------------------------------------------------

        {
            ImRect const itemAreaRect( m_trackAreaRect.GetTL() + ImVec2( g_trackHeaderWidth, 0 ), m_trackAreaRect.GetBR() );
            int32_t const itemAreaFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoTitleBar;
            ImGui::SetCursorPos( trackAreaCursorStartPos + ImVec2( g_trackHeaderWidth, 0 ) );
            ImGui::PushStyleColor( ImGuiCol_ChildBg, Colors::Transparent );
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 0, 0 ) );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
            if ( ImGui::BeginChild( "TrackItemArea", itemAreaRect.GetSize(), ImGuiChildFlags_AlwaysUseWindowPadding, itemAreaFlags ) )
            {
                float const contentRegionWidth = Math::Max( ImGui::GetContentRegionAvail().x, 0.0f );

                // Create dummy to let imgui know the actual size that we want.
                ImGui::Dummy( m_desiredTrackAreaSize );

                // Set scroll position
                ImGui::SetScrollX( ConvertFramesToPixels( m_viewRange.m_begin ) );

                // Get scrollbar info
                m_hasScrollbar[0] = ImGui::GetCurrentWindow()->ScrollbarX;
                m_scrollbarValues.x = m_hasScrollbar[0] ? ImGui::GetScrollX() : 0.0f;

                if ( m_hasScrollbar[0] )
                {
                    m_viewRange.m_begin = ConvertPixelsToFrames( m_scrollbarValues[0] );
                    m_viewRange.m_end = m_viewRange.m_begin + ConvertPixelsToFrames( contentRegionWidth );
                }
                else
                {
                    float const viewRangeLength = m_viewRange.GetLength();
                    m_viewRange.m_begin = 0.0f;
                    m_viewRange.m_end = viewRangeLength;
                }

                EE_ASSERT( m_viewRange.IsSetAndValid() );

                //-------------------------------------------------------------------------

                m_hasScrollbar[1] = ImGui::GetCurrentWindow()->ScrollbarY;
                m_scrollbarValues.y = m_hasScrollbar[1] ? ImGui::GetScrollY() : 0.0f;


            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar( 3 );
        }

        // Track Header
        //-------------------------------------------------------------------------
        // This is done second so we can use the up-to-date scrollbar values

        {
            ImRect const headerAreaRect( m_trackAreaRect.GetTL(), ImVec2( m_trackAreaRect.GetTL().x + g_trackHeaderWidth, m_trackAreaRect.GetBR().y - ( m_hasScrollbar[0] ? style.ScrollbarSize : 0 ) ) );
            int32_t const headerAreaFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar;
            ImGui::SetCursorPos( trackAreaCursorStartPos );
            ImGui::PushStyleColor( ImGuiCol_ChildBg, Colors::Transparent );
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
            if ( ImGui::BeginChild( "TrackHeaderArea", headerAreaRect.GetSize(), false, headerAreaFlags ) )
            {
                ImGui::SetScrollY( m_scrollbarValues.y );

                ImGui::Dummy( ImVec2( 1, g_trackRowHalfSpacing ) );

                int32_t const numTracks = (int32_t) m_visualTracks.size();
                for ( int32_t i = 0; i < numTracks; i++ )
                {
                    VisualTrack const& visualTrack = m_visualTracks[i];
                    Track* pTrack = m_pTimeline->GetTrack( i );
                    ImGui::PushID( pTrack );
                    {
                        int32_t const headerFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar;
                        uint32_t const headerID = ImGui::GetID( pTrack );
                        ImGui::PushStyleColor( ImGuiCol_ChildBg, Colors::Transparent );
                        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( g_trackHeaderHorizontalPadding, 0 ) );
                        if ( ImGui::BeginChild( headerID, visualTrack.m_headerRect.GetSize(), ImGuiChildFlags_AlwaysUseWindowPadding, headerFlags ) )
                        {
                            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, originalItemSpacing );
                            pTrack->DrawHeader( m_context, visualTrack.m_headerRect );
                            ImGui::PopStyleVar();
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleVar();
                        ImGui::PopStyleColor();
                    }
                    ImGui::PopID();

                    ImGui::Dummy( ImVec2( 1, g_trackRowSpacing ) );
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }
    }

    void TimelineEditor::DrawUI()
    {
        auto const& style = ImGui::GetStyle();
        m_pDrawlist = ImGui::GetWindowDrawList();

        int32_t const numTracks = (int32_t) m_visualTracks.size();

        //-------------------------------------------------------------------------
        // Draw background
        //-------------------------------------------------------------------------

        {
            m_pDrawlist->AddRectFilled( m_trackAreaRect.GetTL(), m_trackAreaRect.GetBR(), ImGuiX::Style::s_colorGray7 );
            m_pDrawlist->AddRectFilled( m_timelineRowRect.GetBL(), m_trackAreaRect.GetTR(), ImGuiX::Style::s_colorGray7 );

            //-------------------------------------------------------------------------

            ImGui::PushClipRect( m_trackAreaRect.GetTL(), m_trackAreaRect.GetBR() - ImVec2( m_hasScrollbar[1] ? style.ScrollbarSize : 0.0f, m_hasScrollbar[0] ? style.ScrollbarSize : 0.0f ), false );

            for ( int32_t i = 0; i < numTracks; i++ )
            {
                auto pTrack = m_pTimeline->GetTrack( i );
                Color const trackBackgroundColor = ( IsSelected( pTrack ) ? ImGuiX::Style::s_colorGray5 : ImGuiX::Style::s_colorGray6 ).GetAlphaVersion( 0.75f );
                m_pDrawlist->AddRectFilled( m_visualTracks[i].m_rect.GetTL() + ImVec2( g_trackHeaderWidth + 1, -m_scrollbarValues.y ), m_visualTracks[i].m_rect.GetBR() + ImVec2( 0, -m_scrollbarValues.y ), trackBackgroundColor );
                m_pDrawlist->AddLine( m_visualTracks[i].m_rect.GetBL() + ImVec2( 0, g_trackRowHalfSpacing - m_scrollbarValues.y ), m_visualTracks[i].m_rect.GetBR() + ImVec2( g_trackHeaderWidth, g_trackRowHalfSpacing - m_scrollbarValues.y ), 0xFF262626 );
            }

            ImGui::PopClipRect();
        }

        //-------------------------------------------------------------------------
        // Draw Timeline
        //-------------------------------------------------------------------------

        constexpr static float const numFramesForLargeInterval = 10;
        constexpr static float const timelineLabelLeftPadding = 4.0f;
        constexpr static float const timelineLargeLineOffset = 2;
        constexpr static float const timelineMediumLineOffset = 10;
        constexpr static float const timelineSmallLineOffset = 20;

        float const visibleRangeLength = m_viewRange.GetLength();
        float const numFramesForMediumInterval = numFramesForLargeInterval / 2;

        float const startPosX = m_timelineRect.GetTL().x;
        float const startPosY = m_timelineRect.GetTL().y;
        float const endPosX = m_timelineRect.GetBR().x;
        float const endPosY = m_trackAreaRect.GetBR().y;
        float const textPosY = m_timelineRect.GetTR().y;

        // Draw Start Line
        //-------------------------------------------------------------------------

        InlineString label( InlineString::CtorSprintf(), "%.2f", m_viewRange.m_begin );
        m_pDrawlist->AddLine( ImVec2( startPosX, startPosY + timelineLargeLineOffset ), ImVec2( startPosX, endPosY ), g_timelineLargeLineColor, 1 );
        m_pDrawlist->AddText( ImVec2( startPosX + timelineLabelLeftPadding, textPosY ), g_timelineLabelColor, label.c_str() );

        // Draw End Line
        //-------------------------------------------------------------------------

        if ( m_viewRange.ContainsInclusive( m_context.m_timelineLength ) )
        {
            float const lineOffsetX = startPosX + Math::Round( ( m_context.m_timelineLength - m_viewRange.m_begin ) * m_pixelsPerFrame );

            label.sprintf( "%.0f", m_context.m_timelineLength );
            m_pDrawlist->AddLine( ImVec2( lineOffsetX, startPosY + timelineLargeLineOffset ), ImVec2( lineOffsetX, endPosY ), g_timelineRangeEndLineColor, 1 );
            m_pDrawlist->AddText( ImVec2( lineOffsetX + timelineLabelLeftPadding, textPosY ), g_timelineRangeEndLineColor, label.c_str() );
        }

        // Draw remaining lines
        //-------------------------------------------------------------------------

        float deltaStartPosX = 0.0f;
        float startIndex = 1.0f;

        float const startValue = Math::Ceiling( m_viewRange.m_begin );
        if ( startValue != m_viewRange.m_begin )
        {
            deltaStartPosX = ConvertFramesToPixels( startValue - m_viewRange.m_begin );
            startIndex = 0.0f;
        }

        for ( float i = startIndex; i <= visibleRangeLength; i += 1 )
        {
            float const lineOffsetX = startPosX + deltaStartPosX + Math::Round( i * m_pixelsPerFrame );
            if ( lineOffsetX < startPosX || lineOffsetX > endPosX )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            float const lineValue = startValue + i;
            if ( lineValue != m_context.m_timelineLength )
            {
                bool const isLargeLine = ( Math::FModF( lineValue, numFramesForLargeInterval ) ) == 0;
                bool const isMediumLine = ( Math::FModF( lineValue, numFramesForMediumInterval ) ) == 0;

                if ( isLargeLine )
                {
                    m_pDrawlist->AddLine( ImVec2( lineOffsetX, startPosY + timelineLargeLineOffset ), ImVec2( lineOffsetX, endPosY ), g_timelineLargeLineColor, 1 );
                }
                else if ( isMediumLine )
                {
                    m_pDrawlist->AddLine( ImVec2( lineOffsetX, startPosY + timelineMediumLineOffset ), ImVec2( lineOffsetX, endPosY ), g_timelineLargeLineColor, 1 );
                }
                else // Small lines
                {
                    m_pDrawlist->AddLine( ImVec2( lineOffsetX, startPosY + timelineSmallLineOffset ), ImVec2( lineOffsetX, endPosY ), g_timelineSmallLineColor, 1 );
                }

                // Draw text label
                if ( isLargeLine )
                {
                    label.sprintf( "%.0f", lineValue );
                    m_pDrawlist->AddText( ImVec2( lineOffsetX + timelineLabelLeftPadding, textPosY ), g_timelineLabelColor, label.c_str() );
                }
            }
        }

        //-------------------------------------------------------------------------
        // Draw items
        //-------------------------------------------------------------------------

        m_pDrawlist->PushClipRect( m_trackAreaRect.Min + ImVec2( g_trackHeaderWidth, 0 ), m_trackAreaRect.Max - ImVec2( 1, 1 ) - ImVec2( m_hasScrollbar[1] ? style.ScrollbarSize : 0, m_hasScrollbar[0] ? style.ScrollbarSize : 0 ), true );

        for ( int32_t i = 0; i < numTracks; i++ )
        {
            VisualTrack const& visualTrack = m_visualTracks[i];

            for ( auto const& visualItem : visualTrack.m_items )
            {
                // Draw
                if ( visualItem.m_pItem->IsImmediateItem() )
                {
                    visualTrack.m_pTrack->DrawImmediateItem( m_context, m_pDrawlist, visualItem.m_rect, visualItem.m_itemState, visualItem.m_pItem );
                }
                else
                {
                    visualTrack.m_pTrack->DrawDurationItem( m_context, m_pDrawlist, visualItem.m_rect, visualItem.m_itemState, visualItem.m_pItem );
                }

                // Tooltip
                if ( visualItem.m_itemState == Track::ItemState::Hovered || visualItem.m_itemState == Track::ItemState::SelectedAndHovered )
                {
                    InlineString const tooltipText = visualTrack.m_pTrack->GetItemTooltip( visualItem.m_pItem );
                    if ( !tooltipText.empty() )
                    {
                        ImGui::SetTooltip( tooltipText.c_str() );
                    }
                }

                // Debug: Draw calculated rect
                //m_pDrawlist->AddRectFilled( visualItem.m_rect.Min, visualItem.m_rect.Max, Colors::Teal );
            }
        }

        m_pDrawlist->PopClipRect();

        //-------------------------------------------------------------------------
        // Draw Playhead
        //-------------------------------------------------------------------------

        if ( m_isPlayheadVisible )
        {
            float const playheadHeight = m_playheadRect.GetHeight();
            ImVec2 const playheadMarkerPosition( m_playheadRect.GetCenter().x, m_playheadRect.GetBR().y - g_timelineBottomPadding );
            ImVec2 points[5] =
            {
                playheadMarkerPosition,
                playheadMarkerPosition + ImVec2{ -g_playheadHalfWidth, -playheadHeight / 2.0f },
                playheadMarkerPosition + ImVec2{ -g_playheadHalfWidth, -playheadHeight },
                playheadMarkerPosition + ImVec2{ g_playheadHalfWidth, -playheadHeight },
                playheadMarkerPosition + ImVec2{ g_playheadHalfWidth, -playheadHeight / 2.0f }
            };

            Color const playheadColor = ( m_playheadTime != m_context.m_timelineLength ) ? ( m_isPlayheadHovered ? g_playheadHoveredColor : g_playheadDefaultColor ) : g_timelineRangeEndLineColor;
            m_pDrawlist->AddConvexPolyFilled( points, 5, playheadColor );
            m_pDrawlist->AddLine( playheadMarkerPosition, ImVec2( playheadMarkerPosition.x, m_trackAreaRect.GetBR().y - ( m_hasScrollbar[0] ? style.ScrollbarSize : 0 ) ), playheadColor );
        }

        m_pDrawlist = nullptr;
    }

    void TimelineEditor::UpdateAndDraw( Seconds deltaTime )
    {
        m_isFocused = ImGui::IsWindowFocused( ImGuiFocusedFlags_RootAndChildWindows );

        //-------------------------------------------------------------------------
        // Manage play state and input
        //-------------------------------------------------------------------------

        if ( IsPlaying() )
        {
            float const deltaTimelineUnits = m_context.m_unitsPerSeconds * deltaTime;
            m_playheadTime += deltaTimelineUnits;

            if ( m_playheadTime >= m_context.m_timelineLength )
            {
                if ( m_isLoopingEnabled )
                {
                    m_playheadTime = Math::FModF( m_playheadTime, m_context.m_timelineLength );
                }
                else // Clamp to end
                {
                    m_playheadTime = m_context.m_timelineLength;
                    SetPlayState( PlayState::Paused );
                }

                EE_ASSERT( m_playheadTime >= 0.0f );
            }
        }

        // Remove any invalid pointers from the current selection
        EnsureValidSelection();

        //-------------------------------------------------------------------------
        // Update View Range
        //-------------------------------------------------------------------------
        // Update the view range, to ensure that we track the playhead, etc...

        if ( m_timelineRect.GetWidth() > 0 )
        {
            UpdateViewRange();
        }

        //-------------------------------------------------------------------------
        // Draw Editor
        //-------------------------------------------------------------------------

        CalculateLayout();
        CreateWindowsAndWidgets();
        DrawUI();

        //-------------------------------------------------------------------------
        // Handle Input
        //-------------------------------------------------------------------------

        HandleUserInput();
        DrawContextMenu();

        //-------------------------------------------------------------------------

        ImGui::PopStyleVar();
        ImGui::PopID();
    }

    void TimelineEditor::HandleUserInput()
    {
        ImVec2 const mousePos = ImGui::GetMousePos();

        // Mouse
        //-------------------------------------------------------------------------

        if ( ImGui::IsWindowHovered( ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_NoPopupHierarchy ) )
        {
            // Context Menu
            //-------------------------------------------------------------------------

            bool const isMouseRightButtonReleased = ImGui::IsMouseReleased( ImGuiMouseButton_Right );
            if ( isMouseRightButtonReleased )
            {
                if ( m_mouseState.m_isHoveredOverTrackEditor )
                {
                    m_contextMenuState.m_pItem = m_mouseState.m_pHoveredItem;
                    m_contextMenuState.m_pTrack = m_mouseState.m_pHoveredTrack;
                    m_contextMenuState.m_playheadTimeForMouse = m_mouseState.m_playheadTimeForMouse;

                    // Shift playhead if we are right clicking on a track
                    if ( m_contextMenuState.m_pItem == nullptr && m_contextMenuState.m_pTrack != nullptr )
                    {
                        SetPlayheadTime( m_contextMenuState.m_playheadTimeForMouse );
                    }

                    m_isContextMenuRequested = true;
                }
            }

            // Handle Selection
            //-------------------------------------------------------------------------

            if ( m_mouseState.m_pHoveredItem != nullptr && m_mouseState.m_hoveredItemMode != ItemEditMode::Move )
            {
                ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeEW );
            }

            bool const isMouseWithinTrackArea = m_trackAreaRect.Contains( mousePos );
            bool const isMouseClicked = ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseClicked( ImGuiMouseButton_Right ) || ImGui::IsMouseClicked( ImGuiMouseButton_Middle );

            // Handle clicks
            if ( isMouseClicked )
            {
                if ( m_mouseState.m_pHoveredItem != nullptr )
                {
                    m_itemEditState.m_pEditedItem = m_mouseState.m_pHoveredItem;
                    m_itemEditState.m_pTrackForEditedItem = m_mouseState.m_pHoveredTrack;
                    m_itemEditState.m_originalTimeRange = m_mouseState.m_pHoveredItem->GetTimeRange();
                    m_itemEditState.m_mode = m_mouseState.m_hoveredItemMode;

                    SetSelection( m_mouseState.m_pHoveredItem );
                }
                else if ( m_mouseState.m_pHoveredTrack )
                {
                    SetSelection( m_mouseState.m_pHoveredTrack );
                }
            }

            // Handle Drag
            //-------------------------------------------------------------------------

            if ( m_timelineRect.Contains( mousePos ) || m_isPlayheadHovered )
            {
                if ( ImGui::IsMouseDragging( ImGuiMouseButton_Left ) )
                {
                    m_isDraggingPlayhead = true;
                }
            }

            // Create events with mouse
            //-------------------------------------------------------------------------

            if ( ImGui::IsMouseReleased( ImGuiMouseButton_Middle ) )
            {
                if ( m_mouseState.m_pHoveredTrack != nullptr )
                {
                    SetSelection( m_mouseState.m_pHoveredTrack );

                    if ( isMouseWithinTrackArea )
                    {
                        for ( Track* pTrack : m_selectedTracks )
                        {
                            if ( pTrack->CanCreateNewItems() )
                            {
                                m_pTimeline->CreateItem( m_context, pTrack, m_playheadTime );
                            }
                        }
                    }
                }
            }

            // Zoom
            //-------------------------------------------------------------------------

            auto const& IO = ImGui::GetIO();
            float const mouseWheelDelta = IO.MouseWheel;
            if ( IO.KeyCtrl && mouseWheelDelta != 0 )
            {
                m_pixelsPerFrame = Math::Max( 1.0f, m_pixelsPerFrame + mouseWheelDelta );
            }
        }

        // Keyboard
        //-------------------------------------------------------------------------

        if ( m_isFocused )
        {
            HandleKeyboardInput();
        }

        // Item Edition
        //-------------------------------------------------------------------------

        if ( m_itemEditState.m_mode != ItemEditMode::None )
        {
            EE_ASSERT( m_itemEditState.m_pEditedItem != nullptr && m_itemEditState.m_pTrackForEditedItem != nullptr );

            if ( ImGui::IsMouseDragging( ImGuiMouseButton_Left ) )
            {
                auto pEditedItem = m_itemEditState.m_pEditedItem;

                if ( !m_itemEditState.m_isEditing )
                {
                    m_context.BeginModification();
                    m_itemEditState.m_isEditing = true;
                }

                // Apply mouse delta to item
                //-------------------------------------------------------------------------

                float const pixelOffset = ImGui::GetMouseDragDelta().x;
                float const timeOffset = pixelOffset / m_pixelsPerFrame;

                FloatRange editedItemTimeRange = pEditedItem->GetTimeRange();

                if ( m_itemEditState.m_mode == ItemEditMode::Move )
                {
                    // Create a new range to clamp the event start time to
                    FloatRange validEventStartRange( 0.0f, m_context.m_timelineLength );
                    if ( pEditedItem->IsDurationItem() )
                    {
                        validEventStartRange.m_end = validEventStartRange.m_end - m_itemEditState.m_originalTimeRange.GetLength();
                    }

                    float newTime = m_itemEditState.m_originalTimeRange.m_begin + timeOffset;
                    if ( m_isFrameSnappingEnabled )
                    {
                        newTime = Math::Round( newTime );
                    }

                    editedItemTimeRange.m_begin = validEventStartRange.GetClampedValue( newTime );
                    editedItemTimeRange.m_end = editedItemTimeRange.m_begin + m_itemEditState.m_originalTimeRange.GetLength();
                    SetPlayheadTime( editedItemTimeRange.m_begin );
                }
                else if ( m_itemEditState.m_mode == ItemEditMode::ResizeLeft )
                {
                    float newTime = m_itemEditState.m_originalTimeRange.m_begin + timeOffset;
                    if ( m_isFrameSnappingEnabled )
                    {
                        newTime = Math::Round( newTime );
                    }

                    editedItemTimeRange.m_begin = Math::Min( m_itemEditState.m_originalTimeRange.m_end - 1, newTime );
                    editedItemTimeRange.m_begin = Math::Max( 0.0f, editedItemTimeRange.m_begin );
                    SetPlayheadTime( editedItemTimeRange.m_begin );
                }
                else if ( m_itemEditState.m_mode == ItemEditMode::ResizeRight )
                {
                    float newTime = m_itemEditState.m_originalTimeRange.m_end + timeOffset;
                    if ( m_isFrameSnappingEnabled )
                    {
                        newTime = Math::Round( newTime );
                    }

                    editedItemTimeRange.m_end = Math::Max( m_itemEditState.m_originalTimeRange.m_begin + 1, newTime );
                    editedItemTimeRange.m_end = Math::Min( m_context.m_timelineLength, editedItemTimeRange.m_end );
                    SetPlayheadTime( editedItemTimeRange.m_end );
                }

                m_pTimeline->UpdateItemTimeRange( m_context, pEditedItem, editedItemTimeRange );
            }
            else if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
            {
                if ( m_itemEditState.m_isEditing )
                {
                    m_context.EndModification();
                }

                m_itemEditState.Reset();
            }
        }

        // Playhead
        //-------------------------------------------------------------------------

        if ( m_isDraggingPlayhead )
        {
            // Any play head manipulation switches mode back to paused
            if ( IsPlaying() )
            {
                SetPlayState( PlayState::Paused );
            }

            // Draw Tooltip
            ImVec2 const tooltipPos( ConvertFramesToPixels( m_playheadTime ) + m_timelineRect.GetTL().x + g_playheadHalfWidth + 2, m_timelineRect.GetTL().y );
            ImGui::SetNextWindowPos( tooltipPos );
            ImGui::SetTooltip( "%.2f", m_playheadTime );

            //-------------------------------------------------------------------------

            // The valid range for the playhead, limit it to the current view range but dont let it leave the actual time range
            FloatRange const playheadValidRange( (float) Math::Max( m_viewRange.m_begin, 0.0f ), (float) Math::Min( m_viewRange.m_end, m_context.m_timelineLength ) );
            float newPlayheadTime = m_viewRange.m_begin + ConvertPixelsToFrames( mousePos.x - m_timelineRect.Min.x );
            newPlayheadTime = playheadValidRange.GetClampedValue( newPlayheadTime );
            SetPlayheadTime( newPlayheadTime );

            if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
            {
                m_isDraggingPlayhead = false;
            }
        }

        //-------------------------------------------------------------------------

        m_mouseState.Reset();
    }

    void TimelineEditor::HandleKeyboardInput()
    {
        if ( !m_isFocused )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( ImGui::IsKeyReleased( ImGuiKey_Space ) )
        {
            SetPlayState( m_playState == PlayState::Playing ? PlayState::Paused : PlayState::Playing );
        }
        else if ( ImGui::IsKeyReleased( ImGuiKey_LeftArrow ) )
        {
            FloatRange const playheadValidRange( (float) Math::Max( m_viewRange.m_begin, 0.0f ), (float) Math::Min( m_viewRange.m_end, m_context.m_timelineLength ) );
            float const newPlayheadTime = playheadValidRange.GetClampedValue( m_playheadTime - 1 );
            SetPlayheadTime( newPlayheadTime );
        }
        else if ( ImGui::IsKeyReleased( ImGuiKey_RightArrow ) )
        {
            FloatRange const playheadValidRange( (float) Math::Max( m_viewRange.m_begin, 0.0f ), (float) Math::Min( m_viewRange.m_end, m_context.m_timelineLength ) );
            float const newPlayheadTime = playheadValidRange.GetClampedValue( m_playheadTime + 1 );
            SetPlayheadTime( newPlayheadTime );
        }
        else if ( ImGui::IsKeyReleased( ImGuiKey_E ) )
        {
            for ( Track* pTrack : m_selectedTracks )
            {
                if( pTrack->CanCreateNewItems() )
                {
                    m_pTimeline->CreateItem( m_context, pTrack, m_playheadTime );
                }
            }
        }
        else  if ( ImGui::IsKeyReleased( ImGuiKey_Delete ) )
        {
            TVector<TrackItem*> tmpSelectedItems = m_selectedItems;
            ClearSelection();

            m_context.BeginModification();
            for ( auto pItem : tmpSelectedItems )
            {
                m_pTimeline->DeleteItem( m_context, pItem );
            }
            m_context.EndModification();
        }
    }
}