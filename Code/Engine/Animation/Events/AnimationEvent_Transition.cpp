#include "AnimationEvent_Transition.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    constexpr static char const* g_markerNames[3] =
    {
        "Fully Allowed",
        "Conditionally Allowed",
        "Blocked",
    };

    constexpr static char const* g_markerConditionNames[4] =
    {
        "Any Allowed",
        "Fully Allowed",
        "Conditionally Allowed",
        "Blocked",
    };

    static Color g_markerColors[3] =
    {
        Colors::CadetBlue,
        Colors::Yellow,
        Colors::Red,
    };

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    char const* GetTransitionRuleName( TransitionRule marker )
    {
        return g_markerNames[(int32_t) marker];
    }

    char const* GetTransitionRuleConditionName( TransitionRuleCondition markerCondition )
    {
        return g_markerConditionNames[(int32_t) markerCondition];
    }

    Color GetTransitionMarkerColor( TransitionRule marker )
    {
        return g_markerColors[(int32_t) marker];
    }
    #endif
}