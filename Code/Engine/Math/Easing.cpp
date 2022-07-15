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
    };

    //-------------------------------------------------------------------------

    float EvaluateEasingFunction( Type type, float parameter )
    {
        return g_easingFunctions[(uint32_t) type]( parameter );
    }
}