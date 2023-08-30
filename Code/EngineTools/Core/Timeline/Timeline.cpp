#include "Timeline.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Serialization/TypeSerialization.h"

//-------------------------------------------------------------------------

namespace EE::Timeline
{
    //-------------------------------------------------------------------------
    // Track Context
    //-------------------------------------------------------------------------

    void TrackContext::BeginModification() const
    {
        if ( m_beginModificationCallCount == 0 )
        {
            if ( m_beginModification != nullptr )
            {
                m_beginModification();
            }
        }
        m_beginModificationCallCount++;
    }

    void TrackContext::EndModification() const
    {
        EE_ASSERT( m_beginModificationCallCount > 0 );
        m_beginModificationCallCount--;

        if ( m_beginModificationCallCount == 0 )
        {
            if ( m_endModification != nullptr )
            {
                m_endModification();
            }
        }
    }

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
        auto const& style = ImGui::GetStyle();
        ImVec2 const initialCursorPos = ImGui::GetCursorPos();
        float const lineHeight = ImGui::GetTextLineHeight();
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

        float const iconWidth = ImGui::CalcTextSize( statusIcons[(int32_t) status] ).x;
        ImGui::PushStyleColor( ImGuiCol_Text, statusColors[(int32_t) status] );
        ImGui::SameLine( style.WindowPadding.x + availableRegion.x - iconWidth );
        ImGui::Text( statusIcons[(int32_t) status] );
        ImGui::PopStyleColor();
        ImGuiX::TextTooltip( GetStatusMessage().c_str() );

        // Extra Widgets
        //-------------------------------------------------------------------------

