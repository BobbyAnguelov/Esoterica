#include "ApplicationGlobalState.h"
#include "System/TypeSystem/CoreTypeIDs.h"
#include "System/CrashHandler/CrashHandler.h"
#include "System/Memory/Memory.h"
#include "System/Types/StringID.h"
#include "System/Profiling.h"
#include "System/Threading/Threading.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace
    {
        static bool g_platformInitialized = false;
    }

    //-------------------------------------------------------------------------

    ApplicationGlobalState::ApplicationGlobalState( char const* pMainThreadName )
    {
        EE_ASSERT( !g_platformInitialized );

        CrashHandling::Initialize();

        // Initialize memory and threading subsystems
        //-------------------------------------------------------------------------

        Memory::Initialize();
        Threading::Initialize( ( pMainThreadName != nullptr ) ? pMainThreadName : "Main Thread" );

        Log::Initialize();

        TypeSystem::CoreTypeRegistry::Initialize();

        g_platformInitialized = true;
        m_primaryState = true;
    }

    ApplicationGlobalState::~ApplicationGlobalState()
    {
        if ( m_primaryState )
        {
            EE_ASSERT( g_platformInitialized );
            g_platformInitialized = false;
            m_primaryState = false;

            TypeSystem::CoreTypeRegistry::Shutdown();

            Log::Shutdown();

            Threading::Shutdown();
            Memory::Shutdown();
        }

        CrashHandling::Shutdown();
    }
}