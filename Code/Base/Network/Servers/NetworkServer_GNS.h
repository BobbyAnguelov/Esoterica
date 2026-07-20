#pragma once

#include "NetworkServer.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------

struct SteamNetConnectionStatusChangedCallback_t;

//-------------------------------------------------------------------------

namespace EE::Network
{
    class EE_BASE_API Server_GNS : public Server
    {
        static void ConnectionChangedCallback( SteamNetConnectionStatusChangedCallback_t* pInfo );

    public:

        virtual ~Server_GNS();
        virtual bool Start( uint16_t port ) override;
        virtual void Stop() override;

        Server_GNS& operator=( Server_GNS const& ) = delete;

        // Server Info
        //-------------------------------------------------------------------------

        virtual bool IsRunning() const override { return m_socketHandle != 0 && m_pollingGroupHandle != 0; }

    private:

        virtual void UpdateConnectionAndReceiveMessages() override;
        virtual void SendOutgoingMessages() override;

    protected:

        uint32_t                                        m_socketHandle = 0;
        uint32_t                                        m_pollingGroupHandle = 0;
    };
}