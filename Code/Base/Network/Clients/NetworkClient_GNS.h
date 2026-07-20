#pragma once
#include "NetworkClient.h"

//-------------------------------------------------------------------------

struct SteamNetConnectionStatusChangedCallback_t;

//-------------------------------------------------------------------------

namespace EE::Network
{
    using AddressString = TInlineString<30>;

    //-------------------------------------------------------------------------

    class EE_BASE_API Client_GNS : public Client
    {
        static void ConnectionChangedCallback( SteamNetConnectionStatusChangedCallback_t* pInfo );

    public:

        Client_GNS() = default;
        Client_GNS( Client_GNS const& ) = default;
        virtual ~Client_GNS();

        virtual bool Start( char const* pAddress, uint16_t port ) override;
        virtual void Stop() override;

    private:

        bool TryConnect();
        virtual void UpdateConnectionAndReceiveMessages() override;
        virtual void SendOutgoingMessages() override;

    private:

        uint32_t                                        m_connectionHandle = 0;
    };
}
