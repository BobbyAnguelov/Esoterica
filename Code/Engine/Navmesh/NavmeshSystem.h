#pragma once
#include "Engine/_Module/API.h"
#include "System/Systems.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class NavPowerAllocator;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API NavmeshSystem final : public ISystem
    {
        friend class NavmeshWorldSystem;

    public:

        EE_SYSTEM_ID( NavmeshSystem );

    public:

        NavmeshSystem();
        ~NavmeshSystem();

    private:

        NavPowerAllocator*                              m_pAllocator = nullptr;
    };
}