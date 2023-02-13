#include "ImguiX.h"
#if EE_DEVELOPMENT_TOOLS

#include "System/Render/RenderTexture.h"
#include "System/Render/RenderViewport.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    //-------------------------------------------------------------------------
    // Conversion
    //-------------------------------------------------------------------------

    ImTextureID ToIm( Render::Texture const& texture )
    {
        return (void*) &texture.GetShaderResourceView();
    }

    ImTextureID ToIm( Render::Texture const* pTexture )
    {
        return (void*) &pTexture->GetShaderResourceView();
    }

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
        ImGui::SetWindowFocus( pWindowName );
    }

    ImVec2 ClampToRect( ImRect const& rect, ImVec2 const& inPoint )
    {
        ImVec2 clampedPos;
        clampedPos.x = Math::Clamp( inPoint.x, rect.Min.x, rect.Max.x );
        clampedPos.y = Math::Clamp( inPoint.y, rect.Min.y, rect.Max.y );
        return clampedPos;
    }

    ImVec2 GetClosestPointOnRectBorder( ImRect const& rect, ImVec2 const& inPoint )
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
        ImColor const separatorColor = ( (int) color == 0 ) ? ImColor( ImGui::GetStyleColorVec4( ImGuiCol_Separator ) ) : ImColor( color );

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
    // Basic Widgets
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

    void TextTooltip( const char* fmt, ... )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );
        if ( ImGui::IsItemHovered() )
        {
            va_list args;
            va_start( args, fmt );
            ImGui::SetTooltipV( fmt, args );
            va_end( args );
        }
        ImGui::PopStyleVar();
    }

    bool IconButton( char const* pIcon, char const* pLabel, ImColor const& iconColor, ImVec2 const& size_arg )
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

        pWindow->DrawList->AddText( pos + style.FramePadding, iconColor, pIcon );

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

    bool ColoredIconButton( ImColor const& backgroundColor, ImColor const& foregroundColor, ImColor const& iconColor, char const* pIcon, char const* pLabel, ImVec2 const& size )
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

    bool FlatIconButton( char const* pIcon, char const* pLabel, ImColor const& iconColor, ImVec2 const& size )
    {
        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
        bool const result = IconButton( pIcon, pLabel, iconColor, size );
        ImGui::PopStyleColor( 1 );

        return result;
    }

    bool ComboButton( const char* pButtonLabel, char const* comboID, float buttonWidth, TFunction<void()>&& comboCallback )
    {
        InlineString const comboIDStr( InlineString::CtorSprintf(), "##%s", comboID );

        // Calculate button size
        //-------------------------------------------------------------------------

        constexpr float const comboWidth = 26;
        if ( buttonWidth > comboWidth )
        {
            buttonWidth -= comboWidth;
        }
        else if ( buttonWidth > 0 )
        {
            buttonWidth = 1;
        }
        else if ( buttonWidth < 0 )
        {
            buttonWidth = ImGui::GetContentRegionAvail().x - comboWidth;
        }

        // Button
        //-------------------------------------------------------------------------

        ImVec2 const actualButtonSize = ImVec2( buttonWidth, 0 );
        bool const buttonResult = ImGui::Button( pButtonLabel, actualButtonSize );

        uint32_t color = ImGui::GetColorU32( ImGuiCol_Button );
        if ( ImGui::IsItemHovered() )
        {
            if ( ImGui::IsItemActive() )
            {
                color = ImGui::GetColorU32( ImGuiCol_ButtonActive );
            }
            else
            {
                color = ImGui::GetColorU32( ImGuiCol_ButtonHovered );
            }
        }

        //-------------------------------------------------------------------------

        ImGui::SameLine( 0, 0 );
        ImVec2 const cursorPos = ImGui::GetCursorPos();

        // Combo
        //-------------------------------------------------------------------------

        ImGui::SetNextItemWidth( comboWidth );
        if ( ImGui::BeginCombo( comboIDStr.c_str(), nullptr, ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLargest ) )
        {
            comboCallback();
            ImGui::EndCombo();
        }

        auto pDrawList = ImGui::GetWindowDrawList();
        ImVec2 const fillerMin = ImGui::GetWindowPos() + ImVec2( cursorPos ) - ImVec2( ImGui::GetStyle().FrameRounding, 0 );
        ImVec2 const fillerMax = ImGui::GetWindowPos() + ImVec2( cursorPos ) + ImVec2( ImGui::GetStyle().FrameRounding, ImGui::GetFrameHeight() );
        pDrawList->AddRectFilled( fillerMin, fillerMax, color );

        // Fill Gap
        //-------------------------------------------------------------------------

        return buttonResult;
    }

    bool IconComboButton( char const* pIcon, const char* pButtonLabel, ImColor const& iconColor, char const* comboID, float buttonWidth, TFunction<void()>&& comboCallback )
    {
        InlineString const comboIDStr( InlineString::CtorSprintf(), "##%s", comboID );

        // Calculate button size
        //-------------------------------------------------------------------------

        constexpr float const comboWidth = 26;
        if ( buttonWidth > comboWidth )
        {
            buttonWidth -= comboWidth;
        }
        else if ( buttonWidth > 0 )
        {
            buttonWidth = 1;
        }
        else if ( buttonWidth < 0 )
        {
            buttonWidth = ImGui::GetContentRegionAvail().x - comboWidth;
        }

        // Button
        //-------------------------------------------------------------------------

        ImVec2 const actualButtonSize = ImVec2( buttonWidth, 0 );
        bool const buttonResult = IconButton( pIcon, pButtonLabel, iconColor, actualButtonSize );

        uint32_t color = ImGui::GetColorU32( ImGuiCol_Button );
        if ( ImGui::IsItemHovered() )
        {
            if ( ImGui::IsItemActive() )
            {
                color = ImGui::GetColorU32( ImGuiCol_ButtonActive );
            }
            else
            {
                color = ImGui::GetColorU32( ImGuiCol_ButtonHovered );
            }
        }

        //-------------------------------------------------------------------------

        ImGui::SameLine( 0, 0 );
        ImVec2 const cursorPos = ImGui::GetCursorPos();

        // Combo
        //-------------------------------------------------------------------------

        ImGui::SetNextItemWidth( comboWidth );
        if ( ImGui::BeginCombo( comboIDStr.c_str(), nullptr, ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLargest ) )
        {
            comboCallback();
            ImGui::EndCombo();
        }

        auto pDrawList = ImGui::GetWindowDrawList();
        ImVec2 const fillerMin = ImGui::GetWindowPos() + ImVec2( cursorPos ) - ImVec2( ImGui::GetStyle().FrameRounding, 0 );
        ImVec2 const fillerMax = ImGui::GetWindowPos() + ImVec2( cursorPos ) + ImVec2( ImGui::GetStyle().FrameRounding, ImGui::GetFrameHeight() );
        pDrawList->AddRectFilled( fillerMin, fillerMax, color );

        // Fill Gap
        //-------------------------------------------------------------------------

        return buttonResult;
    }

    bool ToggleButton( char const* pOnLabel, char const* pOffLabel, bool& value, ImVec2 const& size, ImColor  const& onColor, ImColor const& offColor )
    {
        ImGui::PushStyleColor( ImGuiCol_Text, ( value ? onColor : offColor ).Value );
        bool const result = ImGui::Button( value ? pOnLabel : pOffLabel, size );
        ImGui::PopStyleColor();

        //-------------------------------------------------------------------------

        if ( result )
        {
            value = !value;
        }

        return result;
    }

    void DrawArrow( ImDrawList* pDrawList, ImVec2 const& arrowStart, ImVec2 const& arrowEnd, ImColor const& color, float arrowWidth, float arrowHeadWidth )
    {
        EE_ASSERT( pDrawList != nullptr );

        ImVec2 const direction = Vector( arrowEnd - arrowStart ).GetNormalized2();
        ImVec2 const orthogonalDirection( -direction.y, direction.x );

        ImVec2 const triangleSideOffset = orthogonalDirection * arrowHeadWidth;
        ImVec2 const triBase = arrowEnd - ( direction * arrowHeadWidth );
        ImVec2 const tri1 = triBase - triangleSideOffset;
        ImVec2 const tri2 = triBase + triangleSideOffset;

        pDrawList->AddLine( arrowStart, triBase, color, arrowWidth );
        pDrawList->AddTriangleFilled( arrowEnd, tri1, tri2, color );
    }

    bool DrawOverlayIcon( ImVec2 const& iconPos, char icon[4], void* iconID, bool isSelected, ImColor const& selectedColor )
    {
        bool result = false;

        //-------------------------------------------------------------------------

        ImGuiX::ScopedFont scopedFont( ImGuiX::Font::Large );
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

    constexpr static float const g_labelWidth = 14.0f;
    constexpr static float const g_labelHeight = 22.0f;

    static bool BeginElementFrame( char const* pLabel, float labelWidth, ImVec2 const& size, ImColor const& backgroundColor)
    {
        ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 3.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 0.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 4, 3 ) );
        ImGui::PushStyleColor( ImGuiCol_ChildBg, backgroundColor.Value );

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

    static bool DrawVectorElement( char const* pID, char const* pLabel, float const& width, ImColor const& backgroundColor, float* pValue, bool isReadOnly = false )
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
            if ( DrawVectorElement( "##x", "X", inputWidth, ImColors::MediumRed, &value.m_x, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##y", "Y", inputWidth, ImColors::Green, &value.m_y, isReadOnly ) )
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
            if ( DrawVectorElement( "##x", "X", inputWidth, ImColors::MediumRed, &value.m_x, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##y", "Y", inputWidth, ImColors::Green, &value.m_y, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##z", "Z", inputWidth, ImColors::RoyalBlue, &value.m_z, isReadOnly ) )
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
            if ( DrawVectorElement( "##x", "X", inputWidth, ImColors::MediumRed, &value.m_x, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##y", "Y", inputWidth, ImColors::Green, &value.m_y, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##z", "Z", inputWidth, ImColors::RoyalBlue, &value.m_z, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##w", "W", inputWidth, ImColors::DarkOrange, &value.m_w, isReadOnly ) )
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
            if ( DrawVectorElement( "##x", "X", inputWidth, ImColors::MediumRed, &value.m_x, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##y", "Y", inputWidth, ImColors::Green, &value.m_y, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##z", "Z", inputWidth, ImColors::RoyalBlue, &value.m_z, isReadOnly ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawVectorElement( "##w", "W", inputWidth, ImColors::DarkOrange, &value.m_w, isReadOnly ) )
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
                float scale = value.GetScale();
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::InputFloat( "##S", &scale ) )
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
    // Advanced Widgets
    //-------------------------------------------------------------------------

    bool FilterWidget::DrawAndUpdate( TBitFlags<Options> options )
    {
        ImGui::PushID( this );

        if ( options.IsFlagSet( Options::TakeInitialFocus ) )
        {
            if ( ImGui::IsWindowAppearing() )
            {
                ImGui::SetKeyboardFocusHere();
            }
        }

        bool filterUpdated = false;
        ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - 26 - ImGui::GetStyle().ItemSpacing.x );
        if ( ImGui::InputText( "##Filter", m_buffer, s_bufferSize ) )
        {
            filterUpdated = true;

            StringUtils::Split( m_buffer, m_tokens );

            for ( auto& token : m_tokens )
            {
                token.make_lower();
            }
        }

        ImGui::SameLine();
        if ( ImGui::Button( EE_ICON_CLOSE_CIRCLE"##Clear", ImVec2( 26, 24 ) ) )
        {
            Clear();
            filterUpdated = true;
        }

        ImGui::PopID();

        //-------------------------------------------------------------------------

        return filterUpdated;
    }

    void FilterWidget::Clear()
    {
        Memory::MemsetZero( m_buffer, s_bufferSize );
        m_tokens.clear();
    }

    bool FilterWidget::MatchesFilter( String string )
    {
        if ( string.empty() )
        {
            return false;
        }

        if ( m_tokens.empty() )
        {
            return true;
        }

        //-------------------------------------------------------------------------

        string.make_lower();
        for ( auto const& token : m_tokens )
        {
            if ( string.find( token ) == String::npos )
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------

        return true;
    }

    //-------------------------------------------------------------------------

    void OrientationGuide::Draw( Float2 const& guideOrigin, Render::Viewport const& viewport )
    {
        // Project world space axis positions to screen space
        //-------------------------------------------------------------------------

        Vector const& originWS = viewport.GetViewPosition() + viewport.GetViewForwardDirection() * g_worldRenderDistanceZ;
        Vector const& worldAxisX = ( Vector::UnitX );
        Vector const& worldAxisY = ( Vector::UnitY );
        Vector const& worldAxisZ = ( Vector::UnitZ );

        Vector const& worldAxisForwardPointX = ( originWS + worldAxisX );
        Vector const& worldAxisForwardPointY = ( originWS + worldAxisY );
        Vector const& worldAxisForwardPointZ = ( originWS + worldAxisZ );
        Vector const& worldAxisBackwardPointX = ( originWS - worldAxisX );
        Vector const& worldAxisBackwardPointY = ( originWS - worldAxisY );
        Vector const& worldAxisBackwardPointZ = ( originWS - worldAxisZ );

        Vector const axisStartPointX = Vector( viewport.WorldSpaceToScreenSpace( worldAxisBackwardPointX ) );
        Vector const axisStartPointY = Vector( viewport.WorldSpaceToScreenSpace( worldAxisBackwardPointY ) );
        Vector const axisStartPointZ = Vector( viewport.WorldSpaceToScreenSpace( worldAxisBackwardPointZ ) );
        Vector const axisEndPointX = Vector( viewport.WorldSpaceToScreenSpace( worldAxisForwardPointX ) );
        Vector const axisEndPointY = Vector( viewport.WorldSpaceToScreenSpace( worldAxisForwardPointY ) );
        Vector const axisEndPointZ = Vector( viewport.WorldSpaceToScreenSpace( worldAxisForwardPointZ ) );

        // Calculate screen space axis lengths
        //-------------------------------------------------------------------------

        float const axisLengthX = axisStartPointX.GetDistance2( axisEndPointX );
        float const axisLengthY = axisStartPointY.GetDistance2( axisEndPointY );
        float const axisLengthZ = axisStartPointZ.GetDistance2( axisEndPointZ );
        float const maxAxisLength = Math::Max( axisLengthX, Math::Max( axisLengthY, axisLengthZ ) );

        static float const axisHalfLengthSS = g_axisHalfLength;
        float const axisScaleX = ( axisLengthX / maxAxisLength ) * axisHalfLengthSS;
        float const axisScaleY = ( axisLengthY / maxAxisLength ) * axisHalfLengthSS;
        float const axisScaleZ = ( axisLengthZ / maxAxisLength ) * axisHalfLengthSS;

        // Calculate screen space axis directions
        Vector const origin = viewport.WorldSpaceToScreenSpace( originWS );
        Vector const axisDirX = ( axisEndPointX - origin ).GetNormalized2();
        Vector const axisDirY = ( axisEndPointY - origin ).GetNormalized2();
        Vector const axisDirZ = ( axisEndPointZ - origin ).GetNormalized2();

        // Sort SS axis and draw them in the correct order
        //-------------------------------------------------------------------------

        struct AxisDrawRequest { Axis m_axis; bool m_isInForwardDirection; float m_distance; };
        TInlineVector<AxisDrawRequest, 3> drawRequests;

        Plane const NearPlane = viewport.GetViewVolume().GetViewPlane( Math::ViewVolume::PlaneID::Near );

        drawRequests.push_back( { Axis::X, true, NearPlane.GetAbsoluteDistanceToPoint( worldAxisForwardPointX ) } );
        drawRequests.push_back( { Axis::Y, true, NearPlane.GetAbsoluteDistanceToPoint( worldAxisForwardPointY ) } );
        drawRequests.push_back( { Axis::Z, true, NearPlane.GetAbsoluteDistanceToPoint( worldAxisForwardPointZ ) } );
        drawRequests.push_back( { Axis::X, false, NearPlane.GetAbsoluteDistanceToPoint( worldAxisBackwardPointX ) } );
        drawRequests.push_back( { Axis::Y, false, NearPlane.GetAbsoluteDistanceToPoint( worldAxisBackwardPointY ) } );
        drawRequests.push_back( { Axis::Z, false, NearPlane.GetAbsoluteDistanceToPoint( worldAxisBackwardPointZ ) } );

        eastl::sort( drawRequests.begin(), drawRequests.end(), [] ( AxisDrawRequest const& lhs, AxisDrawRequest const& rhs ) { return lhs.m_distance > rhs.m_distance; } );

        //-------------------------------------------------------------------------

        auto pDrawList = ImGui::GetWindowDrawList();
        for ( auto const& request : drawRequests )
        {
            // X
            if ( request.m_axis == Axis::X && request.m_isInForwardDirection )
            {
                pDrawList->AddLine( guideOrigin, guideOrigin + axisDirX * ( axisScaleX - g_axisHeadRadius + 1.0f ), 0xBB0000FF, g_axisThickness );
                pDrawList->AddCircleFilled( guideOrigin + axisDirX * axisScaleX, g_axisHeadRadius, 0xBB0000FF );
            }
            else if ( request.m_axis == Axis::X && !request.m_isInForwardDirection )
            {
                pDrawList->AddCircleFilled( guideOrigin - axisDirX * axisScaleX, g_axisHeadRadius, 0x660000FF );
            }
            //Y
            else if ( request.m_axis == Axis::Y && request.m_isInForwardDirection )
            {
                pDrawList->AddLine( guideOrigin, guideOrigin + axisDirY * ( axisScaleY - g_axisHeadRadius + 1.0f ), 0xBB00FF00, g_axisThickness );
                pDrawList->AddCircleFilled( guideOrigin + axisDirY * axisScaleY, g_axisHeadRadius, 0xBB00FF00 );
            }
            else if ( request.m_axis == Axis::Y && !request.m_isInForwardDirection )
            {
                pDrawList->AddCircleFilled( guideOrigin - axisDirY * axisScaleY, g_axisHeadRadius, 0x6600FF00 );
            }
            // Z
            else if ( request.m_axis == Axis::Z && request.m_isInForwardDirection )
            {
                pDrawList->AddLine( guideOrigin, guideOrigin + axisDirZ * ( axisScaleZ - g_axisHeadRadius + 1.0f ), 0xBBFF0000, g_axisThickness );
                pDrawList->AddCircleFilled( guideOrigin + axisDirZ * axisScaleZ, g_axisHeadRadius, 0xBBFF0000 );
            }
            else if ( request.m_axis == Axis::Z && !request.m_isInForwardDirection )
            {
                pDrawList->AddCircleFilled( guideOrigin - axisDirZ * axisScaleZ, g_axisHeadRadius, 0x66FF0000 );
            }
        }
    }
}
#endif