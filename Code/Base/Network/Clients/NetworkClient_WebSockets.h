#pragma once
#include "NetworkClient.h"
#include "Base/Threading/Threading.h"
#include <vector>

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace ix
{
    class WebSocket;
    enum class WebSocketMessageType : int32_t;
}

//-------------------------------------------------------------------------

namespace EE::Network
{
    class EE_BASE_API Client_WS final : public Client
    {

    public:

        virtual ~Client_WS();
        virtual bool Start( char const* pAddress, uint16_t port ) override;
        virtual void Stop() override;
        virtual void SendOutgoingMessages() override;

    private:

        virtual void UpdateConnectionAndReceiveMessages() override
        {
            // Nothing to do, messages are received and pumped async on another thread
        }

    private:

        ix::WebSocket*                  m_pSocket = nullptr;
    };
}
#endif