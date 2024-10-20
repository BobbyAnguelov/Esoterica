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
        result.m_byteColor.m_r = (uint8_t) Math::Clamp( Math::RoundToInt( lerpedR ), 0, 255 );
        result.m_byteColor.m_g = (uint8_t) Math::Clamp( Math::RoundToInt( lerpedG ), 0, 255 );
        result.m_byteColor.m_b = (uint8_t) Math::Clamp( Math::RoundToInt( lerpedB ), 0, 255 );
        result.m_byteColor.m_a = (uint8_t) Math::Clamp( Math::RoundToInt( lerpedA ), 0, 255 );
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
        int32_t const intervalIndex = Math::FloorToInt( interval );
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
}