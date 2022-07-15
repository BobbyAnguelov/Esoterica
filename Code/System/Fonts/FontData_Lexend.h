#pragma once
#include "../_Module/API.h"
#include "System/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE::Fonts::Lexend
{
    namespace Regular
    {
        EE_SYSTEM_API uint8_t const* GetData();
    }

    namespace Medium
    {
        EE_SYSTEM_API uint8_t const* GetData();
    }

    namespace Bold
    {
        EE_SYSTEM_API uint8_t const* GetData();
    }
};