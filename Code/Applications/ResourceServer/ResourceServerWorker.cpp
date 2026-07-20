#include "ResourceServerWorker.h"
#include "ResourceServerContext.h"
#include "ResourceServerRequest.h"
#include "Base/Memory/Memory.h"
#include "Base/Network/NetworkMessage.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    void WorkerTask::AddRequest( Request* pRequest )
    {
        EE_ASSERT( pRequest->GetStatus() == RequestStatus::Pending );
        EE_ASSERT( pRequest->m_log.empty() );

        m_requests.emplace_back( pRequest );

        switch ( m_state )
        {
            case State::Pending:
            {
                pRequest->m_status = RequestStatus::Pending;
            }
            break;

            case State::Requested:
            {
                pRequest->m_status = RequestStatus::Compiling;
            }
            break;

            default:
            EE_UNREACHABLE_CODE();
            break;
        }
    }

    //-------------------------------------------------------------------------

    void ResourceServerWorker::NetworkResponseBucket::Reset()
    {
        m_clientID = 0;
        m_updateResponses.clear();
        m_requestResponses.clear();
        m_requestHeartbeats.clear();
    }

    void ResourceServerWorker::NetworkResponseBucket::AddUpdateResponse( ResourceID const& ID, String const& filePath, String const& log )
    {
        if ( m_updateResponses.empty() )
        {
            m_updateResponses.push_back();
        }

        m_updateResponses.back().m_results.emplace_back( ID, filePath, log );

        if ( m_updateResponses.size() == NetworkMessageSettings::s_maxNumberOfRequestsPerMessage )
        {
            m_updateResponses.push_back();
        }
    }

    void ResourceServerWorker::NetworkResponseBucket::AddRequestResponse( ResourceID const& ID, String const& filePath, String const& log )
    {
        if ( m_requestResponses.empty() )
        {
            m_requestResponses.push_back();
        }

        m_requestResponses.back().m_results.emplace_back( ID, filePath, log );

        if ( m_requestResponses.size() == 64 )
        {
            m_requestResponses.push_back();
        }
    }

    void ResourceServerWorker::NetworkResponseBucket::AddRequestHeartbeat( ResourceID const& ID )
    {
        if ( m_requestHeartbeats.empty() )
        {
            m_requestHeartbeats.push_back();
        }

        // Heartbeats need to be unique
        bool shouldAdd = true;
        for ( auto const& result : m_requestHeartbeats.back().m_results )
        {
            if ( result.m_resourceID == ID )
            {
                shouldAdd = false;
                break;
            }
        }

        if ( shouldAdd )
        {
            m_requestHeartbeats.back().m_results.emplace_back( ID, "" );
        }

        if ( m_requestHeartbeats.size() == 64 )
        {
            m_requestHeartbeats.push_back();
        }
    }

    //-------------------------------------------------------------------------

    ResourceServerWorker::ResourceServerWorker( ResourceServerContext& context, uint64_t workerID )
        : m_workerID( workerID )
        , m_context( context )
    {
        EE_ASSERT( m_workerID != 0 );
        EE_ASSERT( m_context.m_pSettings->m_resourceCompilerExecutablePath.Exists() );

        InlineString const str( InlineString::CtorSprintf(), "%d", m_workerID );
        char const* processCommandLineArgs[] = { m_context.m_pSettings->m_resourceCompilerExecutablePath.c_str(), "-worker", str.c_str(), nullptr };

        Memory::MemsetZero( &m_process );
        int32_t const result = subprocess_create( processCommandLineArgs, subprocess_option_enable_async | subprocess_option_combined_stdout_stderr | subprocess_option_inherit_environment | subprocess_option_no_window, &m_process );
        if ( result != 0 )
        {
            EE_HALT();
        }
    }

    ResourceServerWorker::~ResourceServerWorker()
    {
        if ( subprocess_alive( &m_process ) )
        {
            subprocess_destroy( &m_process );
        }
    }

    WorkerTask* ResourceServerWorker::CreateTask( ResourceID const& resourceID, bool isPackagingRequest, bool forceCompilation )
    {
        return m_tasks.emplace_back( EE::New<WorkerTask>( resourceID, isPackagingRequest, forceCompilation ) );
    }

    WorkerTask* ResourceServerWorker::TryFindTaskMatchingRequest( Request const* pRequest ) const
    {
        EE_ASSERT( pRequest != nullptr );

        bool const isForcedCompilation = pRequest->m_origin == RequestOrigin::ManualCompileForced;
        bool const isPackagingRequest = pRequest->IsPackagingRequest();

        for ( auto pTask : m_tasks )
        {
            if ( pTask->m_resourceID != pRequest->m_resourceID )
            {
                continue;
            }

            if ( pTask->m_isPackagingRequest != isPackagingRequest )
            {
                continue;
            }

            // If the task is not forced and the new request is forced then we cannot re-use this task
            if ( !pTask->m_isForcedCompilation && isForcedCompilation )
            {
                continue;
            }

            return pTask;
        }

        return nullptr;
    }

    void ResourceServerWorker::FailTask( WorkerTask* pTask, String const& reason )
    {
        pTask->m_state = WorkerTask::State::Requested;

        auto& result = m_completionResults.emplace_back();
        result.m_taskID = pTask->m_ID;
        result.m_resourceID = pTask->m_resourceID;
        result.m_log = reason;
        result.m_compilationResult = CompilationResult::Failure;
    }

    void ResourceServerWorker::HandleHeartbeatMessage( NetworkResourceCompilerRequest&& msg )
    {
        for ( auto const& hb : msg.m_requests )
        {
            for ( auto pTask : m_tasks )
            {
                if ( pTask->m_ID == hb.m_taskID )
                {
                    // Reset heartbeat timer
                    EE_ASSERT( pTask->m_state == WorkerTask::State::Requested );
                    pTask->m_timeSinceLastHeartbeat.Reset();

                    // Update requests with new heartbeat time
                    for ( Request* pRequest : pTask->m_requests )
                    {
                        pRequest->m_lastWorkerHeartbeatTime = pTask->m_timeSinceLastHeartbeat.GetStartTimeMilliseconds();
                    }

                    break;
                }
            }
        }
    }

    void ResourceServerWorker::HandleCompletionMessage( NetworkResourceCompilerResponse&& msg )
    {
        m_completionResults.insert( m_completionResults.end(), msg.m_results.begin(), msg.m_results.end() );
    }

    void ResourceServerWorker::Update()
    {
        CheckIfProcessStillRunning();

        if ( m_networkClientID == 0 )
        {
            return;
        }

        // Update response buckets
        //-------------------------------------------------------------------------

        size_t const numBuckets = m_context.m_nonWorkerClientIDs.size();
        m_responseBuckets.resize( numBuckets );
        for ( size_t i = 0; i < numBuckets; i++ )
        {
            m_responseBuckets[i].Reset();
            m_responseBuckets[i].m_clientID = m_context.m_nonWorkerClientIDs[i];
        }

        //-------------------------------------------------------------------------

        ProcessPendingTasks();
        ProcessTimedOutTasks();
        ProcessCompletionResults();
        GenerateHeartbeats();

        // Send responses
        //-------------------------------------------------------------------------

        for ( size_t i = 0; i < numBuckets; i++ )
        {
            // Update notifications
            for ( NetworkResourceResponse const& response : m_responseBuckets[i].m_updateResponses )
            {
                if ( response.Empty() )
                {
                    continue;
                }

                Network::Message message( (int32_t) NetworkMessageID::ResourceUpdated, response );
                message.SetClientID( m_responseBuckets[i].m_clientID );
                m_context.m_networkServer.SendNetworkMessage( eastl::move( message ) );
            }

            // Completed requests
            for ( NetworkResourceResponse const& response : m_responseBuckets[i].m_requestResponses )
            {
                if ( response.Empty() )
                {
                    continue;
                }

                Network::Message message( (int32_t) NetworkMessageID::ResourceRequestComplete, response );
                message.SetClientID( m_responseBuckets[i].m_clientID );
                m_context.m_networkServer.SendNetworkMessage( eastl::move( message ) );
            }

            // Heartbeats
            for ( NetworkResourceResponse const& response : m_responseBuckets[i].m_requestHeartbeats )
            {
                if ( response.Empty() )
                {
                    continue;
                }

                Network::Message message( (int32_t) NetworkMessageID::ResourceRequestHeartbeat, response );
                message.SetClientID( m_responseBuckets[i].m_clientID );
                m_context.m_networkServer.SendNetworkMessage( eastl::move( message ) );
            }
        }
    }

    void ResourceServerWorker::CheckIfProcessStillRunning()
    {
        if ( subprocess_alive( &m_process ) )
        {
            return;
        }

        // Restart Process if it has crashed
        //-------------------------------------------------------------------------

        m_log.LogError( "Resource Worker %d Crashed, Restarting!", m_workerID );

        int32_t result = subprocess_destroy( &m_process );
        if ( result != 0 )
        {
            EE_HALT();
        }
        Memory::MemsetZero( &m_process );

        m_networkClientID = 0;

        InlineString const str( InlineString::CtorSprintf(), "%d", m_workerID );
        char const* processCommandLineArgs[] = { m_context.m_pSettings->m_resourceCompilerExecutablePath.c_str(), "-worker", str.c_str(), nullptr };
        result = subprocess_create( processCommandLineArgs, subprocess_option_enable_async | subprocess_option_combined_stdout_stderr | subprocess_option_inherit_environment | subprocess_option_no_window, &m_process );
        if ( result != 0 )
        {
            EE_HALT();
        }

        // Resend all registered tasks
        //-------------------------------------------------------------------------

        for ( auto pTask : m_tasks )
        {
            if ( ( ++pTask->m_retryAttempts ) > s_maxRetryAttempts )
            {
                FailTask( pTask, "Process Crashed - Max retry attempts exceeded" );
            }
            else
            {
                pTask->m_state = WorkerTask::State::Pending;
                pTask->m_log.clear();
                pTask->m_timeSinceLastHeartbeat.Reset();

                // Update requests
                for ( auto pRequest : pTask->m_requests )
                {
                    pRequest->m_status = RequestStatus::Pending;
                }
            }
        }
    }

    void ResourceServerWorker::ProcessPendingTasks()
    {
        NetworkResourceCompilerRequest requestData;

        for ( auto pTask : m_tasks )
        {
            if ( pTask->m_state == WorkerTask::State::Pending )
            {
                NetworkResourceCompilerRequest::Request& networkRequest = requestData.m_requests.emplace_back();
                networkRequest.m_resourceID = pTask->m_resourceID;
                networkRequest.m_isForPackagedBuild = pTask->m_isPackagingRequest;
                networkRequest.m_forceCompilation = pTask->m_isForcedCompilation;
                networkRequest.m_taskID = pTask->m_ID;

                pTask->m_state = WorkerTask::State::Requested;

                // Update requests
                for ( auto pRequest : pTask->m_requests )
                {
                    pRequest->m_status = RequestStatus::Compiling;
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( !requestData.m_requests.empty() )
        {
            m_context.m_networkServer.SendNetworkMessage( m_networkClientID, Network::Message( (int32_t) NetworkResourceWorkerMessageID::CompileResource, requestData ) );
        }
    }

    void ResourceServerWorker::ProcessTimedOutTasks()
    {
        NetworkResourceCompilerRequest requestData;

        for ( auto pTask : m_tasks )
        {
            if ( pTask->m_state == WorkerTask::State::Requested && pTask->m_timeSinceLastHeartbeat.GetElapsedTimeSeconds() > s_heartbeatTimeoutSeconds )
            {
                // If we exceed the retry attempts, then fail the task
                if ( ( ++pTask->m_retryAttempts ) > s_maxRetryAttempts )
                {
                    FailTask( pTask, "Lost Connection With Worker - Max retry attempts exceeded" );
                }
                else // Retry task
                {
                    NetworkResourceCompilerRequest::Request& networkRequest = requestData.m_requests.emplace_back();
                    networkRequest.m_resourceID = pTask->m_resourceID;
                    networkRequest.m_isForPackagedBuild = pTask->m_isPackagingRequest;
                    networkRequest.m_forceCompilation = false;
                    networkRequest.m_taskID = pTask->m_ID;

                    pTask->m_state = WorkerTask::State::Requested;
                    pTask->m_log.clear();
                    pTask->m_timeSinceLastHeartbeat.Reset();
                }
            }
        }

        if ( !requestData.m_requests.empty() )
        {
            m_context.m_networkServer.SendNetworkMessage( m_networkClientID, Network::Message( (int32_t) NetworkResourceWorkerMessageID::CompileResource, requestData ) );
        }
    }

    void ResourceServerWorker::ProcessCompletionResults()
    {
        // Process all completion messages
        //-------------------------------------------------------------------------

        for ( NetworkResourceCompilerResponse::Result const& result : m_completionResults )
        {
            for ( auto iter = m_tasks.begin(); iter != m_tasks.end(); ++iter )
            {
                WorkerTask* pTask = *iter;
                if ( pTask->m_ID != result.m_taskID )
                {
                    continue;
                }

                //-------------------------------------------------------------------------

                EE_ASSERT( pTask->m_state == WorkerTask::State::Requested );
                EE_ASSERT( pTask->m_resourceID == result.m_resourceID );
                pTask->m_state = WorkerTask::State::Complete;

                // Update requests
                for ( Request* pRequest : pTask->m_requests )
                {
                    // Set request to complete
                    //-------------------------------------------------------------------------

                    pRequest->m_log = result.m_log;
                    pRequest->m_upToDateCheckTime = result.m_upToDateCheckTimeMS;
                    pRequest->m_compileTime = result.m_compilationTimeMS;
                    pRequest->m_timeCompleted = PlatformClock::GetTimeInMilliseconds();

                    switch ( result.m_compilationResult )
                    {
                        case CompilationResult::Failure:
                        {
                            pRequest->m_status = RequestStatus::Failed;
                        }
                        break;

                        case CompilationResult::SuccessUpToDate:
                        {
                            pRequest->m_status = RequestStatus::SucceededUpToDate;
                        }
                        break;

                        case CompilationResult::Success:
                        {
                            pRequest->m_status = RequestStatus::Succeeded;
                        }
                        break;

                        case CompilationResult::SuccessWithWarnings:
                        {
                            pRequest->m_status = RequestStatus::SucceededWithWarnings;
                        }
                        break;
                    }

                    // Create network notifications
                    //-------------------------------------------------------------------------

                    if ( pRequest->IsInternalRequest() )
                    {
                        // No need to notify the client for internal requests resources that are up to date and certain auto-triggered compiles
                        if ( pRequest->ShouldNotifyClientsWhenComplete() && pRequest->m_status != RequestStatus::SucceededUpToDate )
                        {
                            // Bulk notify all connected client that a resource has been recompiled so that they can reload it if necessary
                            for ( auto& clientBucket : m_responseBuckets )
                            {
                                if ( pRequest->HasSucceeded() )
                                {
                                    clientBucket.AddUpdateResponse( pRequest->m_resourceID, pRequest->GetDestinationFilePath().ToString() );
                                }
                                else
                                {
                                    clientBucket.AddUpdateResponse( pRequest->m_resourceID, "", pRequest->m_log );
                                }
                            }
                        }
                    }
                    else // Notify single client
                    {
                        for ( auto& clientBucket : m_responseBuckets )
                        {
                            if ( clientBucket.m_clientID == pRequest->m_clientID )
                            {
                                if ( pRequest->HasSucceeded() )
                                {
                                    clientBucket.AddRequestResponse( pRequest->m_resourceID, pRequest->GetDestinationFilePath().ToString() );
                                }
                                else
                                {
                                    clientBucket.AddRequestResponse( pRequest->m_resourceID, "", pRequest->m_log );
                                }

                                break;
                            }
                        }
                    }
                }

                // Destroy Task
                //-------------------------------------------------------------------------

                EE::Delete( pTask );
                m_tasks.erase( iter );
                break;
            }
        }
    }

    void ResourceServerWorker::GenerateHeartbeats()
    {
        for ( auto pTask : m_tasks )
        {
            for ( Request* pRequest : pTask->m_requests )
            {
                if ( pRequest->IsInternalRequest() )
                {
                    continue;
                }

                for ( auto& clientBucket : m_responseBuckets )
                {
                    if ( clientBucket.m_clientID != pRequest->m_clientID )
                    {
                        continue;
                    }

                    Seconds const currentTime = PlatformClock::GetTimeInSeconds();
                    Seconds const timeSinceLastSentHeartbeat = currentTime - pRequest->m_lastSentHeartbeatTime;
                    if ( timeSinceLastSentHeartbeat > NetworkMessageSettings::s_heartbeatSendIntervalSeconds )
                    {
                        clientBucket.AddRequestHeartbeat( pRequest->m_resourceID );
                        pRequest->m_lastSentHeartbeatTime = currentTime;
                    }
                }
            }
        }
    }
}
