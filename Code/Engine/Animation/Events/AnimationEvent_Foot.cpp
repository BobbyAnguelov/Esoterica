#include "AnimationEvent_Foot.h"
#include "Base/Encoding/Hash.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    constexpr static char const* g_phaseNames[4] =
    {
        "Left Foot Down",
        "Right Foot Passing",
        "Right Foot Down",
        "Left Foot Passing",
    };

    constexpr static char const* g_phaseConditionNames[6] =
    {
        "Left Foot Down",
        "Left Foot Passing",

        "Right Foot Down",
        "Right Foot Passing",

        "Left Phase",
        "Right Phase"
    };

    //-------------------------------------------------------------------------

    StringID FootEvent::GetSyncEventID() const
    {
        static StringID const phaseIDs[4] =
        {
            StringID( g_phaseNames[0] ),
            StringID( g_phaseNames[1] ),
            StringID( g_phaseNames[2] ),
            StringID( g_phaseNames[3] ),
        };

        return StringID( phaseIDs[(int32_t) m_phase] );
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
        static Color const phaseColors[4] =
        {
            Colors::LightSkyBlue,
            Colors::LightPink,
            Colors::HotPink,
            Colors::DeepSkyBlue,
        };

        return phaseColors[(int32_t) phase];
    }
    #endif
}