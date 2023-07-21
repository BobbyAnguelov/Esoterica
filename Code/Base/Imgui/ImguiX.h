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

namespace EE::Render { class Texture; class Viewport; }

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

    EE_BASE_API void MakeTabVisible( char const* const pWindowName );

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
    inline bool BeginViewportPopupModal( char const* pPopupName, bool* pIsPopupOpen, ImVec2 const& size = ImVec2( -1, -1 ), ImGuiCond windowSizeCond = ImGuiCond_Always, ImGuiWindowFlags windowFlags = ( ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        ImGui::OpenPopup( pPopupName );
        ImGui::SetNextWindowSize( size, windowSizeCond );
        ImGui::SetNextWindowViewport( ImGui::GetWindowViewport()->ID );
        return ImGui::BeginPopupModal( pPopupName, pIsPopupOpen, windowFlags );
    }

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
    // Separators
    //-------------------------------------------------------------------------

    // Create a labeled separator: --- TEXT ---------------
    EE_BASE_API void TextSeparator( char const* text, float preWidth = 10.0f, float desiredWidth = 0 );

    // Same as the Imgui::SameLine except it also draws a vertical separator.
    EE_BASE_API void SameLineSeparator( float width = 0, Color const& color = 0 );

    //-------------------------------------------------------------------------
    // Basic Widgets
    //-------------------------------------------------------------------------

    // Draw a tooltip for the immediately preceding item
    EE_BASE_API void ItemTooltip( const char* fmt, ... );

    // Draw a tooltip with a custom hover delay for the immediately preceding item
    EE_BASE_API void ItemTooltipDelayed( float tooltipDelay, const char* fmt, ... );

    // For use with text widget
    EE_BASE_API void TextTooltip( const char* fmt, ... );

    // A smaller check box allowing us to use a larger frame padding value
    EE_BASE_API bool Checkbox( char const* pLabel, bool* pValue );

    // Draw a button with an explicit icon
    EE_BASE_API bool IconButton( char const* pIcon, char const* pLabel, Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false );

    // Draw a colored button
    EE_BASE_API bool ColoredButton( Color const& backgroundColor, Color const& foregroundColor, char const* label, ImVec2 const& size = ImVec2( 0, 0 ) );

    // Draw a colored icon button
    EE_BASE_API bool ColoredIconButton( Color const& backgroundColor, Color const& foregroundColor, Color const& iconColor, char const* pIcon, char const* label, ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false );

    // Draws a flat button - a button with no background
    EE_BASE_API bool FlatButton( char const* label, ImVec2 const& size = ImVec2( 0, 0 ) );

    // Draws a flat button - with a custom text color
    EE_FORCE_INLINE bool FlatButtonColored( Color const& foregroundColor, char const* label, ImVec2 const& size = ImVec2( 0, 0 ) )
    {
        ImGui::PushStyleColor( ImGuiCol_Button, 0 );
        ImGui::PushStyleColor( ImGuiCol_Text, foregroundColor );
        bool const result = ImGui::Button( label, size );
        ImGui::PopStyleColor( 2 );

        return result;
    }

    // Draw a colored icon button
    EE_BASE_API bool FlatIconButton( char const* pIcon, char const* pLabel, Color const& iconColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), ImVec2 const& size = ImVec2( 0, 0 ), bool shouldCenterContents = false );

    // Button with extra drop down options - returns true if the primary button was pressed
    EE_BASE_API bool IconButtonWithDropDown( char const* widgetID, char const* pIcon, char const* pButtonLabel, Color const& iconColor, float buttonWidth, TFunction<void()> const& comboCallback, bool shouldCenterContents = false );

    // Toggle button
    EE_BASE_API bool ToggleButton( char const* pOnLabel, char const* pOffLabel, bool& value, ImVec2 const& size = ImVec2( 0, 0 ), Color const& onColor = ImGuiX::Style::s_colorAccent0, Color const& offColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ) );

    // Toggle button
    EE_BASE_API bool FlatToggleButton( char const* pOnLabel, char const* pOffLabel, bool& value, ImVec2 const& size = ImVec2( 0, 0 ), Color const& onColor = ImGuiX::Style::s_colorAccent0, Color const& offColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ) );

    // Draw an arrow between two points
    EE_BASE_API void DrawArrow( ImDrawList* pDrawList, ImVec2 const& arrowStart, ImVec2 const& arrowEnd, Color const& color, float arrowWidth, float arrowHeadWidth = 5.0f );

    // Draw an overlaid icon in a window, returns true if clicked
    EE_BASE_API bool DrawOverlayIcon( ImVec2 const& iconPos, char icon[4], void* iconID, bool isSelected = false, Color const& selectedColor = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] ) );

    // Draw a basic spinner
    EE_BASE_API bool DrawSpinner( char const* pLabel, Color const& color = ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ), float radius = 6.0f, float thickness = 3.0f );

    // Draw a combo button - wrapper on imgui begin combo, so call end combo as expected
    EE_BASE_API bool BeginComboButton( char const* pLabelAndID, ImGuiComboFlags flags = 0 );

    //-------------------------------------------------------------------------

    EE_BASE_API bool InputFloat2( char const* pID, Float2& value, float width = -1, bool readOnly = false );
    EE_BASE_API bool InputFloat3( char const* pID, Float3& value, float width = -1, bool readOnly = false );
    EE_BASE_API bool InputFloat4( char const* pID, Float4& value, float width = -1, bool readOnly = false );

    EE_BASE_API bool InputTransform( char const* pID, Transform& value, float width = -1, bool readOnly = false );

    //-------------------------------------------------------------------------
    // Images
    //-------------------------------------------------------------------------

    struct ImageInfo
    {
        inline bool IsValid() const { return m_ID != 0; }

    public:

        ImTextureID             m_ID = 0;
        Render::Texture*        m_pTexture = nullptr;
        ImVec2                  m_size = ImVec2( 0, 0 );
    };

    EE_BASE_API ImTextureID ToIm( Render::Texture const& texture );

    EE_BASE_API ImTextureID ToIm( Render::Texture const* pTexture );

    EE_FORCE_INLINE void Image( ImageInfo const& img, ImVec2 const& uv0 = ImVec2( 0, 0 ), ImVec2 const& uv1 = ImVec2( 1, 1 ), Color const& tintColor = Colors::White, Color const& borderColor = Colors::Transparent )
    {
        ImGui::Image( img.m_ID, img.m_size, uv0, uv1, ImGui::ColorConvertU32ToFloat4( tintColor ), ImGui::ColorConvertU32ToFloat4( borderColor ) );
    }

    EE_FORCE_INLINE void ImageButton( char const* pButtonID, ImageInfo const& img, ImVec2 const& uv0 = ImVec2( 0, 0 ), ImVec2 const& uv1 = ImVec2( 1, 1 ), Color const& backgroundColor = Colors::Transparent, Color const& tintColor = Colors::White )
    {
        ImGui::ImageButton( pButtonID, img.m_ID, img.m_size, uv0, uv1, ImGui::ColorConvertU32ToFloat4( backgroundColor ), ImGui::ColorConvertU32ToFloat4( tintColor ) );
    }

    //-------------------------------------------------------------------------
    // Advanced widgets
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
        void SetFilter( String const& filterText );

        // Set the help text shown when we dont have focus and the filter is empty
        void SetFilterHelpText( String const& helpText ) { m_filterHelpText = helpText; }

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

    private:

        void OnBufferUpdated();

    private:

        char                m_buffer[s_bufferSize] = { 0 };
        TVector<String>     m_tokens;
        String              m_filterHelpText = "Filter...";
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