#pragma once

#include "Engine/_Module/API.h"
#include "Base/Math/Math.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------
// Easing utility functions 
//  - Reference: http://easings.net/ and http://robertpenner.com/easing/
//-------------------------------------------------------------------------

namespace EE::Math::Easing
{
    enum class Function : uint8_t
    {
        EE_REFLECT_ENUM

        Linear,
        Quad,
        Cubic,
        Quart,
        Quint,
        Sine,
        Expo,
        Circ,
    };

    //-------------------------------------------------------------------------

    enum class Operation : uint8_t
    {
        EE_REFLECT_ENUM

        Linear,

        InQuad,
        OutQuad,
        InOutQuad,

        InCubic,
        OutCubic,
        InOutCubic,

        InQuart,
        OutQuart,
        InOutQuart,

        InQuint,
        OutQuint,
        InOutQuint,

        InSine,
        OutSine,
        InOutSine,

        InExpo,
        OutExpo,
        InOutExpo,

        InCirc,
        OutCirc,
        InOutCirc,

        None
    };

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    EE_BASE_API char const* GetName( Function type );
    EE_BASE_API char const* GetName( Operation op );
    #endif

    //-------------------------------------------------------------------------

    EE_FORCE_INLINE float In( Function func, float t )
    {
        switch ( func )
        {
            case Function::Linear:
            {
                return t;
            }
            break;

            case Function::Quad:
            {
                return t * t;
            }
            break;

            case Function::Cubic:
            {
                return t * t * t;
            }
            break;

            case Function::Quart:
            {
                return t * t * t * t;
            }
            break;

            case Function::Quint:
            {
                return t * t * t * t * t;
            }
            break;

            case Function::Sine:
            {
                return 1.0f - Math::Cos( t * Math::PiDivTwo );
            }
            break;

            case Function::Expo:
            {
                return Math::Pow( 2.0f, 10.0f * ( t - 1.0f ) ) - 0.001f;
            }
            break;

            case Function::Circ:
            {
                return -( Math::Sqrt( 1.0f - t * t ) - 1.0f );
            }
            break;

            default:
            {
                EE_UNREACHABLE_CODE();
                return t;
            }
            break;
        }
    }

    EE_FORCE_INLINE float Out( Function func, float t )
    {
        return 1.0f - In( func, 1.0f - t );
    }

    EE_FORCE_INLINE float InOut( Function funcIn, Function funcOut, float t )
    {
        if ( t < 0.5f )
        {
            return In( funcIn, 2.0f * t ) * 0.5f;
        }
        else
        {
            return ( Out( funcOut, 2.0f * t - 1.0f ) * 0.5f ) + 0.5f;
        }
    }

    EE_FORCE_INLINE float Evaluate( Operation op, float t )
    {
        switch ( op )
        {
            case Operation::None:
            case Operation::Linear:
            {
                return t;
            }
            break;

            //-------------------------------------------------------------------------

            case Operation::InQuad:
            {
                return In( Function::Quad, t );
            }
            break;

            case Operation::OutQuad:
            {
                return Out( Function::Quad, t );
            }
            break;

            case Operation::InOutQuad:
            {
                return InOut( Function::Quad, Function::Quad, t );
            }
            break;

            //-------------------------------------------------------------------------

            case Operation::InCubic:
            {
                return In( Function::Cubic, t );
            }
            break;

            case Operation::OutCubic:
            {
                return Out( Function::Cubic, t );
            }
            break;

            case Operation::InOutCubic:
            {
                return InOut( Function::Cubic, Function::Cubic, t );
            }
            break;

            //-------------------------------------------------------------------------

            case Operation::InQuart:
            {
                return In( Function::Quart, t );
            }
            break;

            case Operation::OutQuart:
            {
                return Out( Function::Quart, t );
            }
            break;

            case Operation::InOutQuart:
            {
                return InOut( Function::Quart, Function::Quart, t );
            }
            break;

            //-------------------------------------------------------------------------

            case Operation::InQuint:
            {
                return In( Function::Quint, t );
            }
            break;

            case Operation::OutQuint:
            {
                return Out( Function::Quint, t );
            }
            break;

            case Operation::InOutQuint:
            {
                return InOut( Function::Quint, Function::Quint, t );
            }
            break;

            //-------------------------------------------------------------------------

            case Operation::InSine:
            {
                return In( Function::Sine, t );
            }
            break;

            case Operation::OutSine:
            {
                return Out( Function::Sine, t );
            }
            break;

            case Operation::InOutSine:
            {
                return InOut( Function::Sine, Function::Sine, t );
            }
            break;

            //-------------------------------------------------------------------------

            case Operation::InExpo:
            {
                return In( Function::Expo, t );
            }
            break;

            case Operation::OutExpo:
            {
                return Out( Function::Expo, t );
            }
            break;

            case Operation::InOutExpo:
            {
                return InOut( Function::Expo, Function::Expo, t );
            }
            break;

            //-------------------------------------------------------------------------

            case Operation::InCirc:
            {
                return In( Function::Circ, t );
            }
            break;

            case Operation::OutCirc:
            {
                return Out( Function::Circ, t );
            }
            break;

            case Operation::InOutCirc:
            {
                return InOut( Function::Circ, Function::Circ, t );
            }
            break;

            default:
            {
                EE_UNREACHABLE_CODE();
                return t;
            }
            break;
        }
    }
}