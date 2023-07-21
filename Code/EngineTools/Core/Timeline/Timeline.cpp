#include "Timeline.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Serialization/TypeSerialization.h"

//-------------------------------------------------------------------------

namespace EE::Timeline
{
    //-------------------------------------------------------------------------
    // Track Item
    //-------------------------------------------------------------------------

    TrackItem::TrackItem( FloatRange const& inRange, IReflectedType* pData )
        : m_pData( pData )
    {
        EE_ASSERT( m_pData != nullptr && inRange.IsSetAndValid() );
        SetTimeRange( inRange );
    }

    TrackItem::~TrackItem()
    {
        EE::Delete( m_pData );
    }

    void TrackItem::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& typeObjectValue )
    {
        m_pData = Serialization::TryCreateAndReadNativeType( typeRegistry, typeObjectValue[s_itemDataKey] );
        EE_ASSERT( m_pData != nullptr );
    }

    void TrackItem::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const
    {
        writer.Key( s_itemDataKey );
        Serialization::WriteNativeType( typeRegistry, m_pData, writer );
    }

    //-------------------------------------------------------------------------
    // Track
    //-------------------------------------------------------------------------

    Track::~Track()
    {
        for ( auto pItem : m_items )
        {
            EE::Delete( pItem );
        }

        m_items.clear();
    }

    const char* Track::GetTypeName() const
    {
        return "Track";
    }

    void Track::DrawHeader( TrackContext const& context, ImRect const& headerRect )
    {
        ImVec2 const initialCursorPos = ImGui::GetCursorPos();
        float const lineHeight = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 const availableRegion = ImGui::GetContentRegionAvail();

        // Set initial Cursor Pos
        ImGui::SetCursorPos( ImVec2( initialCursorPos.x, initialCursorPos.y + ( ( availableRegion.y - lineHeight ) / 2 ) ) );

        // Label
        //-------------------------------------------------------------------------

        char const* const pLabel = GetName();
        ImGui::Text( pLabel == nullptr ? "" : pLabel );

        // Validation Icon
        //-------------------------------------------------------------------------

        constexpr static char const* const statusIcons[3] = { EE_ICON_CHECK, EE_ICON_ALERT_RHOMBUS, EE_ICON_ALERT };
        static Color statusColors[3] = { Colors::LimeGreen, Colors::Gold, Colors::Red };
        auto const status = GetValidationStatus( context );

        float const iconWidth = ImGui::CalcTextSize( statusIcons[(int32_t) status] ).x + 2;
        ImGui::PushStyleColor( ImGuiCol_Text, statusColors[(int32_t) status] );
        ImGui::SameLine( headerRect.GetWidth() - iconWidth );
        ImGui::Text( statusIcons[(int32_t) status] );
        ImGui::PopStyleColor();
        ImGuiX::TextTooltip( GetStatusMessage().c_str() );

        // Extra Widgets
        //-------------------------------------------------------------------------

        ImGui::SetCursorPos( initialCursorPos );
        auto const labelSize = ImGui::CalcTextSize( pLabel ) + ImVec2( 8, 0 );
        ImGui::SameLine( labelSize.x );

        ImRect const widgetsRect( headerRect.Min + ImVec2( labelSize.x, 0 ), headerRect.Max - ImVec2( iconWidth, 0 ) );
        DrawExtraHeaderWidgets( widgetsRect );
    }

    void Track::CreateItem( TrackContext const& context, float itemStartTime )
    {
        ScopedTimelineModification const stm( context );
        auto pCreatedItem = CreateItemInternal( context, itemStartTime );
        EE_ASSERT( pCreatedItem != nullptr );
        PostCreateItem( pCreatedItem );
    };

    bool Track::DeleteItem( TrackContext const& context, TrackItem* pItem )
    {
        auto foundIter = eastl::find( m_items.begin(), m_items.end(), pItem );
        if ( foundIter != m_items.end() )
        {
            ScopedTimelineModification const stm( context );
            EE::Delete( *foundIter );
            m_items.erase( foundIter );
            return true;
        }

        return false;
    }

    Color Track::GetItemBackgroundColor( ItemState itemState, bool isHovered )
    {
        Color baseItemColor = ImGuiX::Style::s_colorGray0;
     
        if ( itemState == Track::ItemState::Selected || itemState == Track::ItemState::Edited )
        {
            baseItemColor.ScaleColor( 1.45f );
        }
        else if ( isHovered )
        {
            baseItemColor.ScaleColor( 1.15f );
        }

        return baseItemColor;
    }

    ImRect Track::DrawImmediateItem( TrackContext const& context, ImDrawList* pDrawList, TrackItem* pItem, Float2 const& itemPos, ItemState itemState )
    {
        EE_ASSERT( pItem != nullptr && Contains( pItem ) );

        constexpr float itemHalfWidth = 5.0f;
        constexpr static float const itemMarginY = 2;

        float const itemPosTopY = itemPos.m_y + itemMarginY;
        float const itemPosBottomY = itemPosTopY + GetTrackHeight() - itemMarginY;
        ImRect const itemRect( ImVec2( itemPos.m_x - itemHalfWidth, itemPosTopY ), ImVec2( itemPos.m_x + itemHalfWidth, itemPosBottomY ) );

        //-------------------------------------------------------------------------

        ImVec2 const base( itemPos.m_x, itemPosBottomY );
        ImVec2 const topLeft( itemPos.m_x - itemHalfWidth, itemPosTopY + 3 );
        ImVec2 const topRight( itemPos.m_x + itemHalfWidth, itemPosTopY + 3 );

        ImVec2 const capTopLeft( itemPos.m_x - itemHalfWidth, itemPosTopY );
        ImVec2 const capBottomRight = topRight;

        //-------------------------------------------------------------------------

        Color const itemColor = GetItemColor( pItem );
        pDrawList->AddRectFilled( capTopLeft, capBottomRight, itemColor );
        pDrawList->AddTriangleFilled( topLeft, topRight, base, GetItemBackgroundColor( itemState, itemRect.Contains( ImGui::GetMousePos() ) ) );

        ImFont const* pTinyFont = ImGuiX::GetFont( ImGuiX::Font::Small );
        InlineString const itemLabel = GetItemLabel( pItem );
        pDrawList->AddText( pTinyFont, pTinyFont->FontSize, topRight + ImVec2( 5, 1 ), 0xFF000000, itemLabel.c_str() );
        pDrawList->AddText( pTinyFont, pTinyFont->FontSize, topRight + ImVec2( 4, 0 ), ImGuiX::Style::s_colorText, itemLabel.c_str() );

        //-------------------------------------------------------------------------

        return itemRect;
    }

    ImRect Track::DrawDurationItem( TrackContext const& context, ImDrawList* pDrawList, TrackItem* pItem, Float2 const& itemStartPos, Float2 const& itemEndPos, ItemState itemState )
    {
        EE_ASSERT( pItem != nullptr && Contains( pItem ) );

        constexpr static float const itemMarginY = 2;
        constexpr static float const textMarginY = 3;

        ImVec2 const adjustedItemStartPos = ImVec2( itemStartPos ) + ImVec2( 1, itemMarginY );
        ImVec2 const adjustedItemEndPos = ImVec2( itemEndPos ) - ImVec2( 1, itemMarginY );
        ImRect const itemRect( adjustedItemStartPos, adjustedItemEndPos );

        ImVec2 const mousePos = ImGui::GetMousePos();
        bool const isHovered = itemRect.Contains( mousePos );

        if ( isHovered )
        {
            InlineString tooltipText = GetItemTooltip( pItem );
            if ( !tooltipText.empty() )
            {
                ImGui::SetTooltip( tooltipText.c_str() );
            }
        }

        //-------------------------------------------------------------------------

        Color const itemColor = GetItemColor( pItem );
        pDrawList->AddRectFilled( adjustedItemStartPos, adjustedItemEndPos, itemColor, 4.0f, ImDrawFlags_RoundCornersBottom );
        pDrawList->AddRectFilled( adjustedItemStartPos, adjustedItemEndPos - ImVec2( 0, 3 ), GetItemBackgroundColor( itemState, itemRect.Contains( ImGui::GetMousePos() ) ), 4.0f, ImDrawFlags_RoundCornersBottom );

        ImFont const* pTinyFont = ImGuiX::GetFont( ImGuiX::Font::Small );
        InlineString const itemLabel = GetItemLabel( pItem );
        pDrawList->AddText( pTinyFont, pTinyFont->FontSize, adjustedItemStartPos + ImVec2( 5, textMarginY + 1 ), 0xFF000000, itemLabel.c_str() );
        pDrawList->AddText( pTinyFont, pTinyFont->FontSize, adjustedItemStartPos + ImVec2( 4, textMarginY ), ImGuiX::Style::s_colorText, itemLabel.c_str() );

        //-------------------------------------------------------------------------

        return itemRect;
    }

    //-------------------------------------------------------------------------
    // Timeline Data
    //-------------------------------------------------------------------------

    TimelineData::TimelineData( TFunction<void()>& onBeginModification, TFunction<void()>& onEndModification )
        : m_onBeginModification( onBeginModification )
        , m_onEndModification( onEndModification )
    {
        // Set context modification function bindings
        m_context.m_beginModification = [this] () { BeginModification(); };
        m_context.m_endModification = [this] () { EndModification(); };
    }

    TimelineData::~TimelineData()
    {
        Reset();
    }

    void TimelineData::Reset()
    {
        for ( auto pTrack : m_tracks )
        {
            EE::Delete( pTrack );
        }
        m_tracks.clear();
    }

    Track* TimelineData::GetTrackForItem( TrackItem const* pItem )
    {
        for ( auto pTrack : m_tracks )
        {
            if ( pTrack->Contains( pItem ) )
            {
                return pTrack;
            }
        }

        return nullptr;
    }

    Track const* TimelineData::GetTrackForItem( TrackItem const* pItem ) const
    {
        return const_cast<TimelineData*>( this )->GetTrackForItem( pItem );
    }

    bool TimelineData::Contains( Track const* pTrack ) const
    {
        return eastl::find( m_tracks.begin(), m_tracks.end(), pTrack ) != m_tracks.end();
    }

    bool TimelineData::Contains( TrackItem const* pItem ) const
    {
        return GetTrackForItem( pItem ) != nullptr;
    }

    Track::Status TimelineData::GetValidationStatus() const
    {
        for ( auto pTrack : m_tracks )
        {
            Track::Status const status = pTrack->GetValidationStatus( m_context );
            if ( status != Track::Status::Valid )
            {
                return status;
            }
        }

        return Track::Status::Valid;
    }

    Track* TimelineData::CreateTrack( TypeSystem::TypeInfo const* pTrackTypeInfo, ItemType itemType )
    {
        EE_ASSERT( pTrackTypeInfo->IsDerivedFrom( Track::GetStaticTypeID() ) );

        ScopedTimelineModification const stm( m_context );
        auto pCreatedTrack = Cast<Track>( pTrackTypeInfo->CreateType() );
        pCreatedTrack->m_itemType = itemType;
        m_tracks.emplace_back( pCreatedTrack );

        return pCreatedTrack;
    }

    void TimelineData::DeleteTrack( Track* pTrack )
    {
        EE_ASSERT( Contains( pTrack ) );

        ScopedTimelineModification const stm( m_context );
        m_tracks.erase_first( pTrack );
        EE::Delete( pTrack );
    }

    void TimelineData::CreateItem( Track* pTrack, float itemStartTime )
    {
        EE_ASSERT( pTrack != nullptr );
        EE_ASSERT( Contains( pTrack ) );

        ScopedTimelineModification const stm( m_context );
        pTrack->CreateItem( m_context, itemStartTime );
    }

    void TimelineData::UpdateItemTimeRange( TrackItem* pItem, FloatRange const& newTimeRange )
    {
        EE_ASSERT( pItem != nullptr );
        EE_ASSERT( Contains( pItem ) );
        EE_ASSERT( newTimeRange.IsSetAndValid() );

        ScopedTimelineModification const stm( m_context );
        pItem->SetTimeRange( newTimeRange );
    }

    void TimelineData::DeleteItem( TrackItem* pItem )
    {
        EE_ASSERT( pItem != nullptr );
        EE_ASSERT( Contains( pItem ) );

        ScopedTimelineModification const stm( m_context );

        for ( auto pTrack : m_tracks )
        {
            if ( pTrack->DeleteItem( m_context, pItem ) )
            {
                break;
            }
        }
    }

    bool TimelineData::Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& dataObjectValue )
    {
        Reset();

        // Get track data
        //-------------------------------------------------------------------------

        // It's ok to not have any track data - this just means an empty file
        auto timelimeDataIter = dataObjectValue.FindMember( TimelineData::s_timelineDataKey );
        if ( timelimeDataIter == dataObjectValue.MemberEnd() )
        {
            return true;
        }

        // The value needs to be an array
        auto const& trackDataValueObject = timelimeDataIter->value;
        if ( !trackDataValueObject.IsArray() )
        {
            EE_LOG_ERROR( "Tools", "Timeline", "Malformed track data" );
            return false;
        }

        // Deserialize track data
        //-------------------------------------------------------------------------

        bool failed = false;

        for ( auto const& trackObjectValue : trackDataValueObject.GetArray() )
        {
            auto trackDataIter = trackObjectValue.FindMember( "Track" );
            if ( trackDataIter == trackObjectValue.MemberEnd() )
            {
                failed = true;
                break;
            }

            // Create track
            Track* pTrack = Serialization::TryCreateAndReadNativeType<Track>( typeRegistry, trackDataIter->value );
            if ( pTrack == nullptr )
            {
                EE_LOG_WARNING( "Serialization", "Timeline Track Container", "Invalid serialized track type! This track was ignored!" );
                continue;
            }
            m_tracks.emplace_back( pTrack );

            // Custom serialization
            pTrack->SerializeCustom( typeRegistry, trackObjectValue );

            //-------------------------------------------------------------------------

            auto itemArrayIter = trackObjectValue.FindMember( "Items" );
            if ( itemArrayIter == trackObjectValue.MemberEnd() || !itemArrayIter->value.IsArray() )
            {
                failed = true;
                break;
            }

            //-------------------------------------------------------------------------

            // Deserialize items
            for ( auto const& itemObjectValue : itemArrayIter->value.GetArray() )
            {
                auto itemDataIter = itemObjectValue.FindMember( "Item" );
                if ( itemDataIter == itemObjectValue.MemberEnd() )
                {
                    failed = true;
                    break;
                }

                // Create item
                TrackItem* pItem = Serialization::TryCreateAndReadNativeType<TrackItem>( typeRegistry, itemDataIter->value );
                if ( pItem != nullptr )
                {
                    pItem->SerializeCustom( typeRegistry, itemObjectValue );
                    pTrack->m_items.emplace_back( pItem );
                }
                else
                {
                    EE_LOG_WARNING( "Serialization", "Timeline Track Container", "Invalid serialized item type! This item was ignored!" );
                }
            }

            if ( failed )
            {
                break;
            }
        }

        //-------------------------------------------------------------------------

        if ( failed )
        {
            Reset();
            EE_LOG_ERROR( "Tools", "Timeline", "Failed to read track data" );
            return false;
        }

        return true;
    }

    void TimelineData::Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer )
    {
        writer.Key( TimelineData::s_timelineDataKey );
        writer.StartArray();

        //-------------------------------------------------------------------------

        for ( auto pTrack : m_tracks )
        {
            writer.StartObject();

            // Track
            //-------------------------------------------------------------------------

            writer.Key( "Track" );
            Serialization::WriteNativeType( typeRegistry, pTrack, writer );

            pTrack->SerializeCustom( typeRegistry, writer );

            // Items
            //-------------------------------------------------------------------------

            writer.Key( "Items" );
            writer.StartArray();
            {
                for ( auto pItem : pTrack->m_items )
                {
                    writer.StartObject();
                    {
                        writer.Key( "Item" );
                        Serialization::WriteNativeType( typeRegistry, pItem, writer );
                        pItem->SerializeCustom( typeRegistry, writer );
                    }
                    writer.EndObject();
                }
            }
            writer.EndArray();

            //-------------------------------------------------------------------------

            writer.EndObject();
        }

        //-------------------------------------------------------------------------

        writer.EndArray();
    }

    void TimelineData::BeginModification()
    {
        if ( m_beginModificationCallCount == 0 )
        {
            if ( m_onBeginModification != nullptr )
            {
                m_onBeginModification();
            }
        }
        m_beginModificationCallCount++;
    }

    void TimelineData::EndModification()
    {
        EE_ASSERT( m_beginModificationCallCount > 0 );
        m_beginModificationCallCount--;

        if ( m_beginModificationCallCount == 0 )
        {
            if ( m_onEndModification != nullptr )
            {
                m_onEndModification();
                m_isDirty = true;
            }
        }
    }
}