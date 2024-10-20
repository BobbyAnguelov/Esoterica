#pragma once
#include "Base/_Module/API.h"

//-------------------------------------------------------------------------

namespace EE
{
    enum class Severity
    {
        Info = 0,
        Warning,
        Error,
        FatalError,
    };

    EE_BASE_API char const* GetSeverityAsString( Severity severity );
}