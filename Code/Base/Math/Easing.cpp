#include "Easing.h"

//-------------------------------------------------------------------------

namespace EE::Math::Easing
{
    #if EE_DEVELOPMENT_TOOLS
    static constexpr const char* const g_easingFunctionNames[] =
    {
        "Linear",
        "Quad",
        "Cubic",
        "Quart",
        "Quint",
        "Sine",
        "Expo",
        "Circ",
    };

    char const* GetName( Function type )
    {
        return g_easingFunctionNames[(uint32_t) type];
    }

    //-------------------------------------------------------------------------

    static constexpr const char* const g_easingOperationNames[] =
    {
        "EaseInLinear",

        "EaseInQuad",
        "EaseOutQuad",
        "EaseInOutQuad",

        "EaseInCubic",
        "EaseOutCubic",
        "EaseInOutCubic",

        "EaseInQuart",
        "EaseOutQuart",
        "EaseInOutQuart",

        "EaseInQuint",
        "EaseOutQuint",
        "EaseInOutQuint",

        "EaseInSine",
        "EaseOutSine",
        "EaseInOutSine",

        "EaseInExpo",
        "EaseOutExpo",
        "EaseInOutExpo",

        "EaseInCirc",
        "EaseOutCirc",
        "EaseInOutCirc",

        "No Easing"
    };

    char const* GetName( Operation op )
    {
        return g_easingOperationNames[(uint32_t) op];
    }
    #endif
}