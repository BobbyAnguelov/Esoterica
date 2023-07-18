#pragma once
#include "../_Module/API.h"
#include "Base/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE::Fonts::Lexend
{
    namespace Regular
    {
        EE_BASE_API uint8_t const* GetData();
    }

    namespace Medium
    {
        EE_BASE_API uint8_t const* GetData();
    }

    namespace Bold
    {
        EE_BASE_API uint8_t const* GetData();
    }
};