#pragma once

#include "Base/Math/Math.h"
#include "EASTL/functional.h"

//-------------------------------------------------------------------------

#ifdef RGB 
    #undef RGB
#endif

//-------------------------------------------------------------------------

namespace EE
{
    //-------------------------------------------------------------------------
    //  Color Abstraction
    //-------------------------------------------------------------------------
    // This is a simply helper to manage colors and the various formats
    // It assumes little endian systems and a uint32_t format as follows: 0xAABBGGRR
    // If you need a different uint32_t format, there are conversion functions provided

    struct EE_BASE_API Color
    {
        EE_SERIALIZE( m_color );

    public:

        // Blend between two colors, blend weight will be clamped to [0, 1]
        static Color Blend( Color const& c0, Color const& c1, float blendWeight );

        // Evaluate a gradient from red = 0 and green = 1
        static Color EvaluateRedGreenGradient( float weight, bool useDistinctColorForZero = true );

        // Evaluate a gradient from blue = 0 and red = 1
        static Color EvaluateBlueRedGradient( float weight, bool useDistinctColorForZero = true );

        // Evaluate a gradient from yellow = 0 and red = 1
        static Color EvaluateYellowRedGradient( float weight, bool useDistinctColorForZero = true );

    public:

        // Default color is transparent
        inline Color() : m_color( 0x00000000 ) {}

        // The format is as follows: 0xAAGGBBRR
        inline Color( uint32_t c ) : m_color( c ) {}

        inline Color( uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255 )
        {
            m_byteColor.m_r = r;
            m_byteColor.m_g = g;
            m_byteColor.m_b = b;
            m_byteColor.m_a = a;
        }

        inline Color( Float4 const& c )
        {
            m_byteColor.m_r = uint8_t( c[0] * 255 );
            m_byteColor.m_g = uint8_t( c[1] * 255 );
            m_byteColor.m_b = uint8_t( c[2] * 255 );
            m_byteColor.m_a = uint8_t( c[3] * 255 );
        }

        //-------------------------------------------------------------------------

        // Get a version with the a different alpha value
        inline Color GetAlphaVersion( uint8_t newAlpha ) const
        {
            Color newColor = *this;
            newColor.m_byteColor.m_a = newAlpha;
            return newColor;
        }

        // Get a version with the a different alpha value
        inline Color GetAlphaVersion( float alpha ) const
        {
            float const floatAlpha = Math::Clamp( alpha * 255, 0.0f, 255.0f );
            return GetAlphaVersion( (uint8_t) floatAlpha );
        }

        // Scale the color values with a multiplier
        inline void ScaleColor( float multiplier )
        {
            m_byteColor.m_r = (uint8_t) Math::Clamp( Math::RoundToInt( float( m_byteColor.m_r ) * multiplier ), 0, 255 );
            m_byteColor.m_g = (uint8_t) Math::Clamp( Math::RoundToInt( float( m_byteColor.m_g ) * multiplier ), 0, 255 );
            m_byteColor.m_b = (uint8_t) Math::Clamp( Math::RoundToInt( float( m_byteColor.m_b ) * multiplier ), 0, 255 );
        }

        // Get a version with color values scaled
        inline Color GetScaledColor( float multiplier ) const
        {
            Color c = *this;
            c.ScaleColor( multiplier );
            return c;
        }

        // Equality
        inline bool operator==( Color const& rhs ) const { return m_color == rhs.m_color; }
        inline bool operator!=( Color const& rhs ) const { return m_color != rhs.m_color; }

        //-------------------------------------------------------------------------

        // Returns a uint32_t color with this byte format: 0xAAGGBBRR
        EE_FORCE_INLINE uint32_t ToUInt32() const { return m_color; }

        // Returns a uint32_t color with this byte format: 0xAAGGBBRR
        EE_FORCE_INLINE operator uint32_t() const { return ToUInt32(); }

        //-------------------------------------------------------------------------

        // Returns a float4 color where x=R, y=G, z=B, w=A
        EE_FORCE_INLINE Float4 ToFloat4() const { return Float4( (float) m_byteColor.m_r / 255, (float) m_byteColor.m_g / 255, (float) m_byteColor.m_b / 255, (float) m_byteColor.m_a / 255 ); }

        // Returns a float4 color where x=R, y=G, z=B, w=A
        EE_FORCE_INLINE operator Float4() const { return ToFloat4(); }

    public:

        union
        {
            struct ByteColor
            {
                uint8_t         m_r;
                uint8_t         m_g;
                uint8_t         m_b;
                uint8_t         m_a;
            };

