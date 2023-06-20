#pragma once
#include "System/_Module/API.h"

//-------------------------------------------------------------------------

namespace EE::CrashHandling
{
    void Initialize();
    void Shutdown();

    EE_SYSTEM_API void RecordAssert( char const* pFile, int line, char const* pAssertInfo );
    EE_SYSTEM_API void RecordAssertVarArgs( char const* pFile, int line, char const* pAssertInfoFormat, ... );
}