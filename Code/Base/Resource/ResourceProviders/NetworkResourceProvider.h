#pragma once

#include "ResourceNetworkMessages.h"
#include "Base/Resource/ResourceProvider.h"
#include "Base/Network/IPC/IPCMessageClient.h"
#include "Base/Time/Timers.h"
#include "Base/Threading/Threading.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Resource
{
    class ResourceGlobalSettings;

    //-------------------------------------------------------------------------

    class EE_BASE_API NetworkResourceProvider final : public ResourceProvider
    {

    public:

        NetworkResourceProvider( ResourceGlobalSettings const& settings ) : ResourceProvider( settings ) {}
        virtual bool IsReady() const override final;

    private:

        virtual bool Initialize() override final;
        virtual void Shutdown() override final;
        virtual void Update() override final;

        virtual void RequestRawResource( ResourceRequest* pRequest ) override;
        virtual void CancelRequest( ResourceRequest* pRequest ) override;

        virtual TVector<ResourceID> const& GetExternallyUpdatedResources() const override { return m_externallyUpdatedResources; }

    private:

        Network::IPC::Client                                m_networkClient;
        String                                              m_address;
        TVector<NetworkResourceResponse::Result>            m_serverResults;
        bool                                                m_networkFailureDetected = false;

        TVector<ResourceRequest*>                           m_pendingRequests; // Requests we need to still send
        TVector<ResourceRequest*>                           m_sentRequests; // Request that were sent but we're still waiting for a response

        TVector<ResourceID>                                 m_externallyUpdatedResources;
    };
}
#endif