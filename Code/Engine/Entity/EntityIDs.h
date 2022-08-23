#pragma once
#include "System/Types/UUID.h"
#include "System/Types/PointerID.h"

//-------------------------------------------------------------------------

namespace EE
{
    using ComponentID = PointerID;
    using EntityID = PointerID;
    using EntityMapID = UUID;
    using EntityWorldID = UUID;
}

//-------------------------------------------------------------------------

namespace eastl
{
    template <>
    struct hash<EE::PointerID>
    {
        EE_FORCE_INLINE eastl_size_t operator()( EE::PointerID const& t ) const { return t.m_ID; }
    };
}