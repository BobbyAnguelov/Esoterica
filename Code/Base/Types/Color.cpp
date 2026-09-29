#include "Color.h"
#include "Base/Math/Lerp.h"

//-------------------------------------------------------------------------

namespace EE
{
    Color Color::Blend( Color const& c0, Color const& c1, float blendWeight )
    {
        blendWeight = Math::Clamp( blendWeight, 0.0f, 1.0f );

        float const lerpedR = Math::Lerp( float( c0.m_byteColor.m_r ), float( c1.m_byteColor.m_r ), blendWeight );
        float const lerpedG = Math::Lerp( float( c0.m_byteColor.m_g ), float( c1.m_byteColor.m_g ), blendWeight );
        float const lerpedB = Math::Lerp( float( c0.m_byteColor.m_b ), float( c1.m_byteColor.m_b ), blendWeight );
        float const lerpedA = Math::Lerp( float( c0.m_byteColor.m_a ), float( c1.m_byteColor.m_a ), blendWeight );

        Color result;
        result.m_byteColor.m_r = (uint8_t) Math::Clamp( Math::RoundToInt32( lerpedR ), 0, 255 );
        result.m_byteColor.m_g = (uint8_t) Math::Clamp( Math::RoundToInt32( lerpedG ), 0, 255 );
        result.m_byteColor.m_b = (uint8_t) Math::Clamp( Math::RoundToInt32( lerpedB ), 0, 255 );
        result.m_byteColor.m_a = (uint8_t) Math::Clamp( Math::RoundToInt32( lerpedA ), 0, 255 );
        return result;
    }

    //-------------------------------------------------------------------------

    constexpr static int32_t const g_colorGradientPoints = 7;
    constexpr static int32_t const g_colorGradientIntervals = g_colorGradientPoints - 1;

    inline static Color EvaluateGradient( Color const gradientColors[], float weight, bool useDistinctColorForZero )
    {
        weight = Math::Clamp( weight, 0.0f, 1.0f );

        // 0%
        //-------------------------------------------------------------------------

        if ( weight <= 0.0f )
        {
            return useDistinctColorForZero ? Colors::Gray : gradientColors[0];
        }

        // 100%
        //-------------------------------------------------------------------------

        if ( weight == 1.0f )
        {
            return gradientColors[g_colorGradientIntervals];
        }

        // Blend
        //-------------------------------------------------------------------------

        float interval;
        float const blendWeight = Math::ModF( weight * g_colorGradientIntervals, interval );
        int32_t const intervalIndex = Math::FloorToInt32( interval );
        return Color::Blend( gradientColors[intervalIndex], gradientColors[intervalIndex + 1], blendWeight );
    }

    //-------------------------------------------------------------------------

    static Color const g_redGreenGradient[g_colorGradientPoints] =
    {
        Color( 0xFF0D0DFF ),
        Color( 0xFF0053EF ),
        Color( 0xFF0078D8 ),
        Color( 0xFF0094BC ),
        Color( 0xFF00AA9B ),
        Color( 0xFF00BD73 ),
        Color( 0xFF32CD32 )
    };

    Color Color::EvaluateRedGreenGradient( float weight, bool useDistinctColorForZero )
    {
        return EvaluateGradient( g_redGreenGradient, weight, useDistinctColorForZero );
    }

    static Color const g_blueRedGradient[g_colorGradientPoints] =
    {
        Color( 0xFF93342F ),
        Color( 0xFF8E2865 ),
        Color( 0xFF82118B ),
        Color( 0xFF6F00A8 ),
        Color( 0xFF5800BD ),
        Color( 0xFF3F00CA ),
        Color( 0xFF221CCE )
    };

    Color Color::EvaluateBlueRedGradient( float weight, bool useDistinctColorForZero )
    {
        return EvaluateGradient( g_blueRedGradient, weight, useDistinctColorForZero );
    }

