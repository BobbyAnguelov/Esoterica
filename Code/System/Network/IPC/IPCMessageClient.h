#pragma once

#include "System/_Module/API.h"
#include "IPCMessage.h"
#include "System/Network/NetworkSystem.h"

//-------------------------------------------------------------------------

namespace EE::Network::IPC
{
    class EE_SYSTEM_API Client final : public ClientConnection
    {
    public:

        // Queues a message to be sent to the server. Note this is a destructive operation!! This call will move the data 
        void SendMessageToServer( Message&& message );

        // Iterates over all incoming messages and calls the processing function
        void ProcessIncomingMessages( TFunction<void( Message const& message )> messageProcessorFunction );

    private:

        virtual void ProcessMessage( void* pData, size_t size ) override;
        virtual void SendMessages( TFunction<void( void*, uint32_t )> const& sendFunction ) override;

    protected:

        TVector<Message>                        m_incomingMessages;
        TVector<Message>                        m_outgoingMessages;
    };
}