#pragma once

#include "Base/Types/String.h"
#include "Base/Time/Time.h"
#include "Base/ThirdParty/concurrentqueue/concurrentqueue.h"
#include <mutex>
#include <shared_mutex>

//-------------------------------------------------------------------------

namespace EE
{
    namespace Threading
    {
        using ThreadID = uint32_t;

        //-------------------------------------------------------------------------
        // Processor Info
        //-------------------------------------------------------------------------

        struct ProcessorInfo
        {
            uint16_t m_numPhysicalCores = 0;
            uint16_t m_numLogicalCores = 0;
        };

        EE_BASE_API ProcessorInfo GetProcessorInfo();

        //-------------------------------------------------------------------------
        // Mutexes and Locks
        //-------------------------------------------------------------------------

        typedef std::thread Thread;
        typedef std::mutex Mutex;
        typedef std::recursive_mutex RecursiveMutex;
        typedef std::condition_variable ConditionVariable;

        using Lock = std::unique_lock<Mutex>;
        using RecursiveLock = std::unique_lock<RecursiveMutex>;

        using ScopeLock = std::lock_guard<Mutex>;
        using RecursiveScopeLock = std::lock_guard<RecursiveMutex>;

        // Read/Write lock
        class ReadWriteMutex
        {
        public:

            inline void LockForWrite() { m_mutex.lock(); }
            inline bool TryLockForWrite() { return m_mutex.try_lock(); }

            inline void LockForRead() { m_mutex.lock_shared(); }
            inline bool TryLockForRead() { return m_mutex.try_lock_shared(); }

        private:

            std::shared_mutex m_mutex;
        };

        //-------------------------------------------------------------------------
        // Synchronization Event Semaphore
        //-------------------------------------------------------------------------

        class EE_BASE_API SyncEvent
        {
        public:

            SyncEvent();
            ~SyncEvent();

            void Signal();
            void Reset();

            void Wait() const;
            void Wait( Milliseconds maxWaitTime ) const;

        private:

            void* m_pNativeHandle;
        };

        //-------------------------------------------------------------------------
        // Data structures
        //-------------------------------------------------------------------------

        template<typename T, typename Traits = moodycamel::ConcurrentQueueDefaultTraits> using LockFreeQueue = moodycamel::ConcurrentQueue<T, Traits>;

        //-------------------------------------------------------------------------
        // Utility Functions
        //-------------------------------------------------------------------------

        inline void Sleep( Milliseconds time )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( time ) );
        }

        EE_BASE_API bool IsMainThread();

        //-------------------------------------------------------------------------
        // Global State
        //-------------------------------------------------------------------------

        EE_BASE_API void Initialize( char const* pMainThreadName );
        EE_BASE_API void Shutdown();
        EE_BASE_API ThreadID GetCurrentThreadID();
        EE_BASE_API void SetCurrentThreadName( char const* pName );
    }
}