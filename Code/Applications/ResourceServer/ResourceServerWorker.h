#pragma once

#include "ResourceCompilationRequest.h"
#include "EngineTools/ThirdParty/subprocess/subprocess.h"
#include "System/Types/String.h"
#include "System/Threading/Threading.h"
#include "System/Threading/TaskSystem.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    struct ResourceServerContext;

    //-------------------------------------------------------------------------

    class ResourceServerWorker final : public ITaskSet
    {
    public:

        enum class Status : uint8_t
        {
            Idle,
            Working,
            Complete
        };

    public:

        ResourceServerWorker( TaskSystem* pTaskSystem, ResourceServerContext const& context, String const& workerFullPath );
        ~ResourceServerWorker();

        // Worker Status
        inline Status GetStatus() const { return m_status; }
        inline bool IsIdle() const { return m_status == Status::Idle; }
        inline bool IsBusy() const { return m_status == Status::Working; }
        inline bool IsComplete() const { return m_status == Status::Complete; }

        // Request an async resource compilation
        void ProcessRequest( CompilationRequest* pRequest );

        // Accept the result of a requesting compilation, will set the compiler back to idle
        inline CompilationRequest* AcceptResult()
        {
            EE_ASSERT( IsComplete() );
            EE_ASSERT( m_pRequest != nullptr && m_pRequest->IsComplete() );

            m_pTaskSystem->WaitForTask( this );

            auto pResult = m_pRequest;
            m_pRequest = nullptr;
            m_status = Status::Idle;
            return pResult;
        }

        // Get the current request ID
        inline ResourceID const& GetRequestResourceID() const 
        {
            EE_ASSERT( IsBusy() );
            EE_ASSERT( m_pRequest != nullptr );
            return m_pRequest->GetResourceID();
        }

    private:

        virtual void ExecuteRange( TaskSetPartition range, uint32_t threadnum ) override final;

        void PerformUpToDateCheck();
        void Compile();

    private:

        TaskSystem*                             m_pTaskSystem = nullptr;
        ResourceServerContext const&            m_context;
        String const                            m_workerFullPath;
        CompilationRequest*                     m_pRequest = nullptr;
        subprocess_s                            m_subProcess;
        std::atomic<Status>                     m_status = Status::Idle;
    };
}