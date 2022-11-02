#pragma once

#include "Engine/_Module/API.h"
#include "System/Math/Math.h"
#include "System/TypeSystem/RegisteredType.h"

//-------------------------------------------------------------------------
// Easing utility functions 
//  - Reference: http://easings.net/ and http://robertpenner.com/easing/
//-------------------------------------------------------------------------

namespace EE::Math::Easing
{
    enum class Type : uint8_t
    {
        EE_REGISTER_ENUM

        Linear,
        InQuad,
        OutQuad,
        InOutQuad,
        OutInQuad,
        InCubic,
        OutCubic,
        InOutCubic,
        OutInCubic,
        InQuart,
        OutQuart,
        InOutQuart,
        OutInQuart,
        InQuint,
        OutQuint,
        InOutQuint,
        OutInQuint,
        InSine,
        OutSine,
        InOutSine,
        OutInSine,
        InExpo,
        OutExpo,
        InOutExpo,
        OutInExpo,
        InCirc,
        OutCirc,
        InOutCirc,
        OutInCirc,
    };

    typedef float( *EasingFuncPtr )( float t );

    // Helper functions
    //-------------------------------------------------------------------------

    EE_ENGINE_API float EvaluateEasingFunction( Type type, float parameter );

    template<EasingFuncPtr easeFuncPtr>
    float EaseIn( float t )
    {
        return easeFuncPtr( t );
    }

    template<EasingFuncPtr easeFuncPtr>
    float EaseOut( float t )
    {
        return 1.0f - easeFuncPtr( 1.0f - t );
    }

    template<EasingFuncPtr easeInFunc, EasingFuncPtr easeOutFunc>
    float EaseInOut( float t )
    {
        if ( t < 0.5f )
        {
            return easeInFunc( 2.0f * t ) * 0.5f;
        }
        else
        {
            return ( easeOutFunc( 2.0f * t - 1.0f ) * 0.5f ) + 0.5f;
        }
    }

    #if EE_DEVELOPMENT_TOOLS
    EE_ENGINE_API char const* GetName( Type type );
    #endif

    // Individual easing functions
    //-------------------------------------------------------------------------

    inline float EaseInLinear( float t )
    {
        return t;
    }

    //-------------------------------------------------------------------------

    inline float EaseInQuad( float t )
    {
        return t * t;
    }

    inline float EaseOutQuad( float t )
    {
        return EaseOut<EaseInQuad>( t );
    }

    inline float EaseInOutQuad( float t )
    {
        return EaseInOut<EaseInQuad, EaseOutQuad>( t );
    }

    inline float EaseOutInQuad( float t )
    {
        return EaseInOut<EaseOutQuad, EaseInQuad>( t );
    }

    //-------------------------------------------------------------------------

    inline float EaseInCubic( float t )
    {
        return t * t * t;
    }

    inline float EaseOutCubic( float t )
    {
        return EaseOut<EaseInCubic>( t );
    }

    inline float EaseInOutCubic( float t )
    {
        return EaseInOut<EaseInCubic, EaseOutCubic>( t );
    }

    inline float EaseOutInCubic( float t )
    {
        return EaseInOut<EaseOutCubic, EaseInCubic>( t );
    }

    //-------------------------------------------------------------------------

    inline float EaseInQuart( float t )
    {
        return t * t * t * t;
    }

    inline float EaseOutQuart( float t )
    {
        return EaseOut<EaseInQuart>( t );
    }

    inline float EaseInOutQuart( float t )
    {
        return EaseInOut<EaseInQuart, EaseOutQuart>( t );
    }

    inline float EaseOutInQuart( float t )
    {
        return EaseInOut<EaseOutQuart, EaseInQuart>( t );
    }

    //-------------------------------------------------------------------------

    inline float EaseInQuint( float t )
    {
        return t * t * t * t * t;
    }

    inline float EaseOutQuint( float t )
    {
        return EaseOut<EaseInQuint>( t );
    }

    inline float EaseInOutQuint( float t )
    {
        return EaseInOut<EaseInQuint, EaseOutQuint>( t );
    }

    inline float EaseOutInQuint( float t )
    {
        return EaseInOut<EaseOutQuint, EaseInQuint>( t );
    }

    //-------------------------------------------------------------------------

    inline float EaseInSine( float t )
    {
        return 1.0f - Cos( t * PiDivTwo );
    }

    inline float EaseOutSine( float t )
    {
        return EaseOut<EaseInSine>( t );
    }

    inline float EaseInOutSine( float t )
    {
        return EaseInOut<EaseInSine, EaseOutSine>( t );
    }

    inline float EaseOutInSine( float t )
    {
        return EaseInOut<EaseOutSine, EaseInSine>( t );
    }

    //-------------------------------------------------------------------------

    inline float EaseInExpo( float t )
    {
        return Pow( 2.0f, 10.0f * ( t - 1.0f ) ) - 0.001f;
    }

    inline float EaseOutExpo( float t )
    {
        return EaseOut<EaseInExpo>( t );
    }

    inline float EaseInOutExpo( float t )
    {
        return EaseInOut<EaseInExpo, EaseOutExpo>( t );
    }

    inline float EaseOutInExpo( float t )
    {
        return EaseInOut<EaseOutExpo, EaseInExpo>( t );
    }

    //-------------------------------------------------------------------------

    inline float EaseInCirc( float t )
    {
        return -( Sqrt( 1.0f - t * t ) - 1.0f );
    }

    inline float EaseOutCirc( float t )
    {
        return EaseOut<EaseInCirc>( t );
    }

    inline float EaseInOutCirc( float t )
    {
        return EaseInOut<EaseInCirc, EaseOutCirc>( t );
    }

    inline float EaseOutInCirc( float t )
    {
        return EaseInOut<EaseOutCirc, EaseInCirc>( t );
    }
}