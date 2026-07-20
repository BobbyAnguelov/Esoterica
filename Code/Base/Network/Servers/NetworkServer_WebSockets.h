#pragma once

#include "NetworkServer.h"
#include "Base/Threading/Threading.h"
#include "Base/Types/HashMap.h"
#include <vector>

//-------------------------------------------------------------------------

namespace ix 
{
    class WebSocketServer;
    class WebSocket;
    enum class WebSocketMessageType : int32_t;
}

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Network
{
    class EE_BASE_API Server_WS final : public Server
    {
    public:

        virtual ~Server_WS();
        virtual bool IsRunning() const override;
        virtual bool Start( uint16_t port ) override;
        virtual void Stop() override;
        virtual void SendOutgoingMessages() override;

    private:

        virtual void ConnectClient( uint64_t clientID, char const* pAddress, void* pUserData ) override;
        virtual void DisconnectClient( uint64_t clientID ) override;
        virtual void UpdateConnectionAndReceiveMessages() override {}

    private:

        ix::WebSocketServer*                        m_pServer = nullptr;
        THashMap<uint64_t, ix::WebSocket*>          m_clientSocketMap;
    };
}
#endif