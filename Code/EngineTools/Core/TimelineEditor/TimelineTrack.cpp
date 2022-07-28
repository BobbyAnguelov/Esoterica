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
        m_pData = Serialization::CreateAndReadNativeType( typeRegistry, typeObjectValue[s_itemDataKey] );
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

    void Track::DrawHeader( ImRect const& headerRect )
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
        auto const status = GetStatus();

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
}