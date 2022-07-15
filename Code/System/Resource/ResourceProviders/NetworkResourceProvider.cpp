#include "NetworkResourceProvider.h"
#include "System/Resource/ResourceRequest.h"
#include "System/Resource/ResourceSettings.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Profiling.h"
#include "System/Log.h"

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
            EE_LOG_ERROR( "Resource Provider", "Failed to connect to the resource server (%s)!", m_address.c_str() );
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
        auto foundIter = VectorFind( m_requests, pRequest->GetResourceID(), predicate );
        EE_ASSERT( foundIter == m_requests.end() );
        #endif

        //-------------------------------------------------------------------------

        m_messagesToSend.enqueue( Network::IPC::Message( (int32_t) NetworkMessageID::RequestResource, NetworkResourceRequest( pRequest->GetResourceID() ) ) );
        m_requests.emplace_back( pRequest );
    }

    void NetworkResourceProvider::CancelRequest( ResourceRequest* pRequest )
    {
        EE_ASSERT( pRequest != nullptr && pRequest->IsValid() );

        auto foundIter = VectorFind( m_requests, pRequest );
        EE_ASSERT( foundIter != m_requests.end() );

        m_requests.erase_unsorted( foundIter );
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
                EE_LOG_FATAL_ERROR( "Resource", "Lost connection to resource server" );
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
                    m_serverReponses.emplace_back( message.GetData<NetworkResourceResponse>() );
                }
                break;

                case NetworkMessageID::ResourceUpdated:
                {
                    #if EE_DEVELOPMENT_TOOLS
                    NetworkResourceResponse response = message.GetData<NetworkResourceResponse>();
                    m_externallyUpdatedResources.emplace_back( response.m_resourceID );
                    #endif
                }
                break;
            };
        };

        m_networkClient.ProcessIncomingMessages( ProcessMessageFunction );

        // Send all requests and keep-alive messages
        //-------------------------------------------------------------------------

        Network::IPC::Message messageToSend;
        while ( m_messagesToSend.try_dequeue( messageToSend ) )
        {
            m_networkClient.SendMessageToServer( eastl::move( messageToSend ) );
        }

        // Process all responses
        //-------------------------------------------------------------------------

        for ( auto& response : m_serverReponses )
        {
            auto predicate = [] ( ResourceRequest* pRequest, ResourceID const& resourceID ) { return pRequest->GetResourceID() == resourceID; };
            auto foundIter = VectorFind( m_requests, response.m_resourceID, predicate );

            // This might have been a canceled request
            if ( foundIter == m_requests.end() )
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
            pFoundRequest->OnRawResourceRequestComplete( response.m_filePath );

            // Remove from request list
            m_requests.erase_unsorted( foundIter );
        }

        m_serverReponses.clear();
    }
}
#endif