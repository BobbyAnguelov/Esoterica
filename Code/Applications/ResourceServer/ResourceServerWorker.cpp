#include "ResourceServerWorker.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceServerWorker::ResourceServerWorker( TaskSystem* pTaskSystem, String const& workerFullPath )
        : m_pTaskSystem( pTaskSystem )
        , m_workerFullPath( workerFullPath )
    {
        EE_ASSERT( pTaskSystem != nullptr && !m_workerFullPath.empty() );
        m_SetSize = 1;

        // No default ctor for subprocess struct, so zero-init
        Memory::MemsetZero( &m_subProcess );
    }

    ResourceServerWorker::~ResourceServerWorker()
    {
        EE_ASSERT( !subprocess_alive( &m_subProcess ) );
    }

    void ResourceServerWorker::Compile( CompilationRequest* pRequest )
    {
        EE_ASSERT( IsIdle() );
        EE_ASSERT( pRequest != nullptr && pRequest->IsPending() );
        m_pRequest = pRequest;
        m_pRequest->m_status = CompilationRequest::Status::Compiling;
        m_status = Status::Compiling;
        m_pTaskSystem->ScheduleTask( this );
    }

    void ResourceServerWorker::ExecuteRange( TaskSetPartition range, uint32_t threadnum )
    {
        EE_ASSERT( IsCompiling() );
        EE_ASSERT( !m_pRequest->m_compilerArgs.empty() );
        char const* processCommandLineArgs[5] = { m_workerFullPath.c_str(), "-compile", m_pRequest->m_compilerArgs.c_str(), nullptr, nullptr };

        // Set package flag for packing request
        if ( m_pRequest->m_origin == CompilationRequest::Origin::Package )
        {
            processCommandLineArgs[3] = "-package";
        }

        // Start compiler process
        //-------------------------------------------------------------------------

        m_pRequest->m_compilationTimeStarted = PlatformClock::GetTime();

        int32_t result = subprocess_create( processCommandLineArgs, subprocess_option_combined_stdout_stderr | subprocess_option_inherit_environment | subprocess_option_no_window, &m_subProcess );
        if ( result != 0 )
        {
            m_pRequest->m_status = CompilationRequest::Status::Failed;
            m_pRequest->m_log = "Resource compiler failed to start!";
            m_pRequest->m_compilationTimeFinished = PlatformClock::GetTime();
            m_status = Status::Complete;
            return;
        }

        // Wait for compilation to complete
        //-------------------------------------------------------------------------

        int32_t exitCode;
        result = subprocess_join( &m_subProcess, &exitCode );
        if ( result != 0 )
        {
            m_pRequest->m_status = CompilationRequest::Status::Failed;
            m_pRequest->m_log = "Resource compiler failed to complete!";
            m_pRequest->m_compilationTimeFinished = PlatformClock::GetTime();
            m_status = Status::Complete;
            subprocess_destroy( &m_subProcess );
            return;
        }

        // Handle completed compilation
        //-------------------------------------------------------------------------

        m_pRequest->m_compilationTimeFinished = PlatformClock::GetTime();

        switch ( exitCode )
        {
            case 0:
            {
                m_pRequest->m_status = CompilationRequest::Status::Succeeded;
            }
            break;

            case 1:
            {
                m_pRequest->m_status = CompilationRequest::Status::SucceededWithWarnings;
            }
            break;

            default:
            {
                m_pRequest->m_status = CompilationRequest::Status::Failed;
            }
            break;
        }

        // Read error and output of process
        //-------------------------------------------------------------------------

        char readBuffer[512];
        while ( fgets( readBuffer, 512, subprocess_stdout( &m_subProcess ) ) )
        {
            m_pRequest->m_log += readBuffer;
        }

        m_status = Status::Complete;

        //-------------------------------------------------------------------------

        subprocess_destroy( &m_subProcess );
    }
}