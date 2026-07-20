#include "NetworkClient.h"

//-------------------------------------------------------------------------

namespace EE::Network
{
    Client::Client()
        : m_receivedMessages( 128 * Threading::TLockFreeQueue<Message*>::BLOCK_SIZE )
    {}

    void Client::Update( TFunction<void( Message const& )> const& handlerFunc )
    {
        EE_ASSERT( Threading::IsMainThread() );

        // Update connection status and receive network messages
        //-------------------------------------------------------------------------

        UpdateConnectionAndReceiveMessages();

        // Handle Incoming Messages
        //-------------------------------------------------------------------------

        Message* pMessage = nullptr;
        while ( m_receivedMessages.try_dequeue( pMessage ) )
        {
            EE_ASSERT( pMessage != nullptr );

            if ( pMessage->GetMessageID() == Message::HeartbeatID )
            {
                SendNetworkMessage( Message( Message::HeartbeatID ) );
            }
            else if ( handlerFunc != nullptr )
            {
                handlerFunc( *pMessage );
            }

            EE::Delete( pMessage );

            EE_ASSERT( m_receivedMessages.size_approx() < 1000 );
        }

        // Send Outgoing Messages
        //-------------------------------------------------------------------------

        if ( IsConnected() )
        {
           SendOutgoingMessages();
        }
    }
}