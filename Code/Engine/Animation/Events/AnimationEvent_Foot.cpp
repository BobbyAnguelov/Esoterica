#include "AnimationEvent_Foot.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    char const* g_phaseNames[4] =
    {
        "Left Foot Down",
        "Right Foot Passing",
        "Right Foot Down",
        "Left Foot Passing",
    };

    //-------------------------------------------------------------------------

    StringID FootEvent::GetSyncEventID() const
    {
        return StringID( g_phaseNames[(int32_t) m_phase] );
    }

    #if EE_DEVELOPMENT_TOOLS
    InlineString FootEvent::GetDisplayText() const
    {
        return g_phaseNames[(int32_t) m_phase];
    }
    #endif
}