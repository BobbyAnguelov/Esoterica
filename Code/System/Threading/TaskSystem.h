#pragma once

#include "../_Module/API.h"
#include "System/Types/Arrays.h"
#include "System/Systems.h"
#include "System/ThirdParty/EnkiTS/TaskScheduler.h"

//-------------------------------------------------------------------------

namespace EE
{
    //-------------------------------------------------------------------------
    // Task scheduler
    //-------------------------------------------------------------------------
    // * Wrapper on top of EnkiTS
    //-------------------------------------------------------------------------
    // Note: Will also use main thread as a worker thread - when using blocking wait!

    using ITaskSet = enki::ITaskSet;
    using IPinnedTask = enki::IPinnedTask;
    using AsyncTask = enki::TaskSet;
    using TaskSetPartition = enki::TaskSetPartition;

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API TaskSystem : public ISystem
    {

    public:

        EE_SYSTEM_ID( TaskSystem );

    public:

        TaskSystem();
        ~TaskSystem();

        inline bool IsInitialized() const { return m_initialized; }
        void Initialize();
        void Shutdown();

        inline uint32_t GetNumWorkers() const { return m_numWorkers; }

        inline void WaitForAll() { m_taskScheduler.WaitforAll(); }

        inline void ScheduleTask( ITaskSet* pTask )
        {
            EE_ASSERT( m_initialized );
            m_taskScheduler.AddTaskSetToPipe( pTask );
        }

        inline void ScheduleTask( IPinnedTask* pTask )
        {
            EE_ASSERT( m_initialized );
            m_taskScheduler.AddPinnedTask( pTask );
        }

        inline void WaitForTask( ITaskSet* pTask )
        {
            m_taskScheduler.WaitforTask( pTask );
        }

        inline void WaitForTask( IPinnedTask* pTask )
        {
            m_taskScheduler.WaitforTask( pTask );
        }

    private:

        enki::TaskScheduler     m_taskScheduler;
        uint32_t                m_numWorkers = 0;
        bool                    m_initialized = false;
    };
}