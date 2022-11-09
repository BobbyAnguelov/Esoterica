#include "AnimationEvent_Transition.h"
#include "System/Types/Color.h"

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
    char const* GetTransitionMarkerName( TransitionMarker marker )
    {
        return g_markerNames[(int32_t) marker];
    }

    char const* GetTransitionMarkerConditionName( TransitionMarkerCondition markerCondition )
    {
        return g_markerConditionNames[(int32_t) markerCondition];
    }

    Color GetTransitionMarkerColor( TransitionMarker marker )
    {
        return g_markerColors[(int32_t) marker];
    }
    #endif
}