#pragma once
#include "Engine/_Module/API.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Engine/Entity/EntityIDs.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    enum class Mobility : uint8_t
    {
        EE_REFLECT_ENUM

        Static = 0,
        Dynamic,
        Kinematic
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API Constants
    {
        static Float3 const Gravity;
        constexpr static float const BaseDensity = 1000;
    };

    //-------------------------------------------------------------------------

    struct UserData
    {
        EntityID            m_entityID;
        ComponentID         m_componentID;

        #if EE_DEVELOPMENT_TOOLS
        TInlineString<32>   m_name;
        #endif
    };
}