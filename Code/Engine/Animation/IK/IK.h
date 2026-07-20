#pragma once

#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    enum class IKBlendMode : uint8_t
    {
        EE_REFLECT_ENUM

        Effector, // Linearly blend the effector target position pre-IK
        Pose // Blend the pre-IK and post-IK poses (Note: This is more expensive)
    };
}