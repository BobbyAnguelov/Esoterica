#include "ApplicationGlobalState.h"
#include "Base/TypeSystem/CoreTypeIDs.h"
#include "Base/Memory/Memory.h"
#include "Base/Types/StringID.h"
#include "Base/Profiling.h"
#include "Base/Threading/Threading.h"
#include "Base/Logging/LoggingSystem.h"
#include "Base/Platform/Platform.h"

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