        ImGui::SetCursorPos( initialCursorPos );
        ImVec2 const labelSize = ImGui::CalcTextSize( pLabel );
        ImGui::SameLine( style.WindowPadding.x + labelSize.x + style.ItemSpacing.x );
        ImRect const widgetsRect( headerRect.Min + ImVec2( style.WindowPadding.x + labelSize.x + style.ItemSpacing.x, style.WindowPadding.y ), headerRect.Max - ImVec2( style.WindowPadding.x + iconWidth + style.ItemSpacing.x, style.WindowPadding.y ) );
        DrawExtraHeaderWidgets( widgetsRect );
    }

    void Track::CreateItem( TrackContext const& context, float itemStartTime )
    {
        ScopedModification const stm( context );
        auto pCreatedItem = CreateItemInternal( context, itemStartTime );
        EE_ASSERT( pCreatedItem != nullptr );
        PostCreateItem( pCreatedItem );
    };

    bool Track::DeleteItem( TrackContext const& context, TrackItem* pItem )
    {
        auto foundIter = eastl::find( m_items.begin(), m_items.end(), pItem );
        if ( foundIter != m_items.end() )
        {
            ScopedModification const stm( context );
            EE::Delete( *foundIter );
            m_items.erase( foundIter );
            return true;
        }

        return false;
    }

    void Track::GrowItemToFillGap( TrackContext const& context, TrackItem const* pItem )
    {
        ScopedModification const stm( context );

        EE_ASSERT( pItem != nullptr && pItem->IsDurationItem() );
        int32_t const itemIdx = GetItemIndex( pItem );
        EE_ASSERT( itemIdx != InvalidIndex );

        // Last event
        if ( itemIdx == GetNumItems() - 1 )
        {
            FloatRange newTimeRange = m_items[itemIdx]->GetTimeRange();
            newTimeRange.m_end = context.GetTimelineLength();
            m_items[itemIdx]->SetTimeRange( newTimeRange );
        }
        else
        {
            FloatRange newTimeRange = m_items[itemIdx]->GetTimeRange();
            newTimeRange.m_end = m_items[itemIdx + 1]->GetTimeRange().m_begin;
            m_items[itemIdx]->SetTimeRange( newTimeRange );
        }
    }

    void Track::FillGapsForDurationItems( TrackContext const& context )
    {
        ScopedModification const stm( context );

        int32_t const numItems = GetNumItems();
        for ( int32_t i = 0; i < numItems; i++ )
        {
            // Last event
            if ( i == numItems - 1 )
            {
                FloatRange newTimeRange = m_items[i]->GetTimeRange();
                newTimeRange.m_end = context.GetTimelineLength();
                m_items[i]->SetTimeRange( newTimeRange );
            }
            else
            {
                FloatRange newTimeRange = m_items[i]->GetTimeRange();
                newTimeRange.m_end = m_items[i + 1]->GetTimeRange().m_begin;
                m_items[i]->SetTimeRange( newTimeRange );
            }
        }
    }

    int32_t Track::GetItemIndex( TrackItem const* pItem ) const
    {
        EE_ASSERT( pItem != nullptr );

        int32_t const numItems = GetNumItems();
        for ( int32_t i = 0; i < numItems; i++ )
        {
            if ( m_items[i] == pItem )
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    Color Track::GetItemBackgroundColor( ItemState itemState )
    {
        Color baseBackgroundColor = ImGuiX::Style::s_colorGray0;
     
        if ( itemState == ItemState::SelectedAndHovered || itemState == ItemState::Edited )
        {
            baseBackgroundColor.ScaleColor( 1.35f );
        }
        else if ( itemState == ItemState::Selected )
        {
            baseBackgroundColor.ScaleColor( 1.25f );
        }
        else if ( itemState == ItemState::Hovered  )
        {
            baseBackgroundColor.ScaleColor( 1.15f );
        }

        return baseBackgroundColor;
    }

    Color Track::GetItemDisplayColor( TrackItem const* pItem, ItemState itemState )
    {
        Color baseItemColor = GetItemColor( pItem );

        if ( itemState == ItemState::SelectedAndHovered || itemState == ItemState::Edited )
        {
            baseItemColor.ScaleColor( 1.35f );
        }
        else if ( itemState == ItemState::Selected )
        {
            baseItemColor.ScaleColor( 1.25f );
        }
        else if ( itemState == ItemState::Hovered )
        {
            baseItemColor.ScaleColor( 1.15f );
        }

        return baseItemColor;
    }

    ImRect Track::CalculateImmediateItemRect( TrackItem* pItem, Float2 const& itemPos ) const
    {
        EE_ASSERT( pItem != nullptr && Contains( pItem ) );

        constexpr static float const itemMarginY = 4;
        float const itemHalfSize = ( GetTrackHeight() - ( itemMarginY * 2 ) ) / 2;
        ImVec2 const itemTL( itemPos.m_x - itemHalfSize, itemPos.m_y - itemHalfSize );
        ImVec2 const itemBR( itemPos.m_x + itemHalfSize, itemPos.m_y + itemHalfSize );
        return ImRect( itemTL, itemBR );
    }

    ImRect Track::CalculateDurationItemRect( TrackItem* pItem, Float2 const& itemStartPos, float itemWidth ) const
    {
        EE_ASSERT( pItem != nullptr && Contains( pItem ) );

        float const itemHalfSize = GetTrackHeight() / 2;
        ImVec2 const itemTL( itemStartPos.m_x, itemStartPos.m_y - itemHalfSize );
        ImVec2 const itemBR( itemStartPos.m_x + itemWidth, itemStartPos.m_y + itemHalfSize );
        return ImRect( itemTL, itemBR );
    }

    void Track::DrawImmediateItem( TrackContext const& context, ImDrawList* pDrawList, ImRect const& itemRect, ItemState itemState, TrackItem* pItem )
    {
        EE_ASSERT( pItem != nullptr && Contains( pItem ) );

        ImVec2 const itemCenter = itemRect.GetCenter();

        ImVec2 points[4] =
        {
            ImVec2( itemCenter.x, itemRect.GetTL().y ),
            ImVec2( itemRect.GetTL().x, itemCenter.y ),
            ImVec2( itemCenter.x, itemRect.GetBR().y ),
            ImVec2( itemRect.GetBR().x, itemCenter.y ),
        };

        //-------------------------------------------------------------------------

        Color const itemColor = GetItemDisplayColor( pItem, itemState );
        Color const backgroundColor = GetItemBackgroundColor( itemState );
        pDrawList->AddConvexPolyFilled( points, 4, backgroundColor );

        float const itemHalfSize = itemRect.GetHeight() / 2;
        float const internalOffset = itemHalfSize - 5;
        if ( internalOffset > 0 )
        {
            points[0] += ImVec2( 0, internalOffset );
            points[1] += ImVec2( internalOffset, 0 );
            points[2] -= ImVec2( 0, internalOffset );
            points[3] -= ImVec2( internalOffset, 0 );

            pDrawList->AddConvexPolyFilled( points, 4, itemColor );
        }

        InlineString const itemLabel = GetItemLabel( pItem );
        ImFont const* pTinyFont = ImGuiX::GetFont( ImGuiX::Font::Small );
        ImVec2 const textSize = pTinyFont->CalcTextSizeA( pTinyFont->FontSize, FLT_MAX, -1, itemLabel.c_str(), nullptr, NULL );
        ImVec2 const textPos( itemRect.GetTR().x, itemCenter.y - ( textSize.y / 2 ) );
        pDrawList->AddText( pTinyFont, pTinyFont->FontSize, textPos + ImVec2( 5, 1 ), 0xFF000000, itemLabel.c_str() );
        pDrawList->AddText( pTinyFont, pTinyFont->FontSize, textPos + ImVec2( 4, 0 ), ImGuiX::Style::s_colorText, itemLabel.c_str() );
    }

    void Track::DrawDurationItem( TrackContext const& context, ImDrawList* pDrawList, ImRect const& itemRect, ItemState itemState, TrackItem* pItem )
    {
        EE_ASSERT( pItem != nullptr && Contains( pItem ) );

        ImVec2 const horizontalBorder( 1, 0 );
        pDrawList->AddRectFilled( itemRect.GetTL() + horizontalBorder, itemRect.GetBR() - horizontalBorder, GetItemDisplayColor( pItem, itemState ), 2.0f, ImDrawFlags_RoundCornersBottom );
        pDrawList->AddRectFilled( itemRect.GetTL() + horizontalBorder, itemRect.GetBR() - horizontalBorder - ImVec2( 0, 3 ), GetItemBackgroundColor( itemState ), 2.0f, ImDrawFlags_RoundCornersBottom );

        InlineString const itemLabel = GetItemLabel( pItem );
        ImFont const* pTinyFont = ImGuiX::GetFont( ImGuiX::Font::Small );
        ImVec2 const textSize = pTinyFont->CalcTextSizeA( pTinyFont->FontSize, FLT_MAX, -1, itemLabel.c_str(), nullptr, NULL );
        ImVec2 const textPos( itemRect.GetTL().x, itemRect.GetCenter().y - ( textSize.y / 2 ) );
        pDrawList->AddText( pTinyFont, pTinyFont->FontSize, textPos + ImVec2( 5, 1 ), 0xFF000000, itemLabel.c_str() );
        pDrawList->AddText( pTinyFont, pTinyFont->FontSize, textPos + ImVec2( 4, 0 ), ImGuiX::Style::s_colorText, itemLabel.c_str() );
    }

    //-------------------------------------------------------------------------
    // Timeline Data
    //-------------------------------------------------------------------------

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

    Track::Status TimelineData::GetValidationStatus( TrackContext const& context ) const
    {
        for ( auto pTrack : m_tracks )
        {
            Track::Status const status = pTrack->GetValidationStatus( context );
            if ( status != Track::Status::Valid )
            {
                return status;
            }
        }

        return Track::Status::Valid;
    }

    Track* TimelineData::CreateTrack( TrackContext const& context, TypeSystem::TypeInfo const* pTrackTypeInfo, ItemType itemType )
    {
        EE_ASSERT( pTrackTypeInfo->IsDerivedFrom( Track::GetStaticTypeID() ) );

        ScopedModification const stm( context );
        auto pCreatedTrack = Cast<Track>( pTrackTypeInfo->CreateType() );
        pCreatedTrack->m_itemType = itemType;
        m_tracks.emplace_back( pCreatedTrack );

        return pCreatedTrack;
    }

    void TimelineData::DeleteTrack( TrackContext const& context, Track* pTrack )
    {
        EE_ASSERT( Contains( pTrack ) );

        ScopedModification const stm( context );
        m_tracks.erase_first( pTrack );
        EE::Delete( pTrack );
    }

    void TimelineData::CreateItem( TrackContext const& context, Track* pTrack, float itemStartTime )
    {
        EE_ASSERT( pTrack != nullptr );
        EE_ASSERT( Contains( pTrack ) );

        ScopedModification const stm( context );
        pTrack->CreateItem( context, itemStartTime );
    }

    void TimelineData::UpdateItemTimeRange( TrackContext const& context, TrackItem* pItem, FloatRange const& newTimeRange )
    {
        EE_ASSERT( pItem != nullptr );
        EE_ASSERT( Contains( pItem ) );
        EE_ASSERT( newTimeRange.IsSetAndValid() );

        ScopedModification const stm( context );
        pItem->SetTimeRange( newTimeRange );
    }

    void TimelineData::DeleteItem( TrackContext const& context, TrackItem* pItem )
    {
        EE_ASSERT( pItem != nullptr );
        EE_ASSERT( Contains( pItem ) );

        ScopedModification const stm( context );

        for ( auto pTrack : m_tracks )
        {
            if ( pTrack->DeleteItem( context, pItem ) )
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
}