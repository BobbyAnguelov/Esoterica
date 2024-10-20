#pragma once

#include "Engine/_Module/API.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    enum class GraphValueType : uint8_t
    {
        EE_REFLECT_ENUM

        Unknown = 0,
        Bool,
        ID,
        Float,
        Vector, // 3D
        Target,
        BoneMask,
        Pose,
        Special, // Only used for custom graph pin types
    };

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    EE_ENGINE_API Color GetColorForValueType( GraphValueType type );
    EE_ENGINE_API char const* GetNameForValueType( GraphValueType type );
    EE_ENGINE_API StringID GetIDForValueType( GraphValueType type );
    EE_ENGINE_API GraphValueType GetValueTypeForID( StringID ID );
    #endif
}