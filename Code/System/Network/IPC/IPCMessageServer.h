#pragma once

#include "IPCMessage.h"
#include "System/Network/NetworkSystem.h"

//-------------------------------------------------------------------------

namespace EE::Network::IPC
{
    class EE_SYSTEM_API Server final : public ServerConnection
    {

    public:

        // Queues a message to be sent. Note this is a destructive operation!! This call will move the data
        void SendNetworkMessage( Message&& message );

        // Iterates over all incoming messages and calls the processing function
        void ProcessIncomingMessages( TFunction<void( Message const& message )> messageProcessorFunction );

    private:

        virtual void ProcessMessage( uint32_t connectionID, void* pData, size_t size ) override;
        virtual void SendMessages( TFunction<void( ServerConnection::ClientConnectionID, void*, uint32_t )> const& sendFunction ) override;

    private:

        TVector<Message>            m_incomingMessages;
        TVector<Message>            m_outgoingMessages;
    };
}
