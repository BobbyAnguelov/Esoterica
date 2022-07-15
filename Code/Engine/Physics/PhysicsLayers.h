#pragma once

#include "System/Esoterica.h"
#include "System/TypeSystem/RegisteredType.h"

//-------------------------------------------------------------------------
// Physics Layers
//-------------------------------------------------------------------------
// The layers that we can add our physics objects to
// We allow a max of 32 layers
// The layers are synonymous with object types.
// When we perform any scene queries, we will specify the layers that those queries can interact with
// e.g., QueryRules rules( CreateLayerMask( Layers::Environment, Layers::Camera ) );
//-------------------------------------------------------------------------
// This enum is kept separate from the rest of the filtering code since it will be changed more often and 
// we want to limit compile time impacts of these changes

namespace EE::Physics
{
    // Max allowed enum value of 32
    enum class Layers : uint8_t
    {
        EE_REGISTER_ENUM

        Environment = 0,
        Navigation = 1,
        Camera = 2,
        IK = 3,
        Characters = 4,
    };

    //-------------------------------------------------------------------------

    template<typename... Args>
    constexpr uint32_t CreateLayerMask( Args&&... args )
    {
        uint32_t layerMask = 0;
        ( ( layerMask |= 1u << (uint8_t) args ), ... );
        return layerMask;
    }
}