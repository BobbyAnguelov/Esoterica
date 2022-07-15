#pragma once

#include "System/Math/Math.h"
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
    // It assumes little endian systems and a uint32_t format as follows: 0xRRGGBBAA
    // If you need a different uint32_t format, there are conversion functions provided

    struct Color
    {
        EE_SERIALIZE( m_color );

        union
        {
            struct ByteColor
            {
                uint8_t       m_a;
                uint8_t       m_b;
                uint8_t       m_g;
                uint8_t       m_r;
            };

            ByteColor       m_byteColor;
            uint32_t          m_color;
        };

        // Default color is white
        inline Color() : m_color( 0xFFFFFFFF ) {}

        // The format is as follows: 0xRRGGBBAA
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

        inline Color GetAlphaVersion( uint8_t newAlpha ) const
        {
            Color newColor = *this;
            newColor.m_byteColor.m_a = newAlpha;
            return newColor;
        }

        inline Color GetAlphaVersion( float alpha ) const
        {
            float const floatAlpha = Math::Clamp( alpha * 255, 0.0f, 255.0f );
            return GetAlphaVersion( (uint8_t) floatAlpha );
        }

        //-------------------------------------------------------------------------

        // Returns a uint32_t color with this byte format: 0xRRGGBBAA
        inline uint32_t ToUInt32() const { return m_color; }
        
        // Returns a uint32_t color with this byte format: 0xAABBGGRR
        inline uint32_t ToUInt32_ABGR() const
        {
            uint32_t outColor = 0;
            outColor |= m_byteColor.m_r;
            outColor |= m_byteColor.m_g << 8;
            outColor |= m_byteColor.m_b << 16;
            outColor |= m_byteColor.m_a << 24;
            return outColor; 
        }

        // Returns a uint32_t color with this byte format: 0xRRGGBBAA
        inline operator uint32_t() const { return ToUInt32(); }

        //-------------------------------------------------------------------------

        // Returns a float4 color where x=R, y=G, z=B, w=A
        inline Float4 ToFloat4() const { return Float4( (float) m_byteColor.m_r / 255, (float) m_byteColor.m_g / 255, (float) m_byteColor.m_b / 255, (float) m_byteColor.m_a / 255 ); }

        // Returns a float4 color where x=A, y=B, z=G, w=R
        inline Float4 ToFloat4_ABGR() const { return Float4( (float) m_byteColor.m_a / 255, (float) m_byteColor.m_b / 255, (float) m_byteColor.m_g / 255, (float) m_byteColor.m_r / 255 ); }

        // Returns a float4 color where x=R, y=G, z=B, w=A
        inline operator Float4() const { return ToFloat4(); }
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

namespace EE
{
    namespace Colors
    {
        static Color const AliceBlue = 0xF0F8FFFF;
        static Color const AntiqueWhite = 0xFAEBD7FF;
        static Color const Aqua = 0x00FFFFFF;
        static Color const Aquamarine = 0x7FFFD4FF;
        static Color const Azure = 0xF0FFFFFF;
        static Color const Beige = 0xF5F5DCFF;
        static Color const Bisque = 0xFFE4C4FF;
        static Color const Black = 0x000000FF;
        static Color const BlanchedAlmond = 0xFFEBCDFF;
        static Color const Blue = 0x0000FFFF;
        static Color const BlueViolet = 0x8A2BE2FF;
        static Color const Brown = 0xA52A2AFF;
        static Color const BurlyWood = 0xDEB887FF;
        static Color const CadetBlue = 0x5F9EA0FF;
        static Color const Chartreuse = 0x7FFF00FF;
        static Color const Chocolate = 0xD2691EFF;
        static Color const Coral = 0xFF7F50FF;
        static Color const CornflowerBlue = 0x6495EDFF;
        static Color const Cornsilk = 0xFFF8DCFF;
        static Color const Crimson = 0xDC143CFF;
        static Color const Cyan = 0x00FFFFFF;
        static Color const DarkBlue = 0x00008BFF;
        static Color const DarkCyan = 0x008B8BFF;
        static Color const DarkGoldenRod = 0xB8860BFF;
        static Color const DarkGray = 0xA9A9A9FF;
        static Color const DarkGreen = 0x006400FF;
        static Color const DarkKhaki = 0xBDB76BFF;
        static Color const DarkMagenta = 0x8B008BFF;
        static Color const DarkOliveGreen = 0x556B2FFF;
        static Color const DarkOrange = 0xFF8C00FF;
        static Color const DarkOrchid = 0x9932CCFF;
        static Color const DarkRed = 0x8B0000FF;
        static Color const DarkSalmon = 0xE9967AFF;
        static Color const DarkSeaGreen = 0x8FBC8FFF;
        static Color const DarkSlateBlue = 0x483D8BFF;
        static Color const DarkSlateGray = 0x2F4F4FFF;
        static Color const DarkTurquoise = 0x00CED1FF;
        static Color const DarkViolet = 0x9400D3FF;
        static Color const DeepPink = 0xFF1493FF;
        static Color const DeepSkyBlue = 0x00BFFFFF;
        static Color const DimGray = 0x696969FF;
        static Color const DodgerBlue = 0x1E90FFFF;
        static Color const FireBrick = 0xB22222FF;
        static Color const FloralWhite = 0xFFFAF0FF;
        static Color const ForestGreen = 0x228B22FF;
        static Color const Fuchsia = 0xFF00FFFF;
        static Color const Gainsboro = 0xDCDCDCFF;
        static Color const GhostWhite = 0xF8F8FFFF;
        static Color const Gold = 0xFFD700FF;
        static Color const GoldenRod = 0xDAA520FF;
        static Color const Gray = 0x808080FF;
        static Color const Green = 0x008000FF;
        static Color const GreenYellow = 0xADFF2FFF;
        static Color const HoneyDew = 0xF0FFF0FF;
        static Color const HotPink = 0xFF69B4FF;
        static Color const IndianRed  = 0xCD5C5CFF;
        static Color const Indigo  = 0x4B0082FF;
        static Color const Ivory = 0xFFFFF0FF;
        static Color const Khaki = 0xF0E68CFF;
        static Color const Lavender = 0xE6E6FAFF;
        static Color const LavenderBlush = 0xFFF0F5FF;
        static Color const LawnGreen = 0x7CFC00FF;
        static Color const LemonChiffon = 0xFFFACDFF;
        static Color const LightBlue = 0xADD8E6FF;
        static Color const LightCoral = 0xF08080FF;
        static Color const LightCyan = 0xE0FFFFFF;
        static Color const LightGoldenRodYellow = 0xFAFAD2FF;
        static Color const LightGray = 0xD3D3D3FF;
        static Color const LightGreen = 0x90EE90FF;
        static Color const LightPink = 0xFFB6C1FF;
        static Color const LightSalmon = 0xFFA07AFF;
        static Color const LightSeaGreen = 0x20B2AAFF;
        static Color const LightSkyBlue = 0x87CEFAFF;
        static Color const LightSlateGray = 0x778899FF;
        static Color const LightSteelBlue = 0xB0C4DEFF;
        static Color const LightYellow = 0xFFFFE0FF;
        static Color const Lime = 0x00FF00FF;
        static Color const LimeGreen = 0x32CD32FF;
        static Color const Linen = 0xFAF0E6FF;
        static Color const Magenta = 0xFF00FFFF;
        static Color const Maroon = 0x800000FF;
        static Color const MediumAquaMarine = 0x66CDAAFF;
        static Color const MediumBlue = 0x0000CDFF;
        static Color const MediumOrchid = 0xBA55D3FF;
        static Color const MediumPurple = 0x9370DBFF;
        static Color const MediumSeaGreen = 0x3CB371FF;
        static Color const MediumSlateBlue = 0x7B68EEFF;
        static Color const MediumSpringGreen = 0x00FA9AFF;
        static Color const MediumTurquoise = 0x48D1CCFF;
        static Color const MediumVioletRed = 0xC71585FF;
        static Color const MidnightBlue = 0x191970FF;
        static Color const MintCream = 0xF5FFFAFF;
        static Color const MistyRose = 0xFFE4E1FF;
        static Color const Moccasin = 0xFFE4B5FF;
        static Color const NavajoWhite = 0xFFDEADFF;
        static Color const Navy = 0x000080FF;
        static Color const OldLace = 0xFDF5E6FF;
        static Color const Olive = 0x808000FF;
        static Color const OliveDrab = 0x6B8E23FF;
        static Color const Orange = 0xFFA500FF;
        static Color const OrangeRed = 0xFF4500FF;
        static Color const Orchid = 0xDA70D6FF;
        static Color const PaleGoldenRod = 0xEEE8AAFF;
        static Color const PaleGreen = 0x98FB98FF;
        static Color const PaleTurquoise = 0xAFEEEEFF;
        static Color const PaleVioletRed = 0xDB7093FF;
        static Color const PapayaWhip = 0xFFEFD5FF;
        static Color const PeachPuff = 0xFFDAB9FF;
        static Color const Peru = 0xCD853FFF;
        static Color const Pink = 0xFFC0CBFF;
        static Color const Plum = 0xDDA0DDFF;
        static Color const PowderBlue = 0xB0E0E6FF;
        static Color const Purple = 0x800080FF;
        static Color const Red = 0xFF0000FF;
        static Color const MediumRed = 0xC90000FF;
        static Color const RosyBrown = 0xBC8F8FFF;
        static Color const RoyalBlue = 0x4169E1FF;
        static Color const SaddleBrown = 0x8B4513FF;
        static Color const Salmon = 0xFA8072FF;
        static Color const SandyBrown = 0xF4A460FF;
        static Color const SeaGreen = 0x2E8B57FF;
        static Color const SeaShell = 0xFFF5EEFF;
        static Color const Sienna = 0xA0522DFF;
        static Color const Silver = 0xC0C0C0FF;
        static Color const SkyBlue = 0x87CEEBFF;
        static Color const SlateBlue = 0x6A5ACDFF;
        static Color const SlateGray = 0x708090FF;
        static Color const Snow = 0xFFFAFAFF;
        static Color const SpringGreen = 0x00FF7FFF;
        static Color const SteelBlue = 0x4682B4FF;
        static Color const Tan = 0xD2B48CFF;
        static Color const Teal = 0x008080FF;
        static Color const Thistle = 0xD8BFD8FF;
        static Color const Tomato = 0xFF6347FF;
        static Color const Turquoise = 0x40E0D0FF;
        static Color const Violet = 0xEE82EEFF;
        static Color const Wheat = 0xF5DEB3FF;
        static Color const White = 0xFFFFFFFF;
        static Color const WhiteSmoke = 0xF5F5F5FF;
        static Color const Yellow = 0xFFFF00FF;
        static Color const YellowGreen = 0x9ACD32FF;
    }
}