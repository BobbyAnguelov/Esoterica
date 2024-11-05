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

    bool ButtonEx( char const* pIcon, char const* pLabel, ImVec2 const& size, Color const& backgroundColor, Color const& iconColor, Color const& foregroundColor, bool shouldCenterContents )
    {
        bool wasPressed = false;

        ImU32 const hoveredColor = backgroundColor.GetScaledColor( 1.15f );
        ImU32 const activeColor = backgroundColor.GetScaledColor( 1.25f );

        //-------------------------------------------------------------------------

        if ( pIcon == nullptr || strlen( pIcon ) == 0 )
        {
            ImGui::PushStyleColor( ImGuiCol_Button, (ImVec4) backgroundColor );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, hoveredColor );
            ImGui::PushStyleColor( ImGuiCol_ButtonActive, activeColor );
            ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) foregroundColor );
            ImGui::PushStyleVar( ImGuiStyleVar_ButtonTextAlign, shouldCenterContents ? ImVec2( 0.5f, 0.5f ) : ImVec2( 0.0f, 0.5f ) );
            wasPressed = ImGui::Button( pLabel, size );
            ImGui::PopStyleColor( 4 );
            ImGui::PopStyleVar();
        }
        else // Icon button
        {
            ImGuiContext& g = *GImGui;

            ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
            if ( pWindow->SkipItems )
            {
                return false;
            }

            // Calculate ID
            //-------------------------------------------------------------------------

            char const* pID = nullptr;
            if ( pLabel == nullptr || strlen( pLabel ) == 0 )
            {
                pID = pIcon;
            }
            else
            {
                pID = pLabel;
            }

            ImGuiID const ID = pWindow->GetID( pID );

            // Calculate sizes
            //-------------------------------------------------------------------------

            ImGuiStyle const& style = ImGui::GetStyle();
            ImVec2 const iconSize = ImGui::CalcTextSize( pIcon, nullptr, true );
            ImVec2 const labelSize = ImGui::CalcTextSize( pLabel, nullptr, true );
            float const spacing = ( iconSize.x > 0 && labelSize.x > 0 ) ? style.ItemSpacing.x : 0.0f;
            float const buttonWidth = labelSize.x + iconSize.x + spacing;
            float const buttonWidthWithFramePadding = buttonWidth + ( style.FramePadding.x * 2.0f );
            float const textHeightMax = Math::Max( iconSize.y, labelSize.y );
            float const buttonHeight = Math::Max( ImGui::GetFrameHeight(), textHeightMax );

            ImVec2 const pos = pWindow->DC.CursorPos;
            ImVec2 const finalButtonSize = ImGui::CalcItemSize( size, buttonWidthWithFramePadding, buttonHeight );

            // Add item and handle input
            //-------------------------------------------------------------------------

            ImGui::ItemSize( finalButtonSize, 0 );
            ImRect const bb( pos, pos + finalButtonSize );
            if ( !ImGui::ItemAdd( bb, ID ) )
            {
                return false;
            }

            bool hovered, held;
            wasPressed = ImGui::ButtonBehavior( bb, ID, &hovered, &held, 0 );

            // Render Button
            //-------------------------------------------------------------------------

            ImGui::PushStyleColor( ImGuiCol_Button, (ImVec4) backgroundColor );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, hoveredColor );
            ImGui::PushStyleColor( ImGuiCol_ButtonActive, activeColor );
            ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) foregroundColor );

            // Render frame
            ImU32 const color = ImGui::GetColorU32( ( held && hovered ) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button );
            ImGui::RenderNavCursor( bb, ID );
            ImGui::RenderFrame( bb.Min, bb.Max, color, true, style.FrameRounding );

            bool const isDisabled = g.CurrentItemFlags & ImGuiItemFlags_Disabled;
            ImU32 const finalIconColor = isDisabled ? Style::s_colorTextDisabled : iconColor;

            if ( shouldCenterContents )
            {
                // Icon and Label - ensure label is centered!
                if ( labelSize.x > 0 )
                {
                    ImVec2 const textOffset( ( bb.GetWidth() / 2 ) - ( buttonWidthWithFramePadding / 2 ) + iconSize.x + spacing + style.FramePadding.x, 0 );
                    ImGui::RenderTextClipped( bb.Min + textOffset, bb.Max, pLabel, NULL, &labelSize, ImVec2( 0, 0.5f ), &bb );

                    float const offsetX = textOffset.x - iconSize.x - spacing;
                    float const offsetY = ( ( bb.GetHeight() - textHeightMax ) / 2.0f );
                    ImVec2 const iconOffset( offsetX, offsetY );
                    pWindow->DrawList->AddText( pos + iconOffset, finalIconColor, pIcon );
                }
                else // Only an icon
                {
                    float const offsetX = ( bb.GetWidth() - iconSize.x ) / 2.0f;
                    float const offsetY = ( ( bb.GetHeight() - iconSize.y ) / 2.0f );
                    ImVec2 const iconOffset( offsetX, offsetY );
                    pWindow->DrawList->AddText( pos + iconOffset, finalIconColor, pIcon );
                }
            }
            else // No centering
            {
                ImVec2 const textOffset( iconSize.x + spacing + style.FramePadding.x, 0 );
                ImGui::RenderTextClipped( bb.Min + textOffset, bb.Max, pLabel, NULL, &labelSize, ImVec2( 0, 0.5f ), &bb );

                float const iconHeightOffset = ( ( bb.GetHeight() - iconSize.y ) / 2.0f );
                pWindow->DrawList->AddText( pos + ImVec2( style.FramePadding.x, iconHeightOffset ), finalIconColor, pIcon );
            }

            ImGui::PopStyleColor( 4 );
        }

        //-------------------------------------------------------------------------

        return wasPressed;
    }

    //-------------------------------------------------------------------------

    bool ToggleButtonEx( char const* pOnIcon, char const* pOnLabel, char const* pOffIcon, char const* pOffLabel, bool& value, ImVec2 const& size, Color const& onForegroundColor, Color const& onIconColor, Color const& offForegroundColor, Color const& offIconColor )
    {
        bool wasPressed = false;

        if ( value )
        {
            wasPressed = ButtonEx( pOnIcon, pOnLabel, size, ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Button] ), onIconColor, onForegroundColor );
        }
        else
        {
            wasPressed = ButtonEx( pOffIcon, pOffLabel, size, ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Button] ), offIconColor, offForegroundColor );
        }

        //-------------------------------------------------------------------------

        if ( wasPressed )
        {
            value = !value;
        }

        return wasPressed;
    }

    //-------------------------------------------------------------------------

    void DropDownButtonEx( char const* pIcon, char const* pLabel, TFunction<void()> const& contextMenuCallback, ImVec2 const& size, Color const& backgroundColor, Color const& iconColor, Color const& foregroundColor, bool shouldCenterContents )
    {
        EE_ASSERT( pLabel != nullptr );
        EE_ASSERT( contextMenuCallback != nullptr );

        bool const isPressed = ButtonEx( pIcon, pLabel, size, backgroundColor, iconColor, foregroundColor, shouldCenterContents );
        ImRect const buttonRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
        ImVec2 const windowStartPos = buttonRect.GetBL() - ImVec2( 0, ImGui::GetStyle().FrameRounding );

        InlineString const dropDownMenuID( InlineString::CtorSprintf(), "%s##dropdownMenu", pLabel );
        bool isDropDownOpen = ImGui::IsPopupOpen( dropDownMenuID.c_str(), ImGuiPopupFlags_None);
        if ( isPressed && !isDropDownOpen )
        {
            ImGui::OpenPopup( dropDownMenuID.c_str() );
        }

        ImGui::SetNextWindowPos( windowStartPos );
        ImGui::SetNextWindowSizeConstraints( ImVec2( buttonRect.GetWidth(), 0 ), ImVec2( FLT_MAX, FLT_MAX ) );

        if ( ImGui::BeginPopup( dropDownMenuID.c_str(), ImGuiPopupFlags_MouseButtonLeft ) )
        {
            contextMenuCallback();
            ImGui::EndPopup();
        }
    }

    //-------------------------------------------------------------------------

    bool ComboButtonEx( char const* pIcon, const char* pLabel, TFunction<void()> const& comboCallback, ImVec2 const& size, Color const& backgroundColor, Color const& iconColor, Color const& foregroundColor, bool shouldCenterContents )
    {
        EE_ASSERT( pLabel != nullptr );
        EE_ASSERT( comboCallback != nullptr );

        ImGuiStyle const& style = ImGui::GetStyle();

        InlineString const dropDownMenuID( InlineString::CtorSprintf(), "%s##DropdownMenu", pLabel );

        // Calculate button size
        //-------------------------------------------------------------------------

        float const dropDownButtonWidth = ImGui::GetFrameHeight() + style.ItemSpacing.x;
        float buttonWidth = size.x;

        if ( buttonWidth > 0 )
        {
            if ( buttonWidth > dropDownButtonWidth )
            {
                buttonWidth -= dropDownButtonWidth;
            }
            else // Smaller than combo width but not zero or negative
            {
                buttonWidth = 1;
            }
        }
        else if ( buttonWidth == 0 )
        {
            ImVec2 const iconSize = ImGui::CalcTextSize( pIcon, nullptr, true );
            ImVec2 const labelSize = ImGui::CalcTextSize( pLabel, nullptr, true );
            float const spacing = ( iconSize.x > 0 && labelSize.x > 0 ) ? style.ItemSpacing.x : 0.0f;
            buttonWidth = iconSize.x + labelSize.x + spacing + ( style.FramePadding.x * 2 );

        }
        else if ( buttonWidth < 0 )
        {
            buttonWidth = ImGui::GetContentRegionAvail().x - dropDownButtonWidth;
        }

        // Button
        //-------------------------------------------------------------------------

        ImVec2 const actualMainButtonSize = ImVec2( buttonWidth, size.y );
        bool const buttonResult = ButtonEx( pIcon, pLabel, actualMainButtonSize, backgroundColor, iconColor, foregroundColor, shouldCenterContents );
        ImRect const mainButtonRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
        ImVec2 const windowStartPos = mainButtonRect.GetBL() - ImVec2( 0, ImGui::GetStyle().FrameRounding );

        uint32_t const hoveredColor = backgroundColor.GetScaledColor( 1.15f );
        uint32_t const activeColor = backgroundColor.GetScaledColor( 1.25f );

        uint32_t buttonColor = backgroundColor;

        if ( ImGui::IsItemHovered() )
        {
            if ( ImGui::IsItemActive() )
            {
                buttonColor = activeColor;
            }
            else
            {
                buttonColor = hoveredColor;
            }
        }

        // Gap
        //-------------------------------------------------------------------------

        ImGui::SameLine( 0, 0 );

        // Combo
        //-------------------------------------------------------------------------

        ImGui::PushID( dropDownMenuID.c_str() );

        ImGui::PushStyleColor( ImGuiCol_Button, (ImVec4) backgroundColor );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, hoveredColor );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, activeColor );
        ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) foregroundColor );
        bool isPressed = ImGui::Button( "##dropdownButton", ImVec2( dropDownButtonWidth, mainButtonRect.GetHeight() ) );
        ImRect const dropDownButtonRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
        ImGui::PopStyleColor( 4 );

        static constexpr char const dropdownID[] = "##dropdownMenu";
        bool isDropDownOpen = ImGui::IsPopupOpen( dropdownID, ImGuiPopupFlags_None);
        if ( isPressed && !isDropDownOpen )
        {
            ImGui::OpenPopup( dropdownID );
        }

        ImGui::SetNextWindowPos( windowStartPos );
        ImGui::SetNextWindowSizeConstraints( ImVec2( buttonWidth + dropDownButtonWidth, 0 ), ImVec2( FLT_MAX, FLT_MAX ) );
        if ( ImGui::BeginPopup( dropdownID ) )
        {
            comboCallback();
            ImGui::EndPopup();
        }
        ImGui::PopID();

        // Fill gap between button and drop down button
        //-------------------------------------------------------------------------

        auto pDrawList = ImGui::GetWindowDrawList();
        ImVec2 const fillerMin = mainButtonRect.GetTR() + ImVec2( -ImGui::GetStyle().FrameRounding, 0 );
        ImVec2 const fillerMax = dropDownButtonRect.GetBL() + ImVec2( ImGui::GetStyle().FrameRounding, 0 );
        pDrawList->AddRectFilled( fillerMin, fillerMax, buttonColor );

        // Draw arrow
        //-------------------------------------------------------------------------

        float const flArrowHeight = pDrawList->_Data->FontSize;
        float arrowVerticalPos = dropDownButtonRect.GetCenter().y - ( flArrowHeight / 2 );
        pDrawList->PushClipRect( dropDownButtonRect.Min, dropDownButtonRect.Max );
        ImGui::RenderArrow( pDrawList, ImVec2( dropDownButtonRect.Min.x + style.FramePadding.x + style.ItemSpacing.x, arrowVerticalPos ), foregroundColor, ImGuiDir_Down, 1.0f );
        pDrawList->PopClipRect();

        return buttonResult;
    }

    //-------------------------------------------------------------------------

    bool InputTextEx( const char* pInputTextID, char* pBuffer, size_t bufferSize, bool hasClearButton, char const* pHelpText, TFunction<void()> const& comboCallback, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* pUserData)
    {
        EE_ASSERT( pInputTextID != nullptr );

        ImGui::PushID( pInputTextID );

        ImGuiStyle const& style = ImGui::GetStyle();
        ImGuiContext& g = *GImGui;
        auto pDrawList = ImGui::GetWindowDrawList();

        // Calculate size
        //-------------------------------------------------------------------------

        bool const hasCombo = comboCallback != nullptr;
        float const clearButtonWidth = 20;
        float const comboButtonWidth = ImGui::GetFrameHeight() + style.ItemSpacing.x;

        float requiredExtraWidth = 0.0f;
        if ( hasClearButton ) requiredExtraWidth += clearButtonWidth;
        if ( hasCombo ) requiredExtraWidth += comboButtonWidth;

        float totalWidgetWidth = 0;
        if ( g.NextItemData.HasFlags & ImGuiNextItemDataFlags_HasWidth )
        {
            totalWidgetWidth = g.NextItemData.Width;
        }

        float inputWidth = 0;
        if ( totalWidgetWidth > 0 )
        {
            inputWidth = Math::Max( totalWidgetWidth - requiredExtraWidth, 1.0f );
        }
        else if ( totalWidgetWidth == 0 )
        {
            inputWidth = Math::Max( ImGui::GetContentRegionAvail().x - requiredExtraWidth, 1.0f ); // Max allowable size
            inputWidth = Math::Min( inputWidth, 100.0f ); // arbitrary choice
        }
        else if ( totalWidgetWidth < 0 )
        {
            inputWidth = Math::Max( ImGui::GetContentRegionAvail().x - requiredExtraWidth, 1.0f );
        }

        // Draw Background
        //-------------------------------------------------------------------------

        if ( hasClearButton )
        {
            pDrawList->AddRectFilled( ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + ImVec2( inputWidth + clearButtonWidth, ImGui::GetFrameHeight() ), ImGui::ColorConvertFloat4ToU32( style.Colors[ImGuiCol_FrameBg] ), style.FrameRounding );
        }

        // Draw Input
        //-------------------------------------------------------------------------

        InlineString const inputFinalID( InlineString::CtorSprintf(), "##%s", pInputTextID ); // Ensure we have no label!

        ImGui::SetNextItemWidth( inputWidth );
        bool inputResult = ImGui::InputText( inputFinalID.c_str(), pBuffer, bufferSize, flags, callback, pUserData );
        inputResult |= ImGui::IsItemDeactivatedAfterEdit();
        ImRect const inputRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
        ImVec2 const windowStartPos = inputRect.GetBL() - ImVec2( 0, style.FrameRounding );
        ImVec2 fillerRectMin = inputRect.GetTR();

        // Clear Button
        //-------------------------------------------------------------------------

        if ( hasClearButton )
        {
            ImGui::SameLine( 0, 0 );

            // Draw clear button
            bool const isInputFocused = ImGui::IsItemFocused() && ImGui::IsItemActive();
            bool const isbufferEmpty = strlen( pBuffer ) == 0;
            if ( !isbufferEmpty )
            {
                ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 0, style.FramePadding.y ) );
                if ( FlatButton( EE_ICON_CLOSE"##Clear", ImVec2( clearButtonWidth, ImGui::GetFrameHeight() ), ImGuiX::Style::s_colorText ) )
                {
                    Memory::MemsetZero( pBuffer, bufferSize );
                    inputResult = true;
                }
                fillerRectMin.x += ImGui::GetItemRectSize().x;
                ImGui::PopStyleVar();
            }
            else
            {
                ImGui::Dummy( ImVec2( clearButtonWidth, 0 ) );
            }

            // Draw filter text
            if ( pHelpText != nullptr && isbufferEmpty && !isInputFocused )
            {
                ImVec2 const textPos = inputRect.GetTL() + ImVec2( style.ItemSpacing.x, style.FramePadding.y );
                pDrawList->AddText( textPos, ImGui::ColorConvertFloat4ToU32( style.Colors[ImGuiCol_TextDisabled] ), pHelpText );
            }
        }

        // Combo
        //-------------------------------------------------------------------------

        if ( hasCombo )
        {
            // Gap
            //-------------------------------------------------------------------------

            ImGui::SameLine( 0, 0 );

            // Combo Button
            //-------------------------------------------------------------------------

            InlineString const comboMenuID( InlineString::CtorSprintf(), "%s##ComboMenu", pInputTextID );
            ImGui::PushID( comboMenuID.c_str() );

            bool isPressed = ImGui::Button( "##comboButton", ImVec2( comboButtonWidth, 0 ) );
            ImRect const comboButtonRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

            static constexpr char const comboID[] = "##comboMenu";
            bool isComboOpen = ImGui::IsPopupOpen( comboID, ImGuiPopupFlags_None );
            if ( isPressed && !isComboOpen )
            {
                ImGui::OpenPopup( comboID );
            }

            ImGui::SetNextWindowPos( windowStartPos );
            ImGui::SetNextWindowSizeConstraints( ImVec2( inputWidth + requiredExtraWidth, 0 ), ImVec2( FLT_MAX, FLT_MAX ) );
            if ( ImGui::BeginPopup( comboID ) )
            {
                comboCallback();
                ImGui::EndPopup();
            }
            ImGui::PopID();

            // Fill gap between button and drop down button
            //-------------------------------------------------------------------------

            ImVec2 const fillerMin = fillerRectMin + ImVec2( -style.FrameRounding, 0 );
            ImVec2 const fillerMax = comboButtonRect.GetBL() + ImVec2( style.FrameRounding, 0 );
            pDrawList->AddRectFilled( fillerMin, fillerMax, ImGui::ColorConvertFloat4ToU32( style.Colors[ImGuiCol_FrameBg] ) );

            // Draw arrow
            //-------------------------------------------------------------------------

            ImGui::RenderArrow( pDrawList, ImVec2( comboButtonRect.Min.x + style.FramePadding.x + style.ItemSpacing.x, comboButtonRect.Min.y + style.FramePadding.y ), ImGui::ColorConvertFloat4ToU32( style.Colors[ImGuiCol_Text] ), ImGuiDir_Down, 1.0f );
        }

        //-------------------------------------------------------------------------

        ImGui::PopID();

        return inputResult;
    }

    //-------------------------------------------------------------------------

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

    bool DrawSpinner( char const* pButtonLabel, Color const& color, float size, float thickness, float padding )
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

        float finalSize = size;

        // Try to fill the remaining space if possible
        if ( finalSize < 0 )
        {
            finalSize = Math::Min( ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y );
        }

        // Ensure that the minimum size is always the frame height
        if ( finalSize <= 0 )
        {
            finalSize = ImGui::GetFrameHeight();
        }

        // Calculate pos, radius and bounding box
        //-------------------------------------------------------------------------

        ImVec2 const startPos = ImGui::GetCursorScreenPos();
        float const radius = ( ( finalSize - thickness ) / 2 ) - padding;

        bool const buttonResult = ImGui::InvisibleButton( pButtonLabel, ImVec2( finalSize, finalSize ) );
        ImRect const spinnerRect = ImRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
        //pWindow->DrawList->AddRect( bb.Min, bb.Max, Colors::Pink.ToUInt32() ); // Debug

        float const time = (float) ImGui::GetTime();
        float const start = Math::Abs( Math::Sin( time * 1.8f ) * ( numSegments - 5 ) );
        float const min = Math::Pi * 2.0f * ( start ) / numSegments;
        float const max = Math::Pi * 2.0f * ( numSegments - 3 ) / numSegments;

        ImVec2 const center = spinnerRect.GetCenter();

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

    constexpr static float const g_labelWidth = 24.0f;

    constexpr static float const g_transformLabelColWidth = 24;

    static bool DrawFloatEditorWithLabel( char const* pID, char const* pLabel, float const& width, Color const& color, float* pValue, bool isReadOnly = false )
    {
        bool result = false;

        // Label
        //-------------------------------------------------------------------------

        ImVec2 const cursorPosBefore = ImGui::GetCursorScreenPos();
        ImGui::Dummy( ImVec2( g_labelWidth, 0 ) );
        ImGui::SameLine();
        ImVec2 const cursorPosAfter = ImGui::GetCursorScreenPos();

        if ( pLabel[0] != '#' )
        {
            ImGuiX::ScopedFont sf( Font::MediumBold, color );
            ImGui::AlignTextToFramePadding();
            ImVec2 const offset = ImVec2( Math::Floor( ( g_labelWidth - ImGui::CalcTextSize( pLabel ).x ) / 2 ), 0.0f );
            ImGui::SetCursorScreenPos( cursorPosBefore + offset );
            ImGui::Text( pLabel );
            ImGui::SetCursorScreenPos( cursorPosAfter );
        }

        // Editor
        //-------------------------------------------------------------------------

        {
            ImGuiX::ScopedFont sf( Font::Small );
            ImGui::SetNextItemWidth( width - g_labelWidth );
            ImGui::InputFloat( pID, pValue, 0, 0, "%.3f", isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0 );
            result = ImGui::IsItemDeactivatedAfterEdit();
        }

        return result;
    }

    static void DrawFloatElementReadOnly( char const* pLabel, float const& width, Color const& color, float const& value )
    {
        InlineString ID( InlineString::CtorSprintf(), "##%s", pLabel );
        DrawFloatEditorWithLabel( ID.c_str(), pLabel, width, color, const_cast<float*>( &value ), true );
    }

    //-------------------------------------------------------------------------

    bool InputFloat2Impl( Float2& value, float width )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, ImGui::GetStyle().ItemSpacing.y ) );
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const inputWidth = Math::Round( contentWidth / 2 );

        //-------------------------------------------------------------------------

        bool valueUpdated = false;
        if ( DrawFloatEditorWithLabel( "##x", "X", inputWidth, Colors::MediumRed, &value.m_x ) )
        {
            valueUpdated = true;
        }

        ImGui::SameLine();

        if ( DrawFloatEditorWithLabel( "##y", "Y", inputWidth, Colors::LimeGreen, &value.m_y ) )
        {
            valueUpdated = true;
        }

        ImGui::PopStyleVar();

        return valueUpdated;
    }

    bool InputFloat3Impl( Float3& value, float width )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, ImGui::GetStyle().ItemSpacing.y ) );
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const inputWidth = Math::Round( contentWidth / 3 );

        //-------------------------------------------------------------------------

        bool valueUpdated = false;
        if ( DrawFloatEditorWithLabel( "##x", "X", inputWidth, Colors::MediumRed, &value.m_x ) )
        {
            valueUpdated = true;
        }

        ImGui::SameLine();
        if ( DrawFloatEditorWithLabel( "##y", "Y", inputWidth, Colors::LimeGreen, &value.m_y ) )
        {
            valueUpdated = true;
        }

        ImGui::SameLine();
        if ( DrawFloatEditorWithLabel( "##z", "Z", inputWidth, Colors::RoyalBlue, &value.m_z ) )
        {
            valueUpdated = true;
        }

        ImGui::PopStyleVar();

        return valueUpdated;
    }

    bool InputFloat4Impl( Float4& value, float width )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, ImGui::GetStyle().ItemSpacing.y ) );
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const inputWidth = Math::Round( contentWidth / 4 );

        //-------------------------------------------------------------------------

        bool valueUpdated = false;
        if ( DrawFloatEditorWithLabel( "##x", "X", inputWidth, Colors::MediumRed, &value.m_x ) )
        {
            valueUpdated = true;
        }

        ImGui::SameLine();
        if ( DrawFloatEditorWithLabel( "##y", "Y", inputWidth, Colors::LimeGreen, &value.m_y ) )
        {
            valueUpdated = true;
        }

        ImGui::SameLine();
        if ( DrawFloatEditorWithLabel( "##z", "Z", inputWidth, Colors::RoyalBlue, &value.m_z ) )
        {
            valueUpdated = true;
        }

        ImGui::SameLine();
        if ( DrawFloatEditorWithLabel( "##w", "W", inputWidth, Colors::DarkOrange, &value.m_w ) )
        {
            valueUpdated = true;
        }

        ImGui::PopStyleVar();

        return valueUpdated;
    }

    bool InputFloat2( void* pID, Float2& value, float width )
    {
        ImGui::PushID( pID );
        bool valueUpdated = InputFloat2Impl( value, width );
        ImGui::PopID();
        return valueUpdated;
    }

    bool InputFloat3( void* pID, Float3& value, float width )
    {
        ImGui::PushID( pID );
        bool valueUpdated = InputFloat3Impl( value, width );
        ImGui::PopID();
        return valueUpdated;
    }

    bool InputFloat4( void* pID, Float4& value, float width )
    {
        ImGui::PushID( pID );
        bool valueUpdated = InputFloat4Impl( value, width );
        ImGui::PopID();
        return valueUpdated;
    }

    bool InputFloat2( char const* pID, Float2& value, float width )
    {
        ImGui::PushID( pID );
        bool valueUpdated = InputFloat2Impl( value, width );
        ImGui::PopID();
        return valueUpdated;
    }

    bool InputFloat3( char const* pID, Float3& value, float width )
    {
        ImGui::PushID( pID );
        bool valueUpdated = InputFloat3Impl( value, width );
        ImGui::PopID();
        return valueUpdated;
    }

    bool InputFloat4( char const* pID, Float4& value, float width )
    {
        ImGui::PushID( pID );
        bool valueUpdated = InputFloat4Impl( value, width );
        ImGui::PopID();
        return valueUpdated;
    }

    static bool InputTransformImpl( Transform& value, float width, bool allowScaleEditing )
    {
        bool valueUpdated = false;

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 0, 2 ) );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, ImGui::GetStyle().ItemSpacing.y ) );

        if ( ImGui::BeginTable( "Transform", 2, ImGuiTableFlags_None, ImVec2( width, 0 ) ) )
        {
            ImGui::TableSetupColumn( "Header", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, g_transformLabelColWidth );
            ImGui::TableSetupColumn( "Values", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );

            ImGui::TableNextRow();
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( EE_ICON_ROTATE_360 );
                ImGuiX::TextTooltip( "Rotation" );

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
                ImGui::Text( EE_ICON_AXIS_ARROW );
                ImGuiX::TextTooltip( "Translation" );

                ImGui::TableNextColumn();
                Float3 translation = value.GetTranslation();
                if ( ImGuiX::InputFloat3( "T", translation ) )
                {
                    value.SetTranslation( translation );
                    valueUpdated = true;
                }
            }

            if ( allowScaleEditing )
            {
                ImGui::TableNextRow();
                {
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( EE_ICON_ARROW_TOP_RIGHT_BOTTOM_LEFT );
                    ImGuiX::TextTooltip( "Scale" );

                    ImGui::TableNextColumn();
                    float scale = value.GetScale();
                    if ( DrawFloatEditorWithLabel( "##S", "##S", ImGui::GetContentRegionAvail().x, Colors::HotPink, &scale ) )
                    {
                        value.SetScale( scale );
                        valueUpdated = true;
                    }
                }
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar( 2);

        return valueUpdated;
    }

    bool InputTransform( void* pID, Transform& value, float width, bool allowScaleEditing )
    {
        ImGui::PushID( pID );
        bool valueUpdated = InputTransformImpl( value, width, allowScaleEditing );
        ImGui::PopID();
        return valueUpdated;
    }

    bool InputTransform( char const* pID, Transform& value, float width, bool allowScaleEditing )
    {
        ImGui::PushID( pID );
        bool valueUpdated = InputTransformImpl( value, width, allowScaleEditing );
        ImGui::PopID();
        return valueUpdated;
    }

    //-------------------------------------------------------------------------

    void DrawFloat2( Float2 const& value, float width )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, ImGui::GetStyle().ItemSpacing.y ) );
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = Math::Floor( ( contentWidth - itemSpacing ) / 2 );

        ImGui::PushID( &value );
        DrawFloatEditorWithLabel( "##X", "X", inputWidth, Colors::MediumRed, const_cast<float*>( &value.m_x ), true );
        ImGui::SameLine();
        DrawFloatEditorWithLabel( "##Y", "Y", inputWidth, Colors::LimeGreen, const_cast<float*>( &value.m_y ), true );
        ImGui::PopID();

        ImGui::PopStyleVar();
    }

    void DrawFloat3( Float3 const& value, float width )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, ImGui::GetStyle().ItemSpacing.y ) );
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = Math::Floor( ( contentWidth - ( itemSpacing * 2 ) ) / 3 );

        ImGui::PushID( &value );
        DrawFloatEditorWithLabel( "##X", "X", inputWidth, Colors::MediumRed, const_cast<float*>( &value.m_x ), true );
        ImGui::SameLine( 0, itemSpacing );
        DrawFloatEditorWithLabel( "##Y", "Y", inputWidth, Colors::LimeGreen, const_cast<float*>( &value.m_y ), true );
        ImGui::SameLine( 0, itemSpacing );
        DrawFloatEditorWithLabel( "##Z", "Z", inputWidth, Colors::RoyalBlue, const_cast<float*>( &value.m_z ), true );
        ImGui::PopID();

        ImGui::PopStyleVar();
    }

    void DrawFloat4( Float4 const& value, float width )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, ImGui::GetStyle().ItemSpacing.y ) );
        float const contentWidth = ( width > 0 ) ? width : ImGui::GetContentRegionAvail().x;
        float const itemSpacing = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = Math::Floor( ( contentWidth - ( itemSpacing * 3 ) ) / 4 );

        ImGui::PushID( &value );
        DrawFloatEditorWithLabel( "##X", "X", inputWidth, Colors::MediumRed, const_cast<float*>( &value.m_x ), true );
        ImGui::SameLine( 0, itemSpacing );
        DrawFloatEditorWithLabel( "##Y", "Y", inputWidth, Colors::LimeGreen, const_cast<float*>( &value.m_y ), true );
        ImGui::SameLine( 0, itemSpacing );
        DrawFloatEditorWithLabel( "##Z", "Z", inputWidth, Colors::RoyalBlue, const_cast<float*>( &value.m_z ), true );
        ImGui::SameLine( 0, itemSpacing );
        DrawFloatEditorWithLabel( "##W", "W", inputWidth, Colors::DarkOrange, const_cast<float*>( &value.m_w ), true );
        ImGui::PopID();

        ImGui::PopStyleVar();
    }

    void DrawTransform( Transform const& value, float width, bool showScale )
    {
        ImGui::PushID( &value );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, ImGui::GetStyle().ItemSpacing.y ) );
        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 0, 2 ) );
        if ( ImGui::BeginTable( "Transform", 2, ImGuiTableFlags_None, ImVec2( width, 0 ) ) )
        {
            ImGui::TableSetupColumn( "Header", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, g_transformLabelColWidth );
            ImGui::TableSetupColumn( "Values", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );

            ImGui::TableNextRow();
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( EE_ICON_ROTATE_360 );
                ImGuiX::TextTooltip( "Rotation" );

                ImGui::TableNextColumn();
                Float3 const rotation = value.GetRotation().ToEulerAngles().GetAsDegrees();
                ImGuiX::DrawFloat3( rotation );
            }

            ImGui::TableNextRow();
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( EE_ICON_AXIS_ARROW );
                ImGuiX::TextTooltip( "Translation" );

                ImGui::TableNextColumn();
                Float3 const translation = value.GetTranslation();
                ImGuiX::DrawFloat3( translation );
            }

            if ( showScale )
            {
                ImGui::TableNextRow();
                {
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( EE_ICON_ARROW_TOP_RIGHT_BOTTOM_LEFT );
                    ImGuiX::TextTooltip( "Scale" );

                    ImGui::TableNextColumn();
                    float const scale = value.GetScale();
                    DrawFloatEditorWithLabel( "##S", "", ImGui::GetContentRegionAvail().x, Colors::HotPink, const_cast<float*>( &scale ), true );
                }
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar( 2 );
        ImGui::PopID();
    }

    //-------------------------------------------------------------------------
    // Advanced Widgets
    //-------------------------------------------------------------------------

    void FilterData::Clear()
    {
        Memory::MemsetZero( m_buffer, s_bufferSize );
        m_tokens.clear();
    }

    bool FilterData::MatchesFilter( String string )
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

    bool FilterData::MatchesFilter( InlineString string )
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

    void FilterData::SetFilter( String const& filterText )
    {
        uint32_t const lengthToCopy = Math::Min( s_bufferSize, (uint32_t) filterText.length() );
        memcpy( m_buffer, filterText.c_str(), lengthToCopy );
        Update();
    }

    void FilterData::Update()
    {
        StringUtils::Split( String( m_buffer ), m_tokens );

        for ( auto& token : m_tokens )
        {
            token.make_lower();
        }
    }

    //-------------------------------------------------------------------------

    bool FilterWidget::UpdateAndDraw( float width, TBitFlags<Flags> flags )
    {
        bool filterUpdated = false;
        ImGui::PushID( this );

        //-------------------------------------------------------------------------

        if ( flags.IsFlagSet( Flags::TakeInitialFocus ) )
        {
            if ( ImGui::IsWindowAppearing() )
            {
                ImGui::SetKeyboardFocusHere();
            }
        }

        ImGui::SetNextItemWidth( width );
        filterUpdated = InputTextWithClearButton( "Input", m_data.GetFilterHelpText(), m_data.m_buffer, FilterData::s_bufferSize );

        if ( filterUpdated )
        {
            m_data.Update();
        }

        //-------------------------------------------------------------------------

        ImGui::PopID();
        return filterUpdated;
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