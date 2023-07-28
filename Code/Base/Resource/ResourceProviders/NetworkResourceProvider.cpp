#include "NetworkResourceProvider.h"
#include "Base/Resource/ResourceRequest.h"
#include "Base/Resource/ResourceSettings.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Profiling.h"


//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Resource
{
    bool NetworkResourceProvider::IsReady() const
    {
        return m_networkClient.IsConnected() || m_networkClient.IsConnecting();
    }

    bool NetworkResourceProvider::Initialize()
    {
        m_address = String( String::CtorSprintf(), "%s:%d", m_settings.m_resourceServerNetworkAddress.c_str(), m_settings.m_resourceServerPort );
        if ( !Network::NetworkSystem::StartClientConnection( &m_networkClient, m_address.c_str() ) )
        {
            EE_LOG_ERROR( "Resource", "Resource Provider", "Failed to connect to the resource server (%s)!", m_address.c_str() );
            return false;
        }

        return true;
    }

    void NetworkResourceProvider::Shutdown()
    {
        Network::NetworkSystem::StopClientConnection( &m_networkClient );
    }

    void NetworkResourceProvider::RequestRawResource( ResourceRequest* pRequest )
    {
        EE_ASSERT( pRequest != nullptr && pRequest->IsValid() && pRequest->GetLoadingStatus() == LoadingStatus::Loading );

        #if EE_DEVELOPMENT_TOOLS
        auto predicate = [] ( ResourceRequest* pRequest, ResourceID const& resourceID ) { return pRequest->GetResourceID() == resourceID; };
        auto foundIter = VectorFind( m_sentRequests, pRequest->GetResourceID(), predicate );
        EE_ASSERT( foundIter == m_sentRequests.end() );
        #endif

        //-------------------------------------------------------------------------

        m_pendingRequests.emplace_back( pRequest );
    }

    void NetworkResourceProvider::CancelRequest( ResourceRequest* pRequest )
    {
        EE_ASSERT( pRequest != nullptr && pRequest->IsValid() );

        auto foundIter = VectorFind( m_sentRequests, pRequest );
        EE_ASSERT( foundIter != m_sentRequests.end() );

        m_sentRequests.erase_unsorted( foundIter );
    }

    void NetworkResourceProvider::Update()
    {
        EE_PROFILE_FUNCTION_RESOURCE();

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        m_externallyUpdatedResources.clear();
        #endif

        // Check connection to resource server
        //-------------------------------------------------------------------------

        if ( m_networkClient.IsConnecting() )
        {
            return;
        }

        if ( m_networkClient.HasConnectionFailed() )
        {
            if ( !m_networkFailureDetected )
            {
                EE_LOG_FATAL_ERROR( "Resource", "Network Resource Provider", "Lost connection to resource server" );
                m_networkFailureDetected = true;
            }
            return;
        }

        // Check for any network messages
        //-------------------------------------------------------------------------

        auto ProcessMessageFunction = [this] ( Network::IPC::Message const& message )
        {
            switch ( (NetworkMessageID) message.GetMessageID() )
            {
                case NetworkMessageID::ResourceRequestComplete:
                {
                    NetworkResourceResponse response = message.GetData<NetworkResourceResponse>();
                    m_serverResults.insert( m_serverResults.end(), response.m_results.begin(), response.m_results.end() );
                }
                break;

                case NetworkMessageID::ResourceUpdated:
                {
                    #if EE_DEVELOPMENT_TOOLS
                    NetworkResourceResponse response = message.GetData<NetworkResourceResponse>();
                    for ( auto const& result : response.m_results )
                    {
                        m_externallyUpdatedResources.emplace_back( result.m_resourceID );
                    }
                    #endif
                }
                break;

                default:
                break;
            };
        };

        m_networkClient.ProcessIncomingMessages( ProcessMessageFunction );

        // Send all requests and keep-alive messages
        //-------------------------------------------------------------------------

        if ( !m_pendingRequests.empty() )
        {
            NetworkResourceRequest request;

            // Process all pending requests
            for( auto pRequest : m_pendingRequests )
            {
                request.m_resourceIDs.emplace_back( pRequest->GetResourceID() );
                m_sentRequests.emplace_back( pRequest );

                // Try to limit the size of the network messages so we limit each message to 128 request
                if ( request.m_resourceIDs.size() == 128 )
                {
                    Network::IPC::Message requestResourceMessage( (int32_t) NetworkMessageID::RequestResource, request );
                    m_networkClient.SendMessageToServer( eastl::move( requestResourceMessage ) );
                    request.m_resourceIDs.clear();
                }
            }
            m_pendingRequests.clear();

            // Send any remaining requests
            if ( request.m_resourceIDs.size() > 0 )
            {
                Network::IPC::Message requestResourceMessage( (int32_t) NetworkMessageID::RequestResource, request );
                m_networkClient.SendMessageToServer( eastl::move( requestResourceMessage ) );
            }
        }

        // Process all server results
        //-------------------------------------------------------------------------

        for ( auto& result : m_serverResults )
        {
            auto predicate = [] ( ResourceRequest* pRequest, ResourceID const& resourceID ) { return pRequest->GetResourceID() == resourceID; };
            auto foundIter = VectorFind( m_sentRequests, result.m_resourceID, predicate );

            // This might have been a canceled request
            if ( foundIter == m_sentRequests.end() )
            {
                continue;
            }

            ResourceRequest* pFoundRequest = static_cast<ResourceRequest*>( *foundIter );
            EE_ASSERT( pFoundRequest->IsValid() );

            // Ignore any responses for requests that may have been canceled or are unloading
            if ( pFoundRequest->GetLoadingStatus() != LoadingStatus::Loading )
            {
                continue;
            }

            // If the request has a filepath set, the compilation was a success
            pFoundRequest->OnRawResourceRequestComplete( result.m_filePath );

            // Remove from request list
            m_sentRequests.erase_unsorted( foundIter );
        }

        m_serverResults.clear();
    }
}
#endif