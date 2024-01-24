#include "ImguiX.h"

#if EE_DEVELOPMENT_TOOLS
#include "Base/Imgui/ImguiXNotifications.h"
#include "Base/Render/RenderTexture.h"
#include "Base/Render/RenderViewport.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    //-------------------------------------------------------------------------
    // General helpers
    //-------------------------------------------------------------------------

    bool BeginViewportPopupModal( char const* pPopupName, bool* pIsPopupOpen, ImVec2 const& size, ImGuiCond windowSizeCond, ImGuiWindowFlags windowFlags )
    {
        ImGui::OpenPopup( pPopupName );
        if ( size.x != 0 || size.y != 0 )
        {
            ImGui::SetNextWindowSize( size, windowSizeCond );
        }
        ImGui::SetNextWindowViewport( ImGui::GetWindowViewport()->ID );
        return ImGui::BeginPopupModal( pPopupName, pIsPopupOpen, windowFlags );
    }

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
    // Layout and Separators
    //-------------------------------------------------------------------------

    bool BeginCollapsibleChildWindow( char const* pLabelAndID, bool initiallyOpen, Color const& backgroundColor )
    {
        ImGui::PushStyleColor( ImGuiCol_ChildBg, backgroundColor );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 8 );
        bool const drawChildWindow = ImGui::BeginChild( pLabelAndID, ImVec2( 0, 0 ), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AlwaysUseWindowPadding, 0 );
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        if ( drawChildWindow )
        {
            ImGui::PushStyleColor( ImGuiCol_Header, backgroundColor );
            ImGui::PushStyleColor( ImGuiCol_HeaderActive, backgroundColor );
            ImGui::PushStyleColor( ImGuiCol_HeaderHovered, backgroundColor );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 2, 4 ) );
            ImGui::SetNextItemOpen( initiallyOpen, ImGuiCond_FirstUseEver );
            ImGuiX::PushFont( Font::MediumBold );
            bool const drawContents = ImGui::CollapsingHeader( pLabelAndID );
            ImGuiX::PopFont();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor( 3 );

            return drawContents;
        }

        return false;
    }

    void EndCollapsibleChildWindow()
    {
        ImGui::EndChild();
    }

    void SameLineSeparator( float width, Color const& color )
    {
        Color const separatorColor = ( color == Colors::Transparent ) ? Color( ImGui::GetStyleColorVec4( ImGuiCol_Separator ) ) : Color( color );
        ImVec2 const seperatorSize( width <= 0 ? ( ImGui::GetStyle().ItemSpacing.x * 2 ) + 1 : width, ImGui::GetFrameHeight() );

        ImGui::SameLine( 0, 0 );

        ImVec2 const canvasPos = ImGui::GetCursorScreenPos();
        float const startPosX = canvasPos.x + Math::Floor( seperatorSize.x / 2 );
        float const startPosY = canvasPos.y + 1;
        float const endPosY = startPosY + seperatorSize.y - 2;

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        pDrawList->AddLine( ImVec2( startPosX, startPosY ), ImVec2( startPosX, endPosY ), separatorColor, 1 );

        ImGui::Dummy( seperatorSize );
        ImGui::SameLine( 0, 0 );
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

    bool Checkbox( char const* pLabel, bool* pValue )
    {
        ImVec2 const newFramePadding( 2, 2 );

        float const originalCursorPosY = ImGui::GetCursorPosY();
        float const offsetY = ImGui::GetStyle().FramePadding.y - newFramePadding.y;
        ImGui::SetCursorPosY( originalCursorPosY + offsetY );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, newFramePadding );
        bool result = ImGui::Checkbox( pLabel, pValue );
        ImGui::PopStyleVar();

        ImGui::SameLine();
        ImGui::SetCursorPosY( originalCursorPosY );
        ImGui::NewLine();

        return result;
    }

    bool IconButton( char const* pIcon, char const* pLabel, Color const& iconColor, ImVec2 const& buttonSize, bool shouldCenterContents )
    {
        if ( pIcon == nullptr )
        {
            return ImGui::Button( pLabel, buttonSize );
        }

        //-------------------------------------------------------------------------

        ImGuiContext& g = *GImGui;

        ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
        if ( pWindow->SkipItems )
        {
            return false;
        }

        ImGuiStyle const& style = ImGui::GetStyle();

        // Calculate sizes
        //-------------------------------------------------------------------------

        ImGuiID const ID = pWindow->GetID( pLabel );
        ImVec2 const iconSize = ImGui::CalcTextSize( pIcon, nullptr, true );
        ImVec2 const labelSize = ImGui::CalcTextSize( pLabel, nullptr, true );

        float totalButtonContentsWidth = labelSize.x + iconSize.x + style.ItemSpacing.x;

        if ( shouldCenterContents )
        {
            if ( labelSize.x > 0 )
            {
                totalButtonContentsWidth = labelSize.x + ( iconSize.x + style.ItemSpacing.x ) * 2;
            }
            else
            {
                totalButtonContentsWidth = iconSize.x;
            }
        }

        float totalButtonWidth = totalButtonContentsWidth + ( style.FramePadding.x * 2.0f );

        float const totalButtonHeight = Math::Max( iconSize.y, labelSize.y ) + ( style.FramePadding.y * 2.0f );

        ImVec2 const pos = pWindow->DC.CursorPos;
        ImVec2 const finalButtonSize = ImGui::CalcItemSize( buttonSize, totalButtonWidth, totalButtonHeight );

        // Add item and handle input
        //-------------------------------------------------------------------------

        ImRect const bb( pos, pos + finalButtonSize );
        ImGui::ItemSize( finalButtonSize, style.FramePadding.y );
        if ( !ImGui::ItemAdd( bb, ID ) )
        {
            return false;
        }

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior( bb, ID, &hovered, &held, 0 );

        // Render Button
        //-------------------------------------------------------------------------

        // Render frame
        ImU32 const color = ImGui::GetColorU32( ( held && hovered ) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button );
        ImGui::RenderNavHighlight( bb, ID );
        ImGui::RenderFrame( bb.Min, bb.Max, color, true, style.FrameRounding );

        bool const isDisabled = g.CurrentItemFlags & ImGuiItemFlags_Disabled;
        ImU32 const finalIconColor = isDisabled ? Style::s_colorTextDisabled : iconColor;

        if ( shouldCenterContents )
        {
            // Icon and Label - ensure label is centered!
            if ( labelSize.x > 0 )
            {
                ImVec2 const textOffset( ( finalButtonSize.x - labelSize.x ) / 2.0f, style.FramePadding.y );
                ImGui::RenderTextClipped( bb.Min + textOffset, bb.Max - style.FramePadding, pLabel, NULL, &labelSize, ImVec2( 0, 0.5f ), &bb );

                ImVec2 const iconOffset( textOffset.x - iconSize.x - style.ItemSpacing.x, style.FramePadding.y );
                pWindow->DrawList->AddText( pos + iconOffset, finalIconColor, pIcon );
            }
            else // Only an icon
            {
                ImVec2 const iconOffset( ( finalButtonSize.x - iconSize.x ) / 2.0f, style.FramePadding.y );
                pWindow->DrawList->AddText( pos + iconOffset, finalIconColor, pIcon );
            }
        }
        else // No centering
        {
            ImVec2 const textOffset( style.FramePadding.x + iconSize.x + style.ItemSpacing.x, style.FramePadding.y );
            ImGui::RenderTextClipped( bb.Min + textOffset, bb.Max - style.FramePadding, pLabel, NULL, &labelSize, ImVec2( 0, 0.5f ), &bb );
            pWindow->DrawList->AddText( pos + style.FramePadding, finalIconColor, pIcon );
        }

        return pressed;
    }

    bool ColoredButton( Color const& backgroundColor, Color const& foregroundColor, char const* label, ImVec2 const& size )
    {
        Color const hoveredColor = backgroundColor.GetScaledColor( 1.15f );
        Color const activeColor = backgroundColor.GetScaledColor( 1.25f );

        ImGui::PushStyleColor( ImGuiCol_Button, backgroundColor.ToFloat4() );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( hoveredColor ) );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( activeColor ) );
        ImGui::PushStyleColor( ImGuiCol_Text, foregroundColor.ToFloat4() );
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

    bool ColoredIconButton( Color const& backgroundColor, Color const& foregroundColor, Color const& iconColor, char const* pIcon, char const* pLabel, ImVec2 const& size, bool shouldCenterContents )
    {
        ImU32 const hoveredColor = backgroundColor.GetScaledColor( 1.15f );
        ImU32 const activeColor = backgroundColor.GetScaledColor( 1.25f );

        ImGui::PushStyleColor( ImGuiCol_Button, (ImVec4) backgroundColor );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, hoveredColor );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, activeColor );
        ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) foregroundColor );
        bool const result = IconButton( pIcon, pLabel, iconColor, size, shouldCenterContents );
        ImGui::PopStyleColor( 4 );

        return result;
    }

    bool FlatIconButton( char const* pIcon, char const* pLabel, Color const& iconColor, ImVec2 const& size, bool shouldCenterContents )
    {
        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
        bool const result = IconButton( pIcon, pLabel, iconColor, size, shouldCenterContents );
        ImGui::PopStyleColor( 1 );

        return result;
    }

    bool IconButtonWithDropDown( char const* comboID, char const* pIcon, const char* pButtonLabel, Color const& iconColor, float buttonWidth, TFunction<void()> const& comboCallback, bool shouldCenterContents )
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
        bool const buttonResult = IconButton( pIcon, pButtonLabel, iconColor, actualButtonSize, shouldCenterContents );

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
            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 8 ) );
            ImGui::PushStyleColor( ImGuiCol_TableBorderStrong, 0 );
            ImGui::PushStyleColor( ImGuiCol_TableBorderLight, 0 );
            bool const drawTable = ImGui::BeginTable( "LayoutTable", 1, ImGuiTableFlags_Borders );
            ImGui::PopStyleVar();
            ImGui::PopStyleColor( 2 );

            if ( drawTable )
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                comboCallback();
                ImGui::EndTable();
            }

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

    bool ToggleButton( char const* pOnLabel, char const* pOffLabel, bool& value, ImVec2 const& size, Color  const& onColor, Color const& offColor )
    {
        ImGui::PushStyleColor( ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4( value ? onColor : offColor ) );
        bool const result = ImGui::Button( value ? pOnLabel : pOffLabel, size );
        ImGui::PopStyleColor();

        //-------------------------------------------------------------------------

        if ( result )
        {
            value = !value;
        }

        return result;
    }

    bool FlatToggleButton( char const* pOnLabel, char const* pOffLabel, bool& value, ImVec2 const& size, Color const& onColor, Color const& offColor )
    {
        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
        bool result = ToggleButton( pOnLabel, pOffLabel, value, size, onColor, offColor );
        ImGui::PopStyleColor( 1 );

        return result;
    }

    void DropDownButton( char const* pLabel, TFunction<void()> const& contextMenuCallback, ImVec2 const& size )
    {
        EE_ASSERT( pLabel != nullptr );

        float const buttonStartPosX = ImGui::GetCursorPosX();
        ImGui::Button( pLabel, size );
        float const buttonEndPosY = ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y;
        ImGui::SetNextWindowPos( ImGui::GetWindowPos() + ImVec2( buttonStartPosX, buttonEndPosY ) );

        InlineString const contextMenuLabel( InlineString::CtorSprintf(), "%s##ContextMenu", pLabel );
        if ( ImGui::BeginPopupContextItem( contextMenuLabel.c_str(), ImGuiPopupFlags_MouseButtonLeft ) )
        {
            contextMenuCallback();
            ImGui::EndPopup();
        }
    }

    void DrawArrow( ImDrawList* pDrawList, ImVec2 const& arrowStart, ImVec2 const& arrowEnd, Color const& color, float arrowWidth, float arrowHeadWidth )
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

    bool DrawOverlayIcon( ImVec2 const& iconPos, char icon[4], void* iconID, bool isSelected, Color const& selectedColor )
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

        ImU32 iconColor = ImGuiX::Style::s_colorText;
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

    bool DrawSpinner( char const* pLabel, Color const& color, ImVec2 size, float thickness, float padding )
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

        // Calculate final size
        //-------------------------------------------------------------------------

        if ( size.x == 0 )
        {
            size.x = ImGui::GetFrameHeight();
        }
        else if ( size.x <= 0 )
        {
            size.x = ImGui::GetContentRegionAvail().x;
        }

        if ( size.y == 0 )
        {
            size.y = ImGui::GetFrameHeight();
        }
        else if ( size.y <= 0 )
        {
            size.y = ImGui::GetContentRegionAvail().y;
        }

        if ( size.x < 0 || size.y < 0 )
        {
            return false;
        }

        // Calculate pos, radius and bounding box
        //-------------------------------------------------------------------------

        ImVec2 const pos = pWindow->DC.CursorPos;
        float const radius = ( ( Math::Min( size.x, size.y ) - thickness ) / 2 ) - padding;
        ImRect const bb( pos, ImVec2( pos.x + size.x, pos.y + size.y ) );

        // Add invisible button
        //-------------------------------------------------------------------------

        bool buttonResult = ImGui::InvisibleButton( pLabel, size );

        // Draw
        //-------------------------------------------------------------------------

        // Debug Only
        {
            //pWindow->DrawList->AddRect( bb.Min, bb.Max, Colors::Pink.ToUInt32() );
        }

        pWindow->DrawList->PathClear();

        float const time = (float) ImGui::GetTime();
        float const start = Math::Abs( Math::Sin( time * 1.8f ) * ( numSegments - 5 ) );
        float const min = Math::Pi * 2.0f * ( start ) / numSegments;
        float const max = Math::Pi * 2.0f * ( numSegments - 3 ) / numSegments;

        ImVec2 const center = ImVec2( pos.x + radius + padding + ( thickness / 2 ), pos.y + radius + padding + ( thickness / 2 ) );

        for ( float i = 0; i < numSegments; i++ )
        {
            float const a = min + ( i / numSegments ) * ( max - min );
            float const b = a + ( time * 8 );
            pWindow->DrawList->PathLineTo( ImVec2( center.x + Math::Cos( b ) * radius, center.y + Math::Sin( b ) * radius ) );
        }

        pWindow->DrawList->PathStroke( color, false, thickness );

        return buttonResult;
    }

    //-------------------------------------------------------------------------
    // Math Widgets
    //-------------------------------------------------------------------------

    constexpr static float const g_labelWidth = 16.0f;

    constexpr static float const g_transformLabelColWidth = 28;

    static bool BeginElementFrame( char const* pLabel, float labelWidth, ImVec2 const& size, Color const& color )
    {
        EE_ASSERT( pLabel != nullptr );

        ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 0.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 0.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );

        ImGui::PushStyleColor( ImGuiCol_ChildBg, ImVec4( 0, 0, 0, 0 ) );
        bool shouldDrawChild = ImGui::BeginChild( pLabel, size, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
        ImGui::PopStyleColor();

        if ( shouldDrawChild )
        {
            ImGui::AlignTextToFramePadding();
            ImGui::SetCursorPosX( 3 );
            {
                if ( pLabel[0] != '#' )
                {
                    ImGuiX::ScopedFont sf( Font::MediumBold, color );
                    ImGui::Text( pLabel );
                }
            }

            ImGui::SameLine( 0, 0 );
            ImGui::SetCursorPosX( labelWidth );
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 1 );
        }

        return shouldDrawChild;
    }

    static void EndElementFrame()
    {
        ImGui::EndChild();
        ImGui::PopStyleVar( 3 );
    }

    static bool DrawFloatEditorElement( char const* pID, char const* pLabel, float const& width, Color const& color, float* pValue )
    {
        bool result = false;

        ImGuiX::ScopedFont sf( Font::Small );

        if ( BeginElementFrame( pLabel, g_labelWidth, ImVec2( width, ImGui::GetFrameHeight() ), color ) )
        {
            ImGui::SetNextItemWidth( width - g_labelWidth - 1 );
            ImGui::InputFloat( pID, pValue, 0, 0, "%.3f" );
            result = ImGui::IsItemDeactivatedAfterEdit();
        }
        EndElementFrame();

        return result;
    }

    static void DrawFloatElement( char const* pLabel, float width, Color const& color, float value )
    {
        ImGuiX::ScopedFont sf( Font::Small );

        if ( BeginElementFrame( pLabel, g_labelWidth, ImVec2( width, ImGui::GetFrameHeight() ), color ) )
        {
            ImGui::SetNextItemWidth( width - g_labelWidth - 1 );
            ImGui::InputFloat( "##v", &value, 0, 0, "%.3f", ImGuiInputTextFlags_ReadOnly );
        }
        EndElementFrame();
    }

    //-------------------------------------------------------------------------

    bool InputFloat2( char const* pID, Float2& value, float width )
    {
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = Math::Floor( ( contentWidth - itemSpacing ) / 2 );

        //-------------------------------------------------------------------------

        bool valueUpdated = false;

        ImGui::PushID( pID );
        {
            if ( DrawFloatEditorElement( "##x", "X", inputWidth, Colors::MediumRed, &value.m_x ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawFloatEditorElement( "##y", "Y", inputWidth, Colors::LimeGreen, &value.m_y ) )
            {
                valueUpdated = true;
            }
        }
        ImGui::PopID();

        return valueUpdated;
    }

    bool InputFloat3( char const* pID, Float3& value, float width )
    {
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = Math::Floor( ( contentWidth - ( itemSpacing * 2 ) ) / 3 );

        //-------------------------------------------------------------------------

        bool valueUpdated = false;

        ImGui::PushID( pID );
        {
            if ( DrawFloatEditorElement( "##x", "X", inputWidth, Colors::MediumRed, &value.m_x ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawFloatEditorElement( "##y", "Y", inputWidth, Colors::LimeGreen, &value.m_y ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawFloatEditorElement( "##z", "Z", inputWidth, Colors::RoyalBlue, &value.m_z ) )
            {
                valueUpdated = true;
            }
        }
        ImGui::PopID();

        return valueUpdated;
    }

    bool InputFloat4( char const* pID, Float4& value, float width )
    {
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = Math::Floor( ( contentWidth - ( itemSpacing * 3 ) ) / 4 );

        //-------------------------------------------------------------------------

        bool valueUpdated = false;

        ImGui::PushID( pID );
        {
            if ( DrawFloatEditorElement( "##x", "X", inputWidth, Colors::MediumRed, &value.m_x ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawFloatEditorElement( "##y", "Y", inputWidth, Colors::LimeGreen, &value.m_y ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawFloatEditorElement( "##z", "Z", inputWidth, Colors::RoyalBlue, &value.m_z ) )
            {
                valueUpdated = true;
            }

            ImGui::SameLine( 0, itemSpacing );
            if ( DrawFloatEditorElement( "##w", "W", inputWidth, Colors::DarkOrange, &value.m_w ) )
            {
                valueUpdated = true;
            }
        }
        ImGui::PopID();

        return valueUpdated;
    }

    bool InputTransform( char const* pID, Transform& value, float width )
    {
        bool valueUpdated = false;

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 0, 2 ) );
        if ( ImGui::BeginTable( "Transform", 2, ImGuiTableFlags_None, ImVec2( width, 0 ) ) )
        {
            ImGui::TableSetupColumn( "Header", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, g_transformLabelColWidth );
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
                if ( DrawFloatEditorElement( "##S", "##S", ImGui::GetContentRegionAvail().x, Colors::HotPink, &scale ) )
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

    void DrawFloat2( Float2 const& value, float width )
    {
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = Math::Floor( ( contentWidth - itemSpacing ) / 2 );

        ImGui::PushID( &value );
        DrawFloatElement( "X", inputWidth, Colors::MediumRed, value.m_x );
        ImGui::SameLine( 0, itemSpacing );
        DrawFloatElement( "Y", inputWidth, Colors::LimeGreen, value.m_y );
        ImGui::PopID();
    }

    void DrawFloat3( Float3 const& value, float width )
    {
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = Math::Floor( ( contentWidth - ( itemSpacing * 2 ) ) / 3 );

        ImGui::PushID( &value );
        DrawFloatElement( "X", inputWidth, Colors::MediumRed, value.m_x );
        ImGui::SameLine( 0, itemSpacing );
        DrawFloatElement( "Y", inputWidth, Colors::LimeGreen, value.m_y );
        ImGui::SameLine( 0, itemSpacing );
        DrawFloatElement( "Z", inputWidth, Colors::RoyalBlue, value.m_z );
        ImGui::PopID();
    }

    void DrawFloat4( Float4 const& value, float width )
    {
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = Math::Floor( ( contentWidth - ( itemSpacing * 3 ) ) / 4 );

        ImGui::PushID( &value );
        DrawFloatElement( "X", inputWidth, Colors::MediumRed, value.m_x );
        ImGui::SameLine( 0, itemSpacing );
        DrawFloatElement( "Y", inputWidth, Colors::LimeGreen, value.m_y );
        ImGui::SameLine( 0, itemSpacing );
        DrawFloatElement( "Z", inputWidth, Colors::RoyalBlue, value.m_z );
        ImGui::SameLine( 0, itemSpacing );
        DrawFloatElement( "W", inputWidth, Colors::DarkOrange, value.m_w );
        ImGui::PopID();
    }

    void DrawTransform( Transform const& value, float width )
    {
        ImGui::PushID( &value );
        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 0, 2 ) );
        if ( ImGui::BeginTable( "Transform", 2, ImGuiTableFlags_None, ImVec2( width, 0 ) ) )
        {
            ImGui::TableSetupColumn( "Header", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, g_transformLabelColWidth );
            ImGui::TableSetupColumn( "Values", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );

            ImGui::TableNextRow();
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Rot" );

                ImGui::TableNextColumn();
                Float3 const rotation = value.GetRotation().ToEulerAngles().GetAsDegrees();
                ImGuiX::DrawFloat3( rotation );
            }

            ImGui::TableNextRow();
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Pos" );

                ImGui::TableNextColumn();
                Float3 const translation = value.GetTranslation();
                ImGuiX::DrawFloat3( translation );
            }

            ImGui::TableNextRow();
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Scl" );

                ImGui::TableNextColumn();
                float scale = value.GetScale();
                ImGuiX::ScopedFont sf( Font::Small );
                DrawFloatElement( "#S", ImGui::GetContentRegionAvail().x, Colors::HotPink, scale );
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        ImGui::PopID();
    }

    //-------------------------------------------------------------------------
    // Advanced Widgets
    //-------------------------------------------------------------------------

    bool FilterWidget::UpdateAndDraw( float width, TBitFlags<Flags> flags )
    {
        bool filterUpdated = false;
        ImGui::PushID( this );

        //-------------------------------------------------------------------------

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
        ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, ImGui::GetStyle().FrameRounding );
        ImGui::PushStyleColor( ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_FrameBg] );
        if ( ImGui::BeginChild( "FilterLayout", ImVec2( width, ImGui::GetFrameHeight() ), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NavFlattened ) )
        {
            if ( flags.IsFlagSet( Flags::TakeInitialFocus ) )
            {
                if ( ImGui::IsWindowAppearing() )
                {
                    ImGui::SetKeyboardFocusHere();
                }
            }

            //-------------------------------------------------------------------------

            ImGui::PushStyleColor( ImGuiCol_FrameBg, 0 );

            ImVec2 const initialCursorPos = ImGui::GetCursorPos();

            // Draw filter input
            float const textInputWidth = ( ( width < 0 ) ? ImGui::GetContentRegionAvail().x : width ) - ( 26 + ImGui::GetStyle().ItemSpacing.x );
            ImGui::SetNextItemWidth( textInputWidth );
            if ( ImGui::InputText( "##Filter", m_buffer, s_bufferSize ) )
            {
                OnBufferUpdated();
                filterUpdated = true;
            }

            // Draw clear button
            bool const isInputFocused = ImGui::IsItemFocused() && ImGui::IsItemActive();
            bool const isbufferEmpty = strlen( m_buffer ) == 0;
            if ( !isbufferEmpty )
            {
                ImGui::SameLine();
                if ( ImGuiX::ColoredButton( Colors::Transparent, ImGuiX::Style::s_colorText, EE_ICON_CLOSE"##Clear", ImVec2( 26, 24 ) ) )
                {
                    Clear();
                    filterUpdated = true;
                }
            }

            // Draw filter text
            if ( isbufferEmpty && !isInputFocused )
            {
                ImGui::SetCursorPos( initialCursorPos + ImVec2( 8, 0 ) );
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored( ImGuiX::Style::s_colorTextDisabled.ToFloat4(), m_filterHelpText.c_str());
            }

            ImGui::PopStyleColor( 1 );
        }
        ImGui::EndChild();
        ImGui::PopStyleVar( 2 );
        ImGui::PopStyleColor();

        //-------------------------------------------------------------------------

        ImGui::PopID();
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

    bool FilterWidget::MatchesFilter( InlineString string )
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
            if ( string.find( token.c_str() ) == InlineString::npos )
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------

        return true;
    }

    void FilterWidget::SetFilter( String const& filterText )
    {
        uint32_t const lengthToCopy = Math::Min( s_bufferSize, (uint32_t) filterText.length() );
        memcpy( m_buffer, filterText.c_str(), lengthToCopy );
        OnBufferUpdated();
    }

    void FilterWidget::OnBufferUpdated()
    {
        StringUtils::Split( String( m_buffer ), m_tokens );

        for ( auto& token : m_tokens )
        {
            token.make_lower();
        }
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