#include "NavmeshSystem.h"
#include "NavPower.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    NavmeshSystem::NavmeshSystem()
    {
        #if EE_ENABLE_NAVPOWER
        m_pAllocator = EE::New<NavPowerAllocator>();
        #endif
    }

    NavmeshSystem::~NavmeshSystem()
    {
        #if EE_ENABLE_NAVPOWER
        EE::Delete( m_pAllocator );
        #endif
    }
}