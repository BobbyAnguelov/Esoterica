#include "ResourceProvider_Network.h"
#include "Base/Resource/Settings/Settings_Resource.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Profiling.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Resource
{
    NetworkResourceProvider::NetworkResourceProvider( ResourceSettings const& settings, TaskSystem& taskSystem )
        : IResourceProvider( settings ), m_taskSystem( taskSystem )
        , m_deserializationTask( [this] ( TaskSetPartition range, uint32_t threadnum ) { DeserializeReceivedMessages(); } )
    {}

    NetworkResourceProvider::~NetworkResourceProvider()
    {
        m_taskSystem.WaitForTask( &m_deserializationTask );

        // Flush deserialization queue
        Network::Message* pMessage = nullptr;
        while ( m_unserializedMessages.try_dequeue( pMessage ) )
        {
            EE::Delete( pMessage );
        }
    }

    bool NetworkResourceProvider::IsReady() const
    {
        return m_networkClient.IsConnected();
    }

    bool NetworkResourceProvider::IsConnecting() const
    {
        return m_networkClient.IsConnecting() || m_networkClient.IsReconnecting();
    }

    bool NetworkResourceProvider::Initialize()
    {
        if ( !m_networkClient.Start( m_settings.m_resourceServerNetworkAddress.c_str(), m_settings.m_resourceServerPort ) )
        {
            EE_LOG_ERROR( LogCategory::Resource, "Resource Provider", "Failed to connect to the resource server (%s:%d)!", m_settings.m_resourceServerNetworkAddress.c_str(), m_settings.m_resourceServerPort );
            return false;
        }

        return true;
    }

    void NetworkResourceProvider::Shutdown()
    {
        m_networkClient.Stop();
    }

    void NetworkResourceProvider::RequestRawResource( ResourceRequest* pRequest )
    {
        EE_ASSERT( pRequest != nullptr && pRequest->IsValid() && pRequest->GetLoadingStatus() == LoadingStatus::Loading );

        Threading::ScopeLock const sl( m_requestModificationMutex );

        #if EE_DEVELOPMENT_TOOLS
        auto foundIter = VectorFind( m_sentRequests, pRequest->GetResourceID() );
        EE_ASSERT( foundIter == m_sentRequests.end() );
        #endif

        m_pendingRequests.emplace_back( pRequest );
    }

    void NetworkResourceProvider::CancelRequest( ResourceRequest* pRequest )
    {
        EE_ASSERT( pRequest != nullptr && pRequest->IsValid() );

        Threading::ScopeLock const sl( m_requestModificationMutex );

        auto foundIter = VectorFind( m_sentRequests, pRequest );
        EE_ASSERT( foundIter != m_sentRequests.end() );
        m_sentRequests.erase_unsorted( foundIter );
    }

    void NetworkResourceProvider::Update()
    {
        EE_PROFILE_FUNCTION_RESOURCE();
        EE_ASSERT( Threading::IsMainThread() );

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_externallyUpdatedResources.clear();
        #endif

        // Update network client
        //-------------------------------------------------------------------------

        m_networkClient.UpdateAndTransferMessageOwnership( [this] ( Network::Message* pMessage ) { m_unserializedMessages.enqueue( pMessage ); } );

        if ( !m_networkClient.IsConnected() )
        {
            return;
        }

        // Lock mutex
        //-------------------------------------------------------------------------

        Threading::ScopeLock const sl( m_requestModificationMutex );

        // Send all requests
        //-------------------------------------------------------------------------

        if ( !m_pendingRequests.empty() )
        {
            NetworkResourceRequest request;

            // Process all pending requests
            for( auto pRequest : m_pendingRequests )
            {
                request.m_resourceIDs.emplace_back( pRequest->GetResourceID() );
                auto& sentRequest = m_sentRequests.emplace_back( pRequest );
                sentRequest.m_timeSinceLastHeartbeat.Reset();

                // Try to limit the size of the network messages so we limit each message to a set number of requests
                if ( request.m_resourceIDs.size() == NetworkMessageSettings::s_maxNumberOfRequestsPerMessage )
                {
                    Network::Message requestResourceMessage( (int32_t) NetworkMessageID::RequestResource, request );
                    m_networkClient.SendNetworkMessage( eastl::move( requestResourceMessage ) );
                    request.m_resourceIDs.clear();
                }
            }
            m_pendingRequests.clear();

            // Send any remaining requests
            if ( !request.m_resourceIDs.empty() )
            {
                Network::Message requestResourceMessage( (int32_t) NetworkMessageID::RequestResource, request );
                m_networkClient.SendNetworkMessage( eastl::move( requestResourceMessage ) );
            }
        }

        // Process Heartbeats
        //-------------------------------------------------------------------------

        {
            for ( NetworkResourceResponse::Result const& ack : m_serverHeartbeats )
            {
                for ( SentRequest& sentRequest : m_sentRequests )
                {
                    if ( sentRequest.m_pRequest->GetResourceID() == ack.m_resourceID )
                    {
                        sentRequest.m_timeSinceLastHeartbeat.Reset();

                        if ( sentRequest.m_state == SentRequest::State::Requested )
                        {
                            sentRequest.m_state = SentRequest::State::Acknowledged;
                            sentRequest.m_retryCount = 0;
                        }
                    }
                }
            }

            m_serverHeartbeats.clear();
        }

        // Check timeouts
        if ( !m_sentRequests.empty() )
        {
            NetworkResourceRequest retryRequest;

            for ( auto& sentRequest : m_sentRequests )
            {
                Seconds const timeSinceLastHeartbeat = sentRequest.GetTimeSinceLastHeartbeat();

                if ( timeSinceLastHeartbeat > NetworkMessageSettings::s_heartbeatTimeoutSeconds )
                {
                    // Retry
                    if ( ( ++sentRequest.m_retryCount ) <= NetworkMessageSettings::s_maxRequestTimeoutRetries )
                    {
                        EE_LOG_WARNING( LogCategory::Resource, "Network Provider", "Request acknowledgement timed out for '%s', retrying %d/%d!", sentRequest.m_pRequest->GetResourceID().c_str(), sentRequest.m_retryCount, NetworkMessageSettings::s_maxRequestTimeoutRetries );
                        retryRequest.m_resourceIDs.emplace_back( sentRequest.m_pRequest->GetResourceID() );
                        sentRequest.m_timeSinceLastHeartbeat.Reset();
                    }
                    else // Fail
                    {
                        EE_LOG_ERROR( LogCategory::Resource, "Network Provider", "Request acknowledgement timed out for '%s', max retries exceeded!", sentRequest.m_pRequest->GetResourceID().c_str() );
                        NetworkResourceResponse::Result& result = m_serverResults.emplace_back();
                        result.m_resourceID = sentRequest.m_pRequest->GetResourceID();
                        result.m_log = ( sentRequest.m_state == SentRequest::State::Requested ) ? "Request Acknowledgment Timed Out" : "Request Timed Out";
                    }
                }
            }

            // Send any retry-requests
            if ( !retryRequest.m_resourceIDs.empty() )
            {
                Network::Message requestResourceMessage( (int32_t) NetworkMessageID::RequestResource, retryRequest );
                m_networkClient.SendNetworkMessage( eastl::move( requestResourceMessage ) );
            }
        }

        // Deserialization task
        //-------------------------------------------------------------------------

        if ( m_deserializationTask.GetIsComplete() )
        {
            m_serverResults.insert( m_serverResults.end(), m_deserializedServerResults.begin(), m_deserializedServerResults.end() );
            m_deserializedServerResults.clear();

            m_serverHeartbeats.insert( m_serverHeartbeats.end(), m_deserializedServerHeartbeats.begin(), m_deserializedServerHeartbeats.end() );
            m_deserializedServerHeartbeats.clear();

            m_externallyUpdatedResources.insert( m_externallyUpdatedResources.end(), m_deserializedExternalResourceUpdates.begin(), m_deserializedExternalResourceUpdates.end() );
            m_deserializedExternalResourceUpdates.clear();

            // Kick off new task
            m_taskSystem.ScheduleTask( &m_deserializationTask );
        }

        // Process all server results
        //-------------------------------------------------------------------------

        for ( auto& result : m_serverResults )
        {
            auto foundIter = VectorFind( m_sentRequests, result.m_resourceID );

            // This might have been a canceled request
            if ( foundIter == m_sentRequests.end() )
            {
                continue;
            }

            ResourceRequest* pFoundRequest = static_cast<SentRequest*>( foundIter )->m_pRequest;
            EE_ASSERT( pFoundRequest->IsValid() );

            // Ignore any responses for requests that may have been canceled or are unloading
            if ( pFoundRequest->GetLoadingStatus() != LoadingStatus::Loading )
            {
                continue;
            }

            // Trigger tools notification on failure
            if ( result.m_filePath.empty() )
            {
                ImGuiX::NotifyError( "%s failed to Compile!\n\nLog: %s", result.m_resourceID.c_str(), result.m_log.c_str() );
            }

            // If the request has a filepath set, the compilation was a success
            pFoundRequest->OnRawResourceRequestComplete( result.m_filePath, result.m_log );

            // Remove from request list
            m_sentRequests.erase_unsorted( foundIter );
        }

        m_serverResults.clear();
    }

    void NetworkResourceProvider::DeserializeReceivedMessages()
    {
        Timer<PlatformClock> timer;
        Network::Message* pMessage = nullptr;
        while ( m_unserializedMessages.try_dequeue( pMessage ) )
        {
            EE_ASSERT( pMessage != nullptr );

            switch ( (NetworkMessageID) pMessage->GetMessageID() )
            {
                case NetworkMessageID::ResourceRequestHeartbeat:
                {
                    NetworkResourceResponse response = pMessage->GetPayload<NetworkResourceResponse>();
                    EE_ASSERT( !response.m_results.empty() );
                    m_deserializedServerHeartbeats.insert( m_deserializedServerHeartbeats.end(), response.m_results.begin(), response.m_results.end() );
                }
                break;

                case NetworkMessageID::ResourceRequestComplete:
                {
                    NetworkResourceResponse response = pMessage->GetPayload<NetworkResourceResponse>();
                    EE_ASSERT( !response.m_results.empty() );
                    m_deserializedServerResults.insert( m_deserializedServerResults.end(), response.m_results.begin(), response.m_results.end() );
                }
                break;

                case NetworkMessageID::ResourceUpdated:
                {
                    NetworkResourceResponse response = pMessage->GetPayload<NetworkResourceResponse>();
                    EE_ASSERT( !response.m_results.empty() );

                    for ( auto const& result : response.m_results )
                    {
                        m_deserializedExternalResourceUpdates.emplace_back( result.m_resourceID );
                    }
                }
                break;

                default:
                break;
            };

            EE::Delete( pMessage );

            //-------------------------------------------------------------------------

            if ( timer.GetElapsedTimeMilliseconds() > 16 )
            {
                return;
            }
        }
    }
}
#endif