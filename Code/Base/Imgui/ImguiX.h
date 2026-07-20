#pragma once

#include "Base/_Module/API.h"
#include "ImguiFont.h"
#include "ImguiStyle.h"
#include "Base/ThirdParty/imgui/imgui_internal.h"
#include "Base/Render/RHI.h"
#include "Base/Math/Transform.h"
#include "Base/Math/Rectangle.h"
#include "Base/Types/Color.h"
#include "Base/Types/String.h"
#include "Base/Types/BitFlags.h"
#include "Base/Types/Function.h"
#include "Base/Types/Arrays.h"
#include "Base/Time/Time.h"

//-------------------------------------------------------------------------
// ImGui Extensions
//-------------------------------------------------------------------------
// This is the primary integration of DearImgui in Esoterica.
//
// * Provides the necessary imgui state updates through the frame start/end functions
// * Provides helpers for common operations
//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    //-------------------------------------------------------------------------
    // General helpers
    //-------------------------------------------------------------------------

    EE_BASE_API ImVec2 ClampToRect( ImRect const& rect, ImVec2 const& inPoint );

    // Returns the closest point on the rect border to the specified point
    EE_BASE_API ImVec2 GetClosestPointOnRectBorder( ImRect const& rect, ImVec2 const& inPoint );

    // Is this a valid name ID character (i.e. A-Z, a-z, 0-9, _ )
    inline bool IsValidNameIDChar( ImWchar c )
    {
        return isalnum( c ) || c == '_';
    }

    // Filter a text callback restricting it to valid name ID characters
    inline int FilterNameIDChars( ImGuiInputTextCallbackData* data )
    {
        if ( IsValidNameIDChar( data->EventChar ) )
        {
            return 0;
        }
        return 1;
    }

    // Filter a text callback restricting it to valid name path characters
    inline int FilterPathChars( ImGuiInputTextCallbackData* data )
    {
        if ( isalnum( data->EventChar ) || data->EventChar == '_' || data->EventChar == '\\' ||  data->EventChar == '/' )
        {
            return 0;
        }
        return 1;
    }

    // Filter a text callback restricting it to valid name path characters
    inline int FilterFilenameChars( ImGuiInputTextCallbackData* data )
    {
        if ( isalnum( data->EventChar ) || data->EventChar == '_' )
        {
            return 0;
        }

        return 1;
    }

    // Display a modal popup that is restricted to the current window's viewport
    EE_BASE_API bool BeginViewportPopupModal( ImGuiID viewportID, char const* pPopupName, bool* pIsPopupOpen, ImVec2 const& size = ImVec2( 0, 0 ), ImGuiCond windowSizeCond = ImGuiCond_Always, ImGuiWindowFlags windowFlags = ( ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) );

    // Cancels an option dialog via ESC
    inline bool CancelDialogViaEsc( bool isDialogOpen )
    {
        if ( ImGui::IsKeyPressed( ImGuiKey_Escape ) )
        {
            ImGui::CloseCurrentPopup();
            return false;
        }

        return isDialogOpen;
    }

    // This is needed to help with specific layouts
    EE_BASE_API float CalculateButtonWidth( char const* pText );
    EE_BASE_API float CalculateButtonHeight( char const* pText );
    EE_BASE_API ImVec2 CalculateButtonDimensions( char const* pText );

    // Create a trimmed string that fits in the desired with, will trim from the end and append...
    EE_BASE_API void TrimStringToWidth( char const* pStr, float width, InlineString& outString );

    inline void DrawShadowedText( Color textColor, Color shadowColor, TFunction<void( Color )> const& drawFunc )
    {
        ImVec2 const cursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos( cursorPos + ImVec2( 0, 1 ) );
        ImGui::Indent( 1 );
        drawFunc( shadowColor );
        ImGui::Unindent( 1 );
        ImGui::SetCursorPos( cursorPos );
        drawFunc( textColor );
    }

    inline void DrawShadowedText( Color textColor, TFunction<void( Color )> const& drawFunc ) { DrawShadowedText( textColor, Colors::Black, drawFunc ); }

    inline void DrawShadowedText( Color textColor, Color shadowColor, char const* pFormat, ... )
    {
        va_list args;
        va_start( args, pFormat );

        ImVec2 const cursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos( cursorPos + ImVec2( 0, 1 ) );
        ImGui::Indent( 1 );
        ImGui::TextColoredV( shadowColor, pFormat, args );
        ImGui::Unindent( 1 );
        ImGui::SetCursorPos( cursorPos );
        ImGui::TextColoredV( textColor, pFormat, args );

        va_end( args );
    }

    inline void DrawShadowedText( Color textColor, char const* pFormat, ... )
    {
        va_list args;
        va_start( args, pFormat );

        ImVec2 const cursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos( cursorPos + ImVec2( 0, 1 ) );
        ImGui::Indent( 1 );
        ImGui::TextColoredV( Colors::Black, pFormat, args );
        ImGui::Unindent( 1 );
        ImGui::SetCursorPos( cursorPos );
        ImGui::TextColoredV( textColor, pFormat, args );

        va_end( args );
    }

    EE_BASE_API void DrawSeverityIcon( Severity severity );

    inline Color GetSeverityColor( Severity severity )
    {
        static Color const severityColors[4] = { Colors::CornflowerBlue, Colors::Gold, Colors::Red, Colors::Red };
        return severityColors[(int32_t) severity];
    }

    //-------------------------------------------------------------------------
    // Layout and Separators
    //-------------------------------------------------------------------------

    // Same as the Imgui::SameLine except it also draws a vertical separator.
    EE_BASE_API void SameLineSeparator( float width = 0, Color const& color = Colors::Transparent );

    struct CollapsibleGroupBoxSettings
    {
        inline void Reset() { *this = CollapsibleGroupBoxSettings(); }

    public:

        bool            m_initiallyOpen = true;
        Color           m_headerColor = ImGuiX::Style::s_colorGray4;
        Color           m_headerTextColor = ImGuiX::Style::s_colorText;
        Color           m_backgroundColor = ImGuiX::Style::s_colorGray7;
        bool            m_hasBorder = true;
        float           m_rounding = 4;
        ImVec2          m_contentsPadding = ImVec2( 6, 6 );
        float           m_contentsHeight = 0; // 0 = auto, -1 = fill, other = fixed
    };

    // Create a collapsible group box
    EE_BASE_API void CollapsibleGroupBox( char const* pLabelAndID, TFunction<void()> const& drawContentsFunction, CollapsibleGroupBoxSettings const& settings = CollapsibleGroupBoxSettings() );

    EE_BASE_API void DrawHeader( char const* pIcon, char const* pLabel, Color iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), Color labelColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), float height = 0, bool offsetHeaderOutwardsToMatchCollapsingHeaders = false );

    EE_FORCE_INLINE void DrawHeaderMatchingCollapsingHeader( char const* pIcon, char const* pLabel, Color iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), Color labelColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), float height = 0 )
    {
        DrawHeader( pIcon, pLabel, iconColor, labelColor, height, true );
    }

    EE_FORCE_INLINE void DrawHeader( char const* pLabel, Color labelColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), float height = 0 ) { DrawHeader( nullptr, pLabel, Colors::Transparent, labelColor, height ); }

    //-------------------------------------------------------------------------
    // Basic Widgets
    //-------------------------------------------------------------------------

    // Draw a tooltip for the immediately preceding item (uses the style hover timer)
    EE_BASE_API void ItemTooltip( const char* fmt, ... );

    // Advanced version of the item tooltip (allows you to do whatever you want when the hover timer elapses)
    EE_BASE_API void ItemTooltipAdvanced( TFunction<void()> const& drawTooltipCode );

    // For use with text items - needed because the GImGui->HoveredIdTimer is not updated for text items
    EE_BASE_API void TextTooltip( const char* fmt, ... );

    // A smaller check box allowing us to use a larger frame padding value
    EE_BASE_API bool Checkbox( char const* pLabel, bool* pValue );

    // A smaller check box allowing us to use a larger frame padding value
    EE_BASE_API bool CheckboxFlags( char const* pLabel, uint32_t* pflags, uint32_t flagsValue );

    // Checkbox that supports tristate values ( <0 = mixed, 0 = false, >0 = true );
    EE_BASE_API bool CheckBoxTristate( char const* pLabel, int32_t* pValue );

    // Buttons
    //-------------------------------------------------------------------------
    // Custom buttons allowing colors and icons

    struct ButtonSettings
    {
        inline void Reset() { *this = ButtonSettings(); }

    public:

        Color   m_backgroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Button] );
        Color   m_hoverColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] );
        Color   m_activeColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] );
        Color   m_iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] );
        Color   m_foregroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] );
        bool    m_shouldCenterContents = false;
    };

    // Custom EE Button - support all various options - syntactic sugar option below
    EE_BASE_API bool ButtonEx( char const* pIcon, char const* pLabel, ImVec2 const& size = ImVec2( 0, 0 ), ButtonSettings const& settings = ButtonSettings() );

    // Draw a colored button
    inline bool ButtonColored( char const* pLabel, Color const& backgroundColor, Color const& foregroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false )
    {
        ButtonSettings settings;
        settings.m_backgroundColor = backgroundColor;
        settings.m_foregroundColor = foregroundColor;
        settings.m_hoverColor = backgroundColor.GetScaledColor( 1.15f );
        settings.m_activeColor = backgroundColor.GetScaledColor( 1.25f );
        settings.m_shouldCenterContents = shouldCenterContents;
        return ButtonEx( nullptr, pLabel, size, settings );
    }

    // Draw a button with an explicit icon (mainly useful if you want to control icon color explicitly and separately from the label color )
    inline bool IconButton( char const* pIcon, char const* pLabel, Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false )
    {
        ButtonSettings settings{ .m_iconColor = iconColor, .m_shouldCenterContents = shouldCenterContents };
        return ButtonEx( pIcon, pLabel, size, settings );
    }

    // Draw a colored icon button (mainly useful if you want to control icon color explicitly and separately from the label color )
    inline bool IconButtonColored( char const* pIcon, char const* pLabel, Color const& backgroundColor, Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), Color const& foregroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false )
    {
        ButtonSettings settings;
        settings.m_backgroundColor = backgroundColor;
        settings.m_iconColor = iconColor;
        settings.m_foregroundColor = foregroundColor;
        settings.m_hoverColor = backgroundColor.GetScaledColor( 1.15f );
        settings.m_activeColor = backgroundColor.GetScaledColor( 1.25f );
        settings.m_shouldCenterContents = shouldCenterContents;
        return ButtonEx( pIcon, pLabel, size, settings );
    }

    // Draws a flat button - a button with no background
    inline bool FlatButton( char const* pLabel, ImVec2 const& size = ImVec2( 0, 0 ), Color const& foregroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), bool shouldCenterContents = false )
    {
        ButtonSettings settings{ .m_backgroundColor = Colors::Transparent, .m_foregroundColor = foregroundColor, .m_shouldCenterContents = shouldCenterContents };
        return ButtonEx( nullptr, pLabel, size, settings );
    }

    // Draw a flat icon button
    inline bool FlatIconButton( char const* pIcon, char const* pLabel, Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false )
    {
        ButtonSettings settings{ .m_backgroundColor = Colors::Transparent, .m_iconColor = iconColor, .m_shouldCenterContents = shouldCenterContents };
        return ButtonEx( pIcon, pLabel, size, settings );
    }

    // Drop Down Button
    //-------------------------------------------------------------------------
    // A button that creates a menu when clicked

    // Button that creates a drop down menu once clicked
    EE_BASE_API void DropDownButtonEx( char const* pIcon, char const* pLabel, TFunction<void()> const& contextMenuCallback, ImVec2 const& size = ImVec2( 0, 0 ), ButtonSettings const& settings = ButtonSettings() );

    // Button that creates a drop down menu once clicked
    inline void DropDownButton( char const* pLabel, TFunction<void()> const& contextMenuCallback, ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false )
    {
        ButtonSettings settings{ .m_shouldCenterContents = shouldCenterContents };
        return DropDownButtonEx( nullptr, pLabel, contextMenuCallback, size, settings );
    }

    // Button that creates a drop down menu once clicked
    inline void DropDownIconButton( char const* pIcon, char const* pLabel, TFunction<void()> const& contextMenuCallback, Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false )
    {
        ButtonSettings settings{ .m_iconColor = iconColor, .m_shouldCenterContents = shouldCenterContents };
        return DropDownButtonEx( pIcon, pLabel, contextMenuCallback, size, settings );
    }

    // Button that creates a drop down menu once clicked
    inline void DropDownIconButtonColored( char const* pIcon, char const* pLabel, TFunction<void()> const& contextMenuCallback, Color const& backgroundColor, Color const& iconColor, Color const& foregroundColor, ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false )
    {
        ButtonSettings settings;
        settings.m_backgroundColor = backgroundColor;
        settings.m_iconColor = iconColor;
        settings.m_foregroundColor = foregroundColor;
        settings.m_hoverColor = backgroundColor.GetScaledColor( 1.15f );
        settings.m_activeColor = backgroundColor.GetScaledColor( 1.25f );
        settings.m_shouldCenterContents = shouldCenterContents;

        return DropDownButtonEx( pIcon, pLabel, contextMenuCallback, size, settings );
    }

    // Combo Button
    //-------------------------------------------------------------------------
    // A button that has an attached combo menu

    EE_BASE_API bool ComboButtonEx( char const* pIcon, const char* pLabel, TFunction<void()> const& comboCallback, ImVec2 const& size = ImVec2( 0, 0 ), ButtonSettings const& settings = ButtonSettings() );

    inline bool ComboButton( char const* pLabel, TFunction<void()> const& comboCallback, ImVec2 const& size = ImVec2( 0, 0 ) )
    {
        return ComboButtonEx( nullptr, pLabel, comboCallback, size );
    }

    inline bool ComboIconButton( char const* pIcon, char const* pLabel, TFunction<void()> const& comboCallback, Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), ImVec2 const& size = ImVec2( 0, 0 ) )
    {
        ButtonSettings settings{ .m_iconColor = iconColor };
        return ComboButtonEx( pIcon, pLabel, comboCallback, size, settings );
    }

    inline bool ComboIconButtonColored( char const* pIcon, const char* pLabel, TFunction<void()> const& comboCallback, Color const& backgroundColor, Color const& iconColor, Color const& foregroundColor, ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false )
    {
        ButtonSettings settings;
        settings.m_backgroundColor = backgroundColor;
        settings.m_iconColor = iconColor;
        settings.m_foregroundColor = foregroundColor;
        settings.m_hoverColor = backgroundColor.GetScaledColor( 1.15f );
        settings.m_activeColor = backgroundColor.GetScaledColor( 1.25f );
        settings.m_shouldCenterContents = shouldCenterContents;

        return ComboButtonEx( pIcon, pLabel, comboCallback, size, settings );
    }

    // Custom InputText
    //-------------------------------------------------------------------------
    // Note these will all return true if deactivated after edit as that is the most common use case in esoterica

    EE_BASE_API bool InputTextEx( const char* pInputTextID, char* pBuffer, size_t bufferSize, bool hasClearButton, char const* pHelpText, TFunction<void()> const& comboCallback, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* pUserData = nullptr );

    // Draw an input text field that has an attached combo menu
    inline bool InputTextCombo( const char* pInputTextID, char* pBuffer, size_t bufferSize, TFunction<void()> const& comboCallback, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* pUserData = nullptr )
    {
        EE_ASSERT( comboCallback != nullptr );
        return InputTextEx( pInputTextID, pBuffer, bufferSize, false, nullptr, comboCallback, flags, callback, pUserData );
    }

    // Draw an input text field that show some help-text when empty and that has a clear button
    inline bool InputTextWithClearButton( const char* pInputTextID, char const* pHelpText, char* pBuffer, size_t bufferSize, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* pUserData = nullptr )
    {
        return InputTextEx( pInputTextID, pBuffer, bufferSize, true, pHelpText, nullptr, flags, callback, pUserData );
    }

    // Draw an input text field that show some help-text when empty and that has a clear button
    inline bool InputTextComboWithClearButton( const char* pInputTextID, char const* pHelpText, char* pBuffer, size_t bufferSize, TFunction<void()> const& comboCallback, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* pUserData = nullptr )
    {
        EE_ASSERT( comboCallback != nullptr );
        return InputTextEx( pInputTextID, pBuffer, bufferSize, true, pHelpText, comboCallback, flags, callback, pUserData );
    }

    // Toggle Buttons
    //-------------------------------------------------------------------------
    // A button that can toggle between on and off

    struct ToggleButtonSettings
    {
        char const* m_pOnIcon = nullptr;
        char const* m_pOffIcon = nullptr;
        char const* m_pOnLabel = nullptr;
        char const* m_pOffLabel = nullptr;
        Color       m_onBackgroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Button] );
        Color       m_offBackgroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Button] );
        Color       m_onForegroundColor = ImGuiX::Style::s_colorAccent0;
        Color       m_offForegroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] );
        Color       m_onIconColor = ImGuiX::Style::s_colorAccent0;
        Color       m_offIconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] );
        bool        m_isFlatButton = false;
    };

    // Toggle button
    EE_BASE_API bool ToggleButtonEx( ToggleButtonSettings const& settings, bool& value, ImVec2 const& size = ImVec2( 0, 0 ) );

    // Toggle button
    inline bool ToggleButton( char const* pOnIcon, char const* pOffIcon, char const* pOnLabel, char const* pOffLabel, bool& value, ImVec2 const& size = ImVec2( 0, 0 ), Color const& onColor = ImGuiX::Style::s_colorAccent0, Color const& offColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ) )
    {
        ToggleButtonSettings settings{ .m_pOnIcon = pOnIcon, .m_pOffIcon = pOffIcon, .m_pOnLabel = pOnLabel, .m_pOffLabel = pOffLabel };
        settings.m_onIconColor = settings.m_onForegroundColor = onColor;
        settings.m_offIconColor = settings.m_offForegroundColor = offColor;
        return ToggleButtonEx( settings, value, size );
    }

    // Toggle button
    inline bool ToggleButton( char const* pOnLabel, char const* pOffLabel, bool& value, ImVec2 const& size = ImVec2( 0, 0 ), Color const& onColor = ImGuiX::Style::s_colorAccent0, Color const& offColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ) )
    {
        ToggleButtonSettings settings{ .m_pOnLabel = pOnLabel, .m_pOffLabel = pOffLabel };
        settings.m_onForegroundColor = onColor;
        settings.m_offForegroundColor = offColor;
        return ToggleButtonEx( settings, value, size );
    }

    // Toggle button with no background
    inline bool FlatToggleButton( char const* pOnIcon, char const* pOnLabel, char const* pOffIcon, char const* pOffLabel, bool& value, ImVec2 const& size = ImVec2( 0, 0 ), Color const& onColor = ImGuiX::Style::s_colorAccent0, Color const& offColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ) )
    {
        ToggleButtonSettings settings{ .m_pOnIcon = pOnIcon, .m_pOffIcon = pOffIcon, .m_pOnLabel = pOnLabel, .m_pOffLabel = pOffLabel };
        settings.m_onIconColor = settings.m_onForegroundColor = onColor;
        settings.m_offIconColor = settings.m_offForegroundColor = offColor;
        settings.m_isFlatButton = true;
        return ToggleButtonEx( settings, value, size );
    }

    // Toggle button
    inline bool FlatToggleButton( char const* pOnLabel, char const* pOffLabel, bool& value, ImVec2 const& size = ImVec2( 0, 0 ), Color const& onColor = ImGuiX::Style::s_colorAccent0, Color const& offColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ) )
    {
        return FlatToggleButton( nullptr, pOnLabel, nullptr, pOffLabel, value, size, onColor, offColor );
    }

    //-------------------------------------------------------------------------

    // Draw a basic spinner - the spinner acts like an invisible button for all intents and purposes
    EE_BASE_API bool DrawSpinner( char const* pButtonLabel, Color const& color = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), float size = 0, float thickness = 3.0f, float padding = ImGui::GetStyle().FramePadding.y );

    // Draw some flashing text
    EE_BASE_API void DrawFlashingText( char const* pText, Color const& color, float frequencyScale = 1.8f );

    //-------------------------------------------------------------------------

    EE_BASE_API bool InputFloat( char const* pID, float& value, Color markerColor, float width = -1 );
    EE_BASE_API bool InputFloat( void* pID, float& value, Color markerColor, float width = -1 );
    EE_FORCE_INLINE bool InputFloat( float &value, Color markerColor, float width = -1 ) { return InputFloat( &value, value, markerColor, width ); }

    EE_BASE_API bool InputFloat2( char const* pID, Float2& value, float width = -1 );
    EE_BASE_API bool InputFloat2( void* pID, Float2& value, float width = -1 );
    EE_FORCE_INLINE bool InputFloat2( Float2 &value, float width = -1 ) { return InputFloat2( &value, value, width ); }

    EE_BASE_API bool InputFloat3( char const* pID, Float3& value, float width = -1 );
    EE_BASE_API bool InputFloat3( void* pID, Float3& value, float width = -1 );
    EE_FORCE_INLINE bool InputFloat3( Float3 &value, float width = -1 ) { return InputFloat3( &value, value, width ); }

    EE_BASE_API bool InputFloat4( char const* pID, Float4& value, float width = -1 );
    EE_BASE_API bool InputFloat4( void* pID, Float4& value, float width = -1 );
    EE_FORCE_INLINE bool InputFloat4( Float4 &value, float width = -1 ) { return InputFloat4( &value, value, width ); }

    EE_BASE_API bool InputTransform( char const* pID, Transform& value, float width = -1 );
    EE_BASE_API bool InputTransform( void* pID, Transform& value, float width = -1 );
    EE_FORCE_INLINE bool InputTransform( Transform &value, float width = -1 ) { return InputTransform( &value, value, width ); }

    EE_BASE_API bool InputTransformNoScale( char const* pID, Transform& value, float width = -1 );
    EE_BASE_API bool InputTransformNoScale( void* pID, Transform& value, float width = -1 );
    EE_FORCE_INLINE bool InputTransformNoScale( Transform &value, float width = -1 ) { return InputTransformNoScale( &value, value, width ); }

    EE_BASE_API void DrawFloat( char const* pID, float& value, Color markerColor, float width = -1 );
    EE_BASE_API void DrawFloat( void* pID, float& value, Color markerColor, float width = -1 );
    EE_FORCE_INLINE void DrawFloat( float &value, Color markerColor, float width = -1 ) { return DrawFloat( &value, value, markerColor, width ); }

    EE_BASE_API void DrawFloat2( char const* pID, Float2 const& value, float width = -1 );
    EE_BASE_API void DrawFloat2( void* pID, Float2 const& value, float width = -1 );
    EE_FORCE_INLINE void DrawFloat2( Float2 const& value, float width = -1 ) { return DrawFloat2( (void*) &value, value, width ); }

    EE_BASE_API void DrawFloat3( char const* pID, Float3 const& value, float width = -1 );
    EE_BASE_API void DrawFloat3( void* pID, Float3 const& value, float width = -1 );
    EE_FORCE_INLINE void DrawFloat3( Float3 const& value, float width = -1 ) { return DrawFloat2( (void*) &value, value, width ); }

    EE_BASE_API void DrawFloat4( char const* pID, Float4 const& value, float width = -1 );
    EE_BASE_API void DrawFloat4( void* pID, Float4 const& value, float width = -1 );
    EE_FORCE_INLINE void DrawFloat4( Float4 const& value, float width = -1 ) { return DrawFloat2( (void*) &value, value, width ); }

    EE_BASE_API void DrawTransform( char const* pID, Transform const& value, float width = -1 );
    EE_BASE_API void DrawTransform( void* pID, Transform const& value, float width = -1 );
    EE_FORCE_INLINE void DrawTransform( Transform const& value, float width = -1 ) { return DrawTransform( (void*) &value, value, width ); }

    EE_BASE_API void DrawTransformNoScale( char const* pID, Transform const& value, float width = -1 );
    EE_BASE_API void DrawTransformNoScale( void* pID, Transform const& value, float width = -1 );
    EE_FORCE_INLINE void DrawTransformNoScale( Transform const& value, float width = -1 ) { return DrawTransformNoScale( (void*) &value, value, width ); }

    // Help Markers
    //-------------------------------------------------------------------------

    // Draw a help marker icon with a tooltip helptext
    static void HelpMarker( const char* pHelpText )
    {
        ImGui::TextDisabled( EE_ICON_HELP_CIRCLE_OUTLINE );
        if ( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayShort ) && ImGui::BeginTooltip() )
        {
            ImGui::PushTextWrapPos( ImGui::GetFontSize() * 35.0f );
            ImGui::TextUnformatted( pHelpText );
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    // Get the width for a help marker at the regular font size
    EE_FORCE_INLINE consteval float GetHelpMarkerDefaultWidth() { return 26.0f; }

    //-------------------------------------------------------------------------
    // Drawing
    //-------------------------------------------------------------------------

    // Draw an arrow between two points
    EE_BASE_API void DrawArrow( ImDrawList* pDrawList, ImVec2 const& arrowStart, ImVec2 const& arrowEnd, Color const& color, float arrowWidth, float arrowHeadWidth = 5.0f );

    EE_BASE_API void DrawDashedLine( ImDrawList* pDrawList, ImVec2 const& start, ImVec2 const& end, ImU32 color, float thickness, float dashLength = 10.f, float gapLength = 10.f );

    //-------------------------------------------------------------------------
    // Notifications
    //-------------------------------------------------------------------------

    EE_BASE_API void NotifyInfo( const char* format, ... );
    EE_BASE_API void NotifySuccess( const char* format, ... );
    EE_BASE_API void NotifyWarning( const char* format, ... );
    EE_BASE_API void NotifyError( const char* format, ... );

    //-------------------------------------------------------------------------
    // Images
    //-------------------------------------------------------------------------

    EE_BASE_API ImTextureID GetImTextureID( Render::RHI::Sampler* pSampler, Render::RHI::Texture* pTexture );

    //-------------------------------------------------------------------------
    // Advanced widgets
    //-------------------------------------------------------------------------

    // Helper that maintains all the state needed to create a filter widget
    // The widget below simply combines this state with a 'InputTextWithClearButton' widget as a convenience
    class EE_BASE_API FilterData
    {
    public:

        constexpr static uint32_t const s_bufferSize = 255;

    public:

        // Manually set the filter buffer
        void SetFilter( String const& filterText );

        // Set the help text shown when we dont have focus and the filter is empty
        void SetFilterHelpText( String const& helpText ) { m_filterHelpText = helpText; }

        // Get the filter help text
        char const* GetFilterHelpText() { return m_filterHelpText.c_str(); }

        // Regenerated the tokens
        void Update();

        // Clear the filter
        inline void Clear();

        // Do we have a filter set?
        inline bool HasFilterSet() const { return !m_tokens.empty(); }

        // Get the split filter text token
        inline TVector<String> const& GetFilterTokens() const { return m_tokens; }

        // Does a provided string match the current filter - the string copy is intentional!
        bool MatchesFilter( String string );

        // Does a provided string match the current filter - the string copy is intentional!
        bool MatchesFilter( InlineString string );

        // Does a provided string match the current filter
        bool MatchesFilter( char const* pString ) { return MatchesFilter( InlineString( pString ) ); }

    public:

        char                m_buffer[s_bufferSize] = { 0 };

    private:

        TVector<String>     m_tokens;
        String              m_filterHelpText = "Filter...";
    };

    //-------------------------------------------------------------------------

    // A simple filter entry widget that allows you to string match to some entered text
    class EE_BASE_API FilterWidget
    {
        constexpr static uint32_t const s_bufferSize = 255;

    public:

        enum Flags : uint8_t
        {
            TakeInitialFocus = 0
        };

    public:

        // Draws the filter. Returns true if the filter has been updated
        bool UpdateAndDraw( float width = -1, TBitFlags<Flags> flags = TBitFlags<Flags>() );

        // Manually set the filter buffer
        inline void SetFilter( String const& filterText ) { m_data.SetFilter( filterText ); }

        // Set the help text shown when we dont have focus and the filter is empty
        inline void SetFilterHelpText( String const& helpText ) { m_data.SetFilterHelpText( helpText ); }

        // Clear the filter
        inline void Clear() { m_data.Clear(); }

        // Do we have a filter set?
        inline bool HasFilterSet() const { return m_data.HasFilterSet(); }

        // Get the filter data
        FilterData const& GetFilterData() const { return m_data; }

        // Get the split filter text token
        inline TVector<String> const& GetFilterTokens() const { return m_data.GetFilterTokens(); }

        // Does a provided string match the current filter
        inline bool MatchesFilter( String const& string ) { return m_data.MatchesFilter( string ); }

        // Does a provided string match the current filter
        inline bool MatchesFilter( InlineString const& string ) { return m_data.MatchesFilter( string ); }

        // Does a provided string match the current filter
        inline bool MatchesFilter( char const* pString ) { return m_data.MatchesFilter( InlineString( pString ) ); }

    private:

        FilterData m_data;
    };

    //-------------------------------------------------------------------------
    // Application level widgets
    //-------------------------------------------------------------------------

    struct EE_BASE_API ApplicationTitleBar
    {
        constexpr static float const s_windowControlButtonWidth = 45;
        constexpr static float const s_minimumDraggableGap = 24; // Minimum open gap left open to allow dragging
        constexpr static float const s_sectionPadding = 8; // Padding between the window frame/window controls and the menu/control sections

        static inline float GetWindowsControlsWidth() { return s_windowControlButtonWidth * 3; }
        static void DrawWindowControls();

    public:

        // This function takes two delegates and sizes each representing the title bar menu and an extra optional controls section
        void Draw( TFunction<void()>&& menuSectionDrawFunction = TFunction<void()>(), float menuSectionWidth = 0, TFunction<void()>&& controlsSectionDrawFunction = TFunction<void()>(), float controlsSectionWidth = 0 );

        // Get the screen space rectangle for this title bar
        Math::ScreenSpaceRectangle const& GetScreenRectangle() const { return m_rect; }

    private:

        Math::ScreenSpaceRectangle  m_rect;
    };
}

// Conversion functions
//-------------------------------------------------------------------------

namespace ImGui
{
    EE_FORCE_INLINE void PushStyleColor( ImGuiCol idx, EE::Color c )
    {
        PushStyleColor( idx, ImVec4( c ) );
    }
}

#endif