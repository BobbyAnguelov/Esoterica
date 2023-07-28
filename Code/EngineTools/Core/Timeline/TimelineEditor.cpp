#include "TimelineEditor.h"
#include "Engine/UpdateContext.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Timeline
{
    constexpr static float const g_headerHeight = 24;
    constexpr static float const g_trackHeaderWidth = 200;
    constexpr static float const g_timelineMinimumWidthForLargeInterval = 100;
    constexpr static float const g_timelineLabelLeftPadding = 4.0f;
    constexpr static float const g_timelineLargeLineOffset = 4;
    constexpr static float const g_timelineMediumLineOffset = 10;
    constexpr static float const g_timelineSmallLineOffset = 16;
    constexpr static float const g_playheadHalfWidth = 7.0f;
    constexpr static float const g_horizontalScrollbarHeight = 16;
    constexpr static float const g_verticalScrollbarHeight = 16;
    constexpr static float const g_itemHandleWidth = 4;

    static Color const g_headerBackgroundColor( 0xFF3D3837 );
    static Color const g_headerLabelColor( 0xFFBBBBBB );
    static Color const g_timelineLargeLineColor( 0xFF606060 );
    static Color const g_timelineMediumLineColor( 0xFF606060 );
    static Color const g_timelineSmallLineColor( 0xFF333333 );
    static Color const g_timelineRangeEndLineColor( 0x990000FF );
    static Color const g_playheadDefaultColor = 0xFF32CD32;
    static Color const g_playheadHoveredColor = g_playheadDefaultColor.GetScaledColor( 1.20f );
    static Color const g_playheadShadowColor = 0x44000000;
    static Color const g_playheadBorderColor = g_playheadDefaultColor.GetScaledColor( 1.25f );
    static Color const g_trackSeparatorColor( 0xFF808080 );

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

    TimelineEditor::TimelineEditor( TimelineData* pTimelineData )
        : m_pTimeline( pTimelineData )
    {
        EE_ASSERT( pTimelineData != nullptr );
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
        ImVec2 const canvasSize = ImGui::GetContentRegionAvail();
        float const trackAreaWidth = ( canvasSize.x - g_trackHeaderWidth );
        float const maxVisibleUnits = Math::Max( 0.0f, Math::Floor( ( canvasSize.x - g_trackHeaderWidth ) / m_pixelsPerFrame ) );
        EE_ASSERT( maxVisibleUnits >= 0.0f );

        // Adjust visible range based on the canvas size
        if ( m_viewRange.GetLength() != maxVisibleUnits )
        {
            m_viewRange.m_end = m_viewRange.m_begin + maxVisibleUnits;
        }

        // Process any update requests
        //-------------------------------------------------------------------------

        float const timelineLength = m_pTimeline->GetLength();

        switch ( m_viewUpdateMode )
        {
            case ViewUpdateMode::ShowFullTimeRange:
            {
                float const viewRangeLength = timelineLength + 1;
                m_pixelsPerFrame = Math::Max( 1.0f, Math::Floor( trackAreaWidth / viewRangeLength ) );
                m_viewRange = FloatRange( 0.0f, viewRangeLength );
                m_viewUpdateMode = ViewUpdateMode::None;
            }
            break;

            case ViewUpdateMode::GoToStart:
            {
                m_viewRange.m_begin = 0;
                m_viewRange.m_end = maxVisibleUnits;
                m_playheadTime = 0;
                m_viewUpdateMode = ViewUpdateMode::None;
            }
            break;

            case ViewUpdateMode::GoToEnd:
            {
                m_viewRange.m_begin = Math::Max( 0.0f, timelineLength - maxVisibleUnits );
                m_viewRange.m_end = m_viewRange.m_begin + maxVisibleUnits;
                m_playheadTime = timelineLength;
                m_viewUpdateMode = ViewUpdateMode::None;
            }
            break;

            case ViewUpdateMode::TrackPlayhead:
            {
                if ( !m_viewRange.ContainsInclusive( m_playheadTime ) )
                {
                    // If the playhead is in the last visible range
                    if ( m_playheadTime + maxVisibleUnits >= timelineLength )
                    {
                        m_viewRange.m_begin = Math::Round( Math::Max( 0.0f, timelineLength - maxVisibleUnits ) );
                        m_viewRange.m_end = timelineLength;
                    }
                    else
                    {
                        m_viewRange.m_begin = Math::Round( m_playheadTime );
                        m_viewRange.m_end = m_viewRange.m_begin + maxVisibleUnits;
                    }
                }
            }
            break;

            default:
            break;
        }
    }

    void TimelineEditor::SetPlayheadTime( float desiredPlayheadTime )
    {
        float const timelineLength = m_pTimeline->GetLength();
        m_playheadTime = FloatRange( 0.0f, timelineLength ).GetClampedValue( desiredPlayheadTime );

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

    void TimelineEditor::DrawTimelineControls( ImRect const& controlsRect )
    {
        ImGui::PushStyleColor( ImGuiCol_Button, (ImVec4) g_headerBackgroundColor );
        ImGui::PushStyleColor( ImGuiCol_ChildBg, (uint32_t) g_headerBackgroundColor );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 2 ) );
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 0, 0 ) );
        if( ImGui::BeginChild( "TimelineControls", controlsRect.GetSize(), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus ) )
        {
            ImVec2 buttonSize = ImVec2( 20, -1 );
            constexpr static float const buttonSeperation = 2;

            // Playback controls
            //-------------------------------------------------------------------------

            ImGui::SameLine( 0, buttonSeperation );

            if ( ImGui::Button( EE_ICON_REWIND "##GoToStart", buttonSize ) )
            {
                SetViewToStart();
            }

            ImGuiX::ItemTooltip( "Rewind to start" );

            //-------------------------------------------------------------------------

            ImGui::SameLine( 0, buttonSeperation );

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

            ImGui::SameLine( 0, buttonSeperation );

            if ( ImGui::Button( EE_ICON_FAST_FORWARD "##GoToEnd", buttonSize ) )
            {
                SetViewToEnd();
            }

            ImGuiX::ItemTooltip( "Fast-forward to end" );

            // Options
            //-------------------------------------------------------------------------

            ImGuiX::SameLineSeparator( 9 );

            if ( ImGuiX::ColoredButton( Colors::Transparent, m_isFrameSnappingEnabled ? ImGuiX::Style::s_colorText : ImGuiX::Style::s_colorTextDisabled, EE_ICON_CURSOR_DEFAULT_CLICK"##Snap", buttonSize ) )
            {
                m_isFrameSnappingEnabled = !m_isFrameSnappingEnabled;
            }

            ImGuiX::ItemTooltip( m_isFrameSnappingEnabled ? "Disable frame snapping" : "Enable frame snapping" );

            //-------------------------------------------------------------------------

            ImGui::SameLine( 0, buttonSeperation );

            if ( IsLoopingEnabled() )
            {
                if ( ImGui::Button( EE_ICON_INFINITY"##PlayOnce", buttonSize ) )
                {
                    m_isLoopingEnabled = false;
                }

                ImGuiX::ItemTooltip( "Disable looping" );
            }
            else // Playing Once
            {
                if ( ImGui::Button( EE_ICON_NUMERIC_1_CIRCLE"##Loop", buttonSize ) )
                {
                    m_isLoopingEnabled = true;
                }

                ImGuiX::ItemTooltip( "Enable looping" );
            }

            //-------------------------------------------------------------------------

            ImGui::SameLine( 0, buttonSeperation );

            if ( ImGui::Button( EE_ICON_FIT_TO_PAGE_OUTLINE"##ResetView", buttonSize ) )
            {
                ResetViewRange();
            }

            ImGuiX::ItemTooltip( "Reset View" );

            //-------------------------------------------------------------------------
            // Add tracks button
            //-------------------------------------------------------------------------

            ImVec2 const addTracksButtonSize( 26, -1 );

            ImGui::SameLine( 0, 0 );
            float const spacerWidth = ImGui::GetContentRegionAvail().x - addTracksButtonSize.x;
            ImGui::SameLine( 0, spacerWidth );

            ImGui::PushStyleColor( ImGuiCol_Text, Colors::LimeGreen );
            bool const showAddTracksMenu = ImGuiX::FlatButton( EE_ICON_MOVIE_PLUS"##AddTrack", addTracksButtonSize );
            ImGui::PopStyleColor();
            ImGuiX::ItemTooltip( "Add Track" );

            if ( showAddTracksMenu )
            {
                ImGui::OpenPopup( "AddTracksPopup" );
            }

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );
            if ( ImGui::BeginPopup( "AddTracksPopup" ) )
            {
                DrawAddTracksMenu();
                ImGui::EndPopup();
            }
            ImGui::PopStyleVar();
        }
        ImGui::EndChild();
        ImGui::PopStyleColor( 2 );
        ImGui::PopStyleVar( 2 );
    }

    void TimelineEditor::DrawTimeline( ImRect const& timelineRect )
    {
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();

        // Draw timeline
        //-------------------------------------------------------------------------

        float const visibleRangeLength = m_viewRange.GetLength();

        int32_t numFramesForLargeInterval = Math::Min( Math::MakeEven( Math::RoundToInt( visibleRangeLength / 4 ) ), 10 );
        int32_t numFramesForSmallInterval = 1;
        while ( ( numFramesForLargeInterval * m_pixelsPerFrame ) < g_timelineMinimumWidthForLargeInterval )
        {
            numFramesForLargeInterval *= 2;
            numFramesForSmallInterval *= 2;
        };

        int32_t const numFramesForMediumInterval = numFramesForLargeInterval / 2;

        //-------------------------------------------------------------------------

        float const startPosX = timelineRect.GetTL().x;
        float const startPosY = timelineRect.GetTL().y;
        float const endPosX = timelineRect.GetBR().x;
        float const endPosY = timelineRect.GetBR().y;

        for ( int32_t i = 0; i <= visibleRangeLength; i += numFramesForSmallInterval )
        {
            float const lineOffsetX = startPosX + Math::Round( i * m_pixelsPerFrame );
            if ( lineOffsetX < startPosX || lineOffsetX > endPosX )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            bool const isLargeLine = ( ( i % numFramesForLargeInterval ) == 0 ) || ( i == m_viewRange.GetLength() || i == 0 );
            bool const isMediumLine = ( i % numFramesForMediumInterval ) == 0;

            //-------------------------------------------------------------------------

            if ( isLargeLine )
            {
                float lineOffsetY = g_timelineLargeLineOffset;
                pDrawList->AddLine( ImVec2( lineOffsetX, startPosY + lineOffsetY ), ImVec2( lineOffsetX, endPosY ), g_timelineLargeLineColor, 1 );

                // Draw text label
                InlineString label( InlineString::CtorSprintf(), "%.0f", m_viewRange.m_begin + i );
                pDrawList->AddText( ImVec2( lineOffsetX + g_timelineLabelLeftPadding, startPosY ), g_headerLabelColor, label.c_str() );
            }
            else if( isMediumLine )
            {
                float const lineOffsetY = g_timelineMediumLineOffset;
                pDrawList->AddLine( ImVec2( lineOffsetX, startPosY + lineOffsetY ), ImVec2( lineOffsetX, endPosY ), g_timelineMediumLineColor, 1 );
            }
            else // Small lines
            {
                float const lineOffsetY = g_timelineSmallLineOffset;
                pDrawList->AddLine( ImVec2( lineOffsetX, startPosY + lineOffsetY ), ImVec2( lineOffsetX, startPosY + g_headerHeight ), g_timelineLargeLineColor, 1 );
                pDrawList->AddLine( ImVec2( lineOffsetX, startPosY + g_headerHeight ), ImVec2( lineOffsetX, endPosY ), g_timelineSmallLineColor, 1 );
            }
        }

        // Draw End Line
        //-------------------------------------------------------------------------

        if ( m_viewRange.ContainsInclusive( m_pTimeline->GetLength() ) )
        {
            float const lineOffsetX = startPosX + Math::Round( m_pTimeline->GetLength() * m_pixelsPerFrame );
            pDrawList->AddLine( ImVec2( lineOffsetX, startPosY + g_timelineLargeLineOffset ), ImVec2( lineOffsetX, endPosY ), g_timelineRangeEndLineColor, 1 );

            // Draw text label
            InlineString label( InlineString::CtorSprintf(), "%.0f", m_viewRange.m_begin + m_pTimeline->GetLength() );
            pDrawList->AddText( ImVec2( lineOffsetX + g_timelineLabelLeftPadding, startPosY ), g_timelineRangeEndLineColor, label.c_str() );
        }
    }

    void TimelineEditor::DrawPlayhead( ImRect const& timelineRect )
    {
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();

        float timelineStartPosX = timelineRect.GetTL().x;
        float timelineEndPosY = timelineRect.GetTL().y + g_headerHeight;

        //-------------------------------------------------------------------------

        constexpr static float const playHeadVerticalPadding = 3.0f;

        float const playheadStartOffsetX = ConvertFramesToPixels( m_playheadTime - m_viewRange.m_begin );
        float const playheadHeight = g_headerHeight - ( playHeadVerticalPadding * 2 );

        // Bottom center point of playhead
        ImVec2 const playheadPosition( timelineStartPosX + playheadStartOffsetX, timelineEndPosY - playHeadVerticalPadding );

        // Draw playhead
        //-------------------------------------------------------------------------

        if ( playheadStartOffsetX < 0 )
        {
            return;
        }

        ImVec2 const playheadMarkerPosition = playheadPosition + ImVec2( 0.5f, 0.5f );
        ImVec2 points[5] =
        {
            playheadMarkerPosition,
            playheadMarkerPosition + ImVec2{ -g_playheadHalfWidth, -playheadHeight / 2.0f },
            playheadMarkerPosition + ImVec2{ -g_playheadHalfWidth, -playheadHeight },
            playheadMarkerPosition + ImVec2{ g_playheadHalfWidth, -playheadHeight },
            playheadMarkerPosition + ImVec2{ g_playheadHalfWidth, -playheadHeight / 2.0f }
        };

        m_playheadRect = ImRect( playheadPosition - ImVec2{ g_playheadHalfWidth, playheadHeight }, playheadPosition + ImVec2{ g_playheadHalfWidth, 0 } );
        bool const isHovered = ImGui::IsWindowHovered( ImGuiHoveredFlags_ChildWindows ) && m_playheadRect.Contains( ImGui::GetMousePos() );

        Color const playheadColor = isHovered ? g_playheadHoveredColor : g_playheadDefaultColor;
        pDrawList->AddConvexPolyFilled( points, 5, playheadColor );

        // Draw marker lines
        //-------------------------------------------------------------------------

        pDrawList->AddLine( playheadPosition, ImVec2( playheadPosition.x, timelineRect.GetBR().y ), m_playheadTime != m_pTimeline->GetLength() ? playheadColor : g_timelineRangeEndLineColor );
    }

    void TimelineEditor::DrawTracks( ImRect const& fullTrackAreaRect )
    {
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        ImVec2 const mousePos = ImGui::GetMousePos();

        // Are we hovered over the track editor
        m_mouseState.m_isHoveredOverTrackEditor = fullTrackAreaRect.Contains( mousePos );

        //-------------------------------------------------------------------------

        // TODO: Draw vertical scrollbar

        //-------------------------------------------------------------------------

        float trackStartY = fullTrackAreaRect.GetTL().y;
        int32_t const numTracks = m_pTimeline->GetNumTracks();
        for ( int32_t i = 0; i < numTracks; i++ )
        {
            auto pTrack = m_pTimeline->GetTrack( i );

            float const trackEndY = trackStartY + pTrack->GetTrackHeight();

            // Terminate loop as soon as a track is no longer visible
            if ( trackStartY > fullTrackAreaRect.GetBR().y )
            {
                break;
            }

            //-------------------------------------------------------------------------

            ImRect const trackRect( ImVec2( fullTrackAreaRect.GetTL().x, trackStartY ), ImVec2( fullTrackAreaRect.GetBR().x, trackEndY ) );
            ImRect const trackHeaderRect( ImVec2( fullTrackAreaRect.GetTL().x, trackStartY ), ImVec2( fullTrackAreaRect.GetTL().x + g_trackHeaderWidth, trackEndY ) );
            ImRect const trackAreaRect( ImVec2( fullTrackAreaRect.GetTL().x + g_trackHeaderWidth + g_playheadHalfWidth, trackStartY ), ImVec2( fullTrackAreaRect.GetBR().x, trackEndY ) );

            // Are we hovered over this track?
            if ( trackRect.Contains( mousePos ) )
            {
                m_mouseState.m_pHoveredTrack = pTrack;
            }

            // Calculate playhead position for the mouse pos
            if ( fullTrackAreaRect.Contains( mousePos ) )
            {
                FloatRange const playheadValidRange( (float) Math::Max( m_viewRange.m_begin, 0.0f ), (float) Math::Min( m_viewRange.m_end, m_pTimeline->GetLength() ) );
                m_mouseState.m_playheadTimeForMouse = m_viewRange.m_begin + ConvertPixelsToFrames( mousePos.x - trackAreaRect.Min.x );
                m_mouseState.m_playheadTimeForMouse = playheadValidRange.GetClampedValue( m_mouseState.m_playheadTimeForMouse );
                m_mouseState.m_snappedPlayheadTimeForMouse = m_mouseState.m_playheadTimeForMouse;

                if ( m_isFrameSnappingEnabled )
                {
                    m_mouseState.m_snappedPlayheadTimeForMouse = Math::Round( m_mouseState.m_snappedPlayheadTimeForMouse );
                }
            }

            //-------------------------------------------------------------------------

            ImGui::PushID( pTrack );
            {
                // Draw track header
                //-------------------------------------------------------------------------

                int32_t const headerFlags = ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar;
                uint32_t const headerID = ImGui::GetID( pTrack );
                ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
                ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 0 ) );
                if( ImGui::BeginChild( headerID, trackHeaderRect.GetSize(), false, headerFlags) )
                {
                    pTrack->DrawHeader( m_pTimeline->GetTrackContext(), trackHeaderRect );
                }
                ImGui::EndChild();
                ImGui::PopStyleVar( 2 );

                // Track Item Area
                //-------------------------------------------------------------------------

                // Draw track highlight
                if ( IsSelected( pTrack ) )
                {
                    pDrawList->AddRectFilled( trackAreaRect.GetTL(), trackAreaRect.GetBR(), ImGuiX::Style::s_colorAccent0.GetAlphaVersion( 0.1f ) );
                }

                // Draw items
                //-------------------------------------------------------------------------

                ImGui::PushClipRect( trackAreaRect.GetTL(), trackAreaRect.GetBR(), false );

                for ( auto const pItem : pTrack->GetItems() )
                {
                    FloatRange const itemTimeRange = pItem->GetTimeRange();
                    if ( !m_viewRange.Overlaps( itemTimeRange ) )
                    {
                        continue;
                    }

                    //-------------------------------------------------------------------------

                    Track::ItemState itemState = Track::ItemState::None;

                    if ( m_itemEditState.m_pEditedItem == pItem )
                    {
                        itemState = Track::ItemState::Edited;
                    }
                    else if ( IsSelected( pItem ) )
                    {
                        itemState = Track::ItemState::Selected;
                    }

                    //-------------------------------------------------------------------------

                    if ( pItem->IsImmediateItem() )
                    {
                        Float2 itemPos;
                        itemPos.m_x = trackAreaRect.GetTL().x + ( itemTimeRange.m_begin - m_viewRange.m_begin ) * m_pixelsPerFrame;
                        itemPos.m_y = trackAreaRect.GetTL().y;

                        ImRect const itemRect = pTrack->DrawImmediateItem( m_pTimeline->GetTrackContext(), pDrawList, pItem, itemPos, itemState );
                        if ( itemRect.Contains( mousePos ) )
                        {
                            m_mouseState.m_pHoveredItem = pItem;
                            m_mouseState.m_hoveredItemMode = ItemEditMode::Move;
                        }
                    }
                    else // Draw Duration Item
                    {
                        Float2 itemStartPos;
                        itemStartPos.m_x = trackAreaRect.GetTL().x + ( itemTimeRange.m_begin - m_viewRange.m_begin ) * m_pixelsPerFrame;
                        itemStartPos.m_y = trackAreaRect.GetTL().y;

                        Float2 itemEndPos;
                        itemEndPos.m_x = trackAreaRect.GetTL().x + ( itemTimeRange.m_end - m_viewRange.m_begin ) * m_pixelsPerFrame;
                        itemEndPos.m_y = trackAreaRect.GetBR().y;

                        ImRect const itemRect = pTrack->DrawDurationItem( m_pTimeline->GetTrackContext(), pDrawList, pItem, itemStartPos, itemEndPos, itemState );
                        if ( itemRect.Contains( mousePos ) )
                        {
                            ImVec2 const visualItemStart = itemRect.GetTL();
                            ImVec2 const visualItemEnd = itemRect.GetBR();
                            bool const isHoveredOverLeftHandle = ImRect( visualItemStart, ImVec2( visualItemStart.x + g_itemHandleWidth, visualItemEnd.y ) ).Contains( mousePos );
                            bool const isHoveredOverRightHandle = ( !isHoveredOverLeftHandle ) ? ImRect( ImVec2( visualItemEnd.x - g_itemHandleWidth, visualItemStart.y ), visualItemEnd ).Contains( mousePos ) : false;

                            m_mouseState.m_pHoveredItem = pItem;

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
                }

                ImGui::PopClipRect();

                // Draw track separator
                //-------------------------------------------------------------------------

                pDrawList->AddLine( ImVec2( fullTrackAreaRect.GetTL().x, trackEndY ), ImVec2( fullTrackAreaRect.GetBR().x, trackEndY ), g_trackSeparatorColor );
                ImGui::Dummy( ImVec2( fullTrackAreaRect.GetWidth(), 1 ) );
                trackStartY = trackEndY + 1;
            }
            ImGui::PopID();
        }
    }

    void TimelineEditor::DrawAddTracksMenu()
    {
        int32_t numAvailableTracks = 0;
        TVector<TypeSystem::TypeInfo const*> const& trackTypes = m_pTimeline->GetAllowedTrackTypes();
        for ( auto pTypeInfo : trackTypes )
        {
            auto const pDefaultTrackInstance = Cast<Track>( pTypeInfo->m_pDefaultInstance );

            bool isAllowedTrackType = true;
            if ( !pDefaultTrackInstance->AllowMultipleTracks() )
            {
                // Check if a track of this type already exists
                for ( auto pTrack : m_pTimeline->GetTracks() )
                {
                    auto pExistingTrack = reinterpret_cast<Track*>( pTrack );
                    if ( pExistingTrack->GetTypeInfo() == pDefaultTrackInstance->GetTypeInfo() )
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
                        m_pTimeline->CreateTrack( pTypeInfo, itemType );
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

                    shouldClearSelection = pTrack->DrawItemContextMenu( m_pTimeline->GetTrackContext(), m_contextMenuState.m_pItem );

                    // Default options
                    //-------------------------------------------------------------------------

                    if ( ImGui::MenuItem( EE_ICON_ARROW_LEFT_RIGHT" Expand event to fill gap" ) )
                    {
                        pTrack->GrowItemToFillGap( m_pTimeline->GetTrackContext(), m_contextMenuState.m_pItem );
                    }

                    if ( ImGui::MenuItem( EE_ICON_DELETE" Delete Item" ) )
                    {
                        shouldClearSelection = true;
                        m_pTimeline->DeleteItem( m_contextMenuState.m_pItem );
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

                        m_pTimeline->CreateItem( m_contextMenuState.m_pTrack, itemStartTime );
                    }

                    // Custom Options
                    //-------------------------------------------------------------------------

                    shouldClearSelection = m_contextMenuState.m_pTrack->DrawContextMenu( m_pTimeline->GetTrackContext(), m_pTimeline->GetTracks(), m_contextMenuState.m_playheadTimeForMouse < 0.0f ? m_playheadTime : m_contextMenuState.m_playheadTimeForMouse );

                    // Default Options
                    //-------------------------------------------------------------------------

                    if ( m_contextMenuState.m_pTrack->GetActualItemType() == Timeline::ItemType::Duration )
                    {
                        if ( ImGui::MenuItem( EE_ICON_ARROW_LEFT_RIGHT" Expand events to fill gaps" ) )
                        {
                            m_contextMenuState.m_pTrack->FillGapsForDurationItems( m_pTimeline->GetTrackContext() );
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

    void TimelineEditor::UpdateAndDraw( Seconds deltaTime )
    {
        ImVec2 const canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 const canvasSize = ImGui::GetContentRegionAvail();

        float const timelineLength = m_pTimeline->GetLength();
        bool const requiresHorizontalScrollBar = ( m_viewRange.m_begin > 0.0f || m_viewRange.m_end < timelineLength );
        float const horizontalScrollBarHeight = requiresHorizontalScrollBar ? g_horizontalScrollbarHeight : 0.0f;

        //-------------------------------------------------------------------------
        // Manage play state and input
        //-------------------------------------------------------------------------

        if ( IsPlaying() )
        {
            float const deltaTimelineUnits = m_pTimeline->ConvertSecondsToTimelineUnit( deltaTime );
            m_playheadTime += deltaTimelineUnits;

            if ( m_playheadTime >= timelineLength )
            {
                if ( m_isLoopingEnabled )
                {
                    m_playheadTime = Math::FModF( m_playheadTime, timelineLength );
                }
                else // Clamp to end
                {
                    m_playheadTime = timelineLength;
                    SetPlayState( PlayState::Paused );
                }

                EE_ASSERT( m_playheadTime >= 0.0f );
            }
        }

        // Remove any invalid pointers from the current selection
        EnsureValidSelection();

        // Update the view range, to ensure that we track the playhead, etc...
        UpdateViewRange();

        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        ImGui::PushStyleColor( ImGuiCol_FrameBg, 0 );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
        if( ImGui::BeginChild( "TLE", canvasSize, false, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus) )
        {
            m_isFocused = ImGui::IsWindowFocused( ImGuiFocusedFlags_RootAndChildWindows );

            //-------------------------------------------------------------------------
            // Header
            //-------------------------------------------------------------------------

            ImDrawList* pDrawList = ImGui::GetWindowDrawList();
            pDrawList->AddRectFilled( canvasPos, ImVec2( canvasPos.x + canvasSize.x, canvasPos.y + g_headerHeight ), g_headerBackgroundColor, 0 );

            ImRect const timelineControlsRect( canvasPos, ImVec2( canvasPos.x + g_trackHeaderWidth, canvasPos.y + g_headerHeight ) );
            DrawTimelineControls( timelineControlsRect );

            m_timelineRect = ImRect( ImVec2( canvasPos.x + g_playheadHalfWidth + g_trackHeaderWidth, canvasPos.y ), ImVec2( canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y - horizontalScrollBarHeight ) );
            DrawTimeline( m_timelineRect );

            //-------------------------------------------------------------------------
            // Tracks
            //-------------------------------------------------------------------------

            ImRect const trackAreaRect( ImVec2( canvasPos.x, canvasPos.y + g_headerHeight ), ImVec2( canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y - horizontalScrollBarHeight ) );
            DrawTracks( trackAreaRect );

            //-------------------------------------------------------------------------
            // Playhead
            //-------------------------------------------------------------------------

            DrawPlayhead( m_timelineRect );

            //-------------------------------------------------------------------------
            // Horizontal Scrollbar
            //-------------------------------------------------------------------------

            ImRect const horizontalScrollBarRect( ImVec2( canvasPos.x + g_trackHeaderWidth, canvasPos.y + canvasSize.y - horizontalScrollBarHeight ), ImVec2( canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y ) );
            int64_t const currentViewSize = Math::RoundToInt( m_viewRange.GetLength() * m_pixelsPerFrame );
            int64_t const totalContentSizeNeeded = Math::RoundToInt( m_pTimeline->GetLength() * m_pixelsPerFrame );
            int64_t scrollbarPosition = Math::RoundToInt( m_viewRange.m_begin * m_pixelsPerFrame );

            ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
            ImGuiID const horizontalScrollBarID = pWindow->GetID( "#TimelineScrollbarY" );
            if ( ImGui::ScrollbarEx( horizontalScrollBarRect, horizontalScrollBarID, ImGuiAxis_X, &scrollbarPosition, currentViewSize, totalContentSizeNeeded, 0 ) )
            {
                float const viewRangeOriginalLength = m_viewRange.GetLength();
                m_viewRange.m_begin = Math::Floor( scrollbarPosition / m_pixelsPerFrame );
                m_viewRange.m_end = m_viewRange.m_begin + viewRangeOriginalLength;
            }

            //-------------------------------------------------------------------------

            HandleUserInput();
            DrawContextMenu();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
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

            bool const isMouseWithinTimeline = m_timelineRect.Contains( mousePos );
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
                else if ( isMouseWithinTimeline || m_playheadRect.Contains( mousePos ) )
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

                    if ( isMouseWithinTimeline )
                    {
                        for ( auto pTrack : m_selectedTracks )
                        {
                            if ( pTrack->CanCreateNewItems() )
                            {
                                m_pTimeline->CreateItem( pTrack, m_playheadTime );
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
                    m_pTimeline->BeginModification();
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
                    FloatRange validEventStartRange( 0.0f, m_pTimeline->GetLength() );
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
                    editedItemTimeRange.m_end = Math::Min( m_pTimeline->GetLength(), editedItemTimeRange.m_end );
                    SetPlayheadTime( editedItemTimeRange.m_end );
                }

                m_pTimeline->UpdateItemTimeRange( pEditedItem, editedItemTimeRange );
            }
            else if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
            {
                if ( m_itemEditState.m_isEditing )
                {
                    m_pTimeline->EndModification();
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
            FloatRange const playheadValidRange( (float) Math::Max( m_viewRange.m_begin, 0.0f ), (float) Math::Min( m_viewRange.m_end, m_pTimeline->GetLength() ) );
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
        if ( ImGui::IsKeyReleased( ImGuiKey_Space ) )
        {
            SetPlayState( m_playState == PlayState::Playing ? PlayState::Paused : PlayState::Playing );
        }
        else if ( ImGui::IsKeyReleased( ImGuiKey_LeftArrow ) )
        {
            FloatRange const playheadValidRange( (float) Math::Max( m_viewRange.m_begin, 0.0f ), (float) Math::Min( m_viewRange.m_end, m_pTimeline->GetLength() ) );
            float const newPlayheadTime = playheadValidRange.GetClampedValue( m_playheadTime - 1 );
            SetPlayheadTime( newPlayheadTime );
        }
        else if ( ImGui::IsKeyReleased( ImGuiKey_RightArrow ) )
        {
            FloatRange const playheadValidRange( (float) Math::Max( m_viewRange.m_begin, 0.0f ), (float) Math::Min( m_viewRange.m_end, m_pTimeline->GetLength() ) );
            float const newPlayheadTime = playheadValidRange.GetClampedValue( m_playheadTime + 1 );
            SetPlayheadTime( newPlayheadTime );
        }
        else if ( ImGui::IsKeyReleased( ImGuiKey_E ) )
        {
            for ( auto pTrack : m_selectedTracks )
            {
                if( pTrack->CanCreateNewItems() )
                {
                    m_pTimeline->CreateItem( pTrack, m_playheadTime );
                }
            }
        }
        else  if ( ImGui::IsKeyReleased( ImGuiKey_Delete ) )
        {
            TVector<TrackItem*> tmpSelectedItems = m_selectedItems;
            ClearSelection();

            m_pTimeline->GetTrackContext().BeginModification();
            for ( auto pItem : tmpSelectedItems )
            {
                m_pTimeline->DeleteItem( pItem );
            }
            m_pTimeline->GetTrackContext().EndModification();
        }
    }

    void TimelineEditor::HandleGlobalKeyboardInputs()
    {
        if ( !m_isFocused )
        {
            HandleKeyboardInput();
        }
    }
}