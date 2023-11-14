#include "Animation_RuntimeGraph_Common.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    #if EE_DEVELOPMENT_TOOLS
    Color GetColorForValueType( GraphValueType type )
    {
        static const Color colors[] =
        {
            Colors::GhostWhite,
            Colors::PaleGreen,
            Colors::MediumOrchid,
            Colors::DodgerBlue,
            Colors::SandyBrown,
            Colors::Cyan,
            Colors::YellowGreen,
            Colors::GreenYellow,
            Colors::White,
        };

        return colors[(uint8_t) type];
    }

    char const* GetNameForValueType( GraphValueType type )
    {
        constexpr static char const* const names[] =
        {
            "Unknown",
            "Bool",
            "ID",
            "Float",
            "Vector",
            "Target",
            "Bone Mask",
            "Pose",
            "Special",
        };

        return names[(uint8_t) type];
    }
    #endif
}