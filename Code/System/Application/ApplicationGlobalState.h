#pragma once
#include "System/_Module/API.h"

//-------------------------------------------------------------------------
// Application Global State
//-------------------------------------------------------------------------
// Scoped object managing the lifetime of all EE global state and systems.
// e.g. Memory Allocators
//      Logging
//      Profiling
//      etc...
//
// An object of ApplicationGlobalState must be the first thing declared on 
// the stack in the entry point function of any EE application.
//-------------------------------------------------------------------------

namespace EE
{
    class EE_SYSTEM_API ApplicationGlobalState
    {
    public:

        ApplicationGlobalState( char const* pMainThreadName = nullptr );
        ~ApplicationGlobalState();

    private:

        bool m_primaryState = false;
    };
}