#include "EntityIDs.h"
#include <atomic>

//-------------------------------------------------------------------------

namespace EE
{
    static std::atomic<uint64_t> g_entityWorldID = 1;

    EntityWorldID EntityWorldID::Generate()
    {
        EntityWorldID ID;
        ID.m_value = g_entityWorldID++;
        EE_ASSERT( ID.m_value != UINT64_MAX );
        return ID;
    }

    //-------------------------------------------------------------------------

    static std::atomic<uint64_t> g_entityID = 1;

    EntityID EntityID::Generate()
    {
        EntityID ID;
        ID.m_value = g_entityID++;
        EE_ASSERT( ID.m_value != UINT64_MAX );
        return ID;
    }

    //-------------------------------------------------------------------------

    static std::atomic<uint64_t> g_componentID = 1;

    ComponentID ComponentID::Generate()
    {
        ComponentID ID;
        ID.m_value = g_componentID++;
        EE_ASSERT( ID.m_value != UINT64_MAX );
        return ID;
    }
}