#include "ImguiX.h"
#include "ImguiStyle.h"

#if EE_DEVELOPMENT_TOOLS

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    //-------------------------------------------------------------------------
    // General helpers
    //-------------------------------------------------------------------------

    void MakeTabVisible( char const* const pWindowName )
    {
        EE_ASSERT( pWindowName != nullptr );
        ImGuiWindow* pWindow = ImGui::FindWindowByName( pWindowName );
        if ( pWindow == nullptr || pWindow->DockNode == nullptr || pWindow->DockNode->TabBar == nullptr )
        {
            return;
        }

        pWindow->DockNode->TabBar->NextSelectedTabId = pWindow->ID;
    }

    ImVec2 const& GetClosestPointOnRect( ImRect const& rect, ImVec2 const& inPoint )
    {
        ImVec2 const points[4] =
        {
            ImLineClosestPoint( rect.GetTL(), rect.GetTR(), inPoint ),
            ImLineClosestPoint( rect.GetBL(), rect.GetBR(), inPoint ),
            ImLineClosestPoint( rect.GetTL(), rect.GetBL(), inPoint ),
            ImLineClosestPoint( rect.GetTR(), rect.GetBR(), inPoint )
        };

        float distancesSq[4] =
        {
            ImLengthSqr( points[0] - inPoint ),
            ImLengthSqr( points[1] - inPoint ),
            ImLengthSqr( points[2] - inPoint ),
            ImLengthSqr( points[3] - inPoint )
        };

        // Get closest point
        float lowestDistance = FLT_MAX;
        int32_t closestPointIdx = -1;
        for ( auto i = 0; i < 4; i++ )
        {
            if ( distancesSq[i] < lowestDistance )
            {
                closestPointIdx = i;
                lowestDistance = distancesSq[i];
            }
        }

        EE_ASSERT( closestPointIdx >= 0 && closestPointIdx < 4 );
        return points[closestPointIdx];
    }

    //-------------------------------------------------------------------------
    // Separators
    //-------------------------------------------------------------------------

    static void CenteredSeparator( float width )
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if ( window->SkipItems )
        {
            return;
        }

        ImGuiContext& g = *GImGui;

        // Horizontal Separator
        float x1, x2;
        if ( window->DC.CurrentColumns == nullptr && ( width == 0 ) )
        {
            // Span whole window
            x1 = window->DC.CursorPos.x;
            x2 = x1 + window->Size.x;
        }
        else
        {
            // Start at the cursor
            x1 = window->DC.CursorPos.x;
            if ( width != 0 )
            {
                x2 = x1 + width;
            }
            else
            {
                x2 = window->ClipRect.Max.x;

                // Pad right side of columns (except the last one)
                if ( window->DC.CurrentColumns && ( window->DC.CurrentColumns->Current < window->DC.CurrentColumns->Count - 1 ) )
                {
                    x2 -= g.Style.ItemSpacing.x;
                }
            }
        }
        float y1 = window->DC.CursorPos.y + int( window->DC.CurrLineSize.y / 2.0f );
        float y2 = y1 + 1.0f;

        window->DC.CursorPos.x += width; //+ g.Style.ItemSpacing.x;
        x1 += window->DC.GroupOffset.x;

        const ImRect bb( ImVec2( x1, y1 ), ImVec2( x2, y2 ) );
        ImGui::ItemSize( ImVec2( 0.0f, 0.0f ) ); // NB: we don't provide our width so that it doesn't get feed back into AutoFit, we don't provide height to not alter layout.
        if ( !ImGui::ItemAdd( bb, NULL ) )
        {
            return;
        }

        window->DrawList->AddLine( bb.Min, ImVec2( bb.Max.x, bb.Min.y ), ImGui::GetColorU32( ImGuiCol_Separator ) );
    }

    void PostSeparator( float width )
    {
        ImGui::SameLine();
        CenteredSeparator( width );
    }

    void PreSeparator( float width )
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if ( window->DC.CurrLineSize.y == 0 )
        {
            window->DC.CurrLineSize.y = ImGui::GetTextLineHeight();
        }
        CenteredSeparator( width );
        ImGui::SameLine();
    }

    void TextSeparator( char const* text, float preWidth, float totalWidth )
    {
        PreSeparator( preWidth );
        ImGui::Text( text );

        // If we have a total width specified, calculate the post separator width
        if ( totalWidth != 0 )
        {
            totalWidth = totalWidth - ( ImGui::CalcTextSize( text ).x + preWidth + ( ImGui::GetStyle().ItemSpacing.x * 2 ) );
        }
        PostSeparator( totalWidth );
    }

    void VerticalSeparator( ImVec2 const& size, ImColor const& color )
    {
        ImColor const separatorColor = ( (int) color == 0 ) ? ImGui::GetStyleColorVec4( ImGuiCol_Separator ) : ImColor( color );

        ImGui::SameLine( 0, 0 );

        auto const availableRegion = ImGui::GetContentRegionAvail();
        ImVec2 seperatorSize( size.x <= 0 ? ( ImGui::GetStyle().ItemSpacing.x * 2 ) + 1 : size.x, size.y <= 0 ? availableRegion.y : size.y);
        if ( Math::IsEven( (int32_t) seperatorSize.x ) )
        {
            seperatorSize.x += 1;
        }

        ImGui::SameLine( 0, seperatorSize.x );
        
        ImVec2 const canvasPos = ImGui::GetCursorScreenPos();
        float const startPosX = canvasPos.x - Math::Floor( seperatorSize.x / 2 ) - 1;
        float const startPosY = canvasPos.y + 1;
        float const endPosY = startPosY + seperatorSize.y - 2;

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        pDrawList->AddLine( ImVec2( startPosX, startPosY ), ImVec2( startPosX, endPosY ), separatorColor, 1 );
    }

    //-------------------------------------------------------------------------
    // Widgets
    //-------------------------------------------------------------------------

    void ItemTooltip( const char* fmt, ... )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );
        if ( ImGui::IsItemHovered() && GImGui->HoveredIdTimer > Style::s_toolTipDelay )
        {
            va_list args;
            va_start( args, fmt );
            ImGui::SetTooltipV( fmt, args );
            va_end( args );
        }
        ImGui::PopStyleVar();
    }

    void ItemTooltipDelayed( float tooltipDelay, const char* fmt, ... )
    {
        EE_ASSERT( tooltipDelay > 0 );
        if ( ImGui::IsItemHovered() && GImGui->HoveredIdTimer > tooltipDelay )
        {
            va_list args;
            va_start( args, fmt );
            ImGui::SetTooltipV( fmt, args );
            va_end( args );
        }
    }

    bool IconButton( char const* pIcon, char const* pLabel, ImVec4 const& iconColor, ImVec2 const& size_arg )
    {
        ImGuiContext& g = *GImGui;
        ImGuiStyle const& style = g.Style;
        ImVec2 const padding = g.Style.FramePadding;

        ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
        if ( pWindow->SkipItems )
        {
            return false;
        }

        ImGuiID const id = pWindow->GetID( pLabel );
        ImVec2 const icon_size = ImGui::CalcTextSize( pIcon, nullptr, true );
        ImVec2 const label_size = ImGui::CalcTextSize( pLabel, nullptr, true );

        ImVec2 pos = pWindow->DC.CursorPos;
        ImVec2 size = ImGui::CalcItemSize( size_arg, icon_size.x + label_size.x + ( style.FramePadding.x * 2.0f ) + ( style.ItemSpacing.x * 2.0f ), Math::Max( icon_size.y, label_size.y ) + style.FramePadding.y * 2.0f );

        ImRect const bb( pos, pos + size );
        ImGui::ItemSize( size, style.FramePadding.y );
        if ( !ImGui::ItemAdd( bb, id ) )
        {
            return false;
        }

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior( bb, id, &hovered, &held, 0 );

        // Render
        ImU32 const col = ImGui::GetColorU32( ( held && hovered ) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button );
        ImGui::RenderNavHighlight( bb, id );
        ImGui::RenderFrame( bb.Min, bb.Max, col, true, style.FrameRounding );
        ImGui::RenderTextClipped( bb.Min + style.FramePadding + ImVec2( icon_size.x + style.ItemSpacing.x, 0 ), bb.Max - style.FramePadding, pLabel, NULL, &label_size, ImVec2( 0, 0.5f ), &bb );

        pWindow->DrawList->AddText( pos + style.FramePadding, ImGui::GetColorU32( iconColor ), pIcon );

        return pressed;
    }

    bool ColoredButton( ImColor const& backgroundColor, ImColor const& foregroundColor, char const* label, ImVec2 const& size )
    {
        ImVec4 const hoveredColor = (ImVec4) AdjustColorBrightness( backgroundColor, 1.15f );
        ImVec4 const activeColor = (ImVec4) AdjustColorBrightness( backgroundColor, 1.25f );

        ImGui::PushStyleColor( ImGuiCol_Button, (ImVec4) backgroundColor );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, hoveredColor );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, activeColor );
        ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) foregroundColor );
        bool const result = ImGui::Button( label, size );
        ImGui::PopStyleColor( 4 );

        return result;
    }

    bool FlatButton( char const* label, ImVec2 const& size )
    {
        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
        bool const result = ImGui::Button( label, size );
        ImGui::PopStyleColor( 1 );

        return result;
    }

    bool ColoredIconButton( ImColor const& backgroundColor, ImColor const& foregroundColor, ImVec4 const& iconColor, char const* pIcon, char const* pLabel, ImVec2 const& size )
    {
        ImVec4 const hoveredColor = (ImVec4) AdjustColorBrightness( backgroundColor, 1.15f );
        ImVec4 const activeColor = (ImVec4) AdjustColorBrightness( backgroundColor, 1.25f );

        ImGui::PushStyleColor( ImGuiCol_Button, (ImVec4) backgroundColor );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, hoveredColor );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, activeColor );
        ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) foregroundColor );
        bool const result = IconButton( pIcon, pLabel, iconColor, size );
        ImGui::PopStyleColor( 4 );

        return result;
    }

    bool FlatIconButton( char const* pIcon, char const* pLabel, ImVec4 const& iconColor, ImVec2 const& size )
    {
        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
        bool const result = IconButton( pIcon, pLabel, iconColor, size );
        ImGui::PopStyleColor( 1 );

        return result;
    }

    void DrawArrow( ImDrawList* pDrawList, ImVec2 const& arrowStart, ImVec2 const& arrowEnd, ImU32 col, float arrowWidth, float arrowHeadWidth )
    {
        EE_ASSERT( pDrawList != nullptr );

        ImVec2 const direction = Vector( arrowEnd - arrowStart ).GetNormalized2();
        ImVec2 const orthogonalDirection( -direction.y, direction.x );

        ImVec2 const triangleSideOffset = orthogonalDirection * arrowHeadWidth;
        ImVec2 const triBase = arrowEnd - ( direction * arrowHeadWidth );
        ImVec2 const tri1 = triBase - triangleSideOffset;
        ImVec2 const tri2 = triBase + triangleSideOffset;

        pDrawList->AddLine( arrowStart, triBase, col, arrowWidth );
        pDrawList->AddTriangleFilled( arrowEnd, tri1, tri2, col );
    }

    bool DrawOverlayIcon( ImVec2 const& iconPos, char icon[4], void* iconID, bool isSelected, ImColor const& selectedColor )
    {
        bool result = false;

        //-------------------------------------------------------------------------

        ImGuiX::ScopedFont scopedFont( ImGuiX::Font::Huge );
        ImVec2 const textSize = ImGui::CalcTextSize( icon );
        ImVec2 const iconSize = textSize + ( ImGui::GetStyle().FramePadding * 2 );
        ImVec2 const iconHalfSize( iconSize.x / 2, iconSize.y / 2 );
        ImRect const iconRect( iconPos - iconHalfSize, iconPos + iconHalfSize );
        ImRect const windowRect( ImVec2( 0, 0 ), ImGui::GetWindowSize() );
        if ( !windowRect.Overlaps( iconRect ) )
        {
            return result;
        }

        //-------------------------------------------------------------------------

        ImVec4 iconColor = ImGuiX::Style::s_colorText;
        if ( isSelected || iconRect.Contains( ImGui::GetMousePos() - ImGui::GetWindowPos() ) )
        {
            iconColor = selectedColor;
        }

        //-------------------------------------------------------------------------

        ImGui::SetCursorPos( iconPos - iconHalfSize );
        ImGui::PushID( iconID );
        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0, 0, 0, 0 ) );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0, 0, 0, 0 ) );
        ImGui::PushStyleColor( ImGuiCol_Text, iconColor );
        if ( ImGui::Button( icon, iconSize ) && !isSelected )
        {
            result = true;
        }
        ImGui::PopStyleColor( 4 );
        ImGui::PopID();

        return result;
    }

    bool DrawSpinner( const char* pLabel, ImColor const& color, float radius, float thickness )
    {
        static float const numSegments = 30.0f;

        //-------------------------------------------------------------------------

        ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
        if ( pWindow->SkipItems )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        ImGuiStyle const& style = ImGui::GetStyle();
        ImGuiID const id = pWindow->GetID( pLabel );

        ImVec2 const pos = pWindow->DC.CursorPos;
        ImVec2 const size( ( radius ) * 2, ( radius + style.FramePadding.y ) * 2 );

        ImRect const bb( pos, ImVec2( pos.x + size.x, pos.y + size.y ) );
        ImGui::ItemSize( bb, style.FramePadding.y );
        if ( !ImGui::ItemAdd( bb, id ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        pWindow->DrawList->PathClear();

        float const time = (float) ImGui::GetTime();
        float const start = Math::Abs( Math::Sin( time * 1.8f ) * ( numSegments - 5 ) );
        float const min = Math::Pi * 2.0f * ( start ) / numSegments;
        float const max = Math::Pi * 2.0f * ( numSegments - 3 ) / numSegments;

        ImVec2 const center = ImVec2( pos.x + radius, pos.y + radius + style.FramePadding.y );

        for ( float i = 0; i < numSegments; i++ )
        {
            float const a = min + ( i / numSegments ) * ( max - min );
            float const b = a + ( time * 8 );
            pWindow->DrawList->PathLineTo( ImVec2( center.x + Math::Cos( b ) * radius, center.y + Math::Sin( b ) * radius ) );
        }

        pWindow->DrawList->PathStroke( color, false, thickness );

        return true;
    }

    //-------------------------------------------------------------------------
    // Numeric Helpers
    //-------------------------------------------------------------------------

    constexpr static float const g_labelWidth = 14.0f;
    constexpr static float const g_labelHeight = 22.0f;

    static bool BeginElementFrame( char const* pLabel, float labelWidth, ImVec2 const& size, ImVec4 backgroundColor)
    {
        ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 3.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 0.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4, 3 ) );
        ImGui::PushStyleColor( ImGuiCol_ChildBg, backgroundColor );

        if ( ImGui::BeginChild( pLabel, size, true, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
        {
            ImGui::AlignTextToFramePadding();
            ImGui::SetCursorPosX( 3 );
            {
                ImGuiX::ScopedFont sf( Font::SmallBold );
                ImGui::Text( pLabel );
            }

            ImGui::SameLine( 0, 0 );
            ImGui::SetCursorPosX( labelWidth );
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 1 );

            return true;
        }

        return false;
    }

    static void EndElementFrame()
    {
        ImGui::EndChild();

        ImGui::PopStyleVar( 4 );
        ImGui::PopStyleColor();
    }

    static bool DrawVectorElement( char const* pID, char const* pLabel, float const& width, ImVec4 backgroundColor, float* pValue, bool isReadOnly = false )
    {
        bool result = false;

        ImGuiX::ScopedFont sf( Font::Small );

        if ( BeginElementFrame( pLabel, g_labelWidth, ImVec2( width, g_labelHeight ), backgroundColor ) )
        {
            ImGui::SetNextItemWidth( width - g_labelWidth - 1 );
            ImGui::InputFloat( pID, pValue, 0, 0, "%.3f", isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0 );
            result = ImGui::IsItemDeactivatedAfterEdit();
        }
        EndElementFrame();

        return result;
    }

    //-------------------------------------------------------------------------

    bool InputFloat2( char const* pID, Float2& value, float width, bool isReadOnly )
    {
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = Math::Floor( ( contentWidth - itemSpacing ) / 2 );

        //-------------------------------------------------------------------------

        bool valueUpdated = false;

        ImGui::PushID( pID );
        {
            if ( DrawVectorElement( "##x", "X", inputWidth, Colors::MediumRed.ToFloat4(), &value.m_x, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##y", "Y", inputWidth, Colors::Green.ToFloat4(), &value.m_y, isReadOnly ) )
            {
                valueUpdated = true;
            }
        }
        ImGui::PopID();

        return valueUpdated;
    }

    bool InputFloat3( char const* pID, Float3& value, float width, bool isReadOnly )
    {
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = Math::Floor( ( contentWidth - ( itemSpacing * 2 ) ) / 3 );

        //-------------------------------------------------------------------------

        bool valueUpdated = false;

        ImGui::PushID( pID );
        {
            if ( DrawVectorElement( "##x", "X", inputWidth, Colors::MediumRed.ToFloat4(), &value.m_x, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##y", "Y", inputWidth, Colors::Green.ToFloat4(), &value.m_y, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##z", "Z", inputWidth, Colors::RoyalBlue.ToFloat4(), &value.m_z, isReadOnly ) )
            {
                valueUpdated = true;
            }
        }
        ImGui::PopID();

        return valueUpdated;
    }

    bool InputFloat4( char const* pID, Float4& value, float width, bool isReadOnly )
    {
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = Math::Floor( ( contentWidth - ( itemSpacing * 3 ) ) / 4 );

        //-------------------------------------------------------------------------

        bool valueUpdated = false;

        ImGui::PushID( pID );
        {
            if ( DrawVectorElement( "##x", "X", inputWidth, Colors::MediumRed.ToFloat4(), &value.m_x, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##y", "Y", inputWidth, Colors::Green.ToFloat4(), &value.m_y, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##z", "Z", inputWidth, Colors::RoyalBlue.ToFloat4(), &value.m_z, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##w", "W", inputWidth, Colors::DarkOrange.ToFloat4(), &value.m_w, isReadOnly ) )
            {
                valueUpdated = true;
            }
        }
        ImGui::PopID();

        return valueUpdated;
    }

    bool InputFloat4( char const* pID, Vector& value, float width, bool isReadOnly )
    {
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = ( contentWidth - ( itemSpacing * 3 ) ) / 4;

        //-------------------------------------------------------------------------

        bool valueUpdated = false;

        ImGui::PushID( pID );
        {
            if ( DrawVectorElement( "##x", "X", inputWidth, Colors::MediumRed.ToFloat4(), &value.m_x, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##y", "Y", inputWidth, Colors::Green.ToFloat4(), &value.m_y, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##z", "Z", inputWidth, Colors::RoyalBlue.ToFloat4(), &value.m_z, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##w", "W", inputWidth, Colors::DarkOrange.ToFloat4(), &value.m_w, isReadOnly ) )
            {
                valueUpdated = true;
            }
        }
        ImGui::PopID();

        return valueUpdated;
    }

    bool InputTransform( char const* pID, Transform& value, float width, bool readOnly )
    {
        bool valueUpdated = false;

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 0, 2 ) );
        if ( ImGui::BeginTable( "Transform", 2, ImGuiTableFlags_None, ImVec2( width, 0 ) ) )
        {
            ImGui::TableSetupColumn( "Header", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 24 );
            ImGui::TableSetupColumn( "Values", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );

            ImGui::TableNextRow();
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Rot" );

                ImGui::TableNextColumn();
                Float3 rotation = value.GetRotation().ToEulerAngles().GetAsDegrees();
                if ( ImGuiX::InputFloat3( "R", rotation ) )
                {
                    value.SetRotation( Quaternion( EulerAngles( rotation ) ) );
                    valueUpdated = true;
                }
            }

            ImGui::TableNextRow();
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Pos" );

                ImGui::TableNextColumn();
                Float3 translation = value.GetTranslation();
                if ( ImGuiX::InputFloat3( "T", translation ) )
                {
                    value.SetTranslation( translation );
                    valueUpdated = true;
                }
            }

            ImGui::TableNextRow();
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Scl" );

                ImGui::TableNextColumn();
                Float3 scale = value.GetScale();
                if ( ImGuiX::InputFloat3( "S", scale ) )
                {
                    value.SetScale( scale );
                    valueUpdated = true;
                }
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        return valueUpdated;
    }

    //-------------------------------------------------------------------------

    void DisplayVector2( ImGuiID ID, Vector const& v, float width )
    {
        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::MediumRed );
            ImGui::Text( "X:" );
            ImGui::SameLine( 0, 3);
        }
        ImGui::Text( "%.3f,", v.GetX() );
        ImGui::SameLine();

        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::Green );
            ImGui::Text( "Y:" );
            ImGui::SameLine();
        }
        ImGui::Text( "%.3f", v.GetY() );
    }

    void DisplayVector3( ImGuiID ID, Vector const& v, float width )
    {
        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::MediumRed );
            ImGui::Text( "X:" );
            ImGui::SameLine( 0, 3 );
        }
        ImGui::Text( "%.3f, ", v.GetX() );
        ImGui::SameLine();

        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::Green );
            ImGui::Text( "Y:" );
            ImGui::SameLine( 0, 3 );
        }
        ImGui::Text( "%.3f, ", v.GetY() );
        ImGui::SameLine();

        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::RoyalBlue );
            ImGui::Text( "Z:" );
            ImGui::SameLine( 0, 3 );
        }
        ImGui::Text( "%.3f", v.GetZ() );
    }

    void DisplayVector4( ImGuiID ID, Vector const& v, float width )
    {
        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::MediumRed );
            ImGui::Text( "X:" );
            ImGui::SameLine();
        }
        ImGui::Text( "%.3f,", v.GetX() );
        ImGui::SameLine();

        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::Green );
            ImGui::Text( "Y:" );
            ImGui::SameLine();
        }
        ImGui::Text( "%.3f,", v.GetY() );
        ImGui::SameLine();

        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::RoyalBlue );
            ImGui::Text( "Z:" );
            ImGui::SameLine();
        }
        ImGui::Text( "%.3f,", v.GetZ() );
        ImGui::SameLine();

        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::Orange );
            ImGui::Text( "W:" );
            ImGui::SameLine();
        }
        ImGui::Text( "%.3f", v.GetW() );
    }

    void DisplayRotation( ImGuiID ID, Quaternion const& q, float width )
    {
        Float3 const rotation = q.ToEulerAngles().GetAsDegrees();
        DisplayVector3( ID, rotation, width );
    }

    void DisplayTransform( ImGuiID ID, Transform const& t, float width )
    {
        Float3 const rotation = t.GetRotation().ToEulerAngles().GetAsDegrees();

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 3, 1 ) );
        ImGui::PushStyleColor( ImGuiCol_TableRowBg, (ImVec4) ImGuiX::Style::s_colorGray8 );
        ImGui::PushStyleColor( ImGuiCol_TableRowBgAlt, (ImVec4) ImGuiX::Style::s_colorGray8 );
        if ( ImGui::BeginTable( "Transform", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_PadOuterX, ImVec2( width, 0 ) ) )
        {
            ImGui::TableSetupColumn( "Label", ImGuiTableColumnFlags_WidthFixed, 45 );
            ImGui::TableSetupColumn( "X", ImGuiTableColumnFlags_WidthStretch, 0.33f );
            ImGui::TableSetupColumn( "Y", ImGuiTableColumnFlags_WidthStretch, 0.33f );
            ImGui::TableSetupColumn( "Z", ImGuiTableColumnFlags_WidthStretch, 0.33f );

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex( 0 );
            {
                ImGuiX::ScopedFont sf( Font::MediumBold );
                ImGui::Text( "Rot =" );
            }

            ImGui::TableSetColumnIndex( 1 );
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::MediumRed );
                ImGui::Text( "X:" );
                ImGui::SameLine( 0, 3 );
            }
            ImGui::Text( "%.3f, ", rotation.m_x );

            ImGui::TableSetColumnIndex( 2 );
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::Green );
                ImGui::Text( "Y:" );
                ImGui::SameLine( 0, 3 );
            }
            ImGui::Text( "%.3f, ", rotation.m_y );

            ImGui::TableSetColumnIndex( 3 );
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::RoyalBlue );
                ImGui::Text( "Z:" );
                ImGui::SameLine( 0, 3 );
            }
            ImGui::Text( "%.3f", rotation.m_z );

            //-------------------------------------------------------------------------

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex( 0 );
            {
                ImGuiX::ScopedFont sf( Font::MediumBold );
                ImGui::Text( "Pos =" );
            }

            ImGui::TableSetColumnIndex( 1 );
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::MediumRed );
                ImGui::Text( "X:" );
                ImGui::SameLine( 0, 3 );
            }
            ImGui::Text( "%.3f, ", t.GetTranslation().m_x);

            ImGui::TableSetColumnIndex( 2 );
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::Green );
                ImGui::Text( "Y:" );
                ImGui::SameLine( 0, 3 );
            }
            ImGui::Text( "%.3f, ", t.GetTranslation().m_y );

            ImGui::TableSetColumnIndex( 3 );
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::RoyalBlue );
                ImGui::Text( "Z:" );
                ImGui::SameLine( 0, 3 );
            }
            ImGui::Text( "%.3f", t.GetTranslation().m_z );

            //-------------------------------------------------------------------------

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex( 0 );
            {
                ImGuiX::ScopedFont sf( Font::MediumBold );
                ImGui::Text( "Scl =" );
            }
        
            ImGui::TableSetColumnIndex( 1 );
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::MediumRed );
                ImGui::Text( "X:" );
                ImGui::SameLine( 0, 3 );
            }
            ImGui::Text( "%.3f, ", t.GetScale().m_x );

            ImGui::TableSetColumnIndex( 2 );
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::Green );
                ImGui::Text( "Y:" );
                ImGui::SameLine( 0, 3 );
            }
            ImGui::Text( "%.3f, ", t.GetScale().m_y );

            ImGui::TableSetColumnIndex( 3 );
            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::Medium, Colors::RoyalBlue );
                ImGui::Text( "Z:" );
                ImGui::SameLine( 0, 3 );
            }
            ImGui::Text( "%.3f", t.GetScale().m_z );

            ImGui::EndTable();
        }
        ImGui::PopStyleColor( 2 );
        ImGui::PopStyleVar();
    }
}
#endif