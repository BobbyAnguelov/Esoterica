#pragma once

#include "ResourceNetworkMessages.h"
#include "Base/Resource/ResourceProvider.h"
#include "Base/Resource/ResourceRequest.h"
#include "Base/Time/Timers.h"
#include "Base/Network/Clients/NetworkClient_WebSockets.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Resource
{
    class ResourceSettings;

    //-------------------------------------------------------------------------

    class EE_BASE_API NetworkResourceProvider final : public ResourceProvider
    {
        struct SentRequest
        {
            enum class State : uint8_t
            {
                Requested,  // Requested and sent but we havent received a heartbeat back
                Acknowledged // Requested and sent and we've received a heartbeat back
            };

            inline bool operator==( ResourceRequest* const& pRequest ) const
            {
                return m_pRequest == pRequest;
            }

            inline bool operator==( ResourceID const& resourceID ) const
            {
                return m_pRequest->GetResourceID() == resourceID;
            }

            inline Seconds GetTimeSinceLastHeartbeat() const { return m_timeSinceLastHeartbeat.GetElapsedTimeSeconds(); }

        public:

            ResourceRequest*                                m_pRequest = nullptr;
            Timer<PlatformClock>                            m_timeSinceLastHeartbeat;
            int8_t                                          m_retryCount = 0;
            State                                           m_state = State::Requested;
        };

        friend class ResourceDebugView;

    public:

        NetworkResourceProvider( ResourceSettings const& settings ) : ResourceProvider( settings ) {}
        virtual bool IsReady() const override final;
        virtual bool IsConnecting() const override final;

    private:

        virtual bool Initialize() override final;
        virtual void Shutdown() override final;
        virtual void Update() override final;

        virtual void RequestRawResource( ResourceRequest* pRequest ) override;
        virtual void CancelRequest( ResourceRequest* pRequest ) override;

        virtual TVector<ResourceID> const& GetExternallyUpdatedResources() const override { return m_externallyUpdatedResources; }

    private:

        Network::Client_WS                                  m_networkClient;
        TVector<NetworkResourceResponse::Result>            m_serverResults;
        TVector<NetworkResourceResponse::Result>            m_serverHeartbeats;

        TVector<ResourceRequest*>                           m_pendingRequests; // Requests we need to still send
        TVector<SentRequest>                                m_sentRequests; // Request that were sent but we're still waiting for a response

        TVector<ResourceID>                                 m_externallyUpdatedResources;
        Threading::Mutex                                    m_requestModificationMutex;
    };
}
#endif