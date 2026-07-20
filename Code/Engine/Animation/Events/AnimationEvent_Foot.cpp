#include "AnimationEvent_Foot.h"
#include "Base/Encoding/Hash.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    constexpr static char const* g_phaseNames[5] =
    {
        "Left Foot Down",
        "Right Foot Passing",
        "Right Foot Down",
        "Left Foot Passing",
        "None"
    };

    constexpr static char const* g_phaseConditionNames[7] =
    {
        "Left Foot Down",
        "Left Foot Passing",

        "Right Foot Down",
        "Right Foot Passing",

        "Left Phase",
        "Right Phase",

        "None"
    };

    //-------------------------------------------------------------------------

    StringID FootEvent::GetSyncEventID() const
    {
        static StringID const phaseIDs[5] =
        {
            StringID( g_phaseNames[0] ),
            StringID( g_phaseNames[1] ),
            StringID( g_phaseNames[2] ),
            StringID( g_phaseNames[3] ),
            StringID( g_phaseNames[4] ),
        };

        return phaseIDs[(int32_t) m_phase];
    }

    #if EE_DEVELOPMENT_TOOLS
    char const* FootEvent::GetPhaseName( Phase phase )
    {
        return g_phaseNames[(int32_t) phase];
    }

    char const* FootEvent::GetPhaseConditionName( PhaseCondition phaseCondition )
    {
        return g_phaseConditionNames[(int32_t) phaseCondition];
    }

    Color FootEvent::GetPhaseColor( Phase phase )
    {
        static Color const phaseColors[5] =
        {
            Colors::LightSkyBlue,
            Colors::LightPink,
            Colors::HotPink,
            Colors::DeepSkyBlue,
            Colors::Gray,
        };

        return phaseColors[(int32_t) phase];
    }
    #endif
}