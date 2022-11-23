#include "ResourceServerWorker.h"
#include "ResourceServerContext.h"
#include "ResourceCompileDependencyTree.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceServerWorker::ResourceServerWorker( TaskSystem* pTaskSystem, ResourceServerContext const& context, String const& workerFullPath )
        : m_pTaskSystem( pTaskSystem )
        , m_context( context )
        , m_workerFullPath( workerFullPath )
    {
        EE_ASSERT( pTaskSystem != nullptr && !m_workerFullPath.empty() );
        EE_ASSERT( m_context.IsValid() );
        m_SetSize = 1;

        // No default ctor for subprocess struct, so zero-init
        Memory::MemsetZero( &m_subProcess );
    }

    ResourceServerWorker::~ResourceServerWorker()
    {
        EE_ASSERT( !subprocess_alive( &m_subProcess ) );
    }

    void ResourceServerWorker::ProcessRequest( CompilationRequest* pRequest )
    {
        EE_ASSERT( IsIdle() );
        EE_ASSERT( pRequest != nullptr && pRequest->IsPending() );
        m_pRequest = pRequest;
        m_pRequest->m_status = CompilationRequest::Status::Compiling;
        m_status = Status::Working;
        m_pTaskSystem->ScheduleTask( this );
    }

    void ResourceServerWorker::ExecuteRange( TaskSetPartition range, uint32_t threadnum )
    {
        EE_ASSERT( IsBusy() );

        //-------------------------------------------------------------------------

        PerformUpToDateCheck();

        //-------------------------------------------------------------------------

        if ( m_pRequest->IsComplete() )
        {
            m_status = Status::Complete;
        }
        else
        {
            Compile();
        }
    }

    void ResourceServerWorker::PerformUpToDateCheck()
    {
        EE_ASSERT( m_pRequest != nullptr );
        EE_ASSERT( m_status == Status::Working );

        //-------------------------------------------------------------------------

        // Check compiler
        ResourceTypeID const resourceTypeID = m_pRequest->m_resourceID.GetResourceTypeID();
        auto pCompiler = m_context.m_pCompilerRegistry->GetCompilerForResourceType( resourceTypeID );
        if ( pCompiler == nullptr )
        {
            m_pRequest->m_log.sprintf( "Error: No compiler found for resource type (%s)!", m_pRequest->m_resourceID.ToString().c_str() );
            m_pRequest->m_status = CompilationRequest::Status::Failed;
        }

        // File Validity check
        bool sourceFileExists = false;
        if ( m_pRequest->m_status != CompilationRequest::Status::Failed )
        {
            sourceFileExists = FileSystem::Exists( m_pRequest->m_sourceFile );
            if ( pCompiler->IsInputFileRequired() && !sourceFileExists )
            {
                m_pRequest->m_log.sprintf( "Error: Source file (%s) doesnt exist!", m_pRequest->m_sourceFile.GetFullPath().c_str() );
                m_pRequest->m_status = CompilationRequest::Status::Failed;
            }
        }

        // Try create target dir
        if ( m_pRequest->m_status != CompilationRequest::Status::Failed )
        {
            if ( !m_pRequest->m_destinationFile.EnsureDirectoryExists() )
            {
                m_pRequest->m_log.sprintf( "Error: Destination path (%s) doesnt exist!", m_pRequest->m_destinationFile.GetParentDirectory().c_str() );
                m_pRequest->m_status = CompilationRequest::Status::Failed;
            }
        }

        // Check that target file isnt read-only
        if ( m_pRequest->m_status != CompilationRequest::Status::Failed )
        {
            if ( FileSystem::Exists( m_pRequest->m_destinationFile ) && FileSystem::IsFileReadOnly( m_pRequest->m_destinationFile ) )
            {
                m_pRequest->m_log.sprintf( "Error: Destination file (%s) is read-only!", m_pRequest->m_destinationFile.GetFullPath().c_str() );
                m_pRequest->m_status = CompilationRequest::Status::Failed;
            }
        }

        // Check compile dependency and if this resource needs compilation
        m_pRequest->m_upToDateCheckTimeStarted = PlatformClock::GetTime();
        CompileDependencyTree compileDependencyTree( m_context );
        if ( compileDependencyTree.BuildTree( m_pRequest->m_resourceID ) )
        {
            CompileDependencyNode const* pRoot = compileDependencyTree.GetRoot();
            m_pRequest->m_compilerVersion = pRoot->m_compilerVersion;
            m_pRequest->m_fileTimestamp = pRoot->m_timestamp;
            m_pRequest->m_sourceTimestampHash = pRoot->m_combinedHash;
            m_pRequest->m_status = pRoot->IsUpToDate() ? CompilationRequest::Status::SucceededUpToDate : CompilationRequest::Status::Pending;
        }
        else // Failed to generate dependency tree
        {
            m_pRequest->m_log.sprintf( compileDependencyTree.GetErrorMessage().c_str() );
            m_pRequest->m_status = CompilationRequest::Status::Failed;
        }
        m_pRequest->m_upToDateCheckTimeFinished = PlatformClock::GetTime();

        // Force compilation
        if ( m_pRequest->HasSucceeded() && m_pRequest->RequiresForcedRecompiliation() )
        {
            m_pRequest->m_status = CompilationRequest::Status::Pending;
        }
    }

    void ResourceServerWorker::Compile()
    {
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