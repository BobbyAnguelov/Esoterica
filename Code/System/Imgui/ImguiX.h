#pragma once

#include "System/_Module/API.h"
#include "ImguiFont.h"
#include "ImguiStyle.h"
#include "System/ThirdParty/imgui/imgui_internal.h"
#include "System/Math/Transform.h"
#include "System/Math/Rectangle.h"
#include "System/Types/Color.h"
#include "System/Types/String.h"
#include "System/Types/BitFlags.h"
#include "System/Types/Function.h"

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
    // Color Helpers
    //-------------------------------------------------------------------------

    // Convert EE color to ImColor
    EE_FORCE_INLINE ImColor ToIm( Color const& color )
    {
        return IM_COL32( color.m_byteColor.m_r, color.m_byteColor.m_g, color.m_byteColor.m_b, color.m_byteColor.m_a );
    }

    // Convert ImColor to EE color
    EE_FORCE_INLINE Color FromIm( ImColor const& color )
    {
        return Color( uint8_t( color.Value.x * 255 ), uint8_t( color.Value.y * 255 ), uint8_t( color.Value.z * 255 ), uint8_t( color.Value.w * 255 ) );
    }

    // Adjust the brightness of color by a multiplier
    EE_FORCE_INLINE ImColor AdjustColorBrightness( ImColor const& color, float multiplier )
    {
        Float4 tmpColor = (ImVec4) color;
        float const alpha = tmpColor.m_w;
        tmpColor *= multiplier;
        tmpColor.m_w = alpha;
        return ImColor( tmpColor );
    }

    // Adjust the brightness of color by a multiplier
    EE_FORCE_INLINE ImU32 AdjustColorBrightness( ImU32 color, float multiplier )
    {
        return AdjustColorBrightness( ImColor( color ), multiplier );
    }

    //-------------------------------------------------------------------------
    // General helpers
    //-------------------------------------------------------------------------

    EE_SYSTEM_API void MakeTabVisible( char const* const pWindowName );

    EE_SYSTEM_API ImVec2 ClampToRect( ImRect const& rect, ImVec2 const& inPoint );

    // Returns the closest point on the rect border to the specified point
    EE_SYSTEM_API ImVec2 GetClosestPointOnRectBorder( ImRect const& rect, ImVec2 const& inPoint );

    // Is this a valid name ID character (i.e. A-Z, a-z, 0-9, _ )
    inline bool IsValidNameIDChar( ImWchar c )
    {
        return isalnum( c ) || c == '_';
    }

    inline int FilterNameIDChars( ImGuiInputTextCallbackData* data )
    {
        if ( IsValidNameIDChar( data->EventChar ) )
        {
            return 0;
        }
        return 1;
    }

    //-------------------------------------------------------------------------
    // Separators
    //-------------------------------------------------------------------------

    // Create a centered separator which can be immediately followed by a item
    EE_SYSTEM_API void PreSeparator( float width = 0 );

    // Create a centered separator which can be immediately followed by a item
    EE_SYSTEM_API void PostSeparator( float width = 0 );

    // Create a labeled separator: --- TEXT ---------------
    EE_SYSTEM_API void TextSeparator( char const* text, float preWidth = 10.0f, float totalWidth = 0 );

    // Draws a vertical separator on the current line and forces the next item to be on the same line. The size is the offset between the previous item and the next
    EE_SYSTEM_API void VerticalSeparator( ImVec2 const& size = ImVec2( -1, -1 ), ImColor const& color = 0 );

    //-------------------------------------------------------------------------
    // Basic Widgets
    //-------------------------------------------------------------------------

    // Draw a tooltip for the immediately preceding item
    EE_SYSTEM_API void ItemTooltip( const char* fmt, ... );

    // Draw a tooltip with a custom hover delay for the immediately preceding item
    EE_SYSTEM_API void ItemTooltipDelayed( float tooltipDelay, const char* fmt, ... );

    // For use with text widget
    EE_SYSTEM_API void TextTooltip( const char* fmt, ... );

    // Draw a button with an explicit icon
    EE_SYSTEM_API bool IconButton( char const* pIcon, char const* pLabel, ImColor const& iconColor = ImGui::GetStyle().Colors[ImGuiCol_Text], ImVec2 const& size = ImVec2( 0, 0 ) );

    // Draw a colored button
    EE_SYSTEM_API bool ColoredButton( ImColor const& backgroundColor, ImColor const& foregroundColor, char const* label, ImVec2 const& size = ImVec2( 0, 0 ) );

    // Draw a colored icon button
    EE_SYSTEM_API bool ColoredIconButton( ImColor const& backgroundColor, ImColor const& foregroundColor, ImColor const& iconColor, char const* pIcon, char const* label, ImVec2 const& size = ImVec2( 0, 0 ) );

    // Draws a flat button - a button with no background
    EE_SYSTEM_API bool FlatButton( char const* label, ImVec2 const& size = ImVec2( 0, 0 ) );

    // Draws a flat button - with a custom text color
    EE_FORCE_INLINE bool FlatButtonColored( ImColor const& foregroundColor, char const* label, ImVec2 const& size = ImVec2( 0, 0 ) )
    {
        ImGui::PushStyleColor( ImGuiCol_Button, 0 );
        ImGui::PushStyleColor( ImGuiCol_Text, foregroundColor.Value );
        bool const result = ImGui::Button( label, size );
        ImGui::PopStyleColor( 2 );

        return result;
    }

    // Draw a colored icon button
    EE_SYSTEM_API bool FlatIconButton( char const* pIcon, char const* pLabel, ImColor const& iconColor = ImGui::GetStyle().Colors[ImGuiCol_Text], ImVec2 const& size = ImVec2( 0, 0 ) );

    // Combo button - button with extra drop down options - returns true if the primary button was pressed
    EE_SYSTEM_API bool ComboButton( char const* pButtonLabel, char const* comboID, float buttonWidth, TFunction<void()>&& comboCallback );

    // Combo button - button with extra drop down options - returns true if the primary button was pressed
    EE_SYSTEM_API bool IconComboButton( char const* pIcon, char const* pButtonLabel, ImColor const& iconColor, char const* comboID, float buttonWidth, TFunction<void()>&& comboCallback );

    // Toggle button
    EE_SYSTEM_API bool ToggleButton( char const* pOnLabel, char const* pOffLabel, bool& value, ImVec2 const& size = ImVec2( 0, 0 ), ImColor const& onColor = ImGuiX::Style::s_colorAccent0, ImColor const& offColor = ImGui::GetStyle().Colors[ImGuiCol_Text] );

    // Draw an arrow between two points
    EE_SYSTEM_API void DrawArrow( ImDrawList* pDrawList, ImVec2 const& arrowStart, ImVec2 const& arrowEnd, ImColor const& color, float arrowWidth, float arrowHeadWidth = 5.0f );

    // Draw an overlaid icon in a window, returns true if clicked
    EE_SYSTEM_API bool DrawOverlayIcon( ImVec2 const& iconPos, char icon[4], void* iconID, bool isSelected = false, ImColor const& selectedColor = ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] );

    // Draw a basic spinner
    EE_SYSTEM_API bool DrawSpinner( char const* pLabel, ImColor const& color = ImGui::GetStyle().Colors[ImGuiCol_Text], float radius = 6.0f, float thickness = 3.0f );

    //-------------------------------------------------------------------------

    EE_SYSTEM_API bool InputFloat2( char const* pID, Float2& value, float width = -1, bool readOnly = false );
    EE_SYSTEM_API bool InputFloat3( char const* pID, Float3& value, float width = -1, bool readOnly = false );
    EE_SYSTEM_API bool InputFloat4( char const* pID, Float4& value, float width = -1, bool readOnly = false );
    EE_SYSTEM_API bool InputFloat4( char const* pID, Vector& value, float width = -1, bool readOnly = false );

    EE_SYSTEM_API bool InputTransform( char const* pID, Transform& value, float width = -1, bool readOnly = false );

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

    EE_SYSTEM_API ImTextureID ToIm( Render::Texture const& texture );

    EE_SYSTEM_API ImTextureID ToIm( Render::Texture const* pTexture );

    EE_FORCE_INLINE void Image( ImageInfo const& img, ImVec2 const& uv0 = ImVec2( 0, 0 ), ImVec2 const& uv1 = ImVec2( 1, 1 ), ImColor const& tintColor = ImVec4( 1, 1, 1, 1 ), ImColor const& borderColor = ImVec4( 0, 0, 0, 0 ) )
    {
        ImGui::Image( img.m_ID, img.m_size, uv0, uv1, tintColor, borderColor );
    }

    EE_FORCE_INLINE void ImageButton( char const* pButtonID, ImageInfo const& img, ImVec2 const& uv0 = ImVec2( 0, 0 ), ImVec2 const& uv1 = ImVec2( 1, 1 ), ImColor const& backgroundColor = ImVec4( 0, 0, 0, 0 ), ImColor const& tintColor = ImVec4( 1, 1, 1, 1 ) )
    {
        ImGui::ImageButton( pButtonID, img.m_ID, img.m_size, uv0, uv1, backgroundColor, tintColor );
    }

    //-------------------------------------------------------------------------
    // Advanced widgets
    //-------------------------------------------------------------------------

    // A simple filter entry widget that allows you to string match to some entered text
    class EE_SYSTEM_API FilterWidget
    {
        constexpr static uint32_t const s_bufferSize = 255;

    public:

        enum Options : uint8_t
        {
            TakeInitialFocus = 0
        };

    public:

        // Draws the filter. Returns true if the filter has been updated
        bool DrawAndUpdate( TBitFlags<Options> options = TBitFlags<Options>() );

        inline void Clear();

        inline bool HasFilterSet() const { return !m_tokens.empty(); }

        inline TVector<String> const& GetFilterTokens() const { return m_tokens; }

        // Does a provided string match the current filter - the string copy is intentional!
        bool MatchesFilter( String string );

    private:

        char                m_buffer[s_bufferSize] = { 0 };
        TVector<String>     m_tokens;
    };

    // A simple 3D gizmo to show the orientation of a camera in a scene
    struct EE_SYSTEM_API OrientationGuide
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

    struct EE_SYSTEM_API ApplicationTitleBar
    {
        static Float2 const s_windowControlButtonSize;
        constexpr static float const s_minimumDraggableGap = 20;

        static inline float GetWindowsControlsWidth() { return s_windowControlButtonSize.m_x * 3; }
        static void DrawWindowControls();

    public:

        // This function takes three delegates and sizes each representing an area of the title bar to draw to.
        void Draw( TFunction<void()>&& leftSectionDrawFunction = TFunction<void()>(), float leftSectionWidth = 0, TFunction<void()>&& midSectionDrawFunction = TFunction<void()>(), float midSectionWidth = 0, TFunction<void()>&& rightSectionDrawFunction = TFunction<void()>(), float rightSectionWidth = 0 );

        // Get the screen space rectangle for this title bar
        Math::ScreenSpaceRectangle const& GetScreenRectangle() const { return m_rect; }

    private:

        Math::ScreenSpaceRectangle  m_rect;
    };
}
#endif