#include "ApplicationGlobalState.h"
#include "System/TypeSystem/CoreTypeIDs.h"
#include "System/Memory/Memory.h"
#include "System/Types/StringID.h"
#include "System/Profiling.h"
#include "System/Threading/Threading.h"
#include "System/Logging/LoggingSystem.h"
#include "System/Platform/Platform.h"

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
        EE_ASSERT( !g_platformInitialized ); // Prevent multiple calls to initialize

        //-------------------------------------------------------------------------

        Platform::Initialize();
        Memory::Initialize();
        Threading::Initialize( ( pMainThreadName != nullptr ) ? pMainThreadName : "Main Thread" );
        Log::System::Initialize();
        TypeSystem::CoreTypeRegistry::Initialize();

        g_platformInitialized = true;
        m_initialized = true;
    }

    ApplicationGlobalState::~ApplicationGlobalState()
    {
        EE_ASSERT( m_initialized );
        EE_ASSERT( g_platformInitialized );
        g_platformInitialized = false;
        m_initialized = false;

        TypeSystem::CoreTypeRegistry::Shutdown();
        Log::System::Shutdown();
        Threading::Shutdown();
        Memory::Shutdown();
        Platform::Shutdown();
    }
}