    static Color const g_yellowRedGradient[g_colorGradientPoints] =
    {
        Color( 0xFF3DF2FF ),
        Color( 0xFF27D5FF ),
        Color( 0xFF1CB8FF ),
        Color( 0xFF1E9BFF ),
        Color( 0xFF277DFF ),
        Color( 0xFF315EF6 ),
        Color( 0xFF3A3DE9 )
    };

    Color Color::EvaluateYellowRedGradient( float weight, bool useDistinctColorForZero )
    {
        return EvaluateGradient( g_yellowRedGradient, weight, useDistinctColorForZero );
    }

    Color Color::GetCategorizedColor( int32_t categoryIdx, float saturation, float lightness )
    {
        constexpr static float const goldenRatioConjugate = 0.618033988749895f;
        float const hue = Math::FModF( categoryIdx * goldenRatioConjugate * 360.0f, 360.0f );

        // HSL lightness isn't perceived brightness - yellows/greens read far brighter than
        // blues/purples at the same value, so bias the lightness by hue to even them out
        float const adjustedLightness = lightness - 0.12f * cosf( Math::DegreesToRadians * ( hue - 60.0f ) );

        return Color::FromHSL( hue, saturation, adjustedLightness );
    }

    void Color::GenerateColors( int32_t numColors, TVector<Color>& outColors )
    {
        constexpr static int32_t const maxHuesPerTier = 16;
        constexpr static float const tierSaturation[4] = { 0.85f, 0.45f, 0.95f, 0.40f };
        constexpr static float const tierLightness[4] = { 0.62f, 0.80f, 0.45f, 0.62f };

        EE_ASSERT( numColors > 0 && numColors <= maxHuesPerTier * 4 );

        // Hue alone can only separate around 16 colors, so spread anything beyond that across
        // additional saturation/lightness tiers
        int32_t const numTiers = ( numColors + maxHuesPerTier - 1 ) / maxHuesPerTier;
        int32_t const numHues = ( numColors + numTiers - 1 ) / numTiers;

        // Visit the hues in a stride that is coprime with the hue count - this hits every hue
        // exactly once while making sure consecutive colors are never neighbours on the wheel
        int32_t hueStride = Math::Max( 1, Math::RoundToInt32( numHues * 0.618f ) );
        while ( hueStride > 1 && Math::GreatestCommonDivisor( hueStride, numHues ) != 1 )
        {
            hueStride--;
        }

        //-------------------------------------------------------------------------

        float const hueStep = 360.0f / numHues;

        outColors.clear();
        outColors.reserve( numColors );

        for ( int32_t i = 0; i < numColors; i++ )
        {
            int32_t const tier = i / numHues;
            int32_t const hueIndex = ( i * hueStride ) % numHues;

            // Offsetting each tier by a fraction of a step stops the tiers from sharing hues
            float const hue = hueStep * ( hueIndex + ( float( tier ) / numTiers ) );

            // HSL lightness isn't perceived brightness, so bias it by hue - without this the blues
            // and purples come out too dark to read and the yellows come out glaring
            float const lightness = Math::Clamp( tierLightness[tier] - ( 0.10f * cosf( Math::DegreesToRadians * ( hue - 60.0f ) ) ), 0.28f, 0.86f );

            outColors.emplace_back( Color::FromHSL( hue, tierSaturation[tier], lightness ) );
        }
    }

    //-------------------------------------------------------------------------

    Color Color::ToLinear() const
    {
        auto sRGBtoLinear = [] ( float c )
        {
            float linearRGBLo = c / 12.92F;
            float linearRGBHi = Math::Pow( ( c + 0.055F ) / 1.055F, 2.4F );
            float linearRGB = ( c <= 0.04045F ) ? linearRGBLo : linearRGBHi;
            return linearRGB;
        };

        Float4 linearColor = ToFloat4();
        linearColor.m_x = sRGBtoLinear( linearColor.m_x );
        linearColor.m_y = sRGBtoLinear( linearColor.m_y );
        linearColor.m_z = sRGBtoLinear( linearColor.m_z );
        linearColor.m_w = linearColor.m_w; // sRGB EOTF is not applied to the alpha channel

        return Color( linearColor );
    }
}