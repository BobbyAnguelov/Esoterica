#include "Easing.h"

//-------------------------------------------------------------------------

namespace EE::Math::Easing
{
    static const EasingFuncPtr g_easingFunctions[] =
    {
        EaseInLinear,
        EaseInQuad,
        EaseOutQuad,
        EaseInOutQuad,
        EaseOutInQuad,
        EaseInCubic,
        EaseOutCubic,
        EaseInOutCubic,
        EaseOutInCubic,
        EaseInQuart,
        EaseOutQuart,
        EaseInOutQuart,
        EaseOutInQuart,
        EaseInQuint,
        EaseOutQuint,
        EaseInOutQuint,
        EaseOutInQuint,
        EaseInSine,
        EaseOutSine,
        EaseInOutSine,
        EaseOutInSine,
        EaseInExpo,
        EaseOutExpo,
        EaseInOutExpo,
        EaseOutInExpo,
        EaseInCirc,
        EaseOutCirc,
        EaseInOutCirc,
        EaseOutInCirc,
        NoEasing
    };

    //-------------------------------------------------------------------------

    float EvaluateEasingFunction( Type type, float parameter )
    {
        return g_easingFunctions[(uint32_t) type]( parameter );
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    static const char* const g_easingFunctionNames[] =
    {
        "EaseInLinear",
        "EaseInQuad",
        "EaseOutQuad",
        "EaseInOutQuad",
        "EaseOutInQuad",
        "EaseInCubic",
        "EaseOutCubic",
        "EaseInOutCubic",
        "EaseOutInCubic",
        "EaseInQuart",
        "EaseOutQuart",
        "EaseInOutQuart",
        "EaseOutInQuart",
        "EaseInQuint",
        "EaseOutQuint",
        "EaseInOutQuint",
        "EaseOutInQuint",
        "EaseInSine",
        "EaseOutSine",
        "EaseInOutSine",
        "EaseOutInSine",
        "EaseInExpo",
        "EaseOutExpo",
        "EaseInOutExpo",
        "EaseOutInExpo",
        "EaseInCirc",
        "EaseOutCirc",
        "EaseInOutCirc",
        "EaseOutInCirc",
        "No Easing"
    };

    char const* GetName( Type type )
    {
        return g_easingFunctionNames[(uint32_t) type];
    }
    #endif
}