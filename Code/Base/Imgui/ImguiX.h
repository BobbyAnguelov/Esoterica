#pragma once

#include "Base/_Module/API.h"
#include "ImguiFont.h"
#include "ImguiStyle.h"
#include "Base/ThirdParty/imgui/imgui_internal.h"
#include "Base/Math/Transform.h"
#include "Base/Math/Rectangle.h"
#include "Base/Types/Color.h"
#include "Base/Types/String.h"
#include "Base/Types/BitFlags.h"
#include "Base/Types/Function.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::Render { class Viewport; }

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

    // Display a modal popup that is restricted to the current window's viewport
    EE_BASE_API bool BeginViewportPopupModal( char const* pPopupName, bool* pIsPopupOpen, ImVec2 const& size = ImVec2( 0, 0 ), ImGuiCond windowSizeCond = ImGuiCond_Always, ImGuiWindowFlags windowFlags = ( ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) );

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

    //-------------------------------------------------------------------------
    // Layout and Separators
    //-------------------------------------------------------------------------

    // Same as the Imgui::SameLine except it also draws a vertical separator.
    EE_BASE_API void SameLineSeparator( float width = 0, Color const& color = Colors::Transparent );

    // Create a collapsible framed child window - Must always call EndCollapsibleChildWindow if you call begin child window
    EE_BASE_API bool BeginCollapsibleChildWindow( char const* pLabelAndID, bool initiallyOpen = true, Color const& backgroundColor = ImGuiX::Style::s_colorGray7 );

    // End a collapsible framed child window - must always be called if you call begin child window to match ImGui child window behavior
    EE_BASE_API void EndCollapsibleChildWindow();

    //-------------------------------------------------------------------------
    // Basic Widgets
    //-------------------------------------------------------------------------

    // Draw a tooltip for the immediately preceding item
    EE_BASE_API void ItemTooltip( const char* fmt, ... );

    // For use with text items - needed because the GImGui->HoveredIdTimer is not updated for text items
    EE_BASE_API void TextTooltip( const char* fmt, ... );

    // A smaller check box allowing us to use a larger frame padding value
    EE_BASE_API bool Checkbox( char const* pLabel, bool* pValue );

    // Buttons
    //-------------------------------------------------------------------------
    // Custom buttons allowing colors and icons

    // Custom EE Button - support all various options - syntactic sugar option below
    EE_BASE_API bool ButtonEx( char const* pIcon, char const* pLabel, ImVec2 const& size = ImVec2( 0, 0 ), Color const& backgroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Button] ), Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), Color const& foregroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), bool shouldCenterContents = false );

    // Draw a colored button
    inline bool ButtonColored( char const* pLabel, Color const& backgroundColor, Color const& foregroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), ImVec2 const& size = ImVec2( 0, 0 ) )
    {
        return ButtonEx( nullptr, pLabel, size, backgroundColor, Colors::Transparent, foregroundColor );
    }

    // Draw a button with an explicit icon (mainly useful if you want to control icon color explicitly and separately from the label color )
    inline bool IconButton( char const* pIcon, char const* pLabel, Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false )
    {
        return ButtonEx( pIcon, pLabel, size, ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Button] ), iconColor, ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), shouldCenterContents );
    }

    // Draw a colored icon button  (mainly useful if you want to control icon color explicitly and separately from the label color )
    inline bool IconButtonColored( char const* pIcon, char const* pLabel, Color const& backgroundColor, Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), Color const& foregroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false )
    {
        return ButtonEx( pIcon, pLabel, size, backgroundColor, iconColor, foregroundColor, shouldCenterContents );
    }

    // Draws a flat button - a button with no background
    inline bool FlatButton( char const* pLabel, ImVec2 const& size = ImVec2( 0, 0 ), Color const& foregroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ) )
    {
        return ButtonEx( nullptr, pLabel, size, Colors::Transparent, Colors::Transparent, ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ) );
    }

    // Draw a colored icon button
    inline bool FlatIconButton( char const* pIcon, char const* pLabel, Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false )
    {
        return ButtonEx( pIcon, pLabel, size, Colors::Transparent, iconColor, ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), shouldCenterContents );
    }

    // Drop Down Button
    //-------------------------------------------------------------------------
    // A button that creates a menu when clicked

    // Button that creates a drop down menu once clicked
    EE_BASE_API void DropDownButtonEx( char const* pIcon, char const* pLabel, TFunction<void()> const& contextMenuCallback, ImVec2 const& size = ImVec2( 0, 0 ), Color const& backgroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Button] ), Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), Color const& foregroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), bool shouldCenterContents = false );

    // Button that creates a drop down menu once clicked
    inline void DropDownButton( char const* pLabel, TFunction<void()> const& contextMenuCallback, ImVec2 const& size = ImVec2( 0, 0 ) )
    {
        return DropDownButtonEx( nullptr, pLabel, contextMenuCallback, size );
    }

    // Button that creates a drop down menu once clicked
    inline void DropDownIconButton( char const* pIcon, char const* pLabel, TFunction<void()> const& contextMenuCallback, Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), ImVec2 const& size = ImVec2( 0, 0 ) )
    {
        return DropDownButtonEx( pIcon, pLabel, contextMenuCallback, size, ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Button] ), iconColor, ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ) );
    }

    // Button that creates a drop down menu once clicked
    inline void DropDownIconButtonColored( char const* pIcon, char const* pLabel, TFunction<void()> const& contextMenuCallback, Color const& backgroundColor, Color const& iconColor, Color const& foregroundColor, ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false )
    {
        return DropDownButtonEx( pIcon, pLabel, contextMenuCallback, size, backgroundColor, iconColor, foregroundColor, shouldCenterContents );
    }

    // Combo Button
    //-------------------------------------------------------------------------
    // A button that has an attached combo menu

    EE_BASE_API bool ComboButtonEx( char const* pIcon, const char* pLabel, TFunction<void()> const& comboCallback, ImVec2 const& size = ImVec2( 0, 0 ), Color const& backgroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Button] ), Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), Color const& foregroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), bool shouldCenterContents = false );

    inline bool ComboButton( char const* pLabel, TFunction<void()> const& comboCallback, ImVec2 const& size = ImVec2( 0, 0 ) )
    {
        return ComboButtonEx( nullptr, pLabel, comboCallback, size );
    }

    inline bool ComboIconButton( char const* pIcon, char const* pLabel, TFunction<void()> const& comboCallback, Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), ImVec2 const& size = ImVec2( 0, 0 ) )
    {
        return ComboButtonEx( pIcon, pLabel, comboCallback, size, ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Button] ), iconColor );
    }

    inline bool ComboIconButtonColored( char const* pIcon, const char* pLabel, TFunction<void()> const& comboCallback, Color const& backgroundColor, Color const& iconColor, Color const& foregroundColor, ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false )
    {
        return ComboButtonEx( pIcon, pLabel, comboCallback, size, backgroundColor, iconColor, foregroundColor, shouldCenterContents );
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

    // Toggle button
    EE_BASE_API bool ToggleButtonEx( char const* pOnIcon, char const* pOnLabel, char const* pOffIcon, char const* pOffLabel, bool& value, ImVec2 const& size = ImVec2( 0, 0 ), Color const& onForegroundColor = ImGuiX::Style::s_colorAccent0, Color const& onIconColor = ImGuiX::Style::s_colorAccent0, Color const& offForegroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), Color const& offIconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ) );

    // Toggle button
    inline bool ToggleButton( char const* pOnLabel, char const* pOffLabel, bool& value, ImVec2 const& size = ImVec2( 0, 0 ), Color const& onColor = ImGuiX::Style::s_colorAccent0, Color const& offColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ) )
    {
        return ToggleButtonEx( nullptr, pOnLabel, nullptr, pOffLabel, value, size, onColor, Colors::Transparent, offColor );
    }

    // Toggle button with no background
    inline bool FlatToggleButton( char const* pOnIcon, char const* pOnLabel, char const* pOffIcon, char const* pOffLabel, bool& value, ImVec2 const& size = ImVec2( 0, 0 ), Color const& onForegroundColor = ImGuiX::Style::s_colorAccent0, Color const& onIconColor = ImGuiX::Style::s_colorAccent0, Color const& offForegroundColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), Color const& offIconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ) )
    {
        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
        bool result = ToggleButtonEx( pOnIcon, pOnLabel, pOnLabel, pOffLabel, value, size, onForegroundColor, onIconColor, offForegroundColor, offIconColor );
        ImGui::PopStyleColor( 1 );

        return result;
    }

    // Toggle button
    inline bool FlatToggleButton( char const* pOnLabel, char const* pOffLabel, bool& value, ImVec2 const& size = ImVec2( 0, 0 ), Color const& onColor = ImGuiX::Style::s_colorAccent0, Color const& offColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ) )
    {
        return FlatToggleButton( nullptr, pOnLabel, nullptr, pOffLabel, value, size, onColor, offColor );
    }

    //-------------------------------------------------------------------------

    // Draw an arrow between two points
    EE_BASE_API void DrawArrow( ImDrawList* pDrawList, ImVec2 const& arrowStart, ImVec2 const& arrowEnd, Color const& color, float arrowWidth, float arrowHeadWidth = 5.0f );

    // Draw a basic spinner - the spinner acts like an invisible button for all intents and purposes
    EE_BASE_API bool DrawSpinner( char const* pButtonLabel, Color const& color = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), float size = 0, float thickness = 3.0f, float padding = ImGui::GetStyle().FramePadding.y );

    //-------------------------------------------------------------------------

    EE_BASE_API bool InputFloat2( char const* pID, Float2& value, float width = -1 );
    EE_BASE_API bool InputFloat2( void* pID, Float2& value, float width = -1 );
    EE_FORCE_INLINE bool InputFloat2( Float2* pValue, float width = -1 ) { return InputFloat2( pValue, *pValue, width ); }

    EE_BASE_API bool InputFloat3( char const* pID, Float3& value, float width = -1 );
    EE_BASE_API bool InputFloat3( void* pID, Float3& value, float width = -1 );
    EE_FORCE_INLINE bool InputFloat3( Float3* pValue, float width = -1 ) { return InputFloat3( pValue, *pValue, width ); }

    EE_BASE_API bool InputFloat4( char const* pID, Float4& value, float width = -1 );
    EE_BASE_API bool InputFloat4( void* pID, Float4& value, float width = -1 );
    EE_FORCE_INLINE bool InputFloat4( Float4* pValue, float width = -1 ) { return InputFloat4( pValue, *pValue, width ); }

    EE_BASE_API bool InputTransform( char const* pID, Transform& value, float width = -1, bool allowScaleEditing = true );
    EE_BASE_API bool InputTransform( void* pID, Transform& value, float width = -1, bool allowScaleEditing = true );
    EE_FORCE_INLINE bool InputTransform( Transform* pValue, float width = -1, bool allowScaleEditing = true ) { return InputTransform( pValue, *pValue, width, allowScaleEditing ); }

    EE_BASE_API void DrawFloat2( Float2 const& value, float width = -1 );
    EE_BASE_API void DrawFloat3( Float3 const& value, float width = -1 );
    EE_BASE_API void DrawFloat4( Float4 const& value, float width = -1 );

    EE_BASE_API void DrawTransform( Transform const& value, float width = -1, bool showScale = true );

    //-------------------------------------------------------------------------

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

    EE_FORCE_INLINE void Image( ImTextureID imageID, ImVec2 const& size, ImVec2 const& uv0 = ImVec2( 0, 0 ), ImVec2 const& uv1 = ImVec2( 1, 1 ), Color const& tintColor = Colors::White, Color const& borderColor = Colors::Transparent )
    {
        ImGui::Image( imageID, size, uv0, uv1, ImVec4( tintColor ), ImVec4( borderColor ) );
    }

    EE_FORCE_INLINE void ImageButton( char const* pButtonID, ImTextureID imageID, ImVec2 const& size, ImVec2 const& uv0 = ImVec2( 0, 0 ), ImVec2 const& uv1 = ImVec2( 1, 1 ), Color const& backgroundColor = Colors::Transparent, Color const& tintColor = Colors::White )
    {
        ImGui::ImageButton( pButtonID, imageID, size, uv0, uv1, ImVec4( backgroundColor ), ImVec4( tintColor ) );
    }

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

    // A simple 3D gizmo to show the orientation of a camera in a scene
    struct EE_BASE_API OrientationGuide
    {
        constexpr static float const g_windowPadding = 4.0f;
        constexpr static float const g_windowRounding = 2.0f;
        constexpr static float const g_guideDimension = 55.0f;
        constexpr static float const g_axisHeadRadius = 3.0f;
        constexpr static float const g_axisHalfLength = ( g_guideDimension / 2 ) - g_axisHeadRadius - 4.0f;
        constexpr static float const g_worldRenderDistanceZ = 5.0f;
        constexpr static float const g_axisThickness = 2.0f;

    public:

        static Float2 GetSize() { return Float2( g_guideDimension, g_guideDimension ); }
        static float GetWidth() { return g_guideDimension / 2; }
        static void Draw( Float2 const& guideOrigin, Render::Viewport const& viewport );
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
#endif