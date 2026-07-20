#include "ImguiX.h"

#if EE_DEVELOPMENT_TOOLS
#include "Base/Imgui/ImguiXNotifications.h"
#include "Base/Render/RHI.h"
#include "Base/Memory/Memory.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    //-------------------------------------------------------------------------

    EE_BASE_API ImTextureID GetImTextureID( Render::RHI::Sampler* pSampler, Render::RHI::Texture* pTexture )
    {
        Render::RHI::SamplerStateHandle samplerHandle = Render::RHI::GetSamplerStateHandle( pSampler );
        Render::RHI::TextureHandle textureHandle = Render::RHI::GetTextureHandle( pTexture, Render::RHI::DescriptorTypeFlags::Texture, 0);
        return ImTextureID_Pack( samplerHandle, textureHandle );
    }

    //-------------------------------------------------------------------------
    // General helpers
    //-------------------------------------------------------------------------

    bool BeginViewportPopupModal( ImGuiID viewportID, char const* pPopupName, bool* pIsPopupOpen, ImVec2 const& size, ImGuiCond windowSizeCond, ImGuiWindowFlags windowFlags )
    {
        ImGui::OpenPopup( pPopupName );
        if ( size.x != 0 || size.y != 0 )
        {
            ImGui::SetNextWindowSize( size, windowSizeCond );
        }
        ImGui::SetNextWindowViewport( viewportID );
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

    float CalculateButtonWidth( char const* pText )
    {
        ImVec2 buttonSize = ImGui::CalcTextSize( pText, nullptr, true );
        buttonSize.x += ( ImGui::GetStyle().FramePadding.x * 2 );
        return buttonSize.x;
    }

    float CalculateButtonHeight( char const* pText )
    {
        ImVec2 buttonSize = ImGui::CalcTextSize( pText, nullptr, true );
        buttonSize.y += ( ImGui::GetStyle().FramePadding.y * 2 );
        return buttonSize.y;
    }

    ImVec2 CalculateButtonDimensions( char const* pText )
    {
        ImVec2 buttonSize = ImGui::CalcTextSize( pText, nullptr, true );
        buttonSize += ( ImGui::GetStyle().FramePadding * 2 );
        return buttonSize;
    }

    void TrimStringToWidth( char const* pStr, float maxWidth, InlineString& outString )
    {
        EE_ASSERT( pStr != nullptr && maxWidth > 0 );
        size_t const strLength = strlen( pStr );
        EE_ASSERT( strLength > 0 );

        float requiredStringWidth = ImGui::CalcTextSize( pStr ).x;
        if ( requiredStringWidth < maxWidth )
        {
            outString = pStr;
            return;
        }

        size_t trimmedLength = Math::Max( strLength - 1, size_t( 1 ) );
        while ( trimmedLength >= 1 && requiredStringWidth > maxWidth )
        {
            outString.resize( trimmedLength );
            memcpy( outString.data(), pStr, trimmedLength );
            outString.append( "..." );
            requiredStringWidth = ImGui::CalcTextSize( outString.c_str() ).x;
            trimmedLength--;
        }
    }

    void DrawSeverityIcon( Severity severity )
    {
        switch ( severity )
        {
            case Severity::Info:
            ImGui::TextColored( GetSeverityColor( severity ), EE_ICON_INFORMATION);
            break;

            case Severity::Warning:
            ImGui::TextColored( GetSeverityColor( severity ), EE_ICON_ALERT );
            break;

            case Severity::Error:
            ImGui::TextColored( GetSeverityColor( severity ), EE_ICON_CLOSE_CIRCLE );
            break;

            case Severity::FatalError:
            ImGui::TextColored( GetSeverityColor( severity ), EE_ICON_ALERT_DECAGRAM );
            break;

            default:
            break;
        }
    }

    //-------------------------------------------------------------------------
    // Layout and Separators
    //-------------------------------------------------------------------------

    void CollapsibleGroupBox( char const* pLabelAndID, TFunction<void()> const& drawContentsFunction, CollapsibleGroupBoxSettings const& settings )
    {
        auto& style = ImGui::GetStyle();

        constexpr float const borderWidth = 8;
        constexpr float const halfBorderWidth = 4;

        auto pDrawList = ImGui::GetWindowDrawList();
        pDrawList->ChannelsSplit( 2 );
        pDrawList->ChannelsSetCurrent( 1 );

        // Header
        //-------------------------------------------------------------------------

        ImGui::SetNextItemOpen( settings.m_initiallyOpen, ImGuiCond_FirstUseEver );
        ImGui::PushStyleColor( ImGuiCol_Header, settings.m_headerColor );
        ImGui::PushStyleColor( ImGuiCol_HeaderActive, settings.m_headerColor );
        ImGui::PushStyleColor( ImGuiCol_HeaderHovered, settings.m_headerColor );

        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
        ImGui::PushStyleVar( ImGuiStyleVar_IndentSpacing, 0 );
        ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, settings.m_rounding );

        bool isOpen = false;
        {
            ScopedFont const sf( Font::MediumBold, settings.m_headerTextColor );
            isOpen = ImGui::TreeNodeEx( pLabelAndID, ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth );
        }

        ImGui::PopStyleVar( 3 );
        ImGui::PopStyleColor( 3 );

        auto headerMin = ImGui::GetItemRectMin();
        auto headerMax = ImGui::GetItemRectMax();

        //-------------------------------------------------------------------------

        if ( isOpen )
        {
            ImGui::PushID( pLabelAndID );

            // Size and Flags
            ImVec2 outerChildSize( 0, settings.m_contentsHeight );
            uint32_t outerChildFlags = ImGuiChildFlags_AlwaysUseWindowPadding;
            if ( settings.m_contentsHeight == 0 )
            {
                outerChildFlags |= ImGuiChildFlags_AutoResizeY;
            }
            else if ( settings.m_contentsHeight == -1 )
            {
                outerChildSize.y = Math::Max( 0.0f, ImGui::GetContentRegionAvail().y );
            }
            else if( settings.m_hasBorder )
            {
                outerChildSize.y += ( borderWidth * 2 );
            }

            uint32_t const outerChildWindowFlags = settings.m_hasBorder ? ImGuiWindowFlags_NoScrollbar : 0;

            // Padding - if we dont have a border there is no inner child so just use the contents padding
            ImVec2 outChildWindowPadding = settings.m_hasBorder ? ImVec2( borderWidth, borderWidth ) : settings.m_contentsPadding;
           
            // If we have a border, shift the outer window up so that the upper border overlaps the tree item
            if ( settings.m_hasBorder )
            {
                ImGui::SetCursorPosY( ImGui::GetCursorPosY() - halfBorderWidth );
            }

            // Create outer child
            ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 0 );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, outChildWindowPadding );
            ImGui::PushStyleColor( ImGuiCol_ChildBg, Colors::Transparent );
            ImGui::PushStyleColor( ImGuiCol_ScrollbarBg, settings.m_backgroundColor );
            bool const drawChildWindowContents = ImGui::BeginChild( "Outer", outerChildSize, outerChildFlags, outerChildWindowFlags);
            ImGui::PopStyleColor( 2 );
            ImGui::PopStyleVar( 2 );

            if ( drawChildWindowContents )
            {
                if ( settings.m_hasBorder )
                {
                    ImVec2 innerChildSize( 0, settings.m_contentsHeight );

                    uint32_t innerChildFlags = ImGuiChildFlags_AlwaysUseWindowPadding;
                    if ( settings.m_contentsHeight == 0 )
                    {
                        innerChildFlags |= ImGuiChildFlags_AutoResizeY;
                    }
                    else if ( settings.m_contentsHeight == -1 )
                    {
                        innerChildSize.y = Math::Max( 0.0f, ImGui::GetContentRegionAvail().y );
                    }

                    ImGui::PushStyleColor( ImGuiCol_ChildBg, Colors::Transparent );
                    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, settings.m_contentsPadding );
                    ImGui::PushStyleColor( ImGuiCol_ScrollbarBg, settings.m_backgroundColor );
                    if ( ImGui::BeginChild( "Inner", innerChildSize, innerChildFlags, 0 ) )
                    {
                        drawContentsFunction();
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor( 2 );
                    ImGui::PopStyleVar();
                }
                else
                {
                    drawContentsFunction();
                }
            }
            ImGui::EndChild();

            //-------------------------------------------------------------------------

            if ( drawChildWindowContents )
            {
                auto boxMin = ImGui::GetItemRectMin();
                auto boxMax = ImGui::GetItemRectMax();

                pDrawList->ChannelsSetCurrent( 0 );

                if ( settings.m_hasBorder )
                {
                    pDrawList->AddRectFilled( headerMin, boxMax, settings.m_headerColor, settings.m_rounding );
                    pDrawList->AddRectFilled( boxMin + ImVec2( halfBorderWidth, halfBorderWidth ), boxMax - ImVec2( halfBorderWidth, halfBorderWidth ), settings.m_backgroundColor, settings.m_rounding );
                }
                else
                {
                    pDrawList->AddRectFilled( headerMin, headerMax, settings.m_headerColor, settings.m_rounding );
                    pDrawList->AddRectFilled( boxMin, boxMax, settings.m_backgroundColor, settings.m_rounding );
                    
                    pDrawList->ChannelsSetCurrent( 1 );
                    pDrawList->AddRectFilled( boxMin + ImVec2( 0, -style.FrameRounding ), ImVec2( boxMax.x, boxMin.y ), settings.m_headerColor, 0 );
                    pDrawList->AddRectFilled( boxMin, ImVec2( boxMax.x, boxMin.y + style.FrameRounding ), settings.m_backgroundColor, 0 );
                }
            }

            ImGui::PopID();

            ImGui::PushStyleVar( ImGuiStyleVar_IndentSpacing, 0 );
            ImGui::TreePop();
            ImGui::PopStyleVar( 1 );
        }
        else
        {
            pDrawList->ChannelsSetCurrent( 0 );
            pDrawList->AddRectFilled( headerMin, headerMax, settings.m_headerColor, settings.m_rounding );

            ImGui::Dummy( ImVec2( -1, 0 ) );
        }

        pDrawList->ChannelsMerge();
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

    void DrawHeader( char const* pIcon, char const* pLabel, Color iconColor, Color labelColor, float height, bool offsetHeaderOutwardsToMatchCollapsingHeaders )
    {
        EE_ASSERT( pLabel != nullptr );

        ImGuiStyle const &style = ImGui::GetStyle();
        ImGuiX::ScopedFont const sf( ImGuiX::Font::MediumBold );

        ImVec2 iconSize( 0, 0 );
        float spacingWidth = 0;
        if ( pIcon != nullptr )
        {
            iconSize = ImGui::CalcTextSize( pIcon );
            spacingWidth = style.ItemSpacing.x;
        }

        ImVec2 textSize = ImGui::CalcTextSize( pLabel );

        //-------------------------------------------------------------------------

        float const textWidth = iconSize.x + spacingWidth + textSize.x;
        float const outerExtend = offsetHeaderOutwardsToMatchCollapsingHeaders ? IM_TRUNC( style.WindowPadding.x * 0.5f ) : 0; // Same as collapsing header
        float const extraVerticalPadding = IM_TRUNC( style.WindowPadding.y * 0.5f ); // Same as collapsing header

        ImVec2 rectSize( ImGui::GetContentRegionAvail().x, height > 0 ? height : textSize.y + ( ( style.FramePadding.y + extraVerticalPadding ) * 2 ) );
        ImVec2 rect_TL = ImGui::GetCursorScreenPos() - ImVec2( outerExtend, 0 );
        ImVec2 rect_BR = rect_TL + rectSize + ImVec2( outerExtend * 2, 0 );

        ImVec2 const textPos( rect_TL.x + ( rectSize.x - textWidth ) / 2, rect_TL.y + ( rectSize.y - textSize.y ) / 2 );

        //-------------------------------------------------------------------------

        ImGui::Dummy( rectSize );

        auto pDrawList = ImGui::GetWindowDrawList();
        pDrawList->AddRectFilledMultiColor( rect_TL, rect_BR, Style::s_colorGray2, Style::s_colorGray2, Style::s_colorGray4, Style::s_colorGray4 );

        if ( pIcon != nullptr )
        {
            pDrawList->AddText( textPos, iconColor, pIcon );
            pDrawList->AddText( textPos + ImVec2( iconSize.x + spacingWidth, 0 ), labelColor, pLabel );
        }
        else
        {
            pDrawList->AddText( textPos, labelColor, pLabel );
        }
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

    void ItemTooltipAdvanced( TFunction<void()> const& drawTooltipCode )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );
        if ( ImGui::IsItemHovered() && GImGui->HoveredIdTimer > Style::s_toolTipDelay )
        {
            drawTooltipCode();
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

    bool CheckboxFlags( char const* pLabel, uint32_t* pflags, uint32_t flagsValue )
    {
        ImVec2 const newFramePadding( 2, 2 );

        float const originalCursorPosY = ImGui::GetCursorPosY();
        float const offsetY = ImGui::GetStyle().FramePadding.y - newFramePadding.y;
        ImGui::SetCursorPosY( originalCursorPosY + offsetY );

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, newFramePadding );
        bool result = ImGui::CheckboxFlags( pLabel, pflags, flagsValue );
        ImGui::PopStyleVar();

        ImGui::SameLine();
        ImGui::SetCursorPosY( originalCursorPosY );
        ImGui::NewLine();

        return result;
    }

    bool CheckBoxTristate( char const* pLabel, int32_t* pValue )
    {
        bool result = false;

        if ( *pValue < 0 )
        {
            ImGui::PushItemFlag( ImGuiItemFlags_MixedValue, true );
            bool b = false;
            result = Checkbox( pLabel, &b );
            if ( result )
            {
                *pValue = 1;
            }
            ImGui::PopItemFlag();
        }
        else
        {
            bool b = ( *pValue != 0 );
            result = Checkbox( pLabel, &b );
            if ( result )
            {
                *pValue = (int32_t) b;
            }
        }
        return result;
    }

    bool ButtonEx( char const* pIcon, char const* pLabel, ImVec2 const& size, ButtonSettings const& settings )
    {
        bool wasPressed = false;

        //-------------------------------------------------------------------------

        if ( pIcon == nullptr || strlen( pIcon ) == 0 )
        {
            ImGui::PushStyleColor( ImGuiCol_Button, (ImVec4) settings.m_backgroundColor );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, settings.m_hoverColor );
            ImGui::PushStyleColor( ImGuiCol_ButtonActive, settings.m_activeColor );
            ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) settings.m_foregroundColor );
            ImGui::PushStyleVar( ImGuiStyleVar_ButtonTextAlign, settings.m_shouldCenterContents ? ImVec2( 0.5f, 0.5f ) : ImVec2( 0.0f, 0.5f ) );
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

            ImGui::ItemSize( finalButtonSize, style.FramePadding.y );
            ImRect const bb( pos, pos + finalButtonSize );
            if ( !ImGui::ItemAdd( bb, ID ) )
            {
                return false;
            }

            bool hovered = false, held = false;
            wasPressed = ImGui::ButtonBehavior( bb, ID, &hovered, &held, 0 );

            // Render Button
            //-------------------------------------------------------------------------

            ImGui::PushStyleColor( ImGuiCol_Button, (ImVec4) settings.m_backgroundColor );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, settings.m_hoverColor );
            ImGui::PushStyleColor( ImGuiCol_ButtonActive, settings.m_activeColor );
            ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) settings.m_foregroundColor );

            // Render frame
            ImU32 const color = ImGui::GetColorU32( ( held && hovered ) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button );
            ImGui::RenderNavCursor( bb, ID );
            ImGui::RenderFrame( bb.Min, bb.Max, color, true, style.FrameRounding );

            bool const isDisabled = g.CurrentItemFlags & ImGuiItemFlags_Disabled;
            ImU32 const finalIconColor = isDisabled ? Style::s_colorTextDisabled : settings.m_iconColor;

            if ( settings.m_shouldCenterContents )
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

    bool ToggleButtonEx( ToggleButtonSettings const& settings, bool& value, ImVec2 const& size )
    {
        ButtonSettings buttonSettings;

        if ( value )
        {
            buttonSettings.m_iconColor = settings.m_onIconColor;
            buttonSettings.m_foregroundColor = settings.m_onForegroundColor;
            buttonSettings.m_backgroundColor = settings.m_onBackgroundColor;
        }
        else
        {
            buttonSettings.m_iconColor = settings.m_offIconColor;
            buttonSettings.m_foregroundColor = settings.m_offForegroundColor;
            buttonSettings.m_backgroundColor = settings.m_offBackgroundColor;
        }

        buttonSettings.m_hoverColor = buttonSettings.m_backgroundColor.GetScaledColor( 1.15f );
        buttonSettings.m_activeColor = buttonSettings.m_backgroundColor.GetScaledColor( 1.25f );

        //-------------------------------------------------------------------------

        bool const wasPressed = ButtonEx( value ? settings.m_pOnIcon : settings.m_pOffIcon, value ? settings.m_pOnLabel : settings.m_pOffLabel, size, buttonSettings );
        if ( wasPressed )
        {
            value = !value;
        }

        return wasPressed;
    }

    //-------------------------------------------------------------------------

    void DropDownButtonEx( char const* pIcon, char const* pLabel, TFunction<void()> const& contextMenuCallback, ImVec2 const& size, ButtonSettings const& settings )
    {
        EE_ASSERT( pLabel != nullptr );
        EE_ASSERT( contextMenuCallback != nullptr );

        bool const isPressed = ButtonEx( pIcon, pLabel, size, settings );
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

    bool ComboButtonEx( char const* pIcon, const char* pLabel, TFunction<void()> const& comboCallback, ImVec2 const& size, ButtonSettings const& settings )
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
            ImVec2 const iconSize = ( pIcon != nullptr ) ? ImGui::CalcTextSize( pIcon, nullptr, true ) : ImVec2( 0, 0 );
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
        bool const buttonResult = ButtonEx( pIcon, pLabel, actualMainButtonSize, settings );
        ImRect const mainButtonRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
        ImVec2 const windowStartPos = mainButtonRect.GetBL() - ImVec2( 0, ImGui::GetStyle().FrameRounding );

        uint32_t buttonColor = settings.m_backgroundColor;

        if ( ImGui::IsItemHovered() )
        {
            if ( ImGui::IsItemActive() )
            {
                buttonColor = settings.m_activeColor;
            }
            else
            {
                buttonColor = settings.m_hoverColor;
            }
        }

        // Gap
        //-------------------------------------------------------------------------

        ImGui::SameLine( 0, 0 );

        // Combo
        //-------------------------------------------------------------------------

        ImGui::PushID( dropDownMenuID.c_str() );

        ImGui::PushStyleColor( ImGuiCol_Button, (ImVec4) settings.m_backgroundColor );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, settings.m_hoverColor );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, settings.m_activeColor );
        ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) settings.m_foregroundColor );
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
        ImGui::RenderArrow( pDrawList, ImVec2( dropDownButtonRect.Min.x + style.FramePadding.x + style.ItemSpacing.x, arrowVerticalPos ), settings.m_foregroundColor, ImGuiDir_Down, 1.0f );

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

        // Debug
        #if 0
        pWindow->DrawList->AddRect( spinnerRect.Min, spinnerRect.Max, Colors::Pink.ToUInt32() );
        #endif

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

    void DrawFlashingText( char const* pText, Color const& color, float frequencyScale )
    {
        float const time = (float) ImGui::GetTime();
        float const start = Math::Abs( Math::Sin( time * frequencyScale ) );

        Color const scaledColor = color.GetAlphaVersion( start );
        ImGui::TextColored( scaledColor.ToFloat4(), pText );
    }

    //-------------------------------------------------------------------------
    // Math Widgets
    //-------------------------------------------------------------------------

    constexpr static float const g_transformLabelColWidth = 24;
    constexpr static float const g_mathInputFramePaddingX = 12;

    static inline void DrawInputColorMarker( Color color, ImVec2 padding = ImVec2( 4, 4 ), float width = 4 )
    {
        ImVec2 const itemRectMin = ImGui::GetItemRectMin();
        ImVec2 const itemRectMax = ImGui::GetItemRectMax();
        ImVec2 const TL = itemRectMin + padding;
        ImVec2 const BR = ImVec2( itemRectMin.x + padding.x + width, itemRectMax.y - padding.y );

        auto pDrawList = ImGui::GetWindowDrawList();
        pDrawList->AddRectFilled( TL, BR, color );
    };

    static inline bool DrawInputFloat( char const* pID, float* pValue, float width, Color color, bool isReadOnly )
    {
        bool valueUpdated = false;

        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( g_mathInputFramePaddingX, ImGui::GetStyle().FramePadding.y ) );
        ImGui::SetNextItemWidth( width );
        if ( ImGui::InputFloat( pID, pValue, 0, 0, "%.3f", isReadOnly ? ImGuiInputTextFlags_ReadOnly : 0 ) )
        {
            valueUpdated = true;
        }

        if ( !isReadOnly && !valueUpdated )
        {
            valueUpdated = ImGui::IsItemDeactivatedAfterEdit();
        }

        DrawInputColorMarker( color );
        ImGui::PopStyleVar();

        return valueUpdated;
    }

    //-------------------------------------------------------------------------

    static bool InputFloatImpl( ImGuiID ID, float& value, Color markerColor, float width, bool isReadOnly )
    {
        bool valueUpdated = false;
        ImGui::PushID( ID );
        float const totalWidth = ( width <= 0 ) ? ImGui::GetContentRegionAvail().x : width;
        valueUpdated |= DrawInputFloat( "##x", &value, totalWidth, markerColor, isReadOnly );
        ImGui::PopID();
        return valueUpdated;
    }

    static bool InputFloat2Impl( ImGuiID ID, Float2& value, float width, bool isReadOnly )
    {
        bool valueUpdated = false;
        ImGui::PushID( ID );

        //-------------------------------------------------------------------------

        float const totalWidth = ( width <= 0 ) ? ImGui::GetContentRegionAvail().x : width;
        float const inputWidth = ( totalWidth - ImGui::GetStyle().ItemSpacing.x ) / 2;

        //-------------------------------------------------------------------------

        valueUpdated |= DrawInputFloat( "##x", &value.m_x, inputWidth, Style::s_axisColorX, isReadOnly );
        ImGui::SameLine();
        valueUpdated |= DrawInputFloat( "##y", &value.m_y, inputWidth, Style::s_axisColorY, isReadOnly );

        //-------------------------------------------------------------------------

        ImGui::PopID();
        return valueUpdated;
    }

    static bool InputFloat3Impl( ImGuiID ID, Float3& value, float width, bool isReadOnly )
    {
        bool valueUpdated = false;
        ImGui::PushID( ID );

        //-------------------------------------------------------------------------

        float const totalWidth = ( width <= 0 ) ? ImGui::GetContentRegionAvail().x : width;
        float const inputWidth = ( totalWidth - ( ImGui::GetStyle().ItemSpacing.x * 2 ) ) / 3;

        //-------------------------------------------------------------------------

        valueUpdated |= DrawInputFloat( "##x", &value.m_x, inputWidth, Style::s_axisColorX, isReadOnly );
        ImGui::SameLine();
        valueUpdated |= DrawInputFloat( "##y", &value.m_y, inputWidth, Style::s_axisColorY, isReadOnly );
        ImGui::SameLine();
        valueUpdated |= DrawInputFloat( "##z", &value.m_z, inputWidth, Style::s_axisColorZ, isReadOnly );

        //-------------------------------------------------------------------------

        ImGui::PopID();
        return valueUpdated;
    }

    static bool InputFloat4Impl( ImGuiID ID, Float4& value, float width, bool isReadOnly )
    {
        bool valueUpdated = false;
        ImGui::PushID( ID );

        //-------------------------------------------------------------------------

        float const totalWidth = ( width <= 0 ) ? ImGui::GetContentRegionAvail().x : width;
        float const inputWidth = ( totalWidth - ( ImGui::GetStyle().ItemSpacing.x * 3 ) ) / 4;

        //-------------------------------------------------------------------------

        valueUpdated |= DrawInputFloat( "##x", &value.m_x, inputWidth, Style::s_axisColorX, isReadOnly );
        ImGui::SameLine();
        valueUpdated |= DrawInputFloat( "##y", &value.m_y, inputWidth, Style::s_axisColorY, isReadOnly );
        ImGui::SameLine();
        valueUpdated |= DrawInputFloat( "##z", &value.m_z, inputWidth, Style::s_axisColorZ, isReadOnly );
        ImGui::SameLine();
        valueUpdated |= DrawInputFloat( "##w", &value.m_w, inputWidth, Style::s_axisColorW, isReadOnly );

        //-------------------------------------------------------------------------

        ImGui::PopID();
        return valueUpdated;
    }

    static bool InputTransformImpl( ImGuiID ID, Transform& value, float width, bool allowScaleEditing, bool isReadOnly )
    {
        bool valueUpdated = false;
        ImGui::PushID( ID );

        //-------------------------------------------------------------------------

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 2, 4 ) );
        if ( ImGui::BeginTable( "Transform", 2, ImGuiTableFlags_None, ImVec2( width, 0 ) ) )
        {
            ImGui::TableSetupColumn( "Header", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40 );
            ImGui::TableSetupColumn( "Values", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );

            ImGui::TableNextRow();
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( EE_ICON_ROTATE_360" R" );
                ImGuiX::TextTooltip( "Rotation" );

                ImGui::TableNextColumn();
                Float3 rotation = value.GetRotation().ToEulerAngles().GetAsDegrees();
                if ( ImGuiX::InputFloat3Impl( ImGui::GetID( "##R" ), rotation, -1, isReadOnly ) )
                {
                    value.SetRotation( Quaternion( EulerAngles( rotation ) ) );
                    valueUpdated = true;
                }
            }

            ImGui::TableNextRow();
            {
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( EE_ICON_AXIS_ARROW" T" );
                ImGuiX::TextTooltip( "Translation" );

                ImGui::TableNextColumn();
                Float3 translation = value.GetTranslation();
                if( ImGuiX::InputFloat3Impl( ImGui::GetID( "##T" ), translation, -1, isReadOnly ) )
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
                    ImGui::Text( EE_ICON_ARROW_TOP_RIGHT_BOTTOM_LEFT" S" );
                    ImGuiX::TextTooltip( "Scale" );

                    ImGui::TableNextColumn();

                    float scale = value.GetScale();
                    ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( g_mathInputFramePaddingX, ImGui::GetStyle().FramePadding.y ) );
                    ImGui::SetNextItemWidth( -1 );
                    if ( ImGuiX::InputFloatImpl( ImGui::GetID( "##S"), scale, Colors::HotPink, -1, isReadOnly ) )
                    {
                        value.SetScale( scale );
                        valueUpdated = true;
                    }
                    ImGui::PopStyleVar();
                }
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        //-------------------------------------------------------------------------

        ImGui::PopID();
        return valueUpdated;
    }

    bool InputFloat( char const* pID, float& value, Color markerColor, float width ) { return InputFloatImpl( ImGui::GetID( pID ), value, markerColor, width, false ); }
    bool InputFloat( void* pID, float& value, Color markerColor, float width ) { return InputFloatImpl( ImGui::GetID( pID ), value, markerColor, width, false ); }

    bool InputFloat2( char const* pID, Float2& value, float width ) { return InputFloat2Impl( ImGui::GetID( pID ), value, width, false ); }
    bool InputFloat2( void* pID, Float2& value, float width ) { return InputFloat2Impl( ImGui::GetID( pID ), value, width, false ); }
    
    bool InputFloat3( char const* pID, Float3& value, float width ) { return InputFloat3Impl( ImGui::GetID( pID ), value, width, false ); }
    bool InputFloat3( void* pID, Float3& value, float width ) { return InputFloat3Impl( ImGui::GetID( pID ), value, width, false ); }

    bool InputFloat4( char const* pID, Float4& value, float width ) { return InputFloat4Impl( ImGui::GetID( pID ), value, width, false ); }
    bool InputFloat4( void* pID, Float4& value, float width ) { return InputFloat4Impl( ImGui::GetID( pID ), value, width, false ); }

    bool InputTransform( char const* pID, Transform& value, float width ) { return InputTransformImpl( ImGui::GetID( pID ), value, width, true, false ); }
    bool InputTransform( void* pID, Transform& value, float width ) { return InputTransformImpl( ImGui::GetID( pID ), value, width, true, false ); }

    bool InputTransformNoScale( char const* pID, Transform& value, float width ) { return InputTransformImpl( ImGui::GetID( pID ), value, width, false, false ); }
    bool InputTransformNoScale( void* pID, Transform& value, float width ) { return InputTransformImpl( ImGui::GetID( pID ), value, width, false, false ); }

    void DrawFloat( char const* pID, float const& value, Color markerColor, float width ) { InputFloatImpl( ImGui::GetID( pID ), (float&) value, markerColor, width, true ); }
    void DrawFloat( void* pID, float const& value, Color markerColor, float width ) { InputFloatImpl( ImGui::GetID( pID ), (float&) value, markerColor, width, true ); }

    void DrawFloat2( char const* pID, Float2 const& value, float width ) { InputFloat2Impl( ImGui::GetID( pID ), (Float2&) value, width, true ); }
    void DrawFloat2( void* pID, Float2 const& value, float width ) { InputFloat2Impl( ImGui::GetID( pID ), (Float2&) value, width, true ); }

    void DrawFloat3( char const* pID, Float3 const& value, float width ) { InputFloat3Impl( ImGui::GetID( pID ), (Float3&) value, width, true ); }
    void DrawFloat3( void* pID, Float3 const& value, float width ) { InputFloat3Impl( ImGui::GetID( pID ), (Float3&) value, width, true ); }

    void DrawFloat4( char const* pID, Float4 const& value, float width ) { InputFloat4Impl( ImGui::GetID( pID ), (Float4&) value, width, true ); }
    void DrawFloat4( void* pID, Float4 const& value, float width ) { InputFloat4Impl( ImGui::GetID( pID ), (Float4&) value, width, true ); }

    void DrawTransform( char const* pID, Transform const& value, float width ) { InputTransformImpl( ImGui::GetID( pID ), (Transform&) value, width, true, true ); }
    void DrawTransform( void* pID, Transform const& value, float width ) { InputTransformImpl( ImGui::GetID( pID ), (Transform&) value, width, true, true ); }

    void DrawTransformNoScale( char const* pID, Transform const& value, float width ) { InputTransformImpl( ImGui::GetID( pID ), (Transform&) value, width, false, true ); }
    void DrawTransformNoScale( void* pID, Transform const& value, float width ) { InputTransformImpl( ImGui::GetID( pID ), (Transform&) value, width, false, true ); }

    //-------------------------------------------------------------------------
    // Drawing
    //-------------------------------------------------------------------------

    void DrawArrow( ImDrawList* pDrawList, ImVec2 const& arrowStart, ImVec2 const& arrowEnd, Color const& color, float arrowWidth, float arrowHeadWidth )
    {
        EE_ASSERT( pDrawList != nullptr );
        EE_ASSERT( arrowWidth > 0.0f );
        EE_ASSERT( arrowHeadWidth > 0.0f );

        ImVec2 const direction = Vector( arrowEnd - arrowStart ).GetNormalized2();
        ImVec2 const orthogonalDirection( -direction.y, direction.x );

        ImVec2 const triangleSideOffset = orthogonalDirection * arrowHeadWidth;
        ImVec2 const triBase = arrowEnd - ( direction * arrowHeadWidth );
        ImVec2 const tri1 = triBase - triangleSideOffset;
        ImVec2 const tri2 = triBase + triangleSideOffset;

        pDrawList->AddLine( arrowStart, triBase, color, arrowWidth );
        pDrawList->AddTriangleFilled( arrowEnd, tri1, tri2, color );
    }

    void DrawDashedLine( ImDrawList* pDrawList, ImVec2 const& start, ImVec2 const& end, ImU32 color, float thickness, float dashLength, float gapLength )
    {
        EE_ASSERT( pDrawList != nullptr );
        EE_ASSERT( thickness > 0.0f );
        EE_ASSERT( dashLength > 0.0f );
        EE_ASSERT( gapLength > 0.0f );

        Vector lineDirection;
        float lineLength = 0.0f;
        Vector( end - start ).ToDirectionAndLength2( lineDirection, lineLength );

        float const segmentLength = dashLength + gapLength;
        Vector const vSegmentLength( segmentLength );

        Vector dashStart = start;
        float lengthDrawn = 0.0f;
        while( lengthDrawn < lineLength )
        {
            Vector const vClampedDashLength( Math::Min( dashLength, lineLength - lengthDrawn ) );
            Vector dashEnd = Vector::MultiplyAdd( lineDirection, vClampedDashLength, dashStart );
            Vector segmentEnd = Vector::MultiplyAdd( lineDirection, vSegmentLength, dashStart );
            pDrawList->AddLine( dashStart, dashEnd, color, thickness );
            dashStart = segmentEnd;
            lengthDrawn += segmentLength;
        }

        // Check if we are ending the line on a dash, if so draw a dot to show the end of the line
        float const lastDashStartDistance = Math::Floor( lineLength / segmentLength ) * segmentLength;
        if ( ( lastDashStartDistance + dashLength ) < lineLength )
        {
            Vector dotEnd = end;
            Vector dotStart = Vector::MultiplyAdd( lineDirection, Vector( -thickness ), dotEnd );
            pDrawList->AddLine( dotStart, dotEnd, color, thickness );
        }
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
        Memory::MemsetZero( m_buffer, s_bufferSize );
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
}
#endif