            ByteColor           m_byteColor;
            uint32_t            m_color;
        };
    };
}

//-------------------------------------------------------------------------

namespace eastl
{
    template <>
    struct hash<EE::Color>
    {
        eastl_size_t operator()( EE::Color const& color ) const
        {
            return color.m_color;
        }
    };
}

//-------------------------------------------------------------------------

template<class Archive> void serialize( Archive& archive, EE::Color& value )
{
    archive( value.m_color );
}

//-------------------------------------------------------------------------
// Predefined list of X11 Colors

namespace EE::Colors
{
    static Color const Transparent          = 0x00000000;
    static Color const AliceBlue            = 0xFFFFF8F0;
    static Color const AntiqueWhite         = 0xFFD7EBFA;
    static Color const Aqua                 = 0xFFFFFF00;
    static Color const Aquamarine           = 0xFFD4FF7F;
    static Color const Azure                = 0xFFFFFFF0;
    static Color const Beige                = 0xFFDCF5F5;
    static Color const Bisque               = 0xFFC4E4FF;
    static Color const Black                = 0xFF000000;
    static Color const BlanchedAlmond       = 0xFFCDEBFF;
    static Color const Blue                 = 0xFFFF0000;
    static Color const BlueViolet           = 0xFFE22B8A;
    static Color const Brown                = 0xFF2A2AA5;
    static Color const BurlyWood            = 0xFF87B8DE;
    static Color const CadetBlue            = 0xFFA09E5F;
    static Color const Chartreuse           = 0xFF00FF7F;
    static Color const Chocolate            = 0xFF1E69D2;
    static Color const Coral                = 0xFF507FFF;
    static Color const CornflowerBlue       = 0xFFED9564;
    static Color const Cornsilk             = 0xFFDCF8FF;
    static Color const Crimson              = 0xFF3C14DC;
    static Color const Cyan                 = 0xFFFFFF00;
    static Color const DarkBlue             = 0xFF8B0000;
    static Color const DarkCyan             = 0xFF8B8B00;
    static Color const DarkGoldenRod        = 0xFF0B86B8;
    static Color const DarkGray             = 0xFFA9A9A9;
    static Color const DarkGreen            = 0xFF006400;
    static Color const DarkKhaki            = 0xFF6BB7BD;
    static Color const DarkMagenta          = 0xFF8B008B;
    static Color const DarkOliveGreen       = 0xFF2F6B55;
    static Color const DarkOrange           = 0xFF008CFF;
    static Color const DarkOrchid           = 0xFFCC3299;
    static Color const DarkRed              = 0xFF00008B;
    static Color const DarkSalmon           = 0xFF7A96E9;
    static Color const DarkSeaGreen         = 0xFF8FBC8F;
    static Color const DarkSlateBlue        = 0xFF8B3D48;
    static Color const DarkSlateGray        = 0xFF4F4F2F;
    static Color const DarkTurquoise        = 0xFFD1CE00;
    static Color const DarkViolet           = 0xFFD30094;
    static Color const DeepPink             = 0xFF9314FF;
    static Color const DeepSkyBlue          = 0xFFFFBF00;
    static Color const DimGray              = 0xFF696969;
    static Color const DodgerBlue           = 0xFFFF901E;
    static Color const FireBrick            = 0xFF2222B2;
    static Color const FloralWhite          = 0xFFF0FAFF;
    static Color const ForestGreen          = 0xFF228B22;
    static Color const Fuchsia              = 0xFFFF00FF;
    static Color const Gainsboro            = 0xFFDCDCDC;
    static Color const GhostWhite           = 0xFFFFF8F8;
    static Color const Gold                 = 0xFF00D7FF;
    static Color const GoldenRod            = 0xFF20A5DA;
    static Color const Gray                 = 0xFF808080;
    static Color const Green                = 0xFF008000;
    static Color const GreenYellow          = 0xFF2FFFAD;
    static Color const HoneyDew             = 0xFFF0FFF0;
    static Color const HotPink              = 0xFFB469FF;
    static Color const IndianRed            = 0xFF5C5CCD;
    static Color const Indigo               = 0xFF82004B;
    static Color const Ivory                = 0xFFF0FFFF;
    static Color const Khaki                = 0xFF8CE6F0;
    static Color const Lavender             = 0xFFFAE6E6;
    static Color const LavenderBlush        = 0xFFF5F0FF;
    static Color const LawnGreen            = 0xFF00FC7C;
    static Color const LemonChiffon         = 0xFFCDFAFF;
    static Color const LightBlue            = 0xFFE6D8AD;
    static Color const LightCoral           = 0xFF8080F0;
    static Color const LightCyan            = 0xFFFFFFE0;
    static Color const LightGoldenRodYellow = 0xFFD2FAFA;
    static Color const LightGray            = 0xFFD3D3D3;
    static Color const LightGreen           = 0xFF90EE90;
    static Color const LightPink            = 0xFFC1B6FF;
    static Color const LightSalmon          = 0xFF7AA0FF;
    static Color const LightSeaGreen        = 0xFFAAB220;
    static Color const LightSkyBlue         = 0xFFFACE87;
    static Color const LightSlateGray       = 0xFF998877;
    static Color const LightSteelBlue       = 0xFFDEC4B0;
    static Color const LightYellow          = 0xFFE0FFFF;
    static Color const Lime                 = 0xFF00FF00;
    static Color const LimeGreen            = 0xFF32CD32;
    static Color const Linen                = 0xFFE6F0FA;
    static Color const Magenta              = 0xFFFF00FF;
    static Color const Maroon               = 0xFF000080;
    static Color const MediumAquaMarine     = 0xFFAACD66;
    static Color const MediumBlue           = 0xFFCD0000;
    static Color const MediumOrchid         = 0xFFD355BA;
    static Color const MediumPurple         = 0xFFDB7093;
    static Color const MediumSeaGreen       = 0xFF71B33C;
    static Color const MediumSlateBlue      = 0xFFEE687B;
    static Color const MediumSpringGreen    = 0xFF9AFA00;
    static Color const MediumTurquoise      = 0xFFCCD148;
    static Color const MediumVioletRed      = 0xFF8515C7;
    static Color const MidnightBlue         = 0xFF701919;
    static Color const MintCream            = 0xFFFAFFF5;
    static Color const MistyRose            = 0xFFE1E4FF;
    static Color const Moccasin             = 0xFFB5E4FF;
    static Color const NavajoWhite          = 0xFFADDEFF;
    static Color const Navy                 = 0xFF800000;
    static Color const OldLace              = 0xFFE6F5FD;
    static Color const Olive                = 0xFF008080;
    static Color const OliveDrab            = 0xFF238E6B;
    static Color const Orange               = 0xFF00A5FF;
    static Color const OrangeRed            = 0xFF0045FF;
    static Color const Orchid               = 0xFFD670DA;
    static Color const PaleGoldenRod        = 0xFFAAE8EE;
    static Color const PaleGreen            = 0xFF98FB98;
    static Color const PaleTurquoise        = 0xFFEEEEAF;
    static Color const PaleVioletRed        = 0xFF9370DB;
    static Color const PapayaWhip           = 0xFFD5EFFF;
    static Color const PeachPuff            = 0xFFB9DAFF;
    static Color const Peru                 = 0xFF3F85CD;
    static Color const Pink                 = 0xFFCBC0FF;
    static Color const Plum                 = 0xFFDDA0DD;
    static Color const PowderBlue           = 0xFFE6E0B0;
    static Color const Purple               = 0xFF800080;
    static Color const Red                  = 0xFF0000FF;
    static Color const MediumRed            = 0xFF0000C9;
    static Color const RosyBrown            = 0xFF8F8FBC;
    static Color const RoyalBlue            = 0xFFE16941;
    static Color const SaddleBrown          = 0xFF13458B;
    static Color const Salmon               = 0xFF7280FA;
    static Color const SandyBrown           = 0xFF60A4F4;
    static Color const SeaGreen             = 0xFF578B2E;
    static Color const SeaShell             = 0xFFEEF5FF;
    static Color const Sienna               = 0xFF2D52A0;
    static Color const Silver               = 0xFFC0C0C0;
    static Color const SkyBlue              = 0xFFEBCE87;
    static Color const SlateBlue            = 0xFFCD5A6A;
    static Color const SlateGray            = 0xFF908070;
    static Color const Snow                 = 0xFFFAFAFF;
    static Color const SpringGreen          = 0xFF7FFF00;
    static Color const SteelBlue            = 0xFFB48246;
    static Color const Tan                  = 0xFF8CB4D2;
    static Color const Teal                 = 0xFF808000;
    static Color const Thistle              = 0xFFD8BFD8;
    static Color const Tomato               = 0xFF4763FF;
    static Color const Turquoise            = 0xFFD0E040;
    static Color const Violet               = 0xFFEE82EE;
    static Color const Wheat                = 0xFFB3DEF5;
    static Color const White                = 0xFFFFFFFF;
    static Color const WhiteSmoke           = 0xFFF5F5F5;
    static Color const Yellow               = 0xFF00FFFF;
    static Color const YellowGreen          = 0xFF32CD9A;
}