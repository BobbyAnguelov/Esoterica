#include "TimelineTrack.h"
#include "System/Imgui/ImguiStyle.h"
#include "System/Imgui/ImguiX.h"
#include "System/Serialization/TypeSerialization.h"

//-------------------------------------------------------------------------

namespace EE::Timeline
{
    TrackItem::TrackItem( FloatRange const& inRange, IRegisteredType* pData )
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

    void Track::DrawHeader( ImRect const& headerRect, FloatRange const& timelineRange )
    {
        ImVec2 const initialCursorPos = ImGui::GetCursorPos();
        float const lineHeight = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 const availableRegion = ImGui::GetContentRegionAvail();
        ImVec2 const itemSpacing = ImGui::GetStyle().ItemSpacing;

        // Set initial Cursor Pos
        ImGui::SetCursorPos( ImVec2( initialCursorPos.x + itemSpacing.x, initialCursorPos.y + ( ( availableRegion.y - lineHeight ) / 2 ) ) );

        // Label
        //-------------------------------------------------------------------------

        char const* const pLabel = GetName();
        ImGui::Text( pLabel == nullptr ? "" : pLabel );

        // Validation Icon
        //-------------------------------------------------------------------------

        constexpr static char const* const statusIcons[3] = { EE_ICON_CHECK, EE_ICON_ALERT_RHOMBUS, EE_ICON_ALERT };
        static Color statusColors[3] = { Colors::LimeGreen, Colors::Gold, Colors::Red };
        auto const status = GetValidationStatus( timelineRange.m_end );

        float const iconWidth = ImGui::CalcTextSize( statusIcons[(int32_t) status] ).x + 2;
        ImGui::PushStyleColor( ImGuiCol_Text, statusColors[(int32_t) status].ToUInt32_ABGR() );
        ImGui::SameLine( headerRect.GetWidth() - iconWidth );
        ImGui::Text( statusIcons[(int32_t) status] );
        ImGui::PopStyleColor();
        ImGuiX::TextTooltip( GetStatusMessage().c_str() );

        // Extra Widgets
        //-------------------------------------------------------------------------

        ImGui::SetCursorPos( initialCursorPos );
        auto const labelSize = ImGui::CalcTextSize( pLabel ) + ImVec2( itemSpacing.x, 0 );
        ImGui::SameLine( labelSize.x );

        ImRect const widgetsRect( headerRect.Min + ImVec2( labelSize.x, 0 ), headerRect.Max - ImVec2( iconWidth, 0 ) );
        DrawExtraHeaderWidgets( widgetsRect );
    }

    bool Track::IsDirty() const
    {
        if ( m_isDirty )
        {
            return true;
        }

        for ( auto const pItem : m_items )
        {
            if ( pItem->IsDirty() )
            {
                return true;
            }
        }

        return false;
    }

    void Track::ClearDirtyFlags()
    {
        m_isDirty = false;

        for ( auto pItem : m_items )
        {
            pItem->ClearDirtyFlag();
        }
    }

    void Track::CreateItem( float itemStartTime )
    {
        auto pCreatedItem = CreateItemInternal( itemStartTime );
        EE_ASSERT( pCreatedItem != nullptr );
        PostCreateItem( pCreatedItem );
        m_isDirty = true;
    };

    bool Track::DeleteItem( TrackItem* pItem )
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

    //-------------------------------------------------------------------------

    uint32_t Track::GetItemBackgroundColor( ItemState itemState, bool isHovered )
    {
        Float4 baseItemColor = ImGuiX::Style::s_colorGray0.Value;

        float originalAlpha = baseItemColor.m_w;

        if ( itemState == Track::ItemState::Selected || itemState == Track::ItemState::Edited )
        {
            baseItemColor = baseItemColor * 1.45f;
        }
        else if ( isHovered )
        {
            baseItemColor = baseItemColor * 1.15f;
        }

        baseItemColor.m_w = originalAlpha;

        return (uint32_t) ImColor( baseItemColor );
    }

    ImRect Track::DrawImmediateItem( ImDrawList* pDrawList, TrackItem* pItem, Float2 const& itemPos, ItemState itemState )
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
        pDrawList->AddRectFilled( capTopLeft, capBottomRight, itemColor.ToUInt32_ABGR() );
        pDrawList->AddTriangleFilled( topLeft, topRight, base, GetItemBackgroundColor( itemState, itemRect.Contains( ImGui::GetMousePos() ) ) );

        ImFont const* pTinyFont = ImGuiX::GetFont( ImGuiX::Font::Small );
        InlineString const itemLabel = GetItemLabel( pItem );
        pDrawList->AddText( pTinyFont, pTinyFont->FontSize, topRight + ImVec2( 5, 1 ), 0xFF000000, itemLabel.c_str() );
        pDrawList->AddText( pTinyFont, pTinyFont->FontSize, topRight + ImVec2( 4, 0 ), ImColor( ImGuiX::Style::s_colorText ), itemLabel.c_str() );

        //-------------------------------------------------------------------------

        return itemRect;
    }

    ImRect Track::DrawDurationItem( ImDrawList* pDrawList, TrackItem* pItem, Float2 const& itemStartPos, Float2 const& itemEndPos, ItemState itemState )
    {
        EE_ASSERT( pItem != nullptr && Contains( pItem ) );

        constexpr static float const itemMarginY = 2;
        constexpr static float const textMarginY = 3;

        ImVec2 const adjustedItemStartPos = ImVec2( itemStartPos ) + ImVec2( 0, itemMarginY );
        ImVec2 const adjustedItemEndPos = ImVec2( itemEndPos ) - ImVec2( 0, itemMarginY );
        ImRect const itemRect( adjustedItemStartPos, adjustedItemEndPos );

        //-------------------------------------------------------------------------

        Color const itemColor = GetItemColor( pItem );
        pDrawList->AddRectFilled( adjustedItemStartPos, adjustedItemEndPos, itemColor.ToUInt32_ABGR(), 4.0f, ImDrawFlags_RoundCornersBottom );
        pDrawList->AddRectFilled( adjustedItemStartPos, adjustedItemEndPos - ImVec2( 0, 3 ), GetItemBackgroundColor( itemState, itemRect.Contains( ImGui::GetMousePos() ) ), 4.0f, ImDrawFlags_RoundCornersBottom );

        ImFont const* pTinyFont = ImGuiX::GetFont( ImGuiX::Font::Small );
        InlineString const itemLabel = GetItemLabel( pItem );
        pDrawList->AddText( pTinyFont, pTinyFont->FontSize, adjustedItemStartPos + ImVec2( 5, textMarginY + 1 ), 0xFF000000, itemLabel.c_str() );
        pDrawList->AddText( pTinyFont, pTinyFont->FontSize, adjustedItemStartPos + ImVec2( 4, textMarginY ), ImColor( ImGuiX::Style::s_colorText ), itemLabel.c_str() );

        //-------------------------------------------------------------------------

        return itemRect;
    }
}