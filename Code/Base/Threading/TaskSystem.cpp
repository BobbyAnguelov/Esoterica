#include "TaskSystem.h"
#include "Threading.h"
#include "Base/Math/Math.h"
#include "Base/Memory/Memory.h"
#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE
{
    static void OnStartThread( uint32_t threadNum )
    {
        Memory::InitializeThreadHeap();

        char nameBuffer[100];
        Printf( nameBuffer, 100, "EE Worker %d", threadNum );

        EE_PROFILE_THREAD_START( nameBuffer );
        Threading::SetCurrentThreadName( nameBuffer );
    }

    static void OnStopThread( uint32_t threadNum )
    {
        Memory::ShutdownThreadHeap();
        EE_PROFILE_THREAD_END();
    }

    static void* CustomAllocFunc( size_t alignment, size_t size, void* userData_, const char* file_, int line_ )
    {
        return EE::Alloc( size, alignment );
    }

    static void CustomFreeFunc( void* ptr, size_t size, void* userData_, const char* file_, int line_ )
    {
        EE::Free( ptr );
    }

    //-------------------------------------------------------------------------

    TaskSystem::TaskSystem( int32_t numWorkers )
        : m_numWorkers( numWorkers )
    {
        EE_ASSERT( numWorkers > 0 );
    }

    TaskSystem::~TaskSystem()
    {
        EE_ASSERT( !m_initialized );
    }

    void TaskSystem::Initialize()
    {
        enki::TaskSchedulerConfig config;
        config.numTaskThreadsToCreate = m_numWorkers;
        config.customAllocator.alloc = CustomAllocFunc;
        config.customAllocator.free = CustomFreeFunc;
        config.profilerCallbacks.threadStart = OnStartThread;
        config.profilerCallbacks.threadStop = OnStopThread;

        m_taskScheduler.Initialize( config );
        m_initialized = true;
    }

    void TaskSystem::Shutdown()
    {
        m_taskScheduler.WaitforAllAndShutdown();
        m_initialized = false;
    }
}
