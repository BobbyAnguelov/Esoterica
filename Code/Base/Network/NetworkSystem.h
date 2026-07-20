#pragma once
#include "Base/_Module/API.h"

//-------------------------------------------------------------------------

namespace EE::Network
{
    struct EE_BASE_API NetworkSystem final
    {
        static bool Initialize();
        static void Shutdown();
    };
}